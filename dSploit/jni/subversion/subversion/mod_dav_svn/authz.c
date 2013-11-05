/*
 * authz.c: authorization related code
 *
 * ====================================================================
 *    Licensed to the Apache Software Foundation (ASF) under one
 *    or more contributor license agreements.  See the NOTICE file
 *    distributed with this work for additional information
 *    regarding copyright ownership.  The ASF licenses this file
 *    to you under the Apache License, Version 2.0 (the
 *    "License"); you may not use this file except in compliance
 *    with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing,
 *    software distributed under the License is distributed on an
 *    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *    KIND, either express or implied.  See the License for the
 *    specific language governing permissions and limitations
 *    under the License.
 * ====================================================================
 */

#include <http_request.h>
#include <http_log.h>

#include "svn_pools.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"

#include "private/svn_fspath.h"

#include "mod_authz_svn.h"
#include "dav_svn.h"


svn_boolean_t
dav_svn__allow_read(request_rec *r,
                    const dav_svn_repos *repos,
                    const char *path,
                    svn_revnum_t rev,
                    apr_pool_t *pool)
{
  const char *uri;
  request_rec *subreq;
  enum dav_svn__build_what uri_type;
  svn_boolean_t allowed = FALSE;
  authz_svn__subreq_bypass_func_t allow_read_bypass = NULL;

  /* Easy out:  if the admin has explicitly set 'SVNPathAuthz Off',
     then this whole callback does nothing. */
  if (! dav_svn__get_pathauthz_flag(r))
    {
      return TRUE;
    }

  /* Sometimes we get paths that do not start with '/' and
     hence below uri concatenation would lead to wrong uris .*/
  if (path && path[0] != '/')
    path = apr_pstrcat(pool, "/", path, NULL);

  /* If bypass is specified and authz has exported the provider.
     Otherwise, we fall through to the full version.  This should be
     safer than allowing or disallowing all accesses if there is a
     configuration error.
     XXX: Is this the proper thing to do in this case? */
  allow_read_bypass = dav_svn__get_pathauthz_bypass(r);
  if (allow_read_bypass != NULL)
    {
      if (allow_read_bypass(r, path, repos->repo_basename) == OK)
        return TRUE;
      else
        return FALSE;
    }

  /* If no revnum is specified, assume HEAD. */
  if (SVN_IS_VALID_REVNUM(rev))
    uri_type = DAV_SVN__BUILD_URI_VERSION;
  else
    uri_type = DAV_SVN__BUILD_URI_PUBLIC;

  /* Build a Version Resource uri representing (rev, path). */
  uri = dav_svn__build_uri(repos, uri_type, rev, path, FALSE, pool);

  /* Check if GET would work against this uri. */
  subreq = ap_sub_req_method_uri("GET", uri, r, r->output_filters);

  if (subreq)
    {
      if (subreq->status == HTTP_OK)
        allowed = TRUE;

      ap_destroy_sub_req(subreq);
    }

  return allowed;
}


/* This function implements 'svn_repos_authz_func_t', specifically
   for read authorization.

   Convert incoming ROOT and PATH into a version-resource URI and
   perform a GET subrequest on it.  This will invoke any authz modules
   loaded into apache.  Set *ALLOWED to TRUE if the subrequest
   succeeds, FALSE otherwise.

   BATON must be a pointer to a dav_svn__authz_read_baton.
   Use POOL for for any temporary allocation.
*/
static svn_error_t *
authz_read(svn_boolean_t *allowed,
           svn_fs_root_t *root,
           const char *path,
           void *baton,
           apr_pool_t *pool)
{
  dav_svn__authz_read_baton *arb = baton;
  svn_revnum_t rev = SVN_INVALID_REVNUM;
  const char *revpath = NULL;

  /* Our ultimate goal here is to create a Version Resource (VR) url,
     which is a url that represents a path within a revision.  We then
     send a subrequest to apache, so that any installed authz modules
     can allow/disallow the path.

     ### That means that we're assuming that any installed authz
     module is *only* paying attention to revision-paths, not paths in
     uncommitted transactions.  Someday we need to widen our horizons. */

  if (svn_fs_is_txn_root(root))
    {
      /* This means svn_repos_dir_delta2 is comparing two txn trees,
         rather than a txn and revision.  It's probably updating a
         working copy that contains 'disjoint urls'.

         Because the 2nd transaction is likely to have all sorts of
         paths linked in from random places, we need to find the
         original (rev,path) of each txn path.  That's what needs
         authorization.  */

      svn_stringbuf_t *path_s = svn_stringbuf_create(path, pool);
      const char *lopped_path = "";

      /* The path might be copied implicitly, because it's down in a
         copied tree.  So we start at path and walk up its parents
         asking if anyone was copied, and if so where from.  */
      while (! (svn_path_is_empty(path_s->data)
                || svn_fspath__is_root(path_s->data, path_s->len)))
        {
          SVN_ERR(svn_fs_copied_from(&rev, &revpath, root,
                                     path_s->data, pool));

          if (SVN_IS_VALID_REVNUM(rev) && revpath)
            {
              revpath = svn_fspath__join(revpath, lopped_path, pool);
              break;
            }

          /* Lop off the basename and try again. */
          lopped_path = svn_relpath_join(svn_fspath__basename(path_s->data,
                                                              pool),
                                         lopped_path, pool);
          svn_path_remove_component(path_s);
        }

      /* If no copy produced this path, its path in the original
         revision is the same as its path in this txn. */
      if ((rev == SVN_INVALID_REVNUM) && (revpath == NULL))
        {
          rev = svn_fs_txn_root_base_revision(root);
          revpath = path;
        }
    }
  else  /* revision root */
    {
      rev = svn_fs_revision_root_revision(root);
      revpath = path;
    }

  /* We have a (rev, path) pair to check authorization on. */
  *allowed = dav_svn__allow_read(arb->r, arb->repos, revpath, rev, pool);

  return SVN_NO_ERROR;
}


svn_repos_authz_func_t
dav_svn__authz_read_func(dav_svn__authz_read_baton *baton)
{
  /* Easy out: If the admin has explicitly set 'SVNPathAuthz Off',
     then we don't need to do any authorization checks. */
  if (! dav_svn__get_pathauthz_flag(baton->r))
    return NULL;

  return authz_read;
}


svn_boolean_t
dav_svn__allow_read_resource(const dav_resource *resource,
                             svn_revnum_t rev,
                             apr_pool_t *pool)
{
  return dav_svn__allow_read(resource->info->r, resource->info->repos,
                             resource->info->repos_path, rev, pool);
}

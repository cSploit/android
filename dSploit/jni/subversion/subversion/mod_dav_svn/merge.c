/*
 * merge.c: handle the MERGE response processing
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

#include <apr_pools.h>
#include <apr_buckets.h>
#include <apr_xml.h>
#include <apr_hash.h>

#include <httpd.h>
#include <util_filter.h>

#include "svn_pools.h"
#include "svn_fs.h"
#include "svn_props.h"
#include "svn_xml.h"

#include "dav_svn.h"

#include "private/svn_fspath.h"

/* #################################################################

   These functions are currently *VERY* SVN specific.

   * we don't check prop_elem for what the client requested
   * we presume a baseline was checked out into the activity, and is
     part of the MERGE
   * we presume that all "changed" files/dirs were checked out into
     the activity and are part of the MERGE
     (not sure if this is SVN specific; I can't see how a file/dir
      would be part of the new revision if a working resource had
      not been created for it)
   * we return some props for some resources, and a different set for
     other resources (to keep the wire smaller for now)

   At some point in the future, we'll want to make this "real". Especially
   for proper interoperability.

   #################################################################
*/



/* -------------------------------------------------------------------------
   PRIVATE HELPER FUNCTIONS
*/

/* send a response to the client for this baton */
static svn_error_t *
send_response(const dav_svn_repos *repos,
              svn_fs_root_t *root,
              const char *path,
              svn_boolean_t is_dir,
              ap_filter_t *output,
              apr_bucket_brigade *bb,
              apr_pool_t *pool)
{
  const char *href;
  const char *vsn_url;
  apr_status_t status;
  svn_revnum_t rev_to_use;

  href = dav_svn__build_uri(repos, DAV_SVN__BUILD_URI_PUBLIC,
                            SVN_IGNORED_REVNUM, path, 0 /* add_href */, pool);
  rev_to_use = dav_svn__get_safe_cr(root, path, pool);
  vsn_url = dav_svn__build_uri(repos, DAV_SVN__BUILD_URI_VERSION,
                               rev_to_use, path, 0 /* add_href */, pool);
  status = ap_fputstrs(output, bb,
                       "<D:response>" DEBUG_CR
                       "<D:href>",
                       apr_xml_quote_string(pool, href, 1),
                       "</D:href>" DEBUG_CR
                       "<D:propstat><D:prop>" DEBUG_CR,
                       is_dir
                         ? "<D:resourcetype><D:collection/></D:resourcetype>"
                         : "<D:resourcetype/>",
                       DEBUG_CR,
                       "<D:checked-in><D:href>",
                       apr_xml_quote_string(pool, vsn_url, 1),
                       "</D:href></D:checked-in>" DEBUG_CR
                       "</D:prop>" DEBUG_CR
                       "<D:status>HTTP/1.1 200 OK</D:status>" DEBUG_CR
                       "</D:propstat>" DEBUG_CR
                       "</D:response>" DEBUG_CR,
                       NULL);
  if (status != APR_SUCCESS)
    return svn_error_wrap_apr(status, "Can't write response to output");

  return SVN_NO_ERROR;
}


static svn_error_t *
do_resources(const dav_svn_repos *repos,
             svn_fs_root_t *root,
             svn_revnum_t revision,
             ap_filter_t *output,
             apr_bucket_brigade *bb,
             apr_pool_t *pool)
{
  apr_hash_t *changes;
  apr_hash_t *sent = apr_hash_make(pool);
  apr_hash_index_t *hi;
  apr_pool_t *subpool = svn_pool_create(pool);

  /* Fetch the paths changed in this revision.  This will contain
     everything except otherwise-unchanged parent directories of added
     and deleted things.  Also, note that deleted things don't merit
     responses of their own -- they are considered modifications to
     their parent.  */
  SVN_ERR(svn_fs_paths_changed2(&changes, root, pool));

  for (hi = apr_hash_first(pool, changes); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      void *val;
      const char *path;
      svn_fs_path_change2_t *change;
      svn_boolean_t send_self;
      svn_boolean_t send_parent;

      svn_pool_clear(subpool);
      apr_hash_this(hi, &key, NULL, &val);
      path = key;
      change = val;

      /* Figure out who needs to get sent. */
      switch (change->change_kind)
        {
        case svn_fs_path_change_delete:
          send_self = FALSE;
          send_parent = TRUE;
          break;

        case svn_fs_path_change_add:
        case svn_fs_path_change_replace:
          send_self = TRUE;
          send_parent = TRUE;
          break;

        case svn_fs_path_change_modify:
        default:
          send_self = TRUE;
          send_parent = FALSE;
          break;
        }

      if (send_self)
        {
          /* If we haven't already sent this path, send it (and then
             remember that we sent it). */
          if (! apr_hash_get(sent, path, APR_HASH_KEY_STRING))
            {
              svn_node_kind_t kind;
              SVN_ERR(svn_fs_check_path(&kind, root, path, subpool));
              SVN_ERR(send_response(repos, root, path,
                                    kind == svn_node_dir,
                                    output, bb, subpool));
              apr_hash_set(sent, path, APR_HASH_KEY_STRING, (void *)1);
            }
        }
      if (send_parent)
        {
          /* If it hasn't already been sent, send the parent directory
             (and then remember that you sent it).  Allocate parent in
             pool, not subpool, because it stays in the sent hash
             afterwards. */
          const char *parent = svn_fspath__dirname(path, pool);
          if (! apr_hash_get(sent, parent, APR_HASH_KEY_STRING))
            {
              SVN_ERR(send_response(repos, root, parent,
                                    TRUE, output, bb, subpool));
              apr_hash_set(sent, parent, APR_HASH_KEY_STRING, (void *)1);
            }
        }
    }

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}



/* -------------------------------------------------------------------------
   PUBLIC FUNCTIONS
*/

dav_error *
dav_svn__merge_response(ap_filter_t *output,
                        const dav_svn_repos *repos,
                        svn_revnum_t new_rev,
                        const char *post_commit_err,
                        apr_xml_elem *prop_elem,
                        svn_boolean_t disable_merge_response,
                        apr_pool_t *pool)
{
  apr_bucket_brigade *bb;
  svn_fs_root_t *root;
  svn_error_t *serr;
  const char *vcc;
  const char *rev;
  svn_string_t *creationdate, *creator_displayname;
  const char *post_commit_err_elem = NULL,
             *post_commit_header_info = NULL;

  serr = svn_fs_revision_root(&root, repos->fs, new_rev, pool);
  if (serr != NULL)
    {
      return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                  "Could not open the FS root for the "
                                  "revision just committed.",
                                  repos->pool);
    }

  bb = apr_brigade_create(pool, output->c->bucket_alloc);

  /* prep some strings */

  /* the HREF for the baseline is actually the VCC */
  vcc = dav_svn__build_uri(repos, DAV_SVN__BUILD_URI_VCC, SVN_IGNORED_REVNUM,
                           NULL, 0 /* add_href */, pool);

  /* the version-name of the baseline is the revision number */
  rev = apr_psprintf(pool, "%ld", new_rev);

  /* get the post-commit hook stderr, if any */
  if (post_commit_err)
    {
      post_commit_header_info = apr_psprintf(pool,
                                             " xmlns:S=\"%s\"",
                                             SVN_XML_NAMESPACE);
      post_commit_err_elem = apr_psprintf(pool,
                                          "<S:post-commit-err>%s"
                                          "</S:post-commit-err>",
                                          apr_xml_quote_string(pool,
                                                               post_commit_err,
                                                               0));
    }
  else
    {
      post_commit_header_info = "" ;
      post_commit_err_elem = "" ;
    }


  /* get the creationdate and creator-displayname of the new revision, too. */
  serr = svn_fs_revision_prop(&creationdate, repos->fs, new_rev,
                              SVN_PROP_REVISION_DATE, pool);
  if (serr != NULL)
    {
      return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                  "Could not get date of newest revision",
                                  repos->pool);
    }
  serr = svn_fs_revision_prop(&creator_displayname, repos->fs, new_rev,
                              SVN_PROP_REVISION_AUTHOR, pool);
  if (serr != NULL)
    {
      return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                  "Could not get author of newest revision",
                                  repos->pool);
    }


  (void) ap_fputstrs(output, bb,
                     DAV_XML_HEADER DEBUG_CR
                     "<D:merge-response xmlns:D=\"DAV:\"",
                     post_commit_header_info,
                     ">" DEBUG_CR
                     "<D:updated-set>" DEBUG_CR

                     /* generate a response for the new baseline */
                     "<D:response>" DEBUG_CR
                     "<D:href>",
                     apr_xml_quote_string(pool, vcc, 1),
                     "</D:href>" DEBUG_CR
                     "<D:propstat><D:prop>" DEBUG_CR
                     /* ### this is wrong. it's a VCC, not a baseline. but
                        ### we need to tell the client to look at *this*
                        ### resource for the version-name. */
                     "<D:resourcetype><D:baseline/></D:resourcetype>" DEBUG_CR,
                     post_commit_err_elem, DEBUG_CR
                     "<D:version-name>", rev, "</D:version-name>" DEBUG_CR,
                     NULL);
  if (creationdate)
    {
      (void) ap_fputstrs(output, bb,
                         "<D:creationdate>",
                         apr_xml_quote_string(pool, creationdate->data, 1),
                         "</D:creationdate>" DEBUG_CR,
                         NULL);
    }
  if (creator_displayname)
    {
      (void) ap_fputstrs(output, bb,
                         "<D:creator-displayname>",
                         apr_xml_quote_string(pool,
                                              creator_displayname->data, 1),
                         "</D:creator-displayname>" DEBUG_CR,
                         NULL);
    }
  (void) ap_fputstrs(output, bb,
                     "</D:prop>" DEBUG_CR
                     "<D:status>HTTP/1.1 200 OK</D:status>" DEBUG_CR
                     "</D:propstat>" DEBUG_CR
                     "</D:response>" DEBUG_CR,

                     NULL);

  /* ONLY have dir_delta drive the editor if the caller asked us to
     generate a full MERGE response.  svn clients can ask us to
     suppress this walk by sending specific request headers. */
  if (! disable_merge_response)
    {
      /* Now we need to generate responses for all the resources which
         changed.  This is done through a delta of the two roots.

         Note that a directory is not marked when open_dir is seen
         (since it typically is used just for changing members in that
         directory); instead, we want for a property change (the only
         reason the client would need to fetch a new directory).

         ### we probably should say something about the dirs, so that
         ### we can pass back the new version URL */

      /* and go make me proud, boy! */
      serr = do_resources(repos, root, new_rev, output, bb, pool);
      if (serr != NULL)
        {
          return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                      "Error constructing resource list.",
                                      repos->pool);
        }
    }

  /* wrap up the merge response */
  (void) ap_fputs(output, bb,
                  "</D:updated-set>" DEBUG_CR
                  "</D:merge-response>" DEBUG_CR);

  /* send whatever is left in the brigade */
  (void) ap_pass_brigade(output, bb);

  return SVN_NO_ERROR;
}

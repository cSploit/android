/*
 * util.c :  utility functions for the libsvn_client library
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
#include <apr_strings.h>

#include "svn_pools.h"
#include "svn_error.h"
#include "svn_types.h"
#include "svn_opt.h"
#include "svn_props.h"
#include "svn_path.h"
#include "svn_wc.h"
#include "svn_client.h"

#include "private/svn_client_private.h"
#include "private/svn_wc_private.h"
#include "private/svn_fspath.h"

#include "client.h"

#include "svn_private_config.h"

svn_client_commit_item3_t *
svn_client_commit_item3_create(apr_pool_t *pool)
{
  return apr_pcalloc(pool, sizeof(svn_client_commit_item3_t));
}

svn_client_commit_item3_t *
svn_client_commit_item3_dup(const svn_client_commit_item3_t *item,
                            apr_pool_t *pool)
{
  svn_client_commit_item3_t *new_item = apr_palloc(pool, sizeof(*new_item));

  *new_item = *item;

  if (new_item->path)
    new_item->path = apr_pstrdup(pool, new_item->path);

  if (new_item->url)
    new_item->url = apr_pstrdup(pool, new_item->url);

  if (new_item->copyfrom_url)
    new_item->copyfrom_url = apr_pstrdup(pool, new_item->copyfrom_url);

  if (new_item->incoming_prop_changes)
    new_item->incoming_prop_changes =
      svn_prop_array_dup(new_item->incoming_prop_changes, pool);

  if (new_item->outgoing_prop_changes)
    new_item->outgoing_prop_changes =
      svn_prop_array_dup(new_item->outgoing_prop_changes, pool);

  return new_item;
}

svn_error_t *
svn_client__path_relative_to_root(const char **rel_path,
                                  svn_wc_context_t *wc_ctx,
                                  const char *abspath_or_url,
                                  const char *repos_root,
                                  svn_boolean_t include_leading_slash,
                                  svn_ra_session_t *ra_session,
                                  apr_pool_t *result_pool,
                                  apr_pool_t *scratch_pool)
{
  const char *repos_relpath;

  /* If we have a WC path... */
  if (! svn_path_is_url(abspath_or_url))
    {
      /* ...fetch its entry, and attempt to get both its full URL and
         repository root URL.  If we can't get REPOS_ROOT from the WC
         entry, we'll get it from the RA layer.*/

      SVN_ERR(svn_wc__node_get_repos_relpath(&repos_relpath,
                                             wc_ctx,
                                             abspath_or_url,
                                             result_pool,
                                             scratch_pool));

      SVN_ERR_ASSERT(repos_relpath != NULL);
    }
     /* Merge handling passes a root that is not the repos root */
  else if (repos_root != NULL)
    {
      if (!svn_uri__is_ancestor(repos_root, abspath_or_url))
        return svn_error_createf(SVN_ERR_CLIENT_UNRELATED_RESOURCES, NULL,
                                 _("URL '%s' is not a child of repository "
                                   "root URL '%s'"),
                                 abspath_or_url, repos_root);

      repos_relpath = svn_uri_skip_ancestor(repos_root, abspath_or_url,
                                            result_pool);
    }
  else
    {
      svn_error_t *err;

      SVN_ERR_ASSERT(ra_session != NULL);

      /* Ask the RA layer to create a relative path for us */
      err = svn_ra_get_path_relative_to_root(ra_session, &repos_relpath,
                                             abspath_or_url, scratch_pool);

      if (err)
        {
          if (err->apr_err == SVN_ERR_RA_ILLEGAL_URL)
            return svn_error_createf(SVN_ERR_CLIENT_UNRELATED_RESOURCES, err,
                                     _("URL '%s' is not inside repository"),
                                     abspath_or_url);

          return svn_error_trace(err);
        }
    }

  if (include_leading_slash)
    *rel_path = apr_pstrcat(result_pool, "/", repos_relpath, NULL);
  else
    *rel_path = repos_relpath;

   return SVN_NO_ERROR;
}

svn_error_t *
svn_client__get_repos_root(const char **repos_root,
                           const char *abspath_or_url,
                           svn_client_ctx_t *ctx,
                           apr_pool_t *result_pool,
                           apr_pool_t *scratch_pool)
{
  svn_ra_session_t *ra_session;

  /* If PATH_OR_URL is a local path we can fetch the repos root locally. */
  if (!svn_path_is_url(abspath_or_url))
    {
      SVN_ERR(svn_wc__node_get_repos_info(repos_root, NULL,
                                          ctx->wc_ctx, abspath_or_url,
                                          result_pool, scratch_pool));

      return SVN_NO_ERROR;
    }

  /* If PATH_OR_URL was a URL, we use the RA layer to look it up. */
  SVN_ERR(svn_client__open_ra_session_internal(&ra_session, NULL,
                                               abspath_or_url,
                                               NULL, NULL, FALSE, TRUE,
                                               ctx, scratch_pool));

  SVN_ERR(svn_ra_get_repos_root2(ra_session, repos_root, result_pool));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_client__default_walker_error_handler(const char *path,
                                         svn_error_t *err,
                                         void *walk_baton,
                                         apr_pool_t *pool)
{
  return svn_error_trace(err);
}


const svn_opt_revision_t *
svn_cl__rev_default_to_head_or_base(const svn_opt_revision_t *revision,
                                    const char *path_or_url)
{
  static svn_opt_revision_t head_rev = { svn_opt_revision_head, { 0 } };
  static svn_opt_revision_t base_rev = { svn_opt_revision_base, { 0 } };

  if (revision->kind == svn_opt_revision_unspecified)
    return svn_path_is_url(path_or_url) ? &head_rev : &base_rev;
  return revision;
}

const svn_opt_revision_t *
svn_cl__rev_default_to_head_or_working(const svn_opt_revision_t *revision,
                                       const char *path_or_url)
{
  static svn_opt_revision_t head_rev = { svn_opt_revision_head, { 0 } };
  static svn_opt_revision_t work_rev = { svn_opt_revision_working, { 0 } };

  if (revision->kind == svn_opt_revision_unspecified)
    return svn_path_is_url(path_or_url) ? &head_rev : &work_rev;
  return revision;
}

const svn_opt_revision_t *
svn_cl__rev_default_to_peg(const svn_opt_revision_t *revision,
                           const svn_opt_revision_t *peg_revision)
{
  if (revision->kind == svn_opt_revision_unspecified)
    return peg_revision;
  return revision;
}

svn_error_t *
svn_client__assert_homogeneous_target_type(const apr_array_header_t *targets)
{
  svn_boolean_t wc_present = FALSE, url_present = FALSE;
  int i;

  for (i = 0; i < targets->nelts; ++i)
    {
      const char *target = APR_ARRAY_IDX(targets, i, const char *);
      if (! svn_path_is_url(target))
        wc_present = TRUE;
      else
        url_present = TRUE;
      if (url_present && wc_present)
        return svn_error_createf(SVN_ERR_ILLEGAL_TARGET, NULL,
                                 _("Cannot mix repository and working copy "
                                   "targets"));
    }

  return SVN_NO_ERROR;
}

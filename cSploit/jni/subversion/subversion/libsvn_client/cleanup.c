/*
 * cleanup.c:  wrapper around wc cleanup functionality.
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

/* ==================================================================== */



/*** Includes. ***/

#include "svn_time.h"
#include "svn_wc.h"
#include "svn_client.h"
#include "svn_config.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_pools.h"
#include "client.h"
#include "svn_props.h"

#include "svn_private_config.h"
#include "private/svn_wc_private.h"


/*** Code. ***/

svn_error_t *
svn_client_cleanup(const char *path,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *scratch_pool)
{
  const char *local_abspath;
  svn_error_t *err;

  if (svn_path_is_url(path))
    return svn_error_createf(SVN_ERR_ILLEGAL_TARGET, NULL,
                             _("'%s' is not a local path"), path);

  SVN_ERR(svn_dirent_get_absolute(&local_abspath, path, scratch_pool));

  err = svn_wc_cleanup3(ctx->wc_ctx, local_abspath, ctx->cancel_func,
                        ctx->cancel_baton, scratch_pool);
  svn_io_sleep_for_timestamps(path, scratch_pool);
  return svn_error_trace(err);
}


/* callback baton for fetch_repos_info */
struct repos_info_baton
{
  apr_pool_t *state_pool;
  svn_client_ctx_t *ctx;
  const char *last_repos;
  const char *last_uuid;
};

/* svn_wc_upgrade_get_repos_info_t implementation for calling
   svn_wc_upgrade() from svn_client_upgrade() */
static svn_error_t *
fetch_repos_info(const char **repos_root,
                 const char **repos_uuid,
                 void *baton,
                 const char *url,
                 apr_pool_t *result_pool,
                 apr_pool_t *scratch_pool)
{
  struct repos_info_baton *ri = baton;
  apr_pool_t *subpool;
  svn_ra_session_t *ra_session;

  /* The same info is likely to retrieved multiple times (e.g. externals) */
  if (ri->last_repos && svn_uri__is_child(ri->last_repos, url, scratch_pool))
    {
      *repos_root = apr_pstrdup(result_pool, ri->last_repos);
      *repos_uuid = apr_pstrdup(result_pool, ri->last_uuid);
      return SVN_NO_ERROR;
    }

  subpool = svn_pool_create(scratch_pool);

  SVN_ERR(svn_client_open_ra_session(&ra_session, url, ri->ctx, subpool));

  SVN_ERR(svn_ra_get_repos_root2(ra_session, repos_root, result_pool));
  SVN_ERR(svn_ra_get_uuid2(ra_session, repos_uuid, result_pool));

  /* Store data for further calls */
  ri->last_repos = apr_pstrdup(ri->state_pool, *repos_root);
  ri->last_uuid = apr_pstrdup(ri->state_pool, *repos_uuid);

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_client_upgrade(const char *path,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *scratch_pool)
{
  const char *local_abspath;
  apr_hash_t *externals;
  apr_hash_index_t *hi;
  apr_pool_t *iterpool;
  apr_pool_t *iterpool2;
  svn_opt_revision_t rev = {svn_opt_revision_unspecified, {0}};
  struct repos_info_baton info_baton;

  info_baton.state_pool = scratch_pool;
  info_baton.ctx = ctx;
  info_baton.last_repos = NULL;
  info_baton.last_uuid = NULL;

  if (svn_path_is_url(path))
    return svn_error_createf(SVN_ERR_ILLEGAL_TARGET, NULL,
                             _("'%s' is not a local path"), path);

  SVN_ERR(svn_dirent_get_absolute(&local_abspath, path, scratch_pool));
  SVN_ERR(svn_wc_upgrade(ctx->wc_ctx, local_abspath,
                         fetch_repos_info, &info_baton,
                         ctx->cancel_func, ctx->cancel_baton,
                         ctx->notify_func2, ctx->notify_baton2,
                         scratch_pool));

  /* Now it's time to upgrade the externals too. We do it after the wc
     upgrade to avoid that errors in the externals causes the wc upgrade to
     fail. Thanks to caching the performance penalty of walking the wc a
     second time shouldn't be too severe */
  SVN_ERR(svn_client_propget4(&externals, SVN_PROP_EXTERNALS, local_abspath,
                              &rev, &rev, NULL, svn_depth_infinity, NULL, ctx,
                              scratch_pool, scratch_pool));

  iterpool = svn_pool_create(scratch_pool);
  iterpool2 = svn_pool_create(scratch_pool);

  for (hi = apr_hash_first(scratch_pool, externals); hi;
       hi = apr_hash_next(hi))
    {
      int i;
      const char *externals_parent = svn__apr_hash_index_key(hi);
      svn_string_t *external_desc = svn__apr_hash_index_val(hi);
      apr_array_header_t *externals_p;

      svn_pool_clear(iterpool);
      externals_p = apr_array_make(iterpool, 1,
                                   sizeof(svn_wc_external_item2_t*));

      SVN_ERR(svn_wc_parse_externals_description3(
                  &externals_p, svn_dirent_dirname(path, iterpool),
                  external_desc->data, TRUE, iterpool));
      for (i = 0; i < externals_p->nelts; i++)
        {
          svn_wc_external_item2_t *item;
          const char *external_abspath;
          const char *external_path;
          const char *repos_relpath;
          const char *repos_root_url;
          const char *repos_uuid;
          svn_node_kind_t kind;
          svn_revnum_t peg_revision;
          svn_revnum_t revision;
          svn_error_t *err;

          item = APR_ARRAY_IDX(externals_p, i, svn_wc_external_item2_t*);

          svn_pool_clear(iterpool2);
          external_path = svn_dirent_join(externals_parent, item->target_dir,
                                          iterpool2);

          err = svn_dirent_get_absolute(&external_abspath, external_path,
                                        iterpool2);
          if (err)
            goto handle_error;

          /* This is hack. We can only send dirs to svn_wc_upgrade(). This
             way we will get an exception saying that the wc must be
             upgraded if it's a dir. If it's a file then the lookup is done
             in an adm_dir belonging to the real wc and since that was
             updated before the externals no error is returned. */
          err = svn_wc_read_kind(&kind, ctx->wc_ctx, external_abspath, FALSE,
                                 iterpool2);

          if (err && err->apr_err == SVN_ERR_WC_UPGRADE_REQUIRED)
            {
              svn_error_clear(err);

              err = svn_client_upgrade(external_abspath, ctx, iterpool2);
            }

          if (err)
            goto handle_error;

          /* The upgrade of any dir should be done now, get the (supposedly
           * now reliable) kind. */
          err = svn_wc_read_kind(&kind, ctx->wc_ctx, external_abspath,
                                 FALSE, iterpool2);
          if (err)
            goto handle_error;

          /* Update the EXTERNALS table according to the root URL,
           * relpath and uuid known in the upgraded external WC. */

          /* We should probably have a function that provides all three
           * of root URL, repos relpath and uuid at once, but here goes... */

          /* First get the relpath, as that returns SVN_ERR_WC_PATH_NOT_FOUND
           * when the node is not present in the file system.
           * (svn_wc__node_get_repos_info() would try to derive the URL). */
          err = svn_wc__node_get_repos_relpath(&repos_relpath,
                                               ctx->wc_ctx,
                                               external_abspath,
                                               iterpool2, iterpool2);
          if (! err)
            {
              /* We got a repos relpath from a WC. So also get the root. */
              err = svn_wc__node_get_repos_info(&repos_root_url,
                                                &repos_uuid,
                                                ctx->wc_ctx,
                                                external_abspath,
                                                iterpool2, iterpool2);
            }
          else if (err && err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
            {
              /* The external is not currently checked out. Try to figure out
               * the URL parts via the defined URL and fetch_repos_info(). */
              svn_error_clear(err);

              /* The repos root / uuid from above get_repos_info() call, if it
               * was successful, has returned the URL as derived from the WC's
               * parent path, which is not what we want for the external. Only
               * makes sense for added/deleted/not-present files. So make sure
               * those values are not used. */
              repos_root_url = NULL;
              repos_relpath = NULL;

              err = fetch_repos_info(&repos_root_url,
                                     &repos_uuid,
                                     &info_baton,
                                     item->url,
                                     scratch_pool, scratch_pool);
              if (err)
                goto handle_error;


              repos_relpath = svn_uri_skip_ancestor(repos_root_url, item->url,
                                                    iterpool2);

              /* There's just this URL, no idea what kind it is. */
              kind = svn_node_unknown;
            }

          if (err)
            goto handle_error;

          peg_revision = (item->peg_revision.kind == svn_opt_revision_number
                          ? item->peg_revision.value.number
                          : SVN_INVALID_REVNUM);

          revision = (item->revision.kind == svn_opt_revision_number
                      ? item->revision.value.number
                      : SVN_INVALID_REVNUM);

          err = svn_wc__upgrade_add_external_info(ctx->wc_ctx,
                                                  external_abspath,
                                                  kind,
                                                  externals_parent,
                                                  repos_relpath,
                                                  repos_root_url,
                                                  repos_uuid,
                                                  peg_revision,
                                                  revision,
                                                  iterpool2);
handle_error:
          if (err)
            {
              svn_wc_notify_t *notify =
                  svn_wc_create_notify(external_abspath,
                                       svn_wc_notify_failed_external,
                                       scratch_pool);
              notify->err = err;

              ctx->notify_func2(ctx->notify_baton2,
                                notify, scratch_pool);

              svn_error_clear(err);
            }
        }
    }

  svn_pool_destroy(iterpool);
  svn_pool_destroy(iterpool2);

  return SVN_NO_ERROR;
}

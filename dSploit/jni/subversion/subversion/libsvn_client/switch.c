/*
 * switch.c:  implement 'switch' feature via WC & RA interfaces.
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

#include "svn_client.h"
#include "svn_error.h"
#include "svn_time.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_config.h"
#include "svn_pools.h"
#include "client.h"

#include "svn_private_config.h"
#include "private/svn_wc_private.h"


/*** Code. ***/

/* This feature is essentially identical to 'svn update' (see
   ./update.c), but with two differences:

     - the reporter->finish_report() routine needs to make the server
       run delta_dirs() on two *different* paths, rather than on two
       identical paths.

     - after the update runs, we need to more than just
       ensure_uniform_revision;  we need to rewrite all the entries'
       URL attributes.
*/


static svn_error_t *
switch_internal(svn_revnum_t *result_rev,
                const char *local_abspath,
                const char *anchor_abspath,
                const char *switch_url,
                const svn_opt_revision_t *peg_revision,
                const svn_opt_revision_t *revision,
                svn_depth_t depth,
                svn_boolean_t depth_is_sticky,
                svn_boolean_t ignore_externals,
                svn_boolean_t allow_unver_obstructions,
                svn_boolean_t ignore_ancestry,
                svn_boolean_t *timestamp_sleep,
                svn_client_ctx_t *ctx,
                apr_pool_t *pool)
{
  const svn_ra_reporter3_t *reporter;
  void *report_baton;
  const char *url, *target, *source_root, *switch_rev_url;
  svn_ra_session_t *ra_session;
  svn_revnum_t revnum;
  svn_error_t *err = SVN_NO_ERROR;
  const char *diff3_cmd;
  svn_boolean_t use_commit_times;
  svn_boolean_t sleep_here = FALSE;
  svn_boolean_t *use_sleep = timestamp_sleep ? timestamp_sleep : &sleep_here;
  const svn_delta_editor_t *switch_editor;
  void *switch_edit_baton;
  const char *preserved_exts_str;
  apr_array_header_t *preserved_exts;
  svn_boolean_t server_supports_depth;
  struct svn_client__dirent_fetcher_baton_t dfb;
  svn_config_t *cfg = ctx->config ? apr_hash_get(ctx->config,
                                                 SVN_CONFIG_CATEGORY_CONFIG,
                                                 APR_HASH_KEY_STRING)
                                  : NULL;

  /* An unknown depth can't be sticky. */
  if (depth == svn_depth_unknown)
    depth_is_sticky = FALSE;

  /* Do not support the situation of both exclude and switch a target. */
  if (depth == svn_depth_exclude)
    return svn_error_createf(SVN_ERR_UNSUPPORTED_FEATURE, NULL,
                             _("Cannot both exclude and switch a path"));

  /* Get the external diff3, if any. */
  svn_config_get(cfg, &diff3_cmd, SVN_CONFIG_SECTION_HELPERS,
                 SVN_CONFIG_OPTION_DIFF3_CMD, NULL);

  if (diff3_cmd != NULL)
    SVN_ERR(svn_path_cstring_to_utf8(&diff3_cmd, diff3_cmd, pool));

  /* See if the user wants last-commit timestamps instead of current ones. */
  SVN_ERR(svn_config_get_bool(cfg, &use_commit_times,
                              SVN_CONFIG_SECTION_MISCELLANY,
                              SVN_CONFIG_OPTION_USE_COMMIT_TIMES, FALSE));

  {
    svn_boolean_t has_working;
    SVN_ERR(svn_wc__node_has_working(&has_working, ctx->wc_ctx, local_abspath,
                                     pool));

    if (has_working)
      return svn_error_createf(SVN_ERR_UNSUPPORTED_FEATURE, NULL,
                               _("Cannot switch '%s' because it is not in the "
                                 "repository yet"),
                               svn_dirent_local_style(local_abspath, pool));
  }

  /* See which files the user wants to preserve the extension of when
     conflict files are made. */
  svn_config_get(cfg, &preserved_exts_str, SVN_CONFIG_SECTION_MISCELLANY,
                 SVN_CONFIG_OPTION_PRESERVED_CF_EXTS, "");
  preserved_exts = *preserved_exts_str
    ? svn_cstring_split(preserved_exts_str, "\n\r\t\v ", FALSE, pool)
    : NULL;

  /* Sanity check.  Without these, the switch is meaningless. */
  SVN_ERR_ASSERT(switch_url && (switch_url[0] != '\0'));

  if (strcmp(local_abspath, anchor_abspath))
    target = svn_dirent_basename(local_abspath, pool);
  else
    target = "";

  SVN_ERR(svn_wc__node_get_url(&url, ctx->wc_ctx, anchor_abspath, pool, pool));
  if (! url)
    return svn_error_createf(SVN_ERR_ENTRY_MISSING_URL, NULL,
                             _("Directory '%s' has no URL"),
                             svn_dirent_local_style(anchor_abspath, pool));

  /* We may need to crop the tree if the depth is sticky */
  if (depth_is_sticky && depth < svn_depth_infinity)
    {
      svn_node_kind_t target_kind;

      if (depth == svn_depth_exclude)
        {
          SVN_ERR(svn_wc_exclude(ctx->wc_ctx,
                                 local_abspath,
                                 ctx->cancel_func, ctx->cancel_baton,
                                 ctx->notify_func2, ctx->notify_baton2,
                                 pool));

          /* Target excluded, we are done now */
          return SVN_NO_ERROR;
        }

      SVN_ERR(svn_wc_read_kind(&target_kind, ctx->wc_ctx, local_abspath, TRUE,
                               pool));

      if (target_kind == svn_node_dir)
        SVN_ERR(svn_wc_crop_tree2(ctx->wc_ctx, local_abspath, depth,
                                  ctx->cancel_func, ctx->cancel_baton,
                                  ctx->notify_func2, ctx->notify_baton2,
                                  pool));
    }

  /* Open an RA session to 'source' URL */
  SVN_ERR(svn_client__ra_session_from_path(&ra_session, &revnum,
                                           &switch_rev_url,
                                           switch_url, anchor_abspath,
                                           peg_revision, revision,
                                           ctx, pool));

  SVN_ERR(svn_ra_get_repos_root2(ra_session, &source_root, pool));

  /* Disallow a switch operation to change the repository root of the
     target. */
  if (! svn_uri__is_ancestor(source_root, url))
    return svn_error_createf(SVN_ERR_WC_INVALID_SWITCH, NULL,
                             _("'%s'\nis not the same repository as\n'%s'"),
                             url, source_root);

  /* If we're not ignoring ancestry, then error out if the switch
     source and target don't have a common ancestory.

     ### We're acting on the anchor here, not the target.  Is that
     ### okay? */
  if (! ignore_ancestry)
    {
      const char *target_url, *yc_path;
      svn_revnum_t target_rev, yc_rev;

      SVN_ERR(svn_wc__node_get_url(&target_url, ctx->wc_ctx, local_abspath,
                                   pool, pool));
      SVN_ERR(svn_wc__node_get_base_rev(&target_rev, ctx->wc_ctx,
                                        local_abspath, pool));
      /* ### It would be nice if this function could reuse the existing
             ra session instead of opening two for its own use. */
      SVN_ERR(svn_client__get_youngest_common_ancestor(&yc_path, &yc_rev,
                                                       switch_rev_url, revnum,
                                                       target_url, target_rev,
                                                       ctx, pool));
      if (! (yc_path && SVN_IS_VALID_REVNUM(yc_rev)))
        return svn_error_createf(SVN_ERR_CLIENT_UNRELATED_RESOURCES, NULL,
                                 _("'%s' shares no common ancestry with '%s'"),
                                 switch_url, local_abspath);
    }


  SVN_ERR(svn_ra_reparent(ra_session, url, pool));

  /* Fetch the switch (update) editor.  If REVISION is invalid, that's
     okay; the RA driver will call editor->set_target_revision() later on. */
  SVN_ERR(svn_ra_has_capability(ra_session, &server_supports_depth,
                                SVN_RA_CAPABILITY_DEPTH, pool));

  dfb.ra_session = ra_session;
  SVN_ERR(svn_ra_get_session_url(ra_session, &dfb.anchor_url, pool));
  dfb.target_revision = revnum;

  SVN_ERR(svn_wc_get_switch_editor4(&switch_editor, &switch_edit_baton,
                                    &revnum, ctx->wc_ctx, anchor_abspath,
                                    target, switch_rev_url, use_commit_times,
                                    depth,
                                    depth_is_sticky, allow_unver_obstructions,
                                    server_supports_depth,
                                    diff3_cmd, preserved_exts,
                                    svn_client__dirent_fetcher, &dfb,
                                    ctx->conflict_func2, ctx->conflict_baton2,
                                    NULL, NULL,
                                    ctx->cancel_func, ctx->cancel_baton,
                                    ctx->notify_func2, ctx->notify_baton2,
                                    pool, pool));

  /* Tell RA to do an update of URL+TARGET to REVISION; if we pass an
     invalid revnum, that means RA will use the latest revision. */
  SVN_ERR(svn_ra_do_switch2(ra_session, &reporter, &report_baton, revnum,
                            target,
                            depth_is_sticky ? depth : svn_depth_unknown,
                            switch_rev_url,
                            switch_editor, switch_edit_baton, pool));

  /* Drive the reporter structure, describing the revisions within
     PATH.  When we call reporter->finish_report, the update_editor
     will be driven by svn_repos_dir_delta2.

     We pass in an external_func for recording all externals. It
     shouldn't be needed for a switch if it wasn't for the relative
     externals of type '../path'. All of those must be resolved to
     the new location.  */
  err = svn_wc_crawl_revisions5(ctx->wc_ctx, local_abspath, reporter,
                                report_baton, TRUE, depth, (! depth_is_sticky),
                                (! server_supports_depth),
                                use_commit_times,
                                ctx->cancel_func, ctx->cancel_baton,
                                ctx->notify_func2, ctx->notify_baton2, pool);

  if (err)
    {
      /* Don't rely on the error handling to handle the sleep later, do
         it now */
      svn_io_sleep_for_timestamps(local_abspath, pool);
      return svn_error_trace(err);
    }
  *use_sleep = TRUE;

  /* We handle externals after the switch is complete, so that
     handling external items (and any errors therefrom) doesn't delay
     the primary operation. */
  if (SVN_DEPTH_IS_RECURSIVE(depth) && (! ignore_externals))
    {
      apr_hash_t *new_externals;
      apr_hash_t *new_depths;
      SVN_ERR(svn_wc__externals_gather_definitions(&new_externals,
                                                   &new_depths,
                                                   ctx->wc_ctx, local_abspath,
                                                   depth, pool, pool));

      SVN_ERR(svn_client__handle_externals(new_externals,
                                           new_depths,
                                           source_root, local_abspath,
                                           depth, use_sleep,
                                           ctx, pool));
    }

  /* Sleep to ensure timestamp integrity (we do this regardless of
     errors in the actual switch operation(s)). */
  if (sleep_here)
    svn_io_sleep_for_timestamps(local_abspath, pool);

  /* Return errors we might have sustained. */
  if (err)
    return svn_error_trace(err);

  /* Let everyone know we're finished here. */
  if (ctx->notify_func2)
    {
      svn_wc_notify_t *notify
        = svn_wc_create_notify(anchor_abspath, svn_wc_notify_update_completed,
                               pool);
      notify->kind = svn_node_none;
      notify->content_state = notify->prop_state
        = svn_wc_notify_state_inapplicable;
      notify->lock_state = svn_wc_notify_lock_state_inapplicable;
      notify->revision = revnum;
      (*ctx->notify_func2)(ctx->notify_baton2, notify, pool);
    }

  /* If the caller wants the result revision, give it to them. */
  if (result_rev)
    *result_rev = revnum;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_client__switch_internal(svn_revnum_t *result_rev,
                            const char *path,
                            const char *switch_url,
                            const svn_opt_revision_t *peg_revision,
                            const svn_opt_revision_t *revision,
                            svn_depth_t depth,
                            svn_boolean_t depth_is_sticky,
                            svn_boolean_t ignore_externals,
                            svn_boolean_t allow_unver_obstructions,
                            svn_boolean_t ignore_ancestry,
                            svn_boolean_t *timestamp_sleep,
                            svn_client_ctx_t *ctx,
                            apr_pool_t *pool)
{
  const char *local_abspath, *anchor_abspath;
  svn_boolean_t acquired_lock;
  svn_error_t *err, *err1, *err2;

  SVN_ERR_ASSERT(path);

  SVN_ERR(svn_dirent_get_absolute(&local_abspath, path, pool));

  /* Rely on svn_wc__acquire_write_lock setting ANCHOR_ABSPATH even
     when it returns SVN_ERR_WC_LOCKED */
  err = svn_wc__acquire_write_lock(&anchor_abspath,
                                   ctx->wc_ctx, local_abspath, TRUE,
                                   pool, pool);
  if (err && err->apr_err != SVN_ERR_WC_LOCKED)
    return svn_error_trace(err);

  acquired_lock = (err == SVN_NO_ERROR);
  svn_error_clear(err);

  err1 = switch_internal(result_rev, local_abspath, anchor_abspath,
                         switch_url, peg_revision, revision,
                         depth, depth_is_sticky,
                         ignore_externals,
                         allow_unver_obstructions, ignore_ancestry,
                         timestamp_sleep, ctx, pool);

  if (acquired_lock)
    err2 = svn_wc__release_write_lock(ctx->wc_ctx, anchor_abspath, pool);
  else
    err2 = SVN_NO_ERROR;

  return svn_error_compose_create(err1, err2);
}

svn_error_t *
svn_client_switch3(svn_revnum_t *result_rev,
                   const char *path,
                   const char *switch_url,
                   const svn_opt_revision_t *peg_revision,
                   const svn_opt_revision_t *revision,
                   svn_depth_t depth,
                   svn_boolean_t depth_is_sticky,
                   svn_boolean_t ignore_externals,
                   svn_boolean_t allow_unver_obstructions,
                   svn_boolean_t ignore_ancestry,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool)
{
  if (svn_path_is_url(path))
    return svn_error_createf(SVN_ERR_ILLEGAL_TARGET, NULL,
                             _("'%s' is not a local path"), path);

  return svn_client__switch_internal(result_rev, path, switch_url,
                                     peg_revision, revision, depth,
                                     depth_is_sticky, ignore_externals,
                                     allow_unver_obstructions,
                                     ignore_ancestry, NULL, ctx, pool);
}

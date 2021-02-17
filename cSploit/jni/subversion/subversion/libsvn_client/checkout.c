/*
 * checkout.c:  wrappers around wc checkout functionality
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

#include "svn_pools.h"
#include "svn_wc.h"
#include "svn_client.h"
#include "svn_ra.h"
#include "svn_types.h"
#include "svn_error.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_io.h"
#include "svn_opt.h"
#include "svn_time.h"
#include "client.h"

#include "private/svn_wc_private.h"

#include "svn_private_config.h"


/*** Public Interfaces. ***/

static svn_error_t *
initialize_area(const char *local_abspath,
                const char *session_url,
                const char *repos_root,
                const char *uuid,
                svn_revnum_t revnum,
                svn_depth_t depth,
                svn_client_ctx_t *ctx,
                apr_pool_t *pool)
{
  if (depth == svn_depth_unknown)
    depth = svn_depth_infinity;

  /* Make the unversioned directory into a versioned one.  */
  SVN_ERR(svn_wc_ensure_adm4(ctx->wc_ctx, local_abspath, session_url,
                             repos_root, uuid, revnum, depth, pool));
  return SVN_NO_ERROR;
}


svn_error_t *
svn_client__checkout_internal(svn_revnum_t *result_rev,
                              const char *url,
                              const char *local_abspath,
                              const svn_opt_revision_t *peg_revision,
                              const svn_opt_revision_t *revision,
                              svn_depth_t depth,
                              svn_boolean_t ignore_externals,
                              svn_boolean_t allow_unver_obstructions,
                              svn_boolean_t *timestamp_sleep,
                              svn_client_ctx_t *ctx,
                              apr_pool_t *pool)
{
  svn_error_t *err = NULL;
  svn_revnum_t revnum;
  svn_boolean_t sleep_here = FALSE;
  svn_boolean_t *use_sleep = timestamp_sleep ? timestamp_sleep : &sleep_here;
  const char *session_url;
  svn_node_kind_t kind;
  const char *uuid, *repos_root;
  apr_pool_t *session_pool = svn_pool_create(pool);
  svn_ra_session_t *ra_session;
  svn_revnum_t tmp_revnum;
  const char *tmp_session_url;

  /* Sanity check.  Without these, the checkout is meaningless. */
  SVN_ERR_ASSERT(local_abspath != NULL);
  SVN_ERR_ASSERT(svn_uri_is_canonical(url, pool));
  SVN_ERR_ASSERT(svn_dirent_is_absolute(local_abspath));

  /* Fulfill the docstring promise of svn_client_checkout: */
  if ((revision->kind != svn_opt_revision_number)
      && (revision->kind != svn_opt_revision_date)
      && (revision->kind != svn_opt_revision_head))
    return svn_error_create(SVN_ERR_CLIENT_BAD_REVISION, NULL, NULL);

  /* Get the RA connection. */
  SVN_ERR(svn_client__ra_session_from_path(&ra_session, &tmp_revnum,
                                           &tmp_session_url, url, NULL,
                                           peg_revision, revision, ctx,
                                           session_pool));

  SVN_ERR(svn_ra_get_repos_root2(ra_session, &repos_root, pool));
  SVN_ERR(svn_ra_get_uuid2(ra_session, &uuid, pool));
  session_url = apr_pstrdup(pool, tmp_session_url);
  revnum = tmp_revnum;
  SVN_ERR(svn_ra_check_path(ra_session, "", revnum, &kind, pool));

  svn_pool_destroy(session_pool);

  if (kind == svn_node_none)
    return svn_error_createf(SVN_ERR_RA_ILLEGAL_URL, NULL,
                             _("URL '%s' doesn't exist"), session_url);
  else if (kind == svn_node_file)
    return svn_error_createf
      (SVN_ERR_UNSUPPORTED_FEATURE , NULL,
       _("URL '%s' refers to a file, not a directory"), session_url);

  SVN_ERR(svn_io_check_path(local_abspath, &kind, pool));

  if (kind == svn_node_none)
    {
      /* Bootstrap: create an incomplete working-copy root dir.  Its
         entries file should only have an entry for THIS_DIR with a
         URL, revnum, and an 'incomplete' flag.  */
      SVN_ERR(svn_io_make_dir_recursively(local_abspath, pool));
      err = initialize_area(local_abspath, session_url,
                            repos_root, uuid, revnum, depth, ctx, pool);
    }
  else if (kind == svn_node_dir)
    {
      int wc_format;
      const char *entry_url;

      SVN_ERR(svn_wc_check_wc2(&wc_format, ctx->wc_ctx, local_abspath, pool));
      if (! wc_format)
        {
          err = initialize_area(local_abspath, session_url,
                                repos_root, uuid, revnum, depth, ctx, pool);
        }
      else
        {
          /* Get PATH's URL. */
          SVN_ERR(svn_wc__node_get_url(&entry_url, ctx->wc_ctx, local_abspath,
                                       pool, pool));

          /* If PATH's existing URL matches the incoming one, then
             just update.  This allows 'svn co' to restart an
             interrupted checkout.  Otherwise bail out. */
          if (strcmp(entry_url, session_url) != 0)
            return svn_error_createf(
                          SVN_ERR_WC_OBSTRUCTED_UPDATE, NULL,
                          _("'%s' is already a working copy for a"
                            " different URL"),
                          svn_dirent_local_style(local_abspath, pool));
        }
    }
  else
    {
      return svn_error_createf(SVN_ERR_WC_NODE_KIND_CHANGE, NULL,
                               _("'%s' already exists and is not a directory"),
                               svn_dirent_local_style(local_abspath, pool));
    }

  /* Have update fix the incompleteness. */
  if (! err)
    {
      err = svn_client__update_internal(result_rev, local_abspath,
                                        revision, depth, TRUE,
                                        ignore_externals,
                                        allow_unver_obstructions,
                                        TRUE /* adds_as_modification */,
                                        FALSE, FALSE,
                                        use_sleep, ctx, pool);
    }

  if (err)
    {
      /* Don't rely on the error handling to handle the sleep later, do
         it now */
      svn_io_sleep_for_timestamps(local_abspath, pool);
      return svn_error_trace(err);
    }
  *use_sleep = TRUE;

  if (sleep_here)
    svn_io_sleep_for_timestamps(local_abspath, pool);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_client_checkout3(svn_revnum_t *result_rev,
                     const char *URL,
                     const char *path,
                     const svn_opt_revision_t *peg_revision,
                     const svn_opt_revision_t *revision,
                     svn_depth_t depth,
                     svn_boolean_t ignore_externals,
                     svn_boolean_t allow_unver_obstructions,
                     svn_client_ctx_t *ctx,
                     apr_pool_t *pool)
{
  const char *local_abspath;

  SVN_ERR(svn_dirent_get_absolute(&local_abspath, path, pool));

  return svn_client__checkout_internal(result_rev, URL, local_abspath,
                                       peg_revision, revision, depth,
                                       ignore_externals,
                                       allow_unver_obstructions, NULL,
                                       ctx, pool);
}

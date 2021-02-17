/*
 * copy-cmd.c -- Subversion copy command
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
#include "svn_path.h"
#include "svn_error.h"
#include "cl.h"

#include "svn_private_config.h"


/*** Code. ***/

/* This implements the `svn_opt_subcommand_t' interface. */
svn_error_t *
svn_cl__copy(apr_getopt_t *os,
             void *baton,
             apr_pool_t *pool)
{
  svn_cl__opt_state_t *opt_state = ((svn_cl__cmd_baton_t *) baton)->opt_state;
  svn_client_ctx_t *ctx = ((svn_cl__cmd_baton_t *) baton)->ctx;
  apr_array_header_t *targets, *sources;
  const char *src_path, *dst_path;
  svn_boolean_t srcs_are_urls, dst_is_url;
  svn_error_t *err;
  int i;

  SVN_ERR(svn_cl__args_to_target_array_print_reserved(&targets, os,
                                                      opt_state->targets,
                                                      ctx, FALSE, pool));
  if (targets->nelts < 2)
    return svn_error_create(SVN_ERR_CL_INSUFFICIENT_ARGS, 0, NULL);

  /* Get the src list and associated peg revs */
  sources = apr_array_make(pool, targets->nelts - 1,
                           sizeof(svn_client_copy_source_t *));
  for (i = 0; i < (targets->nelts - 1); i++)
    {
      const char *target = APR_ARRAY_IDX(targets, i, const char *);
      svn_client_copy_source_t *source = apr_palloc(pool, sizeof(*source));
      const char *src;
      svn_opt_revision_t *peg_revision = apr_palloc(pool,
                                                    sizeof(*peg_revision));

      SVN_ERR(svn_opt_parse_path(peg_revision, &src, target, pool));
      source->path = src;
      source->revision = &(opt_state->start_revision);
      source->peg_revision = peg_revision;

      APR_ARRAY_PUSH(sources, svn_client_copy_source_t *) = source;
    }

  SVN_ERR(svn_cl__eat_peg_revisions(&targets, targets, pool));

  /* Figure out which type of notification to use.
     (There is no need to check that the src paths are homogeneous;
     svn_client_copy6() through its subroutine try_copy() will return an
     error if they are not.) */
  src_path = APR_ARRAY_IDX(targets, 0, const char *);
  srcs_are_urls = svn_path_is_url(src_path);
  dst_path = APR_ARRAY_IDX(targets, targets->nelts - 1, const char *);
  apr_array_pop(targets);
  dst_is_url = svn_path_is_url(dst_path);

  if ((! srcs_are_urls) && (! dst_is_url))
    {
      /* WC->WC */
    }
  else if ((! srcs_are_urls) && (dst_is_url))
    {
      /* WC->URL : Use notification. */
      if (! opt_state->quiet)
        SVN_ERR(svn_cl__notifier_mark_wc_to_repos_copy(ctx->notify_baton2));
    }
  else if ((srcs_are_urls) && (! dst_is_url))
    {
     /* URL->WC : Use checkout-style notification. */
     if (! opt_state->quiet)
       SVN_ERR(svn_cl__notifier_mark_checkout(ctx->notify_baton2));
    }
  else
    {
      /* URL -> URL, meaning that no notification is needed. */
      ctx->notify_func2 = NULL;
    }

  if (! dst_is_url)
    {
      ctx->log_msg_func3 = NULL;
      if (opt_state->message || opt_state->filedata || opt_state->revprop_table)
        return svn_error_create
          (SVN_ERR_CL_UNNECESSARY_LOG_MESSAGE, NULL,
           _("Local, non-commit operations do not take a log message "
             "or revision properties"));
    }

  if (ctx->log_msg_func3)
    SVN_ERR(svn_cl__make_log_msg_baton(&(ctx->log_msg_baton3), opt_state,
                                       NULL, ctx->config, pool));

  err = svn_client_copy6(sources, dst_path, TRUE,
                         opt_state->parents, opt_state->ignore_externals,
                         opt_state->revprop_table,
                         (opt_state->quiet ? NULL : svn_cl__print_commit_info),
                         NULL,
                         ctx, pool);

  if (ctx->log_msg_func3)
    SVN_ERR(svn_cl__cleanup_log_msg(ctx->log_msg_baton3, err, pool));
  else if (err)
    return svn_error_trace(err);

  return SVN_NO_ERROR;
}

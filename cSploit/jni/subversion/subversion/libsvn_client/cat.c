/*
 * cat.c:  implementation of the 'cat' command
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
#include "svn_string.h"
#include "svn_error.h"
#include "svn_subst.h"
#include "svn_io.h"
#include "svn_time.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_props.h"
#include "client.h"

#include "svn_private_config.h"
#include "private/svn_wc_private.h"


/*** Code. ***/

svn_error_t *
svn_client__get_normalized_stream(svn_stream_t **normal_stream,
                                  svn_wc_context_t *wc_ctx,
                                  const char *local_abspath,
                                  const svn_opt_revision_t *revision,
                                  svn_boolean_t expand_keywords,
                                  svn_boolean_t normalize_eols,
                                  svn_cancel_func_t cancel_func,
                                  void *cancel_baton,
                                  apr_pool_t *result_pool,
                                  apr_pool_t *scratch_pool)
{
  apr_hash_t *kw = NULL;
  svn_subst_eol_style_t style;
  apr_hash_t *props;
  svn_string_t *eol_style, *keywords, *special;
  const char *eol = NULL;
  svn_boolean_t local_mod = FALSE;
  apr_time_t tm;
  svn_stream_t *input;
  svn_node_kind_t kind;

  SVN_ERR_ASSERT(SVN_CLIENT__REVKIND_IS_LOCAL_TO_WC(revision->kind));

  SVN_ERR(svn_wc_read_kind(&kind, wc_ctx, local_abspath, FALSE, scratch_pool));

  if (kind == svn_node_unknown || kind == svn_node_none)
    return svn_error_createf(SVN_ERR_UNVERSIONED_RESOURCE, NULL,
                             _("'%s' is not under version control"),
                             svn_dirent_local_style(local_abspath,
                                                    scratch_pool));
  if (kind != svn_node_file)
    return svn_error_createf(SVN_ERR_CLIENT_IS_DIRECTORY, NULL,
                             _("'%s' refers to a directory"),
                             svn_dirent_local_style(local_abspath,
                                                    scratch_pool));

  if (revision->kind != svn_opt_revision_working)
    {
      SVN_ERR(svn_wc_get_pristine_contents2(&input, wc_ctx, local_abspath,
                                            result_pool, scratch_pool));
      if (input == NULL)
        return svn_error_createf(SVN_ERR_ILLEGAL_TARGET, NULL,
                 _("'%s' has no base revision until it is committed"),
                 svn_dirent_local_style(local_abspath, scratch_pool));

      SVN_ERR(svn_wc_get_pristine_props(&props, wc_ctx, local_abspath,
                                        scratch_pool, scratch_pool));
    }
  else
    {
      svn_wc_status3_t *status;

      SVN_ERR(svn_stream_open_readonly(&input, local_abspath, scratch_pool,
                                       result_pool));

      SVN_ERR(svn_wc_prop_list2(&props, wc_ctx, local_abspath, scratch_pool,
                                scratch_pool));
      SVN_ERR(svn_wc_status3(&status, wc_ctx, local_abspath, scratch_pool,
                             scratch_pool));
      if (status->text_status != svn_wc_status_normal)
        local_mod = TRUE;
    }

  eol_style = apr_hash_get(props, SVN_PROP_EOL_STYLE,
                           APR_HASH_KEY_STRING);
  keywords = apr_hash_get(props, SVN_PROP_KEYWORDS,
                          APR_HASH_KEY_STRING);
  special = apr_hash_get(props, SVN_PROP_SPECIAL,
                         APR_HASH_KEY_STRING);

  if (eol_style)
    svn_subst_eol_style_from_value(&style, &eol, eol_style->data);

  if (local_mod && (! special))
    {
      /* Use the modified time from the working copy if
         the file */
      SVN_ERR(svn_io_file_affected_time(&tm, local_abspath, scratch_pool));
    }
  else
    {
      SVN_ERR(svn_wc__node_get_changed_info(NULL, &tm, NULL, wc_ctx,
                                            local_abspath, scratch_pool,
                                            scratch_pool));
    }

  if (keywords)
    {
      svn_revnum_t changed_rev;
      const char *rev_str;
      const char *author;
      const char *url;

      SVN_ERR(svn_wc__node_get_changed_info(&changed_rev, NULL, &author, wc_ctx,
                                            local_abspath, scratch_pool,
                                            scratch_pool));
      SVN_ERR(svn_wc__node_get_url(&url, wc_ctx, local_abspath, scratch_pool,
                                   scratch_pool));

      if (local_mod)
        {
          /* For locally modified files, we'll append an 'M'
             to the revision number, and set the author to
             "(local)" since we can't always determine the
             current user's username */
          rev_str = apr_psprintf(scratch_pool, "%ldM", changed_rev);
          author = _("(local)");
        }
      else
        {
          rev_str = apr_psprintf(scratch_pool, "%ld", changed_rev);
        }

      SVN_ERR(svn_subst_build_keywords2(&kw, keywords->data, rev_str, url, tm,
                                        author, scratch_pool));
    }

  /* Wrap the output stream if translation is needed. */
  if (eol != NULL || kw != NULL)
    input = svn_subst_stream_translated(
      input,
      (eol_style && normalize_eols) ? SVN_SUBST_NATIVE_EOL_STR : eol,
      FALSE, kw, expand_keywords, result_pool);

  *normal_stream = input;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_client_cat2(svn_stream_t *out,
                const char *path_or_url,
                const svn_opt_revision_t *peg_revision,
                const svn_opt_revision_t *revision,
                svn_client_ctx_t *ctx,
                apr_pool_t *pool)
{
  svn_ra_session_t *ra_session;
  svn_revnum_t rev;
  svn_string_t *eol_style;
  svn_string_t *keywords;
  apr_hash_t *props;
  const char *url;
  svn_stream_t *output = out;
  svn_error_t *err;

  /* ### Inconsistent default revision logic in this command. */
  if (peg_revision->kind == svn_opt_revision_unspecified)
    {
      peg_revision = svn_cl__rev_default_to_head_or_working(peg_revision,
                                                            path_or_url);
      revision = svn_cl__rev_default_to_head_or_base(revision, path_or_url);
    }
  else
    {
      peg_revision = svn_cl__rev_default_to_head_or_working(peg_revision,
                                                            path_or_url);
      revision = svn_cl__rev_default_to_peg(revision, peg_revision);
    }

  if (! svn_path_is_url(path_or_url)
      && SVN_CLIENT__REVKIND_IS_LOCAL_TO_WC(peg_revision->kind)
      && SVN_CLIENT__REVKIND_IS_LOCAL_TO_WC(revision->kind))
    {
      const char *local_abspath;
      svn_stream_t *normal_stream;

      SVN_ERR(svn_dirent_get_absolute(&local_abspath, path_or_url, pool));
      SVN_ERR(svn_client__get_normalized_stream(&normal_stream, ctx->wc_ctx,
                                            local_abspath, revision, TRUE, FALSE,
                                            ctx->cancel_func, ctx->cancel_baton,
                                            pool, pool));

      /* We don't promise to close output, so disown it to ensure we don't. */
      output = svn_stream_disown(output, pool);

      return svn_error_trace(svn_stream_copy3(normal_stream, output,
                                              ctx->cancel_func,
                                              ctx->cancel_baton, pool));
    }

  /* Get an RA plugin for this filesystem object. */
  SVN_ERR(svn_client__ra_session_from_path(&ra_session, &rev,
                                           &url, path_or_url, NULL,
                                           peg_revision,
                                           revision, ctx, pool));

  /* Grab some properties we need to know in order to figure out if anything
     special needs to be done with this file. */
  err = svn_ra_get_file(ra_session, "", rev, NULL, NULL, &props, pool);
  if (err)
    {
      if (err->apr_err == SVN_ERR_FS_NOT_FILE)
        {
          return svn_error_createf(SVN_ERR_CLIENT_IS_DIRECTORY, err,
                                   _("URL '%s' refers to a directory"), url);
        }
      else
        {
          return svn_error_trace(err);
        }
    }

  eol_style = apr_hash_get(props, SVN_PROP_EOL_STYLE, APR_HASH_KEY_STRING);
  keywords = apr_hash_get(props, SVN_PROP_KEYWORDS, APR_HASH_KEY_STRING);

  if (eol_style || keywords)
    {
      /* It's a file with no special eol style or keywords. */
      svn_subst_eol_style_t eol;
      const char *eol_str;
      apr_hash_t *kw;

      if (eol_style)
        svn_subst_eol_style_from_value(&eol, &eol_str, eol_style->data);
      else
        {
          eol = svn_subst_eol_style_none;
          eol_str = NULL;
        }


      if (keywords)
        {
          svn_string_t *cmt_rev, *cmt_date, *cmt_author;
          apr_time_t when = 0;

          cmt_rev = apr_hash_get(props, SVN_PROP_ENTRY_COMMITTED_REV,
                                 APR_HASH_KEY_STRING);
          cmt_date = apr_hash_get(props, SVN_PROP_ENTRY_COMMITTED_DATE,
                                  APR_HASH_KEY_STRING);
          cmt_author = apr_hash_get(props, SVN_PROP_ENTRY_LAST_AUTHOR,
                                    APR_HASH_KEY_STRING);
          if (cmt_date)
            SVN_ERR(svn_time_from_cstring(&when, cmt_date->data, pool));

          SVN_ERR(svn_subst_build_keywords2
                  (&kw, keywords->data,
                   cmt_rev->data,
                   url,
                   when,
                   cmt_author ? cmt_author->data : NULL,
                   pool));
        }
      else
        kw = NULL;

      /* Interject a translating stream */
      output = svn_subst_stream_translated(svn_stream_disown(out, pool),
                                           eol_str, FALSE, kw, TRUE, pool);
    }

  SVN_ERR(svn_ra_get_file(ra_session, "", rev, output, NULL, NULL, pool));

  if (out != output)
    /* Close the interjected stream */
    SVN_ERR(svn_stream_close(output));

  return SVN_NO_ERROR;
}

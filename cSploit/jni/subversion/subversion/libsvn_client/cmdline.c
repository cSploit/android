/*
 * cmdline.c:  command-line processing
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
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_opt.h"
#include "svn_utf.h"

#include "client.h"

#include "private/svn_opt_private.h"

#include "svn_private_config.h"


/*** Code. ***/

#define DEFAULT_ARRAY_SIZE 5

/* Return true iff ARG is a repository-relative URL: specifically that
 * it starts with the characters "^/".
 * ARG is in UTF-8 encoding.
 * Do not check whether ARG is properly URI-encoded, canonical, or valid
 * in any other way. */
static svn_boolean_t
arg_is_repos_relative_url(const char *arg)
{
  return (0 == strncmp("^/", arg, 2));
}

/* Set *ABSOLUTE_URL to the absolute URL represented by RELATIVE_URL
 * relative to REPOS_ROOT_URL.
 * *ABSOLUTE_URL will end with a peg revision specifier if RELATIVE_URL did.
 * RELATIVE_URL is in repository-relative syntax:
 * "^/[REL-URL][@PEG]",
 * REPOS_ROOT_URL is the absolute URL of the repository root.
 * All strings are in UTF-8 encoding.
 * Allocate *ABSOLUTE_URL in POOL.
 *
 * REPOS_ROOT_URL and RELATIVE_URL do not have to be properly URI-encoded,
 * canonical, or valid in any other way.  The caller is expected to perform
 * canonicalization on *ABSOLUTE_URL after the call to the function.
 */
static svn_error_t *
resolve_repos_relative_url(const char **absolute_url,
                           const char *relative_url,
                           const char *repos_root_url,
                           apr_pool_t *pool)
{
  if (! arg_is_repos_relative_url(relative_url))
    return svn_error_createf(SVN_ERR_BAD_URL, NULL,
                             _("Improper relative URL '%s'"),
                             relative_url);

  /* No assumptions are made about the canonicalization of the input
   * arguments, it is presumed that the output will be canonicalized after
   * this function, which will remove any duplicate path separator.
   */
  *absolute_url = apr_pstrcat(pool, repos_root_url, relative_url + 1,
                              (char *)NULL);

  return SVN_NO_ERROR;
}


/* Attempt to find the repository root url for TARGET, possibly using CTX for
 * authentication.  If one is found and *ROOT_URL is not NULL, then just check
 * that the root url for TARGET matches the value given in *ROOT_URL and
 * return an error if it does not.  If one is found and *ROOT_URL is NULL then
 * set *ROOT_URL to the root url for TARGET, allocated from POOL.
 * If a root url is not found for TARGET because it does not exist in the
 * repository, then return with no error.
 *
 * TARGET is a UTF-8 encoded string that is fully canonicalized and escaped.
 */
static svn_error_t *
check_root_url_of_target(const char **root_url,
                         const char *target,
                         svn_client_ctx_t *ctx,
                         apr_pool_t *pool)
{
  svn_error_t *err;
  const char *tmp_root_url;
  const char *truepath;
  svn_opt_revision_t opt_rev;

  SVN_ERR(svn_opt_parse_path(&opt_rev, &truepath, target, pool));
  if (!svn_path_is_url(truepath))
    SVN_ERR(svn_dirent_get_absolute(&truepath, truepath, pool));

  err =  svn_client__get_repos_root(&tmp_root_url, truepath,
                                    ctx, pool, pool);

  if (err)
    {
      /* It is OK if the given target does not exist, it just means
       * we will not be able to determine the root url from this particular
       * argument.
       *
       * If the target itself is a URL to a repository that does not exist,
       * that's fine, too. The callers will deal with this argument in an
       * appropriate manter if it does not make any sense.
       *
       * Also tolerate locally added targets ("bad revision" error).
       */
      if ((err->apr_err == SVN_ERR_ENTRY_NOT_FOUND)
          || (err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
          || (err->apr_err == SVN_ERR_WC_NOT_WORKING_COPY)
          || (err->apr_err == SVN_ERR_RA_LOCAL_REPOS_OPEN_FAILED)
          || (err->apr_err == SVN_ERR_CLIENT_BAD_REVISION))
        {
          svn_error_clear(err);
          return SVN_NO_ERROR;
        }
      else
        return svn_error_trace(err);
     }

   if (*root_url && tmp_root_url)
     {
       if (strcmp(*root_url, tmp_root_url) != 0)
         return svn_error_createf(SVN_ERR_ILLEGAL_TARGET, NULL,
                                  _("All non-relative targets must have "
                                    "the same root URL"));
     }
   else
     *root_url = tmp_root_url;

   return SVN_NO_ERROR;
}

/* Note: This is substantially copied from svn_opt__args_to_target_array() in
 * order to move to libsvn_client while maintaining backward compatibility. */
svn_error_t *
svn_client_args_to_target_array2(apr_array_header_t **targets_p,
                                 apr_getopt_t *os,
                                 const apr_array_header_t *known_targets,
                                 svn_client_ctx_t *ctx,
                                 svn_boolean_t keep_last_origpath_on_truepath_collision,
                                 apr_pool_t *pool)
{
  int i;
  svn_boolean_t rel_url_found = FALSE;
  const char *root_url = NULL;
  svn_error_t *err = SVN_NO_ERROR;
  apr_array_header_t *input_targets =
    apr_array_make(pool, DEFAULT_ARRAY_SIZE, sizeof(const char *));
  apr_array_header_t *output_targets =
    apr_array_make(pool, DEFAULT_ARRAY_SIZE, sizeof(const char *));
  apr_array_header_t *reserved_names = NULL;

  /* Step 1:  create a master array of targets that are in UTF-8
     encoding, and come from concatenating the targets left by apr_getopt,
     plus any extra targets (e.g., from the --targets switch.)
     If any of the targets are relative urls, then set the rel_url_found
     flag.*/

  for (; os->ind < os->argc; os->ind++)
    {
      /* The apr_getopt targets are still in native encoding. */
      const char *raw_target = os->argv[os->ind];
      const char *utf8_target;

      SVN_ERR(svn_utf_cstring_to_utf8(&utf8_target,
                                      raw_target, pool));

      if (arg_is_repos_relative_url(utf8_target))
        rel_url_found = TRUE;

      APR_ARRAY_PUSH(input_targets, const char *) = utf8_target;
    }

  if (known_targets)
    {
      for (i = 0; i < known_targets->nelts; i++)
        {
          /* The --targets array have already been converted to UTF-8,
             because we needed to split up the list with svn_cstring_split. */
          const char *utf8_target = APR_ARRAY_IDX(known_targets,
                                                  i, const char *);

          if (arg_is_repos_relative_url(utf8_target))
            rel_url_found = TRUE;

          APR_ARRAY_PUSH(input_targets, const char *) = utf8_target;
        }
    }

  /* Step 2:  process each target.  */

  for (i = 0; i < input_targets->nelts; i++)
    {
      const char *utf8_target = APR_ARRAY_IDX(input_targets, i, const char *);

      /* Relative urls will be canonicalized when they are resolved later in
       * the function
       */
      if (arg_is_repos_relative_url(utf8_target))
        {
          APR_ARRAY_PUSH(output_targets, const char *) = utf8_target;
        }
      else
        {
          const char *true_target;
          const char *peg_rev;
          const char *target;

          /*
           * This is needed so that the target can be properly canonicalized,
           * otherwise the canonicalization does not treat a ".@BASE" as a "."
           * with a BASE peg revision, and it is not canonicalized to "@BASE".
           * If any peg revision exists, it is appended to the final
           * canonicalized path or URL.  Do not use svn_opt_parse_path()
           * because the resulting peg revision is a structure that would have
           * to be converted back into a string.  Converting from a string date
           * to the apr_time_t field in the svn_opt_revision_value_t and back to
           * a string would not necessarily preserve the exact bytes of the
           * input date, so its easier just to keep it in string form.
           */
          SVN_ERR(svn_opt__split_arg_at_peg_revision(&true_target, &peg_rev,
                                                     utf8_target, pool));

          /* URLs and wc-paths get treated differently. */
          if (svn_path_is_url(true_target))
            {
              SVN_ERR(svn_opt__arg_canonicalize_url(&true_target,
                                                    true_target, pool));
            }
          else  /* not a url, so treat as a path */
            {
              const char *base_name;
              const char *original_target;

              original_target = svn_dirent_internal_style(true_target, pool);
              SVN_ERR(svn_opt__arg_canonicalize_path(&true_target,
                                                     true_target, pool));

              /* There are two situations in which a 'truepath-conversion'
                 (case-canonicalization to on-disk path on case-insensitive
                 filesystem) needs to be undone:

                 1. If KEEP_LAST_ORIGPATH_ON_TRUEPATH_COLLISION is TRUE, and
                    this is the last target of a 2-element target list, and
                    both targets have the same truepath. */
              if (keep_last_origpath_on_truepath_collision
                  && input_targets->nelts == 2 && i == 1
                  && strcmp(original_target, true_target) != 0)
                {
                  const char *src_truepath = APR_ARRAY_IDX(output_targets,
                                                           0,
                                                           const char *);
                  if (strcmp(src_truepath, true_target) == 0)
                    true_target = original_target;
                }

              /* 2. If there is an exact match in the wc-db without a
                    corresponding on-disk path (e.g. a scheduled-for-delete
                    file only differing in case from an on-disk file). */
              if (strcmp(original_target, true_target) != 0)
                {
                  const char *target_abspath;
                  svn_node_kind_t kind;
                  svn_error_t *err2;

                  SVN_ERR(svn_dirent_get_absolute(&target_abspath,
                                                  original_target, pool));
                  err2 = svn_wc_read_kind(&kind, ctx->wc_ctx, target_abspath,
                                          FALSE, pool);
                  if (err2
                      && (err2->apr_err == SVN_ERR_WC_NOT_WORKING_COPY
                          || err2->apr_err == SVN_ERR_WC_UPGRADE_REQUIRED))
                    {
                      svn_error_clear(err2);
                    }
                  else
                    {
                      SVN_ERR(err2);
                      /* We successfully did a lookup in the wc-db. Now see
                         if it's something interesting. */
                      if (kind == svn_node_file || kind == svn_node_dir)
                        true_target = original_target;
                    }
                }

              /* If the target has the same name as a Subversion
                 working copy administrative dir, skip it. */
              base_name = svn_dirent_basename(true_target, pool);

              if (svn_wc_is_adm_dir(base_name, pool))
                {
                  if (!reserved_names)
                    reserved_names = apr_array_make(pool, DEFAULT_ARRAY_SIZE,
                                                    sizeof(const char *));

                  APR_ARRAY_PUSH(reserved_names, const char *) = utf8_target;

                  continue;
                }
            }

          target = apr_pstrcat(pool, true_target, peg_rev, (char *)NULL);

          if (rel_url_found)
            {
              /* Later targets have priority over earlier target, I
                 don't know why, see basic_relative_url_multi_repo. */
              SVN_ERR(check_root_url_of_target(&root_url, target,
                                               ctx, pool));
            }

          APR_ARRAY_PUSH(output_targets, const char *) = target;
        }
    }

  /* Only resolve relative urls if there were some actually found earlier. */
  if (rel_url_found)
    {
      /*
       * Use the current directory's root url if one wasn't found using the
       * arguments.
       */
      if (root_url == NULL)
        {
          err = svn_client_root_url_from_path(&root_url, "", ctx, pool);
          if (err || root_url == NULL)
            return svn_error_create(SVN_ERR_WC_NOT_WORKING_COPY, err,
                                    _("Resolving '^/': no repository root "
                                      "found in the target arguments or "
                                      "in the current directory"));
        }

      *targets_p = apr_array_make(pool, output_targets->nelts,
                                  sizeof(const char *));

      for (i = 0; i < output_targets->nelts; i++)
        {
          const char *target = APR_ARRAY_IDX(output_targets, i,
                                             const char *);

          if (arg_is_repos_relative_url(target))
            {
              const char *abs_target;
              const char *true_target;
              const char *peg_rev;

              SVN_ERR(svn_opt__split_arg_at_peg_revision(&true_target, &peg_rev,
                                                         target, pool));

              SVN_ERR(resolve_repos_relative_url(&abs_target, true_target,
                                                 root_url, pool));

              SVN_ERR(svn_opt__arg_canonicalize_url(&true_target, abs_target,
                                                    pool));

              target = apr_pstrcat(pool, true_target, peg_rev, (char *)NULL);
            }

          APR_ARRAY_PUSH(*targets_p, const char *) = target;
        }
    }
  else
    *targets_p = output_targets;

  if (reserved_names && ! err)
    for (i = 0; i < reserved_names->nelts; ++i)
      err = svn_error_createf(SVN_ERR_RESERVED_FILENAME_SPECIFIED, err,
                              _("'%s' ends in a reserved name"),
                              APR_ARRAY_IDX(reserved_names, i, const char *));

  return svn_error_trace(err);
}

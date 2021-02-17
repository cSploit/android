/*
 * diff-cmd.c -- Display context diff of a file
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
#include "svn_client.h"
#include "svn_string.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_error_codes.h"
#include "svn_error.h"
#include "svn_types.h"
#include "svn_cmdline.h"
#include "svn_xml.h"
#include "cl.h"

#include "svn_private_config.h"


/*** Code. ***/

/* Convert KIND into a single character for display to the user. */
static char
kind_to_char(svn_client_diff_summarize_kind_t kind)
{
  switch (kind)
    {
      case svn_client_diff_summarize_kind_modified:
        return 'M';

      case svn_client_diff_summarize_kind_added:
        return 'A';

      case svn_client_diff_summarize_kind_deleted:
        return 'D';

      default:
        return ' ';
    }
}

/* Convert KIND into a word describing the kind to the user. */
static const char *
kind_to_word(svn_client_diff_summarize_kind_t kind)
{
  switch (kind)
    {
      case svn_client_diff_summarize_kind_modified: return "modified";
      case svn_client_diff_summarize_kind_added:    return "added";
      case svn_client_diff_summarize_kind_deleted:  return "deleted";
      default:                                      return "none";
    }
}

/* Print summary information about a given change as XML, implements the
 * svn_client_diff_summarize_func_t interface. The @a baton is a 'char *'
 * representing the either the path to the working copy root or the url
 * the path the working copy root corresponds to. */
static svn_error_t *
summarize_xml(const svn_client_diff_summarize_t *summary,
              void *baton,
              apr_pool_t *pool)
{
  /* Full path to the object being diffed.  This is created by taking the
   * baton, and appending the target's relative path. */
  const char *path = *(const char **)baton;
  svn_stringbuf_t *sb = svn_stringbuf_create("", pool);

  /* Tack on the target path, so we can differentiate between different parts
   * of the output when we're given multiple targets. */
  if (svn_path_is_url(path))
    {
      path = svn_path_url_add_component2(path, summary->path, pool);
    }
  else
    {
      path = svn_dirent_join(path, summary->path, pool);

      /* Convert non-urls to local style, so that things like ""
         show up as "." */
      path = svn_dirent_local_style(path, pool);
    }

  svn_xml_make_open_tag(&sb, pool, svn_xml_protect_pcdata, "path",
                        "kind", svn_cl__node_kind_str_xml(summary->node_kind),
                        "item", kind_to_word(summary->summarize_kind),
                        "props", summary->prop_changed ? "modified" : "none",
                        NULL);

  svn_xml_escape_cdata_cstring(&sb, path, pool);
  svn_xml_make_close_tag(&sb, pool, "path");

  return svn_cl__error_checked_fputs(sb->data, stdout);
}

/* Print summary information about a given change, implements the
 * svn_client_diff_summarize_func_t interface. */
static svn_error_t *
summarize_regular(const svn_client_diff_summarize_t *summary,
                  void *baton,
                  apr_pool_t *pool)
{
  const char *path = *(const char **)baton;

  /* Tack on the target path, so we can differentiate between different parts
   * of the output when we're given multiple targets. */
  if (svn_path_is_url(path))
    {
      path = svn_path_url_add_component2(path, summary->path, pool);
    }
  else
    {
      path = svn_dirent_join(path, summary->path, pool);

      /* Convert non-urls to local style, so that things like ""
         show up as "." */
      path = svn_dirent_local_style(path, pool);
    }

  /* Note: This output format tries to look like the output of 'svn status',
   *       thus the blank spaces where information that is not relevant to
   *       a diff summary would go. */

  SVN_ERR(svn_cmdline_printf(pool,
                             "%c%c      %s\n",
                             kind_to_char(summary->summarize_kind),
                             summary->prop_changed ? 'M' : ' ',
                             path));

  return svn_cmdline_fflush(stdout);
}

/* An svn_opt_subcommand_t to handle the 'diff' command.
   This implements the `svn_opt_subcommand_t' interface. */
svn_error_t *
svn_cl__diff(apr_getopt_t *os,
             void *baton,
             apr_pool_t *pool)
{
  svn_cl__opt_state_t *opt_state = ((svn_cl__cmd_baton_t *) baton)->opt_state;
  svn_client_ctx_t *ctx = ((svn_cl__cmd_baton_t *) baton)->ctx;
  apr_array_header_t *options;
  apr_array_header_t *targets;
  apr_file_t *outfile, *errfile;
  apr_status_t status;
  const char *old_target, *new_target;
  apr_pool_t *iterpool;
  svn_boolean_t pegged_diff = FALSE;
  int i;
  const svn_client_diff_summarize_func_t summarize_func =
    (opt_state->xml ? summarize_xml : summarize_regular);

  if (opt_state->extensions)
    options = svn_cstring_split(opt_state->extensions, " \t\n\r", TRUE, pool);
  else
    options = NULL;

  /* Get an apr_file_t representing stdout and stderr, which is where
     we'll have the external 'diff' program print to. */
  if ((status = apr_file_open_stdout(&outfile, pool)))
    return svn_error_wrap_apr(status, _("Can't open stdout"));
  if ((status = apr_file_open_stderr(&errfile, pool)))
    return svn_error_wrap_apr(status, _("Can't open stderr"));

  if (opt_state->xml)
    {
      svn_stringbuf_t *sb;

      /* Check that the --summarize is passed as well. */
      if (!opt_state->summarize)
        return svn_error_create(SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                                _("'--xml' option only valid with "
                                  "'--summarize' option"));

      SVN_ERR(svn_cl__xml_print_header("diff", pool));

      sb = svn_stringbuf_create("", pool);
      svn_xml_make_open_tag(&sb, pool, svn_xml_normal, "paths", NULL);
      SVN_ERR(svn_cl__error_checked_fputs(sb->data, stdout));
    }

  SVN_ERR(svn_cl__args_to_target_array_print_reserved(&targets, os,
                                                      opt_state->targets,
                                                      ctx, FALSE, pool));

  if (! opt_state->old_target && ! opt_state->new_target
      && (targets->nelts == 2)
      && svn_path_is_url(APR_ARRAY_IDX(targets, 0, const char *))
      && svn_path_is_url(APR_ARRAY_IDX(targets, 1, const char *))
      && opt_state->start_revision.kind == svn_opt_revision_unspecified
      && opt_state->end_revision.kind == svn_opt_revision_unspecified)
    {
      /* The 'svn diff OLD_URL[@OLDREV] NEW_URL[@NEWREV]' case matches. */

      SVN_ERR(svn_opt_parse_path(&opt_state->start_revision, &old_target,
                                 APR_ARRAY_IDX(targets, 0, const char *),
                                 pool));
      SVN_ERR(svn_opt_parse_path(&opt_state->end_revision, &new_target,
                                 APR_ARRAY_IDX(targets, 1, const char *),
                                 pool));
      targets->nelts = 0;

      if (opt_state->start_revision.kind == svn_opt_revision_unspecified)
        opt_state->start_revision.kind = svn_opt_revision_head;
      if (opt_state->end_revision.kind == svn_opt_revision_unspecified)
        opt_state->end_revision.kind = svn_opt_revision_head;
    }
  else if (opt_state->old_target)
    {
      apr_array_header_t *tmp, *tmp2;
      svn_opt_revision_t old_rev, new_rev;

      /* The 'svn diff --old=OLD[@OLDREV] [--new=NEW[@NEWREV]]
         [PATH...]' case matches. */

      tmp = apr_array_make(pool, 2, sizeof(const char *));
      APR_ARRAY_PUSH(tmp, const char *) = (opt_state->old_target);
      APR_ARRAY_PUSH(tmp, const char *) = (opt_state->new_target
                                            ? opt_state->new_target
                                           : APR_ARRAY_IDX(tmp, 0,
                                                           const char *));

      SVN_ERR(svn_cl__args_to_target_array_print_reserved(&tmp2, os, tmp,
                                                          ctx, FALSE, pool));

      /* Check if either or both targets were skipped (e.g. because they
       * were .svn directories). */
      if (tmp2->nelts < 2)
        return svn_error_create(SVN_ERR_CL_INSUFFICIENT_ARGS, NULL, NULL);

      SVN_ERR(svn_opt_parse_path(&old_rev, &old_target,
                                 APR_ARRAY_IDX(tmp2, 0, const char *),
                                 pool));
      if (old_rev.kind != svn_opt_revision_unspecified)
        opt_state->start_revision = old_rev;
      SVN_ERR(svn_opt_parse_path(&new_rev, &new_target,
                                 APR_ARRAY_IDX(tmp2, 1, const char *),
                                 pool));
      if (new_rev.kind != svn_opt_revision_unspecified)
        opt_state->end_revision = new_rev;

      if (opt_state->start_revision.kind == svn_opt_revision_unspecified)
        opt_state->start_revision.kind = svn_path_is_url(old_target)
          ? svn_opt_revision_head : svn_opt_revision_base;

      if (opt_state->end_revision.kind == svn_opt_revision_unspecified)
        opt_state->end_revision.kind = svn_path_is_url(new_target)
          ? svn_opt_revision_head : svn_opt_revision_working;
    }
  else if (opt_state->new_target)
    {
      return svn_error_create(SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                              _("'--new' option only valid with "
                                "'--old' option"));
    }
  else
    {
      svn_boolean_t working_copy_present;

      /* The 'svn diff [-r N[:M]] [TARGET[@REV]...]' case matches. */

      /* Here each target is a pegged object. Find out the starting
         and ending paths for each target. */

      svn_opt_push_implicit_dot_target(targets, pool);

      old_target = "";
      new_target = "";

      SVN_ERR(svn_cl__assert_homogeneous_target_type(targets));

      working_copy_present = ! svn_path_is_url(APR_ARRAY_IDX(targets, 0,
                                                             const char *));

      if (opt_state->start_revision.kind == svn_opt_revision_unspecified
          && working_copy_present)
        opt_state->start_revision.kind = svn_opt_revision_base;
      if (opt_state->end_revision.kind == svn_opt_revision_unspecified)
        opt_state->end_revision.kind = working_copy_present
          ? svn_opt_revision_working : svn_opt_revision_head;

      /* Determine if we need to do pegged diffs. */
      if ((opt_state->start_revision.kind != svn_opt_revision_base
           && opt_state->start_revision.kind != svn_opt_revision_working)
          || (opt_state->end_revision.kind != svn_opt_revision_base
              && opt_state->end_revision.kind != svn_opt_revision_working))
        pegged_diff = TRUE;

    }

  svn_opt_push_implicit_dot_target(targets, pool);

  iterpool = svn_pool_create(pool);

  for (i = 0; i < targets->nelts; ++i)
    {
      const char *path = APR_ARRAY_IDX(targets, i, const char *);
      const char *target1, *target2;

      svn_pool_clear(iterpool);
      if (! pegged_diff)
        {
          /* We can't be tacking URLs onto base paths! */
          if (svn_path_is_url(path))
            return svn_error_createf(SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                                     _("Path '%s' not relative to base URLs"),
                                     path);

          if (svn_path_is_url(old_target))
            target1 = svn_path_url_add_component2(
                          old_target,
                          svn_relpath_canonicalize(path, iterpool),
                          iterpool);
          else
            target1 = svn_dirent_join(old_target, path, iterpool);

          if (svn_path_is_url(new_target))
            target2 = svn_path_url_add_component2(
                          new_target,
                          svn_relpath_canonicalize(path, iterpool),
                          iterpool);
          else
            target2 = svn_dirent_join(new_target, path, iterpool);

          if (opt_state->summarize)
            SVN_ERR(svn_client_diff_summarize2
                    (target1,
                     &opt_state->start_revision,
                     target2,
                     &opt_state->end_revision,
                     opt_state->depth,
                     ! opt_state->notice_ancestry,
                     opt_state->changelists,
                     summarize_func, &target1,
                     ctx, iterpool));
          else
            SVN_ERR(svn_client_diff5
                    (options,
                     target1,
                     &(opt_state->start_revision),
                     target2,
                     &(opt_state->end_revision),
                     NULL,
                     opt_state->depth,
                     ! opt_state->notice_ancestry,
                     opt_state->no_diff_deleted,
                     opt_state->show_copies_as_adds,
                     opt_state->force,
                     opt_state->use_git_diff_format,
                     svn_cmdline_output_encoding(pool),
                     outfile,
                     errfile,
                     opt_state->changelists,
                     ctx, iterpool));
        }
      else
        {
          const char *truepath;
          svn_opt_revision_t peg_revision;

          /* First check for a peg revision. */
          SVN_ERR(svn_opt_parse_path(&peg_revision, &truepath, path,
                                     iterpool));

          /* Set the default peg revision if one was not specified. */
          if (peg_revision.kind == svn_opt_revision_unspecified)
            peg_revision.kind = svn_path_is_url(path)
              ? svn_opt_revision_head : svn_opt_revision_working;

          if (opt_state->summarize)
            SVN_ERR(svn_client_diff_summarize_peg2
                    (truepath,
                     &peg_revision,
                     &opt_state->start_revision,
                     &opt_state->end_revision,
                     opt_state->depth,
                     ! opt_state->notice_ancestry,
                     opt_state->changelists,
                     summarize_func, &truepath,
                     ctx, iterpool));
          else
            SVN_ERR(svn_client_diff_peg5
                    (options,
                     truepath,
                     &peg_revision,
                     &opt_state->start_revision,
                     &opt_state->end_revision,
                     NULL,
                     opt_state->depth,
                     ! opt_state->notice_ancestry,
                     opt_state->no_diff_deleted,
                     opt_state->show_copies_as_adds,
                     opt_state->force,
                     opt_state->use_git_diff_format,
                     svn_cmdline_output_encoding(pool),
                     outfile,
                     errfile,
                     opt_state->changelists,
                     ctx, iterpool));
        }
    }

  if (opt_state->xml)
    {
      svn_stringbuf_t *sb = svn_stringbuf_create("", pool);
      svn_xml_make_close_tag(&sb, pool, "paths");
      SVN_ERR(svn_cl__error_checked_fputs(sb->data, stdout));
      SVN_ERR(svn_cl__xml_print_footer("diff", pool));
    }

  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}

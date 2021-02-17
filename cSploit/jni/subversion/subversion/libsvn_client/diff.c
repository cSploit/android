/*
 * diff.c: comparing
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

#include <apr_strings.h>
#include <apr_pools.h>
#include <apr_hash.h>
#include "svn_types.h"
#include "svn_hash.h"
#include "svn_wc.h"
#include "svn_delta.h"
#include "svn_diff.h"
#include "svn_mergeinfo.h"
#include "svn_client.h"
#include "svn_string.h"
#include "svn_error.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_io.h"
#include "svn_utf.h"
#include "svn_pools.h"
#include "svn_config.h"
#include "svn_props.h"
#include "svn_time.h"
#include "svn_sorts.h"
#include "svn_subst.h"
#include "client.h"

#include "private/svn_wc_private.h"

#include "svn_private_config.h"


/*
 * Constant separator strings
 */
static const char equal_string[] =
  "===================================================================";
static const char under_string[] =
  "___________________________________________________________________";


/*-----------------------------------------------------------------*/

/* Utilities */

/* Wrapper for apr_file_printf(), which see.  FORMAT is a utf8-encoded
   string after it is formatted, so this function can convert it to
   ENCODING before printing. */
static svn_error_t *
file_printf_from_utf8(apr_file_t *fptr, const char *encoding,
                      const char *format, ...)
  __attribute__ ((format(printf, 3, 4)));
static svn_error_t *
file_printf_from_utf8(apr_file_t *fptr, const char *encoding,
                      const char *format, ...)
{
  va_list ap;
  const char *buf, *buf_apr;

  va_start(ap, format);
  buf = apr_pvsprintf(apr_file_pool_get(fptr), format, ap);
  va_end(ap);

  SVN_ERR(svn_utf_cstring_from_utf8_ex2(&buf_apr, buf, encoding,
                                        apr_file_pool_get(fptr)));

  return svn_io_file_write_full(fptr, buf_apr, strlen(buf_apr),
                                NULL, apr_file_pool_get(fptr));
}


/* A helper function for display_prop_diffs.  Output the differences between
   the mergeinfo stored in ORIG_MERGEINFO_VAL and NEW_MERGEINFO_VAL in a
   human-readable form to FILE, using ENCODING.  Use POOL for temporary
   allocations. */
static svn_error_t *
display_mergeinfo_diff(const char *old_mergeinfo_val,
                       const char *new_mergeinfo_val,
                       const char *encoding,
                       apr_file_t *file,
                       apr_pool_t *pool)
{
  apr_hash_t *old_mergeinfo_hash, *new_mergeinfo_hash, *added, *deleted;
  apr_hash_index_t *hi;

  if (old_mergeinfo_val)
    SVN_ERR(svn_mergeinfo_parse(&old_mergeinfo_hash, old_mergeinfo_val, pool));
  else
    old_mergeinfo_hash = NULL;

  if (new_mergeinfo_val)
    SVN_ERR(svn_mergeinfo_parse(&new_mergeinfo_hash, new_mergeinfo_val, pool));
  else
    new_mergeinfo_hash = NULL;

  SVN_ERR(svn_mergeinfo_diff(&deleted, &added, old_mergeinfo_hash,
                             new_mergeinfo_hash,
                             TRUE, pool));

  for (hi = apr_hash_first(pool, deleted);
       hi; hi = apr_hash_next(hi))
    {
      const char *from_path = svn__apr_hash_index_key(hi);
      apr_array_header_t *merge_revarray = svn__apr_hash_index_val(hi);
      svn_string_t *merge_revstr;

      SVN_ERR(svn_rangelist_to_string(&merge_revstr, merge_revarray, pool));

      SVN_ERR(file_printf_from_utf8(file, encoding,
                                    _("   Reverse-merged %s:r%s%s"),
                                    from_path, merge_revstr->data,
                                    APR_EOL_STR));
    }

  for (hi = apr_hash_first(pool, added);
       hi; hi = apr_hash_next(hi))
    {
      const char *from_path = svn__apr_hash_index_key(hi);
      apr_array_header_t *merge_revarray = svn__apr_hash_index_val(hi);
      svn_string_t *merge_revstr;

      SVN_ERR(svn_rangelist_to_string(&merge_revstr, merge_revarray, pool));

      SVN_ERR(file_printf_from_utf8(file, encoding,
                                    _("   Merged %s:r%s%s"),
                                    from_path, merge_revstr->data,
                                    APR_EOL_STR));
    }

  return SVN_NO_ERROR;
}

#define MAKE_ERR_BAD_RELATIVE_PATH(path, relative_to_dir) \
        svn_error_createf(SVN_ERR_BAD_RELATIVE_PATH, NULL, \
                          _("Path '%s' must be an immediate child of " \
                            "the directory '%s'"), path, relative_to_dir)

/* A helper function used by display_prop_diffs.
   TOKEN is a string holding a property value.
   If TOKEN is empty, or is already terminated by an EOL marker,
   return TOKEN unmodified. Else, return a new string consisting
   of the concatenation of TOKEN and the system's default EOL marker.
   The new string is allocated from POOL.
   If HAD_EOL is not NULL, indicate in *HAD_EOL if the token had a EOL. */
static const svn_string_t *
maybe_append_eol(const svn_string_t *token, svn_boolean_t *had_eol,
                 apr_pool_t *pool)
{
  const char *curp;

  if (had_eol)
    *had_eol = FALSE;

  if (token->len == 0)
    return token;

  curp = token->data + token->len - 1;
  if (*curp == '\r')
    {
      if (had_eol)
        *had_eol = TRUE;
      return token;
    }
  else if (*curp != '\n')
    {
      return svn_string_createf(pool, "%s%s", token->data, APR_EOL_STR);
    }
  else
    {
      if (had_eol)
        *had_eol = TRUE;
      return token;
    }
}

/* Adjust PATH to be relative to the repository root beneath ORIG_TARGET,
 * using RA_SESSION and WC_CTX, and return the result in *ADJUSTED_PATH.
 * ORIG_TARGET is one of the original targets passed to the diff command,
 * and may be used to derive leading path components missing from PATH.
 * WC_ROOT_ABSPATH is the absolute path to the root directory of a working
 * copy involved in a repos-wc diff, and may be NULL.
 * Do all allocations in POOL. */
static svn_error_t *
adjust_relative_to_repos_root(const char **adjusted_path,
                              const char *path,
                              const char *orig_target,
                              svn_ra_session_t *ra_session,
                              svn_wc_context_t *wc_ctx,
                              const char *wc_root_abspath,
                              apr_pool_t *pool)
{
  const char *local_abspath;
  const char *orig_relpath;
  const char *child_relpath;

  if (! ra_session)
    {
      /* We're doing a WC-WC diff, so we can retrieve all information we
       * need from the working copy. */
      SVN_ERR(svn_dirent_get_absolute(&local_abspath, path, pool));
      SVN_ERR(svn_wc__node_get_repos_relpath(adjusted_path, wc_ctx,
                                             local_abspath, pool, pool));
      return SVN_NO_ERROR;
    }

  /* Now deal with the repos-repos and repos-wc diff cases.
   * We need to make PATH appear as a child of ORIG_TARGET.
   * ORIG_TARGET is either a URL or a path to a working copy. First,
   * find out what ORIG_TARGET looks like relative to the repository root.*/
  if (svn_path_is_url(orig_target))
    SVN_ERR(svn_ra_get_path_relative_to_root(ra_session,
                                             &orig_relpath,
                                             orig_target, pool));
  else
    {
      const char *orig_abspath;

      SVN_ERR(svn_dirent_get_absolute(&orig_abspath, orig_target, pool));
      SVN_ERR(svn_wc__node_get_repos_relpath(&orig_relpath, wc_ctx,
                                             orig_abspath, pool, pool));
    }

  /* PATH is either a child of the working copy involved in the diff (in
   * the repos-wc diff case), or it's a relative path we can readily use
   * (in either of the repos-repos and repos-wc diff cases). */
  child_relpath = NULL;
  if (wc_root_abspath)
    {
      SVN_ERR(svn_dirent_get_absolute(&local_abspath, path, pool));
      child_relpath = svn_dirent_is_child(wc_root_abspath, local_abspath, pool);
    }
  if (child_relpath == NULL)
    child_relpath = path;

  *adjusted_path = svn_relpath_join(orig_relpath, child_relpath, pool);

  return SVN_NO_ERROR;
}

/* Adjust PATH, ORIG_PATH_1 and ORIG_PATH_2, representing the changed file
 * and the two original targets passed to the diff command, to handle the
 * case when we're dealing with different anchors. RELATIVE_TO_DIR is the
 * directory the diff target should be considered relative to. All
 * allocations are done in POOL. */
static svn_error_t *
adjust_paths_for_diff_labels(const char **path,
                             const char **orig_path_1,
                             const char **orig_path_2,
                             const char *relative_to_dir,
                             apr_pool_t *pool)
{
  apr_size_t len;
  const char *new_path = *path;
  const char *new_path1 = *orig_path_1;
  const char *new_path2 = *orig_path_2;

  /* ### Holy cow.  Due to anchor/target weirdness, we can't
     simply join diff_cmd_baton->orig_path_1 with path, ditto for
     orig_path_2.  That will work when they're directory URLs, but
     not for file URLs.  Nor can we just use anchor1 and anchor2
     from do_diff(), at least not without some more logic here.
     What a nightmare.

     For now, to distinguish the two paths, we'll just put the
     unique portions of the original targets in parentheses after
     the received path, with ellipses for handwaving.  This makes
     the labels a bit clumsy, but at least distinctive.  Better
     solutions are possible, they'll just take more thought. */

  len = strlen(svn_dirent_get_longest_ancestor(new_path1, new_path2, pool));
  new_path1 = new_path1 + len;
  new_path2 = new_path2 + len;

  /* ### Should diff labels print paths in local style?  Is there
     already a standard for this?  In any case, this code depends on
     a particular style, so not calling svn_dirent_local_style() on the
     paths below.*/
  if (new_path1[0] == '\0')
    new_path1 = apr_psprintf(pool, "%s", new_path);
  else if (new_path1[0] == '/')
    new_path1 = apr_psprintf(pool, "%s\t(...%s)", new_path, new_path1);
  else
    new_path1 = apr_psprintf(pool, "%s\t(.../%s)", new_path, new_path1);

  if (new_path2[0] == '\0')
    new_path2 = apr_psprintf(pool, "%s", new_path);
  else if (new_path2[0] == '/')
    new_path2 = apr_psprintf(pool, "%s\t(...%s)", new_path, new_path2);
  else
    new_path2 = apr_psprintf(pool, "%s\t(.../%s)", new_path, new_path2);

  if (relative_to_dir)
    {
      /* Possibly adjust the paths shown in the output (see issue #2723). */
      const char *child_path = svn_dirent_is_child(relative_to_dir, new_path,
                                                   pool);

      if (child_path)
        new_path = child_path;
      else if (!svn_path_compare_paths(relative_to_dir, new_path))
        new_path = ".";
      else
        return MAKE_ERR_BAD_RELATIVE_PATH(new_path, relative_to_dir);

      child_path = svn_dirent_is_child(relative_to_dir, new_path1, pool);

      if (child_path)
        new_path1 = child_path;
      else if (!svn_path_compare_paths(relative_to_dir, new_path1))
        new_path1 = ".";
      else
        return MAKE_ERR_BAD_RELATIVE_PATH(new_path1, relative_to_dir);

      child_path = svn_dirent_is_child(relative_to_dir, new_path2, pool);

      if (child_path)
        new_path2 = child_path;
      else if (!svn_path_compare_paths(relative_to_dir, new_path2))
        new_path2 = ".";
      else
        return MAKE_ERR_BAD_RELATIVE_PATH(new_path2, relative_to_dir);
    }
  *path = new_path;
  *orig_path_1 = new_path1;
  *orig_path_2 = new_path2;

  return SVN_NO_ERROR;
}


/* Generate a label for the diff output for file PATH at revision REVNUM.
   If REVNUM is invalid then it is assumed to be the current working
   copy.  Assumes the paths are already in the desired style (local
   vs internal).  Allocate the label in POOL. */
static const char *
diff_label(const char *path,
           svn_revnum_t revnum,
           apr_pool_t *pool)
{
  const char *label;
  if (revnum != SVN_INVALID_REVNUM)
    label = apr_psprintf(pool, _("%s\t(revision %ld)"), path, revnum);
  else
    label = apr_psprintf(pool, _("%s\t(working copy)"), path);

  return label;
}

/* Print a git diff header for an addition within a diff between PATH1 and
 * PATH2 to the stream OS using HEADER_ENCODING.
 * All allocations are done in RESULT_POOL. */
static svn_error_t *
print_git_diff_header_added(svn_stream_t *os, const char *header_encoding,
                            const char *path1, const char *path2,
                            apr_pool_t *result_pool)
{
  SVN_ERR(svn_stream_printf_from_utf8(os, header_encoding, result_pool,
                                      "diff --git a/%s b/%s%s",
                                      path1, path2, APR_EOL_STR));
  SVN_ERR(svn_stream_printf_from_utf8(os, header_encoding, result_pool,
                                      "new file mode 10644" APR_EOL_STR));
  return SVN_NO_ERROR;
}

/* Print a git diff header for a deletion within a diff between PATH1 and
 * PATH2 to the stream OS using HEADER_ENCODING.
 * All allocations are done in RESULT_POOL. */
static svn_error_t *
print_git_diff_header_deleted(svn_stream_t *os, const char *header_encoding,
                              const char *path1, const char *path2,
                              apr_pool_t *result_pool)
{
  SVN_ERR(svn_stream_printf_from_utf8(os, header_encoding, result_pool,
                                      "diff --git a/%s b/%s%s",
                                      path1, path2, APR_EOL_STR));
  SVN_ERR(svn_stream_printf_from_utf8(os, header_encoding, result_pool,
                                      "deleted file mode 10644"
                                      APR_EOL_STR));
  return SVN_NO_ERROR;
}

/* Print a git diff header for a copy from COPYFROM_PATH to PATH to the stream
 * OS using HEADER_ENCODING. All allocations are done in RESULT_POOL. */
static svn_error_t *
print_git_diff_header_copied(svn_stream_t *os, const char *header_encoding,
                             const char *copyfrom_path, const char *path,
                             apr_pool_t *result_pool)
{
  SVN_ERR(svn_stream_printf_from_utf8(os, header_encoding, result_pool,
                                      "diff --git a/%s b/%s%s",
                                      copyfrom_path, path, APR_EOL_STR));
  SVN_ERR(svn_stream_printf_from_utf8(os, header_encoding, result_pool,
                                      "copy from %s%s", copyfrom_path,
                                      APR_EOL_STR));
  SVN_ERR(svn_stream_printf_from_utf8(os, header_encoding, result_pool,
                                      "copy to %s%s", path, APR_EOL_STR));
  return SVN_NO_ERROR;
}

/* Print a git diff header for a rename from COPYFROM_PATH to PATH to the
 * stream OS using HEADER_ENCODING. All allocations are done in RESULT_POOL. */
static svn_error_t *
print_git_diff_header_renamed(svn_stream_t *os, const char *header_encoding,
                              const char *copyfrom_path, const char *path,
                              apr_pool_t *result_pool)
{
  SVN_ERR(svn_stream_printf_from_utf8(os, header_encoding, result_pool,
                                      "diff --git a/%s b/%s%s",
                                      copyfrom_path, path, APR_EOL_STR));
  SVN_ERR(svn_stream_printf_from_utf8(os, header_encoding, result_pool,
                                      "rename from %s%s", copyfrom_path,
                                      APR_EOL_STR));
  SVN_ERR(svn_stream_printf_from_utf8(os, header_encoding, result_pool,
                                      "rename to %s%s", path, APR_EOL_STR));
  return SVN_NO_ERROR;
}

/* Print a git diff header for a modification within a diff between PATH1 and
 * PATH2 to the stream OS using HEADER_ENCODING.
 * All allocations are done in RESULT_POOL. */
static svn_error_t *
print_git_diff_header_modified(svn_stream_t *os, const char *header_encoding,
                               const char *path1, const char *path2,
                               apr_pool_t *result_pool)
{
  SVN_ERR(svn_stream_printf_from_utf8(os, header_encoding, result_pool,
                                      "diff --git a/%s b/%s%s",
                                      path1, path2, APR_EOL_STR));
  return SVN_NO_ERROR;
}

/* Print a git diff header showing the OPERATION to the stream OS using
 * HEADER_ENCODING. Return suitable diff labels for the git diff in *LABEL1
 * and *LABEL2. REPOS_RELPATH1 and REPOS_RELPATH2 are relative to reposroot.
 * are the paths passed to the original diff command. REV1 and REV2 are
 * revisions being diffed. COPYFROM_PATH indicates where the diffed item
 * was copied from. RA_SESSION and WC_CTX are used to adjust paths in the
 * headers to be relative to the repository root.
 * WC_ROOT_ABSPATH is the absolute path to the root directory of a working
 * copy involved in a repos-wc diff, and may be NULL.
 * Use SCRATCH_POOL for temporary allocations. */
static svn_error_t *
print_git_diff_header(svn_stream_t *os,
                      const char **label1, const char **label2,
                      svn_diff_operation_kind_t operation,
                      const char *repos_relpath1,
                      const char *repos_relpath2,
                      svn_revnum_t rev1,
                      svn_revnum_t rev2,
                      const char *copyfrom_path,
                      const char *header_encoding,
                      svn_ra_session_t *ra_session,
                      svn_wc_context_t *wc_ctx,
                      const char *wc_root_abspath,
                      apr_pool_t *scratch_pool)
{
  if (operation == svn_diff_op_deleted)
    {
      SVN_ERR(print_git_diff_header_deleted(os, header_encoding,
                                            repos_relpath1, repos_relpath2,
                                            scratch_pool));
      *label1 = diff_label(apr_psprintf(scratch_pool, "a/%s", repos_relpath1),
                           rev1, scratch_pool);
      *label2 = diff_label("/dev/null", rev2, scratch_pool);

    }
  else if (operation == svn_diff_op_copied)
    {
      SVN_ERR(print_git_diff_header_copied(os, header_encoding,
                                           copyfrom_path, repos_relpath2,
                                           scratch_pool));
      *label1 = diff_label(apr_psprintf(scratch_pool, "a/%s", copyfrom_path),
                           rev1, scratch_pool);
      *label2 = diff_label(apr_psprintf(scratch_pool, "b/%s", repos_relpath2),
                           rev2, scratch_pool);
    }
  else if (operation == svn_diff_op_added)
    {
      SVN_ERR(print_git_diff_header_added(os, header_encoding,
                                          repos_relpath1, repos_relpath2,
                                          scratch_pool));
      *label1 = diff_label("/dev/null", rev1, scratch_pool);
      *label2 = diff_label(apr_psprintf(scratch_pool, "b/%s", repos_relpath2),
                           rev2, scratch_pool);
    }
  else if (operation == svn_diff_op_modified)
    {
      SVN_ERR(print_git_diff_header_modified(os, header_encoding,
                                             repos_relpath1, repos_relpath2,
                                             scratch_pool));
      *label1 = diff_label(apr_psprintf(scratch_pool, "a/%s", repos_relpath1),
                           rev1, scratch_pool);
      *label2 = diff_label(apr_psprintf(scratch_pool, "b/%s", repos_relpath2),
                           rev2, scratch_pool);
    }
  else if (operation == svn_diff_op_moved)
    {
      SVN_ERR(print_git_diff_header_renamed(os, header_encoding,
                                            copyfrom_path, repos_relpath2,
                                            scratch_pool));
      *label1 = diff_label(apr_psprintf(scratch_pool, "a/%s", copyfrom_path),
                           rev1, scratch_pool);
      *label2 = diff_label(apr_psprintf(scratch_pool, "b/%s", repos_relpath2),
                           rev2, scratch_pool);
    }

  return SVN_NO_ERROR;
}

/* A helper func that writes out verbal descriptions of property diffs
   to FILE.   Of course, the apr_file_t will probably be the 'outfile'
   passed to svn_client_diff5, which is probably stdout.

   ### FIXME needs proper docstring

   If USE_GIT_DIFF_FORMAT is TRUE, pring git diff headers, which always
   show paths relative to the repository root. RA_SESSION and WC_CTX are
   needed to normalize paths relative the repository root, and are ignored
   if USE_GIT_DIFF_FORMAT is FALSE.

   WC_ROOT_ABSPATH is the absolute path to the root directory of a working
   copy involved in a repos-wc diff, and may be NULL. */
static svn_error_t *
display_prop_diffs(const apr_array_header_t *propchanges,
                   apr_hash_t *original_props,
                   const char *path,
                   const char *orig_path1,
                   const char *orig_path2,
                   svn_revnum_t rev1,
                   svn_revnum_t rev2,
                   const char *encoding,
                   apr_file_t *file,
                   const char *relative_to_dir,
                   svn_boolean_t show_diff_header,
                   svn_boolean_t use_git_diff_format,
                   svn_ra_session_t *ra_session,
                   svn_wc_context_t *wc_ctx,
                   const char *wc_root_abspath,
                   apr_pool_t *pool)
{
  int i;
  const char *path1 = apr_pstrdup(pool, orig_path1);
  const char *path2 = apr_pstrdup(pool, orig_path2);

  if (use_git_diff_format)
    {
      SVN_ERR(adjust_relative_to_repos_root(&path1, path, orig_path1,
                                            ra_session, wc_ctx,
                                            wc_root_abspath,
                                            pool));
      SVN_ERR(adjust_relative_to_repos_root(&path2, path, orig_path2,
                                            ra_session, wc_ctx,
                                            wc_root_abspath,
                                            pool));
    }

  /* If we're creating a diff on the wc root, path would be empty. */
  if (path[0] == '\0')
    path = apr_psprintf(pool, ".");

  if (show_diff_header)
    {
      const char *label1;
      const char *label2;
      const char *adjusted_path1 = apr_pstrdup(pool, path1);
      const char *adjusted_path2 = apr_pstrdup(pool, path2);

      SVN_ERR(adjust_paths_for_diff_labels(&path, &adjusted_path1,
                                           &adjusted_path2,
                                           relative_to_dir, pool));

      label1 = diff_label(adjusted_path1, rev1, pool);
      label2 = diff_label(adjusted_path2, rev2, pool);

      /* ### Should we show the paths in platform specific format,
       * ### diff_content_changed() does not! */

      SVN_ERR(file_printf_from_utf8(file, encoding,
                                    "Index: %s" APR_EOL_STR
                                    "%s" APR_EOL_STR,
                                    path, equal_string));

      if (use_git_diff_format)
        {
          svn_stream_t *os;

          os = svn_stream_from_aprfile2(file, TRUE, pool);
          SVN_ERR(print_git_diff_header(os, &label1, &label2,
                                        svn_diff_op_modified,
                                        path1, path2, rev1, rev2, NULL,
                                        encoding, ra_session, wc_ctx,
                                        wc_root_abspath, pool));
          SVN_ERR(svn_stream_close(os));
        }

      SVN_ERR(file_printf_from_utf8(file, encoding,
                                          "--- %s" APR_EOL_STR
                                          "+++ %s" APR_EOL_STR,
                                          label1,
                                          label2));
    }

  SVN_ERR(file_printf_from_utf8(file, encoding,
                                _("%sProperty changes on: %s%s"),
                                APR_EOL_STR,
                                use_git_diff_format ? path1 : path,
                                APR_EOL_STR));

  SVN_ERR(file_printf_from_utf8(file, encoding, "%s" APR_EOL_STR,
                                under_string));

  for (i = 0; i < propchanges->nelts; i++)
    {
      const char *action;
      const svn_string_t *original_value;
      const svn_prop_t *propchange =
        &APR_ARRAY_IDX(propchanges, i, svn_prop_t);

      if (original_props)
        original_value = apr_hash_get(original_props,
                                      propchange->name, APR_HASH_KEY_STRING);
      else
        original_value = NULL;

      /* If the property doesn't exist on either side, or if it exists
         with the same value, skip it.  */
      if ((! (original_value || propchange->value))
          || (original_value && propchange->value
              && svn_string_compare(original_value, propchange->value)))
        continue;

      if (! original_value)
        action = "Added";
      else if (! propchange->value)
        action = "Deleted";
      else
        action = "Modified";
      SVN_ERR(file_printf_from_utf8(file, encoding, "%s: %s%s", action,
                                    propchange->name, APR_EOL_STR));

      if (strcmp(propchange->name, SVN_PROP_MERGEINFO) == 0)
        {
          const char *orig = original_value ? original_value->data : NULL;
          const char *val = propchange->value ? propchange->value->data : NULL;
          svn_error_t *err = display_mergeinfo_diff(orig, val, encoding,
                                                    file, pool);

          /* Issue #3896: If we can't pretty-print mergeinfo differences
             because invalid mergeinfo is present, then don't let the diff
             fail, just print the diff as any other property. */
          if (err && err->apr_err == SVN_ERR_MERGEINFO_PARSE_ERROR)
            {
              svn_error_clear(err);
            }
          else
            {
              SVN_ERR(err);
              continue;
            }
        }

      {
        svn_stream_t *os = svn_stream_from_aprfile2(file, TRUE, pool);
        svn_diff_t *diff;
        svn_diff_file_options_t options = { 0 };
        const svn_string_t *tmp;
        const svn_string_t *orig;
        const svn_string_t *val;
        svn_boolean_t val_has_eol;

        /* The last character in a property is often not a newline.
           An eol character is appended to prevent the diff API to add a
           ' \ No newline at end of file' line. We add 
           ' \ No newline at end of property' manually if needed. */
        tmp = original_value ? original_value : svn_string_create("", pool);
        orig = maybe_append_eol(tmp, NULL, pool);

        tmp = propchange->value ? propchange->value :
                                  svn_string_create("", pool);
        val = maybe_append_eol(tmp, &val_has_eol, pool);

        SVN_ERR(svn_diff_mem_string_diff(&diff, orig, val, &options, pool));

        /* UNIX patch will try to apply a diff even if the diff header
         * is missing. It tries to be helpful by asking the user for a
         * target filename when it can't determine the target filename
         * from the diff header. But there usually are no files which
         * UNIX patch could apply the property diff to, so we use "##"
         * instead of "@@" as the default hunk delimiter for property diffs.
         * We also supress the diff header. */
        SVN_ERR(svn_diff_mem_string_output_unified2(os, diff, FALSE, "##",
                                           svn_dirent_local_style(path, pool),
                                           svn_dirent_local_style(path, pool),
                                           encoding, orig, val, pool));
        SVN_ERR(svn_stream_close(os));
        if (!val_has_eol)
          {
            const char *s = "\\ No newline at end of property" APR_EOL_STR;
            apr_size_t len = strlen(s);
            SVN_ERR(svn_stream_write(os, s, &len));
          }
      }
    }

  return SVN_NO_ERROR;
}

/*-----------------------------------------------------------------*/

/*** Callbacks for 'svn diff', invoked by the repos-diff editor. ***/


struct diff_cmd_baton {

  /* If non-null, the external diff command to invoke. */
  const char *diff_cmd;

  /* This is allocated in this struct's pool or a higher-up pool. */
  union {
    /* If 'diff_cmd' is null, then this is the parsed options to
       pass to the internal libsvn_diff implementation. */
    svn_diff_file_options_t *for_internal;
    /* Else if 'diff_cmd' is non-null, then... */
    struct {
      /* ...this is an argument array for the external command, and */
      const char **argv;
      /* ...this is the length of argv. */
      int argc;
    } for_external;
  } options;

  apr_pool_t *pool;
  apr_file_t *outfile;
  apr_file_t *errfile;

  const char *header_encoding;

  /* The original targets passed to the diff command.  We may need
     these to construct distinctive diff labels when comparing the
     same relative path in the same revision, under different anchors
     (for example, when comparing a trunk against a branch). */
  const char *orig_path_1;
  const char *orig_path_2;

  /* These are the numeric representations of the revisions passed to
     svn_client_diff5, either may be SVN_INVALID_REVNUM.  We need these
     because some of the svn_wc_diff_callbacks4_t don't get revision
     arguments.

     ### Perhaps we should change the callback signatures and eliminate
     ### these?
  */
  svn_revnum_t revnum1;
  svn_revnum_t revnum2;

  /* Set this if you want diff output even for binary files. */
  svn_boolean_t force_binary;

  /* Set this flag if you want diff_file_changed to output diffs
     unconditionally, even if the diffs are empty. */
  svn_boolean_t force_empty;

  /* The directory that diff target paths should be considered as
     relative to for output generation (see issue #2723). */
  const char *relative_to_dir;

  /* Whether we're producing a git-style diff. */
  svn_boolean_t use_git_diff_format;

  svn_wc_context_t *wc_ctx;

  /* The RA session used during diffs involving the repository. */
  svn_ra_session_t *ra_session;

  /* During a repos-wc diff, this is the absolute path to the root
   * directory of the working copy involved in the diff. */
  const char *wc_root_abspath;

  /* The anchor to prefix before wc paths */
  const char *anchor;

  /* A hashtable using the visited paths as keys.
   * ### This is needed for us to know if we need to print a diff header for
   * ### a path that has property changes. */
  apr_hash_t *visited_paths;
};


/* A helper function that marks a path as visited. It copies PATH
 * into the correct pool before referencing it from the hash table. */
static void
mark_path_as_visited(struct diff_cmd_baton *diff_cmd_baton, const char *path)
{
  const char *p;

  p = apr_pstrdup(apr_hash_pool_get(diff_cmd_baton->visited_paths), path);
  apr_hash_set(diff_cmd_baton->visited_paths, p, APR_HASH_KEY_STRING, p);
}

/* An helper for diff_dir_props_changed, diff_file_changed and diff_file_added
 */
static svn_error_t *
diff_props_changed(svn_wc_notify_state_t *state,
                   svn_boolean_t *tree_conflicted,
                   const char *path,
                   svn_boolean_t dir_was_added,
                   const apr_array_header_t *propchanges,
                   apr_hash_t *original_props,
                   void *diff_baton,
                   apr_pool_t *scratch_pool)
{
  struct diff_cmd_baton *diff_cmd_baton = diff_baton;
  apr_array_header_t *props;
  svn_boolean_t show_diff_header;

  SVN_ERR(svn_categorize_props(propchanges, NULL, NULL, &props,
                               scratch_pool));

  if (apr_hash_get(diff_cmd_baton->visited_paths, path, APR_HASH_KEY_STRING))
    show_diff_header = FALSE;
  else
    show_diff_header = TRUE;

  if (props->nelts > 0)
    {
      /* We're using the revnums from the diff_cmd_baton since there's
       * no revision argument to the svn_wc_diff_callback_t
       * dir_props_changed(). */
      SVN_ERR(display_prop_diffs(props, original_props, path,
                                 diff_cmd_baton->orig_path_1,
                                 diff_cmd_baton->orig_path_2,
                                 diff_cmd_baton->revnum1,
                                 diff_cmd_baton->revnum2,
                                 diff_cmd_baton->header_encoding,
                                 diff_cmd_baton->outfile,
                                 diff_cmd_baton->relative_to_dir,
                                 show_diff_header,
                                 diff_cmd_baton->use_git_diff_format,
                                 diff_cmd_baton->ra_session,
                                 diff_cmd_baton->wc_ctx,
                                 diff_cmd_baton->wc_root_abspath,
                                 scratch_pool));

      /* We've printed the diff header so now we can mark the path as
       * visited. */
      if (show_diff_header)
        mark_path_as_visited(diff_cmd_baton, path);
    }

  if (state)
    *state = svn_wc_notify_state_unknown;
  if (tree_conflicted)
    *tree_conflicted = FALSE;

  return SVN_NO_ERROR;
}

/* An svn_wc_diff_callbacks4_t function. */
static svn_error_t *
diff_dir_props_changed(svn_wc_notify_state_t *state,
                       svn_boolean_t *tree_conflicted,
                       const char *path,
                       svn_boolean_t dir_was_added,
                       const apr_array_header_t *propchanges,
                       apr_hash_t *original_props,
                       void *diff_baton,
                       apr_pool_t *scratch_pool)
{
  struct diff_cmd_baton *diff_cmd_baton = diff_baton;

  if (diff_cmd_baton->anchor)
    path = svn_dirent_join(diff_cmd_baton->anchor, path, scratch_pool);

  return svn_error_trace(diff_props_changed(state,
                                            tree_conflicted, path,
                                            dir_was_added,
                                            propchanges,
                                            original_props,
                                            diff_baton,
                                            scratch_pool));
}


/* Show differences between TMPFILE1 and TMPFILE2. PATH, REV1, and REV2 are
   used in the headers to indicate the file and revisions.  If either
   MIMETYPE1 or MIMETYPE2 indicate binary content, don't show a diff,
   but instead print a warning message. */
static svn_error_t *
diff_content_changed(const char *path,
                     const char *tmpfile1,
                     const char *tmpfile2,
                     svn_revnum_t rev1,
                     svn_revnum_t rev2,
                     const char *mimetype1,
                     const char *mimetype2,
                     svn_diff_operation_kind_t operation,
                     const char *copyfrom_path,
                     void *diff_baton)
{
  struct diff_cmd_baton *diff_cmd_baton = diff_baton;
  int exitcode;
  apr_pool_t *subpool = svn_pool_create(diff_cmd_baton->pool);
  svn_stream_t *os;
  const char *rel_to_dir = diff_cmd_baton->relative_to_dir;
  apr_file_t *errfile = diff_cmd_baton->errfile;
  const char *label1, *label2;
  svn_boolean_t mt1_binary = FALSE, mt2_binary = FALSE;
  const char *path1, *path2;

  /* Get a stream from our output file. */
  os = svn_stream_from_aprfile2(diff_cmd_baton->outfile, TRUE, subpool);

  /* Generate the diff headers. */

  path1 = apr_pstrdup(subpool, diff_cmd_baton->orig_path_1);
  path2 = apr_pstrdup(subpool, diff_cmd_baton->orig_path_2);

  SVN_ERR(adjust_paths_for_diff_labels(&path, &path1, &path2,
                                       rel_to_dir, subpool));

  label1 = diff_label(path1, rev1, subpool);
  label2 = diff_label(path2, rev2, subpool);

  /* Possible easy-out: if either mime-type is binary and force was not
     specified, don't attempt to generate a viewable diff at all.
     Print a warning and exit. */
  if (mimetype1)
    mt1_binary = svn_mime_type_is_binary(mimetype1);
  if (mimetype2)
    mt2_binary = svn_mime_type_is_binary(mimetype2);

  if (! diff_cmd_baton->force_binary && (mt1_binary || mt2_binary))
    {
      /* Print out the diff header. */
      SVN_ERR(svn_stream_printf_from_utf8
              (os, diff_cmd_baton->header_encoding, subpool,
               "Index: %s" APR_EOL_STR "%s" APR_EOL_STR, path, equal_string));

      /* ### Print git diff headers. */

      SVN_ERR(svn_stream_printf_from_utf8
              (os, diff_cmd_baton->header_encoding, subpool,
               _("Cannot display: file marked as a binary type.%s"),
               APR_EOL_STR));

      if (mt1_binary && !mt2_binary)
        SVN_ERR(svn_stream_printf_from_utf8
                (os, diff_cmd_baton->header_encoding, subpool,
                 "svn:mime-type = %s" APR_EOL_STR, mimetype1));
      else if (mt2_binary && !mt1_binary)
        SVN_ERR(svn_stream_printf_from_utf8
                (os, diff_cmd_baton->header_encoding, subpool,
                 "svn:mime-type = %s" APR_EOL_STR, mimetype2));
      else if (mt1_binary && mt2_binary)
        {
          if (strcmp(mimetype1, mimetype2) == 0)
            SVN_ERR(svn_stream_printf_from_utf8
                    (os, diff_cmd_baton->header_encoding, subpool,
                     "svn:mime-type = %s" APR_EOL_STR,
                     mimetype1));
          else
            SVN_ERR(svn_stream_printf_from_utf8
                    (os, diff_cmd_baton->header_encoding, subpool,
                     "svn:mime-type = (%s, %s)" APR_EOL_STR,
                     mimetype1, mimetype2));
        }

      /* Exit early. */
      svn_pool_destroy(subpool);
      return SVN_NO_ERROR;
    }


  if (diff_cmd_baton->diff_cmd)
    {
      /* Print out the diff header. */
      SVN_ERR(svn_stream_printf_from_utf8
              (os, diff_cmd_baton->header_encoding, subpool,
               "Index: %s" APR_EOL_STR "%s" APR_EOL_STR, path, equal_string));
      /* Close the stream (flush) */
      SVN_ERR(svn_stream_close(os));

      /* ### Do we want to add git diff headers here too? I'd say no. The
       * ### 'Index' and '===' line is something subversion has added. The rest
       * ### is up to the external diff application. We may be dealing with
       * ### a non-git compatible diff application.*/

      SVN_ERR(svn_io_run_diff2(".",
                               diff_cmd_baton->options.for_external.argv,
                               diff_cmd_baton->options.for_external.argc,
                               label1, label2,
                               tmpfile1, tmpfile2,
                               &exitcode, diff_cmd_baton->outfile, errfile,
                               diff_cmd_baton->diff_cmd, subpool));

      /* We have a printed a diff for this path, mark it as visited. */
      mark_path_as_visited(diff_cmd_baton, path);
    }
  else   /* use libsvn_diff to generate the diff  */
    {
      svn_diff_t *diff;

      SVN_ERR(svn_diff_file_diff_2(&diff, tmpfile1, tmpfile2,
                                   diff_cmd_baton->options.for_internal,
                                   subpool));

      if (svn_diff_contains_diffs(diff) || diff_cmd_baton->force_empty ||
          diff_cmd_baton->use_git_diff_format)
        {
          /* Print out the diff header. */
          SVN_ERR(svn_stream_printf_from_utf8
                  (os, diff_cmd_baton->header_encoding, subpool,
                   "Index: %s" APR_EOL_STR "%s" APR_EOL_STR,
                   path, equal_string));

          if (diff_cmd_baton->use_git_diff_format)
            {
              const char *tmp_path1, *tmp_path2;
              SVN_ERR(adjust_relative_to_repos_root(
                         &tmp_path1, path, diff_cmd_baton->orig_path_1,
                         diff_cmd_baton->ra_session, diff_cmd_baton->wc_ctx,
                         diff_cmd_baton->wc_root_abspath, subpool));
              SVN_ERR(adjust_relative_to_repos_root(
                         &tmp_path2, path, diff_cmd_baton->orig_path_2,
                         diff_cmd_baton->ra_session, diff_cmd_baton->wc_ctx,
                         diff_cmd_baton->wc_root_abspath, subpool));
              SVN_ERR(print_git_diff_header(os, &label1, &label2, operation,
                                            tmp_path1, tmp_path2, rev1, rev2,
                                            copyfrom_path,
                                            diff_cmd_baton->header_encoding,
                                            diff_cmd_baton->ra_session,
                                            diff_cmd_baton->wc_ctx,
                                            diff_cmd_baton->wc_root_abspath,
                                            subpool));
            }

          /* Output the actual diff */
          if (svn_diff_contains_diffs(diff) || diff_cmd_baton->force_empty)
            SVN_ERR(svn_diff_file_output_unified3
                    (os, diff, tmpfile1, tmpfile2, label1, label2,
                     diff_cmd_baton->header_encoding, rel_to_dir,
                     diff_cmd_baton->options.for_internal->show_c_function,
                     subpool));

          /* We have a printed a diff for this path, mark it as visited. */
          mark_path_as_visited(diff_cmd_baton, path);
        }

      /* Close the stream (flush) */
      SVN_ERR(svn_stream_close(os));
    }

  /* ### todo: someday we'll need to worry about whether we're going
     to need to write a diff plug-in mechanism that makes use of the
     two paths, instead of just blindly running SVN_CLIENT_DIFF.  */

  /* Destroy the subpool. */
  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}

static svn_error_t *
diff_file_opened(svn_boolean_t *tree_conflicted,
                 svn_boolean_t *skip,
                 const char *path,
                 svn_revnum_t rev,
                 void *diff_baton,
                 apr_pool_t *scratch_pool)
{
  return SVN_NO_ERROR;
}

/* An svn_wc_diff_callbacks4_t function. */
static svn_error_t *
diff_file_changed(svn_wc_notify_state_t *content_state,
                  svn_wc_notify_state_t *prop_state,
                  svn_boolean_t *tree_conflicted,
                  const char *path,
                  const char *tmpfile1,
                  const char *tmpfile2,
                  svn_revnum_t rev1,
                  svn_revnum_t rev2,
                  const char *mimetype1,
                  const char *mimetype2,
                  const apr_array_header_t *prop_changes,
                  apr_hash_t *original_props,
                  void *diff_baton,
                  apr_pool_t *scratch_pool)
{
  struct diff_cmd_baton *diff_cmd_baton = diff_baton;
  if (diff_cmd_baton->anchor)
    path = svn_dirent_join(diff_cmd_baton->anchor, path, scratch_pool);
  if (tmpfile1)
    SVN_ERR(diff_content_changed(path,
                                 tmpfile1, tmpfile2, rev1, rev2,
                                 mimetype1, mimetype2,
                                 svn_diff_op_modified, NULL, diff_baton));
  if (prop_changes->nelts > 0)
    SVN_ERR(diff_props_changed(prop_state, tree_conflicted,
                               path, FALSE, prop_changes,
                               original_props, diff_baton, scratch_pool));
  if (content_state)
    *content_state = svn_wc_notify_state_unknown;
  if (prop_state)
    *prop_state = svn_wc_notify_state_unknown;
  if (tree_conflicted)
    *tree_conflicted = FALSE;
  return SVN_NO_ERROR;
}

/* Because the repos-diff editor passes at least one empty file to
   each of these next two functions, they can be dumb wrappers around
   the main workhorse routine. */

/* An svn_wc_diff_callbacks4_t function. */
static svn_error_t *
diff_file_added(svn_wc_notify_state_t *content_state,
                svn_wc_notify_state_t *prop_state,
                svn_boolean_t *tree_conflicted,
                const char *path,
                const char *tmpfile1,
                const char *tmpfile2,
                svn_revnum_t rev1,
                svn_revnum_t rev2,
                const char *mimetype1,
                const char *mimetype2,
                const char *copyfrom_path,
                svn_revnum_t copyfrom_revision,
                const apr_array_header_t *prop_changes,
                apr_hash_t *original_props,
                void *diff_baton,
                apr_pool_t *scratch_pool)
{
  struct diff_cmd_baton *diff_cmd_baton = diff_baton;

  if (diff_cmd_baton->anchor)
    path = svn_dirent_join(diff_cmd_baton->anchor, path, scratch_pool);

  /* We want diff_file_changed to unconditionally show diffs, even if
     the diff is empty (as would be the case if an empty file were
     added.)  It's important, because 'patch' would still see an empty
     diff and create an empty file.  It's also important to let the
     user see that *something* happened. */
  diff_cmd_baton->force_empty = TRUE;

  if (tmpfile1 && copyfrom_path)
    SVN_ERR(diff_content_changed(path,
                                 tmpfile1, tmpfile2, rev1, rev2,
                                 mimetype1, mimetype2,
                                 svn_diff_op_copied, copyfrom_path,
                                 diff_baton));
  else if (tmpfile1)
    SVN_ERR(diff_content_changed(path,
                                 tmpfile1, tmpfile2, rev1, rev2,
                                 mimetype1, mimetype2,
                                 svn_diff_op_added, NULL, diff_baton));
  if (prop_changes->nelts > 0)
    SVN_ERR(diff_props_changed(prop_state, tree_conflicted,
                               path, FALSE, prop_changes,
                               original_props, diff_baton, scratch_pool));
  if (content_state)
    *content_state = svn_wc_notify_state_unknown;
  if (prop_state)
    *prop_state = svn_wc_notify_state_unknown;
  if (tree_conflicted)
    *tree_conflicted = FALSE;

  diff_cmd_baton->force_empty = FALSE;

  return SVN_NO_ERROR;
}

/* An svn_wc_diff_callbacks4_t function. */
static svn_error_t *
diff_file_deleted_with_diff(svn_wc_notify_state_t *state,
                            svn_boolean_t *tree_conflicted,
                            const char *path,
                            const char *tmpfile1,
                            const char *tmpfile2,
                            const char *mimetype1,
                            const char *mimetype2,
                            apr_hash_t *original_props,
                            void *diff_baton,
                            apr_pool_t *scratch_pool)
{
  struct diff_cmd_baton *diff_cmd_baton = diff_baton;

  if (diff_cmd_baton->anchor)
    path = svn_dirent_join(diff_cmd_baton->anchor, path, scratch_pool);

  if (tmpfile1)
    SVN_ERR(diff_content_changed(path,
                                 tmpfile1, tmpfile2, diff_cmd_baton->revnum1,
                                 diff_cmd_baton->revnum2,
                                 mimetype1, mimetype2,
                                 svn_diff_op_deleted, NULL, diff_baton));

  /* We don't list all the deleted properties. */

  if (state)
    *state = svn_wc_notify_state_unknown;
  if (tree_conflicted)
    *tree_conflicted = FALSE;

  return SVN_NO_ERROR;
}

/* An svn_wc_diff_callbacks4_t function. */
static svn_error_t *
diff_file_deleted_no_diff(svn_wc_notify_state_t *state,
                          svn_boolean_t *tree_conflicted,
                          const char *path,
                          const char *tmpfile1,
                          const char *tmpfile2,
                          const char *mimetype1,
                          const char *mimetype2,
                          apr_hash_t *original_props,
                          void *diff_baton,
                          apr_pool_t *scratch_pool)
{
  struct diff_cmd_baton *diff_cmd_baton = diff_baton;

  if (diff_cmd_baton->anchor)
    path = svn_dirent_join(diff_cmd_baton->anchor, path, scratch_pool);

  if (state)
    *state = svn_wc_notify_state_unknown;
  if (tree_conflicted)
    *tree_conflicted = FALSE;

  return file_printf_from_utf8
          (diff_cmd_baton->outfile,
           diff_cmd_baton->header_encoding,
           "Index: %s (deleted)" APR_EOL_STR "%s" APR_EOL_STR,
           path, equal_string);
}

/* An svn_wc_diff_callbacks4_t function. */
static svn_error_t *
diff_dir_added(svn_wc_notify_state_t *state,
               svn_boolean_t *tree_conflicted,
               svn_boolean_t *skip,
               svn_boolean_t *skip_children,
               const char *path,
               svn_revnum_t rev,
               const char *copyfrom_path,
               svn_revnum_t copyfrom_revision,
               void *diff_baton,
               apr_pool_t *scratch_pool)
{
  /*struct diff_cmd_baton *diff_cmd_baton = diff_baton;
  if (diff_cmd_baton->anchor)
    path = svn_dirent_join(diff_cmd_baton->anchor, path, scratch_pool);*/

  /* Do nothing. */

  return SVN_NO_ERROR;
}

/* An svn_wc_diff_callbacks4_t function. */
static svn_error_t *
diff_dir_deleted(svn_wc_notify_state_t *state,
                 svn_boolean_t *tree_conflicted,
                 const char *path,
                 void *diff_baton,
                 apr_pool_t *scratch_pool)
{
  /*struct diff_cmd_baton *diff_cmd_baton = diff_baton;
  if (diff_cmd_baton->anchor)
    path = svn_dirent_join(diff_cmd_baton->anchor, path, scratch_pool);*/

  /* Do nothing. */

  return SVN_NO_ERROR;
}

/* An svn_wc_diff_callbacks4_t function. */
static svn_error_t *
diff_dir_opened(svn_boolean_t *tree_conflicted,
                svn_boolean_t *skip,
                svn_boolean_t *skip_children,
                const char *path,
                svn_revnum_t rev,
                void *diff_baton,
                apr_pool_t *scratch_pool)
{
  /*struct diff_cmd_baton *diff_cmd_baton = diff_baton;
  if (diff_cmd_baton->anchor)
    path = svn_dirent_join(diff_cmd_baton->anchor, path, scratch_pool);*/

  /* Do nothing. */

  return SVN_NO_ERROR;
}

/* An svn_wc_diff_callbacks4_t function. */
static svn_error_t *
diff_dir_closed(svn_wc_notify_state_t *contentstate,
                svn_wc_notify_state_t *propstate,
                svn_boolean_t *tree_conflicted,
                const char *path,
                svn_boolean_t dir_was_added,
                void *diff_baton,
                apr_pool_t *scratch_pool)
{
  /*struct diff_cmd_baton *diff_cmd_baton = diff_baton;
  if (diff_cmd_baton->anchor)
    path = svn_dirent_join(diff_cmd_baton->anchor, path, scratch_pool);*/

  /* Do nothing. */

  return SVN_NO_ERROR;
}


/*-----------------------------------------------------------------*/

/** The logic behind 'svn diff' and 'svn merge'.  */


/* Hi!  This is a comment left behind by Karl, and Ben is too afraid
   to erase it at this time, because he's not fully confident that all
   this knowledge has been grokked yet.

   There are five cases:
      1. path is not an URL and start_revision != end_revision
      2. path is not an URL and start_revision == end_revision
      3. path is an URL and start_revision != end_revision
      4. path is an URL and start_revision == end_revision
      5. path is not an URL and no revisions given

   With only one distinct revision the working copy provides the
   other.  When path is an URL there is no working copy. Thus

     1: compare repository versions for URL coresponding to working copy
     2: compare working copy against repository version
     3: compare repository versions for URL
     4: nothing to do.
     5: compare working copy against text-base

   Case 4 is not as stupid as it looks, for example it may occur if
   the user specifies two dates that resolve to the same revision.  */




/* Helper function: given a working-copy ABSPATH_OR_URL, return its
   associated url in *URL, allocated in RESULT_POOL.  If ABSPATH_OR_URL is
   *already* a URL, that's fine, return ABSPATH_OR_URL allocated in
   RESULT_POOL.

   Use SCRATCH_POOL for temporary allocations. */
static svn_error_t *
convert_to_url(const char **url,
               svn_wc_context_t *wc_ctx,
               const char *abspath_or_url,
               apr_pool_t *result_pool,
               apr_pool_t *scratch_pool)
{
  if (svn_path_is_url(abspath_or_url))
    {
      *url = apr_pstrdup(result_pool, abspath_or_url);
      return SVN_NO_ERROR;
    }

  SVN_ERR(svn_wc__node_get_url(url, wc_ctx, abspath_or_url,
                               result_pool, scratch_pool));
  if (! *url)
    return svn_error_createf(SVN_ERR_ENTRY_MISSING_URL, NULL,
                             _("Path '%s' has no URL"),
                             svn_dirent_local_style(abspath_or_url,
                                                    scratch_pool));
  return SVN_NO_ERROR;
}

/** Check if paths PATH1 and PATH2 are urls and if the revisions REVISION1
 *  and REVISION2 are local. If PEG_REVISION is not unspecified, ensure that
 *  at least one of the two revisions is non-local.
 *  If PATH1 can only be found in the repository, set *IS_REPOS1 to TRUE.
 *  If PATH2 can only be found in the repository, set *IS_REPOS2 to TRUE. */
static svn_error_t *
check_paths(svn_boolean_t *is_repos1,
            svn_boolean_t *is_repos2,
            const char *path1,
            const char *path2,
            const svn_opt_revision_t *revision1,
            const svn_opt_revision_t *revision2,
            const svn_opt_revision_t *peg_revision)
{
  svn_boolean_t is_local_rev1, is_local_rev2;

  /* Verify our revision arguments in light of the paths. */
  if ((revision1->kind == svn_opt_revision_unspecified)
      || (revision2->kind == svn_opt_revision_unspecified))
    return svn_error_create(SVN_ERR_CLIENT_BAD_REVISION, NULL,
                            _("Not all required revisions are specified"));

  /* Revisions can be said to be local or remote.  BASE and WORKING,
     for example, are local.  */
  is_local_rev1 =
    ((revision1->kind == svn_opt_revision_base)
     || (revision1->kind == svn_opt_revision_working));
  is_local_rev2 =
    ((revision2->kind == svn_opt_revision_base)
     || (revision2->kind == svn_opt_revision_working));

  if (peg_revision->kind != svn_opt_revision_unspecified)
    {
      if (is_local_rev1 && is_local_rev2)
        return svn_error_create(SVN_ERR_CLIENT_BAD_REVISION, NULL,
                                _("At least one revision must be non-local "
                                  "for a pegged diff"));

      *is_repos1 = ! is_local_rev1 || svn_path_is_url(path1);
      *is_repos2 = ! is_local_rev2 || svn_path_is_url(path2);
    }
  else
    {
      /* Working copy paths with non-local revisions get turned into
         URLs.  We don't do that here, though.  We simply record that it
         needs to be done, which is information that helps us choose our
         diff helper function.  */
      *is_repos1 = ! is_local_rev1 || svn_path_is_url(path1);
      *is_repos2 = ! is_local_rev2 || svn_path_is_url(path2);
    }

  return SVN_NO_ERROR;
}

/* Raise an error if the diff target URL does not exist at REVISION.
 * If REVISION does not equal OTHER_REVISION, mention both revisions in
 * the error message. Use RA_SESSION to contact the repository.
 * Use POOL for temporary allocations. */
static svn_error_t *
check_diff_target_exists(const char *url,
                         svn_revnum_t revision,
                         svn_revnum_t other_revision,
                         svn_ra_session_t *ra_session,
                         apr_pool_t *pool)
{
  svn_node_kind_t kind;
  const char *session_url;

  SVN_ERR(svn_ra_get_session_url(ra_session, &session_url, pool));

  if (strcmp(url, session_url) != 0)
    SVN_ERR(svn_ra_reparent(ra_session, url, pool));

  SVN_ERR(svn_ra_check_path(ra_session, "", revision, &kind, pool));
  if (kind == svn_node_none)
    {
      if (revision == other_revision)
        return svn_error_createf(SVN_ERR_FS_NOT_FOUND, NULL,
                                 _("Diff target '%s' was not found in the "
                                   "repository at revision '%ld'"),
                                 url, revision);
      else
        return svn_error_createf(SVN_ERR_FS_NOT_FOUND, NULL,
                                 _("Diff target '%s' was not found in the "
                                   "repository at revision '%ld' or '%ld'"),
                                 url, revision, other_revision);
     }

  if (strcmp(url, session_url) != 0)
    SVN_ERR(svn_ra_reparent(ra_session, session_url, pool));

  return SVN_NO_ERROR;
}

/** Prepare a repos repos diff between PATH1 and PATH2@PEG_REVISION,
 * in the revision range REVISION1:REVISION2.
 * Return URLs and peg revisions in *URL1, *REV1 and in *URL2, *REV2.
 * Return suitable anchors in *ANCHOR1 and *ANCHOR2, and targets in
 * *TARGET1 and *TARGET2, based on *URL1 and *URL2.
 * Set *BASE_PATH corresponding to the URL opened in the new *RA_SESSION
 * which is pointing at *ANCHOR1.
 * Use client context CTX. Do all allocations in POOL. */
static svn_error_t *
diff_prepare_repos_repos(const char **url1,
                         const char **url2,
                         const char **base_path,
                         svn_revnum_t *rev1,
                         svn_revnum_t *rev2,
                         const char **anchor1,
                         const char **anchor2,
                         const char **target1,
                         const char **target2,
                         svn_ra_session_t **ra_session,
                         svn_client_ctx_t *ctx,
                         const char *path1,
                         const char *path2,
                         const svn_opt_revision_t *revision1,
                         const svn_opt_revision_t *revision2,
                         const svn_opt_revision_t *peg_revision,
                         apr_pool_t *pool)
{
  svn_node_kind_t kind1, kind2;
  const char *path2_abspath;
  const char *path1_abspath;

  if (!svn_path_is_url(path2))
    SVN_ERR(svn_dirent_get_absolute(&path2_abspath, path2,
                                    pool));
  else
    path2_abspath = path2;

  if (!svn_path_is_url(path1))
    SVN_ERR(svn_dirent_get_absolute(&path1_abspath, path1,
                                    pool));
  else
    path1_abspath = path1;

  /* Figure out URL1 and URL2. */
  SVN_ERR(convert_to_url(url1, ctx->wc_ctx, path1_abspath,
                         pool, pool));
  SVN_ERR(convert_to_url(url2, ctx->wc_ctx, path2_abspath,
                         pool, pool));

  /* We need exactly one BASE_PATH, so we'll let the BASE_PATH
     calculated for PATH2 override the one for PATH1 (since the diff
     will be "applied" to URL2 anyway). */
  *base_path = NULL;
  if (strcmp(*url1, path1) != 0)
    *base_path = path1;
  if (strcmp(*url2, path2) != 0)
    *base_path = path2;

  SVN_ERR(svn_client__open_ra_session_internal(ra_session, NULL, *url2,
                                               NULL, NULL, FALSE,
                                               TRUE, ctx, pool));

  /* If we are performing a pegged diff, we need to find out what our
     actual URLs will be. */
  if (peg_revision->kind != svn_opt_revision_unspecified)
    {
      svn_opt_revision_t *start_ignore, *end_ignore;
      svn_error_t *err;

      err = svn_client__repos_locations(url1, &start_ignore,
                                        url2, &end_ignore,
                                        *ra_session,
                                        path2,
                                        peg_revision,
                                        revision1,
                                        revision2,
                                        ctx, pool);
      if (err)
        {
          if (err->apr_err == SVN_ERR_CLIENT_UNRELATED_RESOURCES)
            {
              /* Don't give up just yet. A missing path might translate
               * into an addition in the diff. Below, we verify that each
               * URL exists on at least one side of the diff. */
              svn_error_clear(err);
            }
          else
            return svn_error_trace(err);
        }
      else
        {
          /* Reparent the session, since *URL2 might have changed as a result
             the above call. */
          SVN_ERR(svn_ra_reparent(*ra_session, *url2, pool));
        }
    }

  /* Resolve revision and get path kind for the second target. */
  SVN_ERR(svn_client__get_revision_number(rev2, NULL, ctx->wc_ctx,
           (path2 == *url2) ? NULL : path2_abspath,
           *ra_session, revision2, pool));
  SVN_ERR(svn_ra_check_path(*ra_session, "", *rev2, &kind2, pool));

  /* Do the same for the first target. */
  SVN_ERR(svn_ra_reparent(*ra_session, *url1, pool));
  SVN_ERR(svn_client__get_revision_number(rev1, NULL, ctx->wc_ctx,
           (strcmp(path1, *url1) == 0) ? NULL : path1_abspath,
           *ra_session, revision1, pool));
  SVN_ERR(svn_ra_check_path(*ra_session, "", *rev1, &kind1, pool));

  /* Either both URLs must exist at their respective revisions,
   * or one of them may be missing from one side of the diff. */
  if (kind1 == svn_node_none && kind2 == svn_node_none)
    {
      if (strcmp(*url1, *url2) == 0)
        return svn_error_createf(SVN_ERR_FS_NOT_FOUND, NULL,
                                 _("Diff target '%s' was not found in the "
                                   "repository at revisions '%ld' and '%ld'"),
                                 *url1, *rev1, *rev2);
      else
        return svn_error_createf(SVN_ERR_FS_NOT_FOUND, NULL,
                                 _("Diff targets '%s and '%s' were not found "
                                   "in the repository at revisions '%ld' and "
                                   "'%ld'"),
                                 *url1, *url2, *rev1, *rev2);
    }
  else if (kind1 == svn_node_none)
    SVN_ERR(check_diff_target_exists(*url1, *rev2, *rev1, *ra_session, pool));
  else if (kind2 == svn_node_none)
    SVN_ERR(check_diff_target_exists(*url2, *rev1, *rev2, *ra_session, pool));

  /* Choose useful anchors and targets for our two URLs. */
  *anchor1 = *url1;
  *anchor2 = *url2;
  *target1 = "";
  *target2 = "";
  if ((kind1 == svn_node_none) || (kind2 == svn_node_none))
    {
      svn_node_kind_t kind;
      const char *repos_root;
      const char *new_anchor;
      svn_revnum_t rev;

      /* The diff target does not exist on one side of the diff.
       * This can happen if the target was added or deleted within the
       * revision range being diffed.
       * However, we don't know how deep within a added/deleted subtree the
       * diff target is. Find a common parent that exists on both sides of
       * the diff and use it as anchor for the diff operation.
       *
       * ### This can fail due to authz restrictions (like in issue #3242).
       * ### But it is the only option we have right now to try to get
       * ### a usable diff in this situation. */

      SVN_ERR(svn_ra_get_repos_root2(*ra_session, &repos_root, pool));

      /* Since we already know that one of the URLs does exist,
       * look for an existing parent of the URL which doesn't exist. */
      new_anchor = (kind1 == svn_node_none ? *anchor1 : *anchor2);
      rev = (kind1 == svn_node_none ? *rev1 : *rev2);
      do
        {
          if (strcmp(new_anchor, repos_root) != 0)
            {
              new_anchor = svn_path_uri_decode(svn_uri_dirname(new_anchor,
                                                               pool),
                                               pool);
              if (*base_path)
                *base_path = svn_dirent_dirname(*base_path, pool);
            }

          SVN_ERR(svn_ra_reparent(*ra_session, new_anchor, pool));
          SVN_ERR(svn_ra_check_path(*ra_session, "", rev, &kind, pool));

        }
      while (kind != svn_node_dir);
      *anchor1 = *anchor2 = new_anchor;
      /* Diff targets must be relative to the new anchor. */
      *target1 = svn_uri_skip_ancestor(new_anchor, *url1, pool);
      *target2 = svn_uri_skip_ancestor(new_anchor, *url2, pool);
      SVN_ERR_ASSERT(*target1 && *target2);
      if (kind1 == svn_node_none)
        kind1 = svn_node_dir;
      else
        kind2 = svn_node_dir;
    }
  else if ((kind1 == svn_node_file) || (kind2 == svn_node_file))
    {
      svn_uri_split(anchor1, target1, *url1, pool);
      svn_uri_split(anchor2, target2, *url2, pool);
      if (*base_path)
        *base_path = svn_dirent_dirname(*base_path, pool);
      SVN_ERR(svn_ra_reparent(*ra_session, *anchor1, pool));
    }

  return SVN_NO_ERROR;
}

/* A Theoretical Note From Ben, regarding do_diff().

   This function is really svn_client_diff5().  If you read the public
   API description for svn_client_diff5(), it sounds quite Grand.  It
   sounds really generalized and abstract and beautiful: that it will
   diff any two paths, be they working-copy paths or URLs, at any two
   revisions.

   Now, the *reality* is that we have exactly three 'tools' for doing
   diffing, and thus this routine is built around the use of the three
   tools.  Here they are, for clarity:

     - svn_wc_diff:  assumes both paths are the same wcpath.
                     compares wcpath@BASE vs. wcpath@WORKING

     - svn_wc_get_diff_editor:  compares some URL@REV vs. wcpath@WORKING

     - svn_client__get_diff_editor:  compares some URL1@REV1 vs. URL2@REV2

   So the truth of the matter is, if the caller's arguments can't be
   pigeonholed into one of these three use-cases, we currently bail
   with a friendly apology.

   Perhaps someday a brave soul will truly make svn_client_diff5
   perfectly general.  For now, we live with the 90% case.  Certainly,
   the commandline client only calls this function in legal ways.
   When there are other users of svn_client.h, maybe this will become
   a more pressing issue.
 */

/* Return a "you can't do that" error, optionally wrapping another
   error CHILD_ERR. */
static svn_error_t *
unsupported_diff_error(svn_error_t *child_err)
{
  return svn_error_create(SVN_ERR_INCORRECT_PARAMS, child_err,
                          _("Sorry, svn_client_diff5 was called in a way "
                            "that is not yet supported"));
}


/* Perform a diff between two working-copy paths.

   PATH1 and PATH2 are both working copy paths.  REVISION1 and
   REVISION2 are their respective revisions.

   All other options are the same as those passed to svn_client_diff5(). */
static svn_error_t *
diff_wc_wc(const char *path1,
           const svn_opt_revision_t *revision1,
           const char *path2,
           const svn_opt_revision_t *revision2,
           svn_depth_t depth,
           svn_boolean_t ignore_ancestry,
           svn_boolean_t show_copies_as_adds,
           svn_boolean_t use_git_diff_format,
           const apr_array_header_t *changelists,
           const svn_wc_diff_callbacks4_t *callbacks,
           struct diff_cmd_baton *callback_baton,
           svn_client_ctx_t *ctx,
           apr_pool_t *pool)
{
  const char *abspath1;
  svn_error_t *err;
  svn_node_kind_t kind;

  SVN_ERR_ASSERT(! svn_path_is_url(path1));
  SVN_ERR_ASSERT(! svn_path_is_url(path2));

  SVN_ERR(svn_dirent_get_absolute(&abspath1, path1, pool));

  /* Currently we support only the case where path1 and path2 are the
     same path. */
  if ((strcmp(path1, path2) != 0)
      || (! ((revision1->kind == svn_opt_revision_base)
             && (revision2->kind == svn_opt_revision_working))))
    return unsupported_diff_error
      (svn_error_create
       (SVN_ERR_INCORRECT_PARAMS, NULL,
        _("Only diffs between a path's text-base "
          "and its working files are supported at this time")));

  /* Resolve named revisions to real numbers. */
  err = svn_client__get_revision_number(&callback_baton->revnum1, NULL,
                                        ctx->wc_ctx, abspath1, NULL,
                                        revision1, pool);

  /* In case of an added node, we have no base rev, and we show a revision
   * number of 0. Note that this code is currently always asking for
   * svn_opt_revision_base.
   * ### TODO: get rid of this 0 for added nodes. */
  if (err && (err->apr_err == SVN_ERR_CLIENT_BAD_REVISION))
    {
      svn_error_clear(err);
      callback_baton->revnum1 = 0;
    }
  else
    SVN_ERR(err);

  callback_baton->revnum2 = SVN_INVALID_REVNUM;  /* WC */

  SVN_ERR(svn_wc_read_kind(&kind, ctx->wc_ctx, abspath1, FALSE, pool));

  if (kind != svn_node_dir)
    callback_baton->anchor = svn_dirent_dirname(path1, pool);
  else
    callback_baton->anchor = path1;

  SVN_ERR(svn_wc_diff6(ctx->wc_ctx,
                       abspath1,
                       callbacks, callback_baton,
                       depth,
                       ignore_ancestry, show_copies_as_adds,
                       use_git_diff_format, changelists,
                       ctx->cancel_func, ctx->cancel_baton,
                       pool));
  return SVN_NO_ERROR;
}


/* Perform a diff between two repository paths.

   PATH1 and PATH2 may be either URLs or the working copy paths.
   REVISION1 and REVISION2 are their respective revisions.
   If PEG_REVISION is specified, PATH2 is the path at the peg revision,
   and the actual two paths compared are determined by following copy
   history from PATH2.

   All other options are the same as those passed to svn_client_diff5(). */
static svn_error_t *
diff_repos_repos(const svn_wc_diff_callbacks4_t *callbacks,
                 struct diff_cmd_baton *callback_baton,
                 svn_client_ctx_t *ctx,
                 const char *path1,
                 const char *path2,
                 const svn_opt_revision_t *revision1,
                 const svn_opt_revision_t *revision2,
                 const svn_opt_revision_t *peg_revision,
                 svn_depth_t depth,
                 svn_boolean_t ignore_ancestry,
                 apr_pool_t *pool)
{
  svn_ra_session_t *extra_ra_session;

  const svn_ra_reporter3_t *reporter;
  void *reporter_baton;

  const svn_delta_editor_t *diff_editor;
  void *diff_edit_baton;

  const char *url1;
  const char *url2;
  const char *base_path;
  svn_revnum_t rev1;
  svn_revnum_t rev2;
  const char *anchor1;
  const char *anchor2;
  const char *target1;
  const char *target2;
  svn_ra_session_t *ra_session;

  /* Prepare info for the repos repos diff. */
  SVN_ERR(diff_prepare_repos_repos(&url1, &url2, &base_path, &rev1, &rev2,
                                   &anchor1, &anchor2, &target1, &target2,
                                   &ra_session, ctx, path1, path2,
                                   revision1, revision2, peg_revision,
                                   pool));

  /* Get actual URLs. */
  callback_baton->orig_path_1 = url1;
  callback_baton->orig_path_2 = url2;

  /* Get numeric revisions. */
  callback_baton->revnum1 = rev1;
  callback_baton->revnum2 = rev2;

  callback_baton->ra_session = ra_session;
  callback_baton->anchor = base_path;

  /* Now, we open an extra RA session to the correct anchor
     location for URL1.  This is used during the editor calls to fetch file
     contents.  */
  SVN_ERR(svn_client__open_ra_session_internal(&extra_ra_session, NULL,
                                               anchor1, NULL, NULL, FALSE,
                                               TRUE, ctx, pool));

  /* Set up the repos_diff editor on BASE_PATH, if available.
     Otherwise, we just use "". */
  SVN_ERR(svn_client__get_diff_editor(
                &diff_editor, &diff_edit_baton,
                NULL, "", depth,
                extra_ra_session, rev1, TRUE, FALSE,
                callbacks, callback_baton,
                ctx->cancel_func, ctx->cancel_baton,
                NULL /* no notify_func */, NULL /* no notify_baton */,
                pool, pool));

  /* We want to switch our txn into URL2 */
  SVN_ERR(svn_ra_do_diff3
          (ra_session, &reporter, &reporter_baton, rev2, target1,
           depth, ignore_ancestry, TRUE,
           url2, diff_editor, diff_edit_baton, pool));

  /* Drive the reporter; do the diff. */
  SVN_ERR(reporter->set_path(reporter_baton, "", rev1,
                             svn_depth_infinity,
                             FALSE, NULL,
                             pool));
  return reporter->finish_report(reporter_baton, pool);
}


/* Perform a diff between a repository path and a working-copy path.

   PATH1 may be either a URL or a working copy path.  PATH2 is a
   working copy path.  REVISION1 and REVISION2 are their respective
   revisions.  If REVERSE is TRUE, the diff will be done in reverse.
   If PEG_REVISION is specified, then PATH1 is the path in the peg
   revision, and the actual repository path to be compared is
   determined by following copy history.

   All other options are the same as those passed to svn_client_diff5(). */
static svn_error_t *
diff_repos_wc(const char *path1,
              const svn_opt_revision_t *revision1,
              const svn_opt_revision_t *peg_revision,
              const char *path2,
              const svn_opt_revision_t *revision2,
              svn_boolean_t reverse,
              svn_depth_t depth,
              svn_boolean_t ignore_ancestry,
              svn_boolean_t show_copies_as_adds,
              svn_boolean_t use_git_diff_format,
              const apr_array_header_t *changelists,
              const svn_wc_diff_callbacks4_t *callbacks,
              struct diff_cmd_baton *callback_baton,
              svn_client_ctx_t *ctx,
              apr_pool_t *pool)
{
  const char *url1, *anchor, *anchor_url, *target;
  svn_revnum_t rev;
  svn_ra_session_t *ra_session;
  svn_depth_t diff_depth;
  const svn_ra_reporter3_t *reporter;
  void *reporter_baton;
  const svn_delta_editor_t *diff_editor;
  void *diff_edit_baton;
  svn_boolean_t rev2_is_base = (revision2->kind == svn_opt_revision_base);
  svn_boolean_t server_supports_depth;
  const char *abspath1;
  const char *abspath2;
  const char *anchor_abspath;

  SVN_ERR_ASSERT(! svn_path_is_url(path2));

  if (!svn_path_is_url(path1))
    SVN_ERR(svn_dirent_get_absolute(&abspath1, path1, pool));
  else
    abspath1 = path1;

  SVN_ERR(svn_dirent_get_absolute(&abspath2, path2, pool));

  /* Convert path1 to a URL to feed to do_diff. */
  SVN_ERR(convert_to_url(&url1, ctx->wc_ctx, abspath1, pool, pool));

  SVN_ERR(svn_wc_get_actual_target2(&anchor, &target,
                                    ctx->wc_ctx, path2,
                                    pool, pool));

  /* Fetch the URL of the anchor directory. */
  SVN_ERR(svn_dirent_get_absolute(&anchor_abspath, anchor, pool));
  SVN_ERR(svn_wc__node_get_url(&anchor_url, ctx->wc_ctx, anchor_abspath,
                               pool, pool));
  if (! anchor_url)
    return svn_error_createf(SVN_ERR_ENTRY_MISSING_URL, NULL,
                             _("Directory '%s' has no URL"),
                             svn_dirent_local_style(anchor, pool));

  /* If we are performing a pegged diff, we need to find out what our
     actual URLs will be. */
  if (peg_revision->kind != svn_opt_revision_unspecified)
    {
      svn_opt_revision_t *start_ignore, *end_ignore, end;
      const char *url_ignore;

      end.kind = svn_opt_revision_unspecified;

      SVN_ERR(svn_client__repos_locations(&url1, &start_ignore,
                                          &url_ignore, &end_ignore,
                                          NULL,
                                          path1,
                                          peg_revision,
                                          revision1, &end,
                                          ctx, pool));
      if (!reverse)
        {
          callback_baton->orig_path_1 = url1;
          callback_baton->orig_path_2 =
            svn_path_url_add_component2(anchor_url, target, pool);
        }
      else
        {
          callback_baton->orig_path_1 =
            svn_path_url_add_component2(anchor_url, target, pool);
          callback_baton->orig_path_2 = url1;
        }
    }

  /* Establish RA session to path2's anchor */
  SVN_ERR(svn_client__open_ra_session_internal(&ra_session, NULL, anchor_url,
                                               NULL, NULL, FALSE, TRUE,
                                               ctx, pool));
  callback_baton->ra_session = ra_session;
  if (use_git_diff_format)
    {
      SVN_ERR(svn_wc__get_wc_root(&callback_baton->wc_root_abspath,
                                  ctx->wc_ctx, anchor_abspath,
                                  pool, pool));
    }
  callback_baton->anchor = anchor;

  SVN_ERR(svn_ra_has_capability(ra_session, &server_supports_depth,
                                SVN_RA_CAPABILITY_DEPTH, pool));

  SVN_ERR(svn_wc_get_diff_editor6(&diff_editor, &diff_edit_baton,
                                  ctx->wc_ctx,
                                  anchor_abspath,
                                  target,
                                  depth,
                                  ignore_ancestry,
                                  show_copies_as_adds,
                                  use_git_diff_format,
                                  rev2_is_base,
                                  reverse,
                                  server_supports_depth,
                                  changelists,
                                  callbacks, callback_baton,
                                  ctx->cancel_func, ctx->cancel_baton,
                                  pool, pool));

  /* Tell the RA layer we want a delta to change our txn to URL1 */
  SVN_ERR(svn_client__get_revision_number(&rev, NULL, ctx->wc_ctx,
                                          (strcmp(path1, url1) == 0)
                                                    ? NULL : abspath1,
                                          ra_session, revision1, pool));

  if (!reverse)
    callback_baton->revnum1 = rev;
  else
    callback_baton->revnum2 = rev;

  if (depth != svn_depth_infinity)
    diff_depth = depth;
  else
    diff_depth = svn_depth_unknown;

  SVN_ERR(svn_ra_do_diff3(ra_session,
                          &reporter, &reporter_baton,
                          rev,
                          target,
                          diff_depth,
                          ignore_ancestry,
                          TRUE,  /* text_deltas */
                          url1,
                          diff_editor, diff_edit_baton, pool));

  /* Create a txn mirror of path2;  the diff editor will print
     diffs in reverse.  :-)  */
  SVN_ERR(svn_wc_crawl_revisions5(ctx->wc_ctx, abspath2,
                                  reporter, reporter_baton,
                                  FALSE, depth, TRUE, (! server_supports_depth),
                                  FALSE,
                                  ctx->cancel_func, ctx->cancel_baton,
                                  NULL, NULL, /* notification is N/A */
                                  pool));

  return SVN_NO_ERROR;
}


/* This is basically just the guts of svn_client_diff[_peg]5(). */
static svn_error_t *
do_diff(const svn_wc_diff_callbacks4_t *callbacks,
        struct diff_cmd_baton *callback_baton,
        svn_client_ctx_t *ctx,
        const char *path1,
        const char *path2,
        const svn_opt_revision_t *revision1,
        const svn_opt_revision_t *revision2,
        const svn_opt_revision_t *peg_revision,
        svn_depth_t depth,
        svn_boolean_t ignore_ancestry,
        svn_boolean_t show_copies_as_adds,
        svn_boolean_t use_git_diff_format,
        const apr_array_header_t *changelists,
        apr_pool_t *pool)
{
  svn_boolean_t is_repos1;
  svn_boolean_t is_repos2;

  /* Check if paths/revisions are urls/local. */
  SVN_ERR(check_paths(&is_repos1, &is_repos2, path1, path2,
                      revision1, revision2, peg_revision));

  if (is_repos1)
    {
      if (is_repos2)
        {
          SVN_ERR(diff_repos_repos(callbacks, callback_baton, ctx,
                                   path1, path2, revision1, revision2,
                                   peg_revision, depth, ignore_ancestry,
                                   pool));
        }
      else /* path2 is a working copy path */
        {
          SVN_ERR(diff_repos_wc(path1, revision1, peg_revision,
                                path2, revision2, FALSE, depth,
                                ignore_ancestry, show_copies_as_adds,
                                use_git_diff_format, changelists,
                                callbacks, callback_baton, ctx, pool));
        }
    }
  else /* path1 is a working copy path */
    {
      if (is_repos2)
        {
          SVN_ERR(diff_repos_wc(path2, revision2, peg_revision,
                                path1, revision1, TRUE, depth,
                                ignore_ancestry, show_copies_as_adds,
                                use_git_diff_format, changelists,
                                callbacks, callback_baton, ctx, pool));
        }
      else /* path2 is a working copy path */
        {
          SVN_ERR(diff_wc_wc(path1, revision1, path2, revision2,
                             depth, ignore_ancestry, show_copies_as_adds,
                             use_git_diff_format, changelists,
                             callbacks, callback_baton, ctx, pool));
        }
    }

  return SVN_NO_ERROR;
}

/* Perform a diff summary between two repository paths. */
static svn_error_t *
diff_summarize_repos_repos(svn_client_diff_summarize_func_t summarize_func,
                           void *summarize_baton,
                           svn_client_ctx_t *ctx,
                           const char *path1,
                           const char *path2,
                           const svn_opt_revision_t *revision1,
                           const svn_opt_revision_t *revision2,
                           const svn_opt_revision_t *peg_revision,
                           svn_depth_t depth,
                           svn_boolean_t ignore_ancestry,
                           apr_pool_t *pool)
{
  svn_ra_session_t *extra_ra_session;

  const svn_ra_reporter3_t *reporter;
  void *reporter_baton;

  const svn_delta_editor_t *diff_editor;
  void *diff_edit_baton;

  const char *url1;
  const char *url2;
  const char *base_path;
  svn_revnum_t rev1;
  svn_revnum_t rev2;
  const char *anchor1;
  const char *anchor2;
  const char *target1;
  const char *target2;
  svn_ra_session_t *ra_session;

  /* Prepare info for the repos repos diff. */
  SVN_ERR(diff_prepare_repos_repos(&url1, &url2, &base_path, &rev1, &rev2,
                                   &anchor1, &anchor2, &target1, &target2,
                                   &ra_session, ctx,
                                   path1, path2, revision1, revision2,
                                   peg_revision, pool));

  /* Now, we open an extra RA session to the correct anchor
     location for URL1.  This is used to get the kind of deleted paths.  */
  SVN_ERR(svn_client__open_ra_session_internal(&extra_ra_session, NULL,
                                               anchor1, NULL, NULL, FALSE,
                                               TRUE, ctx, pool));

  /* Set up the repos_diff editor. */
  SVN_ERR(svn_client__get_diff_summarize_editor
          (target2, summarize_func,
           summarize_baton, extra_ra_session, rev1, ctx->cancel_func,
           ctx->cancel_baton, &diff_editor, &diff_edit_baton, pool));

  /* We want to switch our txn into URL2 */
  SVN_ERR(svn_ra_do_diff3
          (ra_session, &reporter, &reporter_baton, rev2, target1,
           depth, ignore_ancestry,
           FALSE /* do not create text delta */, url2, diff_editor,
           diff_edit_baton, pool));

  /* Drive the reporter; do the diff. */
  SVN_ERR(reporter->set_path(reporter_baton, "", rev1,
                             svn_depth_infinity,
                             FALSE, NULL, pool));
  return reporter->finish_report(reporter_baton, pool);
}

/* This is basically just the guts of svn_client_diff_summarize[_peg]2(). */
static svn_error_t *
do_diff_summarize(svn_client_diff_summarize_func_t summarize_func,
                  void *summarize_baton,
                  svn_client_ctx_t *ctx,
                  const char *path1,
                  const char *path2,
                  const svn_opt_revision_t *revision1,
                  const svn_opt_revision_t *revision2,
                  const svn_opt_revision_t *peg_revision,
                  svn_depth_t depth,
                  svn_boolean_t ignore_ancestry,
                  apr_pool_t *pool)
{
  svn_boolean_t is_repos1;
  svn_boolean_t is_repos2;

  /* Check if paths/revisions are urls/local. */
  SVN_ERR(check_paths(&is_repos1, &is_repos2, path1, path2,
                      revision1, revision2, peg_revision));

  if (is_repos1 && is_repos2)
    return diff_summarize_repos_repos(summarize_func, summarize_baton, ctx,
                                      path1, path2, revision1, revision2,
                                      peg_revision, depth, ignore_ancestry,
                                      pool);
  else
    return svn_error_create(SVN_ERR_UNSUPPORTED_FEATURE, NULL,
                            _("Summarizing diff can only compare repository "
                              "to repository"));
}


/* Initialize DIFF_CMD_BATON.diff_cmd and DIFF_CMD_BATON.options,
 * according to OPTIONS and CONFIG.  CONFIG and OPTIONS may be null.
 * Allocate the fields in POOL, which should be at least as long-lived
 * as the pool DIFF_CMD_BATON itself is allocated in.
 */
static svn_error_t *
set_up_diff_cmd_and_options(struct diff_cmd_baton *diff_cmd_baton,
                            const apr_array_header_t *options,
                            apr_hash_t *config, apr_pool_t *pool)
{
  const char *diff_cmd = NULL;

  /* See if there is a diff command and/or diff arguments. */
  if (config)
    {
      svn_config_t *cfg = apr_hash_get(config, SVN_CONFIG_CATEGORY_CONFIG,
                                       APR_HASH_KEY_STRING);
      svn_config_get(cfg, &diff_cmd, SVN_CONFIG_SECTION_HELPERS,
                     SVN_CONFIG_OPTION_DIFF_CMD, NULL);
      if (options == NULL)
        {
          const char *diff_extensions;
          svn_config_get(cfg, &diff_extensions, SVN_CONFIG_SECTION_HELPERS,
                         SVN_CONFIG_OPTION_DIFF_EXTENSIONS, NULL);
          if (diff_extensions)
            options = svn_cstring_split(diff_extensions, " \t\n\r", TRUE, pool);
        }
    }

  if (options == NULL)
    options = apr_array_make(pool, 0, sizeof(const char *));

  if (diff_cmd)
    SVN_ERR(svn_path_cstring_to_utf8(&diff_cmd_baton->diff_cmd, diff_cmd,
                                     pool));
  else
    diff_cmd_baton->diff_cmd = NULL;

  /* If there was a command, arrange options to pass to it. */
  if (diff_cmd_baton->diff_cmd)
    {
      const char **argv = NULL;
      int argc = options->nelts;
      if (argc)
        {
          int i;
          argv = apr_palloc(pool, argc * sizeof(char *));
          for (i = 0; i < argc; i++)
            SVN_ERR(svn_utf_cstring_to_utf8(&argv[i],
                      APR_ARRAY_IDX(options, i, const char *), pool));
        }
      diff_cmd_baton->options.for_external.argv = argv;
      diff_cmd_baton->options.for_external.argc = argc;
    }
  else  /* No command, so arrange options for internal invocation instead. */
    {
      diff_cmd_baton->options.for_internal
        = svn_diff_file_options_create(pool);
      SVN_ERR(svn_diff_file_options_parse
              (diff_cmd_baton->options.for_internal, options, pool));
    }

  return SVN_NO_ERROR;
}

/*----------------------------------------------------------------------- */

/*** Public Interfaces. ***/

/* Display context diffs between two PATH/REVISION pairs.  Each of
   these inputs will be one of the following:

   - a repository URL at a given revision.
   - a working copy path, ignoring local mods.
   - a working copy path, including local mods.

   We can establish a matrix that shows the nine possible types of
   diffs we expect to support.


      ` .     DST ||  URL:rev   | WC:base    | WC:working |
          ` .     ||            |            |            |
      SRC     ` . ||            |            |            |
      ============++============+============+============+
       URL:rev    || (*)        | (*)        | (*)        |
                  ||            |            |            |
                  ||            |            |            |
                  ||            |            |            |
      ------------++------------+------------+------------+
       WC:base    || (*)        |                         |
                  ||            | New svn_wc_diff which   |
                  ||            | is smart enough to      |
                  ||            | handle two WC paths     |
      ------------++------------+ and their related       +
       WC:working || (*)        | text-bases and working  |
                  ||            | files.  This operation  |
                  ||            | is entirely local.      |
                  ||            |                         |
      ------------++------------+------------+------------+
      * These cases require server communication.
*/
svn_error_t *
svn_client_diff5(const apr_array_header_t *options,
                 const char *path1,
                 const svn_opt_revision_t *revision1,
                 const char *path2,
                 const svn_opt_revision_t *revision2,
                 const char *relative_to_dir,
                 svn_depth_t depth,
                 svn_boolean_t ignore_ancestry,
                 svn_boolean_t no_diff_deleted,
                 svn_boolean_t show_copies_as_adds,
                 svn_boolean_t ignore_content_type,
                 svn_boolean_t use_git_diff_format,
                 const char *header_encoding,
                 apr_file_t *outfile,
                 apr_file_t *errfile,
                 const apr_array_header_t *changelists,
                 svn_client_ctx_t *ctx,
                 apr_pool_t *pool)
{
  struct diff_cmd_baton diff_cmd_baton = { 0 };
  svn_wc_diff_callbacks4_t diff_callbacks;

  /* We will never do a pegged diff from here. */
  svn_opt_revision_t peg_revision;
  peg_revision.kind = svn_opt_revision_unspecified;

  /* setup callback and baton */
  diff_callbacks.file_opened = diff_file_opened;
  diff_callbacks.file_changed = diff_file_changed;
  diff_callbacks.file_added = diff_file_added;
  diff_callbacks.file_deleted = no_diff_deleted ? diff_file_deleted_no_diff :
                                                  diff_file_deleted_with_diff;
  diff_callbacks.dir_added =  diff_dir_added;
  diff_callbacks.dir_deleted = diff_dir_deleted;
  diff_callbacks.dir_props_changed = diff_dir_props_changed;
  diff_callbacks.dir_opened = diff_dir_opened;
  diff_callbacks.dir_closed = diff_dir_closed;

  diff_cmd_baton.orig_path_1 = path1;
  diff_cmd_baton.orig_path_2 = path2;

  SVN_ERR(set_up_diff_cmd_and_options(&diff_cmd_baton, options,
                                      ctx->config, pool));
  diff_cmd_baton.pool = pool;
  diff_cmd_baton.outfile = outfile;
  diff_cmd_baton.errfile = errfile;
  diff_cmd_baton.header_encoding = header_encoding;
  diff_cmd_baton.revnum1 = SVN_INVALID_REVNUM;
  diff_cmd_baton.revnum2 = SVN_INVALID_REVNUM;

  diff_cmd_baton.force_empty = FALSE;
  diff_cmd_baton.force_binary = ignore_content_type;
  diff_cmd_baton.relative_to_dir = relative_to_dir;
  diff_cmd_baton.use_git_diff_format = use_git_diff_format;
  diff_cmd_baton.wc_ctx = ctx->wc_ctx;
  diff_cmd_baton.visited_paths = apr_hash_make(pool);
  diff_cmd_baton.ra_session = NULL;
  diff_cmd_baton.wc_root_abspath = NULL;
  diff_cmd_baton.anchor = NULL;

  return do_diff(&diff_callbacks, &diff_cmd_baton, ctx,
                 path1, path2, revision1, revision2, &peg_revision,
                 depth, ignore_ancestry, show_copies_as_adds,
                 use_git_diff_format, changelists, pool);
}

svn_error_t *
svn_client_diff_peg5(const apr_array_header_t *options,
                     const char *path,
                     const svn_opt_revision_t *peg_revision,
                     const svn_opt_revision_t *start_revision,
                     const svn_opt_revision_t *end_revision,
                     const char *relative_to_dir,
                     svn_depth_t depth,
                     svn_boolean_t ignore_ancestry,
                     svn_boolean_t no_diff_deleted,
                     svn_boolean_t show_copies_as_adds,
                     svn_boolean_t ignore_content_type,
                     svn_boolean_t use_git_diff_format,
                     const char *header_encoding,
                     apr_file_t *outfile,
                     apr_file_t *errfile,
                     const apr_array_header_t *changelists,
                     svn_client_ctx_t *ctx,
                     apr_pool_t *pool)
{
  struct diff_cmd_baton diff_cmd_baton = { 0 };
  svn_wc_diff_callbacks4_t diff_callbacks;

  /* setup callback and baton */
  diff_callbacks.file_opened = diff_file_opened;
  diff_callbacks.file_changed = diff_file_changed;
  diff_callbacks.file_added = diff_file_added;
  diff_callbacks.file_deleted = no_diff_deleted ? diff_file_deleted_no_diff :
                                                  diff_file_deleted_with_diff;
  diff_callbacks.dir_added =  diff_dir_added;
  diff_callbacks.dir_deleted = diff_dir_deleted;
  diff_callbacks.dir_props_changed = diff_dir_props_changed;
  diff_callbacks.dir_opened = diff_dir_opened;
  diff_callbacks.dir_closed = diff_dir_closed;

  diff_cmd_baton.orig_path_1 = path;
  diff_cmd_baton.orig_path_2 = path;

  SVN_ERR(set_up_diff_cmd_and_options(&diff_cmd_baton, options,
                                      ctx->config, pool));
  diff_cmd_baton.pool = pool;
  diff_cmd_baton.outfile = outfile;
  diff_cmd_baton.errfile = errfile;
  diff_cmd_baton.header_encoding = header_encoding;
  diff_cmd_baton.revnum1 = SVN_INVALID_REVNUM;
  diff_cmd_baton.revnum2 = SVN_INVALID_REVNUM;

  diff_cmd_baton.force_empty = FALSE;
  diff_cmd_baton.force_binary = ignore_content_type;
  diff_cmd_baton.relative_to_dir = relative_to_dir;
  diff_cmd_baton.use_git_diff_format = use_git_diff_format;
  diff_cmd_baton.wc_ctx = ctx->wc_ctx;
  diff_cmd_baton.visited_paths = apr_hash_make(pool);
  diff_cmd_baton.ra_session = NULL;
  diff_cmd_baton.wc_root_abspath = NULL;
  diff_cmd_baton.anchor = NULL;

  return do_diff(&diff_callbacks, &diff_cmd_baton, ctx,
                 path, path, start_revision, end_revision, peg_revision,
                 depth, ignore_ancestry, show_copies_as_adds,
                 use_git_diff_format, changelists, pool);
}

svn_error_t *
svn_client_diff_summarize2(const char *path1,
                           const svn_opt_revision_t *revision1,
                           const char *path2,
                           const svn_opt_revision_t *revision2,
                           svn_depth_t depth,
                           svn_boolean_t ignore_ancestry,
                           const apr_array_header_t *changelists,
                           svn_client_diff_summarize_func_t summarize_func,
                           void *summarize_baton,
                           svn_client_ctx_t *ctx,
                           apr_pool_t *pool)
{
  /* We will never do a pegged diff from here. */
  svn_opt_revision_t peg_revision;
  peg_revision.kind = svn_opt_revision_unspecified;

  /* ### CHANGELISTS parameter isn't used */
  return do_diff_summarize(summarize_func, summarize_baton, ctx,
                           path1, path2, revision1, revision2, &peg_revision,
                           depth, ignore_ancestry, pool);
}

svn_error_t *
svn_client_diff_summarize_peg2(const char *path,
                               const svn_opt_revision_t *peg_revision,
                               const svn_opt_revision_t *start_revision,
                               const svn_opt_revision_t *end_revision,
                               svn_depth_t depth,
                               svn_boolean_t ignore_ancestry,
                               const apr_array_header_t *changelists,
                               svn_client_diff_summarize_func_t summarize_func,
                               void *summarize_baton,
                               svn_client_ctx_t *ctx,
                               apr_pool_t *pool)
{
  /* ### CHANGELISTS parameter isn't used */
  return do_diff_summarize(summarize_func, summarize_baton, ctx,
                           path, path, start_revision, end_revision,
                           peg_revision, depth, ignore_ancestry, pool);
}

svn_client_diff_summarize_t *
svn_client_diff_summarize_dup(const svn_client_diff_summarize_t *diff,
                              apr_pool_t *pool)
{
  svn_client_diff_summarize_t *dup_diff = apr_palloc(pool, sizeof(*dup_diff));

  *dup_diff = *diff;

  if (diff->path)
    dup_diff->path = apr_pstrdup(pool, diff->path);

  return dup_diff;
}

/*
 * questions.c:  routines for asking questions about working copies
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



#include <string.h>

#include <apr_pools.h>
#include <apr_file_io.h>
#include <apr_file_info.h>
#include <apr_time.h>

#include "svn_pools.h"
#include "svn_types.h"
#include "svn_string.h"
#include "svn_error.h"
#include "svn_dirent_uri.h"
#include "svn_time.h"
#include "svn_io.h"
#include "svn_props.h"

#include "wc.h"
#include "adm_files.h"
#include "props.h"
#include "translate.h"
#include "wc_db.h"

#include "svn_private_config.h"
#include "private/svn_wc_private.h"



/*** svn_wc_text_modified_p ***/

/* svn_wc_text_modified_p answers the question:

   "Are the contents of F different than the contents of
   .svn/text-base/F.svn-base or .svn/tmp/text-base/F.svn-base?"

   In the first case, we're looking to see if a user has made local
   modifications to a file since the last update or commit.  In the
   second, the file may not be versioned yet (it doesn't exist in
   entries).  Support for the latter case came about to facilitate
   forced checkouts, updates, and switches, where an unversioned file
   may obstruct a file about to be added.

   Note: Assuming that F lives in a directory D at revision V, please
   notice that we are *NOT* answering the question, "are the contents
   of F different than revision V of F?"  While F may be at a different
   revision number than its parent directory, but we're only looking
   for local edits on F, not for consistent directory revisions.

   TODO:  the logic of the routines on this page might change in the
   future, as they bear some relation to the user interface.  For
   example, if a file is removed -- without telling subversion about
   it -- how should subversion react?  Should it copy the file back
   out of text-base?  Should it ask whether one meant to officially
   mark it for removal?
*/


/* Set *MODIFIED_P to TRUE if (after translation) VERSIONED_FILE_ABSPATH
 * (of VERSIONED_FILE_SIZE bytes) differs from PRISTINE_STREAM (of
 * PRISTINE_SIZE bytes), else to FALSE if not.
 *
 * If EXACT_COMPARISON is FALSE, translate VERSIONED_FILE_ABSPATH's EOL
 * style and keywords to repository-normal form according to its properties,
 * and compare the result with PRISTINE_STREAM.  If EXACT_COMPARISON is
 * TRUE, translate PRISTINE_STREAM's EOL style and keywords to working-copy
 * form according to VERSIONED_FILE_ABSPATH's properties, and compare the
 * result with VERSIONED_FILE_ABSPATH.
 *
 * HAS_PROPS should be TRUE if the file had properties when it was not
 * modified, otherwise FALSE.
 *
 * PROPS_MOD should be TRUE if the file's properties have been changed,
 * otherwise FALSE.
 *
 * PRISTINE_STREAM will be closed before a successful return.
 *
 * DB is a wc_db; use SCRATCH_POOL for temporary allocation.
 */
static svn_error_t *
compare_and_verify(svn_boolean_t *modified_p,
                   svn_wc__db_t *db,
                   const char *versioned_file_abspath,
                   svn_filesize_t versioned_file_size,
                   svn_stream_t *pristine_stream,
                   svn_filesize_t pristine_size,
                   svn_boolean_t has_props,
                   svn_boolean_t props_mod,
                   svn_boolean_t exact_comparison,
                   apr_pool_t *scratch_pool)
{
  svn_boolean_t same;
  svn_subst_eol_style_t eol_style;
  const char *eol_str;
  apr_hash_t *keywords;
  svn_boolean_t special = FALSE;
  svn_boolean_t need_translation;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(versioned_file_abspath));

  if (props_mod)
    has_props = TRUE; /* Maybe it didn't have properties; but it has now */

  if (has_props)
    {
      SVN_ERR(svn_wc__get_translate_info(&eol_style, &eol_str,
                                         &keywords,
                                         &special,
                                         db, versioned_file_abspath, NULL,
                                         !exact_comparison,
                                         scratch_pool, scratch_pool));

      need_translation = svn_subst_translation_required(eol_style, eol_str,
                                                        keywords, special,
                                                        TRUE);
    }
  else
    need_translation = FALSE;

  if (! need_translation
      && (versioned_file_size != pristine_size))
    {
      *modified_p = TRUE;

      /* ### Why did we open the pristine? */
      return svn_error_trace(svn_stream_close(pristine_stream));
    }

#if 0
  /* ### On second thought, I think this needs more review before enabling
     ### This case might break when we have a fixed "\r\n" EOL, because
     ### we use a repair mode in the compare itself. */
  if (need_translation
      && !special
      && !props_mod
      && (keywords == NULL)
      && (versioned_file_size < pristine_file_size))
    {
      *modified_p = TRUE; /* The file is < its repository normal form
                             and the properties didn't change.

                             That must be a change. */

      /* ### Why did we open the pristine? */
      return svn_error_trace(svn_stream_close(pristine_stream));
    }
#endif

  /* ### Other checks possible? */

  if (need_translation)
    {
      /* Reading files is necessary. */
      svn_stream_t *v_stream;  /* versioned_file */

      if (special)
        {
          SVN_ERR(svn_subst_read_specialfile(&v_stream, versioned_file_abspath,
                                             scratch_pool, scratch_pool));
        }
      else
        {
          SVN_ERR(svn_stream_open_readonly(&v_stream, versioned_file_abspath,
                                           scratch_pool, scratch_pool));

          if (!exact_comparison && need_translation)
            {
              if (eol_style == svn_subst_eol_style_native)
                eol_str = SVN_SUBST_NATIVE_EOL_STR;
              else if (eol_style != svn_subst_eol_style_fixed
                       && eol_style != svn_subst_eol_style_none)
                return svn_error_create(SVN_ERR_IO_UNKNOWN_EOL, NULL, NULL);

              /* Wrap file stream to detranslate into normal form,
               * "repairing" the EOL style if it is inconsistent. */
              v_stream = svn_subst_stream_translated(v_stream,
                                                     eol_str,
                                                     TRUE /* repair */,
                                                     keywords,
                                                     FALSE /* expand */,
                                                     scratch_pool);
            }
          else if (need_translation)
            {
              /* Wrap base stream to translate into working copy form, and
               * arrange to throw an error if its EOL style is inconsistent. */
              pristine_stream = svn_subst_stream_translated(pristine_stream,
                                                            eol_str, FALSE,
                                                            keywords, TRUE,
                                                            scratch_pool);
            }
        }

      SVN_ERR(svn_stream_contents_same2(&same, pristine_stream, v_stream,
                                        scratch_pool));
    }
  else
    {
      /* Translation would be a no-op, so compare the original file. */
      svn_stream_t *v_stream;  /* versioned_file */

      SVN_ERR(svn_stream_open_readonly(&v_stream, versioned_file_abspath,
                                       scratch_pool, scratch_pool));

      SVN_ERR(svn_stream_contents_same2(&same, pristine_stream, v_stream,
                                        scratch_pool));
    }

  *modified_p = (! same);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__internal_file_modified_p(svn_boolean_t *modified_p,
                                 svn_wc__db_t *db,
                                 const char *local_abspath,
                                 svn_boolean_t exact_comparison,
                                 apr_pool_t *scratch_pool)
{
  svn_stream_t *pristine_stream;
  svn_filesize_t pristine_size;
  svn_wc__db_status_t status;
  svn_wc__db_kind_t kind;
  const svn_checksum_t *checksum;
  svn_filesize_t recorded_size;
  apr_time_t recorded_mod_time;
  svn_boolean_t has_props;
  svn_boolean_t props_mod;
  const svn_io_dirent2_t *dirent;

  /* Read the relevant info */
  SVN_ERR(svn_wc__db_read_info(&status, &kind, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, &checksum, NULL, NULL, NULL,
                               NULL, NULL, NULL,
                               &recorded_size, &recorded_mod_time,
                               NULL, NULL, NULL, &has_props, &props_mod,
                               NULL, NULL, NULL,
                               db, local_abspath,
                               scratch_pool, scratch_pool));

  /* If we don't have a pristine or the node has a status that allows a
     pristine, just say that the node is modified */
  if (!checksum
      || (kind != svn_wc__db_kind_file)
      || ((status != svn_wc__db_status_normal)
          && (status != svn_wc__db_status_added)))
    {
      *modified_p = TRUE;
      return SVN_NO_ERROR;
    }

  SVN_ERR(svn_io_stat_dirent(&dirent, local_abspath, TRUE,
                             scratch_pool, scratch_pool));

  if (dirent->kind != svn_node_file)
    {
      /* There is no file on disk, so the text is missing, not modified. */
      *modified_p = FALSE;
      return SVN_NO_ERROR;
    }

  if (! exact_comparison)
    {
      /* We're allowed to use a heuristic to determine whether files may
         have changed.  The heuristic has these steps:

         1. Compare the working file's size
            with the size cached in the entries file
         2. If they differ, do a full file compare
         3. Compare the working file's timestamp
            with the timestamp cached in the entries file
         4. If they differ, do a full file compare
         5. Otherwise, return indicating an unchanged file.

         There are 2 problematic situations which may occur:

         1. The cached working size is missing
         --> In this case, we forget we ever tried to compare
             and skip to the timestamp comparison.  This is
             because old working copies do not contain cached sizes

         2. The cached timestamp is missing
         --> In this case, we forget we ever tried to compare
             and skip to full file comparison.  This is because
             the timestamp will be removed when the library
             updates a locally changed file.  (ie, this only happens
             when the file was locally modified.)

      */

      /* Compare the sizes, if applicable */
      if (recorded_size != SVN_INVALID_FILESIZE
          && dirent->filesize != recorded_size)
        goto compare_them;

      /* Compare the timestamps

         Note: recorded_mod_time == 0 means not available,
               which also means the timestamps won't be equal,
               so there's no need to explicitly check the 'absent' value. */
      if (recorded_mod_time != dirent->mtime)
        goto compare_them;

      *modified_p = FALSE;
      return SVN_NO_ERROR;
    }

 compare_them:
  SVN_ERR(svn_wc__db_pristine_read(&pristine_stream, &pristine_size,
                                   db, local_abspath, checksum,
                                   scratch_pool, scratch_pool));

  /* Check all bytes, and verify checksum if requested. */
  {
    svn_error_t *err;
    err = compare_and_verify(modified_p, db,
                             local_abspath, dirent->filesize,
                             pristine_stream, pristine_size,
                             has_props, props_mod,
                             exact_comparison,
                             scratch_pool);

    /* At this point we already opened the pristine file, so we know that
       the access denied applies to the working copy path */
    if (err && APR_STATUS_IS_EACCES(err->apr_err))
      return svn_error_create(SVN_ERR_WC_PATH_ACCESS_DENIED, err, NULL);
    else
      SVN_ERR(err);
  }

  if (!*modified_p)
    {
      svn_boolean_t own_lock;

      /* The timestamp is missing or "broken" so "repair" it if we can. */
      SVN_ERR(svn_wc__db_wclock_owns_lock(&own_lock, db, local_abspath, FALSE,
                                          scratch_pool));
      if (own_lock)
        SVN_ERR(svn_wc__db_global_record_fileinfo(db, local_abspath,
                                                  dirent->filesize,
                                                  dirent->mtime,
                                                  scratch_pool));
    }

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc_text_modified_p2(svn_boolean_t *modified_p,
                        svn_wc_context_t *wc_ctx,
                        const char *local_abspath,
                        svn_boolean_t force_comparison,
                        apr_pool_t *scratch_pool)
{
  /* ### We ignore FORCE_COMPARISON, but we also fixed its only
         remaining use-case */
  return svn_wc__internal_file_modified_p(modified_p, wc_ctx->db,
                                          local_abspath, FALSE, scratch_pool);
}



svn_error_t *
svn_wc__internal_conflicted_p(svn_boolean_t *text_conflicted_p,
                              svn_boolean_t *prop_conflicted_p,
                              svn_boolean_t *tree_conflicted_p,
                              svn_wc__db_t *db,
                              const char *local_abspath,
                              apr_pool_t *scratch_pool)
{
  svn_node_kind_t kind;
  svn_wc__db_kind_t node_kind;
  const apr_array_header_t *conflicts;
  int i;
  svn_boolean_t conflicted;

  if (text_conflicted_p)
    *text_conflicted_p = FALSE;
  if (prop_conflicted_p)
    *prop_conflicted_p = FALSE;
  if (tree_conflicted_p)
    *tree_conflicted_p = FALSE;

  SVN_ERR(svn_wc__db_read_info(NULL, &node_kind, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, &conflicted,
                               NULL, NULL, NULL, NULL, NULL, NULL,
                               db, local_abspath, scratch_pool,
                               scratch_pool));

  if (!conflicted)
    return SVN_NO_ERROR;


  SVN_ERR(svn_wc__db_read_conflicts(&conflicts, db, local_abspath,
                                    scratch_pool, scratch_pool));

  for (i = 0; i < conflicts->nelts; i++)
    {
      const svn_wc_conflict_description2_t *cd;
      cd = APR_ARRAY_IDX(conflicts, i, const svn_wc_conflict_description2_t *);

      switch (cd->kind)
        {
          case svn_wc_conflict_kind_text:
            /* Look for any text conflict, exercising only as much effort as
               necessary to obtain a definitive answer.  This only applies to
               files, but we don't have to explicitly check that entry is a
               file, since these attributes would never be set on a directory
               anyway.  A conflict file entry notation only counts if the
               conflict file still exists on disk.  */

            if (!text_conflicted_p || *text_conflicted_p)
              break;

            if (cd->base_abspath)
              {
                SVN_ERR(svn_io_check_path(cd->base_abspath, &kind,
                                          scratch_pool));

                *text_conflicted_p = (kind == svn_node_file);

                if (*text_conflicted_p)
                  break;
              }

            if (cd->their_abspath)
              {
                SVN_ERR(svn_io_check_path(cd->their_abspath, &kind,
                                          scratch_pool));

                *text_conflicted_p = (kind == svn_node_file);

                if (*text_conflicted_p)
                  break;
              }

            if (cd->my_abspath)
              {
                SVN_ERR(svn_io_check_path(cd->my_abspath, &kind,
                                          scratch_pool));

                *text_conflicted_p = (kind == svn_node_file);
              }
            break;

          case svn_wc_conflict_kind_property:
            if (!prop_conflicted_p || *prop_conflicted_p)
              break;

            if (cd->their_abspath)
              {
                SVN_ERR(svn_io_check_path(cd->their_abspath, &kind,
                                          scratch_pool));

                *prop_conflicted_p = (kind == svn_node_file);
              }

            break;

          case svn_wc_conflict_kind_tree:
            if (tree_conflicted_p)
              *tree_conflicted_p = TRUE;

            break;

          default:
            /* Ignore other conflict types */
            break;
        }
    }
  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc_conflicted_p3(svn_boolean_t *text_conflicted_p,
                     svn_boolean_t *prop_conflicted_p,
                     svn_boolean_t *tree_conflicted_p,
                     svn_wc_context_t *wc_ctx,
                     const char *local_abspath,
                     apr_pool_t *scratch_pool)
{
  return svn_error_trace(svn_wc__internal_conflicted_p(text_conflicted_p,
                                                       prop_conflicted_p,
                                                       tree_conflicted_p,
                                                       wc_ctx->db,
                                                       local_abspath,
                                                       scratch_pool));
}

svn_error_t *
svn_wc__min_max_revisions(svn_revnum_t *min_revision,
                          svn_revnum_t *max_revision,
                          svn_wc_context_t *wc_ctx,
                          const char *local_abspath,
                          svn_boolean_t committed,
                          apr_pool_t *scratch_pool)
{
  return svn_error_trace(svn_wc__db_min_max_revisions(min_revision,
                                                      max_revision,
                                                      wc_ctx->db,
                                                      local_abspath,
                                                      committed,
                                                      scratch_pool));
}


svn_error_t *
svn_wc__is_sparse_checkout(svn_boolean_t *is_sparse_checkout,
                           svn_wc_context_t *wc_ctx,
                           const char *local_abspath,
                           apr_pool_t *scratch_pool)
{
  return svn_error_trace(svn_wc__db_is_sparse_checkout(is_sparse_checkout,
                                                       wc_ctx->db,
                                                       local_abspath,
                                                       scratch_pool));
}


svn_error_t *
svn_wc__has_switched_subtrees(svn_boolean_t *is_switched,
                              svn_wc_context_t *wc_ctx,
                              const char *local_abspath,
                              const char *trail_url,
                              apr_pool_t *scratch_pool)
{
  return svn_error_trace(svn_wc__db_has_switched_subtrees(is_switched,
                                                          wc_ctx->db,
                                                          local_abspath,
                                                          trail_url,
                                                          scratch_pool));
}


svn_error_t *
svn_wc__has_local_mods(svn_boolean_t *is_modified,
                       svn_wc_context_t *wc_ctx,
                       const char *local_abspath,
                       svn_cancel_func_t cancel_func,
                       void *cancel_baton,
                       apr_pool_t *scratch_pool)
{
  return svn_error_trace(svn_wc__db_has_local_mods(is_modified,
                                                   wc_ctx->db,
                                                   local_abspath,
                                                   cancel_func,
                                                   cancel_baton,
                                                   scratch_pool));
}

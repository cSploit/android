/*
 * merge.c:  merging changes into a working file
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

#include "svn_wc.h"
#include "svn_diff.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_pools.h"

#include "wc.h"
#include "adm_files.h"
#include "translate.h"
#include "workqueue.h"

#include "private/svn_skel.h"

#include "svn_private_config.h"

/* Contains some information on the merge target before merge, and some
   information needed for the diff processing. */
typedef struct merge_target_t
{
  svn_wc__db_t *db;                         /* The DB used to access target */
  const char *local_abspath;                /* The absolute path to target */
  const char *wri_abspath;                  /* The working copy of target */

  apr_hash_t *actual_props;                 /* The set of actual properties
                                               before merging */
  const apr_array_header_t *prop_diff;      /* The property changes */

  const char *diff3_cmd;                    /* The diff3 command and options */
  const apr_array_header_t *merge_options;

} merge_target_t;


/* Return a pointer to the svn_prop_t structure from PROP_DIFF
   belonging to PROP_NAME, if any.  NULL otherwise.*/
static const svn_prop_t *
get_prop(const merge_target_t *mt,
         const char *prop_name)
{
  if (mt && mt->prop_diff)
    {
      int i;
      for (i = 0; i < mt->prop_diff->nelts; i++)
        {
          const svn_prop_t *elt = &APR_ARRAY_IDX(mt->prop_diff, i,
                                                 svn_prop_t);

          if (strcmp(elt->name,prop_name) == 0)
            return elt;
        }
    }

  return NULL;
}


/* Detranslate a working copy file MERGE_TARGET to achieve the effect of:

   1. Detranslate
   2. Install new props
   3. Retranslate
   4. Detranslate

   in 1 pass to get a file which can be compared with the left and right
   files which were created with the 'new props' above.

   Property changes make this a little complex though. Changes in

   - svn:mime-type
   - svn:eol-style
   - svn:keywords
   - svn:special

   may change the way a file is translated.

   Effect for svn:mime-type:

     The value for svn:mime-type affects the translation wrt keywords
     and eol-style settings.

   I) both old and new mime-types are texty
      -> just do the translation dance (as lined out below)

   II) the old one is texty, the new one is binary
      -> detranslate with the old eol-style and keywords
         (the new re+detranslation is a no-op)

   III) the old one is binary, the new one texty
      -> detranslate with the new eol-style
         (the old detranslation is a no-op)

   IV) the old and new ones are binary
      -> don't detranslate, just make a straight copy


   Effect for svn:eol-style

   I) On add or change use the new value

   II) otherwise: use the old value (absent means 'no translation')


   Effect for svn:keywords

     Always use old settings (re+detranslation are no-op)


   Effect for svn:special

     Always use the old settings (same reasons as for svn:keywords)

  Sets *DETRANSLATED_ABSPATH to the path to the detranslated file,
  this may be the same as SOURCE_ABSPATH if FORCE_COPY is FALSE and no
  translation is required.

  If FORCE_COPY is FALSE and *DETRANSLATED_ABSPATH is a file distinct
  from SOURCE_ABSPATH then the file will be deleted on RESULT_POOL
  cleanup.

  If FORCE_COPY is TRUE then *DETRANSLATED_ABSPATH will always be a
  new file distinct from SOURCE_ABSPATH and it will be the callers
  responsibility to delete the file.

*/
static svn_error_t *
detranslate_wc_file(const char **detranslated_abspath,
                    const merge_target_t *mt,
                    svn_boolean_t force_copy,
                    const char *source_abspath,
                    svn_cancel_func_t cancel_func,
                    void *cancel_baton,
                    apr_pool_t *result_pool,
                    apr_pool_t *scratch_pool)
{
  svn_boolean_t is_binary;
  const svn_prop_t *prop;
  svn_subst_eol_style_t style;
  const char *eol;
  apr_hash_t *keywords;
  svn_boolean_t special;
  const char *mime_value = svn_prop_get_value(mt->actual_props,
                                              SVN_PROP_MIME_TYPE);

  is_binary = (mime_value && svn_mime_type_is_binary(mime_value));

  /* See if we need to do a straight copy:
     - old and new mime-types are binary, or
     - old mime-type is binary and no new mime-type specified */
  if (is_binary
      && (((prop = get_prop(mt, SVN_PROP_MIME_TYPE))
           && prop->value && svn_mime_type_is_binary(prop->value->data))
          || prop == NULL))
    {
      /* this is case IV above */
      keywords = NULL;
      special = FALSE;
      eol = NULL;
      style = svn_subst_eol_style_none;
    }
  else if ((!is_binary)
           && (prop = get_prop(mt, SVN_PROP_MIME_TYPE))
           && prop->value && svn_mime_type_is_binary(prop->value->data))
    {
      /* Old props indicate texty, new props indicate binary:
         detranslate keywords and old eol-style */
      SVN_ERR(svn_wc__get_translate_info(&style, &eol,
                                         &keywords,
                                         &special,
                                         mt->db, mt->local_abspath,
                                         mt->actual_props, TRUE,
                                         scratch_pool, scratch_pool));
    }
  else
    {
      /* New props indicate texty, regardless of old props */

      /* In case the file used to be special, detranslate specially */
      SVN_ERR(svn_wc__get_translate_info(&style, &eol,
                                         &keywords,
                                         &special,
                                         mt->db, mt->local_abspath,
                                         mt->actual_props, TRUE,
                                         scratch_pool, scratch_pool));

      if (special)
        {
          keywords = NULL;
          eol = NULL;
          style = svn_subst_eol_style_none;
        }
      else
        {
          /* In case a new eol style was set, use that for detranslation */
          if ((prop = get_prop(mt, SVN_PROP_EOL_STYLE)) && prop->value)
            {
              /* Value added or changed */
              svn_subst_eol_style_from_value(&style, &eol, prop->value->data);
            }
          else if (!is_binary)
            {
              /* Already fetched */
            }
          else
            {
              eol = NULL;
              style = svn_subst_eol_style_none;
            }

          /* In case there were keywords, detranslate with keywords
             (iff we were texty) */
          if (is_binary)
            keywords = NULL;
        }
    }

  /* Now, detranslate with the settings we created above */

  if (force_copy || keywords || eol || special)
    {
      const char *wcroot_abspath, *temp_dir_abspath;
      const char *detranslated;

      /* Force a copy into the temporary wc area to avoid having
         temporary files created below to appear in the actual wc. */
      SVN_ERR(svn_wc__db_get_wcroot(&wcroot_abspath, mt->db, mt->wri_abspath,
                                    scratch_pool, scratch_pool));
      SVN_ERR(svn_wc__db_temp_wcroot_tempdir(&temp_dir_abspath, mt->db,
                                             mt->wri_abspath,
                                             scratch_pool, scratch_pool));

      /* ### svn_subst_copy_and_translate4() also creates a tempfile
         ### internally.  Anyway to piggyback on that? */
      SVN_ERR(svn_io_open_unique_file3(NULL, &detranslated, temp_dir_abspath,
                                       (force_copy
                                        ? svn_io_file_del_none
                                        : svn_io_file_del_on_pool_cleanup),
                                       result_pool, scratch_pool));

      /* Always 'repair' EOLs here, so that we can apply a diff that
         changes from inconsistent newlines and no 'svn:eol-style' to
         consistent newlines and 'svn:eol-style' set.  */

      if (style == svn_subst_eol_style_native)
        eol = SVN_SUBST_NATIVE_EOL_STR;
      else if (style != svn_subst_eol_style_fixed
               && style != svn_subst_eol_style_none)
        return svn_error_create(SVN_ERR_IO_UNKNOWN_EOL, NULL, NULL);

      SVN_ERR(svn_subst_copy_and_translate4(source_abspath,
                                            detranslated,
                                            eol,
                                            TRUE /* repair */,
                                            keywords,
                                            FALSE /* contract keywords */,
                                            special,
                                            cancel_func, cancel_baton,
                                            scratch_pool));

      SVN_ERR(svn_dirent_get_absolute(detranslated_abspath, detranslated,
                                      result_pool));
    }
  else
    *detranslated_abspath = apr_pstrdup(result_pool, source_abspath);

  return SVN_NO_ERROR;
}

/* Updates (by copying and translating) the eol style in
   OLD_TARGET returning the filename containing the
   correct eol style in NEW_TARGET, if an eol style
   change is contained in PROP_DIFF */
static svn_error_t *
maybe_update_target_eols(const char **new_target_abspath,
                         const merge_target_t *mt,
                         const char *old_target_abspath,
                         svn_cancel_func_t cancel_func,
                         void *cancel_baton,
                         apr_pool_t *result_pool,
                         apr_pool_t *scratch_pool)
{
  const svn_prop_t *prop = get_prop(mt, SVN_PROP_EOL_STYLE);

  if (prop && prop->value)
    {
      const char *eol;
      const char *tmp_new;

      svn_subst_eol_style_from_value(NULL, &eol, prop->value->data);
      SVN_ERR(svn_io_open_unique_file3(NULL, &tmp_new, NULL,
                                       svn_io_file_del_on_pool_cleanup,
                                       result_pool, scratch_pool));

      /* Always 'repair' EOLs here, so that we can apply a diff that
         changes from inconsistent newlines and no 'svn:eol-style' to
         consistent newlines and 'svn:eol-style' set.  */
      SVN_ERR(svn_subst_copy_and_translate4(old_target_abspath,
                                            tmp_new,
                                            eol,
                                            TRUE /* repair */,
                                            NULL /* keywords */,
                                            FALSE /* expand */,
                                            FALSE /* special */,
                                            cancel_func, cancel_baton,
                                            scratch_pool));
      *new_target_abspath = apr_pstrdup(result_pool, tmp_new);
    }
  else
    *new_target_abspath = apr_pstrdup(result_pool, old_target_abspath);

  return SVN_NO_ERROR;
}


/* Set *TARGET_MARKER, *LEFT_MARKER and *RIGHT_MARKER to strings suitable
   for delimiting the alternative texts in a text conflict.  Include in each
   marker a string that may be given by TARGET_LABEL, LEFT_LABEL and
   RIGHT_LABEL respectively or a default value where any of those are NULL.

   Allocate the results in POOL or statically. */
static void
init_conflict_markers(const char **target_marker,
                      const char **left_marker,
                      const char **right_marker,
                      const char *target_label,
                      const char *left_label,
                      const char *right_label,
                      apr_pool_t *pool)
{
  /* Labels fall back to sensible defaults if not specified. */
  if (target_label)
    *target_marker = apr_psprintf(pool, "<<<<<<< %s", target_label);
  else
    *target_marker = "<<<<<<< .working";

  if (left_label)
    *left_marker = apr_psprintf(pool, "||||||| %s", left_label);
  else
    *left_marker = "||||||| .old";

  if (right_label)
    *right_marker = apr_psprintf(pool, ">>>>>>> %s", right_label);
  else
    *right_marker = ">>>>>>> .new";
}

/* Do a 3-way merge of the files at paths LEFT, DETRANSLATED_TARGET,
 * and RIGHT, using diff options provided in OPTIONS.  Store the merge
 * result in the file RESULT_F.
 * If there are conflicts, set *CONTAINS_CONFLICTS to true, and use
 * TARGET_LABEL, LEFT_LABEL, and RIGHT_LABEL as labels for conflict
 * markers.  Else, set *CONTAINS_CONFLICTS to false.
 * Do all allocations in POOL. */
static svn_error_t*
do_text_merge(svn_boolean_t *contains_conflicts,
              apr_file_t *result_f,
              const merge_target_t *mt,
              const char *detranslated_target,
              const char *left,
              const char *right,
              const char *target_label,
              const char *left_label,
              const char *right_label,
              apr_pool_t *pool)
{
  svn_diff_t *diff;
  svn_stream_t *ostream;
  const char *target_marker;
  const char *left_marker;
  const char *right_marker;
  svn_diff_file_options_t *diff3_options;

  diff3_options = svn_diff_file_options_create(pool);

  if (mt->merge_options)
    SVN_ERR(svn_diff_file_options_parse(diff3_options,
                                        mt->merge_options, pool));


  init_conflict_markers(&target_marker, &left_marker, &right_marker,
                        target_label, left_label, right_label, pool);

  SVN_ERR(svn_diff_file_diff3_2(&diff, left, detranslated_target, right,
                                diff3_options, pool));

  ostream = svn_stream_from_aprfile2(result_f, TRUE, pool);

  SVN_ERR(svn_diff_file_output_merge2(ostream, diff,
                                      left, detranslated_target, right,
                                      left_marker,
                                      target_marker,
                                      right_marker,
                                      "=======", /* separator */
                                      svn_diff_conflict_display_modified_latest,
                                      pool));
  SVN_ERR(svn_stream_close(ostream));

  *contains_conflicts = svn_diff_contains_conflicts(diff);

  return SVN_NO_ERROR;
}

/* Same as do_text_merge() above, but use the external diff3
 * command DIFF3_CMD to perform the merge.  Pass MERGE_OPTIONS
 * to the diff3 command.  Do all allocations in POOL. */
static svn_error_t*
do_text_merge_external(svn_boolean_t *contains_conflicts,
                       apr_file_t *result_f,
                       const merge_target_t *mt,
                       const char *detranslated_target,
                       const char *left_abspath,
                       const char *right_abspath,
                       const char *target_label,
                       const char *left_label,
                       const char *right_label,
                       apr_pool_t *scratch_pool)
{
  int exit_code;

  SVN_ERR(svn_io_run_diff3_3(&exit_code, ".",
                             detranslated_target, left_abspath, right_abspath,
                             target_label, left_label, right_label,
                             result_f, mt->diff3_cmd,
                             mt->merge_options, scratch_pool));

  *contains_conflicts = exit_code == 1;

  return SVN_NO_ERROR;
}

/* Create a new file in the same directory as VERSIONED_ABSPATH, with the
   same basename as VERSIONED_ABSPATH, with a ".edited" extension, and set
   *WORK_ITEM to a new work item that will copy and translate from the file
   SOURCE to that new file.  It will be translated from repository-normal
   form to working-copy form according to the versioned properties of
   VERSIONED_ABSPATH that are current when the work item is executed.

   DB should have a write lock for the directory containing SOURCE.

   Allocate *WORK_ITEM in RESULT_POOL. */
static svn_error_t*
save_merge_result(svn_skel_t **work_item,
                  const merge_target_t *mt,
                  const char *source,
                  apr_pool_t *result_pool,
                  apr_pool_t *scratch_pool)
{
  const char *edited_copy_abspath;
  const char *dir_abspath;
  const char *filename;

  svn_dirent_split(&dir_abspath, &filename, mt->local_abspath, scratch_pool);

  /* ### Should use preserved-conflict-file-exts. */
  /* Create the .edited file within this file's DIR_ABSPATH  */
  SVN_ERR(svn_io_open_uniquely_named(NULL,
                                     &edited_copy_abspath,
                                     dir_abspath,
                                     filename,
                                     ".edited",
                                     svn_io_file_del_none,
                                     scratch_pool, scratch_pool));
  SVN_ERR(svn_wc__wq_build_file_copy_translated(work_item,
                                                mt->db, mt->local_abspath,
                                                source, edited_copy_abspath,
                                                result_pool, scratch_pool));

  return SVN_NO_ERROR;
}

/* Deal with the result of the conflict resolution callback, as indicated by
 * CHOICE.
 *
 * Set *WORK_ITEMS to new work items that will ...
 * Set *MERGE_OUTCOME to the result of the 3-way merge.
 *
 * LEFT_ABSPATH, RIGHT_ABSPATH, and TARGET_ABSPATH are the input files to
 * the 3-way merge, and MERGED_FILE is the merged result as generated by the
 * internal or external merge or by the conflict resolution callback.
 *
 * DETRANSLATED_TARGET is the detranslated version of TARGET_ABSPATH
 * (see detranslate_wc_file() above).  DIFF3_OPTIONS are passed to the
 * diff3 implementation in case a 3-way merge has to be carried out. */
static svn_error_t*
eval_conflict_func_result(svn_skel_t **work_items,
                          enum svn_wc_merge_outcome_t *merge_outcome,
                          svn_wc_conflict_choice_t choice,
                          const merge_target_t *mt,
                          const char *left_abspath,
                          const char *right_abspath,
                          const char *merged_file,
                          const char *detranslated_target,
                          apr_pool_t *result_pool,
                          apr_pool_t *scratch_pool)
{
  const char *install_from = NULL;
  svn_boolean_t remove_source = FALSE;

  *work_items = NULL;

  switch (choice)
    {
      /* If the callback wants to use one of the fulltexts
         to resolve the conflict, so be it.*/
      case svn_wc_conflict_choose_base:
        {
          install_from = left_abspath;
          *merge_outcome = svn_wc_merge_merged;
          break;
        }
      case svn_wc_conflict_choose_theirs_full:
        {
          install_from = right_abspath;
          *merge_outcome = svn_wc_merge_merged;
          break;
        }
      case svn_wc_conflict_choose_mine_full:
        {
          /* Do nothing to merge_target, let it live untouched! */
          *merge_outcome = svn_wc_merge_merged;
          return SVN_NO_ERROR;
        }
      case svn_wc_conflict_choose_theirs_conflict:
      case svn_wc_conflict_choose_mine_conflict:
        {
          const char *chosen_path;
          const char *temp_dir;
          svn_stream_t *chosen_stream;
          svn_diff_t *diff;
          svn_diff_conflict_display_style_t style;
          svn_diff_file_options_t *diff3_options;

          diff3_options = svn_diff_file_options_create(scratch_pool);

          if (mt->merge_options)
             SVN_ERR(svn_diff_file_options_parse(diff3_options,
                                                 mt->merge_options,
                                                 scratch_pool));

          style = choice == svn_wc_conflict_choose_theirs_conflict
                    ? svn_diff_conflict_display_latest
                    : svn_diff_conflict_display_modified;

          SVN_ERR(svn_wc__db_temp_wcroot_tempdir(&temp_dir, mt->db,
                                                 mt->wri_abspath,
                                                 scratch_pool, scratch_pool));
          SVN_ERR(svn_stream_open_unique(&chosen_stream, &chosen_path,
                                         temp_dir, svn_io_file_del_none,
                                         scratch_pool, scratch_pool));

          SVN_ERR(svn_diff_file_diff3_2(&diff,
                                        left_abspath,
                                        detranslated_target, right_abspath,
                                        diff3_options, scratch_pool));
          SVN_ERR(svn_diff_file_output_merge2(chosen_stream, diff,
                                              left_abspath,
                                              detranslated_target,
                                              right_abspath,
                                              /* markers ignored */
                                              NULL, NULL,
                                              NULL, NULL,
                                              style,
                                              scratch_pool));
          SVN_ERR(svn_stream_close(chosen_stream));

          install_from = chosen_path;
          remove_source = TRUE;
          *merge_outcome = svn_wc_merge_merged;
          break;
        }

        /* For the case of 3-way file merging, we don't
           really distinguish between these return values;
           if the callback claims to have "generally
           resolved" the situation, we still interpret
           that as "OK, we'll assume the merged version is
           good to use". */
      case svn_wc_conflict_choose_merged:
        {
          install_from = merged_file;
          *merge_outcome = svn_wc_merge_merged;
          break;
        }
      case svn_wc_conflict_choose_postpone:
      default:
        {
#if 0
          /* ### what should this value be? no caller appears to initialize
             ### it, so we really SHOULD be setting a value here.  */
          *merge_outcome = svn_wc_merge_merged;
#endif

          /* Assume conflict remains. */
          return SVN_NO_ERROR;
        }
    }

  SVN_ERR_ASSERT(install_from != NULL);

  {
    svn_skel_t *work_item;

    SVN_ERR(svn_wc__wq_build_file_install(&work_item,
                                          mt->db, mt->local_abspath,
                                          install_from,
                                          FALSE /* use_commit_times */,
                                          FALSE /* record_fileinfo */,
                                          result_pool, scratch_pool));
    *work_items = svn_wc__wq_merge(*work_items, work_item, result_pool);

    if (remove_source)
      {
        SVN_ERR(svn_wc__wq_build_file_remove(&work_item,
                                             mt->db, install_from,
                                             result_pool, scratch_pool));
        *work_items = svn_wc__wq_merge(*work_items, work_item, result_pool);
      }
  }

  return SVN_NO_ERROR;
}

/* Preserve the three pre-merge files.

   Create three empty files, with unique names that each include the
   basename of TARGET_ABSPATH and one of LEFT_LABEL, RIGHT_LABEL and
   TARGET_LABEL, in the directory that contains TARGET_ABSPATH.  Typical
   names are "foo.c.r37" or "foo.c.2.mine".  Set *LEFT_COPY, *RIGHT_COPY and
   *TARGET_COPY to their absolute paths.

   Set *WORK_ITEMS to a list of new work items that will write copies of
   LEFT_ABSPATH, RIGHT_ABSPATH and TARGET_ABSPATH into the three files,
   translated to working-copy form.

   The translation to working-copy form will be done according to the
   versioned properties of TARGET_ABSPATH that are current when the work
   queue items are executed.

   If target_abspath is not versioned use detranslated_target_abspath
   as the target file.
*/
static svn_error_t *
preserve_pre_merge_files(svn_skel_t **work_items,
                         const char **left_copy,
                         const char **right_copy,
                         const char **target_copy,
                         const merge_target_t *mt,
                         const char *left_abspath,
                         const char *right_abspath,
                         const char *left_label,
                         const char *right_label,
                         const char *target_label,
                         const char *detranslated_target_abspath,
                         svn_cancel_func_t cancel_func,
                         void *cancel_baton,
                         apr_pool_t *result_pool,
                         apr_pool_t *scratch_pool)
{
  const char *tmp_left, *tmp_right, *detranslated_target_copy;
  const char *dir_abspath, *target_name;
  const char *wcroot_abspath, *temp_dir_abspath;
  svn_skel_t *work_item, *last_items = NULL;

  *work_items = NULL;

  svn_dirent_split(&dir_abspath, &target_name, mt->local_abspath,
                   scratch_pool);

  SVN_ERR(svn_wc__db_get_wcroot(&wcroot_abspath, mt->db, mt->wri_abspath,
                                scratch_pool, scratch_pool));
  SVN_ERR(svn_wc__db_temp_wcroot_tempdir(&temp_dir_abspath, mt->db,
                                         mt->wri_abspath,
                                         scratch_pool, scratch_pool));

  /* Create three empty files in DIR_ABSPATH, naming them with unique names
     that each include TARGET_NAME and one of {LEFT,RIGHT,TARGET}_LABEL,
     and set *{LEFT,RIGHT,TARGET}_COPY to those names. */
  SVN_ERR(svn_io_open_uniquely_named(
            NULL, left_copy, dir_abspath, target_name, left_label,
            svn_io_file_del_none, result_pool, scratch_pool));
  SVN_ERR(svn_io_open_uniquely_named(
            NULL, right_copy, dir_abspath, target_name, right_label,
            svn_io_file_del_none, result_pool, scratch_pool));
  SVN_ERR(svn_io_open_uniquely_named(
            NULL, target_copy, dir_abspath, target_name, target_label,
            svn_io_file_del_none, result_pool, scratch_pool));

  /* We preserve all the files with keywords expanded and line
     endings in local (working) form. */

  /* The workingqueue requires its paths to be in the subtree
     relative to the wcroot path they are executed in.

     Make our LEFT and RIGHT files 'local' if they aren't... */
  if (! svn_dirent_is_ancestor(wcroot_abspath, left_abspath))
    {
      SVN_ERR(svn_io_open_unique_file3(NULL, &tmp_left, temp_dir_abspath,
                                       svn_io_file_del_none,
                                       scratch_pool, scratch_pool));
      SVN_ERR(svn_io_copy_file(left_abspath, tmp_left, TRUE, scratch_pool));

      /* And create a wq item to remove the file later */
      SVN_ERR(svn_wc__wq_build_file_remove(&work_item, mt->db, tmp_left,
                                           result_pool, scratch_pool));

      last_items = svn_wc__wq_merge(last_items, work_item, result_pool);
    }
  else
    tmp_left = left_abspath;

  if (! svn_dirent_is_ancestor(wcroot_abspath, right_abspath))
    {
      SVN_ERR(svn_io_open_unique_file3(NULL, &tmp_right, temp_dir_abspath,
                                       svn_io_file_del_none,
                                       scratch_pool, scratch_pool));
      SVN_ERR(svn_io_copy_file(right_abspath, tmp_right, TRUE, scratch_pool));

      /* And create a wq item to remove the file later */
      SVN_ERR(svn_wc__wq_build_file_remove(&work_item, mt->db, tmp_right,
                                           result_pool, scratch_pool));

      last_items = svn_wc__wq_merge(last_items, work_item, result_pool);
    }
  else
    tmp_right = right_abspath;

  /* NOTE: Callers must ensure that the svn:eol-style and
     svn:keywords property values are correct in the currently
     installed props.  With 'svn merge', it's no big deal.  But
     when 'svn up' calls this routine, it needs to make sure that
     this routine is using the newest property values that may
     have been received *during* the update.  Since this routine
     will be run from within a log-command, merge_file()
     needs to make sure that a previous log-command to 'install
     latest props' has already executed first.  Ben and I just
     checked, and that is indeed the order in which the log items
     are written, so everything should be fine.  Really.  */

  /* Create LEFT and RIGHT backup files, in expanded form.
     We use TARGET_ABSPATH's current properties to do the translation. */
  /* Derive the basenames of the 3 backup files. */
  SVN_ERR(svn_wc__wq_build_file_copy_translated(&work_item,
                                                mt->db, mt->local_abspath,
                                                tmp_left, *left_copy,
                                                result_pool, scratch_pool));
  *work_items = svn_wc__wq_merge(*work_items, work_item, result_pool);

  SVN_ERR(svn_wc__wq_build_file_copy_translated(&work_item,
                                                mt->db, mt->local_abspath,
                                                tmp_right, *right_copy,
                                                result_pool, scratch_pool));
  *work_items = svn_wc__wq_merge(*work_items, work_item, result_pool);

  /* Back up TARGET_ABSPATH through detranslation/retranslation:
     the new translation properties may not match the current ones */
  SVN_ERR(detranslate_wc_file(&detranslated_target_copy, mt, TRUE,
                              mt->local_abspath,
                              cancel_func, cancel_baton,
                              scratch_pool, scratch_pool));

  SVN_ERR(svn_wc__wq_build_file_copy_translated(&work_item,
                                                mt->db, mt->local_abspath,
                                                detranslated_target_copy,
                                                *target_copy,
                                                result_pool, scratch_pool));
  *work_items = svn_wc__wq_merge(*work_items, work_item, result_pool);

  /* And maybe delete some tempfiles */
  SVN_ERR(svn_wc__wq_build_file_remove(&work_item, mt->db,
                                       detranslated_target_copy,
                                       result_pool, scratch_pool));
  *work_items = svn_wc__wq_merge(*work_items, work_item, result_pool);

  *work_items = svn_wc__wq_merge(*work_items, last_items, result_pool);

  return SVN_NO_ERROR;
}

/* Helper for maybe_resolve_conflicts() below. */
static const svn_wc_conflict_description2_t *
setup_text_conflict_desc(const char *left_abspath,
                         const char *right_abspath,
                         const char *target_abspath,
                         const svn_wc_conflict_version_t *left_version,
                         const svn_wc_conflict_version_t *right_version,
                         const char *result_target,
                         const char *detranslated_target,
                         const svn_prop_t *mimeprop,
                         svn_boolean_t is_binary,
                         apr_pool_t *pool)
{
  svn_wc_conflict_description2_t *cdesc;

  cdesc = svn_wc_conflict_description_create_text2(target_abspath, pool);
  cdesc->is_binary = is_binary;
  cdesc->mime_type = (mimeprop && mimeprop->value)
                     ? mimeprop->value->data : NULL,
  cdesc->base_abspath = left_abspath;
  cdesc->their_abspath = right_abspath;
  cdesc->my_abspath = detranslated_target;
  cdesc->merged_file = result_target;

  cdesc->src_left_version = left_version;
  cdesc->src_right_version = right_version;

  return cdesc;
}

/* XXX Insane amount of parameters... */
/* RESULT_TARGET is the path to the merged file produced by the internal or
   external 3-way merge. */
static svn_error_t*
maybe_resolve_conflicts(svn_skel_t **work_items,
                        const merge_target_t *mt,
                        const char *left_abspath,
                        const char *right_abspath,
                        const char *left_label,
                        const char *right_label,
                        const char *target_label,
                        enum svn_wc_merge_outcome_t *merge_outcome,
                        const svn_wc_conflict_version_t *left_version,
                        const svn_wc_conflict_version_t *right_version,
                        const char *result_target,
                        const char *detranslated_target,
                        svn_wc_conflict_resolver_func2_t conflict_func,
                        void *conflict_baton,
                        svn_cancel_func_t cancel_func,
                        void *cancel_baton,
                        apr_pool_t *result_pool,
                        apr_pool_t *scratch_pool)
{
  svn_wc_conflict_result_t *result;
  svn_skel_t *work_item;

  *work_items = NULL;

  /* Give the conflict resolution callback a chance to clean
     up the conflicts before we mark the file 'conflicted' */
  if (!conflict_func)
    {
      /* If there is no interactive conflict resolution then we are effectively
         postponing conflict resolution. */
      result = svn_wc_create_conflict_result(svn_wc_conflict_choose_postpone,
                                             NULL, result_pool);
    }
  else
    {
      const svn_wc_conflict_description2_t *cdesc;

      cdesc = setup_text_conflict_desc(left_abspath,
                                       right_abspath,
                                       mt->local_abspath,
                                       left_version,
                                       right_version,
                                       result_target,
                                       detranslated_target,
                                       get_prop(mt, SVN_PROP_MIME_TYPE),
                                       FALSE,
                                       scratch_pool);

      SVN_ERR(conflict_func(&result, cdesc, conflict_baton, scratch_pool,
                            scratch_pool));
      if (result == NULL)
        return svn_error_create(SVN_ERR_WC_CONFLICT_RESOLVER_FAILURE,
                                NULL, _("Conflict callback violated API:"
                                        " returned no results"));

      if (result->save_merged)
        {
          SVN_ERR(save_merge_result(work_items,
                                    mt,
                                    /* Look for callback's own
                                       merged-file first: */
                                    result->merged_file
                                      ? result->merged_file
                                      : result_target,
                                    result_pool, scratch_pool));
        }
    }

  SVN_ERR(eval_conflict_func_result(&work_item,
                                    merge_outcome,
                                    result->choice,
                                    mt,
                                    left_abspath,
                                    right_abspath,
                                    result->merged_file
                                      ? result->merged_file
                                      : result_target,
                                    detranslated_target,
                                    result_pool, scratch_pool));
  *work_items = svn_wc__wq_merge(*work_items, work_item, result_pool);

  if (result->choice != svn_wc_conflict_choose_postpone)
    /* The conflicts have been dealt with, nothing else
     * to do for us here. */
    return SVN_NO_ERROR;

  /* The conflicts have not been dealt with. */
  *merge_outcome = svn_wc_merge_conflict;

  return SVN_NO_ERROR;
}


/* Attempt a trivial merge of LEFT_ABSPATH and RIGHT_ABSPATH to TARGET_ABSPATH.
 * The merge is trivial if the file at LEFT_ABSPATH equals TARGET_ABSPATH,
 * because in this case the content of RIGHT_ABSPATH can be copied to the
 * target. On success, set *MERGE_OUTCOME to SVN_WC_MERGE_MERGED in case the
 * target was changed, or to SVN_WC_MERGE_UNCHANGED if the target was not
 * changed. Install work queue items allocated in RESULT_POOL in *WORK_ITEMS.
 * On failure, set *MERGE_OUTCOME to SVN_WC_MERGE_NO_MERGE. */
static svn_error_t *
merge_file_trivial(svn_skel_t **work_items,
                   enum svn_wc_merge_outcome_t *merge_outcome,
                   const char *left_abspath,
                   const char *right_abspath,
                   const char *target_abspath,
                   svn_boolean_t dry_run,
                   svn_wc__db_t *db,
                   svn_cancel_func_t cancel_func,
                   void *cancel_baton,
                   apr_pool_t *result_pool,
                   apr_pool_t *scratch_pool)
{
  svn_skel_t *work_item;
  svn_boolean_t same_contents = FALSE;
  svn_node_kind_t kind;
  svn_boolean_t is_special;

  /* If the target is not a normal file, do not attempt a trivial merge. */
  SVN_ERR(svn_io_check_special_path(target_abspath, &kind, &is_special,
                                    scratch_pool));
  if (kind != svn_node_file || is_special)
    {
      *merge_outcome = svn_wc_merge_no_merge;
      return SVN_NO_ERROR;
    }

  /* If the LEFT side of the merge is equal to WORKING, then we can
   * copy RIGHT directly. */
  SVN_ERR(svn_io_files_contents_same_p(&same_contents, left_abspath,
                                       target_abspath, scratch_pool));
  if (same_contents)
    {
      /* Check whether the left side equals the right side.
       * If it does, there is no change to merge so we leave the target
       * unchanged. */
      SVN_ERR(svn_io_files_contents_same_p(&same_contents, left_abspath,
                                           right_abspath, scratch_pool));
      if (same_contents)
        {
          *merge_outcome = svn_wc_merge_unchanged;
        }
      else
        {
          *merge_outcome = svn_wc_merge_merged;
          if (!dry_run)
            {
              const char *wcroot_abspath;
              svn_boolean_t delete_src = FALSE;

              /* The right_abspath might be outside our working copy. In that
                 case we should copy the file to a safe location before
                 installing to avoid breaking the workqueue */

              SVN_ERR(svn_wc__db_get_wcroot(&wcroot_abspath,
                                            db, target_abspath,
                                            scratch_pool, scratch_pool));

              if (!svn_dirent_is_child(wcroot_abspath, right_abspath, NULL))
                {
                  svn_stream_t *tmp_src;
                  svn_stream_t *tmp_dst;

                  SVN_ERR(svn_stream_open_readonly(&tmp_src, right_abspath,
                                                   scratch_pool,
                                                   scratch_pool));

                  SVN_ERR(svn_wc__open_writable_base(&tmp_dst, &right_abspath,
                                                     NULL, NULL,
                                                     db, target_abspath,
                                                     scratch_pool,
                                                     scratch_pool));

                  SVN_ERR(svn_stream_copy3(tmp_src, tmp_dst,
                                           cancel_func, cancel_baton,
                                           scratch_pool));

                  /* no need to strdup right_abspath, as the wq_build_()
                     call already does that for us */
                  delete_src = TRUE;
                }

              SVN_ERR(svn_wc__wq_build_file_install(
                        &work_item, db, target_abspath, right_abspath,
                        FALSE /* use_commit_times */,
                        FALSE /* record_fileinfo */,
                        result_pool, scratch_pool));
              *work_items = svn_wc__wq_merge(*work_items, work_item,
                                             result_pool);

              if (delete_src)
                {
                  SVN_ERR(svn_wc__wq_build_file_remove(
                                    &work_item, db, right_abspath,
                                    result_pool, scratch_pool));
                  *work_items = svn_wc__wq_merge(*work_items, work_item,
                                                 result_pool);
                }
            }
        }

      return SVN_NO_ERROR;
    }

  *merge_outcome = svn_wc_merge_no_merge;
  return SVN_NO_ERROR;
}


/* XXX Insane amount of parameters... */
static svn_error_t*
merge_text_file(svn_skel_t **work_items,
                enum svn_wc_merge_outcome_t *merge_outcome,
                const merge_target_t *mt,
                const char *left_abspath,
                const char *right_abspath,
                const char *left_label,
                const char *right_label,
                const char *target_label,
                svn_boolean_t dry_run,
                const svn_wc_conflict_version_t *left_version,
                const svn_wc_conflict_version_t *right_version,
                const char *detranslated_target_abspath,
                svn_wc_conflict_resolver_func2_t conflict_func,
                void *conflict_baton,
                svn_cancel_func_t cancel_func,
                void *cancel_baton,
                apr_pool_t *result_pool,
                apr_pool_t *scratch_pool)
{
  apr_pool_t *pool = scratch_pool;  /* ### temporary rename  */
  svn_boolean_t contains_conflicts;
  apr_file_t *result_f;
  const char *result_target;
  const char *base_name;
  const char *temp_dir;
  svn_skel_t *work_item;

  *work_items = NULL;

  base_name = svn_dirent_basename(mt->local_abspath, scratch_pool);

  /* Open a second temporary file for writing; this is where diff3
     will write the merged results.  We want to use a tempfile
     with a name that reflects the original, in case this
     ultimately winds up in a conflict resolution editor.  */
  SVN_ERR(svn_wc__db_temp_wcroot_tempdir(&temp_dir, mt->db, mt->wri_abspath,
                                         pool, pool));
  SVN_ERR(svn_io_open_uniquely_named(&result_f, &result_target,
                                     temp_dir, base_name, ".tmp",
                                     svn_io_file_del_none, pool, pool));

  /* Run the external or internal merge, as requested. */
  if (mt->diff3_cmd)
      SVN_ERR(do_text_merge_external(&contains_conflicts,
                                     result_f,
                                     mt,
                                     detranslated_target_abspath,
                                     left_abspath,
                                     right_abspath,
                                     target_label,
                                     left_label,
                                     right_label,
                                     pool));
  else /* Use internal merge. */
    SVN_ERR(do_text_merge(&contains_conflicts,
                          result_f,
                          mt,
                          detranslated_target_abspath,
                          left_abspath,
                          right_abspath,
                          target_label,
                          left_label,
                          right_label,
                          pool));

  SVN_ERR(svn_io_file_close(result_f, pool));

  if (contains_conflicts && ! dry_run)
    {
      SVN_ERR(maybe_resolve_conflicts(work_items,
                                      mt,
                                      left_abspath,
                                      right_abspath,
                                      left_label,
                                      right_label,
                                      target_label,
                                      merge_outcome,
                                      left_version,
                                      right_version,
                                      result_target,
                                      detranslated_target_abspath,
                                      conflict_func, conflict_baton,
                                      cancel_func, cancel_baton,
                                      result_pool, scratch_pool));

      if (*merge_outcome == svn_wc_merge_conflict)
        {
          const char *left_copy, *right_copy, *target_copy;

          /* Preserve the three conflict files */
          SVN_ERR(preserve_pre_merge_files(
                    &work_item,
                    &left_copy, &right_copy, &target_copy,
                    mt, left_abspath, right_abspath,
                    left_label, right_label, target_label,
                    detranslated_target_abspath,
                    cancel_func, cancel_baton,
                    result_pool, scratch_pool));
          *work_items = svn_wc__wq_merge(*work_items, work_item, result_pool);

          /* Track the three conflict files in the metadata.
           * ### TODO WC-NG: Do this outside the work queue: see
           * svn_wc__wq_tmp_build_set_text_conflict_markers()'s doc string. */
          SVN_ERR(svn_wc__wq_tmp_build_set_text_conflict_markers(
                            &work_item, mt->db, mt->local_abspath,
                            left_copy, right_copy, target_copy,
                            result_pool, scratch_pool));
          *work_items = svn_wc__wq_merge(*work_items, work_item, result_pool);
        }

      if (*merge_outcome == svn_wc_merge_merged)
        goto done;
    }
  else if (contains_conflicts && dry_run)
      *merge_outcome = svn_wc_merge_conflict;
  else
    {
      svn_boolean_t same, special;

      /* If 'special', then use the detranslated form of the
         target file.  This is so we don't try to follow symlinks,
         but the same treatment is probably also appropriate for
         whatever special file types we may invent in the future. */
      SVN_ERR(svn_wc__get_translate_info(NULL, NULL, NULL,
                                         &special, mt->db, mt->local_abspath,
                                         mt->actual_props, TRUE,
                                         pool, pool));
      SVN_ERR(svn_io_files_contents_same_p(&same, result_target,
                                           (special ?
                                              detranslated_target_abspath :
                                              mt->local_abspath),
                                           pool));

      *merge_outcome = same ? svn_wc_merge_unchanged : svn_wc_merge_merged;
    }

  if (*merge_outcome != svn_wc_merge_unchanged && ! dry_run)
    {
      /* replace TARGET_ABSPATH with the new merged file, expanding. */
      SVN_ERR(svn_wc__wq_build_file_install(&work_item,
                                            mt->db, mt->local_abspath,
                                            result_target,
                                            FALSE /* use_commit_times */,
                                            FALSE /* record_fileinfo */,
                                            result_pool, scratch_pool));
      *work_items = svn_wc__wq_merge(*work_items, work_item, result_pool);
    }

done:
  /* Remove the tempfile after use */
  SVN_ERR(svn_wc__wq_build_file_remove(&work_item,
                                       mt->db, result_target,
                                       result_pool, scratch_pool));

  *work_items = svn_wc__wq_merge(*work_items, work_item, result_pool);

  return SVN_NO_ERROR;
}


/* XXX Insane amount of parameters... */
static svn_error_t *
merge_binary_file(svn_skel_t **work_items,
                  enum svn_wc_merge_outcome_t *merge_outcome,
                  const merge_target_t *mt,
                  const char *left_abspath,
                  const char *right_abspath,
                  const char *left_label,
                  const char *right_label,
                  const char *target_label,
                  svn_boolean_t dry_run,
                  const svn_wc_conflict_version_t *left_version,
                  const svn_wc_conflict_version_t *right_version,
                  const char *detranslated_target_abspath,
                  svn_wc_conflict_resolver_func2_t conflict_func,
                  void *conflict_baton,
                  apr_pool_t *result_pool,
                  apr_pool_t *scratch_pool)
{
  apr_pool_t *pool = scratch_pool;  /* ### temporary rename  */
  /* ### when making the binary-file backups, should we be honoring
     keywords and eol stuff?   */
  const char *left_copy, *right_copy;
  const char *merge_dirpath, *merge_filename;
  const char *conflict_wrk;
  svn_skel_t *work_item;

  *work_items = NULL;

  svn_dirent_split(&merge_dirpath, &merge_filename, mt->local_abspath, pool);

  /* If we get here the binary files differ. Because we don't know how
   * to merge binary files in a non-trivial way we always flag a conflict. */

  if (dry_run)
    {
      *merge_outcome = svn_wc_merge_conflict;
      return SVN_NO_ERROR;
    }

  /* Give the conflict resolution callback a chance to clean
     up the conflict before we mark the file 'conflicted' */
  if (conflict_func)
    {
      svn_wc_conflict_result_t *result = NULL;
      const svn_wc_conflict_description2_t *cdesc;
      const char *install_from = NULL;

      cdesc = setup_text_conflict_desc(left_abspath, right_abspath,
                                       mt->local_abspath,
                                       left_version, right_version,
                                       NULL /* result_target */,
                                       detranslated_target_abspath,
                                       get_prop(mt, SVN_PROP_MIME_TYPE),
                                       TRUE, pool);

      SVN_ERR(conflict_func(&result, cdesc, conflict_baton, pool, pool));
      if (result == NULL)
        return svn_error_create(SVN_ERR_WC_CONFLICT_RESOLVER_FAILURE,
                                NULL, _("Conflict callback violated API:"
                                        " returned no results"));

      switch (result->choice)
        {
          /* For a binary file, there's no merged file to look at,
             unless the conflict-callback did the merging itself. */
          case svn_wc_conflict_choose_base:
            {
              install_from = left_abspath;
              *merge_outcome = svn_wc_merge_merged;
              break;
            }
          case svn_wc_conflict_choose_theirs_full:
            {
              install_from = right_abspath;
              *merge_outcome = svn_wc_merge_merged;
              break;
            }
            /* For a binary file, if the response is to use the
               user's file, we do nothing.  We also do nothing if
               the response claims to have already resolved the
               problem.*/
          case svn_wc_conflict_choose_mine_full:
            {
              *merge_outcome = svn_wc_merge_merged;
              return SVN_NO_ERROR;
            }
          case svn_wc_conflict_choose_merged:
            {
              if (! result->merged_file)
                {
                  /* Callback asked us to choose its own
                     merged file, but didn't provide one! */
                  return svn_error_create
                      (SVN_ERR_WC_CONFLICT_RESOLVER_FAILURE,
                       NULL, _("Conflict callback violated API:"
                               " returned no merged file"));
                }
              else
                {
                  install_from = result->merged_file;
                  *merge_outcome = svn_wc_merge_merged;
                  break;
                }
            }
          case svn_wc_conflict_choose_postpone:
          default:
            {
              /* Assume conflict remains, fall through to code below. */
            }
        }

      if (install_from != NULL)
        {
          SVN_ERR(svn_wc__wq_build_file_install(work_items,
                                                mt->db, mt->local_abspath,
                                                install_from,
                                                FALSE /* use_commit_times */,
                                                FALSE /* record_fileinfo */,
                                                result_pool, scratch_pool));

          /* A merge choice was made, so we're done here.  */
          return SVN_NO_ERROR;
        }
    }

  /* reserve names for backups of left and right fulltexts */
  SVN_ERR(svn_io_open_uniquely_named(NULL,
                                     &left_copy,
                                     merge_dirpath,
                                     merge_filename,
                                     left_label,
                                     svn_io_file_del_none,
                                     pool, pool));

  SVN_ERR(svn_io_open_uniquely_named(NULL,
                                     &right_copy,
                                     merge_dirpath,
                                     merge_filename,
                                     right_label,
                                     svn_io_file_del_none,
                                     pool, pool));

  /* create the backup files */
  SVN_ERR(svn_io_copy_file(left_abspath, left_copy, TRUE, pool));
  SVN_ERR(svn_io_copy_file(right_abspath, right_copy, TRUE, pool));

  /* Was the merge target detranslated? */
  if (strcmp(mt->local_abspath, detranslated_target_abspath) != 0)
    {
      /* Create a .mine file too */
      SVN_ERR(svn_io_open_uniquely_named(NULL,
                                         &conflict_wrk,
                                         merge_dirpath,
                                         merge_filename,
                                         target_label,
                                         svn_io_file_del_none,
                                         pool, pool));
      SVN_ERR(svn_wc__wq_build_file_move(work_items, mt->db,
                                         mt->local_abspath,
                                         detranslated_target_abspath,
                                         conflict_wrk,
                                         pool, result_pool));
    }
  else
    {
      conflict_wrk = NULL;
    }

  /* Mark target_abspath's entry as "Conflicted", and start tracking
     the backup files in the entry as well. */
  SVN_ERR(svn_wc__wq_tmp_build_set_text_conflict_markers(&work_item,
                                                         mt->db,
                                                         mt->local_abspath,
                                                         left_copy,
                                                         right_copy,
                                                         conflict_wrk,
                                                         result_pool,
                                                         scratch_pool));

  *work_items = svn_wc__wq_merge(*work_items, work_item, result_pool);

  *merge_outcome = svn_wc_merge_conflict; /* a conflict happened */

  return SVN_NO_ERROR;
}

/* XXX Insane amount of parameters... */
svn_error_t *
svn_wc__internal_merge(svn_skel_t **work_items,
                       enum svn_wc_merge_outcome_t *merge_outcome,
                       svn_wc__db_t *db,
                       const char *left_abspath,
                       const svn_wc_conflict_version_t *left_version,
                       const char *right_abspath,
                       const svn_wc_conflict_version_t *right_version,
                       const char *target_abspath,
                       const char *wri_abspath,
                       const char *left_label,
                       const char *right_label,
                       const char *target_label,
                       apr_hash_t *actual_props,
                       svn_boolean_t dry_run,
                       const char *diff3_cmd,
                       const apr_array_header_t *merge_options,
                       const apr_array_header_t *prop_diff,
                       svn_wc_conflict_resolver_func2_t conflict_func,
                       void *conflict_baton,
                       svn_cancel_func_t cancel_func,
                       void *cancel_baton,
                       apr_pool_t *result_pool,
                       apr_pool_t *scratch_pool)
{
  apr_pool_t *pool = scratch_pool;  /* ### temporary rename  */
  const char *detranslated_target_abspath;
  svn_boolean_t is_binary = FALSE;
  const svn_prop_t *mimeprop;
  svn_skel_t *work_item;
  merge_target_t mt;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(left_abspath));
  SVN_ERR_ASSERT(svn_dirent_is_absolute(right_abspath));
  SVN_ERR_ASSERT(svn_dirent_is_absolute(target_abspath));

  *work_items = NULL;

  /* Fill the merge target baton */
  mt.db = db;
  mt.local_abspath = target_abspath;
  mt.wri_abspath = wri_abspath;
  mt.actual_props = actual_props;
  mt.prop_diff = prop_diff;
  mt.diff3_cmd = diff3_cmd;
  mt.merge_options = merge_options;

  /* Decide if the merge target is a text or binary file. */
  if ((mimeprop = get_prop(&mt, SVN_PROP_MIME_TYPE))
      && mimeprop->value)
    is_binary = svn_mime_type_is_binary(mimeprop->value->data);
  else
    {
      const char *value = svn_prop_get_value(mt.actual_props,
                                             SVN_PROP_MIME_TYPE);

      is_binary = value && svn_mime_type_is_binary(value);
    }

  SVN_ERR(detranslate_wc_file(&detranslated_target_abspath, &mt,
                              (! is_binary) && diff3_cmd != NULL,
                              target_abspath,
                              cancel_func, cancel_baton, pool, pool));

  /* We cannot depend on the left file to contain the same eols as the
     right file. If the merge target has mods, this will mark the entire
     file as conflicted, so we need to compensate. */
  SVN_ERR(maybe_update_target_eols(&left_abspath, &mt, left_abspath,
                                   cancel_func, cancel_baton, pool, pool));

  SVN_ERR(merge_file_trivial(work_items, merge_outcome,
                             left_abspath, right_abspath,
                             target_abspath, dry_run, db,
                             cancel_func, cancel_baton,
                             result_pool, scratch_pool));
  if (*merge_outcome == svn_wc_merge_no_merge)
    {
      if (is_binary)
        {
          SVN_ERR(merge_binary_file(work_items,
                                    merge_outcome,
                                    &mt,
                                    left_abspath,
                                    right_abspath,
                                    left_label,
                                    right_label,
                                    target_label,
                                    dry_run,
                                    left_version,
                                    right_version,
                                    detranslated_target_abspath,
                                    conflict_func,
                                    conflict_baton,
                                    result_pool, scratch_pool));
        }
      else
        {
          SVN_ERR(merge_text_file(work_items,
                                  merge_outcome,
                                  &mt,
                                  left_abspath,
                                  right_abspath,
                                  left_label,
                                  right_label,
                                  target_label,
                                  dry_run,
                                  left_version,
                                  right_version,
                                  detranslated_target_abspath,
                                  conflict_func, conflict_baton,
                                  cancel_func, cancel_baton,
                                  result_pool, scratch_pool));
        }
    }

  /* Merging is complete.  Regardless of text or binariness, we might
     need to tweak the executable bit on the new working file, and
     possibly make it read-only. */
  if (! dry_run)
    {
      SVN_ERR(svn_wc__wq_build_sync_file_flags(&work_item, db,
                                               target_abspath,
                                               result_pool, scratch_pool));
      *work_items = svn_wc__wq_merge(*work_items, work_item, result_pool);
    }

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc_merge4(enum svn_wc_merge_outcome_t *merge_outcome,
              svn_wc_context_t *wc_ctx,
              const char *left_abspath,
              const char *right_abspath,
              const char *target_abspath,
              const char *left_label,
              const char *right_label,
              const char *target_label,
              const svn_wc_conflict_version_t *left_version,
              const svn_wc_conflict_version_t *right_version,
              svn_boolean_t dry_run,
              const char *diff3_cmd,
              const apr_array_header_t *merge_options,
              const apr_array_header_t *prop_diff,
              svn_wc_conflict_resolver_func2_t conflict_func,
              void *conflict_baton,
              svn_cancel_func_t cancel_func,
              void *cancel_baton,
              apr_pool_t *scratch_pool)
{
  const char *dir_abspath = svn_dirent_dirname(target_abspath, scratch_pool);
  svn_skel_t *work_items;
  apr_hash_t *actual_props;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(left_abspath));
  SVN_ERR_ASSERT(svn_dirent_is_absolute(right_abspath));
  SVN_ERR_ASSERT(svn_dirent_is_absolute(target_abspath));

  /* Before we do any work, make sure we hold a write lock.  */
  if (!dry_run)
    SVN_ERR(svn_wc__write_check(wc_ctx->db, dir_abspath, scratch_pool));

  /* Sanity check:  the merge target must be under revision control,
   * unless the merge target is a copyfrom text, which lives in a
   * temporary file and does not exist in ACTUAL yet. */
  {
    svn_wc__db_kind_t kind;
    svn_boolean_t hidden;
    SVN_ERR(svn_wc__db_read_kind(&kind, wc_ctx->db, target_abspath, TRUE,
                                 scratch_pool));

    if (kind == svn_wc__db_kind_unknown)
      {
        *merge_outcome = svn_wc_merge_no_merge;
        return SVN_NO_ERROR;
      }

    SVN_ERR(svn_wc__db_node_hidden(&hidden, wc_ctx->db, target_abspath,
                                   scratch_pool));

    if (hidden)
      {
        *merge_outcome = svn_wc_merge_no_merge;
        return SVN_NO_ERROR;
      }
  }

  SVN_ERR(svn_wc__db_read_props(&actual_props, wc_ctx->db, target_abspath,
                                scratch_pool, scratch_pool));

  /* Queue all the work.  */
  SVN_ERR(svn_wc__internal_merge(&work_items,
                                 merge_outcome,
                                 wc_ctx->db,
                                 left_abspath, left_version,
                                 right_abspath, right_version,
                                 target_abspath,
                                 target_abspath,
                                 left_label, right_label, target_label,
                                 actual_props,
                                 dry_run,
                                 diff3_cmd,
                                 merge_options,
                                 prop_diff,
                                 conflict_func, conflict_baton,
                                 cancel_func, cancel_baton,
                                 scratch_pool, scratch_pool));

  /* If this isn't a dry run, then run the work!  */
  if (!dry_run)
    {
      SVN_ERR(svn_wc__db_wq_add(wc_ctx->db, target_abspath, work_items,
                                scratch_pool));
      SVN_ERR(svn_wc__wq_run(wc_ctx->db, target_abspath,
                             cancel_func, cancel_baton,
                             scratch_pool));
    }

  return SVN_NO_ERROR;
}


/* Constructor for the result-structure returned by conflict callbacks. */
svn_wc_conflict_result_t *
svn_wc_create_conflict_result(svn_wc_conflict_choice_t choice,
                              const char *merged_file,
                              apr_pool_t *pool)
{
  svn_wc_conflict_result_t *result = apr_pcalloc(pool, sizeof(*result));
  result->choice = choice;
  result->merged_file = merged_file;
  result->save_merged = FALSE;

  /* If we add more fields to svn_wc_conflict_result_t, add them here. */

  return result;
}

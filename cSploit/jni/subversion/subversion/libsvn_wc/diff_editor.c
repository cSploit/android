/*
 * diff_editor.c -- The diff editor for comparing the working copy against the
 *                  repository.
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

/*
 * This code uses an svn_delta_editor_t editor driven by
 * svn_wc_crawl_revisions (like the update command) to retrieve the
 * differences between the working copy and the requested repository
 * version. Rather than updating the working copy, this new editor creates
 * temporary files that contain the pristine repository versions. When the
 * crawler closes the files the editor calls back to a client layer
 * function to compare the working copy and the temporary file. There is
 * only ever one temporary file in existence at any time.
 *
 * When the crawler closes a directory, the editor then calls back to the
 * client layer to compare any remaining files that may have been modified
 * locally. Added directories do not have corresponding temporary
 * directories created, as they are not needed.
 *
 * The diff result from this editor is a combination of the restructuring
 * operations from the repository with the local restructurings since checking
 * out.
 *
 * ### TODO: Make sure that we properly support and report multi layered
 *           operations instead of only simple file replacements.
 *
 * ### TODO: Replacements where the node kind changes needs support. It
 * mostly works when the change is in the repository, but not when it is
 * in the working copy.
 *
 * ### TODO: Do we need to support copyfrom?
 *
 */

#include <apr_hash.h>
#include <apr_md5.h>

#include "svn_error.h"
#include "svn_pools.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_hash.h"

#include "private/svn_wc_private.h"

#include "wc.h"
#include "props.h"
#include "adm_files.h"
#include "translate.h"

#include "svn_private_config.h"


/*-------------------------------------------------------------------------*/
/* A little helper function.

   You see, when we ask the server to update us to a certain revision,
   we construct the new fulltext, and then run

         'diff <repos_fulltext> <working_fulltext>'

   which is, of course, actually backwards from the repository's point
   of view.  It thinks we want to move from working->repos.

   So when the server sends property changes, they're effectively
   backwards from what we want.  We don't want working->repos, but
   repos->working.  So this little helper "reverses" the value in
   BASEPROPS and PROPCHANGES before we pass them off to the
   prop_changed() diff-callback.  */
static void
reverse_propchanges(apr_hash_t *baseprops,
                    apr_array_header_t *propchanges,
                    apr_pool_t *pool)
{
  int i;

  /* ### todo: research lifetimes for property values below */

  for (i = 0; i < propchanges->nelts; i++)
    {
      svn_prop_t *propchange
        = &APR_ARRAY_IDX(propchanges, i, svn_prop_t);

      const svn_string_t *original_value =
        apr_hash_get(baseprops, propchange->name, APR_HASH_KEY_STRING);

      if ((original_value == NULL) && (propchange->value != NULL))
        {
          /* found an addition.  make it look like a deletion. */
          apr_hash_set(baseprops, propchange->name, APR_HASH_KEY_STRING,
                       svn_string_dup(propchange->value, pool));
          propchange->value = NULL;
        }

      else if ((original_value != NULL) && (propchange->value == NULL))
        {
          /* found a deletion.  make it look like an addition. */
          propchange->value = svn_string_dup(original_value, pool);
          apr_hash_set(baseprops, propchange->name, APR_HASH_KEY_STRING,
                       NULL);
        }

      else if ((original_value != NULL) && (propchange->value != NULL))
        {
          /* found a change.  just swap the values.  */
          const svn_string_t *str = svn_string_dup(propchange->value, pool);
          propchange->value = svn_string_dup(original_value, pool);
          apr_hash_set(baseprops, propchange->name, APR_HASH_KEY_STRING, str);
        }
    }
}


/* Set *RESULT_ABSPATH to the absolute path to a readable file containing
   the pristine text of LOCAL_ABSPATH in DB, or to NULL if it does not have
   any pristine text.

   If USE_BASE is FALSE it gets the pristine text of what is currently in the
   working copy. (So it returns the pristine file of a copy).

   If USE_BASE is TRUE, it looks in the lowest layer of the working copy and
   shows exactly what was originally checked out (or updated to).

   Rationale:

   Which text-base do we want to use for the diff?  If the node is replaced
   by a new file, then the base of the replaced file is called (in WC-1) the
   "revert base".  If the replacement is a copy or move, then there is also
   the base of the copied file to consider.

   One could argue that we should never diff against the revert
   base, and instead diff against the empty-file for both types of
   replacement.  After all, there is no ancestry relationship
   between the working file and the base file.  But my guess is that
   in practice, users want to see the diff between their working
   file and "the nearest versioned thing", whatever that is.  I'm
   not 100% sure this is the right decision, but it at least seems
   to match our test suite's expectations. */
static svn_error_t *
get_pristine_file(const char **result_abspath,
                  svn_wc__db_t *db,
                  const char *local_abspath,
                  svn_boolean_t use_base,
                  apr_pool_t *result_pool,
                  apr_pool_t *scratch_pool)
{
  const svn_checksum_t *checksum;

  if (!use_base)
    {
      SVN_ERR(svn_wc__db_read_pristine_info(NULL, NULL, NULL, NULL, NULL, NULL,
                                            &checksum, NULL, NULL,
                                            db, local_abspath,
                                            scratch_pool, scratch_pool));
    }
  else
    {
      SVN_ERR(svn_wc__db_base_get_info(NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL, NULL, NULL, &checksum,
                                       NULL, NULL, NULL, NULL,
                                       db, local_abspath,
                                       scratch_pool, scratch_pool));
    }

  if (checksum != NULL)
    {
      SVN_ERR(svn_wc__db_pristine_get_path(result_abspath, db, local_abspath,
                                           checksum,
                                           result_pool, scratch_pool));
      return SVN_NO_ERROR;
    }

  *result_abspath = NULL;
  return SVN_NO_ERROR;
}


/*-------------------------------------------------------------------------*/


/* Overall crawler editor baton.
 */
struct edit_baton {
  /* A wc db. */
  svn_wc__db_t *db;

  /* ANCHOR/TARGET represent the base of the hierarchy to be compared. */
  const char *target;

  /* The absolute path of the anchor directory */
  const char *anchor_abspath;

  /* Target revision */
  svn_revnum_t revnum;

  /* Was the root opened? */
  svn_boolean_t root_opened;

  /* The callbacks and callback argument that implement the file comparison
     functions */
  const svn_wc_diff_callbacks4_t *callbacks;
  void *callback_baton;

  /* How does this diff descend? */
  svn_depth_t depth;

  /* Should this diff ignore node ancestry? */
  svn_boolean_t ignore_ancestry;

  /* Should this diff not compare copied files with their source? */
  svn_boolean_t show_copies_as_adds;

  /* Are we producing a git-style diff? */
  svn_boolean_t use_git_diff_format;

  /* Possibly diff repos against text-bases instead of working files. */
  svn_boolean_t use_text_base;

  /* Possibly show the diffs backwards. */
  svn_boolean_t reverse_order;

  /* Empty file used to diff adds / deletes */
  const char *empty_file;

  /* Hash whose keys are const char * changelist names. */
  apr_hash_t *changelist_hash;

  /* Cancel function/baton */
  svn_cancel_func_t cancel_func;
  void *cancel_baton;

  apr_pool_t *pool;
};

/* Directory level baton.
 */
struct dir_baton {
  /* Gets set if the directory is added rather than replaced/unchanged. */
  svn_boolean_t added;

  /* The depth at which this directory should be diffed. */
  svn_depth_t depth;

  /* The name and path of this directory as if they would be/are in the
      local working copy. */
  const char *name;
  const char *local_abspath;

  /* The "correct" path of the directory, but it may not exist in the
     working copy. */
  const char *path;

  /* Identifies those directory elements that get compared while running
     the crawler.  These elements should not be compared again when
     recursively looking for local modifications.

     This hash maps the full path of the entry to an unimportant value
     (presence in the hash is the important factor here, not the value
     itself).

     If the directory's properties have been compared, an item with hash
     key of path will be present in the hash. */
  apr_hash_t *compared;

  /* The list of incoming BASE->repos propchanges. */
  apr_array_header_t *propchanges;

  /* The overall crawler editor baton. */
  struct edit_baton *eb;

  apr_pool_t *pool;
};

/* File level baton.
 */
struct file_baton {
  /* Gets set if the file is added rather than replaced. */
  svn_boolean_t added;

  /* The name and path of this file as if they would be/are in the
      local working copy. */
  const char *name;
  const char *local_abspath;

  /* PATH is the "correct" path of the file, but it may not exist in the
     working copy */
  const char *path;

 /* When constructing the requested repository version of the file, we
    drop the result into a file at TEMP_FILE_PATH. */
  const char *temp_file_path;

  /* The list of incoming BASE->repos propchanges. */
  apr_array_header_t *propchanges;

  /* The current checksum on disk */
  const svn_checksum_t *base_checksum;

  /* The resulting checksum from apply_textdelta */
  svn_checksum_t *result_checksum;

  /* The overall crawler editor baton. */
  struct edit_baton *eb;

  apr_pool_t *pool;
};

/* Create a new edit baton. TARGET_PATH/ANCHOR are working copy paths
 * that describe the root of the comparison. CALLBACKS/CALLBACK_BATON
 * define the callbacks to compare files. DEPTH defines if and how to
 * descend into subdirectories; see public doc string for exactly how.
 * IGNORE_ANCESTRY defines whether to utilize node ancestry when
 * calculating diffs.  USE_TEXT_BASE defines whether to compare
 * against working files or text-bases.  REVERSE_ORDER defines which
 * direction to perform the diff.
 *
 * CHANGELIST_FILTER is a list of const char * changelist names, used to
 * filter diff output responses to only those items in one of the
 * specified changelists, empty (or NULL altogether) if no changelist
 * filtering is requested.
 */
static svn_error_t *
make_edit_baton(struct edit_baton **edit_baton,
                svn_wc__db_t *db,
                const char *anchor_abspath,
                const char *target,
                const svn_wc_diff_callbacks4_t *callbacks,
                void *callback_baton,
                svn_depth_t depth,
                svn_boolean_t ignore_ancestry,
                svn_boolean_t show_copies_as_adds,
                svn_boolean_t use_git_diff_format,
                svn_boolean_t use_text_base,
                svn_boolean_t reverse_order,
                const apr_array_header_t *changelist_filter,
                svn_cancel_func_t cancel_func,
                void *cancel_baton,
                apr_pool_t *pool)
{
  apr_hash_t *changelist_hash = NULL;
  struct edit_baton *eb;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(anchor_abspath));

  if (changelist_filter && changelist_filter->nelts)
    SVN_ERR(svn_hash_from_cstring_keys(&changelist_hash, changelist_filter,
                                       pool));

  eb = apr_pcalloc(pool, sizeof(*eb));
  eb->db = db;
  eb->anchor_abspath = apr_pstrdup(pool, anchor_abspath);
  eb->target = apr_pstrdup(pool, target);
  eb->callbacks = callbacks;
  eb->callback_baton = callback_baton;
  eb->depth = depth;
  eb->ignore_ancestry = ignore_ancestry;
  eb->show_copies_as_adds = show_copies_as_adds;
  eb->use_git_diff_format = use_git_diff_format;
  eb->use_text_base = use_text_base;
  eb->reverse_order = reverse_order;
  eb->changelist_hash = changelist_hash;
  eb->cancel_func = cancel_func;
  eb->cancel_baton = cancel_baton;
  eb->pool = pool;

  *edit_baton = eb;
  return SVN_NO_ERROR;
}

/* Create a new directory baton.  PATH is the directory path,
 * including anchor_path.  ADDED is set if this directory is being
 * added rather than replaced.  PARENT_BATON is the baton of the
 * parent directory, it will be null if this is the root of the
 * comparison hierarchy.  The directory and its parent may or may not
 * exist in the working copy.  EDIT_BATON is the overall crawler
 * editor baton.
 */
static struct dir_baton *
make_dir_baton(const char *path,
               struct dir_baton *parent_baton,
               struct edit_baton *eb,
               svn_boolean_t added,
               svn_depth_t depth,
               apr_pool_t *result_pool)
{
  apr_pool_t *dir_pool = svn_pool_create(result_pool);
  struct dir_baton *db = apr_pcalloc(dir_pool, sizeof(*db));

  db->eb = eb;
  db->added = added;
  db->depth = depth;
  db->pool = dir_pool;
  db->propchanges = apr_array_make(dir_pool, 1, sizeof(svn_prop_t));
  db->compared = apr_hash_make(dir_pool);
  db->path = apr_pstrdup(dir_pool, path);

  db->name = svn_dirent_basename(db->path, NULL);

  if (parent_baton != NULL)
    db->local_abspath = svn_dirent_join(parent_baton->local_abspath, db->name,
                                        dir_pool);
  else
    db->local_abspath = apr_pstrdup(dir_pool, eb->anchor_abspath);

  return db;
}

/* Create a new file baton.  PATH is the file path, including
 * anchor_path.  ADDED is set if this file is being added rather than
 * replaced.  PARENT_BATON is the baton of the parent directory.
 * The directory and its parent may or may not exist in the working copy.
 */
static struct file_baton *
make_file_baton(const char *path,
                svn_boolean_t added,
                struct dir_baton *parent_baton,
                apr_pool_t *result_pool)
{
  apr_pool_t *file_pool = svn_pool_create(result_pool);
  struct file_baton *fb = apr_pcalloc(file_pool, sizeof(*fb));
  struct edit_baton *eb = parent_baton->eb;

  fb->eb = eb;
  fb->added = added;
  fb->pool = file_pool;
  fb->propchanges  = apr_array_make(file_pool, 1, sizeof(svn_prop_t));
  fb->path = apr_pstrdup(file_pool, path);

  fb->name = svn_dirent_basename(fb->path, NULL);
  fb->local_abspath = svn_dirent_join(parent_baton->local_abspath, fb->name,
                                      file_pool);

  return fb;
}

/* Get the empty file associated with the edit baton. This is cached so
 * that it can be reused, all empty files are the same.
 */
static svn_error_t *
get_empty_file(struct edit_baton *b,
               const char **empty_file)
{
  /* Create the file if it does not exist */
  /* Note that we tried to use /dev/null in r857294, but
     that won't work on Windows: it's impossible to stat NUL */
  if (!b->empty_file)
    {
      SVN_ERR(svn_io_open_unique_file3(NULL, &b->empty_file, NULL,
                                       svn_io_file_del_on_pool_cleanup,
                                       b->pool, b->pool));
    }

  *empty_file = b->empty_file;

  return SVN_NO_ERROR;
}


/* Return the value of the svn:mime-type property held in PROPS, or NULL
   if no such property exists. */
static const char *
get_prop_mimetype(apr_hash_t *props)
{
  return svn_prop_get_value(props, SVN_PROP_MIME_TYPE);
}


/* Return the property hash resulting from combining PROPS and PROPCHANGES.
 *
 * A note on pool usage: The returned hash and hash keys are allocated in
 * the same pool as PROPS, but the hash values will be taken directly from
 * either PROPS or PROPCHANGES, as appropriate.  Caller must therefore
 * ensure that the returned hash is only used for as long as PROPS and
 * PROPCHANGES remain valid.
 */
static apr_hash_t *
apply_propchanges(apr_hash_t *props,
                  const apr_array_header_t *propchanges)
{
  apr_hash_t *newprops = apr_hash_copy(apr_hash_pool_get(props), props);
  int i;

  for (i = 0; i < propchanges->nelts; ++i)
    {
      const svn_prop_t *prop = &APR_ARRAY_IDX(propchanges, i, svn_prop_t);
      apr_hash_set(newprops, prop->name, APR_HASH_KEY_STRING, prop->value);
    }

  return newprops;
}


/* Diff the file PATH against its text base.  At this
 * stage we are dealing with a file that does exist in the working copy.
 *
 * DIR_BATON is the parent directory baton, PATH is the path to the file to
 * be compared.
 *
 * Do all allocation in POOL.
 *
 * ### TODO: Need to work on replace if the new filename used to be a
 * directory.
 */
static svn_error_t *
file_diff(struct edit_baton *eb,
          const char *local_abspath,
          const char *path,
          apr_pool_t *scratch_pool)
{
  svn_wc__db_t *db = eb->db;
  const char *textbase;
  const char *empty_file;
  svn_boolean_t replaced;
  svn_wc__db_status_t status;
  const char *original_repos_relpath;
  svn_revnum_t revision;
  svn_revnum_t revert_base_revnum;
  svn_boolean_t have_base;
  svn_wc__db_status_t base_status;
  svn_boolean_t use_base = FALSE;

  SVN_ERR_ASSERT(! eb->use_text_base);

  /* If the item is not a member of a specified changelist (and there are
     some specified changelists), skip it. */
  if (! svn_wc__internal_changelist_match(db, local_abspath,
                                          eb->changelist_hash, scratch_pool))
    return SVN_NO_ERROR;

  SVN_ERR(svn_wc__db_read_info(&status, NULL, &revision, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL,
                               &have_base, NULL, NULL,
                               db, local_abspath, scratch_pool, scratch_pool));
  if (have_base)
    SVN_ERR(svn_wc__db_base_get_info(&base_status, NULL, &revert_base_revnum,
                                     NULL, NULL, NULL, NULL, NULL, NULL,
                                     NULL, NULL, NULL, NULL, NULL, NULL,
                                     db, local_abspath,
                                     scratch_pool, scratch_pool));

  replaced = ((status == svn_wc__db_status_added)
              && have_base
              && base_status != svn_wc__db_status_not_present);

  /* Now refine ADDED to one of: ADDED, COPIED, MOVED_HERE. Note that only
     the latter two have corresponding pristine info to diff against.  */
  if (status == svn_wc__db_status_added)
    SVN_ERR(svn_wc__db_scan_addition(&status, NULL, NULL, NULL, NULL,
                                     &original_repos_relpath, NULL, NULL,
                                     NULL, db, local_abspath,
                                     scratch_pool, scratch_pool));

  /* A wc-wc diff of replaced files actually shows a diff against the
   * revert-base, showing all previous lines as removed and adding all new
   * lines. This does not happen for copied/moved-here files, not even with
   * show_copies_as_adds == TRUE (in which case copy/move is really shown as
   * an add, diffing against the empty file).
   * So show the revert-base revision for plain replaces. */
  if (replaced
      && ! (status == svn_wc__db_status_copied
            || status == svn_wc__db_status_moved_here))
    {
      use_base = TRUE;
      revision = revert_base_revnum;
    }

  /* Set TEXTBASE to the path to the text-base file that we want to diff
     against.

     ### There shouldn't be cases where the result is NULL, but at present
     there might be - see get_nearest_pristine_text_as_file(). */
  SVN_ERR(get_pristine_file(&textbase, db, local_abspath,
                            use_base, scratch_pool, scratch_pool));

  SVN_ERR(get_empty_file(eb, &empty_file));

  /* Delete compares text-base against empty file, modifications to the
   * working-copy version of the deleted file are not wanted.
   * Replace is treated like a delete plus an add: two comparisons are
   * generated, first one for the delete and then one for the add.
   * However, if this file was replaced and we are ignoring ancestry,
   * report it as a normal file modification instead. */
  if ((! replaced && status == svn_wc__db_status_deleted) ||
      (replaced && ! eb->ignore_ancestry))
    {
      const char *base_mimetype;
      apr_hash_t *baseprops;

      /* Get svn:mime-type from pristine props (in BASE or WORKING) of PATH. */
      SVN_ERR(svn_wc__get_pristine_props(&baseprops, db, local_abspath,
                                         scratch_pool, scratch_pool));
      if (baseprops)
        base_mimetype = get_prop_mimetype(baseprops);
      else
        base_mimetype = NULL;

      SVN_ERR(eb->callbacks->file_deleted(NULL, NULL, path,
                                          textbase,
                                          empty_file,
                                          base_mimetype,
                                          NULL,
                                          baseprops,
                                          eb->callback_baton,
                                          scratch_pool));

      if (! (replaced && ! eb->ignore_ancestry))
        {
          /* We're here only for showing a delete, so we're done. */
          return SVN_NO_ERROR;
        }
    }

 /* Now deal with showing additions, or the add-half of replacements.
  * If the item is schedule-add *with history*, then we usually want
  * to see the usual working vs. text-base comparison, which will show changes
  * made since the file was copied.  But in case we're showing copies as adds,
  * we need to compare the copied file to the empty file. If we're doing a git
  * diff, and the file was copied, we need to report the file as added and
  * diff it against the text base, so that a "copied" git diff header, and
  * possibly a diff against the copy source, will be generated for it. */
  if ((! replaced && status == svn_wc__db_status_added) ||
     (replaced && ! eb->ignore_ancestry) ||
     ((status == svn_wc__db_status_copied ||
       status == svn_wc__db_status_moved_here) &&
         (eb->show_copies_as_adds || eb->use_git_diff_format)))
    {
      const char *translated = NULL;
      const char *working_mimetype;
      apr_hash_t *baseprops;
      apr_hash_t *workingprops;
      apr_array_header_t *propchanges;

      /* Get svn:mime-type from ACTUAL props of PATH. */
      SVN_ERR(svn_wc__get_actual_props(&workingprops, db, local_abspath,
                                       scratch_pool, scratch_pool));
      working_mimetype = get_prop_mimetype(workingprops);

      /* Set the original properties to empty, then compute "changes" from
         that. Essentially, all ACTUAL props will be "added".  */
      baseprops = apr_hash_make(scratch_pool);
      SVN_ERR(svn_prop_diffs(&propchanges, workingprops, baseprops,
                             scratch_pool));

      SVN_ERR(svn_wc__internal_translated_file(
              &translated, local_abspath, db, local_abspath,
              SVN_WC_TRANSLATE_TO_NF | SVN_WC_TRANSLATE_USE_GLOBAL_TMP,
              eb->cancel_func, eb->cancel_baton,
              scratch_pool, scratch_pool));

      SVN_ERR(eb->callbacks->file_added(NULL, NULL, NULL, path,
                                        (! eb->show_copies_as_adds &&
                                         eb->use_git_diff_format &&
                                         status != svn_wc__db_status_added) ?
                                          textbase : empty_file,
                                        translated,
                                        0, revision,
                                        NULL,
                                        working_mimetype,
                                        original_repos_relpath,
                                        SVN_INVALID_REVNUM, propchanges,
                                        baseprops, eb->callback_baton,
                                        scratch_pool));
    }
  else
    {
      const char *translated = NULL;
      apr_hash_t *baseprops;
      const char *base_mimetype;
      const char *working_mimetype;
      apr_hash_t *workingprops;
      apr_array_header_t *propchanges;
      svn_boolean_t modified;

      /* Here we deal with showing pure modifications. */
      SVN_ERR(svn_wc__internal_file_modified_p(&modified, db, local_abspath,
                                               FALSE, scratch_pool));
      if (modified)
        {
          /* Note that this might be the _second_ time we translate
             the file, as svn_wc__text_modified_internal_p() might have used a
             tmp translated copy too.  But what the heck, diff is
             already expensive, translating twice for the sake of code
             modularity is liveable. */
          SVN_ERR(svn_wc__internal_translated_file(
                    &translated, local_abspath, db, local_abspath,
                    SVN_WC_TRANSLATE_TO_NF | SVN_WC_TRANSLATE_USE_GLOBAL_TMP,
                    eb->cancel_func, eb->cancel_baton,
                    scratch_pool, scratch_pool));
        }

      /* Get the properties, the svn:mime-type values, and compute the
         differences between the two.  */
      if (replaced
          && eb->ignore_ancestry)
        {
          /* We don't want the normal pristine properties (which are
             from the WORKING tree). We want the pristines associated
             with the BASE tree, which are saved as "revert" props.  */
          SVN_ERR(svn_wc__db_base_get_props(&baseprops,
                                            db, local_abspath,
                                            scratch_pool, scratch_pool));
        }
      else
        {
          /* We can only fetch the pristine props (from BASE or WORKING) if
             the node has not been replaced, or it was copied/moved here.  */
          SVN_ERR_ASSERT(!replaced
                         || status == svn_wc__db_status_copied
                         || status == svn_wc__db_status_moved_here);

          SVN_ERR(svn_wc__get_pristine_props(&baseprops, db, local_abspath,
                                             scratch_pool, scratch_pool));

          /* baseprops will be NULL for added nodes */
          if (!baseprops)
            baseprops = apr_hash_make(scratch_pool);
        }
      base_mimetype = get_prop_mimetype(baseprops);

      SVN_ERR(svn_wc__get_actual_props(&workingprops, db, local_abspath,
                                       scratch_pool, scratch_pool));
      working_mimetype = get_prop_mimetype(workingprops);

      SVN_ERR(svn_prop_diffs(&propchanges, workingprops, baseprops, scratch_pool));

      if (modified || propchanges->nelts > 0)
        {
          SVN_ERR(eb->callbacks->file_changed(NULL, NULL, NULL,
                                              path,
                                              modified ? textbase : NULL,
                                              translated,
                                              revision,
                                              SVN_INVALID_REVNUM,
                                              base_mimetype,
                                              working_mimetype,
                                              propchanges, baseprops,
                                              eb->callback_baton,
                                              scratch_pool));
        }
    }

  return SVN_NO_ERROR;
}

/* Called when the directory is closed to compare any elements that have
 * not yet been compared.  This identifies local, working copy only
 * changes.  At this stage we are dealing with files/directories that do
 * exist in the working copy.
 *
 * DIR_BATON is the baton for the directory.
 */
static svn_error_t *
walk_local_nodes_diff(struct edit_baton *eb,
                      const char *local_abspath,
                      const char *path,
                      svn_depth_t depth,
                      apr_hash_t *compared,
                      apr_pool_t *scratch_pool)
{
  svn_wc__db_t *db = eb->db;
  const apr_array_header_t *children;
  int i;
  svn_boolean_t in_anchor_not_target;
  apr_pool_t *iterpool;

  /* Everything we do below is useless if we are comparing to BASE. */
  if (eb->use_text_base)
    return SVN_NO_ERROR;

  /* Determine if this is the anchor directory if the anchor is different
     to the target. When the target is a file, the anchor is the parent
     directory and if this is that directory the non-target entries must be
     skipped. */
  in_anchor_not_target = ((*path == '\0') && (*eb->target != '\0'));

  /* Check for local property mods on this directory, if we haven't
     already reported them and we aren't changelist-filted.
     ### it should be noted that we do not currently allow directories
     ### to be part of changelists, so if a changelist is provided, the
     ### changelist check will always fail. */
  if (svn_wc__internal_changelist_match(db, local_abspath,
                                        eb->changelist_hash, scratch_pool)
      && (! in_anchor_not_target)
      && (!compared || ! apr_hash_get(compared, path, 0)))
    {
      svn_boolean_t modified;

      SVN_ERR(svn_wc__props_modified(&modified, db, local_abspath,
                                     scratch_pool));
      if (modified)
        {
          apr_array_header_t *propchanges;
          apr_hash_t *baseprops;

          SVN_ERR(svn_wc__internal_propdiff(&propchanges, &baseprops,
                                            db, local_abspath,
                                            scratch_pool, scratch_pool));

          SVN_ERR(eb->callbacks->dir_props_changed(NULL, NULL,
                                                   path, FALSE /* ### ? */,
                                                   propchanges, baseprops,
                                                   eb->callback_baton,
                                                   scratch_pool));
        }
    }

  if (depth == svn_depth_empty && !in_anchor_not_target)
    return SVN_NO_ERROR;

  iterpool = svn_pool_create(scratch_pool);

  SVN_ERR(svn_wc__db_read_children(&children, db, local_abspath,
                                   scratch_pool, iterpool));

  for (i = 0; i < children->nelts; i++)
    {
      const char *name = APR_ARRAY_IDX(children, i, const char*);
      const char *child_abspath, *child_path;
      svn_wc__db_status_t status;
      svn_wc__db_kind_t kind;

      svn_pool_clear(iterpool);

      if (eb->cancel_func)
        SVN_ERR(eb->cancel_func(eb->cancel_baton));

      /* In the anchor directory, if the anchor is not the target then all
         entries other than the target should not be diff'd. Running diff
         on one file in a directory should not diff other files in that
         directory. */
      if (in_anchor_not_target && strcmp(eb->target, name))
        continue;

      child_abspath = svn_dirent_join(local_abspath, name, iterpool);

      SVN_ERR(svn_wc__db_read_info(&status, &kind, NULL, NULL, NULL, NULL,
                                   NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                   NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                   NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                   db, child_abspath,
                                   iterpool, iterpool));

      if (status == svn_wc__db_status_not_present
          || status == svn_wc__db_status_excluded
          || status == svn_wc__db_status_server_excluded)
        continue;

      child_path = svn_relpath_join(path, name, iterpool);

      /* Skip this node if it is in the list of nodes already diff'd. */
      if (compared && apr_hash_get(compared, child_path, APR_HASH_KEY_STRING))
        continue;

      switch (kind)
        {
        case svn_wc__db_kind_file:
        case svn_wc__db_kind_symlink:
          SVN_ERR(file_diff(eb, child_abspath, child_path, iterpool));
          break;

        case svn_wc__db_kind_dir:
          /* ### TODO: Don't know how to do replaced dirs. How do I get
             information about what is being replaced? If it was a
             directory then the directory elements are also going to be
             deleted. We need to show deletion diffs for these
             files. If it was a file we need to show a deletion diff
             for that file. */

          /* Check the subdir if in the anchor (the subdir is the target), or
             if recursive */
          if (in_anchor_not_target
              || (depth > svn_depth_files)
              || (depth == svn_depth_unknown))
            {
              svn_depth_t depth_below_here = depth;

              if (depth_below_here == svn_depth_immediates)
                depth_below_here = svn_depth_empty;

              SVN_ERR(walk_local_nodes_diff(eb,
                                            child_abspath,
                                            child_path,
                                            depth_below_here,
                                            NULL,
                                            iterpool));
            }
          break;

        default:
          break;
        }
    }

  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}

/* Report an existing file in the working copy (either in BASE or WORKING)
 * as having been added.
 *
 * DIR_BATON is the parent directory baton, ADM_ACCESS/PATH is the path
 * to the file to be compared.
 *
 * Do all allocation in POOL.
 */
static svn_error_t *
report_wc_file_as_added(struct edit_baton *eb,
                        const char *local_abspath,
                        const char *path,
                        apr_pool_t *scratch_pool)
{
  svn_wc__db_t *db = eb->db;
  apr_hash_t *emptyprops;
  const char *mimetype;
  apr_hash_t *wcprops = NULL;
  apr_array_header_t *propchanges;
  const char *empty_file;
  const char *source_file;
  const char *translated_file;
  svn_wc__db_status_t status;
  svn_revnum_t revision;

  /* If this entry is filtered by changelist specification, do nothing. */
  if (! svn_wc__internal_changelist_match(db, local_abspath,
                                          eb->changelist_hash, scratch_pool))
    return SVN_NO_ERROR;

  SVN_ERR(get_empty_file(eb, &empty_file));

  SVN_ERR(svn_wc__db_read_info(&status, NULL, &revision, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               db, local_abspath,
                               scratch_pool, scratch_pool));

  if (status == svn_wc__db_status_added)
    SVN_ERR(svn_wc__db_scan_addition(&status, NULL, NULL, NULL, NULL, NULL,
                                     NULL, NULL, NULL, db, local_abspath,
                                     scratch_pool, scratch_pool));

  /* We can't show additions for files that don't exist. */
  SVN_ERR_ASSERT(status != svn_wc__db_status_deleted || eb->use_text_base);

  /* If the file was added *with history*, then we don't want to
     see a comparison to the empty file;  we want the usual working
     vs. text-base comparison. */
  if (status == svn_wc__db_status_copied ||
      status == svn_wc__db_status_moved_here)
    {
      /* Don't show anything if we're comparing to BASE, since by
         definition there can't be any local modifications. */
      if (eb->use_text_base)
        return SVN_NO_ERROR;

      /* Otherwise show just the local modifications. */
      return file_diff(eb, local_abspath, path, scratch_pool);
    }

  emptyprops = apr_hash_make(scratch_pool);

  if (eb->use_text_base)
    SVN_ERR(svn_wc__get_pristine_props(&wcprops, db, local_abspath,
                                       scratch_pool, scratch_pool));
  else
    SVN_ERR(svn_wc__get_actual_props(&wcprops, db, local_abspath,
                                     scratch_pool, scratch_pool));
  mimetype = get_prop_mimetype(wcprops);

  SVN_ERR(svn_prop_diffs(&propchanges,
                         wcprops, emptyprops, scratch_pool));


  if (eb->use_text_base)
    SVN_ERR(get_pristine_file(&source_file, db, local_abspath,
                              FALSE, scratch_pool, scratch_pool));
  else
    source_file = local_abspath;

  SVN_ERR(svn_wc__internal_translated_file(
           &translated_file, source_file, db, local_abspath,
           SVN_WC_TRANSLATE_TO_NF | SVN_WC_TRANSLATE_USE_GLOBAL_TMP,
           eb->cancel_func, eb->cancel_baton,
           scratch_pool, scratch_pool));

  SVN_ERR(eb->callbacks->file_added(NULL, NULL, NULL,
                                    path,
                                    empty_file, translated_file,
                                    0, revision,
                                    NULL, mimetype,
                                    NULL, SVN_INVALID_REVNUM,
                                    propchanges, emptyprops,
                                    eb->callback_baton,
                                    scratch_pool));

  return SVN_NO_ERROR;
}

/* Report an existing directory in the working copy (either in BASE
 * or WORKING) as having been added.  If recursing, also report any
 * subdirectories as added.
 *
 * DIR_BATON is the baton for the directory.
 *
 * Do all allocation in POOL.
 */
static svn_error_t *
report_wc_directory_as_added(struct edit_baton *eb,
                             const char *local_abspath,
                             const char *path,
                             svn_depth_t depth,
                             apr_pool_t *scratch_pool)
{
  svn_wc__db_t *db = eb->db;
  apr_hash_t *emptyprops, *wcprops = NULL;
  apr_array_header_t *propchanges;
  const apr_array_header_t *children;
  int i;
  apr_pool_t *iterpool;

  emptyprops = apr_hash_make(scratch_pool);

  /* If this directory passes changelist filtering, get its BASE or
     WORKING properties, as appropriate, and simulate their
     addition.
     ### it should be noted that we do not currently allow directories
     ### to be part of changelists, so if a changelist is provided, this
     ### check will always fail. */
  if (svn_wc__internal_changelist_match(db, local_abspath,
                                        eb->changelist_hash, scratch_pool))
    {
      if (eb->use_text_base)
        SVN_ERR(svn_wc__get_pristine_props(&wcprops, db, local_abspath,
                                           scratch_pool, scratch_pool));
      else
        SVN_ERR(svn_wc__get_actual_props(&wcprops, db, local_abspath,
                                         scratch_pool, scratch_pool));

      SVN_ERR(svn_prop_diffs(&propchanges, wcprops, emptyprops, scratch_pool));

      if (propchanges->nelts > 0)
        SVN_ERR(eb->callbacks->dir_props_changed(NULL, NULL,
                                                 path, TRUE,
                                                 propchanges, emptyprops,
                                                 eb->callback_baton,
                                                 scratch_pool));
    }

  /* Report the addition of the directory's contents. */
  iterpool = svn_pool_create(scratch_pool);

  SVN_ERR(svn_wc__db_read_children(&children, db, local_abspath,
                                   scratch_pool, iterpool));

  for (i = 0; i < children->nelts; i++)
    {
      const char *name = APR_ARRAY_IDX(children, i, const char *);
      const char *child_abspath, *child_path;
      svn_wc__db_status_t status;
      svn_wc__db_kind_t kind;

      svn_pool_clear(iterpool);

      if (eb->cancel_func)
        SVN_ERR(eb->cancel_func(eb->cancel_baton));

      child_abspath = svn_dirent_join(local_abspath, name, iterpool);

      SVN_ERR(svn_wc__db_read_info(&status, &kind, NULL, NULL, NULL, NULL,
                                   NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                   NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                   NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                   db, child_abspath, iterpool, iterpool));

      if (status == svn_wc__db_status_not_present
          || status == svn_wc__db_status_excluded
          || status == svn_wc__db_status_server_excluded)
        {
          continue;
        }

      /* If comparing against WORKING, skip entries that are
         schedule-deleted - they don't really exist. */
      if (!eb->use_text_base && status == svn_wc__db_status_deleted)
        continue;

      child_path = svn_relpath_join(path, name, iterpool);

      switch (kind)
        {
        case svn_wc__db_kind_file:
        case svn_wc__db_kind_symlink:
          SVN_ERR(report_wc_file_as_added(eb, child_abspath, child_path,
                                          iterpool));
          break;

        case svn_wc__db_kind_dir:
          if (depth > svn_depth_files || depth == svn_depth_unknown)
            {
              svn_depth_t depth_below_here = depth;

              if (depth_below_here == svn_depth_immediates)
                depth_below_here = svn_depth_empty;

              SVN_ERR(report_wc_directory_as_added(eb,
                                                   child_abspath,
                                                   child_path,
                                                   depth_below_here,
                                                   iterpool));
            }
          break;

        default:
          break;
        }
    }

  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}


/* An editor function. */
static svn_error_t *
set_target_revision(void *edit_baton,
                    svn_revnum_t target_revision,
                    apr_pool_t *pool)
{
  struct edit_baton *eb = edit_baton;
  eb->revnum = target_revision;

  return SVN_NO_ERROR;
}

/* An editor function. The root of the comparison hierarchy */
static svn_error_t *
open_root(void *edit_baton,
          svn_revnum_t base_revision,
          apr_pool_t *dir_pool,
          void **root_baton)
{
  struct edit_baton *eb = edit_baton;
  struct dir_baton *db;

  eb->root_opened = TRUE;
  db = make_dir_baton("", NULL, eb, FALSE, eb->depth, dir_pool);
  *root_baton = db;

  return SVN_NO_ERROR;
}

/* An editor function. */
static svn_error_t *
delete_entry(const char *path,
             svn_revnum_t base_revision,
             void *parent_baton,
             apr_pool_t *pool)
{
  struct dir_baton *pb = parent_baton;
  struct edit_baton *eb = pb->eb;
  svn_wc__db_t *db = eb->db;
  const char *empty_file;
  const char *name = svn_dirent_basename(path, NULL);
  const char *local_abspath = svn_dirent_join(pb->local_abspath, name, pool);
  svn_wc__db_status_t status;
  svn_wc__db_kind_t kind;

  /* Mark this node as compared in the parent directory's baton. */
  apr_hash_set(pb->compared, apr_pstrdup(pb->pool, path),
               APR_HASH_KEY_STRING, "");

  SVN_ERR(svn_wc__db_read_info(&status, &kind, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               db, local_abspath, pool, pool));

  /* If comparing against WORKING, skip nodes that are deleted
     - they don't really exist. */
  if (!eb->use_text_base && status == svn_wc__db_status_deleted)
    return SVN_NO_ERROR;

  SVN_ERR(get_empty_file(pb->eb, &empty_file));
  switch (kind)
    {
    case svn_wc__db_kind_file:
    case svn_wc__db_kind_symlink:
      /* A delete is required to change working-copy into requested
         revision, so diff should show this as an add. Thus compare
         the empty file against the current working copy.  If
         'reverse_order' is set, then show a deletion. */

      if (eb->reverse_order)
        {
          /* Whenever showing a deletion, we show the text-base vanishing. */
          /* ### This is wrong if we're diffing WORKING->repos. */
          const char *textbase;

          apr_hash_t *baseprops = NULL;
          const char *base_mimetype;

          SVN_ERR(get_pristine_file(&textbase, db, local_abspath,
                                    eb->use_text_base, pool, pool));

          SVN_ERR(svn_wc__get_pristine_props(&baseprops, eb->db, local_abspath,
                                             pool, pool));
          base_mimetype = get_prop_mimetype(baseprops);

          SVN_ERR(eb->callbacks->file_deleted(NULL, NULL, path,
                                              textbase,
                                              empty_file,
                                              base_mimetype,
                                              NULL,
                                              baseprops,
                                              eb->callback_baton,
                                              pool));
        }
      else
        {
          /* Or normally, show the working file being added. */
          SVN_ERR(report_wc_file_as_added(eb, local_abspath, path, pool));
        }
      break;
    case svn_wc__db_kind_dir:
      /* A delete is required to change working-copy into requested
         revision, so diff should show this as an add. */
      SVN_ERR(report_wc_directory_as_added(eb,
                                           local_abspath,
                                           path,
                                           svn_depth_infinity,
                                           pool));

    default:
      break;
    }

  return SVN_NO_ERROR;
}

/* An editor function. */
static svn_error_t *
add_directory(const char *path,
              void *parent_baton,
              const char *copyfrom_path,
              svn_revnum_t copyfrom_revision,
              apr_pool_t *dir_pool,
              void **child_baton)
{
  struct dir_baton *pb = parent_baton;
  struct dir_baton *db;
  svn_depth_t subdir_depth = (pb->depth == svn_depth_immediates)
                              ? svn_depth_empty : pb->depth;

  /* ### TODO: support copyfrom? */

  db = make_dir_baton(path, pb, pb->eb, TRUE, subdir_depth,
                      dir_pool);
  *child_baton = db;

  /* Issue #3797: Don't add this filename to the parent directory's list of
     elements that have been compared, to show local additions via the local
     diff. The repository node is unrelated from the working copy version
     (similar to not-present in the working copy) */

  return SVN_NO_ERROR;
}

/* An editor function. */
static svn_error_t *
open_directory(const char *path,
               void *parent_baton,
               svn_revnum_t base_revision,
               apr_pool_t *dir_pool,
               void **child_baton)
{
  struct dir_baton *pb = parent_baton;
  struct dir_baton *db;
  svn_depth_t subdir_depth = (pb->depth == svn_depth_immediates)
                              ? svn_depth_empty : pb->depth;

  /* Allocate path from the parent pool since the memory is used in the
     parent's compared hash */
  db = make_dir_baton(path, pb, pb->eb, FALSE, subdir_depth, dir_pool);
  *child_baton = db;

  /* Add this path to the parent directory's list of elements that
     have been compared. */
  apr_hash_set(pb->compared, apr_pstrdup(pb->pool, db->path),
               APR_HASH_KEY_STRING, "");

  SVN_ERR(db->eb->callbacks->dir_opened(NULL, NULL, NULL,
                                        path, base_revision,
                                        db->eb->callback_baton, dir_pool));

  return SVN_NO_ERROR;
}


/* An editor function.  When a directory is closed, all the directory
 * elements that have been added or replaced will already have been
 * diff'd. However there may be other elements in the working copy
 * that have not yet been considered.  */
static svn_error_t *
close_directory(void *dir_baton,
                apr_pool_t *pool)
{
  struct dir_baton *db = dir_baton;
  struct edit_baton *eb = db->eb;
  apr_pool_t *scratch_pool = db->pool;

  /* Report the property changes on the directory itself, if necessary. */
  if (db->propchanges->nelts > 0)
    {
      /* The working copy properties at the base of the wc->repos comparison:
         either BASE or WORKING. */
      apr_hash_t *originalprops;

      if (db->added)
        {
          originalprops = apr_hash_make(scratch_pool);
        }
      else
        {
          if (db->eb->use_text_base)
            {
              SVN_ERR(svn_wc__get_pristine_props(&originalprops,
                                                 eb->db, db->local_abspath,
                                                 scratch_pool, scratch_pool));
            }
          else
            {
              apr_hash_t *base_props, *repos_props;

              SVN_ERR(svn_wc__get_actual_props(&originalprops,
                                               eb->db, db->local_abspath,
                                               scratch_pool, scratch_pool));

              /* Load the BASE and repository directory properties. */
              SVN_ERR(svn_wc__get_pristine_props(&base_props,
                                                 eb->db, db->local_abspath,
                                                 scratch_pool, scratch_pool));

              repos_props = apply_propchanges(base_props, db->propchanges);

              /* Recalculate b->propchanges as the change between WORKING
                 and repos. */
              SVN_ERR(svn_prop_diffs(&db->propchanges, repos_props,
                                     originalprops, scratch_pool));
            }
        }

      if (!eb->reverse_order)
        reverse_propchanges(originalprops, db->propchanges, db->pool);

      SVN_ERR(eb->callbacks->dir_props_changed(NULL, NULL,
                                               db->path,
                                               db->added,
                                               db->propchanges,
                                               originalprops,
                                               eb->callback_baton,
                                               scratch_pool));

      /* Mark the properties of this directory as having already been
         compared so that we know not to show any local modifications
         later on. */
      apr_hash_set(db->compared, db->path, 0, "");
    }

  /* Report local modifications for this directory.  Skip added
     directories since they can only contain added elements, all of
     which have already been diff'd. */
  if (!db->added)
    SVN_ERR(walk_local_nodes_diff(eb,
                                  db->local_abspath,
                                  db->path,
                                  db->depth,
                                  db->compared,
                                  scratch_pool));

  /* Mark this directory as compared in the parent directory's baton,
     unless this is the root of the comparison. */
  SVN_ERR(db->eb->callbacks->dir_closed(NULL, NULL, NULL, db->path,
                                        db->added, db->eb->callback_baton,
                                        scratch_pool));

  svn_pool_destroy(db->pool); /* destroys scratch_pool */

  return SVN_NO_ERROR;
}

/* An editor function. */
static svn_error_t *
add_file(const char *path,
         void *parent_baton,
         const char *copyfrom_path,
         svn_revnum_t copyfrom_revision,
         apr_pool_t *file_pool,
         void **file_baton)
{
  struct dir_baton *pb = parent_baton;
  struct file_baton *fb;

  /* ### TODO: support copyfrom? */

  fb = make_file_baton(path, TRUE, pb, file_pool);
  *file_baton = fb;

  /* Issue #3797: Don't add this filename to the parent directory's list of
     elements that have been compared, to show local additions via the local
     diff. The repository node is unrelated from the working copy version
     (similar to not-present in the working copy) */

  return SVN_NO_ERROR;
}

/* An editor function. */
static svn_error_t *
open_file(const char *path,
          void *parent_baton,
          svn_revnum_t base_revision,
          apr_pool_t *file_pool,
          void **file_baton)
{
  struct dir_baton *pb = parent_baton;
  struct edit_baton *eb = pb->eb;
  struct file_baton *fb;

  fb = make_file_baton(path, FALSE, pb, file_pool);
  *file_baton = fb;

  /* Add this filename to the parent directory's list of elements that
     have been compared. */
  apr_hash_set(pb->compared, apr_pstrdup(pb->pool, path),
               APR_HASH_KEY_STRING, "");

  SVN_ERR(svn_wc__db_base_get_info(NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                   NULL, NULL, NULL, &fb->base_checksum, NULL,
                                   NULL, NULL, NULL,
                                   eb->db, fb->local_abspath,
                                   fb->pool, fb->pool));

  SVN_ERR(eb->callbacks->file_opened(NULL, NULL, fb->path, base_revision,
                                     eb->callback_baton, fb->pool));

  return SVN_NO_ERROR;
}

/* Baton for window_handler */
struct window_handler_baton
{
  struct file_baton *fb;

  /* APPLY_HANDLER/APPLY_BATON represent the delta applcation baton. */
  svn_txdelta_window_handler_t apply_handler;
  void *apply_baton;

  unsigned char result_digest[APR_MD5_DIGESTSIZE];
};

/* Do the work of applying the text delta. */
static svn_error_t *
window_handler(svn_txdelta_window_t *window,
               void *window_baton)
{
  struct window_handler_baton *whb = window_baton;
  struct file_baton *fb = whb->fb;

  SVN_ERR(whb->apply_handler(window, whb->apply_baton));

  if (!window)
    {
      fb->result_checksum = svn_checksum__from_digest(whb->result_digest,
                                                      svn_checksum_md5,
                                                      fb->pool);
    }

  return SVN_NO_ERROR;
}

/* An editor function. */
static svn_error_t *
apply_textdelta(void *file_baton,
                const char *base_checksum,
                apr_pool_t *pool,
                svn_txdelta_window_handler_t *handler,
                void **handler_baton)
{
  struct file_baton *fb = file_baton;
  struct window_handler_baton *whb;
  struct edit_baton *eb = fb->eb;
  svn_stream_t *source;
  svn_stream_t *temp_stream;

  if (fb->base_checksum)
    SVN_ERR(svn_wc__db_pristine_read(&source, NULL,
                                     eb->db, fb->local_abspath,
                                     fb->base_checksum,
                                     pool, pool));
  else
    source = svn_stream_empty(pool);

  /* This is the file that will contain the pristine repository version. */
  SVN_ERR(svn_stream_open_unique(&temp_stream, &fb->temp_file_path, NULL,
                                 svn_io_file_del_on_pool_cleanup,
                                 fb->pool, fb->pool));

  whb = apr_pcalloc(fb->pool, sizeof(*whb));
  whb->fb = fb;

  svn_txdelta_apply(source, temp_stream,
                    whb->result_digest,
                    fb->path /* error_info */,
                    fb->pool,
                    &whb->apply_handler, &whb->apply_baton);

  *handler = window_handler;
  *handler_baton = whb;
  return SVN_NO_ERROR;
}

/* An editor function.  When the file is closed we have a temporary
 * file containing a pristine version of the repository file. This can
 * be compared against the working copy.
 *
 * Ignore TEXT_CHECKSUM.
 */
static svn_error_t *
close_file(void *file_baton,
           const char *expected_md5_digest,
           apr_pool_t *pool)
{
  struct file_baton *fb = file_baton;
  struct edit_baton *eb = fb->eb;
  svn_wc__db_t *db = eb->db;
  apr_pool_t *scratch_pool = fb->pool;
  svn_wc__db_status_t status;
  const char *empty_file;
  svn_error_t *err;

  /* The BASE information */
  const svn_checksum_t *pristine_checksum;
  const char *pristine_file;
  apr_hash_t *pristine_props;

  /* The repository information; constructed from BASE + Changes */
  const char *repos_file;
  apr_hash_t *repos_props;
  const char *repos_mimetype;
  svn_boolean_t had_props, props_mod;

  /* The path to the wc file: either a pristine or actual. */
  const char *localfile;
  svn_boolean_t modified;
  /* The working copy properties at the base of the wc->repos
     comparison: either BASE or WORKING. */
  apr_hash_t *originalprops;

  if (expected_md5_digest != NULL)
    {
      svn_checksum_t *expected_checksum;
      const svn_checksum_t *repos_checksum = fb->result_checksum;

      SVN_ERR(svn_checksum_parse_hex(&expected_checksum, svn_checksum_md5,
                                     expected_md5_digest, scratch_pool));

      if (repos_checksum == NULL)
        repos_checksum = fb->base_checksum;

      if (repos_checksum->kind != svn_checksum_md5)
        SVN_ERR(svn_wc__db_pristine_get_md5(&repos_checksum,
                                            eb->db, fb->local_abspath,
                                            repos_checksum,
                                            scratch_pool, scratch_pool));

      if (!svn_checksum_match(expected_checksum, repos_checksum))
        return svn_checksum_mismatch_err(
                            expected_checksum,
                            repos_checksum,
                            pool,
                            _("Checksum mismatch for '%s'"),
                            svn_dirent_local_style(fb->local_abspath,
                                                   scratch_pool));
    }

  err = svn_wc__db_read_info(&status, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, &pristine_checksum, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, &had_props, &props_mod,
                             NULL, NULL, NULL,
                             db, fb->local_abspath,
                             scratch_pool, scratch_pool);
  if (fb->added
      && err && err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
    {
      svn_error_clear(err);
      status = svn_wc__db_status_not_present;
      pristine_checksum = NULL;
      had_props = FALSE;
      props_mod = FALSE;
    }
  else
    SVN_ERR(err);

  SVN_ERR(get_empty_file(eb, &empty_file));

  if (fb->added)
    {
      pristine_props = apr_hash_make(scratch_pool);
      pristine_file = empty_file;
    }
  else
    {
      if (status != svn_wc__db_status_normal)
        SVN_ERR(svn_wc__db_base_get_info(NULL, NULL, NULL, NULL, NULL, NULL,
                                         NULL, NULL, NULL, NULL,
                                         &pristine_checksum,
                                         NULL, NULL,
                                         &had_props, NULL,
                                         db, fb->local_abspath,
                                         scratch_pool, scratch_pool));

      SVN_ERR(svn_wc__db_pristine_get_path(&pristine_file,
                                           db, fb->local_abspath,
                                           pristine_checksum,
                                           scratch_pool, scratch_pool));

      if (had_props)
        SVN_ERR(svn_wc__db_base_get_props(&pristine_props,
                                           db, fb->local_abspath,
                                           scratch_pool, scratch_pool));
      else
        pristine_props = apr_hash_make(scratch_pool);
    }

  if (status == svn_wc__db_status_added)
    SVN_ERR(svn_wc__db_scan_addition(&status, NULL, NULL, NULL, NULL, NULL,
                                     NULL, NULL, NULL, eb->db,
                                     fb->local_abspath,
                                     scratch_pool, scratch_pool));

  repos_props = apply_propchanges(pristine_props, fb->propchanges);
  repos_mimetype = get_prop_mimetype(repos_props);
  repos_file = fb->temp_file_path ? fb->temp_file_path : pristine_file;

  /* If the file isn't in the working copy (either because it was added
     in the BASE->repos diff or because we're diffing against WORKING
     and it was marked as schedule-deleted), we show either an addition
     or a deletion of the complete contents of the repository file,
     depending upon the direction of the diff. */
  if (fb->added || (!eb->use_text_base && status == svn_wc__db_status_deleted))
    {
      if (eb->reverse_order)
        return eb->callbacks->file_added(NULL, NULL, NULL, fb->path,
                                         empty_file,
                                         repos_file,
                                         0,
                                         eb->revnum,
                                         NULL,
                                         repos_mimetype,
                                         NULL, SVN_INVALID_REVNUM,
                                         fb->propchanges,
                                         apr_hash_make(pool),
                                         eb->callback_baton,
                                         scratch_pool);
      else
        return eb->callbacks->file_deleted(NULL, NULL, fb->path,
                                           repos_file,
                                           empty_file,
                                           repos_mimetype,
                                           NULL,
                                           repos_props,
                                           eb->callback_baton,
                                           scratch_pool);
    }

  /* If the file was locally added with history, and we want to show copies
   * as added, diff the file with the empty file. */
  if ((status == svn_wc__db_status_copied ||
       status == svn_wc__db_status_moved_here) && eb->show_copies_as_adds)
    return eb->callbacks->file_added(NULL, NULL, NULL, fb->path,
                                     empty_file,
                                     fb->local_abspath,
                                     0,
                                     eb->revnum,
                                     NULL,
                                     repos_mimetype,
                                     NULL, SVN_INVALID_REVNUM,
                                     fb->propchanges,
                                     apr_hash_make(pool),
                                     eb->callback_baton,
                                     scratch_pool);

  /* If we didn't see any content changes between the BASE and repository
     versions (i.e. we only saw property changes), then, if we're diffing
     against WORKING, we also need to check whether there are any local
     (BASE:WORKING) modifications. */
  modified = (fb->temp_file_path != NULL);
  if (!modified && !eb->use_text_base)
    SVN_ERR(svn_wc__internal_file_modified_p(&modified, eb->db,
                                             fb->local_abspath,
                                             FALSE, scratch_pool));

  if (modified)
    {
      if (eb->use_text_base)
        SVN_ERR(get_pristine_file(&localfile, eb->db, fb->local_abspath,
                                  FALSE, scratch_pool, scratch_pool));
      else
        /* a detranslated version of the working file */
        SVN_ERR(svn_wc__internal_translated_file(
                 &localfile, fb->local_abspath, eb->db, fb->local_abspath,
                 SVN_WC_TRANSLATE_TO_NF | SVN_WC_TRANSLATE_USE_GLOBAL_TMP,
                 eb->cancel_func, eb->cancel_baton,
                 scratch_pool, scratch_pool));
    }
  else
    localfile = repos_file = NULL;

  if (eb->use_text_base)
    {
      originalprops = pristine_props;
    }
  else
    {
      SVN_ERR(svn_wc__get_actual_props(&originalprops,
                                       eb->db, fb->local_abspath,
                                       scratch_pool, scratch_pool));

      /* We have the repository properties in repos_props, and the
         WORKING properties in originalprops.  Recalculate
         fb->propchanges as the change between WORKING and repos. */
      SVN_ERR(svn_prop_diffs(&fb->propchanges,
                             repos_props, originalprops, scratch_pool));
    }

  if (localfile || fb->propchanges->nelts > 0)
    {
      const char *original_mimetype = get_prop_mimetype(originalprops);

      if (fb->propchanges->nelts > 0
          && ! eb->reverse_order)
        reverse_propchanges(originalprops, fb->propchanges, scratch_pool);

      SVN_ERR(eb->callbacks->file_changed(NULL, NULL, NULL,
                                          fb->path,
                                          eb->reverse_order ? localfile
                                                            : repos_file,
                                           eb->reverse_order
                                                          ? repos_file
                                                          : localfile,
                                           eb->reverse_order
                                                          ? SVN_INVALID_REVNUM
                                                          : eb->revnum,
                                           eb->reverse_order
                                                          ? eb->revnum
                                                          : SVN_INVALID_REVNUM,
                                           eb->reverse_order
                                                          ? original_mimetype
                                                          : repos_mimetype,
                                           eb->reverse_order
                                                          ? repos_mimetype
                                                          : original_mimetype,
                                           fb->propchanges, originalprops,
                                           eb->callback_baton,
                                           scratch_pool));
    }

  svn_pool_destroy(fb->pool); /* destroys scratch_pool */
  return SVN_NO_ERROR;
}


/* An editor function. */
static svn_error_t *
change_file_prop(void *file_baton,
                 const char *name,
                 const svn_string_t *value,
                 apr_pool_t *pool)
{
  struct file_baton *fb = file_baton;
  svn_prop_t *propchange;

  propchange = apr_array_push(fb->propchanges);
  propchange->name = apr_pstrdup(fb->pool, name);
  propchange->value = value ? svn_string_dup(value, fb->pool) : NULL;

  return SVN_NO_ERROR;
}


/* An editor function. */
static svn_error_t *
change_dir_prop(void *dir_baton,
                const char *name,
                const svn_string_t *value,
                apr_pool_t *pool)
{
  struct dir_baton *db = dir_baton;
  svn_prop_t *propchange;

  propchange = apr_array_push(db->propchanges);
  propchange->name = apr_pstrdup(db->pool, name);
  propchange->value = value ? svn_string_dup(value, db->pool) : NULL;

  return SVN_NO_ERROR;
}


/* An editor function. */
static svn_error_t *
close_edit(void *edit_baton,
           apr_pool_t *pool)
{
  struct edit_baton *eb = edit_baton;

  if (!eb->root_opened)
    {
      SVN_ERR(walk_local_nodes_diff(eb,
                                    eb->anchor_abspath,
                                    "",
                                    eb->depth,
                                    NULL,
                                    eb->pool));
    }

  return SVN_NO_ERROR;
}

/* Public Interface */


/* Create a diff editor and baton. */
svn_error_t *
svn_wc_get_diff_editor6(const svn_delta_editor_t **editor,
                        void **edit_baton,
                        svn_wc_context_t *wc_ctx,
                        const char *anchor_abspath,
                        const char *target,
                        svn_depth_t depth,
                        svn_boolean_t ignore_ancestry,
                        svn_boolean_t show_copies_as_adds,
                        svn_boolean_t use_git_diff_format,
                        svn_boolean_t use_text_base,
                        svn_boolean_t reverse_order,
                        svn_boolean_t server_performs_filtering,
                        const apr_array_header_t *changelist_filter,
                        const svn_wc_diff_callbacks4_t *callbacks,
                        void *callback_baton,
                        svn_cancel_func_t cancel_func,
                        void *cancel_baton,
                        apr_pool_t *result_pool,
                        apr_pool_t *scratch_pool)
{
  struct edit_baton *eb;
  void *inner_baton;
  svn_delta_editor_t *tree_editor;
  const svn_delta_editor_t *inner_editor;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(anchor_abspath));

  SVN_ERR(make_edit_baton(&eb,
                          wc_ctx->db,
                          anchor_abspath, target,
                          callbacks, callback_baton,
                          depth, ignore_ancestry, show_copies_as_adds,
                          use_git_diff_format,
                          use_text_base, reverse_order, changelist_filter,
                          cancel_func, cancel_baton,
                          result_pool));

  tree_editor = svn_delta_default_editor(eb->pool);

  tree_editor->set_target_revision = set_target_revision;
  tree_editor->open_root = open_root;
  tree_editor->delete_entry = delete_entry;
  tree_editor->add_directory = add_directory;
  tree_editor->open_directory = open_directory;
  tree_editor->close_directory = close_directory;
  tree_editor->add_file = add_file;
  tree_editor->open_file = open_file;
  tree_editor->apply_textdelta = apply_textdelta;
  tree_editor->change_file_prop = change_file_prop;
  tree_editor->change_dir_prop = change_dir_prop;
  tree_editor->close_file = close_file;
  tree_editor->close_edit = close_edit;

  inner_editor = tree_editor;
  inner_baton = eb;

  if (!server_performs_filtering
      && depth == svn_depth_unknown)
    SVN_ERR(svn_wc__ambient_depth_filter_editor(&inner_editor,
                                                &inner_baton,
                                                wc_ctx->db,
                                                anchor_abspath,
                                                target,
                                                inner_editor,
                                                inner_baton,
                                                result_pool));

  return svn_delta_get_cancellation_editor(cancel_func,
                                           cancel_baton,
                                           inner_editor,
                                           inner_baton,
                                           editor,
                                           edit_baton,
                                           result_pool);
}

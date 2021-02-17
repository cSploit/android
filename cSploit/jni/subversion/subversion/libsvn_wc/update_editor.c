/*
 * update_editor.c :  main editor for checkouts and updates
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



#include <stdlib.h>
#include <string.h>

#include <apr_pools.h>
#include <apr_hash.h>
#include <apr_md5.h>
#include <apr_tables.h>
#include <apr_strings.h>

#include "svn_types.h"
#include "svn_pools.h"
#include "svn_delta.h"
#include "svn_string.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_error.h"
#include "svn_io.h"
#include "svn_private_config.h"
#include "svn_time.h"

#include "wc.h"
#include "adm_files.h"
#include "entries.h"
#include "translate.h"
#include "workqueue.h"

#include "private/svn_wc_private.h"
/* Checks whether a svn_wc__db_status_t indicates whether a node is
   present in a working copy. Used by the editor implementation */
#define IS_NODE_PRESENT(status)                             \
           ((status) != svn_wc__db_status_server_excluded &&\
            (status) != svn_wc__db_status_excluded &&       \
            (status) != svn_wc__db_status_not_present)

static svn_error_t *
path_join_under_root(const char **result_path,
                     const char *base_path,
                     const char *add_path,
                     apr_pool_t *result_pool);


/*
 * This code handles "checkout" and "update" and "switch".
 * A checkout is similar to an update that is only adding new items.
 *
 * The intended behaviour of "update" and "switch", focusing on the checks
 * to be made before applying a change, is:
 *
 *   For each incoming change:
 *     if target is already in conflict or obstructed:
 *       skip this change
 *     else
 *     if this action will cause a tree conflict:
 *       record the tree conflict
 *       skip this change
 *     else:
 *       make this change
 *
 * In more detail:
 *
 *   For each incoming change:
 *
 *   1.   if  # Incoming change is inside an item already in conflict:
 *    a.    tree/text/prop change to node beneath tree-conflicted dir
 *        then  # Skip all changes in this conflicted subtree [*1]:
 *          do not update the Base nor the Working
 *          notify "skipped because already in conflict" just once
 *            for the whole conflicted subtree
 *
 *        if  # Incoming change affects an item already in conflict:
 *    b.    tree/text/prop change to tree-conflicted dir/file, or
 *    c.    tree change to a text/prop-conflicted file/dir, or
 *    d.    text/prop change to a text/prop-conflicted file/dir [*2], or
 *    e.    tree change to a dir tree containing any conflicts,
 *        then  # Skip this change [*1]:
 *          do not update the Base nor the Working
 *          notify "skipped because already in conflict"
 *
 *   2.   if  # Incoming change affects an item that's "obstructed":
 *    a.    on-disk node kind doesn't match recorded Working node kind
 *            (including an absence/presence mis-match),
 *        then  # Skip this change [*1]:
 *          do not update the Base nor the Working
 *          notify "skipped because obstructed"
 *
 *   3.   if  # Incoming change raises a tree conflict:
 *    a.    tree/text/prop change to node beneath sched-delete dir, or
 *    b.    tree/text/prop change to sched-delete dir/file, or
 *    c.    text/prop change to tree-scheduled dir/file,
 *        then  # Skip this change:
 *          do not update the Base nor the Working [*3]
 *          notify "tree conflict"
 *
 *   4.   Apply the change:
 *          update the Base
 *          update the Working, possibly raising text/prop conflicts
 *          notify
 *
 * Notes:
 *
 *      "Tree change" here refers to an add or delete of the target node,
 *      including the add or delete part of a copy or move or rename.
 *
 * [*1] We should skip changes to an entire node, as the base revision number
 *      applies to the entire node. Not sure how this affects attempts to
 *      handle text and prop changes separately.
 *
 * [*2] Details of which combinations of property and text changes conflict
 *      are not specified here.
 *
 * [*3] For now, we skip the update, and require the user to:
 *        - Modify the WC to be compatible with the incoming change;
 *        - Mark the conflict as resolved;
 *        - Repeat the update.
 *      Ideally, it would be possible to resolve any conflict without
 *      repeating the update. To achieve this, we would have to store the
 *      necessary data at conflict detection time, and delay the update of
 *      the Base until the time of resolving.
 */


/*** batons ***/

struct edit_baton
{
  /* For updates, the "destination" of the edit is ANCHOR_ABSPATH, the
     directory containing TARGET_ABSPATH. If ANCHOR_ABSPATH itself is the
     target, the values are identical.

     TARGET_BASENAME is the name of TARGET_ABSPATH in ANCHOR_ABSPATH, or "" if
     ANCHOR_ABSPATH is the target */
  const char *target_basename;

  /* Absolute variants of ANCHOR and TARGET */
  const char *anchor_abspath;
  const char *target_abspath;

  /* The DB handle for managing the working copy state.  */
  svn_wc__db_t *db;

  /* Array of file extension patterns to preserve as extensions in
     generated conflict files. */
  const apr_array_header_t *ext_patterns;

  /* The revision we're targeting...or something like that.  This
     starts off as a pointer to the revision to which we are updating,
     or SVN_INVALID_REVNUM, but by the end of the edit, should be
     pointing to the final revision. */
  svn_revnum_t *target_revision;

  /* The requested depth of this edit. */
  svn_depth_t requested_depth;

  /* Is the requested depth merely an operational limitation, or is
     also the new sticky ambient depth of the update target? */
  svn_boolean_t depth_is_sticky;

  /* Need to know if the user wants us to overwrite the 'now' times on
     edited/added files with the last-commit-time. */
  svn_boolean_t use_commit_times;

  /* Was the root actually opened (was this a non-empty edit)? */
  svn_boolean_t root_opened;

  /* Was the update-target deleted?  This is a special situation. */
  svn_boolean_t target_deleted;

  /* Allow unversioned obstructions when adding a path. */
  svn_boolean_t allow_unver_obstructions;

  /* Handle local additions as modifications of new nodes */
  svn_boolean_t adds_as_modification;

  /* If set, we check out into an empty directory. This allows for a number
     of conflict checks to be omitted. */
  svn_boolean_t clean_checkout;

  /* If this is a 'switch' operation, the new relpath of target_abspath,
     else NULL. */
  const char *switch_relpath;

  /* The URL to the root of the repository. */
  const char *repos_root;

  /* The UUID of the repos, or NULL. */
  const char *repos_uuid;

  /* External diff3 to use for merges (can be null, in which case
     internal merge code is used). */
  const char *diff3_cmd;

  /* Externals handler */
  svn_wc_external_update_t external_func;
  void *external_baton;

  /* This editor sends back notifications as it edits. */
  svn_wc_notify_func2_t notify_func;
  void *notify_baton;

  /* This editor is normally wrapped in a cancellation editor anyway,
     so it doesn't bother to check for cancellation itself.  However,
     it needs a cancel_func and cancel_baton available to pass to
     long-running functions. */
  svn_cancel_func_t cancel_func;
  void *cancel_baton;

  /* This editor will invoke a interactive conflict-resolution
     callback, if available. */
  svn_wc_conflict_resolver_func2_t conflict_func;
  void *conflict_baton;

  /* Subtrees that were skipped during the edit, and therefore shouldn't
     have their revision/url info updated at the end.  If a path is a
     directory, its descendants will also be skipped.  The keys are paths
     relative to the working copy root and the values unspecified. */
  apr_hash_t *skipped_trees;

  /* A mapping from const char * repos_relpaths to the apr_hash_t * instances
     returned from fetch_dirents_func for that repos_relpath. These
     are used to avoid issue #3569 in specific update scenarios where a
     restricted depth is used. */
  apr_hash_t *dir_dirents;

  /* Absolute path of the working copy root or NULL if not initialized yet */
  const char *wcroot_abspath;

  apr_pool_t *pool;
};


/* Record in the edit baton EB that LOCAL_ABSPATH's base version is not being
 * updated.
 *
 * Add to EB->skipped_trees a copy (allocated in EB->pool) of the string
 * LOCAL_ABSPATH.
 */
static svn_error_t *
remember_skipped_tree(struct edit_baton *eb,
                      const char *local_abspath,
                      apr_pool_t *scratch_pool)
{
  SVN_ERR_ASSERT(svn_dirent_is_absolute(local_abspath));

  apr_hash_set(eb->skipped_trees,
               apr_pstrdup(eb->pool,
                           svn_dirent_skip_ancestor(eb->wcroot_abspath,
                                                    local_abspath)),
               APR_HASH_KEY_STRING, (void*)1);

  return SVN_NO_ERROR;
}

struct dir_baton
{
  /* Basename of this directory. */
  const char *name;

  /* Absolute path of this directory */
  const char *local_abspath;

  /* The repository relative path this directory will correspond to. */
  const char *new_relpath;

  /* The revision of the directory before updating */
  svn_revnum_t old_revision;

  /* The global edit baton. */
  struct edit_baton *edit_baton;

  /* Baton for this directory's parent, or NULL if this is the root
     directory. */
  struct dir_baton *parent_baton;

  /* Set if updates to this directory are skipped */
  svn_boolean_t skip_this;

  /* Set if there was a previous notification for this directory */
  svn_boolean_t already_notified;

  /* Set if this directory is being added during this editor drive. */
  svn_boolean_t adding_dir;

  /* Set on a node and its descendants are not present in the working copy
     but should still be updated (not skipped). These nodes should all be
     marked as deleted. */
  svn_boolean_t shadowed;

  /* The (new) changed_* information, cached to avoid retrieving it later */
  svn_revnum_t changed_rev;
  apr_time_t changed_date;
  const char *changed_author;

  /* If not NULL, contains a mapping of const char* basenames of children that
     have been deleted to their svn_wc_conflict_description2_t* tree conflicts.
     We store this hash to allow replacements to continue under a just
     installed tree conflict.

     The add after the delete will then update the tree conflicts information
     and reinstall it. */
  apr_hash_t *deletion_conflicts;

  /* A hash of file names (only the hash key matters) seen by add_file
     and not yet added to the database by close_file. */
  apr_hash_t *not_present_files;

  /* Set if an unversioned dir of the same name already existed in
     this directory. */
  svn_boolean_t obstruction_found;

  /* Set if a dir of the same name already exists and is
     scheduled for addition without history. */
  svn_boolean_t add_existed;

  /* An array of svn_prop_t structures, representing all the property
     changes to be applied to this directory. */
  apr_array_header_t *propchanges;

  /* A boolean indicating whether this node or one of its children has
     received any 'real' changes. Used to avoid tree conflicts for simple
     entryprop changes, like lock management */
  svn_boolean_t edited;

  /* The tree conflict to install once the node is really edited */
  svn_wc_conflict_description2_t *edit_conflict;

  /* The bump information for this directory. */
  struct bump_dir_info *bump_info;

  /* The depth of the directory in the wc (or inferred if added).  Not
     used for filtering; we have a separate wrapping editor for that. */
  svn_depth_t ambient_depth;

  /* Was the directory marked as incomplete before the update?
     (In other words, are we resuming an interrupted update?)

     If WAS_INCOMPLETE is set to TRUE we expect to receive all child nodes
     and properties for/of the directory. If WAS_INCOMPLETE is FALSE then
     we only receive the changes in/for children and properties.*/
  svn_boolean_t was_incomplete;

  /* The pool in which this baton itself is allocated. */
  apr_pool_t *pool;
};


/* The bump information is tracked separately from the directory batons.
   This is a small structure kept in the edit pool, while the heavier
   directory baton is managed by the editor driver.

   In a postfix delta case, the directory batons are going to disappear.
   The files will refer to these structures, rather than the full
   directory baton.  */
struct bump_dir_info
{
  /* ptr to the bump information for the parent directory */
  struct bump_dir_info *parent;

  /* how many entries are referring to this bump information? */
  int ref_count;

  /* Pool that should be cleared after the dir is bumped */
  apr_pool_t *pool;
};


struct handler_baton
{
  svn_txdelta_window_handler_t apply_handler;
  void *apply_baton;
  apr_pool_t *pool;
  struct file_baton *fb;

  /* Where we are assembling the new file. */
  const char *new_text_base_tmp_abspath;

    /* The expected source checksum of the text source or NULL if no base
     checksum is available (MD5 if the server provides a checksum, SHA1 if
     the server doesn't) */
  svn_checksum_t *expected_source_checksum;

  /* Why two checksums?
     The editor currently provides an md5 which we use to detect corruption
     during transmission.  We use the sha1 inside libsvn_wc both for pristine
     handling and corruption detection.  In the future, the editor will also
     provide a sha1, so we may not have to calculate both, but for the time
     being, that's the way it is. */

  /* The calculated checksum of the text source or NULL if the acual
     checksum is not being calculated. The checksum kind is identical to the
     kind of expected_source_checksum. */
  svn_checksum_t *actual_source_checksum;

  /* The stream used to calculate the source checksums */
  svn_stream_t *source_checksum_stream;

  /* A calculated MD5 digest of NEW_TEXT_BASE_TMP_ABSPATH.
     This is initialized to all zeroes when the baton is created, then
     populated with the MD5 digest of the resultant fulltext after the
     last window is handled by the handler returned from
     apply_textdelta(). */
  unsigned char new_text_base_md5_digest[APR_MD5_DIGESTSIZE];

  /* A calculated SHA-1 of NEW_TEXT_BASE_TMP_ABSPATH, which we'll use for
     eventually writing the pristine. */
  svn_checksum_t * new_text_base_sha1_checksum;
};


/* Get an empty file in the temporary area for WRI_ABSPATH.  The file will
   not be set for automatic deletion, and the name will be returned in
   TMP_FILENAME.

   This implementation creates a new empty file with a unique name.

   ### This is inefficient for callers that just want an empty file to read
   ### from.  There could be (and there used to be) a permanent, shared
   ### empty file for this purpose.

   ### This is inefficient for callers that just want to reserve a unique
   ### file name to create later.  A better way may not be readily available.
 */
static svn_error_t *
get_empty_tmp_file(const char **tmp_filename,
                   svn_wc__db_t *db,
                   const char *wri_abspath,
                   apr_pool_t *result_pool,
                   apr_pool_t *scratch_pool)
{
  const char *temp_dir_abspath;

  SVN_ERR(svn_wc__db_temp_wcroot_tempdir(&temp_dir_abspath, db, wri_abspath,
                                         scratch_pool, scratch_pool));
  SVN_ERR(svn_io_open_unique_file3(NULL, tmp_filename, temp_dir_abspath,
                                   svn_io_file_del_none,
                                   scratch_pool, scratch_pool));

  return SVN_NO_ERROR;
}

/* An APR pool cleanup handler.  This runs the working queue for an
   editor baton. */
static apr_status_t
cleanup_edit_baton(void *edit_baton)
{
  struct edit_baton *eb = edit_baton;
  svn_error_t *err;
  apr_pool_t *pool = apr_pool_parent_get(eb->pool);

  err = svn_wc__wq_run(eb->db, eb->wcroot_abspath,
                       NULL /* cancel_func */, NULL /* cancel_baton */,
                       pool);

  if (err)
    {
      apr_status_t apr_err = err->apr_err;
      svn_error_clear(err);
      return apr_err;
    }
  return APR_SUCCESS;
}

/* An APR pool cleanup handler.  This is a child handler, it removes
   the mail pool handler.
   <stsp> mail pool?
   <hwright> that's where the missing commit mails are going!  */
static apr_status_t
cleanup_edit_baton_child(void *edit_baton)
{
  struct edit_baton *eb = edit_baton;
  apr_pool_cleanup_kill(eb->pool, eb, cleanup_edit_baton);
  return APR_SUCCESS;
}


/* Make a new dir baton in a subpool of PB->pool. PB is the parent baton.
   If PATH and PB are NULL, this is the root directory of the edit; in this
   case, make the new dir baton in a subpool of EB->pool.
   ADDING should be TRUE if we are adding this directory.  */
static svn_error_t *
make_dir_baton(struct dir_baton **d_p,
               const char *path,
               struct edit_baton *eb,
               struct dir_baton *pb,
               svn_boolean_t adding,
               apr_pool_t *scratch_pool)
{
  apr_pool_t *dir_pool;
  struct dir_baton *d;
  struct bump_dir_info *bdi;

  if (pb != NULL)
    dir_pool = svn_pool_create(pb->pool);
  else
    dir_pool = svn_pool_create(eb->pool);

  SVN_ERR_ASSERT(path || (! pb));

  /* Okay, no easy out, so allocate and initialize a dir baton. */
  d = apr_pcalloc(dir_pool, sizeof(*d));

  /* Construct the PATH and baseNAME of this directory. */
  if (path)
    {
      d->name = svn_dirent_basename(path, dir_pool);
      SVN_ERR(path_join_under_root(&d->local_abspath,
                                   pb->local_abspath, d->name, dir_pool));
    }
  else
    {
      /* This is the root baton. */
      d->name = NULL;
      d->local_abspath = eb->anchor_abspath;
    }

  /* Figure out the new_relpath for this directory. */
  if (eb->switch_relpath)
    {
      /* Handle switches... */

      if (pb == NULL)
        {
          if (*eb->target_basename == '\0')
            {
              /* No parent baton and target_basename=="" means that we are
                 the target of the switch. Thus, our NEW_RELPATH will be
                 the SWITCH_RELPATH.  */
              d->new_relpath = eb->switch_relpath;
            }
          else
            {
              /* This node is NOT the target of the switch (one of our
                 children is the target); therefore, it must already exist.
                 Get its old REPOS_RELPATH, as it won't be changing.  */
              SVN_ERR(svn_wc__db_scan_base_repos(&d->new_relpath, NULL, NULL,
                                                 eb->db, d->local_abspath,
                                                 dir_pool, scratch_pool));
            }
        }
      else
        {
          /* This directory is *not* the root (has a parent). If there is
             no grandparent, then we may have anchored at the parent,
             and self is the target. If we match the target, then set
             NEW_RELPATH to the SWITCH_RELPATH.

             Otherwise, we simply extend NEW_RELPATH from the parent.  */
          if (pb->parent_baton == NULL
              && strcmp(eb->target_basename, d->name) == 0)
            d->new_relpath = eb->switch_relpath;
          else
            d->new_relpath = svn_relpath_join(pb->new_relpath, d->name,
                                              dir_pool);
        }
    }
  else  /* must be an update */
    {
      /* If we are adding the node, then simply extend the parent's
         relpath for our own.  */
      if (adding)
        {
          SVN_ERR_ASSERT(pb != NULL);
          d->new_relpath = svn_relpath_join(pb->new_relpath, d->name,
                                            dir_pool);
        }
      else
        {
          SVN_ERR(svn_wc__db_scan_base_repos(&d->new_relpath, NULL, NULL,
                                             eb->db, d->local_abspath,
                                             dir_pool, scratch_pool));
          SVN_ERR_ASSERT(d->new_relpath);
        }
    }

  /* the bump information lives in the edit pool */
  bdi = apr_pcalloc(dir_pool, sizeof(*bdi));
  bdi->parent = pb ? pb->bump_info : NULL;
  bdi->ref_count = 1;
  bdi->pool = dir_pool;

  /* the parent's bump info has one more referer */
  if (pb)
    ++bdi->parent->ref_count;

  d->edit_baton   = eb;
  d->parent_baton = pb;
  d->pool         = dir_pool;
  d->propchanges  = apr_array_make(dir_pool, 1, sizeof(svn_prop_t));
  d->obstruction_found = FALSE;
  d->add_existed  = FALSE;
  d->bump_info    = bdi;
  d->old_revision = SVN_INVALID_REVNUM;
  d->adding_dir   = adding;
  d->changed_rev  = SVN_INVALID_REVNUM;
  d->not_present_files = apr_hash_make(dir_pool);

  /* Copy some flags from the parent baton */
  if (pb)
    {
      d->skip_this = pb->skip_this;
      d->shadowed = pb->shadowed;
    }

  /* The caller of this function needs to fill these in. */
  d->ambient_depth = svn_depth_unknown;
  d->was_incomplete = FALSE;

  *d_p = d;
  return SVN_NO_ERROR;
}


/* Forward declarations. */
static svn_error_t *
already_in_a_tree_conflict(svn_boolean_t *conflicted,
                           svn_wc__db_t *db,
                           const char *local_abspath,
                           apr_pool_t *scratch_pool);


static void
do_notification(const struct edit_baton *eb,
                const char *local_abspath,
                svn_node_kind_t kind,
                svn_wc_notify_action_t action,
                apr_pool_t *scratch_pool)
{
  svn_wc_notify_t *notify;

  if (eb->notify_func == NULL)
    return;

  notify = svn_wc_create_notify(local_abspath, action, scratch_pool);
  notify->kind = kind;

  (*eb->notify_func)(eb->notify_baton, notify, scratch_pool);
}

/* Decrement the bump_dir_info's reference count. If it hits zero,
   then this directory is "done". This means it is safe to clear its pool.

   In addition, when the directory is "done", we loop onto the parent's
   bump information to possibly mark it as done, too.
*/
static svn_error_t *
maybe_release_dir_info(struct bump_dir_info *bdi)
{
  /* Keep moving up the tree of directories until we run out of parents,
     or a directory is not yet "done".  */
  while (bdi != NULL)
    {
      apr_pool_t *destroy_pool;

      if (--bdi->ref_count > 0)
        break;  /* directory isn't done yet */

      destroy_pool = bdi->pool;
      bdi = bdi->parent;

      svn_pool_destroy(destroy_pool);
    }
  /* we exited the for loop because there are no more parents */

  return SVN_NO_ERROR;
}

struct file_baton
{
  /* Pool specific to this file_baton. */
  apr_pool_t *pool;

  /* Name of this file (its entry in the directory). */
  const char *name;

  /* Absolute path to this file */
  const char *local_abspath;

  /* The repository relative path this file will correspond to. */
  const char *new_relpath;

  /* The revision of the file before updating */
  svn_revnum_t old_revision;

  /* The global edit baton. */
  struct edit_baton *edit_baton;

  /* The parent directory of this file. */
  struct dir_baton *dir_baton;

  /* Set if updates to this directory are skipped */
  svn_boolean_t skip_this;

  /* Set if there was a previous notification  */
  svn_boolean_t already_notified;

  /* Set if this file is new. */
  svn_boolean_t adding_file;

  /* Set if an unversioned file of the same name already existed in
     this directory. */
  svn_boolean_t obstruction_found;

  /* Set if a file of the same name already exists and is
     scheduled for addition without history. */
  svn_boolean_t add_existed;

  /* Set if this file is being added in the BASE layer, but is not-present
     in the working copy (replaced, deleted, etc.). */
  svn_boolean_t shadowed;

  /* The (new) changed_* information, cached to avoid retrieving it later */
  svn_revnum_t changed_rev;
  apr_time_t changed_date;
  const char *changed_author;

  /* If there are file content changes, these are the checksums of the
     resulting new text base, which is in the pristine store, else NULL. */
  const svn_checksum_t *new_text_base_md5_checksum;
  const svn_checksum_t *new_text_base_sha1_checksum;

  /* The checksum of the file before the update */
  const svn_checksum_t *original_checksum;

  /* An array of svn_prop_t structures, representing all the property
     changes to be applied to this file.  Once a file baton is
     initialized, this is never NULL, but it may have zero elements.  */
  apr_array_header_t *propchanges;

  /* For existing files, whether there are local modifications. FALSE for added
     files */
  svn_boolean_t local_prop_mods;

  /* Bump information for the directory this file lives in */
  struct bump_dir_info *bump_info;

  /* A boolean indicating whether this node or one of its children has
     received any 'real' changes. Used to avoid tree conflicts for simple
     entryprop changes, like lock management */
  svn_boolean_t edited;

  /* The tree conflict to install once the node is really edited */
  svn_wc_conflict_description2_t *edit_conflict;
};


/* Make a new file baton in a subpool of PB->pool. PB is the parent baton.
 * PATH is relative to the root of the edit. ADDING tells whether this file
 * is being added. */
static svn_error_t *
make_file_baton(struct file_baton **f_p,
                struct dir_baton *pb,
                const char *path,
                svn_boolean_t adding,
                apr_pool_t *scratch_pool)
{
  struct edit_baton *eb = pb->edit_baton;
  apr_pool_t *file_pool = svn_pool_create(pb->pool);

  struct file_baton *f = apr_pcalloc(file_pool, sizeof(*f));

  SVN_ERR_ASSERT(path);

  /* Make the file's on-disk name. */
  f->name = svn_dirent_basename(path, file_pool);
  f->old_revision = SVN_INVALID_REVNUM;
  SVN_ERR(path_join_under_root(&f->local_abspath,
                               pb->local_abspath, f->name, file_pool));

  /* Figure out the new URL for this file. */
  if (eb->switch_relpath)
    {
      /* Handle switches... */

      /* This file has a parent directory. If there is
         no grandparent, then we may have anchored at the parent,
         and self is the target. If we match the target, then set
         NEW_RELPATH to the SWITCH_RELPATH.

         Otherwise, we simply extend NEW_RELPATH from the parent.  */
      if (pb->parent_baton == NULL
          && strcmp(eb->target_basename, f->name) == 0)
        f->new_relpath = eb->switch_relpath;
      else
        f->new_relpath = svn_relpath_join(pb->new_relpath, f->name,
                                          file_pool);
    }
  else  /* must be an update */
    {
      if (adding)
        f->new_relpath = svn_relpath_join(pb->new_relpath, f->name, file_pool);
      else
        {
          SVN_ERR(svn_wc__db_scan_base_repos(&f->new_relpath, NULL, NULL,
                                             eb->db, f->local_abspath,
                                             file_pool, scratch_pool));
          SVN_ERR_ASSERT(f->new_relpath);
        }
    }

  f->pool              = file_pool;
  f->edit_baton        = pb->edit_baton;
  f->propchanges       = apr_array_make(file_pool, 1, sizeof(svn_prop_t));
  f->bump_info         = pb->bump_info;
  f->adding_file       = adding;
  f->obstruction_found = FALSE;
  f->add_existed       = FALSE;
  f->skip_this         = pb->skip_this;
  f->shadowed          = pb->shadowed;
  f->dir_baton         = pb;
  f->changed_rev       = SVN_INVALID_REVNUM;

  /* the directory's bump info has one more referer now */
  ++f->bump_info->ref_count;

  *f_p = f;
  return SVN_NO_ERROR;
}

/* Called when a directory is really edited, to avoid marking a
   tree conflict on a node for a no-change edit */
static svn_error_t *
mark_directory_edited(struct dir_baton *db, apr_pool_t *scratch_pool)
{
  if (db->edited)
    return SVN_NO_ERROR;

  if (db->parent_baton)
    SVN_ERR(mark_directory_edited(db->parent_baton, scratch_pool));

  db->edited = TRUE;

  if (db->edit_conflict)
    {
      /* We have a (delayed) tree conflict to install */
      SVN_ERR(svn_wc__db_op_set_tree_conflict(db->edit_baton->db,
                                              db->local_abspath,
                                              db->edit_conflict,
                                              scratch_pool));

      do_notification(db->edit_baton, db->local_abspath, svn_node_dir,
                      svn_wc_notify_tree_conflict, scratch_pool);
      db->already_notified = TRUE;
    }

  return SVN_NO_ERROR;
}

/* Called when a file is really edited, to avoid marking a
   tree conflict on a node for a no-change edit */
static svn_error_t *
mark_file_edited(struct file_baton *fb, apr_pool_t *scratch_pool)
{
  if (fb->edited)
    return SVN_NO_ERROR;

  SVN_ERR(mark_directory_edited(fb->dir_baton, scratch_pool));

  fb->edited = TRUE;

  if (fb->edit_conflict)
    {
      /* We have a (delayed) tree conflict to install */
      SVN_ERR(svn_wc__db_op_set_tree_conflict(fb->edit_baton->db,
                                              fb->local_abspath,
                                              fb->edit_conflict,
                                              scratch_pool));

      do_notification(fb->edit_baton, fb->local_abspath, svn_node_file,
                      svn_wc_notify_tree_conflict, scratch_pool);
      fb->already_notified = TRUE;
    }

  return SVN_NO_ERROR;
}


/* Handle the next delta window of the file described by BATON.  If it is
 * the end (WINDOW == NULL), then check the checksum, store the text in the
 * pristine store and write its details into BATON->fb->new_text_base_*. */
static svn_error_t *
window_handler(svn_txdelta_window_t *window, void *baton)
{
  struct handler_baton *hb = baton;
  struct file_baton *fb = hb->fb;
  svn_wc__db_t *db = fb->edit_baton->db;
  svn_error_t *err;

  /* Apply this window.  We may be done at that point.  */
  err = hb->apply_handler(window, hb->apply_baton);
  if (window != NULL && !err)
    return SVN_NO_ERROR;

  if (hb->expected_source_checksum)
    {
      /* Close the stream to calculate HB->actual_source_md5_checksum. */
      svn_error_t *err2 = svn_stream_close(hb->source_checksum_stream);

      if (!err2)
        {
          SVN_ERR_ASSERT(hb->expected_source_checksum->kind ==
                        hb->actual_source_checksum->kind);

          if (!svn_checksum_match(hb->expected_source_checksum,
                                  hb->actual_source_checksum))
            {
              err = svn_error_createf(SVN_ERR_WC_CORRUPT_TEXT_BASE, err,
                        _("Checksum mismatch while updating '%s':\n"
                          "   expected:  %s\n"
                          "     actual:  %s\n"),
                        svn_dirent_local_style(fb->local_abspath, hb->pool),
                        svn_checksum_to_cstring(hb->expected_source_checksum,
                                                hb->pool),
                        svn_checksum_to_cstring(hb->actual_source_checksum,
                                                hb->pool));
            }
        }

      err = svn_error_compose_create(err, err2);
    }

  if (err)
    {
      /* We failed to apply the delta; clean up the temporary file.  */
      svn_error_clear(svn_io_remove_file2(hb->new_text_base_tmp_abspath, TRUE,
                                          hb->pool));
    }
  else
    {
      /* Tell the file baton about the new text base's checksums. */
      fb->new_text_base_md5_checksum =
        svn_checksum__from_digest(hb->new_text_base_md5_digest,
                                  svn_checksum_md5, fb->pool);
      fb->new_text_base_sha1_checksum =
        svn_checksum_dup(hb->new_text_base_sha1_checksum, fb->pool);

      /* Store the new pristine text in the pristine store now.  Later, in a
         single transaction we will update the BASE_NODE to include a
         reference to this pristine text's checksum. */
      SVN_ERR(svn_wc__db_pristine_install(db, hb->new_text_base_tmp_abspath,
                                          fb->new_text_base_sha1_checksum,
                                          fb->new_text_base_md5_checksum,
                                          hb->pool));
    }

  svn_pool_destroy(hb->pool);

  return err;
}


/* Find the last-change info within ENTRY_PROPS, and return then in the
   CHANGED_* parameters. Each parameter will be initialized to its "none"
   value, and will contain the relavent info if found.

   CHANGED_AUTHOR will be allocated in RESULT_POOL. SCRATCH_POOL will be
   used for some temporary allocations.
*/
static svn_error_t *
accumulate_last_change(svn_revnum_t *changed_rev,
                       apr_time_t *changed_date,
                       const char **changed_author,
                       const apr_array_header_t *entry_props,
                       apr_pool_t *result_pool,
                       apr_pool_t *scratch_pool)
{
  int i;

  *changed_rev = SVN_INVALID_REVNUM;
  *changed_date = 0;
  *changed_author = NULL;

  for (i = 0; i < entry_props->nelts; ++i)
    {
      const svn_prop_t *prop = &APR_ARRAY_IDX(entry_props, i, svn_prop_t);

      /* A prop value of NULL means the information was not
         available.  We don't remove this field from the entries
         file; we have convention just leave it empty.  So let's
         just skip those entry props that have no values. */
      if (! prop->value)
        continue;

      if (! strcmp(prop->name, SVN_PROP_ENTRY_LAST_AUTHOR))
        *changed_author = apr_pstrdup(result_pool, prop->value->data);
      else if (! strcmp(prop->name, SVN_PROP_ENTRY_COMMITTED_REV))
        {
          apr_int64_t rev;
          SVN_ERR(svn_cstring_atoi64(&rev, prop->value->data));
          *changed_rev = (svn_revnum_t)rev;
        }
      else if (! strcmp(prop->name, SVN_PROP_ENTRY_COMMITTED_DATE))
        SVN_ERR(svn_time_from_cstring(changed_date, prop->value->data,
                                      scratch_pool));

      /* Starting with Subversion 1.7 we ignore the SVN_PROP_ENTRY_UUID
         property here. */
    }

  return SVN_NO_ERROR;
}


/* Join ADD_PATH to BASE_PATH.  If ADD_PATH is absolute, or if any ".."
 * component of it resolves to a path above BASE_PATH, then return
 * SVN_ERR_WC_OBSTRUCTED_UPDATE.
 *
 * This is to prevent the situation where the repository contains,
 * say, "..\nastyfile".  Although that's perfectly legal on some
 * systems, when checked out onto Win32 it would cause "nastyfile" to
 * be created in the parent of the current edit directory.
 *
 * (http://cve.mitre.org/cgi-bin/cvename.cgi?name=2007-3846)
 */
static svn_error_t *
path_join_under_root(const char **result_path,
                     const char *base_path,
                     const char *add_path,
                     apr_pool_t *pool)
{
  svn_boolean_t under_root;

  SVN_ERR(svn_dirent_is_under_root(&under_root,
                                   result_path, base_path, add_path, pool));

  if (! under_root)
    {
      return svn_error_createf(
          SVN_ERR_WC_OBSTRUCTED_UPDATE, NULL,
          _("Path '%s' is not in the working copy"),
          svn_dirent_local_style(svn_dirent_join(base_path, add_path, pool),
                                 pool));
    }

  return SVN_NO_ERROR;
}


/*** The callbacks we'll plug into an svn_delta_editor_t structure. ***/

/* An svn_delta_editor_t function. */
static svn_error_t *
set_target_revision(void *edit_baton,
                    svn_revnum_t target_revision,
                    apr_pool_t *pool)
{
  struct edit_baton *eb = edit_baton;

  *(eb->target_revision) = target_revision;
  return SVN_NO_ERROR;
}

/* An svn_delta_editor_t function. */
static svn_error_t *
open_root(void *edit_baton,
          svn_revnum_t base_revision, /* This is ignored in co */
          apr_pool_t *pool,
          void **dir_baton)
{
  struct edit_baton *eb = edit_baton;
  struct dir_baton *db;
  svn_boolean_t already_conflicted;
  svn_error_t *err;

  /* Note that something interesting is actually happening in this
     edit run. */
  eb->root_opened = TRUE;

  SVN_ERR(make_dir_baton(&db, NULL, eb, NULL, FALSE, pool));
  *dir_baton = db;

  err = already_in_a_tree_conflict(&already_conflicted, eb->db,
                                   db->local_abspath, pool);

  if (err)
    {
      if (err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND)
        return svn_error_trace(err);

      svn_error_clear(err);
      already_conflicted = FALSE;
    }
  else if (already_conflicted)
    {
      db->skip_this = TRUE;
      db->already_notified = TRUE;

      /* Notify that we skipped the target, while we actually skipped
         the anchor */
      do_notification(eb, eb->target_abspath, svn_node_unknown,
                      svn_wc_notify_skip_conflicted, pool);

      return SVN_NO_ERROR;
    }

  if (*eb->target_basename == '\0')
    {
      /* For an update with a NULL target, this is equivalent to open_dir(): */
      svn_wc__db_status_t status;

      /* Read the depth from the entry. */
      SVN_ERR(svn_wc__db_base_get_info(&status, NULL, NULL, NULL, NULL, NULL,
                                       &db->changed_rev, &db->changed_date,
                                       &db->changed_author, &db->ambient_depth,
                                       NULL, NULL, NULL, NULL, NULL,
                                       eb->db, db->local_abspath,
                                       db->pool, pool));
      db->was_incomplete = (status == svn_wc__db_status_incomplete);

      SVN_ERR(svn_wc__db_temp_op_start_directory_update(eb->db,
                                                        db->local_abspath,
                                                        db->new_relpath,
                                                        *eb->target_revision,
                                                        pool));
    }

  return SVN_NO_ERROR;
}


/* ===================================================================== */
/* Checking for local modifications. */

/* A baton for use with modcheck_found_entry(). */
typedef struct modcheck_baton_t {
  svn_wc__db_t *db;         /* wc_db to access nodes */
  svn_boolean_t found_mod;  /* whether a modification has been found */
  svn_boolean_t found_not_delete;  /* Found a not-delete modification */
} modcheck_baton_t;

/* An implementation of svn_wc_status_func4_t. */
static svn_error_t *
modcheck_callback(void *baton,
                  const char *local_abspath,
                  const svn_wc_status3_t *status,
                  apr_pool_t *scratch_pool)
{
  modcheck_baton_t *mb = baton;

  switch (status->node_status)
    {
      case svn_wc_status_normal:
      case svn_wc_status_incomplete:
      case svn_wc_status_ignored:
      case svn_wc_status_none:
      case svn_wc_status_unversioned:
      case svn_wc_status_external:
        break;

      case svn_wc_status_deleted:
        mb->found_mod = TRUE;
        break;

      case svn_wc_status_missing:
      case svn_wc_status_obstructed:
        if (status->prop_status != svn_wc_status_modified)
          break;
        /* Fall through in the found modification case */

      default:
      case svn_wc_status_added:
      case svn_wc_status_replaced:
      case svn_wc_status_modified:
        mb->found_mod = TRUE;
        mb->found_not_delete = TRUE;
        /* Exit from the status walker: We know what we want to know */
        return svn_error_create(SVN_ERR_CEASE_INVOCATION, NULL, NULL);
    }

  return SVN_NO_ERROR;
}


/* Set *MODIFIED to true iff there are any local modifications within the
 * tree rooted at LOCAL_ABSPATH, using DB. If *MODIFIED
 * is set to true and all the local modifications were deletes then set
 * *ALL_EDITS_ARE_DELETES to true, set it to false otherwise.  LOCAL_ABSPATH
 * may be a file or a directory. */
static svn_error_t *
node_has_local_mods(svn_boolean_t *modified,
                    svn_boolean_t *all_edits_are_deletes,
                    svn_wc__db_t *db,
                    const char *local_abspath,
                    svn_cancel_func_t cancel_func,
                    void *cancel_baton,
                    apr_pool_t *scratch_pool)
{
  modcheck_baton_t modcheck_baton = { NULL, FALSE, FALSE };
  svn_error_t *err;

  modcheck_baton.db = db;

  /* Walk the WC tree for status with depth infinity, looking for any local
   * modifications. If it's a "sparse" directory, that's OK: there can be
   * no local mods in the pieces that aren't present in the WC. */

  err = svn_wc__internal_walk_status(db, local_abspath,
                                     svn_depth_infinity,
                                     FALSE, FALSE, FALSE, NULL,
                                     modcheck_callback, &modcheck_baton,
                                     cancel_func, cancel_baton,
                                     scratch_pool);

  if (err && err->apr_err == SVN_ERR_CEASE_INVOCATION)
    svn_error_clear(err);
  else
    SVN_ERR(err);

  *modified = modcheck_baton.found_mod;
  *all_edits_are_deletes = !modcheck_baton.found_not_delete;

  return SVN_NO_ERROR;
}


/* Indicates an unset svn_wc_conflict_reason_t. */
#define SVN_WC_CONFLICT_REASON_NONE (svn_wc_conflict_reason_t)(-1)

/* Create a tree conflict struct.
 *
 * The REASON is stored directly in the tree conflict info.
 *
 * All temporary allocactions are be made in SCRATCH_POOL, while allocations
 * needed for the returned conflict struct are made in RESULT_POOL.
 *
 * All other parameters are identical to and described by
 * check_tree_conflict(), with the slight modification that this function
 * relies on the reason passed in REASON instead of actively looking for one. */
static svn_error_t *
create_tree_conflict(svn_wc_conflict_description2_t **pconflict,
                     struct edit_baton *eb,
                     const char *local_abspath,
                     svn_wc_conflict_reason_t reason,
                     svn_wc_conflict_action_t action,
                     svn_node_kind_t their_node_kind,
                     const char *their_relpath,
                     apr_pool_t *result_pool, apr_pool_t *scratch_pool)
{
  const char *repos_root_url = NULL;
  const char *left_repos_relpath;
  svn_revnum_t left_revision;
  svn_node_kind_t left_kind;
  const char *right_repos_relpath;
  const char *added_repos_relpath = NULL;
  svn_node_kind_t conflict_node_kind;
  svn_wc_conflict_version_t *src_left_version;
  svn_wc_conflict_version_t *src_right_version;

  *pconflict = NULL;

  SVN_ERR_ASSERT(reason != SVN_WC_CONFLICT_REASON_NONE);
  SVN_ERR_ASSERT(their_relpath != NULL);

  /* Get the source-left information, i.e. the local state of the node
   * before any changes were made to the working copy, i.e. the state the
   * node would have if it was reverted. */
  if (reason == svn_wc_conflict_reason_added)
    {
      svn_wc__db_status_t added_status;

      /* ###TODO: It would be nice to tell the user at which URL and
       * ### revision source-left was empty, which could be quite difficult
       * ### to code, and is a slight theoretical leap of the svn mind.
       * ### Update should show
       * ###   URL: svn_wc__db_scan_addition( &repos_relpath )
       * ###   REV: The base revision of the parent of before this update
       * ###        started
       * ###        ### BUT what if parent was updated/switched away with
       * ###        ### depth=empty after this node was added?
       * ### Switch should show
       * ###   URL: scan_addition URL of before this switch started
       * ###   REV: same as above */

      /* In case of a local addition, source-left is non-existent / empty. */
      left_kind = svn_node_none;
      left_revision = SVN_INVALID_REVNUM;
      left_repos_relpath = NULL;

      /* Still get the repository root needed by both 'update' and 'switch',
       * and the would-be repos_relpath needed to construct the source-right
       * in case of an 'update'. Check sanity while we're at it. */
      SVN_ERR(svn_wc__db_scan_addition(&added_status, NULL,
                                       &added_repos_relpath,
                                       &repos_root_url,
                                       NULL, NULL, NULL, NULL, NULL,
                                       eb->db, local_abspath,
                                       result_pool, scratch_pool));

      /* This better really be an added status. */
      SVN_ERR_ASSERT(added_status == svn_wc__db_status_added
                     || added_status == svn_wc__db_status_copied
                     || added_status == svn_wc__db_status_moved_here);
    }
  else if (reason == svn_wc_conflict_reason_unversioned)
    {
      /* Obstructed by an unversioned node. Source-left is
       * non-existent/empty. */
      left_kind = svn_node_none;
      left_revision = SVN_INVALID_REVNUM;
      left_repos_relpath = NULL;
      repos_root_url = eb->repos_root;
    }
  else
    {
      /* A BASE node should exist. */
      svn_wc__db_kind_t base_kind;

      /* If anything else shows up, then this assertion is probably naive
       * and that other case should also be handled. */
      SVN_ERR_ASSERT(reason == svn_wc_conflict_reason_edited
                     || reason == svn_wc_conflict_reason_deleted
                     || reason == svn_wc_conflict_reason_replaced
                     || reason == svn_wc_conflict_reason_obstructed);

      SVN_ERR(svn_wc__db_base_get_info(NULL, &base_kind,
                                       &left_revision,
                                       &left_repos_relpath,
                                       &repos_root_url,
                                       NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL, NULL, NULL,
                                       eb->db,
                                       local_abspath,
                                       result_pool,
                                       scratch_pool));
      /* Translate the node kind. */
      if (base_kind == svn_wc__db_kind_file
          || base_kind == svn_wc__db_kind_symlink)
        left_kind = svn_node_file;
      else if (base_kind == svn_wc__db_kind_dir)
        left_kind = svn_node_dir;
      else
        SVN_ERR_MALFUNCTION();
    }

  SVN_ERR_ASSERT(strcmp(repos_root_url, eb->repos_root) == 0);

  /* Find the source-right information, i.e. the state in the repository
   * to which we would like to update. */
  if (eb->switch_relpath)
    {
      /* This is a 'switch' operation. */
      right_repos_relpath = their_relpath;
    }
  else
    {
      /* This is an 'update', so REPOS_RELPATH would be the same as for
       * source-left. However, we don't have a source-left for locally
       * added files. */
      right_repos_relpath = (reason == svn_wc_conflict_reason_added ?
                             added_repos_relpath : left_repos_relpath);
      if (! right_repos_relpath)
        right_repos_relpath = their_relpath;
    }

  SVN_ERR_ASSERT(right_repos_relpath != NULL);

  /* Determine PCONFLICT's overall node kind, which is not allowed to be
   * svn_node_none. We give it the source-right revision (THEIR_NODE_KIND)
   * -- unless source-right is deleted and hence == svn_node_none, in which
   * case we take it from source-left, which has to be the node kind that
   * was deleted. */
  conflict_node_kind = (action == svn_wc_conflict_action_delete ?
                        left_kind : their_node_kind);
  SVN_ERR_ASSERT(conflict_node_kind == svn_node_file
                 || conflict_node_kind == svn_node_dir);


  /* Construct the tree conflict info structs. */

  if (left_repos_relpath == NULL)
    /* A locally added or unversioned path in conflict with an incoming add.
     * Send an 'empty' left revision. */
    src_left_version = NULL;
  else
    src_left_version = svn_wc_conflict_version_create(repos_root_url,
                                                      left_repos_relpath,
                                                      left_revision,
                                                      left_kind,
                                                      result_pool);

  src_right_version = svn_wc_conflict_version_create(repos_root_url,
                                                     right_repos_relpath,
                                                     *eb->target_revision,
                                                     their_node_kind,
                                                     result_pool);

  *pconflict = svn_wc_conflict_description_create_tree2(
                   local_abspath, conflict_node_kind,
                   eb->switch_relpath ?
                     svn_wc_operation_switch : svn_wc_operation_update,
                   src_left_version, src_right_version, result_pool);
  (*pconflict)->action = action;
  (*pconflict)->reason = reason;

  return SVN_NO_ERROR;
}


/* Check whether the incoming change ACTION on FULL_PATH would conflict with
 * LOCAL_ABSPATH's scheduled change. If so, then raise a tree conflict with
 * LOCAL_ABSPATH as the victim.
 *
 * The edit baton EB gives information including whether the operation is
 * an update or a switch.
 *
 * WORKING_STATUS and WORKING_KIND are the current node status of LOCAL_ABSPATH
 * and EXISTS_IN_REPOS specifies whether a BASE_NODE representation for exists
 * for this node.
 *
 * If a tree conflict reason was found for the incoming action, the resulting
 * tree conflict info is returned in *PCONFLICT. PCONFLICT must be non-NULL,
 * while *PCONFLICT is always overwritten.
 *
 * THEIR_NODE_KIND should be the node kind reflected by the incoming edit
 * function. E.g. dir_opened() should pass svn_node_dir, etc.
 * In some cases of delete, svn_node_none may be used here.
 *
 * THEIR_RELPATH should be the involved node's repository-relative path on the
 * source-right side, the side that the target should become after the update.
 * Simply put, that's the URL obtained from the node's dir_baton->new_relpath
 * or file_baton->new_relpath (but it's more complex for a delete).
 *
 * The tree conflict is allocated in RESULT_POOL. Temporary allocations use
 * SCRACTH_POOl.
 */
static svn_error_t *
check_tree_conflict(svn_wc_conflict_description2_t **pconflict,
                    struct edit_baton *eb,
                    const char *local_abspath,
                    svn_wc__db_status_t working_status,
                    svn_wc__db_kind_t working_kind,
                    svn_boolean_t exists_in_repos,
                    svn_wc_conflict_action_t action,
                    svn_node_kind_t their_node_kind,
                    const char *their_relpath,
                    apr_pool_t *result_pool,
                    apr_pool_t *scratch_pool)
{
  svn_wc_conflict_reason_t reason = SVN_WC_CONFLICT_REASON_NONE;
  svn_boolean_t locally_replaced = FALSE;
  svn_boolean_t modified = FALSE;
  svn_boolean_t all_mods_are_deletes = FALSE;

  SVN_ERR_ASSERT(their_relpath != NULL);

  *pconflict = NULL;

  /* Find out if there are any local changes to this node that may
   * be the "reason" of a tree-conflict with the incoming "action". */
  switch (working_status)
    {
      case svn_wc__db_status_added:
      case svn_wc__db_status_moved_here:
      case svn_wc__db_status_copied:
        /* Is it a replace? */
        if (exists_in_repos)
          {
            svn_wc__db_status_t base_status;
            SVN_ERR(svn_wc__db_base_get_info(&base_status, NULL, NULL,
                                             NULL, NULL, NULL, NULL, NULL,
                                             NULL, NULL, NULL, NULL, NULL,
                                             NULL, NULL,
                                             eb->db, local_abspath,
                                             scratch_pool, scratch_pool));
            if (base_status != svn_wc__db_status_not_present)
              locally_replaced = TRUE;
          }

        if (!locally_replaced)
          {
            /* The node is locally added, and it did not exist before.  This
             * is an 'update', so the local add can only conflict with an
             * incoming 'add'.  In fact, if we receive anything else than an
             * svn_wc_conflict_action_add (which includes 'added',
             * 'copied-here' and 'moved-here') during update on a node that
             * did not exist before, then something is very wrong.
             * Note that if there was no action on the node, this code
             * would not have been called in the first place. */
            SVN_ERR_ASSERT(action == svn_wc_conflict_action_add);

            reason = svn_wc_conflict_reason_added;
          }
        else
          {
            /* The node is locally replaced. */
            reason = svn_wc_conflict_reason_replaced;
          }
        break;


      case svn_wc__db_status_deleted:
        /* The node is locally deleted. */
        reason = svn_wc_conflict_reason_deleted;
        break;

      case svn_wc__db_status_incomplete:
        /* We used svn_wc__db_read_info(), so 'incomplete' means
         * - there is no node in the WORKING tree
         * - a BASE node is known to exist
         * So the node exists and is essentially 'normal'. We still need to
         * check prop and text mods, and those checks will retrieve the
         * missing information (hopefully). */
      case svn_wc__db_status_normal:
        if (action == svn_wc_conflict_action_edit)
          /* An edit onto a local edit or onto *no* local changes is no
           * tree-conflict. (It's possibly a text- or prop-conflict,
           * but we don't handle those here.) */
          return SVN_NO_ERROR;

        /* Check if the update wants to delete or replace a locally
         * modified node. */


        /* Do a deep tree detection of local changes. The update editor will
         * not visit the subdirectories of a directory that it wants to delete.
         * Therefore, we need to start a separate crawl here. */

        SVN_ERR(node_has_local_mods(&modified, &all_mods_are_deletes,
                                    eb->db, local_abspath,
                                    eb->cancel_func, eb->cancel_baton,
                                    scratch_pool));

        if (modified)
          {
            if (all_mods_are_deletes)
              reason = svn_wc_conflict_reason_deleted;
            else
              reason = svn_wc_conflict_reason_edited;
          }
        break;

      case svn_wc__db_status_server_excluded:
        /* Not allowed to view the node. Not allowed to report tree
         * conflicts. */
      case svn_wc__db_status_excluded:
        /* Locally marked as excluded. No conflicts wanted. */
      case svn_wc__db_status_not_present:
        /* A committed delete (but parent not updated). The delete is
           committed, so no conflict possible during update. */
        return SVN_NO_ERROR;

      case svn_wc__db_status_base_deleted:
        /* An internal status. Should never show up here. */
        SVN_ERR_MALFUNCTION();
        break;

    }

  if (reason == SVN_WC_CONFLICT_REASON_NONE)
    /* No conflict with the current action. */
    return SVN_NO_ERROR;


  /* Sanity checks. Note that if there was no action on the node, this function
   * would not have been called in the first place.*/
  if (reason == svn_wc_conflict_reason_edited
      || reason == svn_wc_conflict_reason_deleted
      || reason == svn_wc_conflict_reason_replaced)
    {
      /* When the node existed before (it was locally deleted, replaced or
       * edited), then 'update' cannot add it "again". So it can only send
       * _action_edit, _delete or _replace. */
    if (action != svn_wc_conflict_action_edit
        && action != svn_wc_conflict_action_delete
        && action != svn_wc_conflict_action_replace)
      return svn_error_createf(SVN_ERR_WC_FOUND_CONFLICT, NULL,
               _("Unexpected attempt to add a node at path '%s'"),
               svn_dirent_local_style(local_abspath, scratch_pool));
    }
  else if (reason == svn_wc_conflict_reason_added)
    {
      /* When the node did not exist before (it was locally added),
       * then 'update' cannot want to modify it in any way.
       * It can only send _action_add. */
      if (action != svn_wc_conflict_action_add)
        return svn_error_createf(SVN_ERR_WC_FOUND_CONFLICT, NULL,
                 _("Unexpected attempt to edit, delete, or replace "
                   "a node at path '%s'"),
                 svn_dirent_local_style(local_abspath, scratch_pool));
 
    }


  /* A conflict was detected. Append log commands to the log accumulator
   * to record it. */
  return svn_error_trace(create_tree_conflict(pconflict, eb, local_abspath,
                                              reason, action, their_node_kind,
                                              their_relpath,
                                              result_pool, scratch_pool));
}


/* If LOCAL_ABSPATH is inside a conflicted tree, set *CONFLICTED to TRUE,
 * Otherwise set *CONFLICTED to FALSE.  Use SCRATCH_POOL for temporary
 * allocations.
 */
static svn_error_t *
already_in_a_tree_conflict(svn_boolean_t *conflicted,
                           svn_wc__db_t *db,
                           const char *local_abspath,
                           apr_pool_t *scratch_pool)
{
  const char *ancestor_abspath = local_abspath;
  apr_pool_t *iterpool = svn_pool_create(scratch_pool);

  SVN_ERR_ASSERT(svn_dirent_is_absolute(local_abspath));

  *conflicted = FALSE;

  while (TRUE)
    {
      svn_boolean_t is_wc_root, has_conflict;

      svn_pool_clear(iterpool);

      SVN_ERR(svn_wc__db_read_info(NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                   NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                   NULL, NULL, NULL, NULL, NULL, NULL,
                                   &has_conflict, NULL, NULL, NULL,
                                   NULL, NULL, NULL,
                                   db, ancestor_abspath, iterpool, iterpool));

      if (has_conflict)
        {
          const svn_wc_conflict_description2_t *conflict;

          SVN_ERR(svn_wc__db_op_read_tree_conflict(&conflict, db,
                                                   ancestor_abspath,
                                                   iterpool, iterpool));

          if (conflict != NULL)
            {
              *conflicted = TRUE;
              break;
            }
        }

      SVN_ERR(svn_wc__db_is_wcroot(&is_wc_root, db, ancestor_abspath,
                                   iterpool));

      if (is_wc_root)
        break;

      ancestor_abspath = svn_dirent_dirname(ancestor_abspath, scratch_pool);
    }

  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}

/* Temporary helper until the new conflict handling is in place */
static svn_error_t *
node_already_conflicted(svn_boolean_t *conflicted,
                        svn_wc__db_t *db,
                        const char *local_abspath,
                        apr_pool_t *scratch_pool)
{
  svn_boolean_t text_conflicted, prop_conflicted, tree_conflicted;

  SVN_ERR(svn_wc__internal_conflicted_p(&text_conflicted,
                                        &prop_conflicted,
                                        &tree_conflicted,
                                        db, local_abspath,
                                        scratch_pool));

  *conflicted = (text_conflicted || prop_conflicted || tree_conflicted);
  return SVN_NO_ERROR;
}


/* An svn_delta_editor_t function. */
static svn_error_t *
delete_entry(const char *path,
             svn_revnum_t revision,
             void *parent_baton,
             apr_pool_t *pool)
{
  struct dir_baton *pb = parent_baton;
  struct edit_baton *eb = pb->edit_baton;
  const char *base = svn_relpath_basename(path, NULL);
  const char *local_abspath;
  const char *repos_relpath;
  svn_wc__db_kind_t kind, base_kind;
  svn_boolean_t conflicted;
  svn_boolean_t have_base;
  svn_boolean_t have_work;
  svn_wc_conflict_description2_t *tree_conflict = NULL;
  svn_skel_t *work_item = NULL;
  svn_wc__db_status_t status;
  svn_wc__db_status_t base_status;
  apr_pool_t *scratch_pool;
  svn_boolean_t deleting_target;

  if (pb->skip_this)
    return SVN_NO_ERROR;

  scratch_pool = svn_pool_create(pb->pool);

  SVN_ERR(mark_directory_edited(pb, scratch_pool));

  SVN_ERR(path_join_under_root(&local_abspath, pb->local_abspath, base,
                               scratch_pool));

  deleting_target =  (strcmp(local_abspath, eb->target_abspath) == 0);

  /* Detect obstructing working copies */
  {
    svn_boolean_t is_root;

    SVN_ERR(svn_wc__db_is_wcroot(&is_root, eb->db, local_abspath,
                                 scratch_pool));

    if (is_root)
      {
        /* Just skip this node; a future update will handle it */
        remember_skipped_tree(eb, local_abspath, pool);
        do_notification(eb, local_abspath, svn_node_unknown,
                        svn_wc_notify_update_skip_obstruction, scratch_pool);

        svn_pool_destroy(scratch_pool);

        return SVN_NO_ERROR;
      }
  }

  SVN_ERR(svn_wc__db_read_info(&status, &kind, NULL, &repos_relpath, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, &conflicted,
                               NULL, NULL, NULL,
                               &have_base, NULL, &have_work,
                               eb->db, local_abspath,
                               scratch_pool, scratch_pool));

  if (!have_work)
    {
      base_status = status;
      base_kind = kind;
    }
  else
    SVN_ERR(svn_wc__db_base_get_info(&base_status, &base_kind, NULL,
                                     &repos_relpath,
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                     NULL, NULL, NULL, NULL,
                                     eb->db, local_abspath,
                                     scratch_pool, scratch_pool));

  /* Is this path a conflict victim? */
  if (conflicted)
    SVN_ERR(node_already_conflicted(&conflicted, eb->db, local_abspath,
                                    scratch_pool));
  if (conflicted)
    {
      SVN_ERR(remember_skipped_tree(eb, local_abspath, scratch_pool));

      do_notification(eb, local_abspath, svn_node_unknown,
                      svn_wc_notify_skip_conflicted,
                      scratch_pool);

      svn_pool_destroy(scratch_pool);

      return SVN_NO_ERROR;
    }



    /* Receive the remote removal of excluded/absent/not present node.
       Do not notify, but perform the change even when the node is shadowed */
  if (base_status == svn_wc__db_status_not_present
      || base_status == svn_wc__db_status_excluded
      || base_status == svn_wc__db_status_server_excluded)
    {
      SVN_ERR(svn_wc__db_base_remove(eb->db, local_abspath, scratch_pool));

      if (deleting_target)
        eb->target_deleted = TRUE;

      svn_pool_destroy(scratch_pool);

      return SVN_NO_ERROR;
    }

  /* Is this path the victim of a newly-discovered tree conflict?  If so,
   * remember it and notify the client. Then (if it was existing and
   * modified), re-schedule the node to be added back again, as a (modified)
   * copy of the previous base version.  */

  /* Check for conflicts only when we haven't already recorded
   * a tree-conflict on a parent node. */
  if (!pb->shadowed)
    {
      SVN_ERR(check_tree_conflict(&tree_conflict, eb, local_abspath,
                                  status, kind, TRUE,
                                  svn_wc_conflict_action_delete, svn_node_none,
                                  repos_relpath, pb->pool, scratch_pool));
    }

  if (tree_conflict != NULL)
    {
      /* When we raise a tree conflict on a node, we don't want to mark the
       * node as skipped, to allow a replacement to continue doing at least
       * a bit of its work (possibly adding a not present node, for the
       * next update) */
      if (!pb->deletion_conflicts)
        pb->deletion_conflicts = apr_hash_make(pb->pool);

      apr_hash_set(pb->deletion_conflicts, apr_pstrdup(pb->pool, base),
                   APR_HASH_KEY_STRING, tree_conflict);

      SVN_ERR(svn_wc__db_op_set_tree_conflict(eb->db,
                                              local_abspath,
                                              tree_conflict,
                                              scratch_pool));

      do_notification(eb, local_abspath, svn_node_unknown,
                      svn_wc_notify_tree_conflict, scratch_pool);

      if (tree_conflict->reason == svn_wc_conflict_reason_edited)
        {
          /* The item exists locally and has some sort of local mod.
           * It no longer exists in the repository at its target URL@REV.
           *
           * To prepare the "accept mine" resolution for the tree conflict,
           * we must schedule the existing content for re-addition as a copy
           * of what it was, but with its local modifications preserved. */
          SVN_ERR(svn_wc__db_temp_op_make_copy(eb->db, local_abspath,
                                               scratch_pool));

          /* Fall through to remove the BASE_NODEs properly, with potentially
             keeping a not-present marker */
        }
      else if (tree_conflict->reason == svn_wc_conflict_reason_deleted
               || tree_conflict->reason == svn_wc_conflict_reason_replaced)
        {
          /* The item does not exist locally because it was already shadowed.
           * We must complete the deletion, leaving the tree conflict info
           * as the only difference from a normal deletion. */

          /* Fall through to the normal "delete" code path. */
        }
      else
        SVN_ERR_MALFUNCTION();  /* other reasons are not expected here */
    }

  /* Issue a wq operation to delete the BASE_NODE data and to delete actual
     nodes based on that from disk, but leave any WORKING_NODEs on disk.

     Local modifications are already turned into copies at this point.

     If the thing being deleted is the *target* of this update, then
     we need to recreate a 'deleted' entry, so that the parent can give
     accurate reports about itself in the future. */
  if (! deleting_target)
    {
      /* Delete, and do not leave a not-present node.  */
      SVN_ERR(svn_wc__wq_build_base_remove(&work_item,
                                           eb->db, local_abspath,
                                           SVN_INVALID_REVNUM,
                                           svn_wc__db_kind_unknown,
                                           scratch_pool, scratch_pool));
    }
  else
    {
      /* Delete, leaving a not-present node.  */
      SVN_ERR(svn_wc__wq_build_base_remove(&work_item,
                                           eb->db, local_abspath,
                                           *eb->target_revision,
                                           base_kind,
                                           scratch_pool, scratch_pool));
      eb->target_deleted = TRUE;
    }

  SVN_ERR(svn_wc__db_wq_add(eb->db, pb->local_abspath, work_item,
                                scratch_pool));

  SVN_ERR(svn_wc__wq_run(eb->db, pb->local_abspath,
                         eb->cancel_func, eb->cancel_baton,
                         scratch_pool));

  /* Notify. (If tree_conflict, we've already notified.) */
  if (tree_conflict == NULL)
    {
      svn_wc_notify_action_t action = svn_wc_notify_update_delete;
      svn_node_kind_t node_kind;

      if (pb->shadowed)
        action = svn_wc_notify_update_shadowed_delete;

      if (kind == svn_wc__db_kind_dir)
        node_kind = svn_node_dir;
      else
        node_kind = svn_node_file;

      do_notification(eb, local_abspath, node_kind, action, scratch_pool);
    }

  svn_pool_destroy(scratch_pool);

  return SVN_NO_ERROR;
}

/* An svn_delta_editor_t function. */
static svn_error_t *
add_directory(const char *path,
              void *parent_baton,
              const char *copyfrom_path,
              svn_revnum_t copyfrom_rev,
              apr_pool_t *pool,
              void **child_baton)
{
  struct dir_baton *pb = parent_baton;
  struct edit_baton *eb = pb->edit_baton;
  struct dir_baton *db;
  svn_node_kind_t kind;
  svn_wc__db_status_t status;
  svn_wc__db_kind_t wc_kind;
  svn_boolean_t conflicted;
  svn_boolean_t versioned_locally_and_present;
  svn_wc_conflict_description2_t *tree_conflict = NULL;
  svn_error_t *err;

  SVN_ERR_ASSERT(! (copyfrom_path || SVN_IS_VALID_REVNUM(copyfrom_rev)));

  SVN_ERR(make_dir_baton(&db, path, eb, pb, TRUE, pool));
  *child_baton = db;

  if (db->skip_this)
    return SVN_NO_ERROR;

  SVN_ERR(mark_directory_edited(db, pool));

  if (strcmp(eb->target_abspath, db->local_abspath) == 0)
    {
      /* The target of the edit is being added, give it the requested
         depth of the edit (but convert svn_depth_unknown to
         svn_depth_infinity). */
      db->ambient_depth = (eb->requested_depth == svn_depth_unknown)
        ? svn_depth_infinity : eb->requested_depth;
    }
  else if (eb->requested_depth == svn_depth_immediates
           || (eb->requested_depth == svn_depth_unknown
               && pb->ambient_depth == svn_depth_immediates))
    {
      db->ambient_depth = svn_depth_empty;
    }
  else
    {
      db->ambient_depth = svn_depth_infinity;
    }

  /* It may not be named the same as the administrative directory. */
  if (svn_wc_is_adm_dir(db->name, pool))
    return svn_error_createf(
       SVN_ERR_WC_OBSTRUCTED_UPDATE, NULL,
       _("Failed to add directory '%s': object of the same name as the "
         "administrative directory"),
       svn_dirent_local_style(db->local_abspath, pool));

  SVN_ERR(svn_io_check_path(db->local_abspath, &kind, db->pool));

  err = svn_wc__db_read_info(&status, &wc_kind, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL,
                             &conflicted, NULL, NULL, NULL, NULL, NULL, NULL,
                             eb->db, db->local_abspath, db->pool, db->pool);
  if (err)
    {
      if (err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND)
        return svn_error_trace(err);

      svn_error_clear(err);
      wc_kind = svn_wc__db_kind_unknown;
      status = svn_wc__db_status_normal;
      conflicted = FALSE;

      versioned_locally_and_present = FALSE;
    }
  else if (wc_kind == svn_wc__db_kind_dir
           && status == svn_wc__db_status_normal)
    {
      /* !! We found the root of a separate working copy obstructing the wc !!

         If the directory would be part of our own working copy then
         we wouldn't have been called as an add_directory().

         The only thing we can do is add a not-present node, to allow
         a future update to bring in the new files when the problem is
         resolved.  Note that svn_wc__db_base_add_not_present_node()
         explicitly adds the node into the parent's node database. */

      SVN_ERR(svn_wc__db_base_add_not_present_node(eb->db, db->local_abspath,
                                                   db->new_relpath,
                                                   eb->repos_root,
                                                   eb->repos_uuid,
                                                   *eb->target_revision,
                                                   svn_wc__db_kind_file,
                                                   NULL, NULL,
                                                   pool));

      remember_skipped_tree(eb, db->local_abspath, pool);
      db->skip_this = TRUE;
      db->already_notified = TRUE;

      do_notification(eb, db->local_abspath, svn_node_dir,
                      svn_wc_notify_update_skip_obstruction, pool);

      return SVN_NO_ERROR;
    }
  else if (status == svn_wc__db_status_normal
           && (wc_kind == svn_wc__db_kind_file
               || wc_kind == svn_wc__db_kind_symlink))
    {
      /* We found a file external occupating the place we need in BASE.

         We can't add a not-present node in this case as that would overwrite
         the file external. Luckily the file external itself stops us from
         forgetting a child of this parent directory like an obstructing
         working copy would.

         The reason we get here is that the adm crawler doesn't report
         file externals.
      */

      remember_skipped_tree(eb, db->local_abspath, pool);
      db->skip_this = TRUE;
      db->already_notified = TRUE;

      do_notification(eb, db->local_abspath, svn_node_file,
                      svn_wc_notify_update_skip_obstruction, pool);

      return SVN_NO_ERROR;
    }
  else if (wc_kind == svn_wc__db_kind_unknown)
    versioned_locally_and_present = FALSE; /* Tree conflict ACTUAL-only node */
  else
    versioned_locally_and_present = IS_NODE_PRESENT(status);

  /* Is this path a conflict victim? */
  if (conflicted)
    {
      if (pb->deletion_conflicts)
        tree_conflict = apr_hash_get(pb->deletion_conflicts, db->name,
                                     APR_HASH_KEY_STRING);

      if (tree_conflict)
        {
          /* So this deletion wasn't just a deletion, it is actually a
             replacement. Luckily we still have the conflict so we can
             just update it. */

          /* ### What else should we update? */
          tree_conflict->action = svn_wc_conflict_action_replace;

          SVN_ERR(svn_wc__db_op_set_tree_conflict(eb->db, db->local_abspath,
                                                  tree_conflict, pool));

          /* And now stop checking for conflicts here and just perform
             a shadowed update */
          tree_conflict = NULL; /* No direct notification */
          db->shadowed = TRUE; /* Just continue */
          conflicted = FALSE; /* No skip */
        }
      else
        SVN_ERR(node_already_conflicted(&conflicted, eb->db,
                                        db->local_abspath, pool));
    }

  /* Now the "usual" behaviour if already conflicted. Skip it. */
  if (conflicted)
    {
      /* Record this conflict so that its descendants are skipped silently. */
      SVN_ERR(remember_skipped_tree(eb, db->local_abspath, pool));

      db->skip_this = TRUE;
      db->already_notified = TRUE;

      /* We skip this node, but once the update completes the parent node will
         be updated to the new revision. So a future recursive update of the
         parent will not bring in this new node as the revision of the parent
         describes to the repository that all children are available.

         To resolve this problem, we add a not-present node to allow bringing
         the node in once this conflict is resolved.

         Note that we can safely assume that no present base node exists,
         because then we would not have received an add_directory.
       */
      SVN_ERR(svn_wc__db_base_add_not_present_node(eb->db, db->local_abspath,
                                                   db->new_relpath,
                                                   eb->repos_root,
                                                   eb->repos_uuid,
                                                   *eb->target_revision,
                                                   svn_wc__db_kind_dir,
                                                   NULL, NULL,
                                                   pool));

      /* ### TODO: Also print victim_path in the skip msg. */
      do_notification(eb, db->local_abspath, svn_node_dir,
                      svn_wc_notify_skip_conflicted, pool);
      return SVN_NO_ERROR;
    }

  if (db->shadowed)
    {
      /* Nothing to check; does not and will not exist in working copy */
    }
  else if (versioned_locally_and_present)
    {
      /* What to do with a versioned or schedule-add dir:

         A dir already added without history is OK.  Set add_existed
         so that user notification is delayed until after any prop
         conflicts have been found.

         An existing versioned dir is an error.  In the future we may
         relax this restriction and simply update such dirs.

         A dir added with history is a tree conflict. */

      svn_boolean_t local_is_non_dir;
      svn_wc__db_status_t add_status = svn_wc__db_status_normal;

      /* Is the local add a copy? */
      if (status == svn_wc__db_status_added)
        SVN_ERR(svn_wc__db_scan_addition(&add_status, NULL, NULL, NULL, NULL,
                                         NULL, NULL, NULL, NULL,
                                         eb->db, db->local_abspath,
                                         pool, pool));


      /* Is there *something* that is not a dir? */
      local_is_non_dir = (wc_kind != svn_wc__db_kind_dir
                          && status != svn_wc__db_status_deleted);

      /* Do tree conflict checking if
       *  - if there is a local copy.
       *  - if this is a switch operation
       *  - the node kinds mismatch
       *
       * During switch, local adds at the same path as incoming adds get
       * "lost" in that switching back to the original will no longer have the
       * local add. So switch always alerts the user with a tree conflict. */
      if (!eb->adds_as_modification
          || local_is_non_dir
          || add_status != svn_wc__db_status_added)
        {
          SVN_ERR(check_tree_conflict(&tree_conflict, eb,
                                      db->local_abspath,
                                      status, wc_kind, FALSE,
                                      svn_wc_conflict_action_add,
                                      svn_node_dir, db->new_relpath,
                                      pool, pool));
        }

      if (tree_conflict == NULL)
        db->add_existed = TRUE; /* Take over WORKING */
      else
        db->shadowed = TRUE; /* Only update BASE */
    }
  else if (kind != svn_node_none)
    {
      /* There's an unversioned node at this path. */
      db->obstruction_found = TRUE;

      /* Unversioned, obstructing dirs are handled by prop merge/conflict,
       * if unversioned obstructions are allowed. */
      if (! (kind == svn_node_dir && eb->allow_unver_obstructions))
        {
          /* Bring in the node as deleted */ /* ### Obstructed Conflict */
          db->shadowed = TRUE;

          /* Mark a conflict */
          SVN_ERR(create_tree_conflict(&tree_conflict, eb,
                                       db->local_abspath,
                                       svn_wc_conflict_reason_unversioned,
                                       svn_wc_conflict_action_add,
                                       svn_node_dir,
                                       db->new_relpath, pool, pool));
          SVN_ERR_ASSERT(tree_conflict != NULL);
        }
    }

  SVN_ERR(svn_wc__db_temp_op_set_new_dir_to_incomplete(eb->db,
                                                       db->local_abspath,
                                                       db->new_relpath,
                                                       eb->repos_root,
                                                       eb->repos_uuid,
                                                       *eb->target_revision,
                                                       db->ambient_depth,
                                                       pool));

  /* Make sure there is a real directory at LOCAL_ABSPATH, unless we are just
     updating the DB */
  if (!db->shadowed)
    SVN_ERR(svn_wc__ensure_directory(db->local_abspath, pool));

  if (!db->shadowed && status == svn_wc__db_status_added)
    /* If there is no conflict we take over any added directory */
    SVN_ERR(svn_wc__db_temp_op_remove_working(eb->db, db->local_abspath, pool));

  /* ### We can't record an unversioned obstruction yet, so
     ### we record a delete instead, which will allow resolving the conflict
     ### to theirs with 'svn revert'. */
  if (db->shadowed && db->obstruction_found)
    {
      SVN_ERR(svn_wc__db_op_delete(eb->db, db->local_abspath,
                                   NULL, NULL /* notification */,
                                   eb->cancel_func, eb->cancel_baton,
                                   pool));
    }

  if (tree_conflict != NULL)
    {
      SVN_ERR(svn_wc__db_op_set_tree_conflict(eb->db, db->local_abspath,
                                              tree_conflict, pool));

      db->already_notified = TRUE;

      do_notification(eb, db->local_abspath, svn_node_dir,
                      svn_wc_notify_tree_conflict, pool);
    }



  /* If this add was obstructed by dir scheduled for addition without
     history let close_file() handle the notification because there
     might be properties to deal with.  If PATH was added inside a locally
     deleted tree, then suppress notification, a tree conflict was already
     issued. */
  if (eb->notify_func && !db->already_notified && !db->add_existed)
    {
      svn_wc_notify_action_t action;

      if (db->shadowed)
        action = svn_wc_notify_update_shadowed_add;
      else if (db->obstruction_found)
        action = svn_wc_notify_exists;
      else
        action = svn_wc_notify_update_add;

      db->already_notified = TRUE;

      do_notification(eb, db->local_abspath, svn_node_dir, action, pool);
    }

  return SVN_NO_ERROR;
}

/* An svn_delta_editor_t function. */
static svn_error_t *
open_directory(const char *path,
               void *parent_baton,
               svn_revnum_t base_revision,
               apr_pool_t *pool,
               void **child_baton)
{
  struct dir_baton *db, *pb = parent_baton;
  struct edit_baton *eb = pb->edit_baton;
  svn_boolean_t have_work;
  svn_boolean_t conflicted;
  svn_wc_conflict_description2_t *tree_conflict = NULL;
  svn_wc__db_status_t status, base_status;
  svn_wc__db_kind_t wc_kind;

  SVN_ERR(make_dir_baton(&db, path, eb, pb, FALSE, pool));
  *child_baton = db;

  if (db->skip_this)
    return SVN_NO_ERROR;

  /* Detect obstructing working copies */
  {
    svn_boolean_t is_root;

    SVN_ERR(svn_wc__db_is_wcroot(&is_root, eb->db, db->local_abspath,
                                 pool));

    if (is_root)
      {
        /* Just skip this node; a future update will handle it */
        remember_skipped_tree(eb, db->local_abspath, pool);
        db->skip_this = TRUE;
        db->already_notified = TRUE;

        do_notification(eb, db->local_abspath, svn_node_dir,
                        svn_wc_notify_update_skip_obstruction, pool);

        return SVN_NO_ERROR;
      }
  }

  /* We should have a write lock on every directory touched.  */
  SVN_ERR(svn_wc__write_check(eb->db, db->local_abspath, pool));

  SVN_ERR(svn_wc__db_read_info(&status, &wc_kind, &db->old_revision, NULL,
                               NULL, NULL, &db->changed_rev, &db->changed_date,
                               &db->changed_author, &db->ambient_depth,
                               NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL,
                               &conflicted, NULL, NULL, NULL,
                               NULL, NULL, &have_work,
                               eb->db, db->local_abspath,
                               db->pool, pool));

  if (!have_work)
    base_status = status;
  else
    SVN_ERR(svn_wc__db_base_get_info(&base_status, NULL, &db->old_revision,
                                     NULL, NULL, NULL, &db->changed_rev,
                                     &db->changed_date, &db->changed_author,
                                     &db->ambient_depth, NULL, NULL, NULL,
                                     NULL, NULL,
                                     eb->db, db->local_abspath,
                                     db->pool, pool));

  db->was_incomplete = (base_status == svn_wc__db_status_incomplete);

  /* Is this path a conflict victim? */
  if (conflicted)
    SVN_ERR(node_already_conflicted(&conflicted, eb->db,
                                    db->local_abspath, pool));
  if (conflicted)
    {
      SVN_ERR(remember_skipped_tree(eb, db->local_abspath, pool));

      db->skip_this = TRUE;
      db->already_notified = TRUE;

      do_notification(eb, db->local_abspath, svn_node_unknown,
                      svn_wc_notify_skip_conflicted, pool);

      return SVN_NO_ERROR;
    }

  /* Is this path a fresh tree conflict victim?  If so, skip the tree
     with one notification. */

  /* Check for conflicts only when we haven't already recorded
   * a tree-conflict on a parent node. */
  if (!db->shadowed)
    SVN_ERR(check_tree_conflict(&tree_conflict, eb, db->local_abspath,
                                status, wc_kind, TRUE,
                                svn_wc_conflict_action_edit, svn_node_dir,
                                db->new_relpath, db->pool, pool));

  /* Remember the roots of any locally deleted trees. */
  if (tree_conflict != NULL)
    {
      db->edit_conflict = tree_conflict;
      /* Other modifications wouldn't be a tree conflict */
      SVN_ERR_ASSERT(
                tree_conflict->reason == svn_wc_conflict_reason_deleted ||
                tree_conflict->reason == svn_wc_conflict_reason_replaced);

      /* Continue updating BASE */
      db->shadowed = TRUE;
    }

  /* Mark directory as being at target_revision and URL, but incomplete. */
  SVN_ERR(svn_wc__db_temp_op_start_directory_update(eb->db, db->local_abspath,
                                                    db->new_relpath,
                                                    *eb->target_revision,
                                                    pool));

  return SVN_NO_ERROR;
}


/* An svn_delta_editor_t function. */
static svn_error_t *
change_dir_prop(void *dir_baton,
                const char *name,
                const svn_string_t *value,
                apr_pool_t *pool)
{
  svn_prop_t *propchange;
  struct dir_baton *db = dir_baton;

  if (db->skip_this)
    return SVN_NO_ERROR;

  propchange = apr_array_push(db->propchanges);
  propchange->name = apr_pstrdup(db->pool, name);
  propchange->value = value ? svn_string_dup(value, db->pool) : NULL;

  if (!db->edited && svn_property_kind(NULL, name) == svn_prop_regular_kind)
    SVN_ERR(mark_directory_edited(db, pool));

  return SVN_NO_ERROR;
}

/* If any of the svn_prop_t objects in PROPCHANGES represents a change
   to the SVN_PROP_EXTERNALS property, return that change, else return
   null.  If PROPCHANGES contains more than one such change, return
   the first. */
static const svn_prop_t *
externals_prop_changed(const apr_array_header_t *propchanges)
{
  int i;

  for (i = 0; i < propchanges->nelts; i++)
    {
      const svn_prop_t *p = &(APR_ARRAY_IDX(propchanges, i, svn_prop_t));
      if (strcmp(p->name, SVN_PROP_EXTERNALS) == 0)
        return p;
    }

  return NULL;
}



/* An svn_delta_editor_t function. */
static svn_error_t *
close_directory(void *dir_baton,
                apr_pool_t *pool)
{
  struct dir_baton *db = dir_baton;
  struct edit_baton *eb = db->edit_baton;
  svn_wc_notify_state_t prop_state = svn_wc_notify_state_unknown;
  apr_array_header_t *entry_prop_changes;
  apr_array_header_t *dav_prop_changes;
  apr_array_header_t *regular_prop_changes;
  apr_hash_t *base_props;
  apr_hash_t *actual_props;
  apr_hash_t *new_base_props = NULL;
  apr_hash_t *new_actual_props = NULL;
  svn_revnum_t new_changed_rev = SVN_INVALID_REVNUM;
  apr_time_t new_changed_date = 0;
  const char *new_changed_author = NULL;
  apr_pool_t *scratch_pool = db->pool;
  svn_skel_t *all_work_items = NULL;

  /* Skip if we're in a conflicted tree. */
  if (db->skip_this)
    {
      /* Allow the parent to complete its update. */
      SVN_ERR(maybe_release_dir_info(db->bump_info));

      return SVN_NO_ERROR;
    }

  SVN_ERR(svn_categorize_props(db->propchanges, &entry_prop_changes,
                               &dav_prop_changes, &regular_prop_changes, pool));

  /* Fetch the existing properties.  */
  if ((!db->adding_dir || db->add_existed)
      && !db->shadowed)
    {
      SVN_ERR(svn_wc__get_actual_props(&actual_props,
                                       eb->db, db->local_abspath,
                                       scratch_pool, scratch_pool));
    }
  else
    actual_props = apr_hash_make(pool);

  if (db->add_existed)
    {
      /* This node already exists. Grab the current pristine properties. */
      SVN_ERR(svn_wc__get_pristine_props(&base_props,
                                         eb->db, db->local_abspath,
                                         scratch_pool, scratch_pool));
    }
  else if (!db->adding_dir)
    {
      /* Get the BASE properties for proper merging. */
      SVN_ERR(svn_wc__db_base_get_props(&base_props,
                                        eb->db, db->local_abspath,
                                        scratch_pool, scratch_pool));
    }
  else
    base_props = apr_hash_make(pool);

  /* An incomplete directory might have props which were supposed to be
     deleted but weren't.  Because the server sent us all the props we're
     supposed to have, any previous base props not in this list must be
     deleted (issue #1672). */
  if (db->was_incomplete)
    {
      int i;
      apr_hash_t *props_to_delete;
      apr_hash_index_t *hi;

      /* In a copy of the BASE props, remove every property that we see an
         incoming change for. The remaining unmentioned properties are those
         which need to be deleted.  */
      props_to_delete = apr_hash_copy(pool, base_props);
      for (i = 0; i < regular_prop_changes->nelts; i++)
        {
          const svn_prop_t *prop;
          prop = &APR_ARRAY_IDX(regular_prop_changes, i, svn_prop_t);
          apr_hash_set(props_to_delete, prop->name,
                       APR_HASH_KEY_STRING, NULL);
        }

      /* Add these props to the incoming propchanges (in
       * regular_prop_changes).  */
      for (hi = apr_hash_first(pool, props_to_delete);
           hi != NULL;
           hi = apr_hash_next(hi))
        {
          const char *propname = svn__apr_hash_index_key(hi);
          svn_prop_t *prop = apr_array_push(regular_prop_changes);

          /* Record a deletion for PROPNAME.  */
          prop->name = propname;
          prop->value = NULL;
        }
    }

  /* If this directory has property changes stored up, now is the time
     to deal with them. */
  if (regular_prop_changes->nelts)
    {
      svn_skel_t *work_item;

      /* If recording traversal info, then see if the
         SVN_PROP_EXTERNALS property on this directory changed,
         and record before and after for the change. */
      if (eb->external_func)
        {
          const svn_prop_t *change
            = externals_prop_changed(regular_prop_changes);

          if (change)
            {
              const svn_string_t *new_val_s = change->value;
              const svn_string_t *old_val_s;

              old_val_s = apr_hash_get(base_props, SVN_PROP_EXTERNALS,
                                       APR_HASH_KEY_STRING);

              if ((new_val_s == NULL) && (old_val_s == NULL))
                ; /* No value before, no value after... so do nothing. */
              else if (new_val_s && old_val_s
                       && (svn_string_compare(old_val_s, new_val_s)))
                ; /* Value did not change... so do nothing. */
              else if (old_val_s || new_val_s)
                /* something changed, record the change */
                {
                  SVN_ERR((eb->external_func)(
                                       eb->external_baton,
                                       db->local_abspath,
                                       old_val_s,
                                       new_val_s,
                                       db->ambient_depth,
                                       db->pool));
                }
            }
        }

      if (db->shadowed)
        {
          /* We don't have a relevant actual row, but we need actual properties
             to allow property merging without conflicts. */
          if (db->adding_dir)
            actual_props = apr_hash_make(scratch_pool);
          else
            actual_props = base_props;
        }

      /* Merge pending properties into temporary files (ignoring
         conflicts). */
      SVN_ERR_W(svn_wc__merge_props(&work_item,
                                    &prop_state,
                                    &new_base_props,
                                    &new_actual_props,
                                    eb->db,
                                    db->local_abspath,
                                    svn_wc__db_kind_dir,
                                    NULL, /* left_version */
                                    NULL, /* right_version */
                                    NULL /* use baseprops */,
                                    base_props,
                                    actual_props,
                                    regular_prop_changes,
                                    TRUE /* base_merge */,
                                    FALSE /* dry_run */,
                                    eb->conflict_func,
                                    eb->conflict_baton,
                                    eb->cancel_func,
                                    eb->cancel_baton,
                                    db->pool,
                                    scratch_pool),
                _("Couldn't do property merge"));
      /* After a (not-dry-run) merge, we ALWAYS have props to save.  */
      SVN_ERR_ASSERT(new_base_props != NULL && new_actual_props != NULL);
      all_work_items = svn_wc__wq_merge(all_work_items, work_item,
                                        scratch_pool);
    }

  SVN_ERR(accumulate_last_change(&new_changed_rev, &new_changed_date,
                                 &new_changed_author, entry_prop_changes,
                                 scratch_pool, scratch_pool));

  /* Check if we should add some not-present markers before marking the
     directory complete (Issue #3569) */
  {
    apr_hash_t *new_children = apr_hash_get(eb->dir_dirents, db->new_relpath,
                                            APR_HASH_KEY_STRING);

    if (new_children != NULL)
      {
        apr_hash_index_t *hi;
        apr_pool_t *iterpool = svn_pool_create(scratch_pool);

        for (hi = apr_hash_first(scratch_pool, new_children);
             hi;
             hi = apr_hash_next(hi))
          {
            const char *child_name;
            const char *child_abspath;
            const char *child_relpath;
            const svn_dirent_t *dirent;
            svn_wc__db_status_t status;
            svn_wc__db_kind_t child_kind;
            svn_error_t *err;

            svn_pool_clear(iterpool);

            child_name = svn__apr_hash_index_key(hi);
            child_abspath = svn_dirent_join(db->local_abspath, child_name,
                                            iterpool);

            dirent = svn__apr_hash_index_val(hi);
            child_kind = (dirent->kind == svn_node_dir)
                                        ? svn_wc__db_kind_dir
                                        : svn_wc__db_kind_file;

            if (db->ambient_depth < svn_depth_immediates
                && child_kind == svn_wc__db_kind_dir)
              continue; /* We don't need the subdirs */

            /* ### We just check if there is some node in BASE at this path */
            err = svn_wc__db_base_get_info(&status, NULL, NULL, NULL, NULL,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL, NULL, NULL,
                                           eb->db, child_abspath,
                                           iterpool, iterpool);

            if (!err)
              {
                svn_boolean_t is_wcroot;
                SVN_ERR(svn_wc__db_is_wcroot(&is_wcroot, eb->db, child_abspath,
                                             iterpool));

                if (!is_wcroot)
                  continue; /* Everything ok... Nothing to do here */
                /* Fall through to allow recovering later */
              }
            else if (err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND)
              return svn_error_trace(err);

            svn_error_clear(err);

            child_relpath = svn_relpath_join(db->new_relpath, child_name,
                                             iterpool);

            SVN_ERR(svn_wc__db_base_add_not_present_node(eb->db,
                                                         child_abspath,
                                                         child_relpath,
                                                         eb->repos_root,
                                                         eb->repos_uuid,
                                                         *eb->target_revision,
                                                         child_kind,
                                                         NULL, NULL,
                                                         iterpool));
          }

        svn_pool_destroy(iterpool);
      }
  }

  if (apr_hash_count(db->not_present_files))
    {
      apr_hash_index_t *hi;
      apr_pool_t *iterpool = svn_pool_create(scratch_pool);

      /* This should call some new function (which could also be used
         for new_children above) to add all the names in single
         transaction, but I can't even trigger it.  I've tried
         ra_local, ra_svn, ra_neon, ra_serf and they all call
         close_file before close_dir. */
      for (hi = apr_hash_first(scratch_pool, db->not_present_files);
           hi;
           hi = apr_hash_next(hi))
        {
          const char *child = svn__apr_hash_index_key(hi);
          const char *child_abspath, *child_relpath;

          svn_pool_clear(iterpool);

          child_abspath = svn_dirent_join(db->local_abspath, child, iterpool);
          child_relpath = svn_dirent_join(db->new_relpath, child, iterpool);

          SVN_ERR(svn_wc__db_base_add_not_present_node(eb->db,
                                                       child_abspath,
                                                       child_relpath,
                                                       eb->repos_root,
                                                       eb->repos_uuid,
                                                       *eb->target_revision,
                                                       svn_wc__db_kind_file,
                                                       NULL, NULL,
                                                       iterpool));
        }
      svn_pool_destroy(iterpool);
    }

  /* If this directory is merely an anchor for a targeted child, then we
     should not be updating the node at all.  */
  if (db->parent_baton == NULL
      && *eb->target_basename != '\0')
    {
      /* And we should not have received any changes!  */
      SVN_ERR_ASSERT(db->propchanges->nelts == 0);
      /* ... which also implies NEW_CHANGED_* are not set,
         and NEW_BASE_PROPS == NULL.  */
    }
  else
    {
      apr_hash_t *props;

      /* ### we know a base node already exists. it was created in
         ### open_directory or add_directory.  let's just preserve the
         ### existing DEPTH value, and possibly CHANGED_*.  */
      /* If we received any changed_* values, then use them.  */
      if (SVN_IS_VALID_REVNUM(new_changed_rev))
        db->changed_rev = new_changed_rev;
      if (new_changed_date != 0)
        db->changed_date = new_changed_date;
      if (new_changed_author != NULL)
        db->changed_author = new_changed_author;

      /* If no depth is set yet, set to infinity. */
      if (db->ambient_depth == svn_depth_unknown)
        db->ambient_depth = svn_depth_infinity;

      if (eb->depth_is_sticky
          && db->ambient_depth != eb->requested_depth)
        {
          /* After a depth upgrade the entry must reflect the new depth.
             Upgrading to infinity changes the depth of *all* directories,
             upgrading to something else only changes the target. */

          if (eb->requested_depth == svn_depth_infinity
              || (strcmp(db->local_abspath, eb->target_abspath) == 0
                  && eb->requested_depth > db->ambient_depth))
            {
              db->ambient_depth = eb->requested_depth;
            }
        }

      /* Do we have new properties to install? Or shall we simply retain
         the prior set of properties? If we're installing new properties,
         then we also want to write them to an old-style props file.  */
      props = new_base_props;
      if (props == NULL)
        props = base_props;

      /* Update the BASE data for the directory and mark the directory
         complete */
      SVN_ERR(svn_wc__db_base_add_directory(
                eb->db, db->local_abspath,
                eb->wcroot_abspath,
                db->new_relpath,
                eb->repos_root, eb->repos_uuid,
                *eb->target_revision,
                props,
                db->changed_rev, db->changed_date, db->changed_author,
                NULL /* children */,
                db->ambient_depth,
                (dav_prop_changes->nelts > 0)
                    ? svn_prop_array_to_hash(dav_prop_changes, pool)
                    : NULL,
                NULL /* conflict */,
                (! db->shadowed) && new_base_props != NULL,
                new_actual_props,
                all_work_items,
                scratch_pool));
    }

  /* Process all of the queued work items for this directory.  */
  SVN_ERR(svn_wc__wq_run(eb->db, db->local_abspath,
                         eb->cancel_func, eb->cancel_baton,
                         scratch_pool));

  /* Notify of any prop changes on this directory -- but do nothing if
     it's an added or skipped directory, because notification has already
     happened in that case - unless the add was obstructed by a dir
     scheduled for addition without history, in which case we handle
     notification here). */
  if (!db->already_notified && eb->notify_func && db->edited)
    {
      svn_wc_notify_t *notify;
      svn_wc_notify_action_t action;

      if (db->shadowed)
        action = svn_wc_notify_update_shadowed_update;
      else if (db->obstruction_found || db->add_existed)
        action = svn_wc_notify_exists;
      else
        action = svn_wc_notify_update_update;

      notify = svn_wc_create_notify(db->local_abspath, action, pool);
      notify->kind = svn_node_dir;
      notify->prop_state = prop_state;
      notify->revision = *eb->target_revision;
      notify->old_revision = db->old_revision;

      eb->notify_func(eb->notify_baton, notify, scratch_pool);
    }

  /* We're done with this directory, so remove one reference from the
     bump information. */
  SVN_ERR(maybe_release_dir_info(db->bump_info));

  return SVN_NO_ERROR;
}


/* Common code for 'absent_file' and 'absent_directory'. */
static svn_error_t *
absent_node(const char *path,
            svn_wc__db_kind_t absent_kind,
            void *parent_baton,
            apr_pool_t *pool)
{
  struct dir_baton *pb = parent_baton;
  struct edit_baton *eb = pb->edit_baton;
  apr_pool_t *scratch_pool = svn_pool_create(pool);
  const char *name = svn_dirent_basename(path, NULL);
  const char *local_abspath;
  svn_error_t *err;
  svn_wc__db_status_t status;
  svn_wc__db_kind_t kind;

  if (pb->skip_this)
    return SVN_NO_ERROR;

  SVN_ERR(mark_directory_edited(pb, scratch_pool));

  local_abspath = svn_dirent_join(pb->local_abspath, name, scratch_pool);

  /* If an item by this name is scheduled for addition that's a
     genuine tree-conflict.  */
  err = svn_wc__db_read_info(&status, &kind, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL,
                             eb->db, local_abspath,
                             scratch_pool, scratch_pool);

  if (err)
    {
      if (err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND)
        return svn_error_trace(err);

      svn_error_clear(err);
      status = svn_wc__db_status_not_present;
      kind = svn_wc__db_kind_unknown;
    }

  if (status == svn_wc__db_status_normal
      && kind == svn_wc__db_kind_dir)
    {
      /* We found an obstructing working copy!

         We can do two things now:
            1) notify the user, record a skip, etc.
            2) Just record the absent node in BASE in the parent
               working copy.

         As option 2 happens to be exactly what we do anyway, lets do that.
      */
    }
  else if (status == svn_wc__db_status_not_present
           || status == svn_wc__db_status_server_excluded
           || status == svn_wc__db_status_excluded)
    {
      /* The BASE node is not actually there, so we can safely turn it into
         an absent node */
    }
  else
    {
      /* We have a local addition. If this would be a BASE node it would have
         been deleted before we get here. (Which might have turned it into
         a copy).

         ### This should be recorded as a tree conflict and the update
         ### can just continue, as we can just record the absent status
         ### in BASE.
       */
      SVN_ERR_ASSERT(status != svn_wc__db_status_normal);

      return svn_error_createf(
         SVN_ERR_WC_OBSTRUCTED_UPDATE, NULL,
         _("Failed to mark '%s' absent: item of the same name is already "
           "scheduled for addition"),
         svn_dirent_local_style(local_abspath, pool));
    }

  {
    const char *repos_relpath;
    repos_relpath = svn_relpath_join(pb->new_relpath, name, scratch_pool);

    /* Insert an excluded node below the parent node to note that this child
       is absent. (This puts it in the parent db if the child is obstructed) */
    SVN_ERR(svn_wc__db_base_add_excluded_node(eb->db, local_abspath,
                                              repos_relpath, eb->repos_root,
                                              eb->repos_uuid,
                                              *(eb->target_revision),
                                              absent_kind,
                                              svn_wc__db_status_server_excluded,
                                              NULL, NULL,
                                              scratch_pool));
  }

  svn_pool_destroy(scratch_pool);

  return SVN_NO_ERROR;
}


/* An svn_delta_editor_t function. */
static svn_error_t *
absent_file(const char *path,
            void *parent_baton,
            apr_pool_t *pool)
{
  return absent_node(path, svn_wc__db_kind_file, parent_baton, pool);
}


/* An svn_delta_editor_t function. */
static svn_error_t *
absent_directory(const char *path,
                 void *parent_baton,
                 apr_pool_t *pool)
{
  return absent_node(path, svn_wc__db_kind_dir, parent_baton, pool);
}


/* An svn_delta_editor_t function. */
static svn_error_t *
add_file(const char *path,
         void *parent_baton,
         const char *copyfrom_path,
         svn_revnum_t copyfrom_rev,
         apr_pool_t *pool,
         void **file_baton)
{
  struct dir_baton *pb = parent_baton;
  struct edit_baton *eb = pb->edit_baton;
  struct file_baton *fb;
  svn_node_kind_t kind = svn_node_none;
  svn_wc__db_kind_t wc_kind = svn_wc__db_kind_unknown;
  svn_wc__db_status_t status = svn_wc__db_status_normal;
  apr_pool_t *scratch_pool;
  svn_boolean_t conflicted = FALSE;
  svn_boolean_t versioned_locally_and_present = FALSE;
  svn_wc_conflict_description2_t *tree_conflict = NULL;
  svn_error_t *err = SVN_NO_ERROR;

  SVN_ERR_ASSERT(! (copyfrom_path || SVN_IS_VALID_REVNUM(copyfrom_rev)));

  SVN_ERR(make_file_baton(&fb, pb, path, TRUE, pool));
  *file_baton = fb;

  if (fb->skip_this)
    return SVN_NO_ERROR;

  SVN_ERR(mark_file_edited(fb, pool));

  /* The file_pool can stick around for a *long* time, so we want to
     use a subpool for any temporary allocations. */
  scratch_pool = svn_pool_create(pool);


  /* It may not be named the same as the administrative directory. */
  if (svn_wc_is_adm_dir(fb->name, pool))
    return svn_error_createf(
       SVN_ERR_WC_OBSTRUCTED_UPDATE, NULL,
       _("Failed to add file '%s': object of the same name as the "
         "administrative directory"),
       svn_dirent_local_style(fb->local_abspath, pool));

  if (!eb->clean_checkout)
    {
      SVN_ERR(svn_io_check_path(fb->local_abspath, &kind, scratch_pool));

      err = svn_wc__db_read_info(&status, &wc_kind, NULL, NULL, NULL, NULL, NULL,
                                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                NULL, NULL, NULL, NULL, NULL,
                                &conflicted, NULL, NULL, NULL, NULL, NULL, NULL,
                                eb->db, fb->local_abspath,
                                scratch_pool, scratch_pool);
    }

  if (err)
    {
      if (err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND)
        return svn_error_trace(err);

      svn_error_clear(err);
      wc_kind = svn_wc__db_kind_unknown;
      conflicted = FALSE;

      versioned_locally_and_present = FALSE;
    }
  else if (wc_kind == svn_wc__db_kind_dir
           && status == svn_wc__db_status_normal)
    {
      /* !! We found the root of a separate working copy obstructing the wc !!

         If the directory would be part of our own working copy then
         we wouldn't have been called as an add_file().

         The only thing we can do is add a not-present node, to allow
         a future update to bring in the new files when the problem is
         resolved. */
      apr_hash_set(pb->not_present_files, apr_pstrdup(pb->pool, fb->name),
                   APR_HASH_KEY_STRING, (void*)1);

      remember_skipped_tree(eb, fb->local_abspath, pool);
      fb->skip_this = TRUE;
      fb->already_notified = TRUE;

      do_notification(eb, fb->local_abspath, svn_node_file,
                      svn_wc_notify_update_skip_obstruction, scratch_pool);

      svn_pool_destroy(scratch_pool);

      return SVN_NO_ERROR;
    }
  else if (status == svn_wc__db_status_normal
           && (wc_kind == svn_wc__db_kind_file
               || wc_kind == svn_wc__db_kind_symlink))
    {
      /* We found a file external occupating the place we need in BASE.

         We can't add a not-present node in this case as that would overwrite
         the file external. Luckily the file external itself stops us from
         forgetting a child of this parent directory like an obstructing
         working copy would.

         The reason we get here is that the adm crawler doesn't report
         file externals.
      */
      remember_skipped_tree(eb, fb->local_abspath, pool);
      fb->skip_this = TRUE;
      fb->already_notified = TRUE;

      do_notification(eb, fb->local_abspath, svn_node_file,
                      svn_wc_notify_update_skip_obstruction, scratch_pool);

      svn_pool_destroy(scratch_pool);

      return SVN_NO_ERROR;
    }
  else if (wc_kind == svn_wc__db_kind_unknown)
    versioned_locally_and_present = FALSE; /* Tree conflict ACTUAL-only node */
  else
    versioned_locally_and_present = IS_NODE_PRESENT(status);


  /* Is this path a conflict victim? */
  if (conflicted)
    if (conflicted)
    {
      if (pb->deletion_conflicts)
        tree_conflict = apr_hash_get(pb->deletion_conflicts, fb->name,
                                     APR_HASH_KEY_STRING);

      if (tree_conflict)
        {
          /* So this deletion wasn't just a deletion, it is actually a
             replacement. Luckily we still have the conflict so we can
             just update it. */

          /* ### What else should we update? */
          tree_conflict->action = svn_wc_conflict_action_replace;

          SVN_ERR(svn_wc__db_op_set_tree_conflict(eb->db, fb->local_abspath,
                                                  tree_conflict, pool));

          /* And now stop checking for conflicts here and just perform
             a shadowed update */
          tree_conflict = NULL; /* No direct notification */
          fb->shadowed = TRUE; /* Just continue */
          conflicted = FALSE; /* No skip */
        }
      else
        SVN_ERR(node_already_conflicted(&conflicted, eb->db,
                                        fb->local_abspath, pool));
    }

  /* Now the usual conflict handling: skip. */
  if (conflicted)
    {
      SVN_ERR(remember_skipped_tree(eb, fb->local_abspath, pool));

      fb->skip_this = TRUE;
      fb->already_notified = TRUE;

      /* We skip this node, but once the update completes the parent node will
         be updated to the new revision. So a future recursive update of the
         parent will not bring in this new node as the revision of the parent
         describes to the repository that all children are available.

         To resolve this problem, we add a not-present node to allow bringing
         the node in once this conflict is resolved.

         Note that we can safely assume that no present base node exists,
         because then we would not have received an add_file.
       */
      apr_hash_set(pb->not_present_files, apr_pstrdup(pb->pool, fb->name),
                   APR_HASH_KEY_STRING, (void*)1);

      do_notification(eb, fb->local_abspath, svn_node_unknown,
                      svn_wc_notify_skip_conflicted, scratch_pool);

      svn_pool_destroy(scratch_pool);

      return SVN_NO_ERROR;
    }

  if (fb->shadowed)
    {
      /* Nothing to check; does not and will not exist in working copy */
    }
  else if (versioned_locally_and_present)
    {
      /* What to do with a versioned or schedule-add file:

         If the UUID doesn't match the parent's, or the URL isn't a child of
         the parent dir's URL, it's an error.

         Set add_existed so that user notification is delayed until after any
         text or prop conflicts have been found.

         Whether the incoming add is a symlink or a file will only be known in
         close_file(), when the props are known. So with a locally added file
         or symlink, let close_file() check for a tree conflict.

         We will never see missing files here, because these would be
         re-added during the crawler phase. */
      svn_boolean_t local_is_file;

      /* Is the local node a copy or move */
      if (status == svn_wc__db_status_added)
        SVN_ERR(svn_wc__db_scan_addition(&status, NULL, NULL, NULL, NULL, NULL,
                                         NULL, NULL, NULL,
                                         eb->db, fb->local_abspath,
                                         scratch_pool, scratch_pool));

      /* Is there something that is a file? */
      local_is_file = (wc_kind == svn_wc__db_kind_file
                       || wc_kind == svn_wc__db_kind_symlink);

      /* Do tree conflict checking if
       *  - if there is a local copy.
       *  - if this is a switch operation
       *  - the node kinds mismatch
       *
       * During switch, local adds at the same path as incoming adds get
       * "lost" in that switching back to the original will no longer have the
       * local add. So switch always alerts the user with a tree conflict. */
      if (!eb->adds_as_modification
          || !local_is_file
          || status != svn_wc__db_status_added)
        {
          SVN_ERR(check_tree_conflict(&tree_conflict, eb,
                                      fb->local_abspath,
                                      status, wc_kind, FALSE,
                                      svn_wc_conflict_action_add,
                                      svn_node_file, fb->new_relpath,
                                      scratch_pool, scratch_pool));
        }

      if (tree_conflict == NULL)
        fb->add_existed = TRUE; /* Take over WORKING */
      else
        fb->shadowed = TRUE; /* Only update BASE */

    }
  else if (kind != svn_node_none)
    {
      /* There's an unversioned node at this path. */
      fb->obstruction_found = TRUE;

      /* Unversioned, obstructing files are handled by text merge/conflict,
       * if unversioned obstructions are allowed. */
      if (! (kind == svn_node_file && eb->allow_unver_obstructions))
        {
          /* Bring in the node as deleted */ /* ### Obstructed Conflict */
          fb->shadowed = TRUE;

          /* Mark a conflict */
          SVN_ERR(create_tree_conflict(&tree_conflict, eb,
                                       fb->local_abspath,
                                       svn_wc_conflict_reason_unversioned,
                                       svn_wc_conflict_action_add,
                                       svn_node_file,
                                       fb->new_relpath,
                                       scratch_pool, scratch_pool));
          SVN_ERR_ASSERT(tree_conflict != NULL);
        }
    }

  /* When this is not the update target add a not-present BASE node now,
     to allow marking the parent directory complete in its close_edit() call.
     This resolves issues when that occurs before the close_file(). */
  if (pb->parent_baton
      || *eb->target_basename == '\0'
      || (strcmp(fb->local_abspath, eb->target_abspath) != 0))
    {
      apr_hash_set(pb->not_present_files, apr_pstrdup(pb->pool, fb->name),
                   APR_HASH_KEY_STRING, (void*)1);
    }

  if (tree_conflict != NULL)
    {
      SVN_ERR(svn_wc__db_op_set_tree_conflict(eb->db,
                                              fb->local_abspath,
                                              tree_conflict,
                                              scratch_pool));

      fb->already_notified = TRUE;
      do_notification(eb, fb->local_abspath, svn_node_file,
                      svn_wc_notify_tree_conflict, scratch_pool);
    }

  svn_pool_destroy(scratch_pool);

  return SVN_NO_ERROR;
}


/* An svn_delta_editor_t function. */
static svn_error_t *
open_file(const char *path,
          void *parent_baton,
          svn_revnum_t base_revision,
          apr_pool_t *pool,
          void **file_baton)
{
  struct dir_baton *pb = parent_baton;
  struct edit_baton *eb = pb->edit_baton;
  struct file_baton *fb;
  svn_boolean_t conflicted;
  svn_boolean_t have_work;
  svn_wc__db_status_t status;
  svn_wc__db_kind_t wc_kind;
  svn_wc_conflict_description2_t *tree_conflict = NULL;

  /* the file_pool can stick around for a *long* time, so we want to use
     a subpool for any temporary allocations. */
  apr_pool_t *scratch_pool = svn_pool_create(pool);

  SVN_ERR(make_file_baton(&fb, pb, path, FALSE, pool));
  *file_baton = fb;

  if (fb->skip_this)
    return SVN_NO_ERROR;

  /* Detect obstructing working copies */
  {
    svn_boolean_t is_root;

    SVN_ERR(svn_wc__db_is_wcroot(&is_root, eb->db, fb->local_abspath,
                                 pool));

    if (is_root)
      {
        /* Just skip this node; a future update will handle it */
        remember_skipped_tree(eb, fb->local_abspath, pool);
        fb->skip_this = TRUE;
        fb->already_notified = TRUE;

        do_notification(eb, fb->local_abspath, svn_node_file,
                        svn_wc_notify_update_skip_obstruction, pool);

        return SVN_NO_ERROR;
      }
  }

  /* Sanity check. */

  /* If replacing, make sure the .svn entry already exists. */
  SVN_ERR(svn_wc__db_read_info(&status, &wc_kind, &fb->old_revision, NULL,
                               NULL, NULL, &fb->changed_rev, &fb->changed_date,
                               &fb->changed_author, NULL,
                               &fb->original_checksum, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL,
                               &conflicted, NULL, NULL, &fb->local_prop_mods,
                               NULL, NULL, &have_work,
                               eb->db, fb->local_abspath,
                               fb->pool, scratch_pool));

  if (have_work)
    SVN_ERR(svn_wc__db_base_get_info(NULL, NULL, &fb->old_revision,
                                     NULL, NULL, NULL, &fb->changed_rev,
                                     &fb->changed_date, &fb->changed_author,
                                     NULL, &fb->original_checksum, NULL, NULL,
                                     NULL, NULL,
                                     eb->db, fb->local_abspath,
                                     fb->pool, scratch_pool));

  /* Is this path a conflict victim? */
  if (conflicted)
    SVN_ERR(node_already_conflicted(&conflicted, eb->db,
                                    fb->local_abspath, pool));
  if (conflicted)
    {
      SVN_ERR(remember_skipped_tree(eb, fb->local_abspath, pool));

      fb->skip_this = TRUE;
      fb->already_notified = TRUE;

      do_notification(eb, fb->local_abspath, svn_node_unknown,
                      svn_wc_notify_skip_conflicted, scratch_pool);

      svn_pool_destroy(scratch_pool);

      return SVN_NO_ERROR;
    }

  fb->shadowed = pb->shadowed;

  /* Check for conflicts only when we haven't already recorded
   * a tree-conflict on a parent node. */
  if (!fb->shadowed)
    SVN_ERR(check_tree_conflict(&tree_conflict, eb, fb->local_abspath,
                                status, wc_kind, TRUE,
                                svn_wc_conflict_action_edit, svn_node_file,
                                fb->new_relpath, fb->pool, scratch_pool));

  /* Is this path the victim of a newly-discovered tree conflict? */
  if (tree_conflict != NULL)
    {
      fb->edit_conflict = tree_conflict;

      /* Other modifications wouldn't be a tree conflict */
      SVN_ERR_ASSERT(
                tree_conflict->reason == svn_wc_conflict_reason_deleted ||
                tree_conflict->reason == svn_wc_conflict_reason_replaced);

      /* Continue updating BASE */
      fb->shadowed = TRUE;
    }

  svn_pool_destroy(scratch_pool);

  return SVN_NO_ERROR;
}

/* An svn_delta_editor_t function. */
static svn_error_t *
apply_textdelta(void *file_baton,
                const char *expected_checksum,
                apr_pool_t *pool,
                svn_txdelta_window_handler_t *handler,
                void **handler_baton)
{
  struct file_baton *fb = file_baton;
  apr_pool_t *handler_pool = svn_pool_create(fb->pool);
  struct handler_baton *hb = apr_pcalloc(handler_pool, sizeof(*hb));
  struct edit_baton *eb = fb->edit_baton;
  svn_error_t *err;
  const svn_checksum_t *recorded_base_checksum;
  svn_checksum_t *expected_base_checksum;
  svn_stream_t *source;
  svn_stream_t *target;

  if (fb->skip_this)
    {
      *handler = svn_delta_noop_window_handler;
      *handler_baton = NULL;
      return SVN_NO_ERROR;
    }

  SVN_ERR(mark_file_edited(fb, pool));

  /* Parse checksum or sets expected_base_checksum to NULL */
  SVN_ERR(svn_checksum_parse_hex(&expected_base_checksum, svn_checksum_md5,
                                 expected_checksum, pool));

  /* Before applying incoming svndiff data to text base, make sure
     text base hasn't been corrupted, and that its checksum
     matches the expected base checksum. */

  /* The incoming delta is targeted against EXPECTED_BASE_CHECKSUM. Find and
     check our RECORDED_BASE_CHECKSUM.  (In WC-1, we could not do this test
     for replaced nodes because we didn't store the checksum of the "revert
     base".  In WC-NG, we do and we can.) */
  recorded_base_checksum = fb->original_checksum;

  /* If we have a checksum that we want to compare to a MD5 checksum,
     ensure that it is a MD5 checksum */
  if (recorded_base_checksum
      && expected_base_checksum
      && recorded_base_checksum->kind != svn_checksum_md5)
    SVN_ERR(svn_wc__db_pristine_get_md5(&recorded_base_checksum,
                                        eb->db, eb->wcroot_abspath,
                                        recorded_base_checksum, pool, pool));


  if (!svn_checksum_match(expected_base_checksum, recorded_base_checksum))
      return svn_error_createf(SVN_ERR_WC_CORRUPT_TEXT_BASE, NULL,
                     _("Checksum mismatch for '%s':\n"
                       "   expected:  %s\n"
                       "   recorded:  %s\n"),
                     svn_dirent_local_style(fb->local_abspath, pool),
                     svn_checksum_to_cstring_display(expected_base_checksum,
                                                     pool),
                     svn_checksum_to_cstring_display(recorded_base_checksum,
                                                     pool));

  /* Open the text base for reading, unless this is an added file. */

  /*
     kff todo: what we really need to do here is:

     1. See if there's a file or dir by this name already here.
     2. See if it's under revision control.
     3. If both are true, open text-base.
     4. If only 1 is true, bail, because we can't go destroying user's
        files (or as an alternative to bailing, move it to some tmp
        name and somehow tell the user, but communicating with the
        user without erroring is a whole callback system we haven't
        finished inventing yet.)
  */

  if (! fb->adding_file)
    {
      SVN_ERR_ASSERT(!fb->original_checksum
                     || fb->original_checksum->kind == svn_checksum_sha1);

      SVN_ERR(svn_wc__db_pristine_read(&source, NULL, fb->edit_baton->db,
                                       fb->local_abspath,
                                       fb->original_checksum,
                                       handler_pool, handler_pool));
    }
  else
    {
      source = svn_stream_empty(handler_pool);
    }

  /* If we don't have a recorded checksum, use the ra provided checksum */
  if (!recorded_base_checksum)
    recorded_base_checksum = expected_base_checksum;

  /* Checksum the text base while applying deltas */
  if (recorded_base_checksum)
    {
      hb->expected_source_checksum = svn_checksum_dup(recorded_base_checksum,
                                                      handler_pool);

      /* Wrap stream and store reference to allow calculating the
         checksum. */
      source = svn_stream_checksummed2(source,
                                       &hb->actual_source_checksum,
                                       NULL, recorded_base_checksum->kind,
                                       TRUE, handler_pool);
      hb->source_checksum_stream = source;
    }

  /* Open the text base for writing (this will get us a temporary file).  */
  err = svn_wc__open_writable_base(&target, &hb->new_text_base_tmp_abspath,
                                   NULL, &hb->new_text_base_sha1_checksum,
                                   fb->edit_baton->db, eb->wcroot_abspath,
                                   handler_pool, pool);
  if (err)
    {
      svn_pool_destroy(handler_pool);
      return svn_error_trace(err);
    }

  /* Prepare to apply the delta.  */
  svn_txdelta_apply(source, target,
                    hb->new_text_base_md5_digest,
                    hb->new_text_base_tmp_abspath /* error_info */,
                    handler_pool,
                    &hb->apply_handler, &hb->apply_baton);

  hb->pool = handler_pool;
  hb->fb = fb;

  /* We're all set.  */
  *handler_baton = hb;
  *handler = window_handler;

  return SVN_NO_ERROR;
}


/* An svn_delta_editor_t function. */
static svn_error_t *
change_file_prop(void *file_baton,
                 const char *name,
                 const svn_string_t *value,
                 apr_pool_t *scratch_pool)
{
  struct file_baton *fb = file_baton;
  svn_prop_t *propchange;

  if (fb->skip_this)
    return SVN_NO_ERROR;

  /* Push a new propchange to the file baton's array of propchanges */
  propchange = apr_array_push(fb->propchanges);
  propchange->name = apr_pstrdup(fb->pool, name);
  propchange->value = value ? svn_string_dup(value, fb->pool) : NULL;

  if (!fb->edited && svn_property_kind(NULL, name) == svn_prop_regular_kind)
    SVN_ERR(mark_file_edited(fb, scratch_pool));

  if (! fb->shadowed
      && strcmp(name, SVN_PROP_SPECIAL) == 0)
    {
      struct edit_baton *eb = fb->edit_baton;
      svn_boolean_t modified = FALSE;
      svn_boolean_t becomes_symlink;
      svn_boolean_t was_symlink;

      /* Let's see if we have a change as in some scenarios servers report
         non-changes of properties. */
      becomes_symlink = (value != NULL);

      if (fb->adding_file)
        was_symlink = becomes_symlink; /* No change */
      else
        {
          apr_hash_t *props;

          /* We read the server-props, not the ACTUAL props here as we just
             want to see if this is really an incoming prop change. */
          SVN_ERR(svn_wc__db_base_get_props(&props, eb->db,
                                            fb->local_abspath,
                                            scratch_pool, scratch_pool));

          was_symlink = ((props
                              && apr_hash_get(props, SVN_PROP_SPECIAL,
                                              APR_HASH_KEY_STRING) != NULL)
                              ? svn_tristate_true
                              : svn_tristate_false);
        }

      if (was_symlink != becomes_symlink)
        {
          /* If the local node was not modified, we continue as usual, if
             modified we want a tree conflict just like how we would handle
             it when receiving a delete + add (aka "replace") */
          if (fb->local_prop_mods)
            modified = TRUE;
          else
            SVN_ERR(svn_wc__internal_file_modified_p(&modified, eb->db,
                                                     fb->local_abspath,
                                                     FALSE, scratch_pool));
        }

      if (modified)
        {
          svn_wc_conflict_description2_t *tree_conflict = NULL;

          /* Create a copy of the existing (pre update) BASE node in WORKING,
             mark a tree conflict and handle the rest of the update as
             shadowed */
          SVN_ERR(svn_wc__db_temp_op_make_copy(eb->db, fb->local_abspath,
                                               scratch_pool));
          /* ### Performance: We should just create the conflict here, without
             ### verifying again */
          SVN_ERR(check_tree_conflict(&tree_conflict, eb, fb->local_abspath,
                                      svn_wc__db_status_added,
                                      svn_wc__db_kind_file, TRUE,
                                      svn_wc_conflict_action_edit,
                                      svn_node_file, fb->new_relpath,
                                      scratch_pool, scratch_pool));
          SVN_ERR_ASSERT(tree_conflict != NULL);
          SVN_ERR(svn_wc__db_op_set_tree_conflict(eb->db,
                                                  fb->local_abspath,
                                                  tree_conflict,
                                                  scratch_pool));
          fb->edit_conflict = tree_conflict;

          do_notification(eb, fb->local_abspath, svn_node_file,
                          svn_wc_notify_tree_conflict, scratch_pool);

          /* Ok, we introduced a replacement, so we can now handle the rest
             as a normal shadowed update */
          fb->shadowed = TRUE;
          fb->add_existed = FALSE;
          fb->already_notified = TRUE;
      }
    }

  return SVN_NO_ERROR;
}

/* Perform the actual merge of file changes between an original file,
   identified by ORIGINAL_CHECKSUM (an empty file if NULL) to a new file
   identified by NEW_CHECKSUM.

   Merge the result into LOCAL_ABSPATH, which is part of the working copy
   identified by WRI_ABSPATH. Use OLD_REVISION and TARGET_REVISION for naming
   the intermediate files.

   The rest of the arguments are passed to svn_wc__internal_merge.
 */
svn_error_t *
svn_wc__perform_file_merge(svn_skel_t **work_items,
                           enum svn_wc_merge_outcome_t *merge_outcome,
                           svn_wc__db_t *db,
                           const char *local_abspath,
                           const char *wri_abspath,
                           const svn_checksum_t *new_checksum,
                           const svn_checksum_t *original_checksum,
                           apr_hash_t *actual_props,
                           const apr_array_header_t *ext_patterns,
                           svn_revnum_t old_revision,
                           svn_revnum_t target_revision,
                           const apr_array_header_t *propchanges,
                           const char *diff3_cmd,
                           svn_wc_conflict_resolver_func2_t conflict_func,
                           void *conflict_baton,
                           svn_cancel_func_t cancel_func,
                           void *cancel_baton,
                           apr_pool_t *result_pool,
                           apr_pool_t *scratch_pool)
{
  /* Actual file exists and has local mods:
     Now we need to let loose svn_wc__merge_internal() to merge
     the textual changes into the working file. */
  const char *oldrev_str, *newrev_str, *mine_str;
  const char *merge_left;
  svn_boolean_t delete_left = FALSE;
  const char *path_ext = "";
  const char *new_text_base_tmp_abspath;
  svn_skel_t *work_item;

  *work_items = NULL;

  SVN_ERR(svn_wc__db_pristine_get_path(&new_text_base_tmp_abspath,
                                       db, wri_abspath, new_checksum,
                                       scratch_pool, scratch_pool));

  /* If we have any file extensions we're supposed to
     preserve in generated conflict file names, then find
     this path's extension.  But then, if it isn't one of
     the ones we want to keep in conflict filenames,
     pretend it doesn't have an extension at all. */
  if (ext_patterns && ext_patterns->nelts)
    {
      svn_path_splitext(NULL, &path_ext, local_abspath, scratch_pool);
      if (! (*path_ext && svn_cstring_match_glob_list(path_ext, ext_patterns)))
        path_ext = "";
    }

  /* old_revision can be invalid when the conflict is against a
     local addition */
  if (!SVN_IS_VALID_REVNUM(old_revision))
    old_revision = 0;

  oldrev_str = apr_psprintf(scratch_pool, ".r%ld%s%s",
                            old_revision,
                            *path_ext ? "." : "",
                            *path_ext ? path_ext : "");

  newrev_str = apr_psprintf(scratch_pool, ".r%ld%s%s",
                            target_revision,
                            *path_ext ? "." : "",
                            *path_ext ? path_ext : "");
  mine_str = apr_psprintf(scratch_pool, ".mine%s%s",
                          *path_ext ? "." : "",
                          *path_ext ? path_ext : "");

  if (! original_checksum)
    {
      SVN_ERR(get_empty_tmp_file(&merge_left, db, wri_abspath,
                                 result_pool, scratch_pool));
      delete_left = TRUE;
    }
  else
    SVN_ERR(svn_wc__db_pristine_get_path(&merge_left, db, wri_abspath,
                                         original_checksum,
                                         result_pool, scratch_pool));

  /* Merge the changes from the old textbase to the new
     textbase into the file we're updating.
     Remember that this function wants full paths! */
  /* ### TODO: Pass version info here. */
  SVN_ERR(svn_wc__internal_merge(&work_item,
                                 merge_outcome,
                                 db,
                                 merge_left, NULL,
                                 new_text_base_tmp_abspath, NULL,
                                 local_abspath,
                                 wri_abspath,
                                 oldrev_str, newrev_str, mine_str,
                                 actual_props,
                                 FALSE /* dry_run */,
                                 diff3_cmd, NULL, propchanges,
                                 conflict_func, conflict_baton,
                                 cancel_func, cancel_baton,
                                 result_pool, scratch_pool));

  *work_items = svn_wc__wq_merge(*work_items, work_item, result_pool);

  /* If we created a temporary left merge file, get rid of it. */
  if (delete_left)
    {
      SVN_ERR(svn_wc__wq_build_file_remove(&work_item, db, merge_left,
                                           result_pool, scratch_pool));
      *work_items = svn_wc__wq_merge(*work_items, work_item, result_pool);
    }

  return SVN_NO_ERROR;
}

/* This is the small planet.  It has the complex responsibility of
 * "integrating" a new revision of a file into a working copy.
 *
 * Given a file_baton FB for a file either already under version control, or
 * prepared (see below) to join version control, fully install a
 * new revision of the file.
 *
 * ### transitional: installation of the working file will be handled
 * ### by the *INSTALL_PRISTINE flag.
 *
 * By "install", we mean: create a new text-base and prop-base, merge
 * any textual and property changes into the working file, and finally
 * update all metadata so that the working copy believes it has a new
 * working revision of the file.  All of this work includes being
 * sensitive to eol translation, keyword substitution, and performing
 * all actions accumulated the parent directory's work queue.
 *
 * Set *CONTENT_STATE to the state of the contents after the
 * installation.
 *
 * Return values are allocated in RESULT_POOL and temporary allocations
 * are performed in SCRATCH_POOL.
 */
static svn_error_t *
merge_file(svn_skel_t **work_items,
           svn_boolean_t *install_pristine,
           const char **install_from,
           svn_wc_notify_state_t *content_state,
           struct file_baton *fb,
           apr_hash_t *actual_props,
           apr_time_t last_changed_date,
           apr_pool_t *result_pool,
           apr_pool_t *scratch_pool)
{
  struct edit_baton *eb = fb->edit_baton;
  struct dir_baton *pb = fb->dir_baton;
  svn_boolean_t is_locally_modified;
  enum svn_wc_merge_outcome_t merge_outcome = svn_wc_merge_unchanged;
  svn_skel_t *work_item;

  SVN_ERR_ASSERT(! fb->shadowed && !fb->obstruction_found);

  /*
     When this function is called on file F, we assume the following
     things are true:

         - The new pristine text of F is present in the pristine store
           iff FB->NEW_TEXT_BASE_SHA1_CHECKSUM is not NULL.

         - The WC metadata still reflects the old version of F.
           (We can still access the old pristine base text of F.)

     The goal is to update the local working copy of F to reflect
     the changes received from the repository, preserving any local
     modifications.
  */

  *work_items = NULL;
  *install_pristine = FALSE;
  *install_from = NULL;

  /* Start by splitting the file path, getting an access baton for the parent,
     and an entry for the file if any. */

  /* Has the user made local mods to the working file?
     Note that this compares to the current pristine file, which is
     different from fb->old_text_base_path if we have a replaced-with-history
     file.  However, in the case we had an obstruction, we check against the
     new text base.
   */
  if (fb->adding_file && !fb->add_existed)
    {
      is_locally_modified = FALSE; /* There is no file: Don't check */
    }
  else
    {
      /* The working file is not an obstruction.
         So: is the file modified, relative to its ORIGINAL pristine?

         This function sets is_locally_modified to FALSE for
         files that do not exist and for directories. */

      SVN_ERR(svn_wc__internal_file_modified_p(&is_locally_modified,
                                               eb->db, fb->local_abspath,
                                               FALSE /* exact_comparison */,
                                               scratch_pool));
    }

  /* For 'textual' merging, we use the following system:

     When a file is modified and we have a new BASE:
      - For text files
          * svn_wc_merge uses diff3
          * possibly makes backups and marks files as conflicted.

      - For binary files
          * svn_wc_merge makes backups and marks files as conflicted.

     If a file is not modified and we have a new BASE:
       * Install from pristine.

     If we have property changes related to magic properties or if the
     svn:keywords property is set:
       * Retranslate from the working file.
   */
  if (! is_locally_modified
      && fb->new_text_base_sha1_checksum)
    {
          /* If there are no local mods, who cares whether it's a text
             or binary file!  Just write a log command to overwrite
             any working file with the new text-base.  If newline
             conversion or keyword substitution is activated, this
             will happen as well during the copy.
             For replaced files, though, we want to merge in the changes
             even if the file is not modified compared to the (non-revert)
             text-base. */

      *install_pristine = TRUE;
    }
  else if (fb->new_text_base_sha1_checksum)
    {
      /* Actual file exists and has local mods:
         Now we need to let loose svn_wc__merge_internal() to merge
         the textual changes into the working file. */
      SVN_ERR(svn_wc__perform_file_merge(work_items,
                                         &merge_outcome,
                                         eb->db,
                                         fb->local_abspath,
                                         pb->local_abspath,
                                         fb->new_text_base_sha1_checksum,
                                         fb->add_existed
                                                  ? NULL
                                                  : fb->original_checksum,
                                         actual_props,
                                         eb->ext_patterns,
                                         fb->old_revision,
                                         *eb->target_revision,
                                         fb->propchanges,
                                         eb->diff3_cmd,
                                         eb->conflict_func, eb->conflict_baton,
                                         eb->cancel_func, eb->cancel_baton,
                                         result_pool, scratch_pool));
    } /* end: working file exists and has mods */
  else
    {
      /* There is no new text base, but let's see if the working file needs
         to be updated for any other reason. */

      apr_hash_t *keywords;

      /* Determine if any of the propchanges are the "magic" ones that
         might require changing the working file. */
      svn_boolean_t magic_props_changed;

      magic_props_changed = svn_wc__has_magic_property(fb->propchanges);

      SVN_ERR(svn_wc__get_translate_info(NULL, NULL,
                                         &keywords,
                                         NULL,
                                         eb->db, fb->local_abspath,
                                         actual_props, TRUE,
                                         scratch_pool, scratch_pool));
      if (magic_props_changed || keywords)
        {
          /* Special edge-case: it's possible that this file installation
             only involves propchanges, but that some of those props still
             require a retranslation of the working file.

             OR that the file doesn't involve propchanges which by themselves
             require retranslation, but receiving a change bumps the revision
             number which requires re-expansion of keywords... */

          if (is_locally_modified)
            {
              const char *tmptext;

              /* Copy and DEtranslate the working file to a temp text-base.
                 Note that detranslation is done according to the old props. */
              SVN_ERR(svn_wc__internal_translated_file(
                        &tmptext, fb->local_abspath, eb->db, fb->local_abspath,
                        SVN_WC_TRANSLATE_TO_NF
                          | SVN_WC_TRANSLATE_NO_OUTPUT_CLEANUP,
                        eb->cancel_func, eb->cancel_baton,
                        result_pool, scratch_pool));

              /* We always want to reinstall the working file if the magic
                 properties have changed, or there are any keywords present.
                 Note that TMPTEXT might actually refer to the working file
                 itself (the above function skips a detranslate when not
                 required). This is acceptable, as we will (re)translate
                 according to the new properties into a temporary file (from
                 the working file), and then rename the temp into place. Magic!
               */
              *install_pristine = TRUE;
              *install_from = tmptext;
            }
          else
            {
              /* Use our existing 'copy' from the pristine store instead
                 of making a new copy. This way we can use the standard code
                 to update the recorded size and modification time.
                 (Issue #3842) */
              *install_pristine = TRUE;
            }
        }
    }

  /* Installing from a pristine will handle timestamps and recording.
     However, if we are NOT creating a new working copy file, then create
     work items to handle the recording of the timestamp and working-size. */
  if (!*install_pristine
      && !is_locally_modified)
    {
      apr_time_t set_date = 0;

      if (eb->use_commit_times && last_changed_date != 0)
        {
          set_date = last_changed_date;
        }

      SVN_ERR(svn_wc__wq_build_record_fileinfo(&work_item,
                                               eb->db, fb->local_abspath,
                                               set_date,
                                               result_pool, scratch_pool));
      *work_items = svn_wc__wq_merge(*work_items, work_item, result_pool);
    }

  /* Set the returned content state. */

  /* This is kind of interesting.  Even if no new text was
     installed (i.e., NEW_TEXT_BASE_ABSPATH was null), we could still
     report a pre-existing conflict state.  Say a file, already
     in a state of textual conflict, receives prop mods during an
     update.  Then we'll notify that it has text conflicts.  This
     seems okay to me.  I guess.  I dunno.  You? */

  if (merge_outcome == svn_wc_merge_conflict)
    *content_state = svn_wc_notify_state_conflicted;
  else if (fb->new_text_base_sha1_checksum)
    {
      if (is_locally_modified)
        *content_state = svn_wc_notify_state_merged;
      else
        *content_state = svn_wc_notify_state_changed;
    }
  else
    *content_state = svn_wc_notify_state_unchanged;

  return SVN_NO_ERROR;
}


/* An svn_delta_editor_t function. */
/* Mostly a wrapper around merge_file. */
static svn_error_t *
close_file(void *file_baton,
           const char *expected_md5_digest,
           apr_pool_t *pool)
{
  struct file_baton *fb = file_baton;
  struct edit_baton *eb = fb->edit_baton;
  svn_wc_notify_state_t content_state, prop_state;
  svn_wc_notify_lock_state_t lock_state;
  svn_checksum_t *expected_md5_checksum = NULL;
  apr_hash_t *new_base_props = NULL;
  apr_hash_t *new_actual_props = NULL;
  apr_array_header_t *entry_prop_changes;
  apr_array_header_t *dav_prop_changes;
  apr_array_header_t *regular_prop_changes;
  apr_hash_t *current_base_props = NULL;
  apr_hash_t *current_actual_props = NULL;
  apr_hash_t *local_actual_props = NULL;
  svn_skel_t *all_work_items = NULL;
  svn_skel_t *work_item;
  apr_pool_t *scratch_pool = fb->pool; /* Destroyed at function exit */
  svn_boolean_t keep_recorded_info = FALSE;

  if (fb->skip_this)
    {
      SVN_ERR(maybe_release_dir_info(fb->bump_info));
      svn_pool_destroy(fb->pool);
      return SVN_NO_ERROR;
    }

  if (expected_md5_digest)
    SVN_ERR(svn_checksum_parse_hex(&expected_md5_checksum, svn_checksum_md5,
                                   expected_md5_digest, scratch_pool));

  if (fb->new_text_base_md5_checksum && expected_md5_checksum
      && !svn_checksum_match(expected_md5_checksum,
                             fb->new_text_base_md5_checksum))
    return svn_checksum_mismatch_err(expected_md5_checksum,
                            fb->new_text_base_md5_checksum, scratch_pool,
                            _("Checksum mismatch for '%s'"),
                            svn_dirent_local_style(fb->local_abspath, pool));

  /* Gather the changes for each kind of property.  */
  SVN_ERR(svn_categorize_props(fb->propchanges, &entry_prop_changes,
                               &dav_prop_changes, &regular_prop_changes,
                               scratch_pool));

  /* Extract the changed_* and lock state information.  */
  {
    svn_revnum_t new_changed_rev;
    apr_time_t new_changed_date;
    const char *new_changed_author;

    SVN_ERR(accumulate_last_change(&new_changed_rev,
                                   &new_changed_date,
                                   &new_changed_author,
                                   entry_prop_changes,
                                   scratch_pool, scratch_pool));

    if (SVN_IS_VALID_REVNUM(new_changed_rev))
      fb->changed_rev = new_changed_rev;
    if (new_changed_date != 0)
      fb->changed_date = new_changed_date;
    if (new_changed_author != NULL)
      fb->changed_author = new_changed_author;
  }

  /* Determine whether the file has become unlocked.  */
  {
    int i;

    lock_state = svn_wc_notify_lock_state_unchanged;

    for (i = 0; i < entry_prop_changes->nelts; ++i)
      {
        const svn_prop_t *prop
          = &APR_ARRAY_IDX(entry_prop_changes, i, svn_prop_t);

        /* If we see a change to the LOCK_TOKEN entry prop, then the only
           possible change is its REMOVAL. Thus, the lock has been removed,
           and we should likewise remove our cached copy of it.  */
        if (! strcmp(prop->name, SVN_PROP_ENTRY_LOCK_TOKEN))
          {
            SVN_ERR_ASSERT(prop->value == NULL);
            SVN_ERR(svn_wc__db_lock_remove(eb->db, fb->local_abspath,
                                           scratch_pool));

            lock_state = svn_wc_notify_lock_state_unlocked;
            break;
          }
      }
  }

  /* Install all kinds of properties.  It is important to do this before
     any file content merging, since that process might expand keywords, in
     which case we want the new entryprops to be in place. */

  /* Write log commands to merge REGULAR_PROPS into the existing
     properties of FB->LOCAL_ABSPATH.  Update *PROP_STATE to reflect
     the result of the regular prop merge.

     BASE_PROPS and WORKING_PROPS are hashes of the base and
     working props of the file; if NULL they are read from the wc.  */

  /* ### some of this feels like voodoo... */

  if ((!fb->adding_file || fb->add_existed)
      && !fb->shadowed)
    SVN_ERR(svn_wc__get_actual_props(&local_actual_props,
                                     eb->db, fb->local_abspath,
                                     scratch_pool, scratch_pool));
  if (local_actual_props == NULL)
    local_actual_props = apr_hash_make(scratch_pool);

  if (fb->add_existed)
    {
      /* This node already exists. Grab the current pristine properties. */
      SVN_ERR(svn_wc__get_pristine_props(&current_base_props,
                                         eb->db, fb->local_abspath,
                                         scratch_pool, scratch_pool));
      current_actual_props = local_actual_props;
    }
  else if (!fb->adding_file)
    {
      /* Get the BASE properties for proper merging. */
      SVN_ERR(svn_wc__db_base_get_props(&current_base_props,
                                        eb->db, fb->local_abspath,
                                        scratch_pool, scratch_pool));
      current_actual_props = local_actual_props;
    }

  /* Note: even if the node existed before, it may not have
     pristine props (e.g a local-add)  */
  if (current_base_props == NULL)
    current_base_props = apr_hash_make(scratch_pool);

  /* And new nodes need an empty set of ACTUAL props.  */
  if (current_actual_props == NULL)
    current_actual_props = apr_hash_make(scratch_pool);

  prop_state = svn_wc_notify_state_unknown;

  if (! fb->shadowed)
    {
      svn_boolean_t install_pristine;
      const char *install_from = NULL;

      /* Merge the 'regular' props into the existing working proplist. */
      /* This will merge the old and new props into a new prop db, and
         write <cp> commands to the logfile to install the merged
         props.  */
      SVN_ERR(svn_wc__merge_props(&work_item,
                                  &prop_state,
                                  &new_base_props,
                                  &new_actual_props,
                                  eb->db,
                                  fb->local_abspath,
                                  svn_wc__db_kind_file,
                                  NULL /* left_version */,
                                  NULL /* right_version */,
                                  NULL /* server_baseprops (update, not merge)  */,
                                  current_base_props,
                                  current_actual_props,
                                  regular_prop_changes, /* propchanges */
                                  TRUE /* base_merge */,
                                  FALSE /* dry_run */,
                                  eb->conflict_func, eb->conflict_baton,
                                  eb->cancel_func, eb->cancel_baton,
                                  scratch_pool,
                                  scratch_pool));
      /* We will ALWAYS have properties to save (after a not-dry-run merge). */
      SVN_ERR_ASSERT(new_base_props != NULL && new_actual_props != NULL);
      all_work_items = svn_wc__wq_merge(all_work_items, work_item,
                                        scratch_pool);

      /* Merge the text. This will queue some additional work.  */
      if (!fb->obstruction_found)
        {
          svn_error_t *err;
          err = merge_file(&work_item, &install_pristine, &install_from,
                           &content_state, fb, current_actual_props,
                           fb->changed_date, scratch_pool, scratch_pool);

          if (err && err->apr_err == SVN_ERR_WC_PATH_ACCESS_DENIED)
            {
              if (eb->notify_func)
                {
                  svn_wc_notify_t *notify =svn_wc_create_notify(
                                fb->local_abspath,
                                svn_wc_notify_update_skip_access_denied,
                                scratch_pool);

                  notify->kind = svn_node_file;
                  notify->err = err;

                  eb->notify_func(eb->notify_baton, notify, scratch_pool);
                }
              svn_error_clear(err);

              SVN_ERR(remember_skipped_tree(eb, fb->local_abspath,
                                            scratch_pool));
              fb->skip_this = TRUE;

              SVN_ERR(maybe_release_dir_info(fb->bump_info));
              svn_pool_destroy(fb->pool);
              return SVN_NO_ERROR;
            }
          else
            SVN_ERR(err);

          all_work_items = svn_wc__wq_merge(all_work_items, work_item,
                                            scratch_pool);
        }
      else
        {
          install_pristine = FALSE;
          if (fb->new_text_base_sha1_checksum)
            content_state = svn_wc_notify_state_changed;
          else
            content_state = svn_wc_notify_state_unchanged;
        }

      if (install_pristine)
        {
          svn_boolean_t record_fileinfo;

          /* If we are installing from the pristine contents, then go ahead and
             record the fileinfo. That will be the "proper" values. Installing
             from some random file means the fileinfo does NOT correspond to
             the pristine (in which case, the fileinfo will be cleared for
             safety's sake).  */
          record_fileinfo = (install_from == NULL);

          SVN_ERR(svn_wc__wq_build_file_install(&work_item,
                                                eb->db,
                                                fb->local_abspath,
                                                install_from,
                                                eb->use_commit_times,
                                                record_fileinfo,
                                                scratch_pool, scratch_pool));
          all_work_items = svn_wc__wq_merge(all_work_items, work_item,
                                            scratch_pool);
        }
      else if (lock_state == svn_wc_notify_lock_state_unlocked
               && !fb->obstruction_found)
        {
          /* If a lock was removed and we didn't update the text contents, we
             might need to set the file read-only.

             Note: this will also update the executable flag, but ... meh.  */
          SVN_ERR(svn_wc__wq_build_sync_file_flags(&work_item, eb->db,
                                                   fb->local_abspath,
                                                   scratch_pool, scratch_pool));
          all_work_items = svn_wc__wq_merge(all_work_items, work_item,
                                            scratch_pool);
        }

      if (! install_pristine
          && (content_state == svn_wc_notify_state_unchanged))
        {
          /* It is safe to keep the current recorded timestamp and size */
          keep_recorded_info = TRUE;
        }

      /* Clean up any temporary files.  */

      /* Remove the INSTALL_FROM file, as long as it doesn't refer to the
         working file.  */
      if (install_from != NULL
          && strcmp(install_from, fb->local_abspath) != 0)
        {
          SVN_ERR(svn_wc__wq_build_file_remove(&work_item, eb->db,
                                               install_from,
                                               scratch_pool, scratch_pool));
          all_work_items = svn_wc__wq_merge(all_work_items, work_item,
                                            scratch_pool);
        }
    }
  else
    {
      /* Adding or updating a BASE node under a locally added node. */
      apr_hash_t *fake_actual_props;

      if (fb->adding_file)
        fake_actual_props = apr_hash_make(scratch_pool);
      else
        fake_actual_props = current_base_props;

      /* Store the incoming props (sent as propchanges) in new_base_props
         and create a set of new actual props to use for notifications */
      SVN_ERR(svn_wc__merge_props(&work_item,
                                  &prop_state,
                                  &new_base_props,
                                  &new_actual_props,
                                  eb->db,
                                  fb->local_abspath,
                                  svn_wc__db_kind_file,
                                  NULL /* left_version */,
                                  NULL /* right_version */,
                                  NULL /* server_baseprops (not merging) */,
                                  current_base_props /* base_props */,
                                  fake_actual_props /* working_props */,
                                  regular_prop_changes, /* propchanges */
                                  TRUE /* base_merge */,
                                  FALSE /* dry_run */,
                                  NULL, NULL, /* No conflict handling */
                                  eb->cancel_func, eb->cancel_baton,
                                  scratch_pool,
                                  scratch_pool));

      all_work_items = svn_wc__wq_merge(all_work_items, work_item,
                                        scratch_pool);

      if (fb->new_text_base_sha1_checksum)
        content_state = svn_wc_notify_state_changed;
      else
        content_state = svn_wc_notify_state_unchanged;
    }

  /* Insert/replace the BASE node with all of the new metadata.  */
  {
      /* Set the 'checksum' column of the file's BASE_NODE row to
       * NEW_TEXT_BASE_SHA1_CHECKSUM.  The pristine text identified by that
       * checksum is already in the pristine store. */
    const svn_checksum_t *new_checksum = fb->new_text_base_sha1_checksum;

    /* If we don't have a NEW checksum, then the base must not have changed.
       Just carry over the old checksum.  */
    if (new_checksum == NULL)
      new_checksum = fb->original_checksum;

    SVN_ERR(svn_wc__db_base_add_file(eb->db, fb->local_abspath,
                                     eb->wcroot_abspath,
                                     fb->new_relpath,
                                     eb->repos_root, eb->repos_uuid,
                                     *eb->target_revision,
                                     new_base_props,
                                     fb->changed_rev,
                                     fb->changed_date,
                                     fb->changed_author,
                                     new_checksum,
                                     (dav_prop_changes->nelts > 0)
                                       ? svn_prop_array_to_hash(
                                                        dav_prop_changes,
                                                        scratch_pool)
                                       : NULL,
                                     NULL /* conflict */,
                                     (! fb->shadowed) && new_base_props,
                                     new_actual_props,
                                     keep_recorded_info,
                                     (fb->shadowed && fb->obstruction_found),
                                     all_work_items,
                                     scratch_pool));
  }

  /* Deal with the WORKING tree, based on updates to the BASE tree.  */

  /* If this file was locally-added and is now being added by the update, we
     can toss the local-add, turning this into a local-edit.
     If the local file is replaced, we don't want to touch ACTUAL. */
  if (fb->add_existed && fb->adding_file)
    {
      SVN_ERR(svn_wc__db_temp_op_remove_working(eb->db, fb->local_abspath,
                                                scratch_pool));
    }

  apr_hash_set(fb->dir_baton->not_present_files, fb->name,
               APR_HASH_KEY_STRING, NULL);

  /* Send a notification to the callback function.  (Skip notifications
     about files which were already notified for another reason.) */
  if (eb->notify_func && !fb->already_notified
      && (fb->edited || lock_state == svn_wc_notify_lock_state_unlocked))
    {
      svn_wc_notify_t *notify;
      svn_wc_notify_action_t action = svn_wc_notify_update_update;

      if (fb->edited)
        {
          if (fb->shadowed)
            action = fb->adding_file
                            ? svn_wc_notify_update_shadowed_add
                            : svn_wc_notify_update_shadowed_update;
          else if (fb->obstruction_found || fb->add_existed)
            {
              if (content_state != svn_wc_notify_state_conflicted)
                action = svn_wc_notify_exists;
            }
          else if (fb->adding_file)
            {
              action = svn_wc_notify_update_add;
            }
        }

      notify = svn_wc_create_notify(fb->local_abspath, action, scratch_pool);
      notify->kind = svn_node_file;
      notify->content_state = content_state;
      notify->prop_state = prop_state;
      notify->lock_state = lock_state;
      notify->revision = *eb->target_revision;
      notify->old_revision = fb->old_revision;

      /* Fetch the mimetype from the actual properties */
      notify->mime_type = svn_prop_get_value(new_actual_props,
                                             SVN_PROP_MIME_TYPE);

      eb->notify_func(eb->notify_baton, notify, scratch_pool);
    }

  /* We have one less referrer to the directory's bump information. */
  SVN_ERR(maybe_release_dir_info(fb->bump_info));

  svn_pool_destroy(fb->pool); /* Destroy scratch_pool */

  return SVN_NO_ERROR;
}


/* An svn_delta_editor_t function. */
static svn_error_t *
close_edit(void *edit_baton,
           apr_pool_t *pool)
{
  struct edit_baton *eb = edit_baton;
  apr_pool_t *scratch_pool = eb->pool;

  /* The editor didn't even open the root; we have to take care of
     some cleanup stuffs. */
  if (! eb->root_opened
      && *eb->target_basename == '\0')
    {
      /* We need to "un-incomplete" the root directory. */
      SVN_ERR(svn_wc__db_temp_op_end_directory_update(eb->db,
                                                      eb->anchor_abspath,
                                                      scratch_pool));
    }

  /* By definition, anybody "driving" this editor for update or switch
     purposes at a *minimum* must have called set_target_revision() at
     the outset, and close_edit() at the end -- even if it turned out
     that no changes ever had to be made, and open_root() was never
     called.  That's fine.  But regardless, when the edit is over,
     this editor needs to make sure that *all* paths have had their
     revisions bumped to the new target revision. */

  /* Make sure our update target now has the new working revision.
     Also, if this was an 'svn switch', then rewrite the target's
     url.  All of this tweaking might happen recursively!  Note
     that if eb->target is NULL, that's okay (albeit "sneaky",
     some might say).  */

  /* Extra check: if the update did nothing but make its target
     'deleted', then do *not* run cleanup on the target, as it
     will only remove the deleted entry!  */
  if (! eb->target_deleted)
    {
      SVN_ERR(svn_wc__db_op_bump_revisions_post_update(eb->db,
                                                       eb->target_abspath,
                                                       eb->requested_depth,
                                                       eb->switch_relpath,
                                                       eb->repos_root,
                                                       eb->repos_uuid,
                                                       *(eb->target_revision),
                                                       eb->skipped_trees,
                                                       eb->pool));

      if (*eb->target_basename != '\0')
        {
          svn_wc__db_status_t status;
          svn_error_t *err;

          /* Note: we are fetching information about the *target*, not anchor.
             There is no guarantee that the target has a BASE node.
             For example:

               The node was not present in BASE, but locally-added, and the
               update did not create a new BASE node "under" the local-add.

             If there is no BASE node for the target, then we certainly don't
             have to worry about removing it. */
          err = svn_wc__db_base_get_info(&status, NULL, NULL, NULL, NULL, NULL,
                                         NULL, NULL, NULL, NULL, NULL, NULL,
                                         NULL, NULL, NULL,
                                         eb->db, eb->target_abspath,
                                         scratch_pool, scratch_pool);
          if (err)
            {
              if (err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND)
                return svn_error_trace(err);

              svn_error_clear(err);
            }
          else if (status == svn_wc__db_status_excluded)
            {
              /* There is a small chance that the explicit target of an update/
                 switch is gone in the repository, in that specific case the
                 node hasn't been re-added to the BASE tree by this update.

                 If so, we should get rid of this excluded node now. */

              SVN_ERR(svn_wc__db_base_remove(eb->db, eb->target_abspath,
                                             scratch_pool));
            }
        }
    }

  /* The edit is over: run the wq with proper cancel support,
     but first kill the handler that would run it on the pool
     cleanup at the end of this function. */
  apr_pool_cleanup_kill(eb->pool, eb, cleanup_edit_baton);

  SVN_ERR(svn_wc__wq_run(eb->db, eb->wcroot_abspath,
                         eb->cancel_func, eb->cancel_baton,
                         eb->pool));

  /* The edit is over, free its pool.
     ### No, this is wrong.  Who says this editor/baton won't be used
     again?  But the change is not merely to remove this call.  We
     should also make eb->pool not be a subpool (see make_editor),
     and change callers of svn_client_{checkout,update,switch} to do
     better pool management. ### */

  svn_pool_destroy(eb->pool);

  return SVN_NO_ERROR;
}



/*** Returning editors. ***/

/* Helper for the three public editor-supplying functions. */
static svn_error_t *
make_editor(svn_revnum_t *target_revision,
            svn_wc__db_t *db,
            const char *anchor_abspath,
            const char *target_basename,
            svn_boolean_t use_commit_times,
            const char *switch_url,
            svn_depth_t depth,
            svn_boolean_t depth_is_sticky,
            svn_boolean_t allow_unver_obstructions,
            svn_boolean_t adds_as_modification,
            svn_boolean_t server_performs_filtering,
            svn_boolean_t clean_checkout,
            svn_wc_notify_func2_t notify_func,
            void *notify_baton,
            svn_cancel_func_t cancel_func,
            void *cancel_baton,
            svn_wc_dirents_func_t fetch_dirents_func,
            void *fetch_dirents_baton,
            svn_wc_conflict_resolver_func2_t conflict_func,
            void *conflict_baton,
            svn_wc_external_update_t external_func,
            void *external_baton,
            const char *diff3_cmd,
            const apr_array_header_t *preserved_exts,
            const svn_delta_editor_t **editor,
            void **edit_baton,
            apr_pool_t *result_pool,
            apr_pool_t *scratch_pool)
{
  struct edit_baton *eb;
  void *inner_baton;
  apr_pool_t *edit_pool = svn_pool_create(result_pool);
  svn_delta_editor_t *tree_editor = svn_delta_default_editor(edit_pool);
  const svn_delta_editor_t *inner_editor;
  const char *repos_root, *repos_uuid;

  /* An unknown depth can't be sticky. */
  if (depth == svn_depth_unknown)
    depth_is_sticky = FALSE;

  /* Get the anchor's repository root and uuid. The anchor must already exist
     in BASE. */
  SVN_ERR(svn_wc__db_scan_base_repos(NULL, &repos_root, &repos_uuid,
                                     db, anchor_abspath,
                                     result_pool, scratch_pool));

  /* With WC-NG we need a valid repository root */
  SVN_ERR_ASSERT(repos_root != NULL && repos_uuid != NULL);

  /* Disallow a switch operation to change the repository root of the target,
     if that is known. */
  if (switch_url && !svn_uri__is_ancestor(repos_root, switch_url))
    return svn_error_createf(SVN_ERR_WC_INVALID_SWITCH, NULL,
                             _("'%s'\nis not the same repository as\n'%s'"),
                             switch_url, repos_root);

  /* Construct an edit baton. */
  eb = apr_pcalloc(edit_pool, sizeof(*eb));
  eb->pool                     = edit_pool;
  eb->use_commit_times         = use_commit_times;
  eb->target_revision          = target_revision;
  eb->repos_root               = repos_root;
  eb->repos_uuid               = repos_uuid;
  eb->db                       = db;
  eb->target_basename          = target_basename;
  eb->anchor_abspath           = anchor_abspath;

  SVN_ERR(svn_wc__db_get_wcroot(&eb->wcroot_abspath, db, anchor_abspath,
                                edit_pool, scratch_pool));

  if (switch_url)
    eb->switch_relpath =
      svn_uri_skip_ancestor(repos_root, switch_url, scratch_pool);
  else
    eb->switch_relpath = NULL;

  if (svn_path_is_empty(target_basename))
    eb->target_abspath = eb->anchor_abspath;
  else
    eb->target_abspath = svn_dirent_join(eb->anchor_abspath, target_basename,
                                         edit_pool);

  eb->requested_depth          = depth;
  eb->depth_is_sticky          = depth_is_sticky;
  eb->notify_func              = notify_func;
  eb->notify_baton             = notify_baton;
  eb->external_func            = external_func;
  eb->external_baton           = external_baton;
  eb->diff3_cmd                = diff3_cmd;
  eb->cancel_func              = cancel_func;
  eb->cancel_baton             = cancel_baton;
  eb->conflict_func            = conflict_func;
  eb->conflict_baton           = conflict_baton;
  eb->allow_unver_obstructions = allow_unver_obstructions;
  eb->adds_as_modification     = adds_as_modification;
  eb->clean_checkout           = clean_checkout;
  eb->skipped_trees            = apr_hash_make(edit_pool);
  eb->dir_dirents              = apr_hash_make(edit_pool);
  eb->ext_patterns             = preserved_exts;

  apr_pool_cleanup_register(edit_pool, eb, cleanup_edit_baton,
                            cleanup_edit_baton_child);

  /* Construct an editor. */
  tree_editor->set_target_revision = set_target_revision;
  tree_editor->open_root = open_root;
  tree_editor->delete_entry = delete_entry;
  tree_editor->add_directory = add_directory;
  tree_editor->open_directory = open_directory;
  tree_editor->change_dir_prop = change_dir_prop;
  tree_editor->close_directory = close_directory;
  tree_editor->absent_directory = absent_directory;
  tree_editor->add_file = add_file;
  tree_editor->open_file = open_file;
  tree_editor->apply_textdelta = apply_textdelta;
  tree_editor->change_file_prop = change_file_prop;
  tree_editor->close_file = close_file;
  tree_editor->absent_file = absent_file;
  tree_editor->close_edit = close_edit;

  /* Fiddle with the type system. */
  inner_editor = tree_editor;
  inner_baton = eb;

  if (!depth_is_sticky
      && depth != svn_depth_unknown
      && svn_depth_empty <= depth && depth < svn_depth_infinity
      && fetch_dirents_func)
    {
      /* We are asked to perform an update at a depth less than the ambient
         depth. In this case the update won't describe additions that would
         have been reported if we updated at the ambient depth. */
      svn_error_t *err;
      svn_wc__db_kind_t dir_kind;
      svn_wc__db_status_t dir_status;
      const char *dir_repos_relpath;
      svn_depth_t dir_depth;

      /* we have to do this on the target of the update, not the anchor */
      err = svn_wc__db_base_get_info(&dir_status, &dir_kind, NULL,
                                     &dir_repos_relpath, NULL, NULL, NULL,
                                     NULL, NULL, &dir_depth, NULL, NULL, NULL,
                                     NULL, NULL,
                                     db, eb->target_abspath,
                                     scratch_pool, scratch_pool);

      if (!err
          && dir_kind == svn_wc__db_kind_dir
          && dir_status == svn_wc__db_status_normal)
        {
          if (dir_depth > depth)
            {
              apr_hash_t *dirents;

              /* If we switch, we should look at the new relpath */
              if (eb->switch_relpath)
                dir_repos_relpath = eb->switch_relpath;

              SVN_ERR(fetch_dirents_func(fetch_dirents_baton, &dirents,
                                         repos_root, dir_repos_relpath,
                                         edit_pool, scratch_pool));

              if (dirents != NULL && apr_hash_count(dirents))
                apr_hash_set(eb->dir_dirents,
                             apr_pstrdup(edit_pool, dir_repos_relpath),
                             APR_HASH_KEY_STRING,
                             dirents);
            }

          if (depth == svn_depth_immediates)
            {
              /* Worst case scenario of issue #3569 fix: We have to do the
                 same for all existing subdirs, but then we check for
                 svn_depth_empty. */
              const apr_array_header_t *children;
              apr_pool_t *iterpool = svn_pool_create(scratch_pool);
              int i;
              SVN_ERR(svn_wc__db_base_get_children(&children, db,
                                                   eb->target_abspath,
                                                   scratch_pool,
                                                   iterpool));

              for (i = 0; i < children->nelts; i++)
                {
                  const char *child_abspath;
                  const char *child_name;

                  svn_pool_clear(iterpool);

                  child_name = APR_ARRAY_IDX(children, i, const char *);

                  child_abspath = svn_dirent_join(eb->target_abspath,
                                                  child_name, iterpool);

                  SVN_ERR(svn_wc__db_base_get_info(&dir_status, &dir_kind,
                                                   NULL, &dir_repos_relpath,
                                                   NULL, NULL, NULL, NULL,
                                                   NULL, &dir_depth, NULL,
                                                   NULL, NULL, NULL,
                                                   NULL,
                                                   db, child_abspath,
                                                   iterpool, iterpool));

                  if (dir_kind == svn_wc__db_kind_dir
                      && dir_status == svn_wc__db_status_normal
                      && dir_depth > svn_depth_empty)
                    {
                      apr_hash_t *dirents;

                      /* If we switch, we should look at the new relpath */
                      if (eb->switch_relpath)
                        dir_repos_relpath = svn_relpath_join(
                                                eb->switch_relpath,
                                                child_name, iterpool);

                      SVN_ERR(fetch_dirents_func(fetch_dirents_baton, &dirents,
                                                 repos_root, dir_repos_relpath,
                                                 edit_pool, iterpool));

                      if (dirents != NULL && apr_hash_count(dirents))
                        apr_hash_set(eb->dir_dirents,
                                     apr_pstrdup(edit_pool, dir_repos_relpath),
                                     APR_HASH_KEY_STRING,
                                     dirents);
                    }
                }
            }
        }
      else if (err && err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
        svn_error_clear(err);
      else
        SVN_ERR(err);
    }

  /* We need to limit the scope of our operation to the ambient depths
     present in the working copy already, but only if the requested
     depth is not sticky. If a depth was explicitly requested,
     libsvn_delta/depth_filter_editor.c will ensure that we never see
     editor calls that extend beyond the scope of the requested depth.
     But even what we do so might extend beyond the scope of our
     ambient depth.  So we use another filtering editor to avoid
     modifying the ambient working copy depth when not asked to do so.
     (This can also be skipped if the server understands depth.) */
  if (!server_performs_filtering
      && !depth_is_sticky)
    SVN_ERR(svn_wc__ambient_depth_filter_editor(&inner_editor,
                                                &inner_baton,
                                                db,
                                                anchor_abspath,
                                                target_basename,
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


svn_error_t *
svn_wc_get_update_editor4(const svn_delta_editor_t **editor,
                          void **edit_baton,
                          svn_revnum_t *target_revision,
                          svn_wc_context_t *wc_ctx,
                          const char *anchor_abspath,
                          const char *target_basename,
                          svn_boolean_t use_commit_times,
                          svn_depth_t depth,
                          svn_boolean_t depth_is_sticky,
                          svn_boolean_t allow_unver_obstructions,
                          svn_boolean_t adds_as_modification,
                          svn_boolean_t server_performs_filtering,
                          svn_boolean_t clean_checkout,
                          const char *diff3_cmd,
                          const apr_array_header_t *preserved_exts,
                          svn_wc_dirents_func_t fetch_dirents_func,
                          void *fetch_dirents_baton,
                          svn_wc_conflict_resolver_func2_t conflict_func,
                          void *conflict_baton,
                          svn_wc_external_update_t external_func,
                          void *external_baton,
                          svn_cancel_func_t cancel_func,
                          void *cancel_baton,
                          svn_wc_notify_func2_t notify_func,
                          void *notify_baton,
                          apr_pool_t *result_pool,
                          apr_pool_t *scratch_pool)
{
  return make_editor(target_revision, wc_ctx->db, anchor_abspath,
                     target_basename, use_commit_times,
                     NULL, depth, depth_is_sticky, allow_unver_obstructions,
                     adds_as_modification, server_performs_filtering,
                     clean_checkout,
                     notify_func, notify_baton,
                     cancel_func, cancel_baton,
                     fetch_dirents_func, fetch_dirents_baton,
                     conflict_func, conflict_baton,
                     external_func, external_baton,
                     diff3_cmd, preserved_exts, editor, edit_baton,
                     result_pool, scratch_pool);
}

svn_error_t *
svn_wc_get_switch_editor4(const svn_delta_editor_t **editor,
                          void **edit_baton,
                          svn_revnum_t *target_revision,
                          svn_wc_context_t *wc_ctx,
                          const char *anchor_abspath,
                          const char *target_basename,
                          const char *switch_url,
                          svn_boolean_t use_commit_times,
                          svn_depth_t depth,
                          svn_boolean_t depth_is_sticky,
                          svn_boolean_t allow_unver_obstructions,
                          svn_boolean_t server_performs_filtering,
                          const char *diff3_cmd,
                          const apr_array_header_t *preserved_exts,
                          svn_wc_dirents_func_t fetch_dirents_func,
                          void *fetch_dirents_baton,
                          svn_wc_conflict_resolver_func2_t conflict_func,
                          void *conflict_baton,
                          svn_wc_external_update_t external_func,
                          void *external_baton,
                          svn_cancel_func_t cancel_func,
                          void *cancel_baton,
                          svn_wc_notify_func2_t notify_func,
                          void *notify_baton,
                          apr_pool_t *result_pool,
                          apr_pool_t *scratch_pool)
{
  SVN_ERR_ASSERT(switch_url && svn_uri_is_canonical(switch_url, scratch_pool));

  return make_editor(target_revision, wc_ctx->db, anchor_abspath,
                     target_basename, use_commit_times,
                     switch_url,
                     depth, depth_is_sticky, allow_unver_obstructions,
                     FALSE /* adds_as_modification */,
                     server_performs_filtering,
                     FALSE /* clean_checkout */,
                     notify_func, notify_baton,
                     cancel_func, cancel_baton,
                     fetch_dirents_func, fetch_dirents_baton,
                     conflict_func, conflict_baton,
                     external_func, external_baton,
                     diff3_cmd, preserved_exts,
                     editor, edit_baton,
                     result_pool, scratch_pool);
}

/* ABOUT ANCHOR AND TARGET, AND svn_wc_get_actual_target2()

   THE GOAL

   Note the following actions, where X is the thing we wish to update,
   P is a directory whose repository URL is the parent of
   X's repository URL, N is directory whose repository URL is *not*
   the parent directory of X (including the case where N is not a
   versioned resource at all):

      1.  `svn up .' from inside X.
      2.  `svn up ...P/X' from anywhere.
      3.  `svn up ...N/X' from anywhere.

   For the purposes of the discussion, in the '...N/X' situation, X is
   said to be a "working copy (WC) root" directory.

   Now consider the four cases for X's type (file/dir) in the working
   copy vs. the repository:

      A.  dir in working copy, dir in repos.
      B.  dir in working copy, file in repos.
      C.  file in working copy, dir in repos.
      D.  file in working copy, file in repos.

   Here are the results we expect for each combination of the above:

      1A. Successfully update X.
      1B. Error (you don't want to remove your current working
          directory out from underneath the application).
      1C. N/A (you can't be "inside X" if X is a file).
      1D. N/A (you can't be "inside X" if X is a file).

      2A. Successfully update X.
      2B. Successfully update X.
      2C. Successfully update X.
      2D. Successfully update X.

      3A. Successfully update X.
      3B. Error (you can't create a versioned file X inside a
          non-versioned directory).
      3C. N/A (you can't have a versioned file X in directory that is
          not its repository parent).
      3D. N/A (you can't have a versioned file X in directory that is
          not its repository parent).

   To summarize, case 2 always succeeds, and cases 1 and 3 always fail
   (or can't occur) *except* when the target is a dir that remains a
   dir after the update.

   ACCOMPLISHING THE GOAL

   Updates are accomplished by driving an editor, and an editor is
   "rooted" on a directory.  So, in order to update a file, we need to
   break off the basename of the file, rooting the editor in that
   file's parent directory, and then updating only that file, not the
   other stuff in its parent directory.

   Secondly, we look at the case where we wish to update a directory.
   This is typically trivial.  However, one problematic case, exists
   when we wish to update a directory that has been removed from the
   repository and replaced with a file of the same name.  If we root
   our edit at the initial directory, there is no editor mechanism for
   deleting that directory and replacing it with a file (this would be
   like having an editor now anchored on a file, which is disallowed).

   All that remains is to have a function with the knowledge required
   to properly decide where to root our editor, and what to act upon
   with that now-rooted editor.  Given a path to be updated, this
   function should conditionally split that path into an "anchor" and
   a "target", where the "anchor" is the directory at which the update
   editor is rooted (meaning, editor->open_root() is called with
   this directory in mind), and the "target" is the actual intended
   subject of the update.

   svn_wc_get_actual_target2() is that function.

   So, what are the conditions?

   Case I: Any time X is '.' (implying it is a directory), we won't
   lop off a basename.  So we'll root our editor at X, and update all
   of X.

   Cases II & III: Any time we are trying to update some path ...N/X,
   we again will not lop off a basename.  We can't root an editor at
   ...N with X as a target, either because ...N isn't a versioned
   resource at all (Case II) or because X is X is not a child of ...N
   in the repository (Case III).  We root at X, and update X.

   Cases IV-???: We lop off a basename when we are updating a
   path ...P/X, rooting our editor at ...P and updating X, or when X
   is missing from disk.

   These conditions apply whether X is a file or directory.

   ---

   As it turns out, commits need to have a similar check in place,
   too, specifically for the case where a single directory is being
   committed (we have to anchor at that directory's parent in case the
   directory itself needs to be modified).
*/


svn_error_t *
svn_wc__check_wc_root(svn_boolean_t *wc_root,
                      svn_wc__db_kind_t *kind,
                      svn_boolean_t *switched,
                      svn_wc__db_t *db,
                      const char *local_abspath,
                      apr_pool_t *scratch_pool)
{
  const char *parent_abspath, *name;
  const char *repos_relpath, *repos_root, *repos_uuid;
  svn_wc__db_status_t status;
  svn_wc__db_kind_t my_kind;

  if (!kind)
    kind = &my_kind;

  /* Initialize our return values to the most common (code-wise) values. */
  *wc_root = TRUE;
  if (switched)
    *switched = FALSE;

  SVN_ERR(svn_wc__db_read_info(&status, kind, NULL, &repos_relpath,
                               &repos_root, &repos_uuid, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL,
                               db, local_abspath,
                               scratch_pool, scratch_pool));

  if (repos_relpath == NULL)
    {
      /* If we inherit our URL, then we can't be a root, nor switched.  */
      *wc_root = FALSE;
      return SVN_NO_ERROR;
    }
  if (*kind != svn_wc__db_kind_dir)
    {
      /* File/symlinks cannot be a root.  */
      *wc_root = FALSE;
    }
  else if (status == svn_wc__db_status_added
           || status == svn_wc__db_status_deleted)
    {
      *wc_root = FALSE;
    }
  else if (status == svn_wc__db_status_server_excluded
           || status == svn_wc__db_status_excluded
           || status == svn_wc__db_status_not_present)
    {
      return svn_error_createf(
                    SVN_ERR_WC_PATH_NOT_FOUND, NULL,
                    _("The node '%s' was not found."),
                    svn_dirent_local_style(local_abspath, scratch_pool));
    }
  else if (svn_dirent_is_root(local_abspath, strlen(local_abspath)))
    return SVN_NO_ERROR;

  if (!*wc_root && switched == NULL )
    return SVN_NO_ERROR; /* No more info needed */

  svn_dirent_split(&parent_abspath, &name, local_abspath, scratch_pool);

  /* Check if the node is recorded in the parent */
  if (*wc_root)
    {
      svn_boolean_t is_root;
      SVN_ERR(svn_wc__db_is_wcroot(&is_root, db, local_abspath, scratch_pool));

      if (is_root)
        {
          /* We're not in the (versioned) parent directory's list of
             children, so we must be the root of a distinct working copy.  */
          return SVN_NO_ERROR;
        }
    }

  {
    const char *parent_repos_root;
    const char *parent_repos_relpath;
    const char *parent_repos_uuid;

    SVN_ERR(svn_wc__db_scan_base_repos(&parent_repos_relpath,
                                       &parent_repos_root,
                                       &parent_repos_uuid,
                                       db, parent_abspath,
                                       scratch_pool, scratch_pool));

    if (strcmp(repos_root, parent_repos_root) != 0
        || strcmp(repos_uuid, parent_repos_uuid) != 0)
      {
        /* This should never happen (### until we get mixed-repos working
           copies). If we're in the parent, then we should be from the
           same repository. For this situation, just declare us the root
           of a separate, unswitched working copy.  */
        return SVN_NO_ERROR;
      }

    *wc_root = FALSE;

    if (switched)
      {
        const char *expected_relpath = svn_relpath_join(parent_repos_relpath,
                                                        name, scratch_pool);

        *switched = (strcmp(expected_relpath, repos_relpath) != 0);
      }
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc_is_wc_root2(svn_boolean_t *wc_root,
                   svn_wc_context_t *wc_ctx,
                   const char *local_abspath,
                   apr_pool_t *scratch_pool)
{
  svn_boolean_t is_root;
  svn_boolean_t is_switched;
  svn_wc__db_kind_t kind;
  svn_error_t *err;
  SVN_ERR_ASSERT(svn_dirent_is_absolute(local_abspath));

  err = svn_wc__check_wc_root(&is_root, &kind, &is_switched,
                              wc_ctx->db, local_abspath, scratch_pool);

  if (err)
    {
      if (err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND &&
          err->apr_err != SVN_ERR_WC_NOT_WORKING_COPY)
        return svn_error_trace(err);

      return svn_error_create(SVN_ERR_ENTRY_NOT_FOUND, err, err->message);
    }

  *wc_root = is_root || (kind == svn_wc__db_kind_dir && is_switched);

  return SVN_NO_ERROR;
}


svn_error_t*
svn_wc__strictly_is_wc_root(svn_boolean_t *wc_root,
                            svn_wc_context_t *wc_ctx,
                            const char *local_abspath,
                            apr_pool_t *scratch_pool)
{
  return svn_error_trace(svn_wc__db_is_wcroot(wc_root,
                                              wc_ctx->db,
                                              local_abspath,
                                              scratch_pool));
}


svn_error_t *
svn_wc__get_wc_root(const char **wcroot_abspath,
                    svn_wc_context_t *wc_ctx,
                    const char *local_abspath,
                    apr_pool_t *result_pool,
                    apr_pool_t *scratch_pool)
{
  return svn_wc__db_get_wcroot(wcroot_abspath, wc_ctx->db,
                               local_abspath, result_pool, scratch_pool);
}


svn_error_t *
svn_wc_get_actual_target2(const char **anchor,
                          const char **target,
                          svn_wc_context_t *wc_ctx,
                          const char *path,
                          apr_pool_t *result_pool,
                          apr_pool_t *scratch_pool)
{
  svn_boolean_t is_wc_root, is_switched;
  svn_wc__db_kind_t kind;
  const char *local_abspath;
  svn_error_t *err;

  SVN_ERR(svn_dirent_get_absolute(&local_abspath, path, scratch_pool));

  err = svn_wc__check_wc_root(&is_wc_root, &kind, &is_switched,
                              wc_ctx->db, local_abspath,
                              scratch_pool);

  if (err)
    {
      if (err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND &&
          err->apr_err != SVN_ERR_WC_NOT_WORKING_COPY)
        return svn_error_trace(err);

      svn_error_clear(err);
      is_wc_root = FALSE;
      is_switched = FALSE;
    }

  /* If PATH is not a WC root, or if it is a file, lop off a basename. */
  if (!(is_wc_root || is_switched) || (kind != svn_wc__db_kind_dir))
    {
      svn_dirent_split(anchor, target, path, result_pool);
    }
  else
    {
      *anchor = apr_pstrdup(result_pool, path);
      *target = "";
    }

  return SVN_NO_ERROR;
}


/* ### Note that this function is completely different from the rest of the
       update editor in what it updates. The update editor changes only BASE
       and ACTUAL and this function just changes WORKING and ACTUAL.

       In the entries world this function shared a lot of code with the
       update editor but in the wonderful new WC-NG world it will probably
       do more and more by itself and would be more logically grouped with
       the add/copy functionality in adm_ops.c and copy.c. */
svn_error_t *
svn_wc_add_repos_file4(svn_wc_context_t *wc_ctx,
                       const char *local_abspath,
                       svn_stream_t *new_base_contents,
                       svn_stream_t *new_contents,
                       apr_hash_t *new_base_props,
                       apr_hash_t *new_props,
                       const char *copyfrom_url,
                       svn_revnum_t copyfrom_rev,
                       svn_cancel_func_t cancel_func,
                       void *cancel_baton,
                       apr_pool_t *scratch_pool)
{
  svn_wc__db_t *db = wc_ctx->db;
  const char *dir_abspath = svn_dirent_dirname(local_abspath, scratch_pool);
  svn_wc__db_status_t status;
  svn_wc__db_kind_t kind;
  const char *tmp_text_base_abspath;
  svn_checksum_t *new_text_base_md5_checksum;
  svn_checksum_t *new_text_base_sha1_checksum;
  const char *source_abspath = NULL;
  svn_skel_t *all_work_items = NULL;
  svn_skel_t *work_item;
  const char *original_root_url;
  const char *original_repos_relpath;
  const char *original_uuid;
  svn_revnum_t changed_rev;
  apr_time_t changed_date;
  const char *changed_author;
  svn_error_t *err;
  apr_pool_t *pool = scratch_pool;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(local_abspath));
  SVN_ERR_ASSERT(new_base_contents != NULL);
  SVN_ERR_ASSERT(new_base_props != NULL);

  /* We should have a write lock on this file's parent directory.  */
  SVN_ERR(svn_wc__write_check(db, dir_abspath, pool));

  err = svn_wc__db_read_info(&status, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL,
                             db, local_abspath, scratch_pool, scratch_pool);

  if (err && err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND)
    return svn_error_trace(err);
  else if(err)
    svn_error_clear(err);
  else
    switch (status)
      {
        case svn_wc__db_status_not_present:
        case svn_wc__db_status_deleted:
          break;
        default:
          return svn_error_createf(SVN_ERR_ENTRY_EXISTS, NULL,
                                   _("Node '%s' exists."),
                                   svn_dirent_local_style(local_abspath,
                                                          scratch_pool));
      }

  SVN_ERR(svn_wc__db_read_info(&status, &kind, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL,
                               db, dir_abspath, scratch_pool, scratch_pool));

  switch (status)
    {
      case svn_wc__db_status_normal:
      case svn_wc__db_status_added:
        break;
      case svn_wc__db_status_deleted:
        return
          svn_error_createf(SVN_ERR_WC_SCHEDULE_CONFLICT, NULL,
                            _("Can't add '%s' to a parent directory"
                              " scheduled for deletion"),
                            svn_dirent_local_style(local_abspath,
                                                   scratch_pool));
      default:
        return svn_error_createf(SVN_ERR_ENTRY_NOT_FOUND, err,
                                 _("Can't find parent directory's node while"
                                   " trying to add '%s'"),
                                 svn_dirent_local_style(local_abspath,
                                                        scratch_pool));
    }
  if (kind != svn_wc__db_kind_dir)
    return svn_error_createf(SVN_ERR_NODE_UNEXPECTED_KIND, NULL,
                             _("Can't schedule an addition of '%s'"
                               " below a not-directory node"),
                             svn_dirent_local_style(local_abspath,
                                                    scratch_pool));

  /* Fabricate the anticipated new URL of the target and check the
     copyfrom URL to be in the same repository. */
  if (copyfrom_url != NULL)
    {
      /* Find the repository_root via the parent directory, which
         is always versioned before this function is called */
      SVN_ERR(svn_wc__internal_get_repos_info(&original_root_url,
                                              &original_uuid,
                                              wc_ctx->db,
                                              dir_abspath,
                                              pool, pool));

      if (!svn_uri__is_ancestor(original_root_url, copyfrom_url))
        return svn_error_createf(SVN_ERR_UNSUPPORTED_FEATURE, NULL,
                                 _("Copyfrom-url '%s' has different repository"
                                   " root than '%s'"),
                                 copyfrom_url, original_root_url);

      original_repos_relpath =
        svn_uri_skip_ancestor(original_root_url, copyfrom_url, pool);
    }
  else
    {
      original_root_url = NULL;
      original_repos_relpath = NULL;
      original_uuid = NULL;
      copyfrom_rev = SVN_INVALID_REVNUM;  /* Just to be sure.  */
    }

  /* Set CHANGED_* to reflect the entry props in NEW_BASE_PROPS, and
     filter NEW_BASE_PROPS so it contains only regular props. */
  {
    apr_array_header_t *regular_props;
    apr_array_header_t *entry_props;

    SVN_ERR(svn_categorize_props(svn_prop_hash_to_array(new_base_props, pool),
                                 &entry_props, NULL, &regular_props,
                                 pool));

    /* Put regular props back into a hash table. */
    new_base_props = svn_prop_array_to_hash(regular_props, pool);

    /* Get the change_* info from the entry props.  */
    SVN_ERR(accumulate_last_change(&changed_rev,
                                   &changed_date,
                                   &changed_author,
                                   entry_props, pool, pool));
  }

  /* Copy NEW_BASE_CONTENTS into a temporary file so our log can refer to
     it, and set TMP_TEXT_BASE_ABSPATH to its path.  Compute its
     NEW_TEXT_BASE_MD5_CHECKSUM and NEW_TEXT_BASE_SHA1_CHECKSUM as we copy. */
  {
    svn_stream_t *tmp_base_contents;

    SVN_ERR(svn_wc__open_writable_base(&tmp_base_contents,
                                       &tmp_text_base_abspath,
                                       &new_text_base_md5_checksum,
                                       &new_text_base_sha1_checksum,
                                       wc_ctx->db, local_abspath,
                                       pool, pool));
    SVN_ERR(svn_stream_copy3(new_base_contents, tmp_base_contents,
                             cancel_func, cancel_baton, pool));
  }

  /* If the caller gave us a new working file, copy it to a safe (temporary)
     location and set SOURCE_ABSPATH to that path. We'll then translate/copy
     that into place after the node's state has been created.  */
  if (new_contents)
    {
      const char *temp_dir_abspath;
      svn_stream_t *tmp_contents;

      SVN_ERR(svn_wc__db_temp_wcroot_tempdir(&temp_dir_abspath, db,
                                             local_abspath, pool, pool));
      SVN_ERR(svn_stream_open_unique(&tmp_contents, &source_abspath,
                                     temp_dir_abspath, svn_io_file_del_none,
                                     pool, pool));
      SVN_ERR(svn_stream_copy3(new_contents, tmp_contents,
                               cancel_func, cancel_baton, pool));
    }

  /* Install new text base for copied files. Added files do NOT have a
     text base.  */
  if (copyfrom_url != NULL)
    {
      SVN_ERR(svn_wc__db_pristine_install(db, tmp_text_base_abspath,
                                          new_text_base_sha1_checksum,
                                          new_text_base_md5_checksum, pool));
    }
  else
    {
      /* ### There's something wrong around here.  Sometimes (merge from a
         foreign repository, at least) we are called with copyfrom_url =
         NULL and an empty new_base_contents (and an empty set of
         new_base_props).  Why an empty "new base"?

         That happens in merge_tests.py 54,87,88,89,143.

         In that case, having been given this supposed "new base" file, we
         copy it and calculate its checksum but do not install it.  Why?
         That must be wrong.

         To crudely work around one issue with this, that we shouldn't
         record a checksum in the database if we haven't installed the
         corresponding pristine text, for now we'll just set the checksum
         to NULL.

         The proper solution is probably more like: the caller should pass
         NULL for the missing information, and this function should learn to
         handle that. */

      new_text_base_sha1_checksum = NULL;
      new_text_base_md5_checksum = NULL;
    }

  /* For added files without NEW_CONTENTS, then generate the working file
     from the provided "pristine" contents.  */
  if (new_contents == NULL && copyfrom_url == NULL)
    source_abspath = tmp_text_base_abspath;

  {
    svn_boolean_t record_fileinfo;

    /* If new contents were provided, then we do NOT want to record the
       file information. We assume the new contents do not match the
       "proper" values for TRANSLATED_SIZE and LAST_MOD_TIME.  */
    record_fileinfo = (new_contents == NULL);

    /* Install the working copy file (with appropriate translation) from
       the appropriate source. SOURCE_ABSPATH will be NULL, indicating an
       installation from the pristine (available for copied/moved files),
       or it will specify a temporary file where we placed a "pristine"
       (for an added file) or a detranslated local-mods file.  */
    SVN_ERR(svn_wc__wq_build_file_install(&work_item,
                                          db, local_abspath,
                                          source_abspath,
                                          FALSE /* use_commit_times */,
                                          record_fileinfo,
                                          pool, pool));
    all_work_items = svn_wc__wq_merge(all_work_items, work_item, pool);

    /* If we installed from somewhere besides the official pristine, then
       it is a temporary file, which needs to be removed.  */
    if (source_abspath != NULL)
      {
        SVN_ERR(svn_wc__wq_build_file_remove(&work_item,
                                             db, source_abspath,
                                             pool, pool));
        all_work_items = svn_wc__wq_merge(all_work_items, work_item, pool);
      }
  }

  /* ### ideally, we would have a single DB operation, and queue the work
     ### items on that. for now, we'll queue them with the second call.  */

  SVN_ERR(svn_wc__db_op_copy_file(db, local_abspath,
                                  new_base_props,
                                  changed_rev,
                                  changed_date,
                                  changed_author,
                                  original_repos_relpath,
                                  original_root_url,
                                  original_uuid,
                                  copyfrom_rev,
                                  new_text_base_sha1_checksum,
                                  NULL /* conflict */,
                                  NULL /* work_items */,
                                  pool));

  /* ### if below fails, then the above db change would remain :-(  */

  SVN_ERR(svn_wc__db_op_set_props(db, local_abspath,
                                  new_props, FALSE,
                                  NULL /* conflict */,
                                  all_work_items,
                                  pool));

  return svn_error_trace(svn_wc__wq_run(db, dir_abspath,
                                        cancel_func, cancel_baton,
                                        pool));
}

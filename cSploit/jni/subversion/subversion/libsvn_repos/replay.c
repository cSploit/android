/*
 * replay.c:   an editor driver for changes made in a given revision
 *             or transaction
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


#include <apr_hash.h>

#include "svn_types.h"
#include "svn_delta.h"
#include "svn_fs.h"
#include "svn_checksum.h"
#include "svn_repos.h"
#include "svn_props.h"
#include "svn_pools.h"
#include "svn_path.h"
#include "svn_private_config.h"
#include "private/svn_fspath.h"


/*** Backstory ***/

/* The year was 2003.  Subversion usage was rampant in the world, and
   there was a rapidly growing issues database to prove it.  To make
   matters worse, svn_repos_dir_delta() had simply outgrown itself.
   No longer content to simply describe the differences between two
   trees, the function had been slowly bearing the added
   responsibility of representing the actions that had been taken to
   cause those differences -- a burden it was never meant to bear.
   Now grown into a twisted mess of razor-sharp metal and glass, and
   trembling with a sort of momentarily stayed spring force,
   svn_repos_dir_delta was a timebomb poised for total annihilation of
   the American Midwest.

   Subversion needed a change.

   Changes, in fact.  And not just in the literary segue sense.  What
   Subversion desperately needed was a new mechanism solely
   responsible for replaying repository actions back to some
   interested party -- to translate and retransmit the contents of the
   Berkeley 'changes' database file. */

/*** Overview ***/

/* The filesystem keeps a record of high-level actions that affect the
   files and directories in itself.  The 'changes' table records
   additions, deletions, textual and property modifications, and so
   on.  The goal of the functions in this file is to examine those
   change records, and use them to drive an editor interface in such a
   way as to effectively replay those actions.

   This is critically different than what svn_repos_dir_delta() was
   designed to do.  That function describes, in the simplest way it
   can, how to transform one tree into another.  It doesn't care
   whether or not this was the same way a user might have done this
   transformation.  More to the point, it doesn't care if this is how
   those differences *did* come into being.  And it is for this reason
   that it cannot be relied upon for tasks such as the repository
   dumpfile-generation code, which is supposed to represent not
   changes, but actions that cause changes.

   So, what's the plan here?

   First, we fetch the changes for a particular revision or
   transaction.  We get these as an array, sorted chronologically.
   From this array we will build a hash, keyed on the path associated
   with each change item, and whose values are arrays of changes made
   to that path, again preserving the chronological ordering.

   Once our hash is built, we then sort all the keys of the hash (the
   paths) using a depth-first directory sort routine.

   Finally, we drive an editor, moving down our list of sorted paths,
   and manufacturing any intermediate editor calls (directory openings
   and closures) needed to navigate between each successive path.  For
   each path, we replay the sorted actions that occurred at that path.

   When we've finished the editor drive, we should have fully replayed
   the filesystem events that occurred in that revision or transaction
   (though not necessarily in the same order in which they
   occurred). */



/*** Helper functions. ***/


/* Information for an active copy, that is a directory which we are currently
   working on and which was added with history. */
struct copy_info
{
  /* Destination relpath (relative to the root of the  . */
  const char *path;

  /* Copy source path (expressed as an absolute FS path) or revision.
     NULL and SVN_INVALID_REVNUM if this is an add without history,
     nested inside an add with history. */
  const char *copyfrom_path;
  svn_revnum_t copyfrom_rev;
};

struct path_driver_cb_baton
{
  const svn_delta_editor_t *editor;
  void *edit_baton;

  /* The root of the revision we're replaying. */
  svn_fs_root_t *root;

  /* The root of the previous revision.  If this is non-NULL it means that
     we are supposed to generate props and text deltas relative to it. */
  svn_fs_root_t *compare_root;

  apr_hash_t *changed_paths;

  svn_repos_authz_func_t authz_read_func;
  void *authz_read_baton;

  const char *base_path; /* relpath */
  int base_path_len;

  svn_revnum_t low_water_mark;
  /* Stack of active copy operations. */
  apr_array_header_t *copies;

  /* The global pool for this replay operation. */
  apr_pool_t *pool;
};

/* Recursively traverse EDIT_PATH (as it exists under SOURCE_ROOT) emitting
   the appropriate editor calls to add it and its children without any
   history.  This is meant to be used when either a subset of the tree
   has been ignored and we need to copy something from that subset to
   the part of the tree we do care about, or if a subset of the tree is
   unavailable because of authz and we need to use it as the source of
   a copy. */
static svn_error_t *
add_subdir(svn_fs_root_t *source_root,
           svn_fs_root_t *target_root,
           const svn_delta_editor_t *editor,
           void *edit_baton,
           const char *edit_path,
           void *parent_baton,
           const char *source_fspath,
           svn_repos_authz_func_t authz_read_func,
           void *authz_read_baton,
           apr_hash_t *changed_paths,
           apr_pool_t *pool,
           void **dir_baton)
{
  apr_pool_t *subpool = svn_pool_create(pool);
  apr_hash_index_t *hi, *phi;
  apr_hash_t *dirents;
  apr_hash_t *props;

  SVN_ERR(editor->add_directory(edit_path, parent_baton, NULL,
                                SVN_INVALID_REVNUM, pool, dir_baton));

  SVN_ERR(svn_fs_node_proplist(&props, target_root, edit_path, pool));

  for (phi = apr_hash_first(pool, props); phi; phi = apr_hash_next(phi))
    {
      const void *key;
      void *val;

      svn_pool_clear(subpool);
      apr_hash_this(phi, &key, NULL, &val);
      SVN_ERR(editor->change_dir_prop(*dir_baton, key, val, subpool));
    }

  /* We have to get the dirents from the source path, not the target,
     because we want nested copies from *readable* paths to be handled by
     path_driver_cb_func, not add_subdir (in order to preserve history). */
  SVN_ERR(svn_fs_dir_entries(&dirents, source_root, source_fspath, pool));

  for (hi = apr_hash_first(pool, dirents); hi; hi = apr_hash_next(hi))
    {
      svn_fs_path_change2_t *change;
      svn_boolean_t readable = TRUE;
      svn_fs_dirent_t *dent;
      const char *copyfrom_path = NULL;
      svn_revnum_t copyfrom_rev = SVN_INVALID_REVNUM;
      const char *new_edit_path;
      void *val;

      svn_pool_clear(subpool);

      apr_hash_this(hi, NULL, NULL, &val);

      dent = val;

      new_edit_path = svn_relpath_join(edit_path, dent->name, subpool);

      /* If a file or subdirectory of the copied directory is listed as a
         changed path (because it was modified after the copy but before the
         commit), we remove it from the changed_paths hash so that future
         calls to path_driver_cb_func will ignore it. */
      change = apr_hash_get(changed_paths, new_edit_path, APR_HASH_KEY_STRING);
      if (change)
        {
          apr_hash_set(changed_paths, new_edit_path, APR_HASH_KEY_STRING, NULL);

          /* If it's a delete, skip this entry. */
          if (change->change_kind == svn_fs_path_change_delete)
            continue;

          /* If it's a replacement, check for copyfrom info (if we
             don't have it already. */
          if (change->change_kind == svn_fs_path_change_replace)
            {
              if (! change->copyfrom_known)
                {
                  SVN_ERR(svn_fs_copied_from(&change->copyfrom_rev,
                                             &change->copyfrom_path,
                                             target_root, new_edit_path, pool));
                  change->copyfrom_known = TRUE;
                }
              copyfrom_path = change->copyfrom_path;
              copyfrom_rev = change->copyfrom_rev;
            }
        }

      if (authz_read_func)
        SVN_ERR(authz_read_func(&readable, target_root, new_edit_path,
                                authz_read_baton, pool));

      if (! readable)
        continue;

      if (dent->kind == svn_node_dir)
        {
          svn_fs_root_t *new_source_root;
          const char *new_source_fspath;
          void *new_dir_baton;

          if (copyfrom_path)
            {
              svn_fs_t *fs = svn_fs_root_fs(source_root);
              SVN_ERR(svn_fs_revision_root(&new_source_root, fs,
                                           copyfrom_rev, pool));
              new_source_fspath = copyfrom_path;
            }
          else
            {
              new_source_root = source_root;
              new_source_fspath = svn_fspath__join(source_fspath, dent->name,
                                                   subpool);
            }

          /* ### authz considerations?
           *
           * I think not; when path_driver_cb_func() calls add_subdir(), it
           * passes SOURCE_ROOT and SOURCE_FSPATH that are unreadable.
           */
          if (change && change->change_kind == svn_fs_path_change_replace
              && copyfrom_path == NULL)
            {
              SVN_ERR(editor->add_directory(new_edit_path, *dir_baton,
                                            NULL, SVN_INVALID_REVNUM,
                                            subpool, &new_dir_baton));
            }
          else
            {
              SVN_ERR(add_subdir(new_source_root, target_root,
                                 editor, edit_baton, new_edit_path,
                                 *dir_baton, new_source_fspath,
                                 authz_read_func, authz_read_baton,
                                 changed_paths, subpool, &new_dir_baton));
            }

          SVN_ERR(editor->close_directory(new_dir_baton, subpool));
        }
      else if (dent->kind == svn_node_file)
        {
          svn_txdelta_window_handler_t delta_handler;
          void *delta_handler_baton, *file_baton;
          svn_txdelta_stream_t *delta_stream;
          svn_checksum_t *checksum;

          SVN_ERR(editor->add_file(new_edit_path, *dir_baton, NULL,
                                   SVN_INVALID_REVNUM, pool, &file_baton));

          SVN_ERR(svn_fs_node_proplist(&props, target_root,
                                       new_edit_path, subpool));

          for (phi = apr_hash_first(pool, props); phi; phi = apr_hash_next(phi))
            {
              const void *key;

              apr_hash_this(phi, &key, NULL, &val);
              SVN_ERR(editor->change_file_prop(file_baton, key, val, subpool));
            }

          SVN_ERR(editor->apply_textdelta(file_baton, NULL, pool,
                                          &delta_handler,
                                          &delta_handler_baton));

          SVN_ERR(svn_fs_get_file_delta_stream(&delta_stream, NULL, NULL,
                                               target_root, new_edit_path,
                                               pool));

          SVN_ERR(svn_txdelta_send_txstream(delta_stream,
                                            delta_handler,
                                            delta_handler_baton,
                                            pool));

          SVN_ERR(svn_fs_file_checksum(&checksum, svn_checksum_md5, target_root,
                                       new_edit_path, TRUE, pool));
          SVN_ERR(editor->close_file(file_baton,
                                     svn_checksum_to_cstring(checksum, pool),
                                     pool));
        }
      else
        SVN_ERR_MALFUNCTION();
    }

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}

static svn_boolean_t
is_within_base_path(const char *path, const char *base_path,
                    apr_ssize_t base_len)
{
  if (base_path[0] == '\0')
    return TRUE;

  if (strncmp(base_path, path, base_len) == 0
      && (path[base_len] == '/' || path[base_len] == '\0'))
    return TRUE;

  return FALSE;
}

/* Given PATH deleted under ROOT, return in READABLE whether the path was
   readable prior to the deletion.  Consult COPIES (a stack of 'struct
   copy_info') and AUTHZ_READ_FUNC. */
static svn_error_t *
was_readable(svn_boolean_t *readable,
             svn_fs_root_t *root,
             const char *path,
             apr_array_header_t *copies,
             svn_repos_authz_func_t authz_read_func,
             void *authz_read_baton,
             apr_pool_t *result_pool,
             apr_pool_t *scratch_pool)
{
  svn_fs_root_t *inquire_root;
  const char *inquire_path;
  struct copy_info *info = NULL;
  const char *relpath;

  /* Short circuit. */
  if (! authz_read_func)
    {
      *readable = TRUE;
      return SVN_NO_ERROR;
    }

  if (copies->nelts != 0)
    info = &APR_ARRAY_IDX(copies, copies->nelts - 1, struct copy_info);

  /* Are we under a copy? */
  if (info && (relpath = svn_relpath_skip_ancestor(info->path, path)))
    {
      SVN_ERR(svn_fs_revision_root(&inquire_root, svn_fs_root_fs(root),
                                   info->copyfrom_rev, scratch_pool));
      inquire_path = svn_fspath__join(info->copyfrom_path, relpath,
                                      scratch_pool);
    }
  else
    {
      /* Compute the revision that ROOT is based on.  (Note that ROOT is not
         r0's root, since this function is only called for deletions.)
         ### Need a more succinct way to express this */
      svn_revnum_t inquire_rev = SVN_INVALID_REVNUM;
      if (svn_fs_is_txn_root(root))
        inquire_rev = svn_fs_txn_root_base_revision(root);
      if (svn_fs_is_revision_root(root))
        inquire_rev =  svn_fs_revision_root_revision(root)-1;
      SVN_ERR_ASSERT(SVN_IS_VALID_REVNUM(inquire_rev));

      SVN_ERR(svn_fs_revision_root(&inquire_root, svn_fs_root_fs(root),
                                   inquire_rev, scratch_pool));
      inquire_path = path;
    }

  SVN_ERR(authz_read_func(readable, inquire_root, inquire_path,
                          authz_read_baton, result_pool));

  return SVN_NO_ERROR;
}

/* Initialize COPYFROM_ROOT, COPYFROM_PATH, and COPYFROM_REV with the
   revision root, fspath, and revnum of the copyfrom of CHANGE, which
   corresponds to PATH under ROOT.  If the copyfrom info is valid
   (i.e., is not (NULL, SVN_INVALID_REVNUM)), then initialize SRC_READABLE
   too, consulting AUTHZ_READ_FUNC and AUTHZ_READ_BATON if provided. */
static svn_error_t *
fill_copyfrom(svn_fs_root_t **copyfrom_root,
              const char **copyfrom_path,
              svn_revnum_t *copyfrom_rev,
              svn_boolean_t *src_readable,
              svn_fs_root_t *root,
              svn_fs_path_change2_t *change,
              svn_repos_authz_func_t authz_read_func,
              void *authz_read_baton,
              const char *path,
              apr_pool_t *result_pool,
              apr_pool_t *scratch_pool)
{
  if (! change->copyfrom_known)
    {
      SVN_ERR(svn_fs_copied_from(&(change->copyfrom_rev),
                                 &(change->copyfrom_path),
                                 root, path, result_pool));
      change->copyfrom_known = TRUE;
    }
  *copyfrom_rev = change->copyfrom_rev;
  *copyfrom_path = change->copyfrom_path;

  if (*copyfrom_path && SVN_IS_VALID_REVNUM(*copyfrom_rev))
    {
      SVN_ERR(svn_fs_revision_root(copyfrom_root,
                                   svn_fs_root_fs(root),
                                   *copyfrom_rev, result_pool));

      if (authz_read_func)
        {
          SVN_ERR(authz_read_func(src_readable, *copyfrom_root,
                                  *copyfrom_path,
                                  authz_read_baton, result_pool));
        }
      else
        *src_readable = TRUE;
    }
  else
    {
      *copyfrom_root = NULL;
      /* SRC_READABLE left uninitialized */
    }
  return SVN_NO_ERROR;
}

static svn_error_t *
path_driver_cb_func(void **dir_baton,
                    void *parent_baton,
                    void *callback_baton,
                    const char *edit_path,
                    apr_pool_t *pool)
{
  struct path_driver_cb_baton *cb = callback_baton;
  const svn_delta_editor_t *editor = cb->editor;
  void *edit_baton = cb->edit_baton;
  svn_fs_root_t *root = cb->root;
  svn_fs_path_change2_t *change;
  svn_boolean_t do_add = FALSE, do_delete = FALSE;
  void *file_baton = NULL;
  svn_revnum_t copyfrom_rev;
  const char *copyfrom_path;
  svn_fs_root_t *source_root = cb->compare_root;
  const char *source_fspath = NULL;
  const char *base_path = cb->base_path;
  int base_path_len = cb->base_path_len;

  *dir_baton = NULL;

  /* Initialize SOURCE_FSPATH. */
  if (source_root)
    source_fspath = svn_fspath__canonicalize(edit_path, pool);

  /* First, flush the copies stack so it only contains ancestors of path. */
  while (cb->copies->nelts > 0
         && ! svn_dirent_is_ancestor(APR_ARRAY_IDX(cb->copies,
                                                   cb->copies->nelts - 1,
                                                   struct copy_info).path,
                                     edit_path))
    cb->copies->nelts--;

  change = apr_hash_get(cb->changed_paths, edit_path, APR_HASH_KEY_STRING);
  if (! change)
    {
      /* This can only happen if the path was removed from cb->changed_paths
         by an earlier call to add_subdir, which means the path was already
         handled and we should simply ignore it. */
      return SVN_NO_ERROR;
    }
  switch (change->change_kind)
    {
    case svn_fs_path_change_add:
      do_add = TRUE;
      break;

    case svn_fs_path_change_delete:
      do_delete = TRUE;
      break;

    case svn_fs_path_change_replace:
      do_add = TRUE;
      do_delete = TRUE;
      break;

    case svn_fs_path_change_modify:
    default:
      /* do nothing */
      break;
    }

  /* Handle any deletions. */
  if (do_delete)
    {
      svn_boolean_t readable;

      /* Issue #4121: delete under under a copy, of a path that was unreadable
         at its pre-copy location. */
      SVN_ERR(was_readable(&readable, root, edit_path, cb->copies,
                            cb->authz_read_func, cb->authz_read_baton,
                            pool, pool));
      if (readable)
        SVN_ERR(editor->delete_entry(edit_path, SVN_INVALID_REVNUM,
                                     parent_baton, pool));
    }

  /* Fetch the node kind if it makes sense to do so. */
  if (! do_delete || do_add)
    {
      if (change->node_kind == svn_node_unknown)
        SVN_ERR(svn_fs_check_path(&(change->node_kind), root, edit_path, pool));
      if ((change->node_kind != svn_node_dir) &&
          (change->node_kind != svn_node_file))
        return svn_error_createf(SVN_ERR_FS_NOT_FOUND, NULL,
                                 _("Filesystem path '%s' is neither a file "
                                   "nor a directory"), edit_path);
    }

  /* Handle any adds/opens. */
  if (do_add)
    {
      svn_boolean_t src_readable;
      svn_fs_root_t *copyfrom_root;

      /* Was this node copied? */
      SVN_ERR(fill_copyfrom(&copyfrom_root, &copyfrom_path, &copyfrom_rev,
                            &src_readable, root, change,
                            cb->authz_read_func, cb->authz_read_baton,
                            edit_path, pool, pool));

      /* If we have a copyfrom path, and we can't read it or we're just
         ignoring it, or the copyfrom rev is prior to the low water mark
         then we just null them out and do a raw add with no history at
         all. */
      if (copyfrom_path
          && ((! src_readable)
              || (! is_within_base_path(copyfrom_path + 1, base_path,
                                        base_path_len))
              || (cb->low_water_mark > copyfrom_rev)))
        {
          copyfrom_path = NULL;
          copyfrom_rev = SVN_INVALID_REVNUM;
        }

      /* Do the right thing based on the path KIND. */
      if (change->node_kind == svn_node_dir)
        {
          /* If this is a copy, but we can't represent it as such,
             then we just do a recursive add of the source path
             contents. */
          if (change->copyfrom_path && ! copyfrom_path)
            {
              SVN_ERR(add_subdir(copyfrom_root, root, editor, edit_baton,
                                 edit_path, parent_baton, change->copyfrom_path,
                                 cb->authz_read_func, cb->authz_read_baton,
                                 cb->changed_paths, pool, dir_baton));
            }
          else
            {
              SVN_ERR(editor->add_directory(edit_path, parent_baton,
                                            copyfrom_path, copyfrom_rev,
                                            pool, dir_baton));
            }
        }
      else
        {
          SVN_ERR(editor->add_file(edit_path, parent_baton, copyfrom_path,
                                   copyfrom_rev, pool, &file_baton));
        }

      /* If we represent this as a copy... */
      if (copyfrom_path)
        {
          /* If it is a directory, make sure descendants get the correct
             delta source by remembering that we are operating inside a
             (possibly nested) copy operation. */
          if (change->node_kind == svn_node_dir)
            {
              struct copy_info *info = &APR_ARRAY_PUSH(cb->copies,
                                                       struct copy_info);
              info->path = apr_pstrdup(cb->pool, edit_path);
              info->copyfrom_path = apr_pstrdup(cb->pool, copyfrom_path);
              info->copyfrom_rev = copyfrom_rev;
            }

          /* Save the source so that we can use it later, when we
             need to generate text and prop deltas. */
          source_root = copyfrom_root;
          source_fspath = copyfrom_path;
        }
      else
        /* Else, we are an add without history... */
        {
          /* If an ancestor is added with history, we need to forget about
             that here, go on with life and repeat all the mistakes of our
             past... */
          if (change->node_kind == svn_node_dir && cb->copies->nelts > 0)
            {
              struct copy_info *info = &APR_ARRAY_PUSH(cb->copies,
                                                       struct copy_info);
              info->path = apr_pstrdup(cb->pool, edit_path);
              info->copyfrom_path = NULL;
              info->copyfrom_rev = SVN_INVALID_REVNUM;
            }
          source_root = NULL;
          source_fspath = NULL;
        }
    }
  else if (! do_delete)
    {
      /* Do the right thing based on the path KIND (and the presence
         of a PARENT_BATON). */
      if (change->node_kind == svn_node_dir)
        {
          if (parent_baton)
            {
              SVN_ERR(editor->open_directory(edit_path, parent_baton,
                                             SVN_INVALID_REVNUM,
                                             pool, dir_baton));
            }
          else
            {
              SVN_ERR(editor->open_root(edit_baton, SVN_INVALID_REVNUM,
                                        pool, dir_baton));
            }
        }
      else
        {
          SVN_ERR(editor->open_file(edit_path, parent_baton, SVN_INVALID_REVNUM,
                                    pool, &file_baton));
        }
      /* If we are inside an add with history, we need to adjust the
         delta source. */
      if (cb->copies->nelts > 0)
        {
          struct copy_info *info = &APR_ARRAY_IDX(cb->copies,
                                                  cb->copies->nelts - 1,
                                                  struct copy_info);
          if (info->copyfrom_path)
            {
              const char *relpath = svn_relpath__is_child(info->path,
                                                          edit_path, pool);
              SVN_ERR(svn_fs_revision_root(&source_root,
                                           svn_fs_root_fs(root),
                                           info->copyfrom_rev, pool));
              source_fspath = svn_fspath__join(info->copyfrom_path,
                                               relpath, pool);
            }
          else
            {
              /* This is an add without history, nested inside an
                 add with history.  We have no delta source in this case. */
              source_root = NULL;
              source_fspath = NULL;
            }
        }
    }

  if (! do_delete || do_add)
    {
      /* Is this a copy that was downgraded to a raw add?  (If so,
         we'll need to transmit properties and file contents and such
         for it regardless of what the CHANGE structure's text_mod
         and prop_mod flags say.)  */
      svn_boolean_t downgraded_copy = (change->copyfrom_known
                                       && change->copyfrom_path
                                       && (! copyfrom_path));

      /* Handle property modifications. */
      if (change->prop_mod || downgraded_copy)
        {
          if (cb->compare_root)
            {
              apr_array_header_t *prop_diffs;
              apr_hash_t *old_props;
              apr_hash_t *new_props;
              int i;

              if (source_root)
                SVN_ERR(svn_fs_node_proplist(&old_props, source_root,
                                             source_fspath, pool));
              else
                old_props = apr_hash_make(pool);

              SVN_ERR(svn_fs_node_proplist(&new_props, root, edit_path, pool));

              SVN_ERR(svn_prop_diffs(&prop_diffs, new_props, old_props,
                                     pool));

              for (i = 0; i < prop_diffs->nelts; ++i)
                {
                  svn_prop_t *pc = &APR_ARRAY_IDX(prop_diffs, i, svn_prop_t);
                   if (change->node_kind == svn_node_dir)
                     SVN_ERR(editor->change_dir_prop(*dir_baton, pc->name,
                                                     pc->value, pool));
                   else if (change->node_kind == svn_node_file)
                     SVN_ERR(editor->change_file_prop(file_baton, pc->name,
                                                      pc->value, pool));
                }
            }
          else
            {
              /* Just do a dummy prop change to signal that there are *any*
                 propmods. */
              if (change->node_kind == svn_node_dir)
                SVN_ERR(editor->change_dir_prop(*dir_baton, "", NULL,
                                                pool));
              else if (change->node_kind == svn_node_file)
                SVN_ERR(editor->change_file_prop(file_baton, "", NULL,
                                                 pool));
            }
        }

      /* Handle textual modifications. */
      if (change->node_kind == svn_node_file
          && (change->text_mod || downgraded_copy))
        {
          svn_txdelta_window_handler_t delta_handler;
          void *delta_handler_baton;
          const char *hex_digest = NULL;

          if (cb->compare_root && source_root && source_fspath)
            {
              svn_checksum_t *checksum;
              SVN_ERR(svn_fs_file_checksum(&checksum, svn_checksum_md5,
                                           source_root, source_fspath, TRUE,
                                           pool));
              hex_digest = svn_checksum_to_cstring(checksum, pool);
            }

          SVN_ERR(editor->apply_textdelta(file_baton, hex_digest, pool,
                                          &delta_handler,
                                          &delta_handler_baton));
          if (cb->compare_root)
            {
              svn_txdelta_stream_t *delta_stream;

              SVN_ERR(svn_fs_get_file_delta_stream(&delta_stream, source_root,
                                                   source_fspath, root,
                                                   edit_path, pool));
              SVN_ERR(svn_txdelta_send_txstream(delta_stream, delta_handler,
                                                delta_handler_baton, pool));
            }
          else
            SVN_ERR(delta_handler(NULL, delta_handler_baton));
        }
    }

  /* Close the file baton if we opened it. */
  if (file_baton)
    {
      svn_checksum_t *checksum;
      SVN_ERR(svn_fs_file_checksum(&checksum, svn_checksum_md5, root, edit_path,
                                   TRUE, pool));
      SVN_ERR(editor->close_file(file_baton,
                                 svn_checksum_to_cstring(checksum, pool),
                                 pool));
    }

  return SVN_NO_ERROR;
}




svn_error_t *
svn_repos_replay2(svn_fs_root_t *root,
                  const char *base_path,
                  svn_revnum_t low_water_mark,
                  svn_boolean_t send_deltas,
                  const svn_delta_editor_t *editor,
                  void *edit_baton,
                  svn_repos_authz_func_t authz_read_func,
                  void *authz_read_baton,
                  apr_pool_t *pool)
{
  apr_hash_t *fs_changes;
  apr_hash_t *changed_paths;
  apr_hash_index_t *hi;
  apr_array_header_t *paths;
  struct path_driver_cb_baton cb_baton;
  size_t base_path_len;

  /* Special-case r0, which we know is an empty revision; if we don't
     special-case it we might end up trying to compare it to "r-1". */
  if (svn_fs_is_revision_root(root) && svn_fs_revision_root_revision(root) == 0)
    {
      SVN_ERR(editor->set_target_revision(edit_baton, 0, pool));
      return SVN_NO_ERROR;
    }

  /* Fetch the paths changed under ROOT. */
  SVN_ERR(svn_fs_paths_changed2(&fs_changes, root, pool));

  if (! base_path)
    base_path = "";
  else if (base_path[0] == '/')
    ++base_path;

  base_path_len = strlen(base_path);

  /* Make an array from the keys of our CHANGED_PATHS hash, and copy
     the values into a new hash whose keys have no leading slashes. */
  paths = apr_array_make(pool, apr_hash_count(fs_changes),
                         sizeof(const char *));
  changed_paths = apr_hash_make(pool);
  for (hi = apr_hash_first(pool, fs_changes); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      void *val;
      apr_ssize_t keylen;
      const char *path;
      svn_fs_path_change2_t *change;
      svn_boolean_t allowed = TRUE;

      apr_hash_this(hi, &key, &keylen, &val);
      path = key;
      change = val;

      if (authz_read_func)
        SVN_ERR(authz_read_func(&allowed, root, path, authz_read_baton,
                                pool));

      if (allowed)
        {
          if (path[0] == '/')
            {
              path++;
              keylen--;
            }

          /* If the base_path doesn't match the top directory of this path
             we don't want anything to do with it... */
          if (is_within_base_path(path, base_path, base_path_len))
            {
              APR_ARRAY_PUSH(paths, const char *) = path;
              apr_hash_set(changed_paths, path, keylen, change);
            }
          /* ...unless this was a change to one of the parent directories of
             base_path. */
          else if (is_within_base_path(base_path, path, keylen))
            {
              APR_ARRAY_PUSH(paths, const char *) = path;
              apr_hash_set(changed_paths, path, keylen, change);
            }
        }
    }

  /* If we were not given a low water mark, assume that everything is there,
     all the way back to revision 0. */
  if (! SVN_IS_VALID_REVNUM(low_water_mark))
    low_water_mark = 0;

  /* Initialize our callback baton. */
  cb_baton.editor = editor;
  cb_baton.edit_baton = edit_baton;
  cb_baton.root = root;
  cb_baton.changed_paths = changed_paths;
  cb_baton.authz_read_func = authz_read_func;
  cb_baton.authz_read_baton = authz_read_baton;
  cb_baton.base_path = base_path;
  cb_baton.base_path_len = base_path_len;
  cb_baton.low_water_mark = low_water_mark;
  cb_baton.compare_root = NULL;

  if (send_deltas)
    {
      SVN_ERR(svn_fs_revision_root(&cb_baton.compare_root,
                                   svn_fs_root_fs(root),
                                   svn_fs_is_revision_root(root)
                                     ? svn_fs_revision_root_revision(root) - 1
                                     : svn_fs_txn_root_base_revision(root),
                                   pool));
    }

  cb_baton.copies = apr_array_make(pool, 4, sizeof(struct copy_info));
  cb_baton.pool = pool;

  /* Determine the revision to use throughout the edit, and call
     EDITOR's set_target_revision() function.  */
  if (svn_fs_is_revision_root(root))
    {
      svn_revnum_t revision = svn_fs_revision_root_revision(root);
      SVN_ERR(editor->set_target_revision(edit_baton, revision, pool));
    }

  /* Call the path-based editor driver. */
  return svn_delta_path_driver(editor, edit_baton,
                               SVN_INVALID_REVNUM, paths,
                               path_driver_cb_func, &cb_baton, pool);
}

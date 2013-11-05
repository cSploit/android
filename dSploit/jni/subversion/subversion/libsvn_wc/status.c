/*
 * status.c: construct a status structure from an entry structure
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



#include <assert.h>
#include <string.h>

#include <apr_pools.h>
#include <apr_file_io.h>
#include <apr_hash.h>

#include "svn_pools.h"
#include "svn_types.h"
#include "svn_delta.h"
#include "svn_string.h"
#include "svn_error.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_io.h"
#include "svn_config.h"
#include "svn_time.h"
#include "svn_hash.h"
#include "svn_sorts.h"

#include "svn_private_config.h"

#include "wc.h"
#include "props.h"
#include "entries.h"
#include "translate.h"
#include "tree_conflicts.h"

#include "private/svn_wc_private.h"
#include "private/svn_fspath.h"



/*** Baton used for walking the local status */
struct walk_status_baton
{
  /* The DB handle for managing the working copy state. */
  svn_wc__db_t *db;

  /*** External handling ***/
  /* Target of the status */
  const char *target_abspath;

  /* Should we ignore text modifications? */
  svn_boolean_t ignore_text_mods;

  /* Externals info harvested during the status run. */
  apr_hash_t *externals;

  /*** Repository lock handling ***/
  /* The repository root URL, if set. */
  const char *repos_root;

  /* Repository locks, if set. */
  apr_hash_t *repos_locks;
};

/*** Editor batons ***/

struct edit_baton
{
  /* For status, the "destination" of the edit.  */
  const char *anchor_abspath;
  const char *target_abspath;
  const char *target_basename;

  /* The DB handle for managing the working copy state.  */
  svn_wc__db_t *db;
  svn_wc_context_t *wc_ctx;

  /* The overall depth of this edit (a dir baton may override this).
   *
   * If this is svn_depth_unknown, the depths found in the working
   * copy will govern the edit; or if the edit depth indicates a
   * descent deeper than the found depths are capable of, the found
   * depths also govern, of course (there's no point descending into
   * something that's not there).
   */
  svn_depth_t default_depth;

  /* Do we want all statuses (instead of just the interesting ones) ? */
  svn_boolean_t get_all;

  /* Ignore the svn:ignores. */
  svn_boolean_t no_ignore;

  /* The comparison revision in the repository.  This is a reference
     because this editor returns this rev to the driver directly, as
     well as in each statushash entry. */
  svn_revnum_t *target_revision;

  /* Status function/baton. */
  svn_wc_status_func4_t status_func;
  void *status_baton;

  /* Cancellation function/baton. */
  svn_cancel_func_t cancel_func;
  void *cancel_baton;

  /* The configured set of default ignores. */
  const apr_array_header_t *ignores;

  /* Status item for the path represented by the anchor of the edit. */
  svn_wc_status3_t *anchor_status;

  /* Was open_root() called for this edit drive? */
  svn_boolean_t root_opened;

  /* The local status baton */
  struct walk_status_baton wb;
};


struct dir_baton
{
  /* The path to this directory. */
  const char *local_abspath;

  /* Basename of this directory. */
  const char *name;

  /* The global edit baton. */
  struct edit_baton *edit_baton;

  /* Baton for this directory's parent, or NULL if this is the root
     directory. */
  struct dir_baton *parent_baton;

  /* The ambient requested depth below this point in the edit.  This
     can differ from the parent baton's depth (with the edit baton
     considered the ultimate parent baton).  For example, if the
     parent baton has svn_depth_immediates, then here we should have
     svn_depth_empty, because there would be no further recursion, not
     even to file children. */
  svn_depth_t depth;

  /* Is this directory filtered out due to depth?  (Note that if this
     is TRUE, the depth field is undefined.) */
  svn_boolean_t excluded;

  /* 'svn status' shouldn't print status lines for things that are
     added;  we're only interest in asking if objects that the user
     *already* has are up-to-date or not.  Thus if this flag is set,
     the next two will be ignored.  :-)  */
  svn_boolean_t added;

  /* Gets set iff there's a change to this directory's properties, to
     guide us when syncing adm files later. */
  svn_boolean_t prop_changed;

  /* This means (in terms of 'svn status') that some child was deleted
     or added to the directory */
  svn_boolean_t text_changed;

  /* Working copy status structures for children of this directory.
     This hash maps const char * abspaths  to svn_wc_status3_t *
     status items. */
  apr_hash_t *statii;

  /* The pool in which this baton itself is allocated. */
  apr_pool_t *pool;

  /* The repository root relative path to this item in the repository. */
  const char *repos_relpath;

  /* out-of-date info corresponding to ood_* fields in svn_wc_status3_t. */
  svn_node_kind_t ood_kind;
  svn_revnum_t ood_changed_rev;
  apr_time_t ood_changed_date;
  const char *ood_changed_author;
};


struct file_baton
{
/* Absolute local path to this file */
  const char *local_abspath;

  /* The global edit baton. */
  struct edit_baton *edit_baton;

  /* Baton for this file's parent directory. */
  struct dir_baton *dir_baton;

  /* Pool specific to this file_baton. */
  apr_pool_t *pool;

  /* Basename of this file */
  const char *name;

  /* 'svn status' shouldn't print status lines for things that are
     added;  we're only interest in asking if objects that the user
     *already* has are up-to-date or not.  Thus if this flag is set,
     the next two will be ignored.  :-)  */
  svn_boolean_t added;

  /* This gets set if the file underwent a text change, which guides
     the code that syncs up the adm dir and working copy. */
  svn_boolean_t text_changed;

  /* This gets set if the file underwent a prop change, which guides
     the code that syncs up the adm dir and working copy. */
  svn_boolean_t prop_changed;

  /* The repository root relative path to this item in the repository. */
  const char *repos_relpath;

  /* out-of-date info corresponding to ood_* fields in svn_wc_status3_t. */
  svn_node_kind_t ood_kind;
  svn_revnum_t ood_changed_rev;
  apr_time_t ood_changed_date;

  const char *ood_changed_author;
};


/** Code **/

/* Fill in *INFO with the information it would contain if it were
   obtained from svn_wc__db_read_children_info. */
static svn_error_t *
read_info(const struct svn_wc__db_info_t **info,
          const char *local_abspath,
          svn_wc__db_t *db,
          apr_pool_t *result_pool,
          apr_pool_t *scratch_pool)
{
  struct svn_wc__db_info_t *mtb = apr_pcalloc(result_pool, sizeof(*mtb));
  const svn_checksum_t *checksum;

  SVN_ERR(svn_wc__db_read_info(&mtb->status, &mtb->kind,
                               &mtb->revnum, &mtb->repos_relpath,
                               &mtb->repos_root_url, &mtb->repos_uuid,
                               &mtb->changed_rev, &mtb->changed_date,
                               &mtb->changed_author, &mtb->depth,
                               &checksum, NULL, NULL, NULL, NULL,
                               NULL, &mtb->lock, &mtb->recorded_size,
                               &mtb->recorded_mod_time, &mtb->changelist,
                               &mtb->conflicted, &mtb->op_root,
                               &mtb->had_props, &mtb->props_mod,
                               &mtb->have_base, &mtb->have_more_work, NULL,
                               db, local_abspath,
                               result_pool, scratch_pool));

  SVN_ERR(svn_wc__db_wclocked(&mtb->locked, db, local_abspath, scratch_pool));

  /* Maybe we have to get some shadowed lock from BASE to make our test suite
     happy... (It might be completely unrelated, but...) */
  if (mtb->have_base
      && (mtb->status == svn_wc__db_status_added
         || mtb->status == svn_wc__db_status_deleted))
    {
      SVN_ERR(svn_wc__db_base_get_info(NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL, NULL, NULL, NULL, NULL,
                                       &mtb->lock, NULL, NULL,
                                       db, local_abspath,
                                       result_pool, scratch_pool));
    }

  mtb->has_checksum = (checksum != NULL);

#ifdef HAVE_SYMLINK
  if (mtb->kind == svn_wc__db_kind_file
      && (mtb->had_props || mtb->props_mod))
    {
      apr_hash_t *properties;

      if (mtb->props_mod)
        SVN_ERR(svn_wc__db_read_props(&properties, db, local_abspath,
                                      scratch_pool, scratch_pool));
      else
        SVN_ERR(svn_wc__db_read_pristine_props(&properties, db, local_abspath,
                                               scratch_pool, scratch_pool));

      mtb->special = (NULL != apr_hash_get(properties, SVN_PROP_SPECIAL,
                                           APR_HASH_KEY_STRING));
    }
#endif
  *info = mtb;

  return SVN_NO_ERROR;
}

/* Return *REPOS_RELPATH and *REPOS_ROOT_URL for LOCAL_ABSPATH using
   information in INFO if available, falling back on
   PARENT_REPOS_RELPATH and PARENT_REPOS_ROOT_URL if available, and
   finally falling back on querying DB. */
static svn_error_t *
get_repos_root_url_relpath(const char **repos_relpath,
                           const char **repos_root_url,
                           const char **repos_uuid,
                           const struct svn_wc__db_info_t *info,
                           const char *parent_repos_relpath,
                           const char *parent_repos_root_url,
                           const char *parent_repos_uuid,
                           svn_wc__db_t *db,
                           const char *local_abspath,
                           apr_pool_t *result_pool,
                           apr_pool_t *scratch_pool)
{
  if (info->repos_relpath && info->repos_root_url)
    {
      *repos_relpath = info->repos_relpath;
      *repos_root_url = info->repos_root_url;
      *repos_uuid = info->repos_uuid;
    }
  else if (parent_repos_relpath && parent_repos_root_url)
    {
      *repos_relpath = svn_relpath_join(parent_repos_relpath,
                                        svn_dirent_basename(local_abspath,
                                                            NULL),
                                        result_pool);
      *repos_root_url = parent_repos_root_url;
      *repos_uuid = parent_repos_uuid;
    }
  else if (info->status == svn_wc__db_status_added)
    {
      SVN_ERR(svn_wc__db_scan_addition(NULL, NULL,
                                       repos_relpath, repos_root_url,
                                       repos_uuid, NULL, NULL, NULL, NULL,
                                       db, local_abspath,
                                       result_pool, scratch_pool));
    }
  else if (info->have_base)
    {
      SVN_ERR(svn_wc__db_scan_base_repos(repos_relpath, repos_root_url,
                                         repos_uuid,
                                         db, local_abspath,
                                         result_pool, scratch_pool));
    }
  else
    {
      *repos_relpath = NULL;
      *repos_root_url = NULL;
      *repos_uuid = NULL;
    }
  return SVN_NO_ERROR;
}

static svn_error_t *
internal_status(svn_wc_status3_t **status,
                svn_wc__db_t *db,
                const char *local_abspath,
                apr_pool_t *result_pool,
                apr_pool_t *scratch_pool);

/* Fill in *STATUS for LOCAL_ABSPATH, using DB. Allocate *STATUS in
   RESULT_POOL and use SCRATCH_POOL for temporary allocations.

   PARENT_REPOS_ROOT_URL and PARENT_REPOS_RELPATH are the the repository root
   and repository relative path of the parent of LOCAL_ABSPATH or NULL if
   LOCAL_ABSPATH doesn't have a versioned parent directory.

   DIRENT is the local representation of LOCAL_ABSPATH in the working copy or
   NULL if the node does not exist on disk.

   If GET_ALL is FALSE, and LOCAL_ABSPATH is not locally modified, then
   *STATUS will be set to NULL.  If GET_ALL is non-zero, then *STATUS will be
   allocated and returned no matter what.  If IGNORE_TEXT_MODS is TRUE then
   don't check for text mods, assume there are none and set and *STATUS
   returned to reflect that assumption.

   The status struct's repos_lock field will be set to REPOS_LOCK.
*/
static svn_error_t *
assemble_status(svn_wc_status3_t **status,
                svn_wc__db_t *db,
                const char *local_abspath,
                const char *parent_repos_root_url,
                const char *parent_repos_relpath,
                const char *parent_repos_uuid,
                const struct svn_wc__db_info_t *info,
                const svn_io_dirent2_t *dirent,
                svn_boolean_t get_all,
                svn_boolean_t ignore_text_mods,
                const svn_lock_t *repos_lock,
                apr_pool_t *result_pool,
                apr_pool_t *scratch_pool)
{
  svn_wc_status3_t *stat;
  svn_boolean_t switched_p = FALSE;
  svn_boolean_t copied = FALSE;
  svn_boolean_t conflicted;
  svn_error_t *err;
  const char *repos_relpath;
  const char *repos_root_url;
  const char *repos_uuid;
  svn_filesize_t filesize = (dirent && (dirent->kind == svn_node_file))
                                ? dirent->filesize
                                : SVN_INVALID_FILESIZE;

  /* Defaults for two main variables. */
  enum svn_wc_status_kind node_status = svn_wc_status_normal;
  enum svn_wc_status_kind text_status = svn_wc_status_normal;
  enum svn_wc_status_kind prop_status = svn_wc_status_none;


  if (!info)
    SVN_ERR(read_info(&info, local_abspath, db, result_pool, scratch_pool));

  if (!info->repos_relpath || !parent_repos_relpath)
    switched_p = FALSE;
  else
    {
      /* A node is switched if it doesn't have the implied repos_relpath */
      const char *name = svn_relpath_skip_ancestor(parent_repos_relpath,
                                                   info->repos_relpath);
      switched_p = !name || (strcmp(name, svn_dirent_basename(local_abspath, NULL)) != 0);
    }

  /* Examine whether our target is missing or obstructed or missing.

     While we are not completely in single-db mode yet, data about
     obstructed or missing nodes might be incomplete here. This is
     reported by svn_wc_db_status_obstructed_XXXX. In single-db
     mode these obstructions are no longer reported and we have
     to detect obstructions by looking at the on disk status in DIRENT.
     */
  if (info->kind == svn_wc__db_kind_dir)
    {
      if (info->status == svn_wc__db_status_incomplete || info->incomplete)
        {
          /* Highest precedence.  */
          node_status = svn_wc_status_incomplete;
        }
      else if (info->status == svn_wc__db_status_deleted)
        {
          node_status = svn_wc_status_deleted;

          if (!info->have_base)
            copied = TRUE;
          else
            SVN_ERR(svn_wc__internal_node_get_schedule(NULL, &copied,
                                                       db, local_abspath,
                                                       scratch_pool));
        }
      else if (!dirent || dirent->kind != svn_node_dir)
        {
          /* A present or added directory should be on disk, so it is
             reported missing or obstructed.  */
          if (!dirent || dirent->kind == svn_node_none)
            node_status = svn_wc_status_missing;
          else
            node_status = svn_wc_status_obstructed;
        }
    }
  else
    {
      if (info->status == svn_wc__db_status_deleted)
        {
          node_status = svn_wc_status_deleted;

          SVN_ERR(svn_wc__internal_node_get_schedule(NULL, &copied,
                                                     db, local_abspath,
                                                     scratch_pool));
        }
      else if (!dirent || dirent->kind != svn_node_file)
        {
          /* A present or added file should be on disk, so it is
             reported missing or obstructed.  */
          if (!dirent || dirent->kind == svn_node_none)
            node_status = svn_wc_status_missing;
          else
            node_status = svn_wc_status_obstructed;
        }
    }

  /* Does the node have props? */
  if (info->status != svn_wc__db_status_deleted)
    {
      if (info->props_mod)
        prop_status = svn_wc_status_modified;
      else if (info->had_props)
        prop_status = svn_wc_status_normal;
    }

  /* If NODE_STATUS is still normal, after the above checks, then
     we should proceed to refine the status.

     If it was changed, then the subdir is incomplete or missing/obstructed.
   */
  if (info->kind != svn_wc__db_kind_dir
      && node_status == svn_wc_status_normal)
    {
      svn_boolean_t text_modified_p = FALSE;

      /* Implement predecence rules: */

      /* 1. Set the two main variables to "discovered" values first (M, C).
            Together, these two stati are of lowest precedence, and C has
            precedence over M. */

      /* If the entry is a file, check for textual modifications */
      if ((info->kind == svn_wc__db_kind_file
          || info->kind == svn_wc__db_kind_symlink)
#ifdef HAVE_SYMLINK
             && (info->special == (dirent && dirent->special))
#endif /* HAVE_SYMLINK */
          )
        {
          /* If the on-disk dirent exactly matches the expected state
             skip all operations in svn_wc__internal_text_modified_p()
             to avoid an extra filestat for every file, which can be
             expensive on network drives as a filestat usually can't
             be cached there */
          if (!info->has_checksum)
            text_modified_p = TRUE; /* Local addition -> Modified */
          else if (ignore_text_mods
                  ||(dirent
                     && info->recorded_size != SVN_INVALID_FILESIZE
                     && info->recorded_mod_time != 0
                     && info->recorded_size == dirent->filesize
                     && info->recorded_mod_time == dirent->mtime))
            text_modified_p = FALSE;
          else
            {
              err = svn_wc__internal_file_modified_p(&text_modified_p,
                                                     db, local_abspath,
                                                     FALSE, scratch_pool);

              if (err)
                {
                  if (err->apr_err != SVN_ERR_WC_PATH_ACCESS_DENIED)
                    return svn_error_trace(err);

                  /* An access denied is very common on Windows when another
                     application has the file open.  Previously we ignored
                     this error in svn_wc__text_modified_internal_p, where it
                     should have really errored. */
                  svn_error_clear(err);
                  text_modified_p = TRUE;
                }
            }
        }
#ifdef HAVE_SYMLINK
      else if (info->special != (dirent && dirent->special))
        node_status = svn_wc_status_obstructed;
#endif /* HAVE_SYMLINK */

      if (text_modified_p)
        text_status = svn_wc_status_modified;
    }

  conflicted = info->conflicted;
  if (conflicted)
    {
      svn_boolean_t text_conflicted, prop_conflicted, tree_conflicted;

      /* ### Check if the conflict was resolved by removing the marker files.
         ### This should really be moved to the users of this API */
      SVN_ERR(svn_wc__internal_conflicted_p(&text_conflicted, &prop_conflicted,
                                            &tree_conflicted,
                                            db, local_abspath, scratch_pool));

      if (!text_conflicted && !prop_conflicted && !tree_conflicted)
        conflicted = FALSE;
    }

  if (node_status == svn_wc_status_normal)
    {
      /* 2. Possibly overwrite the text_status variable with "scheduled"
            states from the entry (A, D, R).  As a group, these states are
            of medium precedence.  They also override any C or M that may
            be in the prop_status field at this point, although they do not
            override a C text status.*/
      if (info->status == svn_wc__db_status_added)
        {
          if (!info->op_root)
            copied = TRUE; /* And keep status normal */
          else if (info->kind == svn_wc__db_kind_file
                   && !info->have_base && !info->have_more_work)
            {
              /* Simple addition or copy, no replacement */
              node_status = svn_wc_status_added;
              /* If an added node has a pristine file, it was copied */
              copied = info->has_checksum;
            }
          else
            {
              svn_wc_schedule_t schedule;
              SVN_ERR(svn_wc__internal_node_get_schedule(&schedule, &copied,
                                                         db, local_abspath,
                                                         scratch_pool));

              if (schedule == svn_wc_schedule_add)
                node_status = svn_wc_status_added;
              else if (schedule == svn_wc_schedule_replace)
                node_status = svn_wc_status_replaced;
            }
        }
    }

  if (node_status == svn_wc_status_normal)
    node_status = text_status;

  if (node_status == svn_wc_status_normal
      && prop_status != svn_wc_status_none)
    node_status = prop_status;

  /* 5. Easy out:  unless we're fetching -every- entry, don't bother
     to allocate a struct for an uninteresting entry. */

  if (! get_all)
    if (((node_status == svn_wc_status_none)
         || (node_status == svn_wc_status_normal))

        && (! switched_p)
        && (! info->locked )
        && (! info->lock)
        && (! repos_lock)
        && (! info->changelist)
        && (! conflicted))
      {
        *status = NULL;
        return SVN_NO_ERROR;
      }

  SVN_ERR(get_repos_root_url_relpath(&repos_relpath, &repos_root_url,
                                     &repos_uuid, info,
                                     parent_repos_relpath,
                                     parent_repos_root_url,
                                     parent_repos_uuid,
                                     db, local_abspath,
                                     scratch_pool, scratch_pool));

  /* 6. Build and return a status structure. */

  stat = apr_pcalloc(result_pool, sizeof(**status));

  switch (info->kind)
    {
      case svn_wc__db_kind_dir:
        stat->kind = svn_node_dir;
        break;
      case svn_wc__db_kind_file:
      case svn_wc__db_kind_symlink:
        stat->kind = svn_node_file;
        break;
      case svn_wc__db_kind_unknown:
      default:
        stat->kind = svn_node_unknown;
    }
  stat->depth = info->depth;
  stat->filesize = filesize;
  stat->node_status = node_status;
  stat->text_status = text_status;
  stat->prop_status = prop_status;
  stat->repos_node_status = svn_wc_status_none;   /* default */
  stat->repos_text_status = svn_wc_status_none;   /* default */
  stat->repos_prop_status = svn_wc_status_none;   /* default */
  stat->switched = switched_p;
  stat->copied = copied;
  stat->repos_lock = repos_lock;
  stat->revision = info->revnum;
  stat->changed_rev = info->changed_rev;
  stat->changed_author = info->changed_author;
  stat->changed_date = info->changed_date;

  stat->ood_kind = svn_node_none;
  stat->ood_changed_rev = SVN_INVALID_REVNUM;
  stat->ood_changed_date = 0;
  stat->ood_changed_author = NULL;

  if (info->lock)
    {
      svn_lock_t *lck = apr_pcalloc(result_pool, sizeof(*lck));
      lck->path = repos_relpath;
      lck->token = info->lock->token;
      lck->owner = info->lock->owner;
      lck->comment = info->lock->comment;
      lck->creation_date = info->lock->date;
      stat->lock = lck;
    }
  else
    stat->lock = NULL;

  stat->locked = info->locked;
  stat->conflicted = conflicted;
  stat->versioned = TRUE;
  stat->changelist = info->changelist;
  stat->repos_root_url = repos_root_url;
  stat->repos_relpath = repos_relpath;
  stat->repos_uuid = repos_uuid;

  *status = stat;

  return SVN_NO_ERROR;
}

/* Fill in *STATUS for the unversioned path LOCAL_ABSPATH, using data
   available in DB. Allocate *STATUS in POOL. Use SCRATCH_POOL for
   temporary allocations.

   If IS_IGNORED is non-zero and this is a non-versioned entity, set
   the node_status to svn_wc_status_none.  Otherwise set the
   node_status to svn_wc_status_unversioned.
 */
static svn_error_t *
assemble_unversioned(svn_wc_status3_t **status,
                     svn_wc__db_t *db,
                     const char *local_abspath,
                     const svn_io_dirent2_t *dirent,
                     svn_boolean_t tree_conflicted,
                     svn_boolean_t is_ignored,
                     apr_pool_t *result_pool,
                     apr_pool_t *scratch_pool)
{
  svn_wc_status3_t *stat;

  /* return a fairly blank structure. */
  stat = apr_pcalloc(result_pool, sizeof(*stat));

  /*stat->versioned = FALSE;*/
  stat->kind = svn_node_unknown; /* not versioned */
  stat->depth = svn_depth_unknown;
  stat->filesize = (dirent && dirent->kind == svn_node_file)
                        ? dirent->filesize
                        : SVN_INVALID_FILESIZE;
  stat->node_status = svn_wc_status_none;
  stat->text_status = svn_wc_status_none;
  stat->prop_status = svn_wc_status_none;
  stat->repos_node_status = svn_wc_status_none;
  stat->repos_text_status = svn_wc_status_none;
  stat->repos_prop_status = svn_wc_status_none;

  /* If this path has no entry, but IS present on disk, it's
     unversioned.  If this file is being explicitly ignored (due
     to matching an ignore-pattern), the node_status is set to
     svn_wc_status_ignored.  Otherwise the node_status is set to
     svn_wc_status_unversioned. */
  if (dirent && dirent->kind != svn_node_none)
    {
      if (is_ignored)
        stat->node_status = svn_wc_status_ignored;
      else
        stat->node_status = svn_wc_status_unversioned;
    }
  else if (tree_conflicted)
    {
      /* If this path has no entry, is NOT present on disk, and IS a
         tree conflict victim, report it as conflicted. */
      stat->node_status = svn_wc_status_conflicted;
    }

  stat->revision = SVN_INVALID_REVNUM;
  stat->changed_rev = SVN_INVALID_REVNUM;
  stat->ood_changed_rev = SVN_INVALID_REVNUM;
  stat->ood_kind = svn_node_none;

  /* For the case of an incoming delete to a locally deleted path during
     an update, we get a tree conflict. */
  stat->conflicted = tree_conflicted;
  stat->changelist = NULL;

  *status = stat;
  return SVN_NO_ERROR;
}


/* Given an ENTRY object representing PATH, build a status structure
   and pass it off to the STATUS_FUNC/STATUS_BATON.  All other
   arguments are the same as those passed to assemble_status().  */
static svn_error_t *
send_status_structure(const struct walk_status_baton *wb,
                      const char *local_abspath,
                      const char *parent_repos_root_url,
                      const char *parent_repos_relpath,
                      const char *parent_repos_uuid,
                      const struct svn_wc__db_info_t *info,
                      const svn_io_dirent2_t *dirent,
                      svn_boolean_t get_all,
                      svn_wc_status_func4_t status_func,
                      void *status_baton,
                      apr_pool_t *scratch_pool)
{
  svn_wc_status3_t *statstruct;
  const svn_lock_t *repos_lock = NULL;

  /* Check for a repository lock. */
  if (wb->repos_locks)
    {
      const char *repos_relpath, *repos_root_url, *repos_uuid;

      SVN_ERR(get_repos_root_url_relpath(&repos_relpath, &repos_root_url,
                                         &repos_uuid,
                                         info, parent_repos_relpath,
                                         parent_repos_root_url,
                                         parent_repos_uuid,
                                         wb->db, local_abspath,
                                         scratch_pool, scratch_pool));
      if (repos_relpath)
        {
          /* repos_lock still uses the deprecated filesystem absolute path
             format */
          repos_lock = apr_hash_get(wb->repos_locks,
                                    svn_fspath__join("/", repos_relpath,
                                                     scratch_pool),
                                    APR_HASH_KEY_STRING);
        }
    }

  SVN_ERR(assemble_status(&statstruct, wb->db, local_abspath,
                          parent_repos_root_url, parent_repos_relpath,
                          parent_repos_uuid,
                          info, dirent, get_all, wb->ignore_text_mods,
                          repos_lock, scratch_pool, scratch_pool));

  if (statstruct && status_func)
    return svn_error_trace((*status_func)(status_baton, local_abspath,
                                          statstruct, scratch_pool));

  return SVN_NO_ERROR;
}


/* Store in PATTERNS a list of all svn:ignore properties from
   the working copy directory, including the default ignores
   passed in as IGNORES.

   Upon return, *PATTERNS will contain zero or more (const char *)
   patterns from the value of the SVN_PROP_IGNORE property set on
   the working directory path.

   IGNORES is a list of patterns to include; typically this will
   be the default ignores as, for example, specified in a config file.

   LOCAL_ABSPATH and DB control how to access the ignore information.

   Allocate results in RESULT_POOL, temporary stuffs in SCRATCH_POOL.

   None of the arguments may be NULL.
*/
static svn_error_t *
collect_ignore_patterns(apr_array_header_t **patterns,
                        svn_wc__db_t *db,
                        const char *local_abspath,
                        const apr_array_header_t *ignores,
                        apr_pool_t *result_pool,
                        apr_pool_t *scratch_pool)
{
  int i;
  const svn_string_t *value;

  /* ### assert we are passed a directory? */

  *patterns = apr_array_make(result_pool, 1, sizeof(const char *));

  /* Copy default ignores into the local PATTERNS array. */
  for (i = 0; i < ignores->nelts; i++)
    {
      const char *ignore = APR_ARRAY_IDX(ignores, i, const char *);
      APR_ARRAY_PUSH(*patterns, const char *) = apr_pstrdup(result_pool,
                                                            ignore);
    }

  /* Then add any svn:ignore globs to the PATTERNS array. */
  SVN_ERR(svn_wc__internal_propget(&value, db, local_abspath, SVN_PROP_IGNORE,
                                   scratch_pool, scratch_pool));
  if (value != NULL)
    svn_cstring_split_append(*patterns, value->data, "\n\r", FALSE,
                             result_pool);

  return SVN_NO_ERROR;
}


/* Compare LOCAL_ABSPATH with items in the EXTERNALS hash to see if
   LOCAL_ABSPATH is the drop location for, or an intermediate directory
   of the drop location for, an externals definition.  Use SCRATCH_POOL
   for scratchwork.  */
static svn_boolean_t
is_external_path(apr_hash_t *externals,
                 const char *local_abspath,
                 apr_pool_t *scratch_pool)
{
  apr_hash_index_t *hi;

  /* First try: does the path exist as a key in the hash? */
  if (apr_hash_get(externals, local_abspath, APR_HASH_KEY_STRING))
    return TRUE;

  /* Failing that, we need to check if any external is a child of
     LOCAL_ABSPATH.  */
  for (hi = apr_hash_first(scratch_pool, externals);
       hi;
       hi = apr_hash_next(hi))
    {
      const char *external_abspath = svn__apr_hash_index_key(hi);

      if (svn_dirent_is_child(local_abspath, external_abspath, NULL))
        return TRUE;
    }

  return FALSE;
}


/* Assuming that LOCAL_ABSPATH is unversioned, send a status structure
   for it through STATUS_FUNC/STATUS_BATON unless this path is being
   ignored.  This function should never be called on a versioned entry.

   LOCAL_ABSPATH is the path to the unversioned file whose status is being
   requested.  PATH_KIND is the node kind of NAME as determined by the
   caller.  PATH_SPECIAL is the special status of the path, also determined
   by the caller.
   PATTERNS points to a list of filename patterns which are marked as
   ignored.  None of these parameter may be NULL.  EXTERNALS is a hash
   of known externals definitions for this status run.

   If NO_IGNORE is non-zero, the item will be added regardless of
   whether it is ignored; otherwise we will only add the item if it
   does not match any of the patterns in PATTERNS.

   Allocate everything in POOL.
*/
static svn_error_t *
send_unversioned_item(const struct walk_status_baton *wb,
                      const char *local_abspath,
                      const svn_io_dirent2_t *dirent,
                      svn_boolean_t tree_conflicted,
                      const apr_array_header_t *patterns,
                      svn_boolean_t no_ignore,
                      svn_wc_status_func4_t status_func,
                      void *status_baton,
                      apr_pool_t *scratch_pool)
{
  svn_boolean_t is_ignored;
  svn_boolean_t is_external;
  svn_wc_status3_t *status;

  is_ignored = svn_wc_match_ignore_list(
                 svn_dirent_basename(local_abspath, NULL),
                 patterns, scratch_pool);

  SVN_ERR(assemble_unversioned(&status,
                               wb->db, local_abspath,
                               dirent, tree_conflicted, is_ignored,
                               scratch_pool, scratch_pool));

  is_external = is_external_path(wb->externals, local_abspath, scratch_pool);
  if (is_external)
    status->node_status = svn_wc_status_external;

  /* We can have a tree conflict on an unversioned path, i.e. an incoming
   * delete on a locally deleted path during an update. Don't ever ignore
   * those! */
  if (status->conflicted)
    is_ignored = FALSE;

  /* If we aren't ignoring it, or if it's an externals path, pass this
     entry to the status func. */
  if (no_ignore || (! is_ignored) || is_external)
    return svn_error_trace((*status_func)(status_baton, local_abspath,
                                          status, scratch_pool));

  return SVN_NO_ERROR;
}

/* Send svn_wc_status3_t * structures for the directory LOCAL_ABSPATH and
   for all its entries through STATUS_FUNC/STATUS_BATON, or, if SELECTED
   is non-NULL, only for that directory entry.

   PARENT_ENTRY is the entry for the parent of the directory or NULL
   if LOCAL_ABSPATH is a working copy root.

   If SKIP_THIS_DIR is TRUE, the directory's own status will not be reported.
   However, upon recursing, all subdirs *will* be reported, regardless of this
   parameter's value.

   DIRENT is LOCAL_ABSPATH's own dirent and is only needed if it is reported,
   so if SKIP_THIS_DIR or SELECTED is not-NULL DIRENT can be left NULL.

   DIR_INFO can be set to the information of LOCAL_ABSPATH, to avoid retrieving
   it again.

   Other arguments are the same as those passed to
   svn_wc_get_status_editor5().  */
static svn_error_t *
get_dir_status(const struct walk_status_baton *wb,
               const char *local_abspath,
               const char *selected,
               svn_boolean_t skip_this_dir,
               const char *parent_repos_root_url,
               const char *parent_repos_relpath,
               const char *parent_repos_uuid,
               const struct svn_wc__db_info_t *dir_info,
               const svn_io_dirent2_t *dirent,
               const apr_array_header_t *ignore_patterns,
               svn_depth_t depth,
               svn_boolean_t get_all,
               svn_boolean_t no_ignore,
               svn_wc_status_func4_t status_func,
               void *status_baton,
               svn_cancel_func_t cancel_func,
               void *cancel_baton,
               apr_pool_t *scratch_pool)
{
  const char *dir_repos_root_url;
  const char *dir_repos_relpath;
  const char *dir_repos_uuid;
  apr_hash_t *dirents, *nodes, *conflicts, *all_children;
  apr_array_header_t *patterns = NULL;
  apr_array_header_t *sorted_children;
  apr_pool_t *iterpool, *subpool = svn_pool_create(scratch_pool);
  svn_error_t *err;
  int i;

  if (cancel_func)
    SVN_ERR(cancel_func(cancel_baton));

  if (depth == svn_depth_unknown)
    depth = svn_depth_infinity;

  iterpool = svn_pool_create(subpool);

  err = svn_io_get_dirents3(&dirents, local_abspath, FALSE, subpool, iterpool);
  if (err
      && (APR_STATUS_IS_ENOENT(err->apr_err)
         || SVN__APR_STATUS_IS_ENOTDIR(err->apr_err)))
    {
      svn_error_clear(err);
      dirents = apr_hash_make(subpool);
    }
  else
    SVN_ERR(err);

  if (!dir_info)
    SVN_ERR(read_info(&dir_info, local_abspath, wb->db,
                      subpool, iterpool));

  SVN_ERR(get_repos_root_url_relpath(&dir_repos_relpath, &dir_repos_root_url,
                                     &dir_repos_uuid, dir_info,
                                     parent_repos_relpath,
                                     parent_repos_root_url, parent_repos_uuid,
                                     wb->db, local_abspath,
                                     subpool, iterpool));
  if (selected == NULL)
    {
      /* Create a hash containing all children.  The source hashes
         don't all map the same types, but only the keys of the result
         hash are subsequently used. */
      SVN_ERR(svn_wc__db_read_children_info(&nodes, &conflicts,
                                        wb->db, local_abspath,
                                        subpool, iterpool));

      all_children = apr_hash_overlay(subpool, nodes, dirents);
      if (apr_hash_count(conflicts) > 0)
        all_children = apr_hash_overlay(subpool, conflicts, all_children);
    }
  else
    {
      const struct svn_wc__db_info_t *info;
      const char *selected_abspath = svn_dirent_join(local_abspath, selected,
                                                     iterpool);
      /* Create a hash containing just selected */
      all_children = apr_hash_make(subpool);
      nodes = apr_hash_make(subpool);
      conflicts = apr_hash_make(subpool);

      err = read_info(&info, selected_abspath, wb->db, subpool, iterpool);

      if (err)
        {
          if (err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND)
            return svn_error_trace(err);
          svn_error_clear(err);
          /* The node is neither a tree conflict nor a versioned node */
        }
      else
        {
          if (!info->conflicted
              || info->status != svn_wc__db_status_normal
              || info->kind != svn_wc__db_kind_unknown)
             {
               /* The node is a normal versioned node */
               apr_hash_set(nodes, selected, APR_HASH_KEY_STRING, info);
             }

          /* Drop it in the list of possible conflicts */
          if (info->conflicted)
            apr_hash_set(conflicts, selected, APR_HASH_KEY_STRING, info);
        }

      apr_hash_set(all_children, selected, APR_HASH_KEY_STRING, selected);
    }

  if (!selected)
    {
      /* Handle "this-dir" first. */
      if (! skip_this_dir)
        SVN_ERR(send_status_structure(wb, local_abspath,
                                      parent_repos_root_url,
                                      parent_repos_relpath,
                                      parent_repos_uuid,
                                      dir_info, dirent, get_all,
                                      status_func, status_baton,
                                      iterpool));

      /* If the requested depth is empty, we only need status on this-dir. */
      if (depth == svn_depth_empty)
        return SVN_NO_ERROR;
    }

  /* Walk all the children of this directory. */
  sorted_children = svn_sort__hash(all_children,
                                   svn_sort_compare_items_lexically,
                                   subpool);
  for (i = 0; i < sorted_children->nelts; i++)
    {
      const void *key;
      apr_ssize_t klen;
      const char *node_abspath;
      svn_io_dirent2_t *dirent_p;
      const struct svn_wc__db_info_t *info;
      svn_sort__item_t item;

      svn_pool_clear(iterpool);

      item = APR_ARRAY_IDX(sorted_children, i, svn_sort__item_t);
      key = item.key;
      klen = item.klen;

      node_abspath = svn_dirent_join(local_abspath, key, iterpool);
      dirent_p = apr_hash_get(dirents, key, klen);

      info = apr_hash_get(nodes, key, klen);
      if (info)
        {
          if (info->status != svn_wc__db_status_not_present
              && info->status != svn_wc__db_status_excluded
              && info->status != svn_wc__db_status_server_excluded)
            {
              if (depth == svn_depth_files
                  && info->kind == svn_wc__db_kind_dir)
                {
                  continue;
                }

              SVN_ERR(send_status_structure(wb, node_abspath,
                                            dir_repos_root_url,
                                            dir_repos_relpath,
                                            dir_repos_uuid,
                                            info, dirent_p, get_all,
                                            status_func, status_baton,
                                            iterpool));

              /* Descend in subdirectories. */
              if (depth == svn_depth_infinity
                  && info->kind == svn_wc__db_kind_dir)
                {
                  SVN_ERR(get_dir_status(wb, node_abspath, NULL, TRUE,
                                         dir_repos_root_url, dir_repos_relpath,
                                         dir_repos_uuid, info,
                                         dirent_p, ignore_patterns,
                                         svn_depth_infinity, get_all,
                                         no_ignore,
                                         status_func, status_baton,
                                         cancel_func, cancel_baton,
                                         iterpool));
                }

              continue;
            }
        }

      if (apr_hash_get(conflicts, key, klen))
        {
          /* Tree conflict */

          if (ignore_patterns && ! patterns)
            SVN_ERR(collect_ignore_patterns(&patterns, wb->db, local_abspath,
                                            ignore_patterns, subpool,
                                            iterpool));

          SVN_ERR(send_unversioned_item(wb,
                                        node_abspath,
                                        dirent_p, TRUE,
                                        patterns,
                                        no_ignore,
                                        status_func,
                                        status_baton,
                                        iterpool));

          continue;
        }

      /* Unversioned node */
      if (dirent_p == NULL)
        continue; /* Selected node, but not found */

      if (depth == svn_depth_files && dirent_p->kind == svn_node_dir)
        continue;

      if (svn_wc_is_adm_dir(key, iterpool))
        continue;

      if (ignore_patterns && ! patterns)
        SVN_ERR(collect_ignore_patterns(&patterns, wb->db, local_abspath,
                                        ignore_patterns, subpool,
                                        iterpool));

      SVN_ERR(send_unversioned_item(wb,
                                    node_abspath,
                                    dirent_p, FALSE,
                                    patterns,
                                    no_ignore || selected,
                                    status_func, status_baton,
                                    iterpool));
    }

  /* Destroy our subpools. */
  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}



/*** Helpers ***/

/* A faux status callback function for stashing STATUS item in an hash
   (which is the BATON), keyed on PATH.  This implements the
   svn_wc_status_func4_t interface. */
static svn_error_t *
hash_stash(void *baton,
           const char *path,
           const svn_wc_status3_t *status,
           apr_pool_t *scratch_pool)
{
  apr_hash_t *stat_hash = baton;
  apr_pool_t *hash_pool = apr_hash_pool_get(stat_hash);
  assert(! apr_hash_get(stat_hash, path, APR_HASH_KEY_STRING));
  apr_hash_set(stat_hash, apr_pstrdup(hash_pool, path),
               APR_HASH_KEY_STRING, svn_wc_dup_status3(status, hash_pool));

  return SVN_NO_ERROR;
}


/* Look up the key PATH in BATON->STATII.  IS_DIR_BATON indicates whether
   baton is a struct *dir_baton or struct *file_baton.  If the value doesn't
   yet exist, and the REPOS_NODE_STATUS indicates that this is an addition,
   create a new status struct using the hash's pool.

   If IS_DIR_BATON is true, THIS_DIR_BATON is a *dir_baton cotaining the out
   of date (ood) information we want to set in BATON.  This is necessary
   because this function tweaks the status of out-of-date directories
   (BATON == THIS_DIR_BATON) and out-of-date directories' parents
   (BATON == THIS_DIR_BATON->parent_baton).  In the latter case THIS_DIR_BATON
   contains the ood info we want to bubble up to ancestor directories so these
   accurately reflect the fact they have an ood descendant.

   Merge REPOS_NODE_STATUS, REPOS_TEXT_STATUS and REPOS_PROP_STATUS into the
   status structure's "network" fields.

   Iff IS_DIR_BATON is true, DELETED_REV is used as follows, otherwise it
   is ignored:

       If REPOS_NODE_STATUS is svn_wc_status_deleted then DELETED_REV is
       optionally the revision path was deleted, in all other cases it must
       be set to SVN_INVALID_REVNUM.  If DELETED_REV is not
       SVN_INVALID_REVNUM and REPOS_TEXT_STATUS is svn_wc_status_deleted,
       then use DELETED_REV to set PATH's ood_last_cmt_rev field in BATON.
       If DELETED_REV is SVN_INVALID_REVNUM and REPOS_NODE_STATUS is
       svn_wc_status_deleted, set PATH's ood_last_cmt_rev to its parent's
       ood_last_cmt_rev value - see comment below.

   If a new struct was added, set the repos_lock to REPOS_LOCK. */
static svn_error_t *
tweak_statushash(void *baton,
                 void *this_dir_baton,
                 svn_boolean_t is_dir_baton,
                 svn_wc__db_t *db,
                 const char *local_abspath,
                 svn_boolean_t is_dir,
                 enum svn_wc_status_kind repos_node_status,
                 enum svn_wc_status_kind repos_text_status,
                 enum svn_wc_status_kind repos_prop_status,
                 svn_revnum_t deleted_rev,
                 const svn_lock_t *repos_lock,
                 apr_pool_t *scratch_pool)
{
  svn_wc_status3_t *statstruct;
  apr_pool_t *pool;
  apr_hash_t *statushash;

  if (is_dir_baton)
    statushash = ((struct dir_baton *) baton)->statii;
  else
    statushash = ((struct file_baton *) baton)->dir_baton->statii;
  pool = apr_hash_pool_get(statushash);

  /* Is PATH already a hash-key? */
  statstruct = apr_hash_get(statushash, local_abspath, APR_HASH_KEY_STRING);

  /* If not, make it so. */
  if (! statstruct)
    {
      /* If this item isn't being added, then we're most likely
         dealing with a non-recursive (or at least partially
         non-recursive) working copy.  Due to bugs in how the client
         reports the state of non-recursive working copies, the
         repository can send back responses about paths that don't
         even exist locally.  Our best course here is just to ignore
         those responses.  After all, if the client had reported
         correctly in the first, that path would either be mentioned
         as an 'add' or not mentioned at all, depending on how we
         eventually fix the bugs in non-recursivity.  See issue
         #2122 for details. */
      if (repos_node_status != svn_wc_status_added)
        return SVN_NO_ERROR;

      /* Use the public API to get a statstruct, and put it into the hash. */
      SVN_ERR(internal_status(&statstruct, db, local_abspath, pool,
                              scratch_pool));
      statstruct->repos_lock = repos_lock;
      apr_hash_set(statushash, apr_pstrdup(pool, local_abspath),
                   APR_HASH_KEY_STRING, statstruct);
    }

  /* Merge a repos "delete" + "add" into a single "replace". */
  if ((repos_node_status == svn_wc_status_added)
      && (statstruct->repos_node_status == svn_wc_status_deleted))
    repos_node_status = svn_wc_status_replaced;

  /* Tweak the structure's repos fields. */
  if (repos_node_status)
    statstruct->repos_node_status = repos_node_status;
  if (repos_text_status)
    statstruct->repos_text_status = repos_text_status;
  if (repos_prop_status)
    statstruct->repos_prop_status = repos_prop_status;

  /* Copy out-of-date info. */
  if (is_dir_baton)
    {
      struct dir_baton *b = this_dir_baton;

      if (!statstruct->repos_relpath && b->repos_relpath)
        {
          if (statstruct->repos_node_status == svn_wc_status_deleted)
            {
              /* When deleting PATH, BATON is for PATH's parent,
                 so we must construct PATH's real statstruct->url. */
              statstruct->repos_relpath =
                            svn_relpath_join(b->repos_relpath,
                                             svn_dirent_basename(local_abspath,
                                                                 NULL),
                                             pool);
            }
          else
            statstruct->repos_relpath = apr_pstrdup(pool, b->repos_relpath);

          statstruct->repos_root_url = 
                              b->edit_baton->anchor_status->repos_root_url;
          statstruct->repos_uuid = 
                              b->edit_baton->anchor_status->repos_uuid;
        }

      /* The last committed date, and author for deleted items
         isn't available. */
      if (statstruct->repos_node_status == svn_wc_status_deleted)
        {
          statstruct->ood_kind = is_dir ? svn_node_dir : svn_node_file;

          /* Pre 1.5 servers don't provide the revision a path was deleted.
             So we punt and use the last committed revision of the path's
             parent, which has some chance of being correct.  At worse it
             is a higher revision than the path was deleted, but this is
             better than nothing... */
          if (deleted_rev == SVN_INVALID_REVNUM)
            statstruct->ood_changed_rev =
              ((struct dir_baton *) baton)->ood_changed_rev;
          else
            statstruct->ood_changed_rev = deleted_rev;
        }
      else
        {
          statstruct->ood_kind = b->ood_kind;
          statstruct->ood_changed_rev = b->ood_changed_rev;
          statstruct->ood_changed_date = b->ood_changed_date;
          if (b->ood_changed_author)
            statstruct->ood_changed_author =
              apr_pstrdup(pool, b->ood_changed_author);
        }

    }
  else
    {
      struct file_baton *b = baton;
      statstruct->ood_changed_rev = b->ood_changed_rev;
      statstruct->ood_changed_date = b->ood_changed_date;
      if (!statstruct->repos_relpath && b->repos_relpath)
        {
          statstruct->repos_relpath = apr_pstrdup(pool, b->repos_relpath);
          statstruct->repos_root_url =
                          b->edit_baton->anchor_status->repos_root_url;
          statstruct->repos_uuid =
                          b->edit_baton->anchor_status->repos_uuid;
        }
      statstruct->ood_kind = b->ood_kind;
      if (b->ood_changed_author)
        statstruct->ood_changed_author =
          apr_pstrdup(pool, b->ood_changed_author);
    }
  return SVN_NO_ERROR;
}

/* Returns the URL for DB */
static const char *
find_dir_repos_relpath(const struct dir_baton *db, apr_pool_t *pool)
{
  /* If we have no name, we're the root, return the anchor URL. */
  if (! db->name)
    return db->edit_baton->anchor_status->repos_relpath;
  else
    {
      const char *repos_relpath;
      struct dir_baton *pb = db->parent_baton;
      const svn_wc_status3_t *status = apr_hash_get(pb->statii,
                                                    db->local_abspath,
                                                    APR_HASH_KEY_STRING);
      /* Note that status->repos_relpath could be NULL in the case of a missing
       * directory, which means we need to recurse up another level to get
       * a useful relpath. */
      if (status && status->repos_relpath)
        return status->repos_relpath;

      repos_relpath = find_dir_repos_relpath(pb, pool);
      return svn_relpath_join(repos_relpath, db->name, pool);
    }
}



/* Create a new dir_baton for subdir PATH. */
static svn_error_t *
make_dir_baton(void **dir_baton,
               const char *path,
               struct edit_baton *edit_baton,
               struct dir_baton *parent_baton,
               apr_pool_t *pool)
{
  struct dir_baton *pb = parent_baton;
  struct edit_baton *eb = edit_baton;
  struct dir_baton *d = apr_pcalloc(pool, sizeof(*d));
  const char *local_abspath;
  const svn_wc_status3_t *status_in_parent;

  SVN_ERR_ASSERT(path || (! pb));

  /* Construct the absolute path of this directory. */
  if (pb)
    local_abspath = svn_dirent_join(eb->anchor_abspath, path, pool);
  else
    local_abspath = eb->anchor_abspath;

  /* Finish populating the baton members. */
  d->local_abspath = local_abspath;
  d->name = path ? svn_dirent_basename(path, pool) : NULL;
  d->edit_baton = edit_baton;
  d->parent_baton = parent_baton;
  d->pool = pool;
  d->statii = apr_hash_make(pool);
  d->ood_changed_rev = SVN_INVALID_REVNUM;
  d->ood_changed_date = 0;
  d->repos_relpath = apr_pstrdup(pool, find_dir_repos_relpath(d, pool));
  d->ood_kind = svn_node_dir;
  d->ood_changed_author = NULL;

  if (pb)
    {
      if (pb->excluded)
        d->excluded = TRUE;
      else if (pb->depth == svn_depth_immediates)
        d->depth = svn_depth_empty;
      else if (pb->depth == svn_depth_files || pb->depth == svn_depth_empty)
        d->excluded = TRUE;
      else if (pb->depth == svn_depth_unknown)
        /* This is only tentative, it can be overridden from d's entry
           later. */
        d->depth = svn_depth_unknown;
      else
        d->depth = svn_depth_infinity;
    }
  else
    {
      d->depth = eb->default_depth;
    }

  /* Get the status for this path's children.  Of course, we only want
     to do this if the path is versioned as a directory. */
  if (pb)
    status_in_parent = apr_hash_get(pb->statii, d->local_abspath,
                                    APR_HASH_KEY_STRING);
  else
    status_in_parent = eb->anchor_status;

  if (status_in_parent
      && status_in_parent->versioned
      && (status_in_parent->kind == svn_node_dir)
      && (! d->excluded)
      && (d->depth == svn_depth_unknown
          || d->depth == svn_depth_infinity
          || d->depth == svn_depth_files
          || d->depth == svn_depth_immediates)
          )
    {
      const svn_wc_status3_t *this_dir_status;
      const apr_array_header_t *ignores = eb->ignores;

      SVN_ERR(get_dir_status(&eb->wb, local_abspath, NULL, TRUE,
                             status_in_parent->repos_root_url,
                             NULL /*parent_repos_relpath*/,
                             status_in_parent->repos_uuid,
                             NULL,
                             NULL /* dirent */, ignores,
                             d->depth == svn_depth_files
                                      ? svn_depth_files
                                      : svn_depth_immediates,
                             TRUE, TRUE,
                             hash_stash, d->statii,
                             eb->cancel_func, eb->cancel_baton,
                             pool));

      /* If we found a depth here, it should govern. */
      this_dir_status = apr_hash_get(d->statii, d->local_abspath,
                                     APR_HASH_KEY_STRING);
      if (this_dir_status && this_dir_status->versioned
          && (d->depth == svn_depth_unknown
              || d->depth > status_in_parent->depth))
        {
          d->depth = this_dir_status->depth;
        }
    }

  *dir_baton = d;
  return SVN_NO_ERROR;
}


/* Make a file baton, using a new subpool of PARENT_DIR_BATON's pool.
   NAME is just one component, not a path. */
static struct file_baton *
make_file_baton(struct dir_baton *parent_dir_baton,
                const char *path,
                apr_pool_t *pool)
{
  struct dir_baton *pb = parent_dir_baton;
  struct edit_baton *eb = pb->edit_baton;
  struct file_baton *f = apr_pcalloc(pool, sizeof(*f));

  /* Finish populating the baton members. */
  f->local_abspath = svn_dirent_join(eb->anchor_abspath, path, pool);
  f->name = svn_dirent_basename(f->local_abspath, NULL);
  f->pool = pool;
  f->dir_baton = pb;
  f->edit_baton = eb;
  f->ood_changed_rev = SVN_INVALID_REVNUM;
  f->ood_changed_date = 0;
  f->repos_relpath = svn_relpath_join(find_dir_repos_relpath(pb, pool),
                                      f->name, pool);
  f->ood_kind = svn_node_file;
  f->ood_changed_author = NULL;
  return f;
}


/**
 * Return a boolean answer to the question "Is @a status something that
 * should be reported?".  @a no_ignore and @a get_all are the same as
 * svn_wc_get_status_editor4().
 */
static svn_boolean_t
is_sendable_status(const svn_wc_status3_t *status,
                   svn_boolean_t no_ignore,
                   svn_boolean_t get_all)
{
  /* If the repository status was touched at all, it's interesting. */
  if (status->repos_node_status != svn_wc_status_none)
    return TRUE;

  /* If there is a lock in the repository, send it. */
  if (status->repos_lock)
    return TRUE;

  if (status->conflicted)
    return TRUE;

  /* If the item is ignored, and we don't want ignores, skip it. */
  if ((status->node_status == svn_wc_status_ignored) && (! no_ignore))
    return FALSE;

  /* If we want everything, we obviously want this single-item subset
     of everything. */
  if (get_all)
    return TRUE;

  /* If the item is unversioned, display it. */
  if (status->node_status == svn_wc_status_unversioned)
    return TRUE;

  /* If the text, property or tree state is interesting, send it. */
  if ((status->node_status != svn_wc_status_none
       && (status->node_status != svn_wc_status_normal)))
    return TRUE;

  /* If it's switched, send it. */
  if (status->switched)
    return TRUE;

  /* If there is a lock token, send it. */
  if (status->versioned && status->lock)
    return TRUE;

  /* If the entry is associated with a changelist, send it. */
  if (status->changelist)
    return TRUE;

  /* Otherwise, don't send it. */
  return FALSE;
}


/* Baton for mark_status. */
struct status_baton
{
  svn_wc_status_func4_t real_status_func;  /* real status function */
  void *real_status_baton;                 /* real status baton */
};

/* A status callback function which wraps the *real* status
   function/baton.   It simply sets the "repos_node_status" field of the
   STATUS to svn_wc_status_deleted and passes it off to the real
   status func/baton. Implements svn_wc_status_func4_t */
static svn_error_t *
mark_deleted(void *baton,
             const char *local_abspath,
             const svn_wc_status3_t *status,
             apr_pool_t *scratch_pool)
{
  struct status_baton *sb = baton;
  svn_wc_status3_t *new_status = svn_wc_dup_status3(status, scratch_pool);
  new_status->repos_node_status = svn_wc_status_deleted;
  return sb->real_status_func(sb->real_status_baton, local_abspath,
                              new_status, scratch_pool);
}


/* Handle a directory's STATII hash.  EB is the edit baton.  DIR_PATH
   and DIR_ENTRY are the on-disk path and entry, respectively, for the
   directory itself.  Descend into subdirectories according to DEPTH.
   Also, if DIR_WAS_DELETED is set, each status that is reported
   through this function will have its repos_text_status field showing
   a deletion.  Use POOL for all allocations. */
static svn_error_t *
handle_statii(struct edit_baton *eb,
              const char *dir_repos_root_url,
              const char *dir_repos_relpath,
              const char *dir_repos_uuid,
              apr_hash_t *statii,
              svn_boolean_t dir_was_deleted,
              svn_depth_t depth,
              apr_pool_t *pool)
{
  const apr_array_header_t *ignores = eb->ignores;
  apr_hash_index_t *hi;
  apr_pool_t *iterpool = svn_pool_create(pool);
  svn_wc_status_func4_t status_func = eb->status_func;
  void *status_baton = eb->status_baton;
  struct status_baton sb;

  if (dir_was_deleted)
    {
      sb.real_status_func = eb->status_func;
      sb.real_status_baton = eb->status_baton;
      status_func = mark_deleted;
      status_baton = &sb;
    }

  /* Loop over all the statii still in our hash, handling each one. */
  for (hi = apr_hash_first(pool, statii); hi; hi = apr_hash_next(hi))
    {
      const char *local_abspath = svn__apr_hash_index_key(hi);
      svn_wc_status3_t *status = svn__apr_hash_index_val(hi);

      /* Clear the subpool. */
      svn_pool_clear(iterpool);

      /* Now, handle the status.  We don't recurse for svn_depth_immediates
         because we already have the subdirectories' statii. */
      if (status->versioned && status->kind == svn_node_dir
          && (depth == svn_depth_unknown
              || depth == svn_depth_infinity))
        {
          SVN_ERR(get_dir_status(&eb->wb,
                                 local_abspath, NULL, TRUE,
                                 dir_repos_root_url, dir_repos_relpath,
                                 dir_repos_uuid,
                                 NULL,
                                 NULL /* dirent */,
                                 ignores, depth, eb->get_all, eb->no_ignore,
                                 status_func, status_baton,
                                 eb->cancel_func, eb->cancel_baton,
                                 iterpool));
        }
      if (dir_was_deleted)
        status->repos_node_status = svn_wc_status_deleted;
      if (is_sendable_status(status, eb->no_ignore, eb->get_all))
        SVN_ERR((eb->status_func)(eb->status_baton, local_abspath, status,
                                  iterpool));
    }

  /* Destroy the subpool. */
  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}


/*----------------------------------------------------------------------*/

/*** The callbacks we'll plug into an svn_delta_editor_t structure. ***/

/* */
static svn_error_t *
set_target_revision(void *edit_baton,
                    svn_revnum_t target_revision,
                    apr_pool_t *pool)
{
  struct edit_baton *eb = edit_baton;
  *(eb->target_revision) = target_revision;
  return SVN_NO_ERROR;
}


/* */
static svn_error_t *
open_root(void *edit_baton,
          svn_revnum_t base_revision,
          apr_pool_t *pool,
          void **dir_baton)
{
  struct edit_baton *eb = edit_baton;
  eb->root_opened = TRUE;
  return make_dir_baton(dir_baton, NULL, eb, NULL, pool);
}


/* */
static svn_error_t *
delete_entry(const char *path,
             svn_revnum_t revision,
             void *parent_baton,
             apr_pool_t *pool)
{
  struct dir_baton *db = parent_baton;
  struct edit_baton *eb = db->edit_baton;
  const char *local_abspath = svn_dirent_join(eb->anchor_abspath, path, pool);
  svn_wc__db_kind_t kind;

  /* Note:  when something is deleted, it's okay to tweak the
     statushash immediately.  No need to wait until close_file or
     close_dir, because there's no risk of having to honor the 'added'
     flag.  We already know this item exists in the working copy. */

  SVN_ERR(svn_wc__db_read_kind(&kind, eb->db, local_abspath, FALSE, pool));
  SVN_ERR(tweak_statushash(db, db, TRUE, eb->db,
                           local_abspath, kind == svn_wc__db_kind_dir,
                           svn_wc_status_deleted, 0, 0, revision, NULL, pool));

  /* Mark the parent dir -- it lost an entry (unless that parent dir
     is the root node and we're not supposed to report on the root
     node).  */
  if (db->parent_baton && (! *eb->target_basename))
    SVN_ERR(tweak_statushash(db->parent_baton, db, TRUE,eb->db,
                             db->local_abspath,
                             kind == svn_wc__db_kind_dir,
                             svn_wc_status_modified, svn_wc_status_modified,
                             0, SVN_INVALID_REVNUM, NULL, pool));

  return SVN_NO_ERROR;
}


/* */
static svn_error_t *
add_directory(const char *path,
              void *parent_baton,
              const char *copyfrom_path,
              svn_revnum_t copyfrom_revision,
              apr_pool_t *pool,
              void **child_baton)
{
  struct dir_baton *pb = parent_baton;
  struct edit_baton *eb = pb->edit_baton;
  struct dir_baton *new_db;

  SVN_ERR(make_dir_baton(child_baton, path, eb, pb, pool));

  /* Make this dir as added. */
  new_db = *child_baton;
  new_db->added = TRUE;

  /* Mark the parent as changed;  it gained an entry. */
  pb->text_changed = TRUE;

  return SVN_NO_ERROR;
}


/* */
static svn_error_t *
open_directory(const char *path,
               void *parent_baton,
               svn_revnum_t base_revision,
               apr_pool_t *pool,
               void **child_baton)
{
  struct dir_baton *pb = parent_baton;
  return make_dir_baton(child_baton, path, pb->edit_baton, pb, pool);
}


/* */
static svn_error_t *
change_dir_prop(void *dir_baton,
                const char *name,
                const svn_string_t *value,
                apr_pool_t *pool)
{
  struct dir_baton *db = dir_baton;
  if (svn_wc_is_normal_prop(name))
    db->prop_changed = TRUE;

  /* Note any changes to the repository. */
  if (value != NULL)
    {
      if (strcmp(name, SVN_PROP_ENTRY_COMMITTED_REV) == 0)
        db->ood_changed_rev = SVN_STR_TO_REV(value->data);
      else if (strcmp(name, SVN_PROP_ENTRY_LAST_AUTHOR) == 0)
        db->ood_changed_author = apr_pstrdup(db->pool, value->data);
      else if (strcmp(name, SVN_PROP_ENTRY_COMMITTED_DATE) == 0)
        {
          apr_time_t tm;
          SVN_ERR(svn_time_from_cstring(&tm, value->data, db->pool));
          db->ood_changed_date = tm;
        }
    }

  return SVN_NO_ERROR;
}



/* */
static svn_error_t *
close_directory(void *dir_baton,
                apr_pool_t *pool)
{
  struct dir_baton *db = dir_baton;
  struct dir_baton *pb = db->parent_baton;
  struct edit_baton *eb = db->edit_baton;

  /* If nothing has changed and directory has no out of
     date descendants, return. */
  if (db->added || db->prop_changed || db->text_changed
      || db->ood_changed_rev != SVN_INVALID_REVNUM)
    {
      enum svn_wc_status_kind repos_node_status;
      enum svn_wc_status_kind repos_text_status;
      enum svn_wc_status_kind repos_prop_status;

      /* If this is a new directory, add it to the statushash. */
      if (db->added)
        {
          repos_node_status = svn_wc_status_added;
          repos_text_status = svn_wc_status_added;
          repos_prop_status = db->prop_changed ? svn_wc_status_added
                              : svn_wc_status_none;
        }
      else
        {
          repos_node_status = (db->text_changed || db->prop_changed)
                                       ? svn_wc_status_modified
                                       : svn_wc_status_none;
          repos_text_status = db->text_changed ? svn_wc_status_modified
                              : svn_wc_status_none;
          repos_prop_status = db->prop_changed ? svn_wc_status_modified
                              : svn_wc_status_none;
        }

      /* Maybe add this directory to its parent's status hash.  Note
         that tweak_statushash won't do anything if repos_text_status
         is not svn_wc_status_added. */
      if (pb)
        {
          /* ### When we add directory locking, we need to find a
             ### directory lock here. */
          SVN_ERR(tweak_statushash(pb, db, TRUE, eb->db, db->local_abspath,
                                   TRUE, repos_node_status, repos_text_status,
                                   repos_prop_status, SVN_INVALID_REVNUM, NULL,
                                   pool));
        }
      else
        {
          /* We're editing the root dir of the WC.  As its repos
             status info isn't otherwise set, set it directly to
             trigger invocation of the status callback below. */
          eb->anchor_status->repos_node_status = repos_node_status;
          eb->anchor_status->repos_prop_status = repos_prop_status;
          eb->anchor_status->repos_text_status = repos_text_status;

          /* If the root dir is out of date set the ood info directly too. */
          if (db->ood_changed_rev != eb->anchor_status->revision)
            {
              eb->anchor_status->ood_changed_rev = db->ood_changed_rev;
              eb->anchor_status->ood_changed_date = db->ood_changed_date;
              eb->anchor_status->ood_kind = db->ood_kind;
              eb->anchor_status->ood_changed_author =
                apr_pstrdup(pool, db->ood_changed_author);
            }
        }
    }

  /* Handle this directory's statuses, and then note in the parent
     that this has been done. */
  if (pb && ! db->excluded)
    {
      svn_boolean_t was_deleted = FALSE;
      const svn_wc_status3_t *dir_status;

      /* See if the directory was deleted or replaced. */
      dir_status = apr_hash_get(pb->statii, db->local_abspath,
                                APR_HASH_KEY_STRING);
      if (dir_status &&
          ((dir_status->repos_node_status == svn_wc_status_deleted)
           || (dir_status->repos_node_status == svn_wc_status_replaced)))
        was_deleted = TRUE;

      /* Now do the status reporting. */
      SVN_ERR(handle_statii(eb,
                            dir_status ? dir_status->repos_root_url : NULL,
                            dir_status ? dir_status->repos_relpath : NULL,
                            dir_status ? dir_status->repos_uuid : NULL,
                            db->statii, was_deleted, db->depth, pool));
      if (dir_status && is_sendable_status(dir_status, eb->no_ignore,
                                           eb->get_all))
        SVN_ERR((eb->status_func)(eb->status_baton, db->local_abspath,
                                  dir_status, pool));
      apr_hash_set(pb->statii, db->local_abspath, APR_HASH_KEY_STRING, NULL);
    }
  else if (! pb)
    {
      /* If this is the top-most directory, and the operation had a
         target, we should only report the target. */
      if (*eb->target_basename)
        {
          const svn_wc_status3_t *tgt_status;

          tgt_status = apr_hash_get(db->statii, eb->target_abspath,
                                    APR_HASH_KEY_STRING);
          if (tgt_status)
            {
              if (tgt_status->versioned
                  && tgt_status->kind == svn_node_dir)
                {
                  SVN_ERR(get_dir_status(&eb->wb,
                                         eb->target_abspath, NULL, TRUE,
                                         NULL, NULL, NULL, NULL,
                                         NULL /* dirent */,
                                         eb->ignores,
                                         eb->default_depth,
                                         eb->get_all, eb->no_ignore,
                                         eb->status_func, eb->status_baton,
                                         eb->cancel_func, eb->cancel_baton,
                                         pool));
                }
              if (is_sendable_status(tgt_status, eb->no_ignore, eb->get_all))
                SVN_ERR((eb->status_func)(eb->status_baton, eb->target_abspath,
                                          tgt_status, pool));
            }
        }
      else
        {
          /* Otherwise, we report on all our children and ourself.
             Note that our directory couldn't have been deleted,
             because it is the root of the edit drive. */
          SVN_ERR(handle_statii(eb,
                                eb->anchor_status->repos_root_url,
                                eb->anchor_status->repos_relpath,
                                eb->anchor_status->repos_uuid,
                                db->statii, FALSE, eb->default_depth, pool));
          if (is_sendable_status(eb->anchor_status, eb->no_ignore,
                                 eb->get_all))
            SVN_ERR((eb->status_func)(eb->status_baton, db->local_abspath,
                                      eb->anchor_status, pool));
          eb->anchor_status = NULL;
        }
    }
  return SVN_NO_ERROR;
}



/* */
static svn_error_t *
add_file(const char *path,
         void *parent_baton,
         const char *copyfrom_path,
         svn_revnum_t copyfrom_revision,
         apr_pool_t *pool,
         void **file_baton)
{
  struct dir_baton *pb = parent_baton;
  struct file_baton *new_fb = make_file_baton(pb, path, pool);

  /* Mark parent dir as changed */
  pb->text_changed = TRUE;

  /* Make this file as added. */
  new_fb->added = TRUE;

  *file_baton = new_fb;
  return SVN_NO_ERROR;
}


/* */
static svn_error_t *
open_file(const char *path,
          void *parent_baton,
          svn_revnum_t base_revision,
          apr_pool_t *pool,
          void **file_baton)
{
  struct dir_baton *pb = parent_baton;
  struct file_baton *new_fb = make_file_baton(pb, path, pool);

  *file_baton = new_fb;
  return SVN_NO_ERROR;
}


/* */
static svn_error_t *
apply_textdelta(void *file_baton,
                const char *base_checksum,
                apr_pool_t *pool,
                svn_txdelta_window_handler_t *handler,
                void **handler_baton)
{
  struct file_baton *fb = file_baton;

  /* Mark file as having textual mods. */
  fb->text_changed = TRUE;

  /* Send back a NULL window handler -- we don't need the actual diffs. */
  *handler_baton = NULL;
  *handler = svn_delta_noop_window_handler;

  return SVN_NO_ERROR;
}


/* */
static svn_error_t *
change_file_prop(void *file_baton,
                 const char *name,
                 const svn_string_t *value,
                 apr_pool_t *pool)
{
  struct file_baton *fb = file_baton;
  if (svn_wc_is_normal_prop(name))
    fb->prop_changed = TRUE;

  /* Note any changes to the repository. */
  if (value != NULL)
    {
      if (strcmp(name, SVN_PROP_ENTRY_COMMITTED_REV) == 0)
        fb->ood_changed_rev = SVN_STR_TO_REV(value->data);
      else if (strcmp(name, SVN_PROP_ENTRY_LAST_AUTHOR) == 0)
        fb->ood_changed_author = apr_pstrdup(fb->dir_baton->pool,
                                              value->data);
      else if (strcmp(name, SVN_PROP_ENTRY_COMMITTED_DATE) == 0)
        {
          apr_time_t tm;
          SVN_ERR(svn_time_from_cstring(&tm, value->data,
                                        fb->dir_baton->pool));
          fb->ood_changed_date = tm;
        }
    }

  return SVN_NO_ERROR;
}


/* */
static svn_error_t *
close_file(void *file_baton,
           const char *text_checksum,  /* ignored, as we receive no data */
           apr_pool_t *pool)
{
  struct file_baton *fb = file_baton;
  enum svn_wc_status_kind repos_node_status;
  enum svn_wc_status_kind repos_text_status;
  enum svn_wc_status_kind repos_prop_status;
  const svn_lock_t *repos_lock = NULL;

  /* If nothing has changed, return. */
  if (! (fb->added || fb->prop_changed || fb->text_changed))
    return SVN_NO_ERROR;

  /* If this is a new file, add it to the statushash. */
  if (fb->added)
    {
      repos_node_status = svn_wc_status_added;
      repos_text_status = svn_wc_status_added;
      repos_prop_status = fb->prop_changed ? svn_wc_status_added : 0;

      if (fb->edit_baton->wb.repos_locks)
        {
          const char *dir_repos_relpath = find_dir_repos_relpath(fb->dir_baton,
                                                                 pool);

          /* repos_lock still uses the deprecated filesystem absolute path
             format */
          const char *repos_relpath = svn_relpath_join(dir_repos_relpath,
                                                       fb->name, pool);

          repos_lock = apr_hash_get(fb->edit_baton->wb.repos_locks,
                                    svn_fspath__join("/", repos_relpath,
                                                     pool),
                                    APR_HASH_KEY_STRING);
        }
    }
  else
    {
      repos_node_status = (fb->text_changed || fb->prop_changed)
                                 ? svn_wc_status_modified : 0;
      repos_text_status = fb->text_changed ? svn_wc_status_modified : 0;
      repos_prop_status = fb->prop_changed ? svn_wc_status_modified : 0;
    }

  return tweak_statushash(fb, NULL, FALSE, fb->edit_baton->db,
                          fb->local_abspath, FALSE, repos_node_status,
                          repos_text_status, repos_prop_status,
                          SVN_INVALID_REVNUM, repos_lock, pool);
}

/* */
static svn_error_t *
close_edit(void *edit_baton,
           apr_pool_t *pool)
{
  struct edit_baton *eb = edit_baton;

  /* If we get here and the root was not opened as part of the edit,
     we need to transmit statuses for everything.  Otherwise, we
     should be done. */
  if (eb->root_opened)
    return SVN_NO_ERROR;

  SVN_ERR(svn_wc_walk_status(eb->wc_ctx,
                             eb->target_abspath,
                             eb->default_depth,
                             eb->get_all,
                             eb->no_ignore,
                             FALSE,
                             eb->ignores,
                             eb->status_func,
                             eb->status_baton,
                             eb->cancel_func,
                             eb->cancel_baton,
                             pool));

  return SVN_NO_ERROR;
}



/*** Public API ***/

svn_error_t *
svn_wc_get_status_editor5(const svn_delta_editor_t **editor,
                          void **edit_baton,
                          void **set_locks_baton,
                          svn_revnum_t *edit_revision,
                          svn_wc_context_t *wc_ctx,
                          const char *anchor_abspath,
                          const char *target_basename,
                          svn_depth_t depth,
                          svn_boolean_t get_all,
                          svn_boolean_t no_ignore,
                          svn_boolean_t depth_as_sticky,
                          svn_boolean_t server_performs_filtering,
                          const apr_array_header_t *ignore_patterns,
                          svn_wc_status_func4_t status_func,
                          void *status_baton,
                          svn_cancel_func_t cancel_func,
                          void *cancel_baton,
                          apr_pool_t *result_pool,
                          apr_pool_t *scratch_pool)
{
  struct edit_baton *eb;
  svn_delta_editor_t *tree_editor = svn_delta_default_editor(result_pool);
  void *inner_baton;
  const svn_delta_editor_t *inner_editor;

  /* Construct an edit baton. */
  eb = apr_pcalloc(result_pool, sizeof(*eb));
  eb->default_depth     = depth;
  eb->target_revision   = edit_revision;
  eb->db                = wc_ctx->db;
  eb->wc_ctx            = wc_ctx;
  eb->get_all           = get_all;
  eb->no_ignore         = no_ignore;
  eb->status_func       = status_func;
  eb->status_baton      = status_baton;
  eb->cancel_func       = cancel_func;
  eb->cancel_baton      = cancel_baton;
  eb->anchor_abspath    = apr_pstrdup(result_pool, anchor_abspath);
  eb->target_abspath    = svn_dirent_join(anchor_abspath, target_basename,
                                          result_pool);

  eb->target_basename   = apr_pstrdup(result_pool, target_basename);
  eb->root_opened       = FALSE;

  eb->wb.db               = wc_ctx->db;
  eb->wb.target_abspath   = eb->target_abspath;
  eb->wb.ignore_text_mods = FALSE;
  eb->wb.repos_locks      = NULL;
  eb->wb.repos_root       = NULL;

  SVN_ERR(svn_wc__db_externals_defined_below(&eb->wb.externals,
                                             wc_ctx->db, eb->target_abspath,
                                             result_pool, scratch_pool));

  /* Use the caller-provided ignore patterns if provided; the build-time
     configured defaults otherwise. */
  if (ignore_patterns)
    {
      eb->ignores = ignore_patterns;
    }
  else
    {
      apr_array_header_t *ignores;

      SVN_ERR(svn_wc_get_default_ignores(&ignores, NULL, result_pool));
      eb->ignores = ignores;
    }

  /* The edit baton's status structure maps to PATH, and the editor
     have to be aware of whether that is the anchor or the target. */
  SVN_ERR(internal_status(&(eb->anchor_status), wc_ctx->db, anchor_abspath,
                         result_pool, scratch_pool));

  /* Construct an editor. */
  tree_editor->set_target_revision = set_target_revision;
  tree_editor->open_root = open_root;
  tree_editor->delete_entry = delete_entry;
  tree_editor->add_directory = add_directory;
  tree_editor->open_directory = open_directory;
  tree_editor->change_dir_prop = change_dir_prop;
  tree_editor->close_directory = close_directory;
  tree_editor->add_file = add_file;
  tree_editor->open_file = open_file;
  tree_editor->apply_textdelta = apply_textdelta;
  tree_editor->change_file_prop = change_file_prop;
  tree_editor->close_file = close_file;
  tree_editor->close_edit = close_edit;

  inner_editor = tree_editor;
  inner_baton = eb;

  if (!server_performs_filtering
      && !depth_as_sticky)
    SVN_ERR(svn_wc__ambient_depth_filter_editor(&inner_editor,
                                                &inner_baton,
                                                wc_ctx->db,
                                                anchor_abspath,
                                                target_basename,
                                                inner_editor,
                                                inner_baton,
                                                result_pool));

  /* Conjoin a cancellation editor with our status editor. */
  SVN_ERR(svn_delta_get_cancellation_editor(cancel_func, cancel_baton,
                                            inner_editor, inner_baton,
                                            editor, edit_baton,
                                            result_pool));

  if (set_locks_baton)
    *set_locks_baton = eb;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__internal_walk_status(svn_wc__db_t *db,
                             const char *local_abspath,
                             svn_depth_t depth,
                             svn_boolean_t get_all,
                             svn_boolean_t no_ignore,
                             svn_boolean_t ignore_text_mods,
                             const apr_array_header_t *ignore_patterns,
                             svn_wc_status_func4_t status_func,
                             void *status_baton,
                             svn_cancel_func_t cancel_func,
                             void *cancel_baton,
                             apr_pool_t *scratch_pool)
{
  struct walk_status_baton wb;
  const svn_io_dirent2_t *dirent;
  const char *anchor_abspath, *target_name;
  svn_boolean_t skip_root;
  const struct svn_wc__db_info_t *dir_info;
  svn_error_t *err;

  wb.db = db;
  wb.target_abspath = local_abspath;
  wb.ignore_text_mods = ignore_text_mods;
  wb.repos_root = NULL;
  wb.repos_locks = NULL;

  SVN_ERR(svn_wc__db_externals_defined_below(&wb.externals, db, local_abspath,
                                             scratch_pool, scratch_pool));

  /* Use the caller-provided ignore patterns if provided; the build-time
     configured defaults otherwise. */
  if (!ignore_patterns)
    {
      apr_array_header_t *ignores;

      SVN_ERR(svn_wc_get_default_ignores(&ignores, NULL, scratch_pool));
      ignore_patterns = ignores;
    }

  err = read_info(&dir_info, local_abspath, db, scratch_pool, scratch_pool);

  if (!err
      && dir_info->kind == svn_wc__db_kind_dir
      && dir_info->status != svn_wc__db_status_not_present
      && dir_info->status != svn_wc__db_status_excluded
      && dir_info->status != svn_wc__db_status_server_excluded)
    {
      anchor_abspath = local_abspath;
      target_name = NULL;
      skip_root = FALSE;
    }
  else if (err && err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND)
    return svn_error_trace(err);
  else
    {
      svn_error_clear(err);
      dir_info = NULL; /* Don't pass information of the child */

      /* Walk the status of the parent of LOCAL_ABSPATH, but only report
         status on its child LOCAL_ABSPATH. */
      anchor_abspath = svn_dirent_dirname(local_abspath, scratch_pool);
      target_name = svn_dirent_basename(local_abspath, NULL);
      skip_root = TRUE;
    }

  SVN_ERR(svn_io_stat_dirent(&dirent, local_abspath, TRUE,
                             scratch_pool, scratch_pool));

#ifdef HAVE_SYMLINK
  if (dirent->special && !skip_root)
    {
      svn_io_dirent2_t *this_dirent = svn_io_dirent2_dup(dirent,
                                                         scratch_pool);

      /* We're being pointed to the status root via a symlink.
       * Get the real node kind and pretend the path is not a symlink.
       * This prevents send_status_structure() from treating the root
       * as a directory obstructed by a file. */
      SVN_ERR(svn_io_check_resolved_path(local_abspath,
                                         &this_dirent->kind, scratch_pool));
      this_dirent->special = FALSE;
      SVN_ERR(send_status_structure(&wb, local_abspath,
                                    NULL, NULL, NULL,
                                    dir_info, this_dirent, get_all,
                                    status_func, status_baton,
                                    scratch_pool));
      skip_root = TRUE;
    }
#endif

  SVN_ERR(get_dir_status(&wb,
                         anchor_abspath,
                         target_name,
                         skip_root,
                         NULL, NULL, NULL,
                         dir_info,
                         dirent,
                         ignore_patterns,
                         depth,
                         get_all,
                         no_ignore,
                         status_func, status_baton,
                         cancel_func, cancel_baton,
                         scratch_pool));

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc_walk_status(svn_wc_context_t *wc_ctx,
                   const char *local_abspath,
                   svn_depth_t depth,
                   svn_boolean_t get_all,
                   svn_boolean_t no_ignore,
                   svn_boolean_t ignore_text_mods,
                   const apr_array_header_t *ignore_patterns,
                   svn_wc_status_func4_t status_func,
                   void *status_baton,
                   svn_cancel_func_t cancel_func,
                   void *cancel_baton,
                   apr_pool_t *scratch_pool)
{
  return svn_error_trace(
           svn_wc__internal_walk_status(wc_ctx->db,
                                        local_abspath,
                                        depth,
                                        get_all,
                                        no_ignore,
                                        ignore_text_mods,
                                        ignore_patterns,
                                        status_func,
                                        status_baton,
                                        cancel_func,
                                        cancel_baton,
                                        scratch_pool));
}


svn_error_t *
svn_wc_status_set_repos_locks(void *edit_baton,
                              apr_hash_t *locks,
                              const char *repos_root,
                              apr_pool_t *pool)
{
  struct edit_baton *eb = edit_baton;

  eb->wb.repos_locks = locks;
  eb->wb.repos_root = apr_pstrdup(pool, repos_root);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc_get_default_ignores(apr_array_header_t **patterns,
                           apr_hash_t *config,
                           apr_pool_t *pool)
{
  svn_config_t *cfg = config ? apr_hash_get(config,
                                            SVN_CONFIG_CATEGORY_CONFIG,
                                            APR_HASH_KEY_STRING) : NULL;
  const char *val;

  /* Check the Subversion run-time configuration for global ignores.
     If no configuration value exists, we fall back to our defaults. */
  svn_config_get(cfg, &val, SVN_CONFIG_SECTION_MISCELLANY,
                 SVN_CONFIG_OPTION_GLOBAL_IGNORES,
                 SVN_CONFIG_DEFAULT_GLOBAL_IGNORES);
  *patterns = apr_array_make(pool, 16, sizeof(const char *));

  /* Split the patterns on whitespace, and stuff them into *PATTERNS. */
  svn_cstring_split_append(*patterns, val, "\n\r\t\v ", FALSE, pool);
  return SVN_NO_ERROR;
}


/* */
static svn_error_t *
internal_status(svn_wc_status3_t **status,
                svn_wc__db_t *db,
                const char *local_abspath,
                apr_pool_t *result_pool,
                apr_pool_t *scratch_pool)
{
  const svn_io_dirent2_t *dirent;
  svn_wc__db_kind_t node_kind;
  const char *parent_repos_relpath;
  const char *parent_repos_root_url;
  const char *parent_repos_uuid;
  svn_wc__db_status_t node_status;
  svn_boolean_t conflicted;
  svn_boolean_t is_root = FALSE;
  svn_error_t *err;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(local_abspath));

  SVN_ERR(svn_io_stat_dirent(&dirent, local_abspath, TRUE,
                             scratch_pool, scratch_pool));

  err = svn_wc__db_read_info(&node_status, &node_kind, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL, &conflicted,
                             NULL, NULL, NULL, NULL, NULL, NULL,
                             db, local_abspath,
                             scratch_pool, scratch_pool);

  if (err && err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
    {
      svn_error_clear(err);
      node_kind = svn_wc__db_kind_unknown;
      /* Ensure conflicted is always set, but don't hide tree conflicts
         on 'hidden' nodes. */
      conflicted = FALSE;
    }
  else if (err)
    {
        return svn_error_trace(err);
    }
  else if (node_status == svn_wc__db_status_not_present
           || node_status == svn_wc__db_status_server_excluded
           || node_status == svn_wc__db_status_excluded)
    {
      node_kind = svn_wc__db_kind_unknown;
    }

  if (node_kind == svn_wc__db_kind_unknown)
    return svn_error_trace(assemble_unversioned(status,
                                                db, local_abspath,
                                                dirent, conflicted,
                                                FALSE /* is_ignored */,
                                                result_pool, scratch_pool));

  if (svn_dirent_is_root(local_abspath, strlen(local_abspath)))
    is_root = TRUE;
  else
    SVN_ERR(svn_wc__db_is_wcroot(&is_root, db, local_abspath, scratch_pool));

  if (!is_root)
    {
      svn_wc__db_status_t parent_status;
      const char *parent_abspath = svn_dirent_dirname(local_abspath,
                                                      scratch_pool);

      err = svn_wc__db_read_info(&parent_status, NULL, NULL,
                                 &parent_repos_relpath, &parent_repos_root_url,
                                 &parent_repos_uuid, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL,
                                 db, parent_abspath,
                                 result_pool, scratch_pool);

      if (err && (err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND
                  || SVN_WC__ERR_IS_NOT_CURRENT_WC(err)))
        {
          svn_error_clear(err);
          parent_repos_root_url = NULL;
          parent_repos_relpath = NULL;
          parent_repos_uuid = NULL;
        }
      else SVN_ERR(err);
    }
  else
    {
      parent_repos_root_url = NULL;
      parent_repos_relpath = NULL;
      parent_repos_uuid = NULL;
    }

  return svn_error_trace(assemble_status(status, db, local_abspath,
                                         parent_repos_root_url,
                                         parent_repos_relpath,
                                         parent_repos_uuid,
                                         NULL,
                                         dirent,
                                         TRUE /* get_all */,
                                         FALSE,
                                         NULL /* repos_lock */,
                                         result_pool, scratch_pool));
}


svn_error_t *
svn_wc_status3(svn_wc_status3_t **status,
               svn_wc_context_t *wc_ctx,
               const char *local_abspath,
               apr_pool_t *result_pool,
               apr_pool_t *scratch_pool)
{
  return svn_error_trace(
    internal_status(status, wc_ctx->db, local_abspath, result_pool,
                    scratch_pool));
}

svn_wc_status3_t *
svn_wc_dup_status3(const svn_wc_status3_t *orig_stat,
                   apr_pool_t *pool)
{
  svn_wc_status3_t *new_stat = apr_palloc(pool, sizeof(*new_stat));

  /* Shallow copy all members. */
  *new_stat = *orig_stat;

  /* Now go back and dup the deep items into this pool. */
  if (orig_stat->repos_lock)
    new_stat->repos_lock = svn_lock_dup(orig_stat->repos_lock, pool);

  if (orig_stat->changed_author)
    new_stat->changed_author = apr_pstrdup(pool, orig_stat->changed_author);

  if (orig_stat->ood_changed_author)
    new_stat->ood_changed_author
      = apr_pstrdup(pool, orig_stat->ood_changed_author);

  if (orig_stat->lock)
    new_stat->lock = svn_lock_dup(orig_stat->lock, pool);

  if (orig_stat->changelist)
    new_stat->changelist
      = apr_pstrdup(pool, orig_stat->changelist);

  if (orig_stat->repos_root_url)
    new_stat->repos_root_url
      = apr_pstrdup(pool, orig_stat->repos_root_url);

  if (orig_stat->repos_relpath)
    new_stat->repos_relpath
      = apr_pstrdup(pool, orig_stat->repos_relpath);

  if (orig_stat->repos_uuid)
    new_stat->repos_uuid
      = apr_pstrdup(pool, orig_stat->repos_uuid);

  /* Return the new hotness. */
  return new_stat;
}

svn_error_t *
svn_wc_get_ignores2(apr_array_header_t **patterns,
                    svn_wc_context_t *wc_ctx,
                    const char *local_abspath,
                    apr_hash_t *config,
                    apr_pool_t *result_pool,
                    apr_pool_t *scratch_pool)
{
  apr_array_header_t *default_ignores;

  SVN_ERR(svn_wc_get_default_ignores(&default_ignores, config, scratch_pool));
  return svn_error_trace(collect_ignore_patterns(patterns, wc_ctx->db,
                                                 local_abspath,
                                                 default_ignores,
                                                 result_pool, scratch_pool));
}

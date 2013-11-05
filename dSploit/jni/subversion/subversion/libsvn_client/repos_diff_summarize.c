/*
 * repos_diff_summarize.c -- The diff summarize editor for summarizing
 * the differences of two repository versions
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


#include "svn_dirent_uri.h"
#include "svn_props.h"
#include "svn_pools.h"

#include "client.h"


/* Overall crawler editor baton.  */
struct edit_baton {
  /* The target of the diff, relative to the root of the edit */
  const char *target;

  /* The summarize callback passed down from the API */
  svn_client_diff_summarize_func_t summarize_func;

  /* The summarize callback baton */
  void *summarize_func_baton;

  /* An RA session used to check the kind of deleted paths */
  svn_ra_session_t *ra_session;

  /* The start revision for the comparison */
  svn_revnum_t revision;

  /* TRUE if the operation needs to walk deleted dirs on the "old" side.
     FALSE otherwise. */
  svn_boolean_t walk_deleted_repos_dirs;

  /* A callback used to see if the client wishes to cancel the running
     operation. */
  svn_cancel_func_t cancel_func;

  /* A baton to pass to the cancellation callback. */
  void *cancel_baton;

};


/* Item baton. */
struct item_baton {
  /* The overall crawler editor baton */
  struct edit_baton *edit_baton;

  /* The summarize filled by the editor calls, NULL if this item hasn't
     been modified (yet) */
  svn_client_diff_summarize_t *summarize;

  /* The path of the file or directory within the repository */
  const char *path;

  /* The kind of this item */
  svn_node_kind_t node_kind;

  /* The file/directory pool */
  apr_pool_t *item_pool;
};


/* Create an item baton, with the fields initialized to EDIT_BATON, PATH,
 * NODE_KIND and POOL, respectively.  Allocate the returned structure in POOL.
 */
static struct item_baton *
create_item_baton(struct edit_baton *edit_baton,
                  const char *path,
                  svn_node_kind_t node_kind,
                  apr_pool_t *pool)
{
  struct item_baton *b = apr_pcalloc(pool, sizeof(*b));

  b->edit_baton = edit_baton;
  /* Issues #2765 & #4408: b->path is supposed to be relative to the target.
     This way the receiver can just concatenate this path to the original
     path without doing any extra checks. */
  b->path = apr_pstrdup(pool,
                        svn_relpath_skip_ancestor(edit_baton->target, path));
  b->node_kind = node_kind;
  b->item_pool = pool;

  return b;
}

/* Make sure that this item baton contains a summarize struct.
 * If it doesn't before this call, allocate a new struct in the item's pool,
 * initializing the diff kind to normal.
 * All other fields are also initialized from IB or to NULL/invalid values. */
static void
ensure_summarize(struct item_baton *ib)
{
  svn_client_diff_summarize_t *sum;
  if (ib->summarize)
    return;

  sum = apr_pcalloc(ib->item_pool, sizeof(*sum));

  sum->node_kind = ib->node_kind;
  sum->summarize_kind = svn_client_diff_summarize_kind_normal;
  sum->path = ib->path;

  ib->summarize = sum;
}


/* An editor function. The root of the comparison hierarchy */
static svn_error_t *
open_root(void *eb,
          svn_revnum_t base_revision,
          apr_pool_t *pool,
          void **root_baton)
{
  struct edit_baton *b = eb;
  struct item_baton *ib = create_item_baton(eb, b->target,
                                            svn_node_dir, pool);

  *root_baton = ib;
  return SVN_NO_ERROR;
}

/* Recursively walk the tree rooted at DIR (at REVISION) in the
 * repository, reporting all files as deleted.  Part of a workaround
 * for issue 2333.
 *
 * DIR is a repository path relative to the URL in RA_SESSION.  REVISION
 * may be NULL, in which case it defaults to HEAD.  EDIT_BATON is the
 * overall crawler editor baton.  If CANCEL_FUNC is not NULL, then it
 * should refer to a cancellation function (along with CANCEL_BATON).
 */
/* ### TODO: Handle depth. */
static svn_error_t *
diff_deleted_dir(const char *dir,
                 svn_revnum_t revision,
                 svn_ra_session_t *ra_session,
                 void *edit_baton,
                 svn_cancel_func_t cancel_func,
                 void *cancel_baton,
                 apr_pool_t *pool)
{
  struct edit_baton *eb = edit_baton;
  apr_hash_t *dirents;
  apr_pool_t *iterpool = svn_pool_create(pool);
  apr_hash_index_t *hi;

  if (cancel_func)
    SVN_ERR(cancel_func(cancel_baton));

  SVN_ERR(svn_ra_get_dir2(ra_session,
                          &dirents,
                          NULL, NULL,
                          dir,
                          revision,
                          SVN_DIRENT_KIND,
                          pool));

  for (hi = apr_hash_first(pool, dirents); hi;
       hi = apr_hash_next(hi))
    {
      const char *path;
      const char *name = svn__apr_hash_index_key(hi);
      svn_dirent_t *dirent = svn__apr_hash_index_val(hi);
      svn_node_kind_t kind;
      svn_client_diff_summarize_t *sum;

      svn_pool_clear(iterpool);

      path = svn_relpath_join(dir, name, iterpool);

      SVN_ERR(svn_ra_check_path(eb->ra_session,
                                path,
                                eb->revision,
                                &kind,
                                iterpool));

      sum = apr_pcalloc(iterpool, sizeof(*sum));
      sum->summarize_kind = svn_client_diff_summarize_kind_deleted;
      /* Issue #4408: sum->path should be relative to target. */
      sum->path = svn_relpath_skip_ancestor(eb->target, path);
      sum->node_kind = kind;

      SVN_ERR(eb->summarize_func(sum,
                                 eb->summarize_func_baton,
                                 iterpool));

      if (dirent->kind == svn_node_dir)
        SVN_ERR(diff_deleted_dir(path,
                                 revision,
                                 ra_session,
                                 eb,
                                 cancel_func,
                                 cancel_baton,
                                 iterpool));
    }

  svn_pool_destroy(iterpool);
  return SVN_NO_ERROR;
}

/* An editor function.  */
static svn_error_t *
delete_entry(const char *path,
             svn_revnum_t base_revision,
             void *parent_baton,
             apr_pool_t *pool)
{
  struct item_baton *ib = parent_baton;
  struct edit_baton *eb = ib->edit_baton;
  svn_client_diff_summarize_t *sum;
  svn_node_kind_t kind;

  /* We need to know if this is a directory or a file */
  SVN_ERR(svn_ra_check_path(eb->ra_session,
                            path,
                            eb->revision,
                            &kind,
                            pool));

  sum = apr_pcalloc(pool, sizeof(*sum));
  sum->summarize_kind = svn_client_diff_summarize_kind_deleted;
  /* Issue #4408: sum->path should be relative to target. */
  sum->path = svn_relpath_skip_ancestor(eb->target, path);
  sum->node_kind = kind;

  SVN_ERR(eb->summarize_func(sum, eb->summarize_func_baton, pool));

  if (kind == svn_node_dir)
        SVN_ERR(diff_deleted_dir(path,
                                 eb->revision,
                                 eb->ra_session,
                                 eb,
                                 eb->cancel_func,
                                 eb->cancel_baton,
                                 pool));

  return SVN_NO_ERROR;
}

/* An editor function.  */
static svn_error_t *
add_directory(const char *path,
              void *parent_baton,
              const char *copyfrom_path,
              svn_revnum_t copyfrom_rev,
              apr_pool_t *pool,
              void **child_baton)
{
  struct item_baton *pb = parent_baton;
  struct item_baton *cb;

  cb = create_item_baton(pb->edit_baton, path, svn_node_dir, pool);
  ensure_summarize(cb);
  cb->summarize->summarize_kind = svn_client_diff_summarize_kind_added;

  *child_baton = cb;
  return SVN_NO_ERROR;
}

/* An editor function.  */
static svn_error_t *
open_directory(const char *path,
               void *parent_baton,
               svn_revnum_t base_revision,
               apr_pool_t *pool,
               void **child_baton)
{
  struct item_baton *pb = parent_baton;
  struct item_baton *cb;

  cb = create_item_baton(pb->edit_baton, path, svn_node_dir, pool);

  *child_baton = cb;
  return SVN_NO_ERROR;
}


/* An editor function.  */
static svn_error_t *
close_directory(void *dir_baton,
                apr_pool_t *pool)
{
  struct item_baton *ib = dir_baton;
  struct edit_baton *eb = ib->edit_baton;

  if (ib->summarize)
    SVN_ERR(eb->summarize_func(ib->summarize, eb->summarize_func_baton,
                               pool));

  return SVN_NO_ERROR;
}


/* An editor function.  */
static svn_error_t *
add_file(const char *path,
         void *parent_baton,
         const char *copyfrom_path,
         svn_revnum_t copyfrom_rev,
         apr_pool_t *pool,
         void **file_baton)
{
  struct item_baton *pb = parent_baton;
  struct item_baton *cb;

  cb = create_item_baton(pb->edit_baton, path, svn_node_file, pool);
  ensure_summarize(cb);
  cb->summarize->summarize_kind = svn_client_diff_summarize_kind_added;

  *file_baton = cb;
  return SVN_NO_ERROR;
}

/* An editor function.  */
static svn_error_t *
open_file(const char *path,
          void *parent_baton,
          svn_revnum_t base_revision,
          apr_pool_t *pool,
          void **file_baton)
{
  struct item_baton *pb = parent_baton;
  struct item_baton *cb;

  cb = create_item_baton(pb->edit_baton, path, svn_node_file, pool);

  *file_baton = cb;
  return SVN_NO_ERROR;
}

/* An editor function.  */
static svn_error_t *
apply_textdelta(void *file_baton,
                const char *base_checksum,
                apr_pool_t *pool,
                svn_txdelta_window_handler_t *handler,
                void **handler_baton)
{
  struct item_baton *ib = file_baton;

  ensure_summarize(ib);
  if (ib->summarize->summarize_kind == svn_client_diff_summarize_kind_normal)
    ib->summarize->summarize_kind = svn_client_diff_summarize_kind_modified;

  *handler = svn_delta_noop_window_handler;
  *handler_baton = NULL;

  return SVN_NO_ERROR;
}


/* An editor function.  */
static svn_error_t *
close_file(void *file_baton,
           const char *text_checksum,
           apr_pool_t *pool)
{
  struct item_baton *fb = file_baton;
  struct edit_baton *eb = fb->edit_baton;

  if (fb->summarize)
    SVN_ERR(eb->summarize_func(fb->summarize, eb->summarize_func_baton,
                               pool));

  return SVN_NO_ERROR;
}


/* An editor function, implementing both change_file_prop and
 * change_dir_prop.  */
static svn_error_t *
change_prop(void *entry_baton,
            const char *name,
            const svn_string_t *value,
            apr_pool_t *pool)
{
  struct item_baton *ib = entry_baton;

  if (svn_property_kind(NULL, name) == svn_prop_regular_kind)
    {
      ensure_summarize(ib);

      if (ib->summarize->summarize_kind != svn_client_diff_summarize_kind_added)
        ib->summarize->prop_changed = TRUE;
    }

  return SVN_NO_ERROR;
}

/* Create a repository diff summarize editor and baton.  */
svn_error_t *
svn_client__get_diff_summarize_editor(const char *target,
                                      svn_client_diff_summarize_func_t
                                      summarize_func,
                                      void *summarize_baton,
                                      svn_ra_session_t *ra_session,
                                      svn_revnum_t revision,
                                      svn_cancel_func_t cancel_func,
                                      void *cancel_baton,
                                      const svn_delta_editor_t **editor,
                                      void **edit_baton,
                                      apr_pool_t *pool)
{
  svn_delta_editor_t *tree_editor = svn_delta_default_editor(pool);
  struct edit_baton *eb = apr_palloc(pool, sizeof(*eb));

  eb->target = target;
  eb->summarize_func = summarize_func;
  eb->summarize_func_baton = summarize_baton;
  eb->ra_session = ra_session;
  eb->revision = revision;
  eb->walk_deleted_repos_dirs = TRUE;
  eb->cancel_func = cancel_func;
  eb->cancel_baton = cancel_baton;

  tree_editor->open_root = open_root;
  tree_editor->delete_entry = delete_entry;
  tree_editor->add_directory = add_directory;
  tree_editor->open_directory = open_directory;
  tree_editor->change_dir_prop = change_prop;
  tree_editor->close_directory = close_directory;

  tree_editor->add_file = add_file;
  tree_editor->open_file = open_file;
  tree_editor->apply_textdelta = apply_textdelta;
  tree_editor->change_file_prop = change_prop;
  tree_editor->close_file = close_file;

  return svn_delta_get_cancellation_editor(cancel_func, cancel_baton,
                                           tree_editor, eb, editor, edit_baton,
                                           pool);
}

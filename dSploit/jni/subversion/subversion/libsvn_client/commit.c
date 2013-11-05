/*
 * commit.c:  wrappers around wc commit functionality.
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

#include <string.h>
#include <apr_strings.h>
#include <apr_hash.h>
#include <apr_md5.h>
#include "svn_wc.h"
#include "svn_ra.h"
#include "svn_delta.h"
#include "svn_subst.h"
#include "svn_client.h"
#include "svn_string.h"
#include "svn_pools.h"
#include "svn_error.h"
#include "svn_error_codes.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_io.h"
#include "svn_time.h"
#include "svn_sorts.h"
#include "svn_props.h"

#include "client.h"
#include "private/svn_wc_private.h"
#include "private/svn_magic.h"

#include "svn_private_config.h"

/* Import context baton.

   ### TODO:  Add the following items to this baton:
      /` import editor/baton. `/
      const svn_delta_editor_t *editor;
      void *edit_baton;

      /` Client context baton `/
      svn_client_ctx_t `ctx;

      /` Paths (keys) excluded from the import (values ignored) `/
      apr_hash_t *excludes;
*/
typedef struct import_ctx_t
{
  /* Whether any changes were made to the repository */
  svn_boolean_t repos_changed;

  /* A magic cookie for mime-type detection. */
  svn_magic__cookie_t *magic_cookie;
} import_ctx_t;


/* Apply PATH's contents (as a delta against the empty string) to
   FILE_BATON in EDITOR.  Use POOL for any temporary allocation.
   PROPERTIES is the set of node properties set on this file.

   Fill DIGEST with the md5 checksum of the sent file; DIGEST must be
   at least APR_MD5_DIGESTSIZE bytes long. */

/* ### how does this compare against svn_wc_transmit_text_deltas2() ??? */

static svn_error_t *
send_file_contents(const char *path,
                   void *file_baton,
                   const svn_delta_editor_t *editor,
                   apr_hash_t *properties,
                   unsigned char *digest,
                   apr_pool_t *pool)
{
  svn_stream_t *contents;
  svn_txdelta_window_handler_t handler;
  void *handler_baton;
  const svn_string_t *eol_style_val = NULL, *keywords_val = NULL;
  svn_boolean_t special = FALSE;
  svn_subst_eol_style_t eol_style;
  const char *eol;
  apr_hash_t *keywords;

  /* If there are properties, look for EOL-style and keywords ones. */
  if (properties)
    {
      eol_style_val = apr_hash_get(properties, SVN_PROP_EOL_STYLE,
                                   sizeof(SVN_PROP_EOL_STYLE) - 1);
      keywords_val = apr_hash_get(properties, SVN_PROP_KEYWORDS,
                                  sizeof(SVN_PROP_KEYWORDS) - 1);
      if (apr_hash_get(properties, SVN_PROP_SPECIAL, APR_HASH_KEY_STRING))
        special = TRUE;
    }

  /* Get an editor func that wants to consume the delta stream. */
  SVN_ERR(editor->apply_textdelta(file_baton, NULL, pool,
                                  &handler, &handler_baton));

  if (eol_style_val)
    svn_subst_eol_style_from_value(&eol_style, &eol, eol_style_val->data);
  else
    {
      eol = NULL;
      eol_style = svn_subst_eol_style_none;
    }

  if (keywords_val)
    SVN_ERR(svn_subst_build_keywords2(&keywords, keywords_val->data,
                                      APR_STRINGIFY(SVN_INVALID_REVNUM),
                                      "", 0, "", pool));
  else
    keywords = NULL;

  if (special)
    {
      SVN_ERR(svn_subst_read_specialfile(&contents, path, pool, pool));
    }
  else
    {
      /* Open the working copy file. */
      SVN_ERR(svn_stream_open_readonly(&contents, path, pool, pool));

      /* If we have EOL styles or keywords, then detranslate the file. */
      if (svn_subst_translation_required(eol_style, eol, keywords,
                                         FALSE, TRUE))
        {
          if (eol_style == svn_subst_eol_style_unknown)
            return svn_error_createf(SVN_ERR_IO_UNKNOWN_EOL, NULL,
                                    _("%s property on '%s' contains "
                                      "unrecognized EOL-style '%s'"),
                                    SVN_PROP_EOL_STYLE, path,
                                    eol_style_val->data);

          /* We're importing, so translate files with 'native' eol-style to
           * repository-normal form, not to this platform's native EOL. */
          if (eol_style == svn_subst_eol_style_native)
            eol = SVN_SUBST_NATIVE_EOL_STR;

          /* Wrap the working copy stream with a filter to detranslate it. */
          contents = svn_subst_stream_translated(contents,
                                                 eol,
                                                 TRUE /* repair */,
                                                 keywords,
                                                 FALSE /* expand */,
                                                 pool);
        }
    }

  /* Send the file's contents to the delta-window handler. */
  return svn_error_trace(svn_txdelta_send_stream(contents, handler,
                                                 handler_baton, digest,
                                                 pool));
}


/* Import file PATH as EDIT_PATH in the repository directory indicated
 * by DIR_BATON in EDITOR.
 *
 * Accumulate file paths and their batons in FILES, which must be
 * non-null.  (These are used to send postfix textdeltas later).
 *
 * If CTX->NOTIFY_FUNC is non-null, invoke it with CTX->NOTIFY_BATON
 * for each file.
 *
 * Use POOL for any temporary allocation.
 */
static svn_error_t *
import_file(const svn_delta_editor_t *editor,
            void *dir_baton,
            const char *path,
            const char *edit_path,
            import_ctx_t *import_ctx,
            svn_client_ctx_t *ctx,
            apr_pool_t *pool)
{
  void *file_baton;
  const char *mimetype = NULL;
  unsigned char digest[APR_MD5_DIGESTSIZE];
  const char *text_checksum;
  apr_hash_t* properties;
  apr_hash_index_t *hi;
  svn_node_kind_t kind;
  svn_boolean_t is_special;

  SVN_ERR(svn_path_check_valid(path, pool));

  SVN_ERR(svn_io_check_special_path(path, &kind, &is_special, pool));

  /* Add the file, using the pool from the FILES hash. */
  SVN_ERR(editor->add_file(edit_path, dir_baton, NULL, SVN_INVALID_REVNUM,
                           pool, &file_baton));

  /* Remember that the repository was modified */
  import_ctx->repos_changed = TRUE;

  if (! is_special)
    {
      /* add automatic properties */
      SVN_ERR(svn_client__get_auto_props(&properties, &mimetype, path,
                                         import_ctx->magic_cookie,
                                         ctx, pool));
    }
  else
    properties = apr_hash_make(pool);

  if (properties)
    {
      for (hi = apr_hash_first(pool, properties); hi; hi = apr_hash_next(hi))
        {
          const char *pname = svn__apr_hash_index_key(hi);
          const svn_string_t *pval = svn__apr_hash_index_val(hi);

          SVN_ERR(editor->change_file_prop(file_baton, pname, pval, pool));
        }
    }

  if (ctx->notify_func2)
    {
      svn_wc_notify_t *notify
        = svn_wc_create_notify(path, svn_wc_notify_commit_added, pool);
      notify->kind = svn_node_file;
      notify->mime_type = mimetype;
      notify->content_state = notify->prop_state
        = svn_wc_notify_state_inapplicable;
      notify->lock_state = svn_wc_notify_lock_state_inapplicable;
      (*ctx->notify_func2)(ctx->notify_baton2, notify, pool);
    }

  /* If this is a special file, we need to set the svn:special
     property and create a temporary detranslated version in order to
     send to the server. */
  if (is_special)
    {
      apr_hash_set(properties, SVN_PROP_SPECIAL, APR_HASH_KEY_STRING,
                   svn_string_create(SVN_PROP_BOOLEAN_TRUE, pool));
      SVN_ERR(editor->change_file_prop(file_baton, SVN_PROP_SPECIAL,
                                       apr_hash_get(properties,
                                                    SVN_PROP_SPECIAL,
                                                    APR_HASH_KEY_STRING),
                                       pool));
    }

  /* Now, transmit the file contents. */
  SVN_ERR(send_file_contents(path, file_baton, editor,
                             properties, digest, pool));

  /* Finally, close the file. */
  text_checksum =
    svn_checksum_to_cstring(svn_checksum__from_digest(digest, svn_checksum_md5,
                                                      pool), pool);

  return editor->close_file(file_baton, text_checksum, pool);
}


/* Import directory PATH into the repository directory indicated by
 * DIR_BATON in EDITOR.  EDIT_PATH is the path imported as the root
 * directory, so all edits are relative to that.
 *
 * DEPTH is the depth at this point in the descent (it may be changed
 * for recursive calls).
 *
 * Accumulate file paths and their batons in FILES, which must be
 * non-null.  (These are used to send postfix textdeltas later).
 *
 * EXCLUDES is a hash whose keys are absolute paths to exclude from
 * the import (values are unused).
 *
 * If NO_IGNORE is FALSE, don't import files or directories that match
 * ignore patterns.
 *
 * If CTX->NOTIFY_FUNC is non-null, invoke it with CTX->NOTIFY_BATON for each
 * directory.
 *
 * Use POOL for any temporary allocation.  */
static svn_error_t *
import_dir(const svn_delta_editor_t *editor,
           void *dir_baton,
           const char *path,
           const char *edit_path,
           svn_depth_t depth,
           apr_hash_t *excludes,
           svn_boolean_t no_ignore,
           svn_boolean_t ignore_unknown_node_types,
           import_ctx_t *import_ctx,
           svn_client_ctx_t *ctx,
           apr_pool_t *pool)
{
  apr_pool_t *subpool = svn_pool_create(pool);  /* iteration pool */
  apr_hash_t *dirents;
  apr_hash_index_t *hi;
  apr_array_header_t *ignores;

  SVN_ERR(svn_path_check_valid(path, pool));

  if (!no_ignore)
    SVN_ERR(svn_wc_get_default_ignores(&ignores, ctx->config, pool));

  SVN_ERR(svn_io_get_dirents3(&dirents, path, TRUE, pool, pool));

  for (hi = apr_hash_first(pool, dirents); hi; hi = apr_hash_next(hi))
    {
      const char *this_path, *this_edit_path, *abs_path;
      const char *filename = svn__apr_hash_index_key(hi);
      const svn_io_dirent_t *dirent = svn__apr_hash_index_val(hi);

      svn_pool_clear(subpool);

      if (ctx->cancel_func)
        SVN_ERR(ctx->cancel_func(ctx->cancel_baton));

      if (svn_wc_is_adm_dir(filename, subpool))
        {
          /* If someone's trying to import a directory named the same
             as our administrative directories, that's probably not
             what they wanted to do.  If they are importing a file
             with that name, something is bound to blow up when they
             checkout what they've imported.  So, just skip items with
             that name.  */
          if (ctx->notify_func2)
            {
              svn_wc_notify_t *notify
                = svn_wc_create_notify(svn_dirent_join(path, filename,
                                                       subpool),
                                       svn_wc_notify_skip, subpool);
              notify->kind = svn_node_dir;
              notify->content_state = notify->prop_state
                = svn_wc_notify_state_inapplicable;
              notify->lock_state = svn_wc_notify_lock_state_inapplicable;
              (*ctx->notify_func2)(ctx->notify_baton2, notify, subpool);
            }
          continue;
        }

      /* Typically, we started importing from ".", in which case
         edit_path is "".  So below, this_path might become "./blah",
         and this_edit_path might become "blah", for example. */
      this_path = svn_dirent_join(path, filename, subpool);
      this_edit_path = svn_relpath_join(edit_path, filename, subpool);

      /* If this is an excluded path, exclude it. */
      SVN_ERR(svn_dirent_get_absolute(&abs_path, this_path, subpool));
      if (apr_hash_get(excludes, abs_path, APR_HASH_KEY_STRING))
        continue;

      if ((!no_ignore) && svn_wc_match_ignore_list(filename, ignores,
                                                   subpool))
        continue;

      if (dirent->kind == svn_node_dir && depth >= svn_depth_immediates)
        {
          void *this_dir_baton;

          /* Add the new subdirectory, getting a descent baton from
             the editor. */
          SVN_ERR(editor->add_directory(this_edit_path, dir_baton,
                                        NULL, SVN_INVALID_REVNUM, subpool,
                                        &this_dir_baton));

          /* Remember that the repository was modified */
          import_ctx->repos_changed = TRUE;

          /* By notifying before the recursive call below, we display
             a directory add before displaying adds underneath the
             directory.  To do it the other way around, just move this
             after the recursive call. */
          if (ctx->notify_func2)
            {
              svn_wc_notify_t *notify
                = svn_wc_create_notify(this_path, svn_wc_notify_commit_added,
                                       subpool);
              notify->kind = svn_node_dir;
              notify->content_state = notify->prop_state
                = svn_wc_notify_state_inapplicable;
              notify->lock_state = svn_wc_notify_lock_state_inapplicable;
              (*ctx->notify_func2)(ctx->notify_baton2, notify, subpool);
            }

          /* Recurse. */
          {
            svn_depth_t depth_below_here = depth;
            if (depth == svn_depth_immediates)
              depth_below_here = svn_depth_empty;

            SVN_ERR(import_dir(editor, this_dir_baton, this_path,
                               this_edit_path, depth_below_here, excludes,
                               no_ignore, ignore_unknown_node_types,
                               import_ctx, ctx,
                               subpool));
          }

          /* Finally, close the sub-directory. */
          SVN_ERR(editor->close_directory(this_dir_baton, subpool));
        }
      else if (dirent->kind == svn_node_file && depth >= svn_depth_files)
        {
          SVN_ERR(import_file(editor, dir_baton, this_path,
                              this_edit_path, import_ctx, ctx, subpool));
        }
      else if (dirent->kind != svn_node_dir && dirent->kind != svn_node_file)
        {
          if (ignore_unknown_node_types)
            {
              /*## warn about it*/
              if (ctx->notify_func2)
                {
                  svn_wc_notify_t *notify
                    = svn_wc_create_notify(this_path,
                                           svn_wc_notify_skip, subpool);
                  notify->kind = svn_node_dir;
                  notify->content_state = notify->prop_state
                    = svn_wc_notify_state_inapplicable;
                  notify->lock_state = svn_wc_notify_lock_state_inapplicable;
                  (*ctx->notify_func2)(ctx->notify_baton2, notify, subpool);
                }
            }
          else
            return svn_error_createf
              (SVN_ERR_NODE_UNKNOWN_KIND, NULL,
               _("Unknown or unversionable type for '%s'"),
               svn_dirent_local_style(this_path, subpool));
        }
    }

  svn_pool_destroy(subpool);
  return SVN_NO_ERROR;
}


/* Recursively import PATH to a repository using EDITOR and
 * EDIT_BATON.  PATH can be a file or directory.
 *
 * DEPTH is the depth at which to import PATH; it behaves as for
 * svn_client_import4().
 *
 * NEW_ENTRIES is an ordered array of path components that must be
 * created in the repository (where the ordering direction is
 * parent-to-child).  If PATH is a directory, NEW_ENTRIES may be empty
 * -- the result is an import which creates as many new entries in the
 * top repository target directory as there are importable entries in
 * the top of PATH; but if NEW_ENTRIES is not empty, its last item is
 * the name of a new subdirectory in the repository to hold the
 * import.  If PATH is a file, NEW_ENTRIES may not be empty, and its
 * last item is the name used for the file in the repository.  If
 * NEW_ENTRIES contains more than one item, all but the last item are
 * the names of intermediate directories that are created before the
 * real import begins.  NEW_ENTRIES may NOT be NULL.
 *
 * EXCLUDES is a hash whose keys are absolute paths to exclude from
 * the import (values are unused).
 *
 * If NO_IGNORE is FALSE, don't import files or directories that match
 * ignore patterns.
 *
 * If CTX->NOTIFY_FUNC is non-null, invoke it with CTX->NOTIFY_BATON for
 * each imported path, passing actions svn_wc_notify_commit_added.
 *
 * Use POOL for any temporary allocation.
 *
 * Note: the repository directory receiving the import was specified
 * when the editor was fetched.  (I.e, when EDITOR->open_root() is
 * called, it returns a directory baton for that directory, which is
 * not necessarily the root.)
 */
static svn_error_t *
import(const char *path,
       const apr_array_header_t *new_entries,
       const svn_delta_editor_t *editor,
       void *edit_baton,
       svn_depth_t depth,
       apr_hash_t *excludes,
       svn_boolean_t no_ignore,
       svn_boolean_t ignore_unknown_node_types,
       svn_client_ctx_t *ctx,
       apr_pool_t *pool)
{
  void *root_baton;
  svn_node_kind_t kind;
  apr_array_header_t *ignores;
  apr_array_header_t *batons = NULL;
  const char *edit_path = "";
  import_ctx_t *import_ctx = apr_pcalloc(pool, sizeof(*import_ctx));

  svn_magic__init(&import_ctx->magic_cookie, pool);

  /* Get a root dir baton.  We pass an invalid revnum to open_root
     to mean "base this on the youngest revision".  Should we have an
     SVN_YOUNGEST_REVNUM defined for these purposes? */
  SVN_ERR(editor->open_root(edit_baton, SVN_INVALID_REVNUM,
                            pool, &root_baton));

  /* Import a file or a directory tree. */
  SVN_ERR(svn_io_check_path(path, &kind, pool));

  /* Make the intermediate directory components necessary for properly
     rooting our import source tree.  */
  if (new_entries->nelts)
    {
      int i;

      batons = apr_array_make(pool, new_entries->nelts, sizeof(void *));
      for (i = 0; i < new_entries->nelts; i++)
        {
          const char *component = APR_ARRAY_IDX(new_entries, i, const char *);
          edit_path = svn_relpath_join(edit_path, component, pool);

          /* If this is the last path component, and we're importing a
             file, then this component is the name of the file, not an
             intermediate directory. */
          if ((i == new_entries->nelts - 1) && (kind == svn_node_file))
            break;

          APR_ARRAY_PUSH(batons, void *) = root_baton;
          SVN_ERR(editor->add_directory(edit_path,
                                        root_baton,
                                        NULL, SVN_INVALID_REVNUM,
                                        pool, &root_baton));

          /* Remember that the repository was modified */
          import_ctx->repos_changed = TRUE;
        }
    }
  else if (kind == svn_node_file)
    {
      return svn_error_create
        (SVN_ERR_NODE_UNKNOWN_KIND, NULL,
         _("New entry name required when importing a file"));
    }

  /* Note that there is no need to check whether PATH's basename is
     the same name that we reserve for our administrative
     subdirectories.  It would be strange -- though not illegal -- to
     import the contents of a directory of that name, because the
     directory's own name is not part of those contents.  Of course,
     if something underneath it also has our reserved name, then we'll
     error. */

  if (kind == svn_node_file)
    {
      svn_boolean_t ignores_match = FALSE;

      if (!no_ignore)
        {
          SVN_ERR(svn_wc_get_default_ignores(&ignores, ctx->config, pool));
          ignores_match = svn_wc_match_ignore_list(path, ignores, pool);
        }
      if (!ignores_match)
        SVN_ERR(import_file(editor, root_baton, path, edit_path,
                            import_ctx, ctx, pool));
    }
  else if (kind == svn_node_dir)
    {
      SVN_ERR(import_dir(editor, root_baton, path, edit_path,
                         depth, excludes, no_ignore,
                         ignore_unknown_node_types, import_ctx, ctx, pool));

    }
  else if (kind == svn_node_none
           || kind == svn_node_unknown)
    {
      return svn_error_createf(SVN_ERR_NODE_UNKNOWN_KIND, NULL,
                               _("'%s' does not exist"),
                               svn_dirent_local_style(path, pool));
    }

  /* Close up shop; it's time to go home. */
  SVN_ERR(editor->close_directory(root_baton, pool));
  if (batons && batons->nelts)
    {
      void **baton;
      while ((baton = (void **) apr_array_pop(batons)))
        {
          SVN_ERR(editor->close_directory(*baton, pool));
        }
    }

  if (import_ctx->repos_changed)
    return editor->close_edit(edit_baton, pool);
  else
    return editor->abort_edit(edit_baton, pool);
}


struct capture_baton_t {
  svn_commit_callback2_t original_callback;
  void *original_baton;

  svn_commit_info_t **info;
  apr_pool_t *pool;
};


static svn_error_t *
capture_commit_info(const svn_commit_info_t *commit_info,
                    void *baton,
                    apr_pool_t *pool)
{
  struct capture_baton_t *cb = baton;

  *(cb->info) = svn_commit_info_dup(commit_info, cb->pool);

  if (cb->original_callback)
    SVN_ERR((cb->original_callback)(commit_info, cb->original_baton, pool));

  return SVN_NO_ERROR;
}


static svn_error_t *
get_ra_editor(svn_ra_session_t **ra_session,
              const svn_delta_editor_t **editor,
              void **edit_baton,
              svn_client_ctx_t *ctx,
              const char *base_url,
              const char *base_dir_abspath,
              const char *log_msg,
              const apr_array_header_t *commit_items,
              const apr_hash_t *revprop_table,
              svn_boolean_t is_commit,
              apr_hash_t *lock_tokens,
              svn_boolean_t keep_locks,
              svn_commit_callback2_t commit_callback,
              void *commit_baton,
              apr_pool_t *pool)
{
  apr_hash_t *commit_revprops;

  /* Open an RA session to URL. */
  SVN_ERR(svn_client__open_ra_session_internal(ra_session, NULL, base_url,
                                               base_dir_abspath, commit_items,
                                               is_commit, !is_commit,
                                               ctx, pool));

  /* If this is an import (aka, not a commit), we need to verify that
     our repository URL exists. */
  if (! is_commit)
    {
      svn_node_kind_t kind;

      SVN_ERR(svn_ra_check_path(*ra_session, "", SVN_INVALID_REVNUM,
                                &kind, pool));
      if (kind == svn_node_none)
        return svn_error_createf(SVN_ERR_FS_NO_SUCH_ENTRY, NULL,
                                 _("Path '%s' does not exist"),
                                 base_url);
    }

  SVN_ERR(svn_client__ensure_revprop_table(&commit_revprops, revprop_table,
                                           log_msg, ctx, pool));

  /* Fetch RA commit editor. */
  return svn_ra_get_commit_editor3(*ra_session, editor, edit_baton,
                                   commit_revprops, commit_callback,
                                   commit_baton, lock_tokens, keep_locks,
                                   pool);
}


/*** Public Interfaces. ***/

svn_error_t *
svn_client_import4(const char *path,
                   const char *url,
                   svn_depth_t depth,
                   svn_boolean_t no_ignore,
                   svn_boolean_t ignore_unknown_node_types,
                   const apr_hash_t *revprop_table,
                   svn_commit_callback2_t commit_callback,
                   void *commit_baton,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  const char *log_msg = "";
  const svn_delta_editor_t *editor;
  void *edit_baton;
  svn_ra_session_t *ra_session;
  apr_hash_t *excludes = apr_hash_make(pool);
  svn_node_kind_t kind;
  const char *local_abspath;
  apr_array_header_t *new_entries = apr_array_make(pool, 4,
                                                   sizeof(const char *));
  const char *temp;
  const char *dir;
  apr_pool_t *subpool;

  if (svn_path_is_url(path))
    return svn_error_createf(SVN_ERR_ILLEGAL_TARGET, NULL,
                             _("'%s' is not a local path"), path);

  SVN_ERR(svn_dirent_get_absolute(&local_abspath, path, pool));

  /* Create a new commit item and add it to the array. */
  if (SVN_CLIENT__HAS_LOG_MSG_FUNC(ctx))
    {
      /* If there's a log message gatherer, create a temporary commit
         item array solely to help generate the log message.  The
         array is not used for the import itself. */
      svn_client_commit_item3_t *item;
      const char *tmp_file;
      apr_array_header_t *commit_items
        = apr_array_make(pool, 1, sizeof(item));

      item = svn_client_commit_item3_create(pool);
      item->path = apr_pstrdup(pool, path);
      item->state_flags = SVN_CLIENT_COMMIT_ITEM_ADD;
      APR_ARRAY_PUSH(commit_items, svn_client_commit_item3_t *) = item;

      SVN_ERR(svn_client__get_log_msg(&log_msg, &tmp_file, commit_items,
                                      ctx, pool));
      if (! log_msg)
        return SVN_NO_ERROR;
      if (tmp_file)
        {
          const char *abs_path;
          SVN_ERR(svn_dirent_get_absolute(&abs_path, tmp_file, pool));
          apr_hash_set(excludes, abs_path, APR_HASH_KEY_STRING, (void *)1);
        }
    }

  SVN_ERR(svn_io_check_path(local_abspath, &kind, pool));

  /* Figure out all the path components we need to create just to have
     a place to stick our imported tree. */
  subpool = svn_pool_create(pool);
  do
    {
      svn_pool_clear(subpool);

      /* See if the user is interested in cancelling this operation. */
      if (ctx->cancel_func)
        SVN_ERR(ctx->cancel_func(ctx->cancel_baton));

      if (err)
        {
          /* If get_ra_editor below failed we either tried to open
             an invalid url, or else some other kind of error.  In case
             the url was bad we back up a directory and try again. */

          if (err->apr_err != SVN_ERR_FS_NO_SUCH_ENTRY)
            return err;
          else
            svn_error_clear(err);

          svn_uri_split(&temp, &dir, url, pool);
          APR_ARRAY_PUSH(new_entries, const char *) = dir;
          url = temp;
        }
    }
  while ((err = get_ra_editor(&ra_session,
                              &editor, &edit_baton, ctx, url, NULL,
                              log_msg, NULL, revprop_table, FALSE, NULL, TRUE,
                              commit_callback, commit_baton, subpool)));

  /* Reverse the order of the components we added to our NEW_ENTRIES array. */
  if (new_entries->nelts)
    {
      int i, j;
      const char *component;
      for (i = 0; i < (new_entries->nelts / 2); i++)
        {
          j = new_entries->nelts - i - 1;
          component =
            APR_ARRAY_IDX(new_entries, i, const char *);
          APR_ARRAY_IDX(new_entries, i, const char *) =
            APR_ARRAY_IDX(new_entries, j, const char *);
          APR_ARRAY_IDX(new_entries, j, const char *) =
            component;
        }
    }

  /* An empty NEW_ENTRIES list the first call to get_ra_editor() above
     succeeded.  That means that URL corresponds to an already
     existing filesystem entity. */
  if (kind == svn_node_file && (! new_entries->nelts))
    return svn_error_createf
      (SVN_ERR_ENTRY_EXISTS, NULL,
       _("Path '%s' already exists"), url);

  /* The repository doesn't know about the reserved administrative
     directory. */
  if (new_entries->nelts
      /* What's this, what's this?  This assignment is here because we
         use the value to construct the error message just below.  It
         may not be aesthetically pleasing, but it's less ugly than
         calling APR_ARRAY_IDX twice. */
      && svn_wc_is_adm_dir(temp = APR_ARRAY_IDX(new_entries,
                                                new_entries->nelts - 1,
                                                const char *),
                           pool))
    return svn_error_createf
      (SVN_ERR_CL_ADM_DIR_RESERVED, NULL,
       _("'%s' is a reserved name and cannot be imported"),
       /* ### Is svn_path_local_style() really necessary for this? */
       svn_dirent_local_style(temp, pool));


  /* If an error occurred during the commit, abort the edit and return
     the error.  We don't even care if the abort itself fails.  */
  if ((err = import(path, new_entries, editor, edit_baton,
                    depth, excludes, no_ignore,
                    ignore_unknown_node_types, ctx, subpool)))
    {
      svn_error_clear(editor->abort_edit(edit_baton, subpool));
      return err;
    }

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


static svn_error_t *
reconcile_errors(svn_error_t *commit_err,
                 svn_error_t *unlock_err,
                 svn_error_t *bump_err,
                 apr_pool_t *pool)
{
  svn_error_t *err;

  /* Early release (for good behavior). */
  if (! (commit_err || unlock_err || bump_err))
    return SVN_NO_ERROR;

  /* If there was a commit error, start off our error chain with
     that. */
  if (commit_err)
    {
      commit_err = svn_error_quick_wrap
        (commit_err, _("Commit failed (details follow):"));
      err = commit_err;
    }

  /* Else, create a new "general" error that will lead off the errors
     that follow. */
  else
    err = svn_error_create(SVN_ERR_BASE, NULL,
                           _("Commit succeeded, but other errors follow:"));

  /* If there was an unlock error... */
  if (unlock_err)
    {
      /* Wrap the error with some headers. */
      unlock_err = svn_error_quick_wrap
        (unlock_err, _("Error unlocking locked dirs (details follow):"));

      /* Append this error to the chain. */
      svn_error_compose(err, unlock_err);
    }

  /* If there was a bumping error... */
  if (bump_err)
    {
      /* Wrap the error with some headers. */
      bump_err = svn_error_quick_wrap
        (bump_err, _("Error bumping revisions post-commit (details follow):"));

      /* Append this error to the chain. */
      svn_error_compose(err, bump_err);
    }

  return err;
}

/* For all lock tokens in ALL_TOKENS for URLs under BASE_URL, add them
   to a new hashtable allocated in POOL.  *RESULT is set to point to this
   new hash table.  *RESULT will be keyed on const char * URI-decoded paths
   relative to BASE_URL.  The lock tokens will not be duplicated. */
static svn_error_t *
collect_lock_tokens(apr_hash_t **result,
                    apr_hash_t *all_tokens,
                    const char *base_url,
                    apr_pool_t *pool)
{
  apr_hash_index_t *hi;

  *result = apr_hash_make(pool);

  for (hi = apr_hash_first(pool, all_tokens); hi; hi = apr_hash_next(hi))
    {
      const char *url = svn__apr_hash_index_key(hi);
      const char *token = svn__apr_hash_index_val(hi);

      if (svn_uri__is_ancestor(base_url, url))
        {
          url = svn_uri_skip_ancestor(base_url, url, pool);

          apr_hash_set(*result, url, APR_HASH_KEY_STRING, token);
        }
    }

  return SVN_NO_ERROR;
}

/* Put ITEM onto QUEUE, allocating it in QUEUE's pool...
 * If a checksum is provided, it can be the MD5 and/or the SHA1. */
static svn_error_t *
post_process_commit_item(svn_wc_committed_queue_t *queue,
                         const svn_client_commit_item3_t *item,
                         svn_wc_context_t *wc_ctx,
                         svn_boolean_t keep_changelists,
                         svn_boolean_t keep_locks,
                         svn_boolean_t commit_as_operations,
                         const svn_checksum_t *sha1_checksum,
                         apr_pool_t *scratch_pool)
{
  svn_boolean_t loop_recurse = FALSE;
  svn_boolean_t remove_lock;

  if (! commit_as_operations
      && (item->state_flags & SVN_CLIENT_COMMIT_ITEM_ADD)
      && (item->kind == svn_node_dir)
      && (item->copyfrom_url))
    loop_recurse = TRUE;

  remove_lock = (! keep_locks && (item->state_flags
                                       & SVN_CLIENT_COMMIT_ITEM_LOCK_TOKEN));

  return svn_wc_queue_committed3(queue, wc_ctx, item->path,
                                 loop_recurse, item->incoming_prop_changes,
                                 remove_lock, !keep_changelists,
                                 sha1_checksum, scratch_pool);
}


static svn_error_t *
check_nonrecursive_dir_delete(svn_wc_context_t *wc_ctx,
                              const char *target_abspath,
                              svn_depth_t depth,
                              apr_pool_t *scratch_pool)
{
  svn_node_kind_t kind;

  SVN_ERR_ASSERT(depth != svn_depth_infinity);

  SVN_ERR(svn_wc_read_kind(&kind, wc_ctx, target_abspath, FALSE,
                           scratch_pool));


  /* ### TODO(sd): This check is slightly too strict.  It should be
     ### possible to:
     ###
     ###   * delete a directory containing only files when
     ###     depth==svn_depth_files;
     ###
     ###   * delete a directory containing only files and empty
     ###     subdirs when depth==svn_depth_immediates.
     ###
     ### But for now, we insist on svn_depth_infinity if you're
     ### going to delete a directory, because we're lazy and
     ### trying to get depthy commits working in the first place.
     ###
     ### This would be fairly easy to fix, though: just, well,
     ### check the above conditions!
     ###
     ### GJS: I think there may be some confusion here. there is
     ###      the depth of the commit, and the depth of a checked-out
     ###      directory in the working copy. Delete, by its nature, will
     ###      always delete all of its children, so it seems a bit
     ###      strange to worry about what is in the working copy.
  */
  if (kind == svn_node_dir)
    {
      svn_wc_schedule_t schedule;

      /* ### Looking at schedule is probably enough, no need for
         pristine compare etc. */
      SVN_ERR(svn_wc__node_get_schedule(&schedule, NULL,
                                        wc_ctx, target_abspath,
                                        scratch_pool));

      if (schedule == svn_wc_schedule_delete
          || schedule == svn_wc_schedule_replace)
        {
          const apr_array_header_t *children;

          SVN_ERR(svn_wc__node_get_children(&children, wc_ctx,
                                            target_abspath, TRUE,
                                            scratch_pool, scratch_pool));

          if (children->nelts > 0)
            return svn_error_createf(SVN_ERR_UNSUPPORTED_FEATURE, NULL,
                                     _("Cannot delete the directory '%s' "
                                       "in a non-recursive commit "
                                       "because it has children"),
                                     svn_dirent_local_style(target_abspath,
                                                            scratch_pool));
        }
    }

  return SVN_NO_ERROR;
}


/* Given a list of committables described by their common base abspath
   BASE_ABSPATH and a list of relative dirents TARGET_RELPATHS determine
   which absolute paths must be locked to commit all these targets and
   return this as a const char * array in LOCK_TARGETS

   Allocate the result in RESULT_POOL and use SCRATCH_POOL for temporary
   storage */
static svn_error_t *
determine_lock_targets(apr_array_header_t **lock_targets,
                       svn_wc_context_t *wc_ctx,
                       const char *base_abspath,
                       const apr_array_header_t *target_relpaths,
                       apr_pool_t *result_pool,
                       apr_pool_t *scratch_pool)
{
  apr_pool_t *iterpool = svn_pool_create(scratch_pool);
  apr_hash_t *wc_items; /* const char *wcroot -> apr_array_header_t */
  apr_hash_index_t *hi;
  int i;

  wc_items = apr_hash_make(scratch_pool);

  /* Create an array of targets for each working copy used */
  for (i = 0; i < target_relpaths->nelts; i++)
    {
      const char *target_abspath;
      const char *wcroot_abspath;
      apr_array_header_t *wc_targets;
      svn_error_t *err;
      const char *target_relpath = APR_ARRAY_IDX(target_relpaths, i,
                                                 const char *);

      svn_pool_clear(iterpool);
      target_abspath = svn_dirent_join(base_abspath, target_relpath,
                                       scratch_pool);

      err = svn_wc__get_wc_root(&wcroot_abspath, wc_ctx, target_abspath,
                                iterpool, iterpool);

      if (err)
        {
          if (err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
            {
              svn_error_clear(err);
              continue;
            }
          return svn_error_trace(err);
        }

      wc_targets = apr_hash_get(wc_items, wcroot_abspath, APR_HASH_KEY_STRING);

      if (! wc_targets)
        {
          wc_targets = apr_array_make(scratch_pool, 4, sizeof(const char *));
          apr_hash_set(wc_items, apr_pstrdup(scratch_pool, wcroot_abspath),
                       APR_HASH_KEY_STRING, wc_targets);
        }

      APR_ARRAY_PUSH(wc_targets, const char *) = target_abspath;
    }

  *lock_targets = apr_array_make(result_pool, apr_hash_count(wc_items),
                                 sizeof(const char *));

  /* For each working copy determine where to lock */
  for (hi = apr_hash_first(scratch_pool, wc_items);
       hi;
       hi = apr_hash_next(hi))
    {
      const char *common;
      const char *wcroot_abspath = svn__apr_hash_index_key(hi);
      apr_array_header_t *wc_targets = svn__apr_hash_index_val(hi);

      svn_pool_clear(iterpool);

      if (wc_targets->nelts == 1)
        {
          const char *target_abspath;
          target_abspath = APR_ARRAY_IDX(wc_targets, 0, const char *);

          if (! strcmp(wcroot_abspath, target_abspath))
            {
              APR_ARRAY_PUSH(*lock_targets, const char *)
                      = apr_pstrdup(result_pool, target_abspath);
            }
          else
            {
              /* Lock the parent to allow deleting the target */
              APR_ARRAY_PUSH(*lock_targets, const char *)
                      = svn_dirent_dirname(target_abspath, result_pool);
            }
        }
      else if (wc_targets->nelts > 1)
        {
          SVN_ERR(svn_dirent_condense_targets(&common, &wc_targets, wc_targets,
                                              FALSE, iterpool, iterpool));

          qsort(wc_targets->elts, wc_targets->nelts, wc_targets->elt_size,
                svn_sort_compare_paths);

          if (wc_targets->nelts == 0
              || !svn_path_is_empty(APR_ARRAY_IDX(wc_targets, 0, const char*))
              || !strcmp(common, wcroot_abspath))
            {
              APR_ARRAY_PUSH(*lock_targets, const char *)
                    = apr_pstrdup(result_pool, common);
            }
          else
            {
              /* Lock the parent to allow deleting the target */
              APR_ARRAY_PUSH(*lock_targets, const char *)
                       = svn_dirent_dirname(common, result_pool);
            }
        }
    }

  svn_pool_destroy(iterpool);
  return SVN_NO_ERROR;
}

/* Baton for check_url_kind */
struct check_url_kind_baton
{
  apr_pool_t *pool;
  svn_ra_session_t *session;
  const char *repos_root_url;
  svn_client_ctx_t *ctx;
};

/* Implements svn_client__check_url_kind_t for svn_client_commit5 */
static svn_error_t *
check_url_kind(void *baton,
               svn_node_kind_t *kind,
               const char *url,
               svn_revnum_t revision,
               apr_pool_t *scratch_pool)
{
  struct check_url_kind_baton *cukb = baton;

  /* If we don't have a session or can't use the session, get one */
  if (!cukb->session || !svn_uri__is_ancestor(cukb->repos_root_url, url))
    {
      SVN_ERR(svn_client_open_ra_session(&cukb->session, url, cukb->ctx,
                                         cukb->pool));
      SVN_ERR(svn_ra_get_repos_root2(cukb->session, &cukb->repos_root_url,
                                     cukb->pool));
    }
  else
    SVN_ERR(svn_ra_reparent(cukb->session, url, scratch_pool));

  return svn_error_trace(
                svn_ra_check_path(cukb->session, "", revision,
                                  kind, scratch_pool));
}

svn_error_t *
svn_client_commit5(const apr_array_header_t *targets,
                   svn_depth_t depth,
                   svn_boolean_t keep_locks,
                   svn_boolean_t keep_changelists,
                   svn_boolean_t commit_as_operations,
                   const apr_array_header_t *changelists,
                   const apr_hash_t *revprop_table,
                   svn_commit_callback2_t commit_callback,
                   void *commit_baton,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool)
{
  const svn_delta_editor_t *editor;
  void *edit_baton;
  struct capture_baton_t cb;
  svn_ra_session_t *ra_session;
  const char *log_msg;
  const char *base_abspath;
  const char *base_url;
  const char *ra_session_wc;
  apr_array_header_t *rel_targets;
  apr_array_header_t *lock_targets;
  apr_array_header_t *locks_obtained;
  svn_client__committables_t *committables;
  apr_hash_t *lock_tokens;
  apr_hash_t *sha1_checksums;
  apr_array_header_t *commit_items;
  svn_error_t *cmt_err = SVN_NO_ERROR;
  svn_error_t *bump_err = SVN_NO_ERROR;
  svn_error_t *unlock_err = SVN_NO_ERROR;
  svn_boolean_t commit_in_progress = FALSE;
  svn_commit_info_t *commit_info = NULL;
  apr_pool_t *iterpool = svn_pool_create(pool);
  const char *current_abspath;
  const char *notify_prefix;
  int i;

  SVN_ERR_ASSERT(depth != svn_depth_unknown && depth != svn_depth_exclude);

  /* Committing URLs doesn't make sense, so error if it's tried. */
  for (i = 0; i < targets->nelts; i++)
    {
      const char *target = APR_ARRAY_IDX(targets, i, const char *);
      if (svn_path_is_url(target))
        return svn_error_createf
          (SVN_ERR_ILLEGAL_TARGET, NULL,
           _("'%s' is a URL, but URLs cannot be commit targets"), target);
    }

  /* Condense the target list. This makes all targets absolute. */
  SVN_ERR(svn_dirent_condense_targets(&base_abspath, &rel_targets, targets,
                                      FALSE, pool, iterpool));

  /* No targets means nothing to commit, so just return. */
  if (base_abspath == NULL)
    return SVN_NO_ERROR;

  SVN_ERR_ASSERT(rel_targets != NULL);

  /* If we calculated only a base and no relative targets, this
     must mean that we are being asked to commit (effectively) a
     single path. */
  if (rel_targets->nelts == 0)
    APR_ARRAY_PUSH(rel_targets, const char *) = "";

  SVN_ERR(determine_lock_targets(&lock_targets, ctx->wc_ctx, base_abspath,
                                 rel_targets, pool, iterpool));

  locks_obtained = apr_array_make(pool, lock_targets->nelts,
                                  sizeof(const char *));

  for (i = 0; i < lock_targets->nelts; i++)
    {
      const char *lock_root;
      const char *target = APR_ARRAY_IDX(lock_targets, i, const char *);

      svn_pool_clear(iterpool);

      cmt_err = svn_error_trace(
                    svn_wc__acquire_write_lock(&lock_root, ctx->wc_ctx, target,
                                           FALSE, pool, iterpool));

      if (cmt_err)
        goto cleanup;

      APR_ARRAY_PUSH(locks_obtained, const char *) = lock_root;
    }

  /* Determine prefix to strip from the commit notify messages */
  SVN_ERR(svn_dirent_get_absolute(&current_abspath, "", pool));
  notify_prefix = svn_dirent_get_longest_ancestor(current_abspath,
                                                  base_abspath,
                                                  pool);

  /* If a non-recursive commit is desired, do not allow a deleted directory
     as one of the targets. */
  if (depth != svn_depth_infinity && ! commit_as_operations)
    for (i = 0; i < rel_targets->nelts; i++)
      {
        const char *relpath = APR_ARRAY_IDX(rel_targets, i, const char *);
        const char *target_abspath;

        svn_pool_clear(iterpool);

        target_abspath = svn_dirent_join(base_abspath, relpath, iterpool);

        cmt_err = svn_error_trace(
          check_nonrecursive_dir_delete(ctx->wc_ctx, target_abspath,
                                        depth, iterpool));

        if (cmt_err)
          goto cleanup;
      }

  /* Crawl the working copy for commit items. */
  {
    struct check_url_kind_baton cukb;

    /* Prepare for when we have a copy containing not-present nodes. */
    cukb.pool = iterpool;
    cukb.session = NULL; /* ### Can we somehow reuse session? */
    cukb.repos_root_url = NULL;
    cukb.ctx = ctx;

    cmt_err = svn_error_trace(
                   svn_client__harvest_committables(&committables,
                                                    &lock_tokens,
                                                    base_abspath,
                                                    rel_targets,
                                                    depth,
                                                    ! keep_locks,
                                                    changelists,
                                                    check_url_kind,
                                                    &cukb,
                                                    ctx,
                                                    pool,
                                                    iterpool));

    svn_pool_clear(iterpool);
  }

  if (cmt_err)
    goto cleanup;

  if (apr_hash_count(committables->by_repository) == 0)
    {
      goto cleanup; /* Nothing to do */
    }
  else if (apr_hash_count(committables->by_repository) > 1)
    {
      cmt_err = svn_error_create(
             SVN_ERR_UNSUPPORTED_FEATURE, NULL,
             _("Commit can only commit to a single repository at a time.\n"
               "Are all targets part of the same working copy?"));
      goto cleanup;
    }

  {
    apr_hash_index_t *hi = apr_hash_first(iterpool,
                                          committables->by_repository);

    commit_items = svn__apr_hash_index_val(hi);
  }

  /* If our array of targets contains only locks (and no actual file
     or prop modifications), then we return here to avoid committing a
     revision with no changes. */
  {
    svn_boolean_t found_changed_path = FALSE;

    for (i = 0; i < commit_items->nelts; ++i)
      {
        svn_client_commit_item3_t *item =
          APR_ARRAY_IDX(commit_items, i, svn_client_commit_item3_t *);

        if (item->state_flags != SVN_CLIENT_COMMIT_ITEM_LOCK_TOKEN)
          {
            found_changed_path = TRUE;
            break;
          }
      }

    if (!found_changed_path)
      goto cleanup;
  }

  /* Go get a log message.  If an error occurs, or no log message is
     specified, abort the operation. */
  if (SVN_CLIENT__HAS_LOG_MSG_FUNC(ctx))
    {
      const char *tmp_file;
      cmt_err = svn_error_trace(
                     svn_client__get_log_msg(&log_msg, &tmp_file, commit_items,
                                             ctx, pool));

      if (cmt_err || (! log_msg))
        goto cleanup;
    }
  else
    log_msg = "";

  /* Sort and condense our COMMIT_ITEMS. */
  cmt_err = svn_error_trace(svn_client__condense_commit_items(&base_url,
                                                              commit_items,
                                                              pool));

  if (cmt_err)
    goto cleanup;

  /* Collect our lock tokens with paths relative to base_url. */
  cmt_err = svn_error_trace(collect_lock_tokens(&lock_tokens, lock_tokens,
                                                base_url, pool));

  if (cmt_err)
    goto cleanup;

  cb.original_callback = commit_callback;
  cb.original_baton = commit_baton;
  cb.info = &commit_info;
  cb.pool = pool;

  /* When committing from multiple WCs, get the RA editor from
   * the first WC, rather than the BASE_ABSPATH. The BASE_ABSPATH
   * might be an unrelated parent of nested working copies.
   * We don't support commits to multiple repositories so using
   * the first WC to get the RA session is safe. */
  if (lock_targets->nelts > 1)
    ra_session_wc = APR_ARRAY_IDX(lock_targets, 0, const char *);
  else
    ra_session_wc = base_abspath;

  cmt_err = svn_error_trace(
                 get_ra_editor(&ra_session, &editor, &edit_baton, ctx,
                               base_url, ra_session_wc, log_msg,
                               commit_items, revprop_table, TRUE, lock_tokens,
                               keep_locks, capture_commit_info,
                               &cb, pool));

  if (cmt_err)
    goto cleanup;

  /* Make a note that we have a commit-in-progress. */
  commit_in_progress = TRUE;

  /* Perform the commit. */
  cmt_err = svn_error_trace(
            svn_client__do_commit(base_url, commit_items, editor, edit_baton,
                                  notify_prefix, NULL,
                                  &sha1_checksums, ctx, pool, iterpool));

  /* Handle a successful commit. */
  if ((! cmt_err)
      || (cmt_err->apr_err == SVN_ERR_REPOS_POST_COMMIT_HOOK_FAILED))
    {
      svn_wc_committed_queue_t *queue = svn_wc_committed_queue_create(pool);

      /* Make a note that our commit is finished. */
      commit_in_progress = FALSE;

      for (i = 0; i < commit_items->nelts; i++)
        {
          svn_client_commit_item3_t *item
            = APR_ARRAY_IDX(commit_items, i, svn_client_commit_item3_t *);

          svn_pool_clear(iterpool);
          bump_err = post_process_commit_item(
                       queue, item, ctx->wc_ctx,
                       keep_changelists, keep_locks, commit_as_operations,
                       apr_hash_get(sha1_checksums,
                                    item->path,
                                    APR_HASH_KEY_STRING),
                       iterpool);
          if (bump_err)
            goto cleanup;
        }

      SVN_ERR_ASSERT(commit_info);
      bump_err = svn_wc_process_committed_queue2(
                   queue, ctx->wc_ctx,
                   commit_info->revision,
                   commit_info->date,
                   commit_info->author,
                   ctx->cancel_func, ctx->cancel_baton,
                   iterpool);
    }

  /* Sleep to ensure timestamp integrity. */
  svn_io_sleep_for_timestamps(base_abspath, pool);

 cleanup:
  /* Abort the commit if it is still in progress. */
  svn_pool_clear(iterpool); /* Close open handles before aborting */
  if (commit_in_progress)
    cmt_err = svn_error_compose_create(cmt_err,
                                       editor->abort_edit(edit_baton, pool));

  /* A bump error is likely to occur while running a working copy log file,
     explicitly unlocking and removing temporary files would be wrong in
     that case.  A commit error (cmt_err) should only occur before any
     attempt to modify the working copy, so it doesn't prevent explicit
     clean-up. */
  if (! bump_err)
    {
      /* Release all locks we obtained */
      for (i = 0; i < locks_obtained->nelts; i++)
        {
          const char *lock_root = APR_ARRAY_IDX(locks_obtained, i,
                                                const char *);

          svn_pool_clear(iterpool);

          unlock_err = svn_error_compose_create(
                           svn_wc__release_write_lock(ctx->wc_ctx, lock_root,
                                                      iterpool),
                           unlock_err);
        }
    }

  svn_pool_destroy(iterpool);

  return svn_error_trace(reconcile_errors(cmt_err, unlock_err, bump_err,
                                          pool));
}

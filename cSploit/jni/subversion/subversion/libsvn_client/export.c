/*
 * export.c:  export a tree.
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

#include <apr_file_io.h>
#include <apr_md5.h>
#include "svn_types.h"
#include "svn_client.h"
#include "svn_string.h"
#include "svn_error.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_pools.h"
#include "svn_subst.h"
#include "svn_time.h"
#include "svn_props.h"
#include "client.h"

#include "svn_private_config.h"
#include "private/svn_wc_private.h"


/*** Code. ***/

/* Add EXTERNALS_PROP_VAL for the export destination path PATH to
   TRAVERSAL_INFO.  */
static svn_error_t *
add_externals(apr_hash_t *externals,
              const char *path,
              const svn_string_t *externals_prop_val)
{
  apr_pool_t *pool = apr_hash_pool_get(externals);
  const char *local_abspath;

  if (! externals_prop_val)
    return SVN_NO_ERROR;

  SVN_ERR(svn_dirent_get_absolute(&local_abspath, path, pool));

  apr_hash_set(externals, local_abspath, APR_HASH_KEY_STRING,
               apr_pstrmemdup(pool, externals_prop_val->data,
                              externals_prop_val->len));

  return SVN_NO_ERROR;
}

/* Helper function that gets the eol style and optionally overrides the
   EOL marker for files marked as native with the EOL marker matching
   the string specified in requested_value which is of the same format
   as the svn:eol-style property values. */
static svn_error_t *
get_eol_style(svn_subst_eol_style_t *style,
              const char **eol,
              const char *value,
              const char *requested_value)
{
  svn_subst_eol_style_from_value(style, eol, value);
  if (requested_value && *style == svn_subst_eol_style_native)
    {
      svn_subst_eol_style_t requested_style;
      const char *requested_eol;

      svn_subst_eol_style_from_value(&requested_style, &requested_eol,
                                     requested_value);

      if (requested_style == svn_subst_eol_style_fixed)
        *eol = requested_eol;
      else
        return svn_error_createf(SVN_ERR_IO_UNKNOWN_EOL, NULL,
                                 _("'%s' is not a valid EOL value"),
                                 requested_value);
    }
  return SVN_NO_ERROR;
}

/* If *APPENDABLE_DIRENT_P represents an existing directory, then append
 * to it the basename of BASENAME_OF and return the result in
 * *APPENDABLE_DIRENT_P.  The kind of BASENAME_OF is either dirent or uri,
 * as given by IS_URI.
 */
static svn_error_t *
append_basename_if_dir(const char **appendable_dirent_p,
                       const char *basename_of,
                       svn_boolean_t is_uri,
                       apr_pool_t *pool)
{
  svn_node_kind_t local_kind;
  SVN_ERR(svn_io_check_resolved_path(*appendable_dirent_p, &local_kind, pool));
  if (local_kind == svn_node_dir)
    {
      const char *base_name;

      if (is_uri)
        base_name = svn_uri_basename(basename_of, pool);
      else
        base_name = svn_dirent_basename(basename_of, NULL);

      *appendable_dirent_p = svn_dirent_join(*appendable_dirent_p,
                                             base_name, pool);
    }

  return SVN_NO_ERROR;
}

/* Make an unversioned copy of the versioned file at FROM_ABSPATH.  Copy it
 * to the destination path TO_ABSPATH.
 *
 * If REVISION is svn_opt_revision_working, copy the working version,
 * otherwise copy the base version.
 *
 * Expand the file's keywords according to the source file's 'svn:keywords'
 * property, if present.  If copying a locally modified working version,
 * append 'M' to the revision number and use '(local)' for the author.
 *
 * Translate the file's line endings according to the source file's
 * 'svn:eol-style' property, if present.  If NATIVE_EOL is not NULL, use it
 * in place of the native EOL style.  Throw an error if the source file has
 * inconsistent line endings and EOL translation is attempted.
 *
 * Set the destination file's modification time to the source file's
 * modification time if copying the working version and the working version
 * is locally modified; otherwise set it to the versioned file's last
 * changed time.
 *
 * Set the destination file's 'executable' flag according to the source
 * file's 'svn:executable' property.
 */
static svn_error_t *
copy_one_versioned_file(const char *from_abspath,
                        const char *to_abspath,
                        svn_client_ctx_t *ctx,
                        const svn_opt_revision_t *revision,
                        const char *native_eol,
                        svn_boolean_t ignore_keywords,
                        apr_pool_t *scratch_pool)
{
  apr_hash_t *kw = NULL;
  svn_subst_eol_style_t style;
  apr_hash_t *props;
  svn_string_t *eol_style, *keywords, *executable, *special;
  const char *eol = NULL;
  svn_boolean_t local_mod = FALSE;
  apr_time_t tm;
  svn_stream_t *source;
  svn_stream_t *dst_stream;
  const char *dst_tmp;
  svn_error_t *err;
  svn_boolean_t is_deleted;
  svn_wc_context_t *wc_ctx = ctx->wc_ctx;

  SVN_ERR(svn_wc__node_is_status_deleted(&is_deleted, wc_ctx, from_abspath,
                                         scratch_pool));

  /* Don't export 'deleted' files and directories unless it's a
     revision other than WORKING.  These files and directories
     don't really exist in WORKING. */
  if (revision->kind == svn_opt_revision_working && is_deleted)
    return SVN_NO_ERROR;

  if (revision->kind != svn_opt_revision_working)
    {
      /* Only export 'added' files when the revision is WORKING. This is not
         WORKING, so skip the 'added' files, since they didn't exist
         in the BASE revision and don't have an associated text-base.

         'replaced' files are technically the same as 'added' files.
         ### TODO: Handle replaced nodes properly.
         ###       svn_opt_revision_base refers to the "new"
         ###       base of the node. That means, if a node is locally
         ###       replaced, export skips this node, as if it was locally
         ###       added, because svn_opt_revision_base refers to the base
         ###       of the added node, not to the node that was deleted.
         ###       In contrast, when the node is copied-here or moved-here,
         ###       the copy/move source's content will be exported.
         ###       It is currently not possible to export the revert-base
         ###       when a node is locally replaced. We need a new
         ###       svn_opt_revision_ enum value for proper distinction
         ###       between revert-base and commit-base.

         Copied-/moved-here nodes have a base, so export both added and
         replaced files when they involve a copy-/move-here.

         We get all this for free from evaluating SOURCE == NULL:
       */
      SVN_ERR(svn_wc_get_pristine_contents2(&source, wc_ctx, from_abspath,
                                            scratch_pool, scratch_pool));
      if (source == NULL)
        return SVN_NO_ERROR;

      SVN_ERR(svn_wc_get_pristine_props(&props, wc_ctx, from_abspath,
                                        scratch_pool, scratch_pool));
    }
  else
    {
      svn_wc_status3_t *status;

      /* ### hmm. this isn't always a specialfile. this will simply open
         ### the file readonly if it is a regular file. */
      SVN_ERR(svn_subst_read_specialfile(&source, from_abspath, scratch_pool,
                                         scratch_pool));

      SVN_ERR(svn_wc_prop_list2(&props, wc_ctx, from_abspath, scratch_pool,
                                scratch_pool));
      SVN_ERR(svn_wc_status3(&status, wc_ctx, from_abspath, scratch_pool,
                             scratch_pool));
      if (status->text_status != svn_wc_status_normal)
        local_mod = TRUE;
    }

  /* We can early-exit if we're creating a special file. */
  special = apr_hash_get(props, SVN_PROP_SPECIAL,
                         APR_HASH_KEY_STRING);
  if (special != NULL)
    {
      /* Create the destination as a special file, and copy the source
         details into the destination stream. */
      SVN_ERR(svn_subst_create_specialfile(&dst_stream, to_abspath,
                                           scratch_pool, scratch_pool));
      return svn_error_trace(
        svn_stream_copy3(source, dst_stream, NULL, NULL, scratch_pool));
    }


  eol_style = apr_hash_get(props, SVN_PROP_EOL_STYLE,
                           APR_HASH_KEY_STRING);
  keywords = apr_hash_get(props, SVN_PROP_KEYWORDS,
                          APR_HASH_KEY_STRING);
  executable = apr_hash_get(props, SVN_PROP_EXECUTABLE,
                            APR_HASH_KEY_STRING);

  if (eol_style)
    SVN_ERR(get_eol_style(&style, &eol, eol_style->data, native_eol));

  if (local_mod)
    {
      /* Use the modified time from the working copy of
         the file */
      SVN_ERR(svn_io_file_affected_time(&tm, from_abspath, scratch_pool));
    }
  else
    {
      SVN_ERR(svn_wc__node_get_changed_info(NULL, &tm, NULL, wc_ctx,
                                            from_abspath, scratch_pool,
                                            scratch_pool));
    }

  if (keywords)
    {
      svn_revnum_t changed_rev;
      const char *suffix;
      const char *url;
      const char *author;

      SVN_ERR(svn_wc__node_get_changed_info(&changed_rev, NULL, &author,
                                            wc_ctx, from_abspath, scratch_pool,
                                            scratch_pool));

      if (local_mod)
        {
          /* For locally modified files, we'll append an 'M'
             to the revision number, and set the author to
             "(local)" since we can't always determine the
             current user's username */
          suffix = "M";
          author = _("(local)");
        }
      else
        {
          suffix = "";
        }

      SVN_ERR(svn_wc__node_get_url(&url, wc_ctx, from_abspath,
                                   scratch_pool, scratch_pool));

      SVN_ERR(svn_subst_build_keywords2
              (&kw, keywords->data,
               apr_psprintf(scratch_pool, "%ld%s", changed_rev, suffix),
               url, tm, author, scratch_pool));
    }

  /* For atomicity, we translate to a tmp file and then rename the tmp file
     over the real destination. */
  SVN_ERR(svn_stream_open_unique(&dst_stream, &dst_tmp,
                                 svn_dirent_dirname(to_abspath, scratch_pool),
                                 svn_io_file_del_none, scratch_pool,
                                 scratch_pool));

  /* If some translation is needed, then wrap the output stream (this is
     more efficient than wrapping the input). */
  if (eol || (kw && (apr_hash_count(kw) > 0)))
    dst_stream = svn_subst_stream_translated(dst_stream,
                                             eol,
                                             FALSE /* repair */,
                                             kw,
                                             ! ignore_keywords /* expand */,
                                             scratch_pool);

  /* ###: use cancel func/baton in place of NULL/NULL below. */
  err = svn_stream_copy3(source, dst_stream, NULL, NULL, scratch_pool);

  if (!err && executable)
    err = svn_io_set_file_executable(dst_tmp, TRUE, FALSE, scratch_pool);

  if (!err)
    err = svn_io_set_file_affected_time(tm, dst_tmp, scratch_pool);

  if (err)
    return svn_error_compose_create(err, svn_io_remove_file2(dst_tmp, FALSE,
                                                             scratch_pool));

  /* Now that dst_tmp contains the translated data, do the atomic rename. */
  SVN_ERR(svn_io_file_rename(dst_tmp, to_abspath, scratch_pool));

  if (ctx->notify_func2)
    {
      svn_wc_notify_t *notify = svn_wc_create_notify(to_abspath,
                                      svn_wc_notify_update_add, scratch_pool);
      notify->kind = svn_node_file;
      (*ctx->notify_func2)(ctx->notify_baton2, notify, scratch_pool);
    }

  return SVN_NO_ERROR;
}

/* Make an unversioned copy of the versioned file or directory tree at the
 * source path FROM_ABSPATH.  Copy it to the destination path TO_ABSPATH.
 *
 * If REVISION is svn_opt_revision_working, copy the working version,
 * otherwise copy the base version.
 *
 * See copy_one_versioned_file() for details of file copying behaviour,
 * including IGNORE_KEYWORDS and NATIVE_EOL.
 *
 * Include externals unless IGNORE_EXTERNALS is true.
 *
 * Recurse according to DEPTH.
 *

 */
static svn_error_t *
copy_versioned_files(const char *from_abspath,
                     const char *to_abspath,
                     const svn_opt_revision_t *revision,
                     svn_boolean_t force,
                     svn_boolean_t ignore_externals,
                     svn_boolean_t ignore_keywords,
                     svn_depth_t depth,
                     const char *native_eol,
                     svn_client_ctx_t *ctx,
                     apr_pool_t *pool)
{
  svn_error_t *err;
  apr_pool_t *iterpool;
  const apr_array_header_t *children;
  svn_node_kind_t from_kind;
  svn_depth_t node_depth;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(from_abspath));
  SVN_ERR_ASSERT(svn_dirent_is_absolute(to_abspath));

  /* Only export 'added' and 'replaced' files when the revision is WORKING;
     when the revision is BASE (i.e. != WORKING), only export 'added' and
     'replaced' files when they are part of a copy-/move-here. Otherwise, skip
     them, since they don't have an associated text-base. This condition for
     added/replaced simply is an optimization. Added and replaced files would
     be handled similarly by svn_wc_get_pristine_contents2(), which would
     return NULL if they have no base associated.
     TODO: We may prefer not to duplicate this condition and rather use
     svn_wc_get_pristine_contents2() or a dedicated new function instead.

     Don't export 'deleted' files and directories unless it's a
     revision other than WORKING.  These files and directories
     don't really exist in WORKING. */
  if (revision->kind != svn_opt_revision_working)
    {
      svn_boolean_t is_added;
      const char *repos_relpath;

      SVN_ERR(svn_wc__node_get_origin(&is_added, NULL, &repos_relpath,
                                      NULL, NULL, NULL,
                                      ctx->wc_ctx, from_abspath, FALSE,
                                      pool, pool));

      if (is_added && !repos_relpath)
        return SVN_NO_ERROR; /* Local addition */
    }
  else
    {
      svn_boolean_t is_deleted;

      SVN_ERR(svn_wc__node_is_status_deleted(&is_deleted, ctx->wc_ctx,
                                             from_abspath, pool));
      if (is_deleted)
        return SVN_NO_ERROR;
    }

  SVN_ERR(svn_wc_read_kind(&from_kind, ctx->wc_ctx, from_abspath, FALSE,
                           pool));

  if (from_kind == svn_node_dir)
    {
      apr_fileperms_t perm = APR_OS_DEFAULT;
      int j;

      /* Try to make the new directory.  If this fails because the
         directory already exists, check our FORCE flag to see if we
         care. */

      /* Keep the source directory's permissions if applicable.
         Skip retrieving the umask on windows. Apr does not implement setting
         filesystem privileges on Windows.
         Retrieving the file permissions with APR_FINFO_PROT | APR_FINFO_OWNER
         is documented to be 'incredibly expensive' */
#ifndef WIN32
      if (revision->kind == svn_opt_revision_working)
        {
          apr_finfo_t finfo;
          SVN_ERR(svn_io_stat(&finfo, from_abspath, APR_FINFO_PROT, pool));
          perm = finfo.protection;
        }
#endif
      err = svn_io_dir_make(to_abspath, perm, pool);
      if (err)
        {
          if (! APR_STATUS_IS_EEXIST(err->apr_err))
            return svn_error_trace(err);
          if (! force)
            SVN_ERR_W(err, _("Destination directory exists, and will not be "
                             "overwritten unless forced"));
          else
            svn_error_clear(err);
        }

      SVN_ERR(svn_wc__node_get_children(&children, ctx->wc_ctx, from_abspath,
                                        FALSE, pool, pool));

      iterpool = svn_pool_create(pool);
      for (j = 0; j < children->nelts; j++)
        {
          const char *child_abspath = APR_ARRAY_IDX(children, j, const char *);
          const char *child_name = svn_dirent_basename(child_abspath, NULL);
          const char *target_abspath;
          svn_node_kind_t child_kind;

          svn_pool_clear(iterpool);

          if (ctx->cancel_func)
            SVN_ERR(ctx->cancel_func(ctx->cancel_baton));

          target_abspath = svn_dirent_join(to_abspath, child_name, iterpool);

          SVN_ERR(svn_wc_read_kind(&child_kind, ctx->wc_ctx, child_abspath,
                                   FALSE, iterpool));

          if (child_kind == svn_node_dir)
            {
              if (depth == svn_depth_infinity
                  || depth == svn_depth_immediates)
                {
                  if (ctx->notify_func2)
                    {
                      svn_wc_notify_t *notify =
                          svn_wc_create_notify(target_abspath,
                                               svn_wc_notify_update_add, pool);
                      notify->kind = svn_node_dir;
                      (*ctx->notify_func2)(ctx->notify_baton2, notify, pool);
                    }

                  if (depth == svn_depth_infinity)
                    SVN_ERR(copy_versioned_files(child_abspath, target_abspath,
                                                 revision, force,
                                                 ignore_externals,
                                                 ignore_keywords, depth,
                                                 native_eol, ctx, iterpool));
                  else
                    SVN_ERR(svn_io_make_dir_recursively(target_abspath,
                                                        iterpool));
                }
            }
          else if (child_kind == svn_node_file
                   && depth >= svn_depth_files)
            {
              svn_node_kind_t external_kind;

              SVN_ERR(svn_wc__read_external_info(&external_kind,
                                                 NULL, NULL, NULL,
                                                 NULL, ctx->wc_ctx,
                                                 child_abspath,
                                                 child_abspath, TRUE,
                                                 pool, pool));

              if (external_kind != svn_node_file)
                SVN_ERR(copy_one_versioned_file(child_abspath, target_abspath,
                                                ctx, revision,
                                                native_eol, ignore_keywords,
                                                iterpool));
            }
        }

      SVN_ERR(svn_wc__node_get_depth(&node_depth, ctx->wc_ctx,
                                     from_abspath, pool));

      /* Handle externals. */
      if (! ignore_externals && depth == svn_depth_infinity
          && node_depth == svn_depth_infinity)
        {
          apr_array_header_t *ext_items;
          const svn_string_t *prop_val;

          SVN_ERR(svn_wc_prop_get2(&prop_val, ctx->wc_ctx, from_abspath,
                                   SVN_PROP_EXTERNALS, pool, pool));
          if (prop_val != NULL)
            {
              int i;

              SVN_ERR(svn_wc_parse_externals_description3(&ext_items,
                                                          from_abspath,
                                                          prop_val->data,
                                                          FALSE, pool));
              for (i = 0; i < ext_items->nelts; ++i)
                {
                  svn_wc_external_item2_t *ext_item;
                  const char *new_from, *new_to;

                  svn_pool_clear(iterpool);

                  ext_item = APR_ARRAY_IDX(ext_items, i,
                                           svn_wc_external_item2_t *);
                  new_from = svn_dirent_join(from_abspath,
                                             ext_item->target_dir,
                                             iterpool);
                  new_to = svn_dirent_join(to_abspath, ext_item->target_dir,
                                           iterpool);

                   /* The target dir might have parents that don't exist.
                      Guarantee the path upto the last component. */
                  if (!svn_dirent_is_root(ext_item->target_dir,
                                          strlen(ext_item->target_dir)))
                    {
                      const char *parent = svn_dirent_dirname(new_to, iterpool);
                      SVN_ERR(svn_io_make_dir_recursively(parent, iterpool));
                    }

                  SVN_ERR(copy_versioned_files(new_from, new_to,
                                               revision, force, FALSE,
                                               ignore_keywords,
                                               svn_depth_infinity, native_eol,
                                               ctx, iterpool));
                }
            }
        }

      svn_pool_destroy(iterpool);
    }
  else if (from_kind == svn_node_file)
    {
      svn_node_kind_t to_kind;

      SVN_ERR(svn_io_check_path(to_abspath, &to_kind, pool));

      if ((to_kind == svn_node_file || to_kind == svn_node_unknown) && ! force)
        return svn_error_createf(SVN_ERR_ILLEGAL_TARGET, NULL,
                                 _("Destination file '%s' exists, and "
                                   "will not be overwritten unless forced"),
                                 svn_dirent_local_style(to_abspath, pool));
      else if (to_kind == svn_node_dir)
        return svn_error_createf(SVN_ERR_ILLEGAL_TARGET, NULL,
                                 _("Destination '%s' exists. Cannot "
                                   "overwrite directory with non-directory"),
                                 svn_dirent_local_style(to_abspath, pool));

      SVN_ERR(copy_one_versioned_file(from_abspath, to_abspath, ctx,
                                      revision, native_eol, ignore_keywords,
                                      pool));
    }

  return SVN_NO_ERROR;
}


/* Abstraction of open_root.
 *
 * Create PATH if it does not exist and is not obstructed, and invoke
 * NOTIFY_FUNC with NOTIFY_BATON on PATH.
 *
 * If PATH exists but is a file, then error with SVN_ERR_WC_NOT_WORKING_COPY.
 *
 * If PATH is a already a directory, then error with
 * SVN_ERR_WC_OBSTRUCTED_UPDATE, unless FORCE, in which case just
 * export into PATH with no error.
 */
static svn_error_t *
open_root_internal(const char *path,
                   svn_boolean_t force,
                   svn_wc_notify_func2_t notify_func,
                   void *notify_baton,
                   apr_pool_t *pool)
{
  svn_node_kind_t kind;

  SVN_ERR(svn_io_check_path(path, &kind, pool));
  if (kind == svn_node_none)
    SVN_ERR(svn_io_make_dir_recursively(path, pool));
  else if (kind == svn_node_file)
    return svn_error_createf(SVN_ERR_WC_NOT_WORKING_COPY, NULL,
                             _("'%s' exists and is not a directory"),
                             svn_dirent_local_style(path, pool));
  else if ((kind != svn_node_dir) || (! force))
    return svn_error_createf(SVN_ERR_WC_OBSTRUCTED_UPDATE, NULL,
                             _("'%s' already exists"),
                             svn_dirent_local_style(path, pool));

  if (notify_func)
    {
      svn_wc_notify_t *notify = svn_wc_create_notify(path,
                                                     svn_wc_notify_update_add,
                                                     pool);
      notify->kind = svn_node_dir;
      (*notify_func)(notify_baton, notify, pool);
    }

  return SVN_NO_ERROR;
}


/* ---------------------------------------------------------------------- */


/*** A dedicated 'export' editor, which does no .svn/ accounting.  ***/


struct edit_baton
{
  const char *root_path;
  const char *root_url;
  svn_boolean_t force;
  svn_revnum_t *target_revision;
  apr_hash_t *externals;
  const char *native_eol;
  svn_boolean_t ignore_keywords;

  svn_cancel_func_t cancel_func;
  void *cancel_baton;
  svn_wc_notify_func2_t notify_func;
  void *notify_baton;
};


struct dir_baton
{
  struct edit_baton *edit_baton;
  const char *path;
};


struct file_baton
{
  struct edit_baton *edit_baton;

  const char *path;
  const char *tmppath;

  /* We need to keep this around so we can explicitly close it in close_file,
     thus flushing its output to disk so we can copy and translate it. */
  svn_stream_t *tmp_stream;

  /* The MD5 digest of the file's fulltext.  This is all zeros until
     the last textdelta window handler call returns. */
  unsigned char text_digest[APR_MD5_DIGESTSIZE];

  /* The three svn: properties we might actually care about. */
  const svn_string_t *eol_style_val;
  const svn_string_t *keywords_val;
  const svn_string_t *executable_val;
  svn_boolean_t special;

  /* Any keyword vals to be substituted */
  const char *revision;
  const char *url;
  const char *author;
  apr_time_t date;

  /* Pool associated with this baton. */
  apr_pool_t *pool;
};


struct handler_baton
{
  svn_txdelta_window_handler_t apply_handler;
  void *apply_baton;
  apr_pool_t *pool;
  const char *tmppath;
};


static svn_error_t *
set_target_revision(void *edit_baton,
                    svn_revnum_t target_revision,
                    apr_pool_t *pool)
{
  struct edit_baton *eb = edit_baton;

  /* Stashing a target_revision in the baton */
  *(eb->target_revision) = target_revision;
  return SVN_NO_ERROR;
}



/* Just ensure that the main export directory exists. */
static svn_error_t *
open_root(void *edit_baton,
          svn_revnum_t base_revision,
          apr_pool_t *pool,
          void **root_baton)
{
  struct edit_baton *eb = edit_baton;
  struct dir_baton *db = apr_pcalloc(pool, sizeof(*db));

  SVN_ERR(open_root_internal(eb->root_path, eb->force,
                             eb->notify_func, eb->notify_baton, pool));

  /* Build our dir baton. */
  db->path = eb->root_path;
  db->edit_baton = eb;
  *root_baton = db;

  return SVN_NO_ERROR;
}


/* Ensure the directory exists, and send feedback. */
static svn_error_t *
add_directory(const char *path,
              void *parent_baton,
              const char *copyfrom_path,
              svn_revnum_t copyfrom_revision,
              apr_pool_t *pool,
              void **baton)
{
  struct dir_baton *pb = parent_baton;
  struct dir_baton *db = apr_pcalloc(pool, sizeof(*db));
  struct edit_baton *eb = pb->edit_baton;
  const char *full_path = svn_dirent_join(eb->root_path, path, pool);
  svn_node_kind_t kind;

  SVN_ERR(svn_io_check_path(full_path, &kind, pool));
  if (kind == svn_node_none)
    SVN_ERR(svn_io_dir_make(full_path, APR_OS_DEFAULT, pool));
  else if (kind == svn_node_file)
    return svn_error_createf(SVN_ERR_WC_NOT_WORKING_COPY, NULL,
                             _("'%s' exists and is not a directory"),
                             svn_dirent_local_style(full_path, pool));
  else if (! (kind == svn_node_dir && eb->force))
    return svn_error_createf(SVN_ERR_WC_OBSTRUCTED_UPDATE, NULL,
                             _("'%s' already exists"),
                             svn_dirent_local_style(full_path, pool));

  if (eb->notify_func)
    {
      svn_wc_notify_t *notify = svn_wc_create_notify(full_path,
                                                     svn_wc_notify_update_add,
                                                     pool);
      notify->kind = svn_node_dir;
      (*eb->notify_func)(eb->notify_baton, notify, pool);
    }

  /* Build our dir baton. */
  db->path = full_path;
  db->edit_baton = eb;
  *baton = db;

  return SVN_NO_ERROR;
}


/* Build a file baton. */
static svn_error_t *
add_file(const char *path,
         void *parent_baton,
         const char *copyfrom_path,
         svn_revnum_t copyfrom_revision,
         apr_pool_t *pool,
         void **baton)
{
  struct dir_baton *pb = parent_baton;
  struct edit_baton *eb = pb->edit_baton;
  struct file_baton *fb = apr_pcalloc(pool, sizeof(*fb));
  const char *full_path = svn_dirent_join(eb->root_path, path, pool);

  /* PATH is not canonicalized, i.e. it may still contain spaces etc.
   * but EB->root_url is. */
  const char *full_url = svn_path_url_add_component2(eb->root_url,
                                                     path,
                                                     pool);

  fb->edit_baton = eb;
  fb->path = full_path;
  fb->url = full_url;
  fb->pool = pool;

  *baton = fb;
  return SVN_NO_ERROR;
}


static svn_error_t *
window_handler(svn_txdelta_window_t *window, void *baton)
{
  struct handler_baton *hb = baton;
  svn_error_t *err;

  err = hb->apply_handler(window, hb->apply_baton);
  if (err)
    {
      /* We failed to apply the patch; clean up the temporary file.  */
      svn_error_clear(svn_io_remove_file2(hb->tmppath, TRUE, hb->pool));
    }

  return svn_error_trace(err);
}



/* Write incoming data into the tmpfile stream */
static svn_error_t *
apply_textdelta(void *file_baton,
                const char *base_checksum,
                apr_pool_t *pool,
                svn_txdelta_window_handler_t *handler,
                void **handler_baton)
{
  struct file_baton *fb = file_baton;
  struct handler_baton *hb = apr_palloc(pool, sizeof(*hb));

  /* Create a temporary file in the same directory as the file. We're going
     to rename the thing into place when we're done. */
  SVN_ERR(svn_stream_open_unique(&fb->tmp_stream, &fb->tmppath,
                                 svn_dirent_dirname(fb->path, pool),
                                 svn_io_file_del_none, fb->pool, fb->pool));

  hb->pool = pool;
  hb->tmppath = fb->tmppath;

  /* svn_txdelta_apply() closes the stream, but we want to close it in the
     close_file() function, so disown it here. */
  /* ### contrast to when we call svn_ra_get_file() which does NOT close the
     ### tmp_stream. we *should* be much more consistent! */
  svn_txdelta_apply(svn_stream_empty(pool),
                    svn_stream_disown(fb->tmp_stream, pool),
                    fb->text_digest, NULL, pool,
                    &hb->apply_handler, &hb->apply_baton);

  *handler_baton = hb;
  *handler = window_handler;
  return SVN_NO_ERROR;
}


static svn_error_t *
change_file_prop(void *file_baton,
                 const char *name,
                 const svn_string_t *value,
                 apr_pool_t *pool)
{
  struct file_baton *fb = file_baton;

  if (! value)
    return SVN_NO_ERROR;

  /* Store only the magic three properties. */
  if (strcmp(name, SVN_PROP_EOL_STYLE) == 0)
    fb->eol_style_val = svn_string_dup(value, fb->pool);

  else if (! fb->edit_baton->ignore_keywords &&
           strcmp(name, SVN_PROP_KEYWORDS) == 0)
    fb->keywords_val = svn_string_dup(value, fb->pool);

  else if (strcmp(name, SVN_PROP_EXECUTABLE) == 0)
    fb->executable_val = svn_string_dup(value, fb->pool);

  /* Try to fill out the baton's keywords-structure too. */
  else if (strcmp(name, SVN_PROP_ENTRY_COMMITTED_REV) == 0)
    fb->revision = apr_pstrdup(fb->pool, value->data);

  else if (strcmp(name, SVN_PROP_ENTRY_COMMITTED_DATE) == 0)
    SVN_ERR(svn_time_from_cstring(&fb->date, value->data, fb->pool));

  else if (strcmp(name, SVN_PROP_ENTRY_LAST_AUTHOR) == 0)
    fb->author = apr_pstrdup(fb->pool, value->data);

  else if (strcmp(name, SVN_PROP_SPECIAL) == 0)
    fb->special = TRUE;

  return SVN_NO_ERROR;
}


static svn_error_t *
change_dir_prop(void *dir_baton,
                const char *name,
                const svn_string_t *value,
                apr_pool_t *pool)
{
  struct dir_baton *db = dir_baton;
  struct edit_baton *eb = db->edit_baton;

  if (value && (strcmp(name, SVN_PROP_EXTERNALS) == 0))
    SVN_ERR(add_externals(eb->externals, db->path, value));

  return SVN_NO_ERROR;
}


/* Move the tmpfile to file, and send feedback. */
static svn_error_t *
close_file(void *file_baton,
           const char *text_digest,
           apr_pool_t *pool)
{
  struct file_baton *fb = file_baton;
  struct edit_baton *eb = fb->edit_baton;
  svn_checksum_t *text_checksum;
  svn_checksum_t *actual_checksum;

  /* Was a txdelta even sent? */
  if (! fb->tmppath)
    return SVN_NO_ERROR;

  SVN_ERR(svn_stream_close(fb->tmp_stream));

  SVN_ERR(svn_checksum_parse_hex(&text_checksum, svn_checksum_md5, text_digest,
                                 pool));
  actual_checksum = svn_checksum__from_digest(fb->text_digest,
                                              svn_checksum_md5, pool);

  /* Note that text_digest can be NULL when talking to certain repositories.
     In that case text_checksum will be NULL and the following match code
     will note that the checksums match */
  if (!svn_checksum_match(text_checksum, actual_checksum))
    return svn_checksum_mismatch_err(text_checksum, actual_checksum, pool,
                                     _("Checksum mismatch for '%s'"),
                                     svn_dirent_local_style(fb->path, pool));

  if ((! fb->eol_style_val) && (! fb->keywords_val) && (! fb->special))
    {
      SVN_ERR(svn_io_file_rename(fb->tmppath, fb->path, pool));
    }
  else
    {
      svn_subst_eol_style_t style;
      const char *eol = NULL;
      svn_boolean_t repair = FALSE;
      apr_hash_t *final_kw = NULL;

      if (fb->eol_style_val)
        {
          SVN_ERR(get_eol_style(&style, &eol, fb->eol_style_val->data,
                                eb->native_eol));
          repair = TRUE;
        }

      if (fb->keywords_val)
        SVN_ERR(svn_subst_build_keywords2(&final_kw, fb->keywords_val->data,
                                          fb->revision, fb->url, fb->date,
                                          fb->author, pool));

      SVN_ERR(svn_subst_copy_and_translate4(fb->tmppath, fb->path,
                                            eol, repair, final_kw,
                                            TRUE, /* expand */
                                            fb->special,
                                            eb->cancel_func, eb->cancel_baton,
                                            pool));

      SVN_ERR(svn_io_remove_file2(fb->tmppath, FALSE, pool));
    }

  if (fb->executable_val)
    SVN_ERR(svn_io_set_file_executable(fb->path, TRUE, FALSE, pool));

  if (fb->date && (! fb->special))
    SVN_ERR(svn_io_set_file_affected_time(fb->date, fb->path, pool));

  if (fb->edit_baton->notify_func)
    {
      svn_wc_notify_t *notify = svn_wc_create_notify(fb->path,
                                                     svn_wc_notify_update_add,
                                                     pool);
      notify->kind = svn_node_file;
      (*fb->edit_baton->notify_func)(fb->edit_baton->notify_baton, notify,
                                     pool);
    }

  return SVN_NO_ERROR;
}



/*** Public Interfaces ***/

svn_error_t *
svn_client_export5(svn_revnum_t *result_rev,
                   const char *from_path_or_url,
                   const char *to_path,
                   const svn_opt_revision_t *peg_revision,
                   const svn_opt_revision_t *revision,
                   svn_boolean_t overwrite,
                   svn_boolean_t ignore_externals,
                   svn_boolean_t ignore_keywords,
                   svn_depth_t depth,
                   const char *native_eol,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool)
{
  svn_revnum_t edit_revision = SVN_INVALID_REVNUM;
  const char *url;
  svn_boolean_t from_is_url = svn_path_is_url(from_path_or_url);

  SVN_ERR_ASSERT(peg_revision != NULL);
  SVN_ERR_ASSERT(revision != NULL);

  if (svn_path_is_url(to_path))
    return svn_error_createf(SVN_ERR_ILLEGAL_TARGET, NULL,
                             _("'%s' is not a local path"), to_path);

  peg_revision = svn_cl__rev_default_to_head_or_working(peg_revision,
                                                        from_path_or_url);
  revision = svn_cl__rev_default_to_peg(revision, peg_revision);

  if (from_is_url || ! SVN_CLIENT__REVKIND_IS_LOCAL_TO_WC(revision->kind))
    {
      svn_revnum_t revnum;
      svn_ra_session_t *ra_session;
      svn_node_kind_t kind;
      struct edit_baton *eb = apr_pcalloc(pool, sizeof(*eb));
      const char *repos_root_url;

      /* Get the RA connection. */
      SVN_ERR(svn_client__ra_session_from_path(&ra_session, &revnum,
                                               &url, from_path_or_url, NULL,
                                               peg_revision,
                                               revision, ctx, pool));

      /* Get the repository root. */
      SVN_ERR(svn_ra_get_repos_root2(ra_session, &repos_root_url, pool));

      eb->root_path = to_path;
      eb->root_url = url;
      eb->force = overwrite;
      eb->target_revision = &edit_revision;
      eb->externals = apr_hash_make(pool);
      eb->native_eol = native_eol;
      eb->ignore_keywords = ignore_keywords;
      eb->cancel_func = ctx->cancel_func;
      eb->cancel_baton = ctx->cancel_baton;
      eb->notify_func = ctx->notify_func2;
      eb->notify_baton = ctx->notify_baton2;

      SVN_ERR(svn_ra_check_path(ra_session, "", revnum, &kind, pool));

      if (kind == svn_node_file)
        {
          apr_hash_t *props;
          apr_hash_index_t *hi;
          struct file_baton *fb = apr_pcalloc(pool, sizeof(*fb));
          svn_node_kind_t to_kind;

          if (svn_path_is_empty(to_path))
            {
              if (from_is_url)
                to_path = svn_uri_basename(from_path_or_url, pool);
              else
                to_path = svn_dirent_basename(from_path_or_url, NULL);
              eb->root_path = to_path;
            }
          else
            {
              SVN_ERR(append_basename_if_dir(&to_path, from_path_or_url,
                                             from_is_url, pool));
              eb->root_path = to_path;
            }

          SVN_ERR(svn_io_check_path(to_path, &to_kind, pool));

          if ((to_kind == svn_node_file || to_kind == svn_node_unknown) &&
              ! overwrite)
            return svn_error_createf(SVN_ERR_ILLEGAL_TARGET, NULL,
                                     _("Destination file '%s' exists, and "
                                       "will not be overwritten unless forced"),
                                     svn_dirent_local_style(to_path, pool));
          else if (to_kind == svn_node_dir)
            return svn_error_createf(SVN_ERR_ILLEGAL_TARGET, NULL,
                                     _("Destination '%s' exists. Cannot "
                                       "overwrite directory with non-directory"),
                                     svn_dirent_local_style(to_path, pool));

          /* Since you cannot actually root an editor at a file, we
           * manually drive a few functions of our editor. */

          /* This is the equivalent of a parentless add_file(). */
          fb->edit_baton = eb;
          fb->path = eb->root_path;
          fb->url = eb->root_url;
          fb->pool = pool;

          /* Copied from apply_textdelta(). */
          SVN_ERR(svn_stream_open_unique(&fb->tmp_stream, &fb->tmppath,
                                         svn_dirent_dirname(fb->path, pool),
                                         svn_io_file_del_none,
                                         fb->pool, fb->pool));

          /* Step outside the editor-likeness for a moment, to actually talk
           * to the repository. */
          /* ### note: the stream will not be closed */
          SVN_ERR(svn_ra_get_file(ra_session, "", revnum,
                                  fb->tmp_stream,
                                  NULL, &props, pool));

          /* Push the props into change_file_prop(), to update the file_baton
           * with information. */
          for (hi = apr_hash_first(pool, props); hi; hi = apr_hash_next(hi))
            {
              const char *propname = svn__apr_hash_index_key(hi);
              const svn_string_t *propval = svn__apr_hash_index_val(hi);

              SVN_ERR(change_file_prop(fb, propname, propval, pool));
            }

          /* And now just use close_file() to do all the keyword and EOL
           * work, and put the file into place. */
          SVN_ERR(close_file(fb, NULL, pool));
        }
      else if (kind == svn_node_dir)
        {
          void *edit_baton;
          const svn_delta_editor_t *export_editor;
          const svn_ra_reporter3_t *reporter;
          void *report_baton;
          svn_delta_editor_t *editor = svn_delta_default_editor(pool);
          svn_boolean_t use_sleep = FALSE;

          editor->set_target_revision = set_target_revision;
          editor->open_root = open_root;
          editor->add_directory = add_directory;
          editor->add_file = add_file;
          editor->apply_textdelta = apply_textdelta;
          editor->close_file = close_file;
          editor->change_file_prop = change_file_prop;
          editor->change_dir_prop = change_dir_prop;

          SVN_ERR(svn_delta_get_cancellation_editor(ctx->cancel_func,
                                                    ctx->cancel_baton,
                                                    editor,
                                                    eb,
                                                    &export_editor,
                                                    &edit_baton,
                                                    pool));


          /* Manufacture a basic 'report' to the update reporter. */
          SVN_ERR(svn_ra_do_update2(ra_session,
                                    &reporter, &report_baton,
                                    revnum,
                                    "", /* no sub-target */
                                    depth,
                                    FALSE, /* don't want copyfrom-args */
                                    export_editor, edit_baton, pool));

          SVN_ERR(reporter->set_path(report_baton, "", revnum,
                                     /* Depth is irrelevant, as we're
                                        passing start_empty=TRUE anyway. */
                                     svn_depth_infinity,
                                     TRUE, /* "help, my dir is empty!" */
                                     NULL, pool));

          SVN_ERR(reporter->finish_report(report_baton, pool));

          /* Special case: Due to our sly export/checkout method of
           * updating an empty directory, no target will have been created
           * if the exported item is itself an empty directory
           * (export_editor->open_root never gets called, because there
           * are no "changes" to make to the empty dir we reported to the
           * repository).
           *
           * So we just create the empty dir manually; but we do it via
           * open_root_internal(), in order to get proper notification.
           */
          SVN_ERR(svn_io_check_path(to_path, &kind, pool));
          if (kind == svn_node_none)
            SVN_ERR(open_root_internal
                    (to_path, overwrite, ctx->notify_func2,
                     ctx->notify_baton2, pool));

          if (! ignore_externals && depth == svn_depth_infinity)
            {
              const char *to_abspath;

              SVN_ERR(svn_dirent_get_absolute(&to_abspath, to_path, pool));
              SVN_ERR(svn_client__export_externals(eb->externals,
                                                   from_path_or_url,
                                                   to_abspath, repos_root_url,
                                                   depth, native_eol,
                                                   ignore_keywords, &use_sleep,
                                                   ctx, pool));
            }
        }
      else if (kind == svn_node_none)
        {
          return svn_error_createf(SVN_ERR_RA_ILLEGAL_URL, NULL,
                                   _("URL '%s' doesn't exist"),
                                   from_path_or_url);
        }
      /* kind == svn_node_unknown not handled */
    }
  else
    {
      svn_node_kind_t kind;
      /* This is a working copy export. */
      /* just copy the contents of the working copy into the target path. */
      SVN_ERR(svn_dirent_get_absolute(&from_path_or_url, from_path_or_url,
                                      pool));

      SVN_ERR(svn_dirent_get_absolute(&to_path, to_path, pool));

      SVN_ERR(svn_io_check_path(from_path_or_url, &kind, pool));

      /* ### [JAF] If something already exists on disk at the destination path,
       * the behaviour depends on the node kinds of the source and destination
       * and on the FORCE flag.  The intention (I guess) is to follow the
       * semantics of svn_client_export5(), semantics that are not fully
       * documented but would be something like:
       *
       * -----------+---------------------------------------------------------
       *        Src | DIR                 FILE                SPECIAL
       * Dst (disk) +---------------------------------------------------------
       * NONE       | simple copy         simple copy         (as src=file?)
       * DIR        | merge if forced [2] inside if root [1]  (as src=file?)
       * FILE       | err                 overwr if forced[3] (as src=file?)
       * SPECIAL    | ???                 ???                 ???
       * -----------+---------------------------------------------------------
       *
       * [1] FILE onto DIR case: If this file is the root of the copy and thus
       *     the only node to be copied, then copy it as a child of the
       *     directory TO, applying these same rules again except that if this
       *     case occurs again (the child path is already a directory) then
       *     error out.  If this file is not the root of the copy (it is
       *     reached by recursion), then error out.
       *
       * [2] DIR onto DIR case.  If the 'FORCE' flag is true then copy the
       *     source's children inside the target dir, else error out.  When
       *     copying the children, apply the same set of rules, except in the
       *     FILE onto DIR case error out like in note [1].
       *
       * [3] If the 'FORCE' flag is true then overwrite the destination file
       *     else error out.
       *
       * The reality (apparently, looking at the code) is somewhat different.
       * For a start, to detect the source kind, it looks at what is on disk
       * rather than the versioned working or base node.
       */

      if (kind == svn_node_file)
        SVN_ERR(append_basename_if_dir(&to_path, from_path_or_url, FALSE,
                                       pool));

      SVN_ERR(copy_versioned_files(from_path_or_url, to_path, revision,
                                   overwrite, ignore_externals, ignore_keywords,
                                   depth, native_eol, ctx, pool));
    }


  if (ctx->notify_func2)
    {
      svn_wc_notify_t *notify
        = svn_wc_create_notify(to_path,
                               svn_wc_notify_update_completed, pool);
      notify->revision = edit_revision;
      (*ctx->notify_func2)(ctx->notify_baton2, notify, pool);
    }

  if (result_rev)
    *result_rev = edit_revision;

  return SVN_NO_ERROR;
}

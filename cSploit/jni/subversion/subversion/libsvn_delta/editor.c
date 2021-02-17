/*
 * editor.c :  editing trees of versioned resources
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

#include <apr_pools.h>

#include "svn_types.h"
#include "svn_error.h"
#include "svn_pools.h"

#include "private/svn_editor.h"


struct svn_editor_t
{
  void *baton;

  /* Standard cancellation function. Called before each callback.  */
  svn_cancel_func_t cancel_func;
  void *cancel_baton;

  /* Our callback functions match that of the set-many structure, so
     just use that.  */
  svn_editor_cb_many_t funcs;

  /* This pool is used as the scratch_pool for all callbacks.  */
  apr_pool_t *scratch_pool;
};


svn_error_t *
svn_editor_create(svn_editor_t **editor,
                  void *editor_baton,
                  svn_cancel_func_t cancel_func,
                  void *cancel_baton,
                  apr_pool_t *result_pool,
                  apr_pool_t *scratch_pool)
{
  *editor = apr_pcalloc(result_pool, sizeof(**editor));

  (*editor)->baton = editor_baton;
  (*editor)->cancel_func = cancel_func;
  (*editor)->cancel_baton = cancel_baton;
  (*editor)->scratch_pool = svn_pool_create(result_pool);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_editor_setcb_add_directory(svn_editor_t *editor,
                               svn_editor_cb_add_directory_t callback,
                               apr_pool_t *scratch_pool)
{
  editor->funcs.cb_add_directory = callback;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_editor_setcb_add_file(svn_editor_t *editor,
                          svn_editor_cb_add_file_t callback,
                          apr_pool_t *scratch_pool)
{
  editor->funcs.cb_add_file = callback;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_editor_setcb_add_symlink(svn_editor_t *editor,
                             svn_editor_cb_add_symlink_t callback,
                             apr_pool_t *scratch_pool)
{
  editor->funcs.cb_add_symlink = callback;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_editor_setcb_add_absent(svn_editor_t *editor,
                            svn_editor_cb_add_absent_t callback,
                            apr_pool_t *scratch_pool)
{
  editor->funcs.cb_add_absent = callback;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_editor_setcb_set_props(svn_editor_t *editor,
                           svn_editor_cb_set_props_t callback,
                           apr_pool_t *scratch_pool)
{
  editor->funcs.cb_set_props = callback;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_editor_setcb_set_text(svn_editor_t *editor,
                          svn_editor_cb_set_text_t callback,
                          apr_pool_t *scratch_pool)
{
  editor->funcs.cb_set_text = callback;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_editor_setcb_set_target(svn_editor_t *editor,
                            svn_editor_cb_set_target_t callback,
                            apr_pool_t *scratch_pool)
{
  editor->funcs.cb_set_target = callback;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_editor_setcb_delete(svn_editor_t *editor,
                        svn_editor_cb_delete_t callback,
                        apr_pool_t *scratch_pool)
{
  editor->funcs.cb_delete = callback;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_editor_setcb_copy(svn_editor_t *editor,
                      svn_editor_cb_copy_t callback,
                      apr_pool_t *scratch_pool)
{
  editor->funcs.cb_copy = callback;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_editor_setcb_move(svn_editor_t *editor,
                      svn_editor_cb_move_t callback,
                      apr_pool_t *scratch_pool)
{
  editor->funcs.cb_move = callback;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_editor_setcb_complete(svn_editor_t *editor,
                          svn_editor_cb_complete_t callback,
                          apr_pool_t *scratch_pool)
{
  editor->funcs.cb_complete = callback;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_editor_setcb_abort(svn_editor_t *editor,
                       svn_editor_cb_abort_t callback,
                       apr_pool_t *scratch_pool)
{
  editor->funcs.cb_abort = callback;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_editor_setcb_many(svn_editor_t *editor,
                      const svn_editor_cb_many_t *many,
                      apr_pool_t *scratch_pool)
{
#define COPY_CALLBACK(NAME) if (many->NAME) editor->funcs.NAME = many->NAME

  COPY_CALLBACK(cb_add_directory);
  COPY_CALLBACK(cb_add_file);
  COPY_CALLBACK(cb_add_symlink);
  COPY_CALLBACK(cb_add_absent);
  COPY_CALLBACK(cb_set_props);
  COPY_CALLBACK(cb_set_text);
  COPY_CALLBACK(cb_set_target);
  COPY_CALLBACK(cb_delete);
  COPY_CALLBACK(cb_copy);
  COPY_CALLBACK(cb_move);
  COPY_CALLBACK(cb_complete);
  COPY_CALLBACK(cb_abort);

#undef COPY_CALLBACK

  return SVN_NO_ERROR;
}


svn_error_t *
svn_editor_add_directory(svn_editor_t *editor,
                         const char *relpath,
                         const apr_array_header_t *children,
                         apr_hash_t *props,
                         svn_revnum_t replaces_rev)
{
  svn_error_t *err;

  SVN_ERR_ASSERT(editor->funcs.cb_add_directory != NULL);

  if (editor->cancel_func)
    SVN_ERR(editor->cancel_func(editor->cancel_baton));

  err = editor->funcs.cb_add_directory(editor->baton, relpath, children,
                                       props, replaces_rev,
                                       editor->scratch_pool);
  svn_pool_clear(editor->scratch_pool);
  return err;
}


svn_error_t *
svn_editor_add_file(svn_editor_t *editor,
                    const char *relpath,
                    apr_hash_t *props,
                    svn_revnum_t replaces_rev)
{
  svn_error_t *err;

  SVN_ERR_ASSERT(editor->funcs.cb_add_file != NULL);

  if (editor->cancel_func)
    SVN_ERR(editor->cancel_func(editor->cancel_baton));

  err = editor->funcs.cb_add_file(editor->baton, relpath, props,
                                  replaces_rev, editor->scratch_pool);
  svn_pool_clear(editor->scratch_pool);
  return err;
}


svn_error_t *
svn_editor_add_symlink(svn_editor_t *editor,
                       const char *relpath,
                       const char *target,
                       apr_hash_t *props,
                       svn_revnum_t replaces_rev)
{
  svn_error_t *err;

  SVN_ERR_ASSERT(editor->funcs.cb_add_symlink != NULL);

  if (editor->cancel_func)
    SVN_ERR(editor->cancel_func(editor->cancel_baton));

  err = editor->funcs.cb_add_symlink(editor->baton, relpath, target, props,
                                     replaces_rev, editor->scratch_pool);
  svn_pool_clear(editor->scratch_pool);
  return err;
}


svn_error_t *
svn_editor_add_absent(svn_editor_t *editor,
                      const char *relpath,
                      svn_node_kind_t kind,
                      svn_revnum_t replaces_rev)
{
  svn_error_t *err;

  SVN_ERR_ASSERT(editor->funcs.cb_add_absent != NULL);

  if (editor->cancel_func)
    SVN_ERR(editor->cancel_func(editor->cancel_baton));

  err = editor->funcs.cb_add_absent(editor->baton, relpath, kind,
                                    replaces_rev, editor->scratch_pool);
  svn_pool_clear(editor->scratch_pool);
  return err;
}


svn_error_t *
svn_editor_set_props(svn_editor_t *editor,
                     const char *relpath,
                     svn_revnum_t revision,
                     apr_hash_t *props,
                     svn_boolean_t complete)
{
  svn_error_t *err;

  SVN_ERR_ASSERT(editor->funcs.cb_set_props != NULL);

  if (editor->cancel_func)
    SVN_ERR(editor->cancel_func(editor->cancel_baton));

  err = editor->funcs.cb_set_props(editor->baton, relpath, revision, props,
                                   complete, editor->scratch_pool);
  svn_pool_clear(editor->scratch_pool);
  return err;
}


svn_error_t *
svn_editor_set_text(svn_editor_t *editor,
                    const char *relpath,
                    svn_revnum_t revision,
                    const svn_checksum_t *checksum,
                    svn_stream_t *contents)
{
  svn_error_t *err;

  SVN_ERR_ASSERT(editor->funcs.cb_set_text != NULL);

  if (editor->cancel_func)
    SVN_ERR(editor->cancel_func(editor->cancel_baton));

  err = editor->funcs.cb_set_text(editor->baton, relpath, revision,
                                  checksum, contents, editor->scratch_pool);
  svn_pool_clear(editor->scratch_pool);
  return err;
}


svn_error_t *
svn_editor_set_target(svn_editor_t *editor,
                      const char *relpath,
                      svn_revnum_t revision,
                      const char *target)
{
  svn_error_t *err;

  SVN_ERR_ASSERT(editor->funcs.cb_set_target != NULL);

  if (editor->cancel_func)
    SVN_ERR(editor->cancel_func(editor->cancel_baton));

  err = editor->funcs.cb_set_target(editor->baton, relpath, revision,
                                    target, editor->scratch_pool);
  svn_pool_clear(editor->scratch_pool);
  return err;
}


svn_error_t *
svn_editor_delete(svn_editor_t *editor,
                  const char *relpath,
                  svn_revnum_t revision)
{
  svn_error_t *err;

  SVN_ERR_ASSERT(editor->funcs.cb_delete != NULL);

  if (editor->cancel_func)
    SVN_ERR(editor->cancel_func(editor->cancel_baton));

  err = editor->funcs.cb_delete(editor->baton, relpath, revision,
                                editor->scratch_pool);
  svn_pool_clear(editor->scratch_pool);
  return err;
}


svn_error_t *
svn_editor_copy(svn_editor_t *editor,
                const char *src_relpath,
                svn_revnum_t src_revision,
                const char *dst_relpath,
                svn_revnum_t replaces_rev)
{
  svn_error_t *err;

  SVN_ERR_ASSERT(editor->funcs.cb_copy != NULL);

  if (editor->cancel_func)
    SVN_ERR(editor->cancel_func(editor->cancel_baton));

  err = editor->funcs.cb_copy(editor->baton, src_relpath, src_revision,
                              dst_relpath, replaces_rev,
                              editor->scratch_pool);
  svn_pool_clear(editor->scratch_pool);
  return err;
}


svn_error_t *
svn_editor_move(svn_editor_t *editor,
                const char *src_relpath,
                svn_revnum_t src_revision,
                const char *dst_relpath,
                svn_revnum_t replaces_rev)
{
  svn_error_t *err;

  SVN_ERR_ASSERT(editor->funcs.cb_move != NULL);

  if (editor->cancel_func)
    SVN_ERR(editor->cancel_func(editor->cancel_baton));

  err = editor->funcs.cb_move(editor->baton, src_relpath, src_revision,
                              dst_relpath, replaces_rev,
                              editor->scratch_pool);
  svn_pool_clear(editor->scratch_pool);
  return err;
}


svn_error_t *
svn_editor_complete(svn_editor_t *editor)
{
  svn_error_t *err;

  SVN_ERR_ASSERT(editor->funcs.cb_complete != NULL);

  err = editor->funcs.cb_complete(editor->baton, editor->scratch_pool);
  svn_pool_clear(editor->scratch_pool);
  return err;
}


svn_error_t *
svn_editor_abort(svn_editor_t *editor)
{
  svn_error_t *err;

  SVN_ERR_ASSERT(editor->funcs.cb_abort != NULL);

  err = editor->funcs.cb_abort(editor->baton, editor->scratch_pool);
  svn_pool_clear(editor->scratch_pool);
  return err;
}

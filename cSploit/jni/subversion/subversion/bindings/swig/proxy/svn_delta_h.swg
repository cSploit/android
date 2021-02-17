/* Proxy classes for svn_delta.h
 * DO NOT EDIT -- AUTOMATICALLY GENERATED */

/* No default constructors for opaque structs */
#ifdef SWIGPYTHON
%nodefault svn_txdelta_stream_t;
#endif

/* Callback typemaps */
#ifdef SWIGPYTHON
%apply CALLABLE_CALLBACK {
  svn_txdelta_window_handler_t,
  svn_txdelta_next_window_fn_t,
  svn_txdelta_md5_digest_fn_t,
  svn_delta_path_driver_cb_func_t,
  svn_file_rev_handler_t,
  svn_file_rev_handler_old_t
};
%apply CALLABLE_CALLBACK * {
  svn_txdelta_window_handler_t *,
  svn_txdelta_next_window_fn_t *,
  svn_txdelta_md5_digest_fn_t *,
  svn_delta_path_driver_cb_func_t *,
  svn_file_rev_handler_t *,
  svn_file_rev_handler_old_t *
};
#endif

/* Baton typemaps */
#ifdef SWIGPYTHON
%apply void *PY_AS_VOID {
  void *baton,
  void *handler_baton,
  void *cancel_baton,
  void *edit_baton,
  void *parent_baton,
  void *dir_baton,
  void *file_baton,
  void *wrapped_baton,
  void *wrapped_edit_baton,
  void *callback_baton
};
#endif

/* Includes */
%{
#include "svn_delta.h"
%}
%include svn_delta.h

/* Structure definitions */
#ifdef SWIGPYTHON
%proxy(svn_txdelta_op_t);
%proxy(svn_txdelta_window_t);
%opaque_proxy(svn_txdelta_stream_t);
%proxy(svn_delta_editor_t);
#endif

/* Callbacks */

%inline %{
static svn_error_t * svn_delta_editor_invoke_set_target_revision(
  svn_delta_editor_t * _obj, void *edit_baton, svn_revnum_t target_revision, apr_pool_t *scratch_pool) {
  return (_obj->set_target_revision)(edit_baton, target_revision, scratch_pool);
}

static svn_error_t * svn_delta_editor_invoke_open_root(
  svn_delta_editor_t * _obj, void *edit_baton, svn_revnum_t base_revision, apr_pool_t *result_pool, void **root_baton) {
  return (_obj->open_root)(edit_baton, base_revision, result_pool, root_baton);
}

static svn_error_t * svn_delta_editor_invoke_delete_entry(
  svn_delta_editor_t * _obj, const char *path, svn_revnum_t revision, void *parent_baton, apr_pool_t *scratch_pool) {
  return (_obj->delete_entry)(path, revision, parent_baton, scratch_pool);
}

static svn_error_t * svn_delta_editor_invoke_add_directory(
  svn_delta_editor_t * _obj, const char *path, void *parent_baton, const char *copyfrom_path, svn_revnum_t copyfrom_revision, apr_pool_t *result_pool, void **child_baton) {
  return (_obj->add_directory)(path, parent_baton, copyfrom_path, copyfrom_revision, result_pool, child_baton);
}

static svn_error_t * svn_delta_editor_invoke_open_directory(
  svn_delta_editor_t * _obj, const char *path, void *parent_baton, svn_revnum_t base_revision, apr_pool_t *result_pool, void **child_baton) {
  return (_obj->open_directory)(path, parent_baton, base_revision, result_pool, child_baton);
}

static svn_error_t * svn_delta_editor_invoke_change_dir_prop(
  svn_delta_editor_t * _obj, void *dir_baton, const char *name, const svn_string_t *value, apr_pool_t *scratch_pool) {
  return (_obj->change_dir_prop)(dir_baton, name, value, scratch_pool);
}

static svn_error_t * svn_delta_editor_invoke_close_directory(
  svn_delta_editor_t * _obj, void *dir_baton, apr_pool_t *scratch_pool) {
  return (_obj->close_directory)(dir_baton, scratch_pool);
}

static svn_error_t * svn_delta_editor_invoke_absent_directory(
  svn_delta_editor_t * _obj, const char *path, void *parent_baton, apr_pool_t *scratch_pool) {
  return (_obj->absent_directory)(path, parent_baton, scratch_pool);
}

static svn_error_t * svn_delta_editor_invoke_add_file(
  svn_delta_editor_t * _obj, const char *path, void *parent_baton, const char *copyfrom_path, svn_revnum_t copyfrom_revision, apr_pool_t *result_pool, void **file_baton) {
  return (_obj->add_file)(path, parent_baton, copyfrom_path, copyfrom_revision, result_pool, file_baton);
}

static svn_error_t * svn_delta_editor_invoke_open_file(
  svn_delta_editor_t * _obj, const char *path, void *parent_baton, svn_revnum_t base_revision, apr_pool_t *result_pool, void **file_baton) {
  return (_obj->open_file)(path, parent_baton, base_revision, result_pool, file_baton);
}

static svn_error_t * svn_delta_editor_invoke_apply_textdelta(
  svn_delta_editor_t * _obj, void *file_baton, const char *base_checksum, apr_pool_t *result_pool, svn_txdelta_window_handler_t *handler, void **handler_baton) {
  return (_obj->apply_textdelta)(file_baton, base_checksum, result_pool, handler, handler_baton);
}

static svn_error_t * svn_delta_editor_invoke_change_file_prop(
  svn_delta_editor_t * _obj, void *file_baton, const char *name, const svn_string_t *value, apr_pool_t *scratch_pool) {
  return (_obj->change_file_prop)(file_baton, name, value, scratch_pool);
}

static svn_error_t * svn_delta_editor_invoke_close_file(
  svn_delta_editor_t * _obj, void *file_baton, const char *text_checksum, apr_pool_t *scratch_pool) {
  return (_obj->close_file)(file_baton, text_checksum, scratch_pool);
}

static svn_error_t * svn_delta_editor_invoke_absent_file(
  svn_delta_editor_t * _obj, const char *path, void *parent_baton, apr_pool_t *scratch_pool) {
  return (_obj->absent_file)(path, parent_baton, scratch_pool);
}

static svn_error_t * svn_delta_editor_invoke_close_edit(
  svn_delta_editor_t * _obj, void *edit_baton, apr_pool_t *scratch_pool) {
  return (_obj->close_edit)(edit_baton, scratch_pool);
}

static svn_error_t * svn_delta_editor_invoke_abort_edit(
  svn_delta_editor_t * _obj, void *edit_baton, apr_pool_t *scratch_pool) {
  return (_obj->abort_edit)(edit_baton, scratch_pool);
}

static svn_error_t * svn_txdelta_invoke_window_handler(
  svn_txdelta_window_handler_t _obj, svn_txdelta_window_t *window, void *baton) {
  return _obj(window, baton);
}

static svn_error_t * svn_txdelta_invoke_next_window_fn(
  svn_txdelta_next_window_fn_t _obj, svn_txdelta_window_t **window, void *baton, apr_pool_t *pool) {
  return _obj(window, baton, pool);
}

static const unsigned char * svn_txdelta_invoke_md5_digest_fn(
  svn_txdelta_md5_digest_fn_t _obj, void *baton) {
  return _obj(baton);
}

static svn_error_t * svn_delta_invoke_path_driver_cb_func(
  svn_delta_path_driver_cb_func_t _obj, void **dir_baton, void *parent_baton, void *callback_baton, const char *path, apr_pool_t *pool) {
  return _obj(dir_baton, parent_baton, callback_baton, path, pool);
}

static svn_error_t * svn_file_invoke_rev_handler(
  svn_file_rev_handler_t _obj, void *baton, const char *path, svn_revnum_t rev, apr_hash_t *rev_props, svn_boolean_t result_of_merge, svn_txdelta_window_handler_t *delta_handler, void **delta_baton, apr_array_header_t *prop_diffs, apr_pool_t *pool) {
  return _obj(baton, path, rev, rev_props, result_of_merge, delta_handler, delta_baton, prop_diffs, pool);
}

static svn_error_t * svn_file_invoke_rev_handler_old(
  svn_file_rev_handler_old_t _obj, void *baton, const char *path, svn_revnum_t rev, apr_hash_t *rev_props, svn_txdelta_window_handler_t *delta_handler, void **delta_baton, apr_array_header_t *prop_diffs, apr_pool_t *pool) {
  return _obj(baton, path, rev, rev_props, delta_handler, delta_baton, prop_diffs, pool);
}

%}

#ifdef SWIGPYTHON
%funcptr_member_proxy(svn_delta_editor_t, set_target_revision, svn_delta_editor_invoke_set_target_revision);
%funcptr_member_proxy(svn_delta_editor_t, open_root, svn_delta_editor_invoke_open_root);
%funcptr_member_proxy(svn_delta_editor_t, delete_entry, svn_delta_editor_invoke_delete_entry);
%funcptr_member_proxy(svn_delta_editor_t, add_directory, svn_delta_editor_invoke_add_directory);
%funcptr_member_proxy(svn_delta_editor_t, open_directory, svn_delta_editor_invoke_open_directory);
%funcptr_member_proxy(svn_delta_editor_t, change_dir_prop, svn_delta_editor_invoke_change_dir_prop);
%funcptr_member_proxy(svn_delta_editor_t, close_directory, svn_delta_editor_invoke_close_directory);
%funcptr_member_proxy(svn_delta_editor_t, absent_directory, svn_delta_editor_invoke_absent_directory);
%funcptr_member_proxy(svn_delta_editor_t, add_file, svn_delta_editor_invoke_add_file);
%funcptr_member_proxy(svn_delta_editor_t, open_file, svn_delta_editor_invoke_open_file);
%funcptr_member_proxy(svn_delta_editor_t, apply_textdelta, svn_delta_editor_invoke_apply_textdelta);
%funcptr_member_proxy(svn_delta_editor_t, change_file_prop, svn_delta_editor_invoke_change_file_prop);
%funcptr_member_proxy(svn_delta_editor_t, close_file, svn_delta_editor_invoke_close_file);
%funcptr_member_proxy(svn_delta_editor_t, absent_file, svn_delta_editor_invoke_absent_file);
%funcptr_member_proxy(svn_delta_editor_t, close_edit, svn_delta_editor_invoke_close_edit);
%funcptr_member_proxy(svn_delta_editor_t, abort_edit, svn_delta_editor_invoke_abort_edit);
%funcptr_proxy(svn_txdelta_window_handler_t, svn_txdelta_invoke_window_handler);
%funcptr_proxy(svn_txdelta_next_window_fn_t, svn_txdelta_invoke_next_window_fn);
%funcptr_proxy(svn_txdelta_md5_digest_fn_t, svn_txdelta_invoke_md5_digest_fn);
%funcptr_proxy(svn_delta_path_driver_cb_func_t, svn_delta_invoke_path_driver_cb_func);
%funcptr_proxy(svn_file_rev_handler_t, svn_file_invoke_rev_handler);
%funcptr_proxy(svn_file_rev_handler_old_t, svn_file_invoke_rev_handler_old);

#endif

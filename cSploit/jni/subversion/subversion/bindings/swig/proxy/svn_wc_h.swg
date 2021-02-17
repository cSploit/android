/* Proxy classes for svn_wc.h
 * DO NOT EDIT -- AUTOMATICALLY GENERATED */

/* No default constructors for opaque structs */
#ifdef SWIGPYTHON
%nodefault svn_wc_context_t;
%nodefault svn_wc_adm_access_t;
%nodefault svn_wc_traversal_info_t;
%nodefault svn_wc_committed_queue_t;
#endif

/* Callback typemaps */
#ifdef SWIGPYTHON
%apply CALLABLE_CALLBACK {
  svn_wc_external_update_t,
  svn_wc_notify_func2_t,
  svn_wc_notify_func_t,
  svn_wc_conflict_resolver_func2_t,
  svn_wc_conflict_resolver_func_t,
  svn_wc_status_func4_t,
  svn_wc_status_func3_t,
  svn_wc_status_func2_t,
  svn_wc_status_func_t,
  svn_wc_get_file_t,
  svn_wc_dirents_func_t,
  svn_wc_canonicalize_svn_prop_get_file_t,
  svn_wc_upgrade_get_repos_info_t,
  svn_wc_relocation_validator3_t,
  svn_wc_relocation_validator2_t,
  svn_wc_relocation_validator_t,
  svn_changelist_receiver_t
};
%apply CALLABLE_CALLBACK * {
  svn_wc_external_update_t *,
  svn_wc_notify_func2_t *,
  svn_wc_notify_func_t *,
  svn_wc_conflict_resolver_func2_t *,
  svn_wc_conflict_resolver_func_t *,
  svn_wc_status_func4_t *,
  svn_wc_status_func3_t *,
  svn_wc_status_func2_t *,
  svn_wc_status_func_t *,
  svn_wc_get_file_t *,
  svn_wc_dirents_func_t *,
  svn_wc_canonicalize_svn_prop_get_file_t *,
  svn_wc_upgrade_get_repos_info_t *,
  svn_wc_relocation_validator3_t *,
  svn_wc_relocation_validator2_t *,
  svn_wc_relocation_validator_t *,
  svn_changelist_receiver_t *
};
#endif

/* Baton typemaps */
#ifdef SWIGPYTHON
%apply void *PY_AS_VOID {
  void *cancel_baton,
  void *baton,
  void *diff_baton,
  void *walk_baton,
  void *status_baton,
  void *set_locks_baton,
  void *notify_baton,
  void *report_baton,
  void *fetch_dirents_baton,
  void *conflict_baton,
  void *external_baton,
  void *fetch_baton,
  void *getter_baton,
  void *callback_baton,
  void *repos_info_baton,
  void *validator_baton,
  void *file_baton
};
#endif

/* Includes */
%{
#include "svn_wc.h"
%}
%include svn_wc.h

/* Structure definitions */
#ifdef SWIGPYTHON
%opaque_proxy(svn_wc_context_t);
%opaque_proxy(svn_wc_adm_access_t);
%opaque_proxy(svn_wc_traversal_info_t);
%proxy(svn_wc_external_item2_t);
%proxy(svn_wc_external_item_t);
%proxy(svn_wc_notify_t);
%proxy(svn_wc_conflict_version_t);
%proxy(svn_wc_conflict_description2_t);
%proxy(svn_wc_conflict_description_t);
%proxy(svn_wc_conflict_result_t);
%proxy(svn_wc_diff_callbacks4_t);
%proxy(svn_wc_diff_callbacks3_t);
%proxy(svn_wc_diff_callbacks2_t);
%proxy(svn_wc_diff_callbacks_t);
%proxy(svn_wc_entry_t);
%proxy(svn_wc_info_t);
%proxy(svn_wc_entry_callbacks2_t);
%proxy(svn_wc_entry_callbacks_t);
%proxy(svn_wc_status3_t);
%proxy(svn_wc_status2_t);
%proxy(svn_wc_status_t);
%opaque_proxy(svn_wc_committed_queue_t);
%proxy(svn_wc_revision_status_t);
#endif

/* Callbacks */

%inline %{
static svn_error_t * svn_wc_diff_callbacks4_invoke_file_opened(
  svn_wc_diff_callbacks4_t * _obj, svn_boolean_t *tree_conflicted, svn_boolean_t *skip, const char *path, svn_revnum_t rev, void *diff_baton, apr_pool_t *scratch_pool) {
  return (_obj->file_opened)(tree_conflicted, skip, path, rev, diff_baton, scratch_pool);
}

static svn_error_t * svn_wc_diff_callbacks4_invoke_file_changed(
  svn_wc_diff_callbacks4_t * _obj, svn_wc_notify_state_t *contentstate, svn_wc_notify_state_t *propstate, svn_boolean_t *tree_conflicted, const char *path, const char *tmpfile1, const char *tmpfile2, svn_revnum_t rev1, svn_revnum_t rev2, const char *mimetype1, const char *mimetype2, const apr_array_header_t *propchanges, apr_hash_t *originalprops, void *diff_baton, apr_pool_t *scratch_pool) {
  return (_obj->file_changed)(contentstate, propstate, tree_conflicted, path, tmpfile1, tmpfile2, rev1, rev2, mimetype1, mimetype2, propchanges, originalprops, diff_baton, scratch_pool);
}

static svn_error_t * svn_wc_diff_callbacks4_invoke_file_added(
  svn_wc_diff_callbacks4_t * _obj, svn_wc_notify_state_t *contentstate, svn_wc_notify_state_t *propstate, svn_boolean_t *tree_conflicted, const char *path, const char *tmpfile1, const char *tmpfile2, svn_revnum_t rev1, svn_revnum_t rev2, const char *mimetype1, const char *mimetype2, const char *copyfrom_path, svn_revnum_t copyfrom_revision, const apr_array_header_t *propchanges, apr_hash_t *originalprops, void *diff_baton, apr_pool_t *scratch_pool) {
  return (_obj->file_added)(contentstate, propstate, tree_conflicted, path, tmpfile1, tmpfile2, rev1, rev2, mimetype1, mimetype2, copyfrom_path, copyfrom_revision, propchanges, originalprops, diff_baton, scratch_pool);
}

static svn_error_t * svn_wc_diff_callbacks4_invoke_file_deleted(
  svn_wc_diff_callbacks4_t * _obj, svn_wc_notify_state_t *state, svn_boolean_t *tree_conflicted, const char *path, const char *tmpfile1, const char *tmpfile2, const char *mimetype1, const char *mimetype2, apr_hash_t *originalprops, void *diff_baton, apr_pool_t *scratch_pool) {
  return (_obj->file_deleted)(state, tree_conflicted, path, tmpfile1, tmpfile2, mimetype1, mimetype2, originalprops, diff_baton, scratch_pool);
}

static svn_error_t * svn_wc_diff_callbacks4_invoke_dir_deleted(
  svn_wc_diff_callbacks4_t * _obj, svn_wc_notify_state_t *state, svn_boolean_t *tree_conflicted, const char *path, void *diff_baton, apr_pool_t *scratch_pool) {
  return (_obj->dir_deleted)(state, tree_conflicted, path, diff_baton, scratch_pool);
}

static svn_error_t * svn_wc_diff_callbacks4_invoke_dir_opened(
  svn_wc_diff_callbacks4_t * _obj, svn_boolean_t *tree_conflicted, svn_boolean_t *skip, svn_boolean_t *skip_children, const char *path, svn_revnum_t rev, void *diff_baton, apr_pool_t *scratch_pool) {
  return (_obj->dir_opened)(tree_conflicted, skip, skip_children, path, rev, diff_baton, scratch_pool);
}

static svn_error_t * svn_wc_diff_callbacks4_invoke_dir_added(
  svn_wc_diff_callbacks4_t * _obj, svn_wc_notify_state_t *state, svn_boolean_t *tree_conflicted, svn_boolean_t *skip, svn_boolean_t *skip_children, const char *path, svn_revnum_t rev, const char *copyfrom_path, svn_revnum_t copyfrom_revision, void *diff_baton, apr_pool_t *scratch_pool) {
  return (_obj->dir_added)(state, tree_conflicted, skip, skip_children, path, rev, copyfrom_path, copyfrom_revision, diff_baton, scratch_pool);
}

static svn_error_t * svn_wc_diff_callbacks4_invoke_dir_props_changed(
  svn_wc_diff_callbacks4_t * _obj, svn_wc_notify_state_t *propstate, svn_boolean_t *tree_conflicted, const char *path, svn_boolean_t dir_was_added, const apr_array_header_t *propchanges, apr_hash_t *original_props, void *diff_baton, apr_pool_t *scratch_pool) {
  return (_obj->dir_props_changed)(propstate, tree_conflicted, path, dir_was_added, propchanges, original_props, diff_baton, scratch_pool);
}

static svn_error_t * svn_wc_diff_callbacks4_invoke_dir_closed(
  svn_wc_diff_callbacks4_t * _obj, svn_wc_notify_state_t *contentstate, svn_wc_notify_state_t *propstate, svn_boolean_t *tree_conflicted, const char *path, svn_boolean_t dir_was_added, void *diff_baton, apr_pool_t *scratch_pool) {
  return (_obj->dir_closed)(contentstate, propstate, tree_conflicted, path, dir_was_added, diff_baton, scratch_pool);
}

static svn_error_t * svn_wc_diff_callbacks3_invoke_file_changed(
  svn_wc_diff_callbacks3_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *contentstate, svn_wc_notify_state_t *propstate, svn_boolean_t *tree_conflicted, const char *path, const char *tmpfile1, const char *tmpfile2, svn_revnum_t rev1, svn_revnum_t rev2, const char *mimetype1, const char *mimetype2, const apr_array_header_t *propchanges, apr_hash_t *originalprops, void *diff_baton) {
  return (_obj->file_changed)(adm_access, contentstate, propstate, tree_conflicted, path, tmpfile1, tmpfile2, rev1, rev2, mimetype1, mimetype2, propchanges, originalprops, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks3_invoke_file_added(
  svn_wc_diff_callbacks3_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *contentstate, svn_wc_notify_state_t *propstate, svn_boolean_t *tree_conflicted, const char *path, const char *tmpfile1, const char *tmpfile2, svn_revnum_t rev1, svn_revnum_t rev2, const char *mimetype1, const char *mimetype2, const apr_array_header_t *propchanges, apr_hash_t *originalprops, void *diff_baton) {
  return (_obj->file_added)(adm_access, contentstate, propstate, tree_conflicted, path, tmpfile1, tmpfile2, rev1, rev2, mimetype1, mimetype2, propchanges, originalprops, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks3_invoke_file_deleted(
  svn_wc_diff_callbacks3_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *state, svn_boolean_t *tree_conflicted, const char *path, const char *tmpfile1, const char *tmpfile2, const char *mimetype1, const char *mimetype2, apr_hash_t *originalprops, void *diff_baton) {
  return (_obj->file_deleted)(adm_access, state, tree_conflicted, path, tmpfile1, tmpfile2, mimetype1, mimetype2, originalprops, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks3_invoke_dir_added(
  svn_wc_diff_callbacks3_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *state, svn_boolean_t *tree_conflicted, const char *path, svn_revnum_t rev, void *diff_baton) {
  return (_obj->dir_added)(adm_access, state, tree_conflicted, path, rev, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks3_invoke_dir_deleted(
  svn_wc_diff_callbacks3_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *state, svn_boolean_t *tree_conflicted, const char *path, void *diff_baton) {
  return (_obj->dir_deleted)(adm_access, state, tree_conflicted, path, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks3_invoke_dir_props_changed(
  svn_wc_diff_callbacks3_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *propstate, svn_boolean_t *tree_conflicted, const char *path, const apr_array_header_t *propchanges, apr_hash_t *original_props, void *diff_baton) {
  return (_obj->dir_props_changed)(adm_access, propstate, tree_conflicted, path, propchanges, original_props, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks3_invoke_dir_opened(
  svn_wc_diff_callbacks3_t * _obj, svn_wc_adm_access_t *adm_access, svn_boolean_t *tree_conflicted, const char *path, svn_revnum_t rev, void *diff_baton) {
  return (_obj->dir_opened)(adm_access, tree_conflicted, path, rev, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks3_invoke_dir_closed(
  svn_wc_diff_callbacks3_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *contentstate, svn_wc_notify_state_t *propstate, svn_boolean_t *tree_conflicted, const char *path, void *diff_baton) {
  return (_obj->dir_closed)(adm_access, contentstate, propstate, tree_conflicted, path, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks2_invoke_file_changed(
  svn_wc_diff_callbacks2_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *contentstate, svn_wc_notify_state_t *propstate, const char *path, const char *tmpfile1, const char *tmpfile2, svn_revnum_t rev1, svn_revnum_t rev2, const char *mimetype1, const char *mimetype2, const apr_array_header_t *propchanges, apr_hash_t *originalprops, void *diff_baton) {
  return (_obj->file_changed)(adm_access, contentstate, propstate, path, tmpfile1, tmpfile2, rev1, rev2, mimetype1, mimetype2, propchanges, originalprops, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks2_invoke_file_added(
  svn_wc_diff_callbacks2_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *contentstate, svn_wc_notify_state_t *propstate, const char *path, const char *tmpfile1, const char *tmpfile2, svn_revnum_t rev1, svn_revnum_t rev2, const char *mimetype1, const char *mimetype2, const apr_array_header_t *propchanges, apr_hash_t *originalprops, void *diff_baton) {
  return (_obj->file_added)(adm_access, contentstate, propstate, path, tmpfile1, tmpfile2, rev1, rev2, mimetype1, mimetype2, propchanges, originalprops, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks2_invoke_file_deleted(
  svn_wc_diff_callbacks2_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *state, const char *path, const char *tmpfile1, const char *tmpfile2, const char *mimetype1, const char *mimetype2, apr_hash_t *originalprops, void *diff_baton) {
  return (_obj->file_deleted)(adm_access, state, path, tmpfile1, tmpfile2, mimetype1, mimetype2, originalprops, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks2_invoke_dir_added(
  svn_wc_diff_callbacks2_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *state, const char *path, svn_revnum_t rev, void *diff_baton) {
  return (_obj->dir_added)(adm_access, state, path, rev, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks2_invoke_dir_deleted(
  svn_wc_diff_callbacks2_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *state, const char *path, void *diff_baton) {
  return (_obj->dir_deleted)(adm_access, state, path, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks2_invoke_dir_props_changed(
  svn_wc_diff_callbacks2_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *state, const char *path, const apr_array_header_t *propchanges, apr_hash_t *original_props, void *diff_baton) {
  return (_obj->dir_props_changed)(adm_access, state, path, propchanges, original_props, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks_invoke_file_changed(
  svn_wc_diff_callbacks_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *state, const char *path, const char *tmpfile1, const char *tmpfile2, svn_revnum_t rev1, svn_revnum_t rev2, const char *mimetype1, const char *mimetype2, void *diff_baton) {
  return (_obj->file_changed)(adm_access, state, path, tmpfile1, tmpfile2, rev1, rev2, mimetype1, mimetype2, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks_invoke_file_added(
  svn_wc_diff_callbacks_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *state, const char *path, const char *tmpfile1, const char *tmpfile2, svn_revnum_t rev1, svn_revnum_t rev2, const char *mimetype1, const char *mimetype2, void *diff_baton) {
  return (_obj->file_added)(adm_access, state, path, tmpfile1, tmpfile2, rev1, rev2, mimetype1, mimetype2, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks_invoke_file_deleted(
  svn_wc_diff_callbacks_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *state, const char *path, const char *tmpfile1, const char *tmpfile2, const char *mimetype1, const char *mimetype2, void *diff_baton) {
  return (_obj->file_deleted)(adm_access, state, path, tmpfile1, tmpfile2, mimetype1, mimetype2, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks_invoke_dir_added(
  svn_wc_diff_callbacks_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *state, const char *path, svn_revnum_t rev, void *diff_baton) {
  return (_obj->dir_added)(adm_access, state, path, rev, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks_invoke_dir_deleted(
  svn_wc_diff_callbacks_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *state, const char *path, void *diff_baton) {
  return (_obj->dir_deleted)(adm_access, state, path, diff_baton);
}

static svn_error_t * svn_wc_diff_callbacks_invoke_props_changed(
  svn_wc_diff_callbacks_t * _obj, svn_wc_adm_access_t *adm_access, svn_wc_notify_state_t *state, const char *path, const apr_array_header_t *propchanges, apr_hash_t *original_props, void *diff_baton) {
  return (_obj->props_changed)(adm_access, state, path, propchanges, original_props, diff_baton);
}

static svn_error_t * svn_wc_entry_callbacks2_invoke_found_entry(
  svn_wc_entry_callbacks2_t * _obj, const char *path, const svn_wc_entry_t *entry, void *walk_baton, apr_pool_t *pool) {
  return (_obj->found_entry)(path, entry, walk_baton, pool);
}

static svn_error_t * svn_wc_entry_callbacks2_invoke_handle_error(
  svn_wc_entry_callbacks2_t * _obj, const char *path, svn_error_t *err, void *walk_baton, apr_pool_t *pool) {
  return (_obj->handle_error)(path, err, walk_baton, pool);
}

static svn_error_t * svn_wc_entry_callbacks_invoke_found_entry(
  svn_wc_entry_callbacks_t * _obj, const char *path, const svn_wc_entry_t *entry, void *walk_baton, apr_pool_t *pool) {
  return (_obj->found_entry)(path, entry, walk_baton, pool);
}

static svn_error_t * svn_wc_invoke_external_update(
  svn_wc_external_update_t _obj, void *baton, const char *local_abspath, const svn_string_t *old_val, const svn_string_t *new_val, svn_depth_t depth, apr_pool_t *scratch_pool) {
  return _obj(baton, local_abspath, old_val, new_val, depth, scratch_pool);
}

static void svn_wc_invoke_notify_func2(
  svn_wc_notify_func2_t _obj, void *baton, const svn_wc_notify_t *notify, apr_pool_t *pool) {
  _obj(baton, notify, pool);
}

static void svn_wc_invoke_notify_func(
  svn_wc_notify_func_t _obj, void *baton, const char *path, svn_wc_notify_action_t action, svn_node_kind_t kind, const char *mime_type, svn_wc_notify_state_t content_state, svn_wc_notify_state_t prop_state, svn_revnum_t revision) {
  _obj(baton, path, action, kind, mime_type, content_state, prop_state, revision);
}

static svn_error_t * svn_wc_invoke_conflict_resolver_func2(
  svn_wc_conflict_resolver_func2_t _obj, svn_wc_conflict_result_t **result, const svn_wc_conflict_description2_t *description, void *baton, apr_pool_t *result_pool, apr_pool_t *scratch_pool) {
  return _obj(result, description, baton, result_pool, scratch_pool);
}

static svn_error_t * svn_wc_invoke_conflict_resolver_func(
  svn_wc_conflict_resolver_func_t _obj, svn_wc_conflict_result_t **result, const svn_wc_conflict_description_t *description, void *baton, apr_pool_t *pool) {
  return _obj(result, description, baton, pool);
}

static svn_error_t * svn_wc_invoke_status_func4(
  svn_wc_status_func4_t _obj, void *baton, const char *local_abspath, const svn_wc_status3_t *status, apr_pool_t *scratch_pool) {
  return _obj(baton, local_abspath, status, scratch_pool);
}

static svn_error_t * svn_wc_invoke_status_func3(
  svn_wc_status_func3_t _obj, void *baton, const char *path, svn_wc_status2_t *status, apr_pool_t *pool) {
  return _obj(baton, path, status, pool);
}

static void svn_wc_invoke_status_func2(
  svn_wc_status_func2_t _obj, void *baton, const char *path, svn_wc_status2_t *status) {
  _obj(baton, path, status);
}

static void svn_wc_invoke_status_func(
  svn_wc_status_func_t _obj, void *baton, const char *path, svn_wc_status_t *status) {
  _obj(baton, path, status);
}

static svn_error_t * svn_wc_invoke_get_file(
  svn_wc_get_file_t _obj, void *baton, const char *path, svn_revnum_t revision, svn_stream_t *stream, svn_revnum_t *fetched_rev, apr_hash_t **props, apr_pool_t *pool) {
  return _obj(baton, path, revision, stream, fetched_rev, props, pool);
}

static svn_error_t * svn_wc_invoke_dirents_func(
  svn_wc_dirents_func_t _obj, void *baton, apr_hash_t **dirents, const char *repos_root_url, const char *repos_relpath, apr_pool_t *result_pool, apr_pool_t *scratch_pool) {
  return _obj(baton, dirents, repos_root_url, repos_relpath, result_pool, scratch_pool);
}

static svn_error_t * svn_wc_invoke_canonicalize_svn_prop_get_file(
  svn_wc_canonicalize_svn_prop_get_file_t _obj, const svn_string_t **mime_type, svn_stream_t *stream, void *baton, apr_pool_t *pool) {
  return _obj(mime_type, stream, baton, pool);
}

static svn_error_t * svn_wc_invoke_upgrade_get_repos_info(
  svn_wc_upgrade_get_repos_info_t _obj, const char **repos_root, const char **repos_uuid, void *baton, const char *url, apr_pool_t *result_pool, apr_pool_t *scratch_pool) {
  return _obj(repos_root, repos_uuid, baton, url, result_pool, scratch_pool);
}

static svn_error_t * svn_wc_invoke_relocation_validator3(
  svn_wc_relocation_validator3_t _obj, void *baton, const char *uuid, const char *url, const char *root_url, apr_pool_t *pool) {
  return _obj(baton, uuid, url, root_url, pool);
}

static svn_error_t * svn_wc_invoke_relocation_validator2(
  svn_wc_relocation_validator2_t _obj, void *baton, const char *uuid, const char *url, svn_boolean_t root, apr_pool_t *pool) {
  return _obj(baton, uuid, url, root, pool);
}

static svn_error_t * svn_wc_invoke_relocation_validator(
  svn_wc_relocation_validator_t _obj, void *baton, const char *uuid, const char *url) {
  return _obj(baton, uuid, url);
}

static svn_error_t * svn_changelist_invoke_receiver(
  svn_changelist_receiver_t _obj, void *baton, const char *path, const char *changelist, apr_pool_t *pool) {
  return _obj(baton, path, changelist, pool);
}

%}

#ifdef SWIGPYTHON
%funcptr_member_proxy(svn_wc_diff_callbacks4_t, file_opened, svn_wc_diff_callbacks4_invoke_file_opened);
%funcptr_member_proxy(svn_wc_diff_callbacks4_t, file_changed, svn_wc_diff_callbacks4_invoke_file_changed);
%funcptr_member_proxy(svn_wc_diff_callbacks4_t, file_added, svn_wc_diff_callbacks4_invoke_file_added);
%funcptr_member_proxy(svn_wc_diff_callbacks4_t, file_deleted, svn_wc_diff_callbacks4_invoke_file_deleted);
%funcptr_member_proxy(svn_wc_diff_callbacks4_t, dir_deleted, svn_wc_diff_callbacks4_invoke_dir_deleted);
%funcptr_member_proxy(svn_wc_diff_callbacks4_t, dir_opened, svn_wc_diff_callbacks4_invoke_dir_opened);
%funcptr_member_proxy(svn_wc_diff_callbacks4_t, dir_added, svn_wc_diff_callbacks4_invoke_dir_added);
%funcptr_member_proxy(svn_wc_diff_callbacks4_t, dir_props_changed, svn_wc_diff_callbacks4_invoke_dir_props_changed);
%funcptr_member_proxy(svn_wc_diff_callbacks4_t, dir_closed, svn_wc_diff_callbacks4_invoke_dir_closed);
%funcptr_member_proxy(svn_wc_diff_callbacks3_t, file_changed, svn_wc_diff_callbacks3_invoke_file_changed);
%funcptr_member_proxy(svn_wc_diff_callbacks3_t, file_added, svn_wc_diff_callbacks3_invoke_file_added);
%funcptr_member_proxy(svn_wc_diff_callbacks3_t, file_deleted, svn_wc_diff_callbacks3_invoke_file_deleted);
%funcptr_member_proxy(svn_wc_diff_callbacks3_t, dir_added, svn_wc_diff_callbacks3_invoke_dir_added);
%funcptr_member_proxy(svn_wc_diff_callbacks3_t, dir_deleted, svn_wc_diff_callbacks3_invoke_dir_deleted);
%funcptr_member_proxy(svn_wc_diff_callbacks3_t, dir_props_changed, svn_wc_diff_callbacks3_invoke_dir_props_changed);
%funcptr_member_proxy(svn_wc_diff_callbacks3_t, dir_opened, svn_wc_diff_callbacks3_invoke_dir_opened);
%funcptr_member_proxy(svn_wc_diff_callbacks3_t, dir_closed, svn_wc_diff_callbacks3_invoke_dir_closed);
%funcptr_member_proxy(svn_wc_diff_callbacks2_t, file_changed, svn_wc_diff_callbacks2_invoke_file_changed);
%funcptr_member_proxy(svn_wc_diff_callbacks2_t, file_added, svn_wc_diff_callbacks2_invoke_file_added);
%funcptr_member_proxy(svn_wc_diff_callbacks2_t, file_deleted, svn_wc_diff_callbacks2_invoke_file_deleted);
%funcptr_member_proxy(svn_wc_diff_callbacks2_t, dir_added, svn_wc_diff_callbacks2_invoke_dir_added);
%funcptr_member_proxy(svn_wc_diff_callbacks2_t, dir_deleted, svn_wc_diff_callbacks2_invoke_dir_deleted);
%funcptr_member_proxy(svn_wc_diff_callbacks2_t, dir_props_changed, svn_wc_diff_callbacks2_invoke_dir_props_changed);
%funcptr_member_proxy(svn_wc_diff_callbacks_t, file_changed, svn_wc_diff_callbacks_invoke_file_changed);
%funcptr_member_proxy(svn_wc_diff_callbacks_t, file_added, svn_wc_diff_callbacks_invoke_file_added);
%funcptr_member_proxy(svn_wc_diff_callbacks_t, file_deleted, svn_wc_diff_callbacks_invoke_file_deleted);
%funcptr_member_proxy(svn_wc_diff_callbacks_t, dir_added, svn_wc_diff_callbacks_invoke_dir_added);
%funcptr_member_proxy(svn_wc_diff_callbacks_t, dir_deleted, svn_wc_diff_callbacks_invoke_dir_deleted);
%funcptr_member_proxy(svn_wc_diff_callbacks_t, props_changed, svn_wc_diff_callbacks_invoke_props_changed);
%funcptr_member_proxy(svn_wc_entry_callbacks2_t, found_entry, svn_wc_entry_callbacks2_invoke_found_entry);
%funcptr_member_proxy(svn_wc_entry_callbacks2_t, handle_error, svn_wc_entry_callbacks2_invoke_handle_error);
%funcptr_member_proxy(svn_wc_entry_callbacks_t, found_entry, svn_wc_entry_callbacks_invoke_found_entry);
%funcptr_proxy(svn_wc_external_update_t, svn_wc_invoke_external_update);
%funcptr_proxy(svn_wc_notify_func2_t, svn_wc_invoke_notify_func2);
%funcptr_proxy(svn_wc_notify_func_t, svn_wc_invoke_notify_func);
%funcptr_proxy(svn_wc_conflict_resolver_func2_t, svn_wc_invoke_conflict_resolver_func2);
%funcptr_proxy(svn_wc_conflict_resolver_func_t, svn_wc_invoke_conflict_resolver_func);
%funcptr_proxy(svn_wc_status_func4_t, svn_wc_invoke_status_func4);
%funcptr_proxy(svn_wc_status_func3_t, svn_wc_invoke_status_func3);
%funcptr_proxy(svn_wc_status_func2_t, svn_wc_invoke_status_func2);
%funcptr_proxy(svn_wc_status_func_t, svn_wc_invoke_status_func);
%funcptr_proxy(svn_wc_get_file_t, svn_wc_invoke_get_file);
%funcptr_proxy(svn_wc_dirents_func_t, svn_wc_invoke_dirents_func);
%funcptr_proxy(svn_wc_canonicalize_svn_prop_get_file_t, svn_wc_invoke_canonicalize_svn_prop_get_file);
%funcptr_proxy(svn_wc_upgrade_get_repos_info_t, svn_wc_invoke_upgrade_get_repos_info);
%funcptr_proxy(svn_wc_relocation_validator3_t, svn_wc_invoke_relocation_validator3);
%funcptr_proxy(svn_wc_relocation_validator2_t, svn_wc_invoke_relocation_validator2);
%funcptr_proxy(svn_wc_relocation_validator_t, svn_wc_invoke_relocation_validator);
%funcptr_proxy(svn_changelist_receiver_t, svn_changelist_invoke_receiver);

#endif

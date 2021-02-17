/* Proxy classes for svn_ra.h
 * DO NOT EDIT -- AUTOMATICALLY GENERATED */

/* No default constructors for opaque structs */
#ifdef SWIGPYTHON
%nodefault svn_ra_session_t;
#endif

/* Callback typemaps */
#ifdef SWIGPYTHON
%apply CALLABLE_CALLBACK {
  svn_ra_get_wc_prop_func_t,
  svn_ra_set_wc_prop_func_t,
  svn_ra_push_wc_prop_func_t,
  svn_ra_invalidate_wc_props_func_t,
  svn_ra_get_latest_revnum_func_t,
  svn_ra_get_client_string_func_t,
  svn_ra_file_rev_handler_t,
  svn_ra_lock_callback_t,
  svn_ra_progress_notify_func_t,
  svn_ra_replay_revstart_callback_t,
  svn_ra_replay_revfinish_callback_t,
  svn_ra_init_func_t
};
%apply CALLABLE_CALLBACK * {
  svn_ra_get_wc_prop_func_t *,
  svn_ra_set_wc_prop_func_t *,
  svn_ra_push_wc_prop_func_t *,
  svn_ra_invalidate_wc_props_func_t *,
  svn_ra_get_latest_revnum_func_t *,
  svn_ra_get_client_string_func_t *,
  svn_ra_file_rev_handler_t *,
  svn_ra_lock_callback_t *,
  svn_ra_progress_notify_func_t *,
  svn_ra_replay_revstart_callback_t *,
  svn_ra_replay_revfinish_callback_t *,
  svn_ra_init_func_t *
};
#endif

/* Baton typemaps */
#ifdef SWIGPYTHON
%apply void *PY_AS_VOID {
  void *baton,
  void *session_baton,
  void *replay_baton,
  void *edit_baton,
  void *report_baton,
  void *callback_baton,
  void *progress_baton,
  void *update_baton,
  void *switch_baton,
  void *status_baton,
  void *diff_baton,
  void *receiver_baton,
  void *handler_baton,
  void *lock_baton,
  void *ra_baton
};
#endif

/* Includes */
%{
#include "svn_ra.h"
%}
%include svn_ra.h

/* Structure definitions */
#ifdef SWIGPYTHON
%proxy(svn_ra_reporter3_t);
%proxy(svn_ra_reporter2_t);
%proxy(svn_ra_reporter_t);
%proxy(svn_ra_callbacks2_t);
%proxy(svn_ra_callbacks_t);
%opaque_proxy(svn_ra_session_t);
%proxy(svn_ra_plugin_t);
#endif

/* Callbacks */

%inline %{
static svn_error_t * svn_ra_reporter3_invoke_set_path(
  svn_ra_reporter3_t * _obj, void *report_baton, const char *path, svn_revnum_t revision, svn_depth_t depth, svn_boolean_t start_empty, const char *lock_token, apr_pool_t *pool) {
  return (_obj->set_path)(report_baton, path, revision, depth, start_empty, lock_token, pool);
}

static svn_error_t * svn_ra_reporter3_invoke_delete_path(
  svn_ra_reporter3_t * _obj, void *report_baton, const char *path, apr_pool_t *pool) {
  return (_obj->delete_path)(report_baton, path, pool);
}

static svn_error_t * svn_ra_reporter3_invoke_link_path(
  svn_ra_reporter3_t * _obj, void *report_baton, const char *path, const char *url, svn_revnum_t revision, svn_depth_t depth, svn_boolean_t start_empty, const char *lock_token, apr_pool_t *pool) {
  return (_obj->link_path)(report_baton, path, url, revision, depth, start_empty, lock_token, pool);
}

static svn_error_t * svn_ra_reporter3_invoke_finish_report(
  svn_ra_reporter3_t * _obj, void *report_baton, apr_pool_t *pool) {
  return (_obj->finish_report)(report_baton, pool);
}

static svn_error_t * svn_ra_reporter3_invoke_abort_report(
  svn_ra_reporter3_t * _obj, void *report_baton, apr_pool_t *pool) {
  return (_obj->abort_report)(report_baton, pool);
}

static svn_error_t * svn_ra_reporter2_invoke_set_path(
  svn_ra_reporter2_t * _obj, void *report_baton, const char *path, svn_revnum_t revision, svn_boolean_t start_empty, const char *lock_token, apr_pool_t *pool) {
  return (_obj->set_path)(report_baton, path, revision, start_empty, lock_token, pool);
}

static svn_error_t * svn_ra_reporter2_invoke_delete_path(
  svn_ra_reporter2_t * _obj, void *report_baton, const char *path, apr_pool_t *pool) {
  return (_obj->delete_path)(report_baton, path, pool);
}

static svn_error_t * svn_ra_reporter2_invoke_link_path(
  svn_ra_reporter2_t * _obj, void *report_baton, const char *path, const char *url, svn_revnum_t revision, svn_boolean_t start_empty, const char *lock_token, apr_pool_t *pool) {
  return (_obj->link_path)(report_baton, path, url, revision, start_empty, lock_token, pool);
}

static svn_error_t * svn_ra_reporter2_invoke_finish_report(
  svn_ra_reporter2_t * _obj, void *report_baton, apr_pool_t *pool) {
  return (_obj->finish_report)(report_baton, pool);
}

static svn_error_t * svn_ra_reporter2_invoke_abort_report(
  svn_ra_reporter2_t * _obj, void *report_baton, apr_pool_t *pool) {
  return (_obj->abort_report)(report_baton, pool);
}

static svn_error_t * svn_ra_reporter_invoke_set_path(
  svn_ra_reporter_t * _obj, void *report_baton, const char *path, svn_revnum_t revision, svn_boolean_t start_empty, apr_pool_t *pool) {
  return (_obj->set_path)(report_baton, path, revision, start_empty, pool);
}

static svn_error_t * svn_ra_reporter_invoke_delete_path(
  svn_ra_reporter_t * _obj, void *report_baton, const char *path, apr_pool_t *pool) {
  return (_obj->delete_path)(report_baton, path, pool);
}

static svn_error_t * svn_ra_reporter_invoke_link_path(
  svn_ra_reporter_t * _obj, void *report_baton, const char *path, const char *url, svn_revnum_t revision, svn_boolean_t start_empty, apr_pool_t *pool) {
  return (_obj->link_path)(report_baton, path, url, revision, start_empty, pool);
}

static svn_error_t * svn_ra_reporter_invoke_finish_report(
  svn_ra_reporter_t * _obj, void *report_baton, apr_pool_t *pool) {
  return (_obj->finish_report)(report_baton, pool);
}

static svn_error_t * svn_ra_reporter_invoke_abort_report(
  svn_ra_reporter_t * _obj, void *report_baton, apr_pool_t *pool) {
  return (_obj->abort_report)(report_baton, pool);
}

static svn_error_t * svn_ra_callbacks2_invoke_open_tmp_file(
  svn_ra_callbacks2_t * _obj, apr_file_t **fp, void *callback_baton, apr_pool_t *pool) {
  return (_obj->open_tmp_file)(fp, callback_baton, pool);
}

static svn_error_t * svn_ra_callbacks_invoke_open_tmp_file(
  svn_ra_callbacks_t * _obj, apr_file_t **fp, void *callback_baton, apr_pool_t *pool) {
  return (_obj->open_tmp_file)(fp, callback_baton, pool);
}

static svn_error_t * svn_ra_plugin_invoke_open(
  svn_ra_plugin_t * _obj, void **session_baton, const char *repos_URL, const svn_ra_callbacks_t *callbacks, void *callback_baton, apr_hash_t *config, apr_pool_t *pool) {
  return (_obj->open)(session_baton, repos_URL, callbacks, callback_baton, config, pool);
}

static svn_error_t * svn_ra_plugin_invoke_get_latest_revnum(
  svn_ra_plugin_t * _obj, void *session_baton, svn_revnum_t *latest_revnum, apr_pool_t *pool) {
  return (_obj->get_latest_revnum)(session_baton, latest_revnum, pool);
}

static svn_error_t * svn_ra_plugin_invoke_get_dated_revision(
  svn_ra_plugin_t * _obj, void *session_baton, svn_revnum_t *revision, apr_time_t tm, apr_pool_t *pool) {
  return (_obj->get_dated_revision)(session_baton, revision, tm, pool);
}

static svn_error_t * svn_ra_plugin_invoke_change_rev_prop(
  svn_ra_plugin_t * _obj, void *session_baton, svn_revnum_t rev, const char *name, const svn_string_t *value, apr_pool_t *pool) {
  return (_obj->change_rev_prop)(session_baton, rev, name, value, pool);
}

static svn_error_t * svn_ra_plugin_invoke_rev_proplist(
  svn_ra_plugin_t * _obj, void *session_baton, svn_revnum_t rev, apr_hash_t **props, apr_pool_t *pool) {
  return (_obj->rev_proplist)(session_baton, rev, props, pool);
}

static svn_error_t * svn_ra_plugin_invoke_rev_prop(
  svn_ra_plugin_t * _obj, void *session_baton, svn_revnum_t rev, const char *name, svn_string_t **value, apr_pool_t *pool) {
  return (_obj->rev_prop)(session_baton, rev, name, value, pool);
}

static svn_error_t * svn_ra_plugin_invoke_get_commit_editor(
  svn_ra_plugin_t * _obj, void *session_baton, const svn_delta_editor_t **editor, void **edit_baton, const char *log_msg, svn_commit_callback_t callback, void *callback_baton, apr_pool_t *pool) {
  return (_obj->get_commit_editor)(session_baton, editor, edit_baton, log_msg, callback, callback_baton, pool);
}

static svn_error_t * svn_ra_plugin_invoke_get_file(
  svn_ra_plugin_t * _obj, void *session_baton, const char *path, svn_revnum_t revision, svn_stream_t *stream, svn_revnum_t *fetched_rev, apr_hash_t **props, apr_pool_t *pool) {
  return (_obj->get_file)(session_baton, path, revision, stream, fetched_rev, props, pool);
}

static svn_error_t * svn_ra_plugin_invoke_get_dir(
  svn_ra_plugin_t * _obj, void *session_baton, const char *path, svn_revnum_t revision, apr_hash_t **dirents, svn_revnum_t *fetched_rev, apr_hash_t **props, apr_pool_t *pool) {
  return (_obj->get_dir)(session_baton, path, revision, dirents, fetched_rev, props, pool);
}

static svn_error_t * svn_ra_plugin_invoke_do_update(
  svn_ra_plugin_t * _obj, void *session_baton, const svn_ra_reporter_t **reporter, void **report_baton, svn_revnum_t revision_to_update_to, const char *update_target, svn_boolean_t recurse, const svn_delta_editor_t *update_editor, void *update_baton, apr_pool_t *pool) {
  return (_obj->do_update)(session_baton, reporter, report_baton, revision_to_update_to, update_target, recurse, update_editor, update_baton, pool);
}

static svn_error_t * svn_ra_plugin_invoke_do_switch(
  svn_ra_plugin_t * _obj, void *session_baton, const svn_ra_reporter_t **reporter, void **report_baton, svn_revnum_t revision_to_switch_to, const char *switch_target, svn_boolean_t recurse, const char *switch_url, const svn_delta_editor_t *switch_editor, void *switch_baton, apr_pool_t *pool) {
  return (_obj->do_switch)(session_baton, reporter, report_baton, revision_to_switch_to, switch_target, recurse, switch_url, switch_editor, switch_baton, pool);
}

static svn_error_t * svn_ra_plugin_invoke_do_status(
  svn_ra_plugin_t * _obj, void *session_baton, const svn_ra_reporter_t **reporter, void **report_baton, const char *status_target, svn_revnum_t revision, svn_boolean_t recurse, const svn_delta_editor_t *status_editor, void *status_baton, apr_pool_t *pool) {
  return (_obj->do_status)(session_baton, reporter, report_baton, status_target, revision, recurse, status_editor, status_baton, pool);
}

static svn_error_t * svn_ra_plugin_invoke_do_diff(
  svn_ra_plugin_t * _obj, void *session_baton, const svn_ra_reporter_t **reporter, void **report_baton, svn_revnum_t revision, const char *diff_target, svn_boolean_t recurse, svn_boolean_t ignore_ancestry, const char *versus_url, const svn_delta_editor_t *diff_editor, void *diff_baton, apr_pool_t *pool) {
  return (_obj->do_diff)(session_baton, reporter, report_baton, revision, diff_target, recurse, ignore_ancestry, versus_url, diff_editor, diff_baton, pool);
}

static svn_error_t * svn_ra_plugin_invoke_get_log(
  svn_ra_plugin_t * _obj, void *session_baton, const apr_array_header_t *paths, svn_revnum_t start, svn_revnum_t end, svn_boolean_t discover_changed_paths, svn_boolean_t strict_node_history, svn_log_message_receiver_t receiver, void *receiver_baton, apr_pool_t *pool) {
  return (_obj->get_log)(session_baton, paths, start, end, discover_changed_paths, strict_node_history, receiver, receiver_baton, pool);
}

static svn_error_t * svn_ra_plugin_invoke_check_path(
  svn_ra_plugin_t * _obj, void *session_baton, const char *path, svn_revnum_t revision, svn_node_kind_t *kind, apr_pool_t *pool) {
  return (_obj->check_path)(session_baton, path, revision, kind, pool);
}

static svn_error_t * svn_ra_plugin_invoke_get_uuid(
  svn_ra_plugin_t * _obj, void *session_baton, const char **uuid, apr_pool_t *pool) {
  return (_obj->get_uuid)(session_baton, uuid, pool);
}

static svn_error_t * svn_ra_plugin_invoke_get_repos_root(
  svn_ra_plugin_t * _obj, void *session_baton, const char **url, apr_pool_t *pool) {
  return (_obj->get_repos_root)(session_baton, url, pool);
}

static svn_error_t * svn_ra_plugin_invoke_get_locations(
  svn_ra_plugin_t * _obj, void *session_baton, apr_hash_t **locations, const char *path, svn_revnum_t peg_revision, apr_array_header_t *location_revisions, apr_pool_t *pool) {
  return (_obj->get_locations)(session_baton, locations, path, peg_revision, location_revisions, pool);
}

static svn_error_t * svn_ra_plugin_invoke_get_file_revs(
  svn_ra_plugin_t * _obj, void *session_baton, const char *path, svn_revnum_t start, svn_revnum_t end, svn_ra_file_rev_handler_t handler, void *handler_baton, apr_pool_t *pool) {
  return (_obj->get_file_revs)(session_baton, path, start, end, handler, handler_baton, pool);
}

static const svn_version_t * svn_ra_plugin_invoke_get_version(
  svn_ra_plugin_t * _obj) {
  return (_obj->get_version)();
}

static svn_error_t * svn_ra_invoke_get_wc_prop_func(
  svn_ra_get_wc_prop_func_t _obj, void *baton, const char *path, const char *name, const svn_string_t **value, apr_pool_t *pool) {
  return _obj(baton, path, name, value, pool);
}

static svn_error_t * svn_ra_invoke_set_wc_prop_func(
  svn_ra_set_wc_prop_func_t _obj, void *baton, const char *path, const char *name, const svn_string_t *value, apr_pool_t *pool) {
  return _obj(baton, path, name, value, pool);
}

static svn_error_t * svn_ra_invoke_push_wc_prop_func(
  svn_ra_push_wc_prop_func_t _obj, void *baton, const char *path, const char *name, const svn_string_t *value, apr_pool_t *pool) {
  return _obj(baton, path, name, value, pool);
}

static svn_error_t * svn_ra_invoke_invalidate_wc_props_func(
  svn_ra_invalidate_wc_props_func_t _obj, void *baton, const char *path, const char *name, apr_pool_t *pool) {
  return _obj(baton, path, name, pool);
}

static svn_error_t * svn_ra_invoke_get_latest_revnum_func(
  svn_ra_get_latest_revnum_func_t _obj, void *session_baton, svn_revnum_t *latest_revnum) {
  return _obj(session_baton, latest_revnum);
}

static svn_error_t * svn_ra_invoke_get_client_string_func(
  svn_ra_get_client_string_func_t _obj, void *baton, const char **name, apr_pool_t *pool) {
  return _obj(baton, name, pool);
}

static svn_error_t * svn_ra_invoke_file_rev_handler(
  svn_ra_file_rev_handler_t _obj, void *baton, const char *path, svn_revnum_t rev, apr_hash_t *rev_props, svn_txdelta_window_handler_t *delta_handler, void **delta_baton, apr_array_header_t *prop_diffs, apr_pool_t *pool) {
  return _obj(baton, path, rev, rev_props, delta_handler, delta_baton, prop_diffs, pool);
}

static svn_error_t * svn_ra_invoke_lock_callback(
  svn_ra_lock_callback_t _obj, void *baton, const char *path, svn_boolean_t do_lock, const svn_lock_t *lock, svn_error_t *ra_err, apr_pool_t *pool) {
  return _obj(baton, path, do_lock, lock, ra_err, pool);
}

static void svn_ra_invoke_progress_notify_func(
  svn_ra_progress_notify_func_t _obj, apr_off_t progress, apr_off_t total, void *baton, apr_pool_t *pool) {
  _obj(progress, total, baton, pool);
}

static svn_error_t * svn_ra_invoke_replay_revstart_callback(
  svn_ra_replay_revstart_callback_t _obj, svn_revnum_t revision, void *replay_baton, const svn_delta_editor_t **editor, void **edit_baton, apr_hash_t *rev_props, apr_pool_t *pool) {
  return _obj(revision, replay_baton, editor, edit_baton, rev_props, pool);
}

static svn_error_t * svn_ra_invoke_replay_revfinish_callback(
  svn_ra_replay_revfinish_callback_t _obj, svn_revnum_t revision, void *replay_baton, const svn_delta_editor_t *editor, void *edit_baton, apr_hash_t *rev_props, apr_pool_t *pool) {
  return _obj(revision, replay_baton, editor, edit_baton, rev_props, pool);
}

static svn_error_t * svn_ra_invoke_init_func(
  svn_ra_init_func_t _obj, int abi_version, apr_pool_t *pool, apr_hash_t *hash) {
  return _obj(abi_version, pool, hash);
}

%}

#ifdef SWIGPYTHON
%funcptr_member_proxy(svn_ra_reporter3_t, set_path, svn_ra_reporter3_invoke_set_path);
%funcptr_member_proxy(svn_ra_reporter3_t, delete_path, svn_ra_reporter3_invoke_delete_path);
%funcptr_member_proxy(svn_ra_reporter3_t, link_path, svn_ra_reporter3_invoke_link_path);
%funcptr_member_proxy(svn_ra_reporter3_t, finish_report, svn_ra_reporter3_invoke_finish_report);
%funcptr_member_proxy(svn_ra_reporter3_t, abort_report, svn_ra_reporter3_invoke_abort_report);
%funcptr_member_proxy(svn_ra_reporter2_t, set_path, svn_ra_reporter2_invoke_set_path);
%funcptr_member_proxy(svn_ra_reporter2_t, delete_path, svn_ra_reporter2_invoke_delete_path);
%funcptr_member_proxy(svn_ra_reporter2_t, link_path, svn_ra_reporter2_invoke_link_path);
%funcptr_member_proxy(svn_ra_reporter2_t, finish_report, svn_ra_reporter2_invoke_finish_report);
%funcptr_member_proxy(svn_ra_reporter2_t, abort_report, svn_ra_reporter2_invoke_abort_report);
%funcptr_member_proxy(svn_ra_reporter_t, set_path, svn_ra_reporter_invoke_set_path);
%funcptr_member_proxy(svn_ra_reporter_t, delete_path, svn_ra_reporter_invoke_delete_path);
%funcptr_member_proxy(svn_ra_reporter_t, link_path, svn_ra_reporter_invoke_link_path);
%funcptr_member_proxy(svn_ra_reporter_t, finish_report, svn_ra_reporter_invoke_finish_report);
%funcptr_member_proxy(svn_ra_reporter_t, abort_report, svn_ra_reporter_invoke_abort_report);
%funcptr_member_proxy(svn_ra_callbacks2_t, open_tmp_file, svn_ra_callbacks2_invoke_open_tmp_file);
%funcptr_member_proxy(svn_ra_callbacks_t, open_tmp_file, svn_ra_callbacks_invoke_open_tmp_file);
%funcptr_member_proxy(svn_ra_plugin_t, open, svn_ra_plugin_invoke_open);
%funcptr_member_proxy(svn_ra_plugin_t, get_latest_revnum, svn_ra_plugin_invoke_get_latest_revnum);
%funcptr_member_proxy(svn_ra_plugin_t, get_dated_revision, svn_ra_plugin_invoke_get_dated_revision);
%funcptr_member_proxy(svn_ra_plugin_t, change_rev_prop, svn_ra_plugin_invoke_change_rev_prop);
%funcptr_member_proxy(svn_ra_plugin_t, rev_proplist, svn_ra_plugin_invoke_rev_proplist);
%funcptr_member_proxy(svn_ra_plugin_t, rev_prop, svn_ra_plugin_invoke_rev_prop);
%funcptr_member_proxy(svn_ra_plugin_t, get_commit_editor, svn_ra_plugin_invoke_get_commit_editor);
%funcptr_member_proxy(svn_ra_plugin_t, get_file, svn_ra_plugin_invoke_get_file);
%funcptr_member_proxy(svn_ra_plugin_t, get_dir, svn_ra_plugin_invoke_get_dir);
%funcptr_member_proxy(svn_ra_plugin_t, do_update, svn_ra_plugin_invoke_do_update);
%funcptr_member_proxy(svn_ra_plugin_t, do_switch, svn_ra_plugin_invoke_do_switch);
%funcptr_member_proxy(svn_ra_plugin_t, do_status, svn_ra_plugin_invoke_do_status);
%funcptr_member_proxy(svn_ra_plugin_t, do_diff, svn_ra_plugin_invoke_do_diff);
%funcptr_member_proxy(svn_ra_plugin_t, get_log, svn_ra_plugin_invoke_get_log);
%funcptr_member_proxy(svn_ra_plugin_t, check_path, svn_ra_plugin_invoke_check_path);
%funcptr_member_proxy(svn_ra_plugin_t, get_uuid, svn_ra_plugin_invoke_get_uuid);
%funcptr_member_proxy(svn_ra_plugin_t, get_repos_root, svn_ra_plugin_invoke_get_repos_root);
%funcptr_member_proxy(svn_ra_plugin_t, get_locations, svn_ra_plugin_invoke_get_locations);
%funcptr_member_proxy(svn_ra_plugin_t, get_file_revs, svn_ra_plugin_invoke_get_file_revs);
%funcptr_member_proxy(svn_ra_plugin_t, get_version, svn_ra_plugin_invoke_get_version);
%funcptr_proxy(svn_ra_get_wc_prop_func_t, svn_ra_invoke_get_wc_prop_func);
%funcptr_proxy(svn_ra_set_wc_prop_func_t, svn_ra_invoke_set_wc_prop_func);
%funcptr_proxy(svn_ra_push_wc_prop_func_t, svn_ra_invoke_push_wc_prop_func);
%funcptr_proxy(svn_ra_invalidate_wc_props_func_t, svn_ra_invoke_invalidate_wc_props_func);
%funcptr_proxy(svn_ra_get_latest_revnum_func_t, svn_ra_invoke_get_latest_revnum_func);
%funcptr_proxy(svn_ra_get_client_string_func_t, svn_ra_invoke_get_client_string_func);
%funcptr_proxy(svn_ra_file_rev_handler_t, svn_ra_invoke_file_rev_handler);
%funcptr_proxy(svn_ra_lock_callback_t, svn_ra_invoke_lock_callback);
%funcptr_proxy(svn_ra_progress_notify_func_t, svn_ra_invoke_progress_notify_func);
%funcptr_proxy(svn_ra_replay_revstart_callback_t, svn_ra_invoke_replay_revstart_callback);
%funcptr_proxy(svn_ra_replay_revfinish_callback_t, svn_ra_invoke_replay_revfinish_callback);
%funcptr_proxy(svn_ra_init_func_t, svn_ra_invoke_init_func);

#endif

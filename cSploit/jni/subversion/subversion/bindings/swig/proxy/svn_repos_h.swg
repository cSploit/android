/* Proxy classes for svn_repos.h
 * DO NOT EDIT -- AUTOMATICALLY GENERATED */

/* No default constructors for opaque structs */
#ifdef SWIGPYTHON
%nodefault svn_repos_t;
%nodefault svn_authz_t;
#endif

/* Callback typemaps */
#ifdef SWIGPYTHON
%apply CALLABLE_CALLBACK {
  svn_repos_authz_func_t,
  svn_repos_authz_callback_t,
  svn_repos_file_rev_handler_t,
  svn_repos_notify_func_t,
  svn_repos_history_func_t
};
%apply CALLABLE_CALLBACK * {
  svn_repos_authz_func_t *,
  svn_repos_authz_callback_t *,
  svn_repos_file_rev_handler_t *,
  svn_repos_notify_func_t *,
  svn_repos_history_func_t *
};
#endif

/* Baton typemaps */
#ifdef SWIGPYTHON
%apply void *PY_AS_VOID {
  void *baton,
  void *notify_baton,
  void *start_callback_baton,
  void *cancel_baton,
  void *edit_baton,
  void *authz_read_baton,
  void *report_baton,
  void *callback_baton,
  void *authz_baton,
  void *history_baton,
  void *receiver_baton,
  void *handler_baton,
  void *parse_baton,
  void *revision_baton,
  void *node_baton
};
#endif

/* Includes */
%{
#include "svn_repos.h"
%}
%include svn_repos.h

/* Structure definitions */
#ifdef SWIGPYTHON
%proxy(svn_repos_notify_t);
%opaque_proxy(svn_repos_t);
%proxy(svn_repos_node_t);
%proxy(svn_repos_parse_fns2_t);
%proxy(svn_repos_parse_fns_t);
%opaque_proxy(svn_authz_t);
#endif

/* Callbacks */

%inline %{
static svn_error_t * svn_repos_parse_fns2_invoke_new_revision_record(
  svn_repos_parse_fns2_t * _obj, void **revision_baton, apr_hash_t *headers, void *parse_baton, apr_pool_t *pool) {
  return (_obj->new_revision_record)(revision_baton, headers, parse_baton, pool);
}

static svn_error_t * svn_repos_parse_fns2_invoke_uuid_record(
  svn_repos_parse_fns2_t * _obj, const char *uuid, void *parse_baton, apr_pool_t *pool) {
  return (_obj->uuid_record)(uuid, parse_baton, pool);
}

static svn_error_t * svn_repos_parse_fns2_invoke_new_node_record(
  svn_repos_parse_fns2_t * _obj, void **node_baton, apr_hash_t *headers, void *revision_baton, apr_pool_t *pool) {
  return (_obj->new_node_record)(node_baton, headers, revision_baton, pool);
}

static svn_error_t * svn_repos_parse_fns2_invoke_set_revision_property(
  svn_repos_parse_fns2_t * _obj, void *revision_baton, const char *name, const svn_string_t *value) {
  return (_obj->set_revision_property)(revision_baton, name, value);
}

static svn_error_t * svn_repos_parse_fns2_invoke_set_node_property(
  svn_repos_parse_fns2_t * _obj, void *node_baton, const char *name, const svn_string_t *value) {
  return (_obj->set_node_property)(node_baton, name, value);
}

static svn_error_t * svn_repos_parse_fns2_invoke_delete_node_property(
  svn_repos_parse_fns2_t * _obj, void *node_baton, const char *name) {
  return (_obj->delete_node_property)(node_baton, name);
}

static svn_error_t * svn_repos_parse_fns2_invoke_remove_node_props(
  svn_repos_parse_fns2_t * _obj, void *node_baton) {
  return (_obj->remove_node_props)(node_baton);
}

static svn_error_t * svn_repos_parse_fns2_invoke_set_fulltext(
  svn_repos_parse_fns2_t * _obj, svn_stream_t **stream, void *node_baton) {
  return (_obj->set_fulltext)(stream, node_baton);
}

static svn_error_t * svn_repos_parse_fns2_invoke_apply_textdelta(
  svn_repos_parse_fns2_t * _obj, svn_txdelta_window_handler_t *handler, void **handler_baton, void *node_baton) {
  return (_obj->apply_textdelta)(handler, handler_baton, node_baton);
}

static svn_error_t * svn_repos_parse_fns2_invoke_close_node(
  svn_repos_parse_fns2_t * _obj, void *node_baton) {
  return (_obj->close_node)(node_baton);
}

static svn_error_t * svn_repos_parse_fns2_invoke_close_revision(
  svn_repos_parse_fns2_t * _obj, void *revision_baton) {
  return (_obj->close_revision)(revision_baton);
}

static svn_error_t * svn_repos_invoke_authz_func(
  svn_repos_authz_func_t _obj, svn_boolean_t *allowed, svn_fs_root_t *root, const char *path, void *baton, apr_pool_t *pool) {
  return _obj(allowed, root, path, baton, pool);
}

static svn_error_t * svn_repos_invoke_authz_callback(
  svn_repos_authz_callback_t _obj, svn_repos_authz_access_t required, svn_boolean_t *allowed, svn_fs_root_t *root, const char *path, void *baton, apr_pool_t *pool) {
  return _obj(required, allowed, root, path, baton, pool);
}

static svn_error_t * svn_repos_invoke_file_rev_handler(
  svn_repos_file_rev_handler_t _obj, void *baton, const char *path, svn_revnum_t rev, apr_hash_t *rev_props, svn_txdelta_window_handler_t *delta_handler, void **delta_baton, apr_array_header_t *prop_diffs, apr_pool_t *pool) {
  return _obj(baton, path, rev, rev_props, delta_handler, delta_baton, prop_diffs, pool);
}

static void svn_repos_invoke_notify_func(
  svn_repos_notify_func_t _obj, void *baton, const svn_repos_notify_t *notify, apr_pool_t *scratch_pool) {
  _obj(baton, notify, scratch_pool);
}

static svn_error_t * svn_repos_invoke_history_func(
  svn_repos_history_func_t _obj, void *baton, const char *path, svn_revnum_t revision, apr_pool_t *pool) {
  return _obj(baton, path, revision, pool);
}

%}

#ifdef SWIGPYTHON
%funcptr_member_proxy(svn_repos_parse_fns2_t, new_revision_record, svn_repos_parse_fns2_invoke_new_revision_record);
%funcptr_member_proxy(svn_repos_parse_fns2_t, uuid_record, svn_repos_parse_fns2_invoke_uuid_record);
%funcptr_member_proxy(svn_repos_parse_fns2_t, new_node_record, svn_repos_parse_fns2_invoke_new_node_record);
%funcptr_member_proxy(svn_repos_parse_fns2_t, set_revision_property, svn_repos_parse_fns2_invoke_set_revision_property);
%funcptr_member_proxy(svn_repos_parse_fns2_t, set_node_property, svn_repos_parse_fns2_invoke_set_node_property);
%funcptr_member_proxy(svn_repos_parse_fns2_t, delete_node_property, svn_repos_parse_fns2_invoke_delete_node_property);
%funcptr_member_proxy(svn_repos_parse_fns2_t, remove_node_props, svn_repos_parse_fns2_invoke_remove_node_props);
%funcptr_member_proxy(svn_repos_parse_fns2_t, set_fulltext, svn_repos_parse_fns2_invoke_set_fulltext);
%funcptr_member_proxy(svn_repos_parse_fns2_t, apply_textdelta, svn_repos_parse_fns2_invoke_apply_textdelta);
%funcptr_member_proxy(svn_repos_parse_fns2_t, close_node, svn_repos_parse_fns2_invoke_close_node);
%funcptr_member_proxy(svn_repos_parse_fns2_t, close_revision, svn_repos_parse_fns2_invoke_close_revision);
%funcptr_proxy(svn_repos_authz_func_t, svn_repos_invoke_authz_func);
%funcptr_proxy(svn_repos_authz_callback_t, svn_repos_invoke_authz_callback);
%funcptr_proxy(svn_repos_file_rev_handler_t, svn_repos_invoke_file_rev_handler);
%funcptr_proxy(svn_repos_notify_func_t, svn_repos_invoke_notify_func);
%funcptr_proxy(svn_repos_history_func_t, svn_repos_invoke_history_func);

#endif

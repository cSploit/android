/* Proxy classes for svn_types.h
 * DO NOT EDIT -- AUTOMATICALLY GENERATED */

/* No default constructors for opaque structs */
#ifdef SWIGPYTHON
%nodefault svn_version_t;
#endif

/* Callback typemaps */
#ifdef SWIGPYTHON
%apply CALLABLE_CALLBACK {
  svn_log_entry_receiver_t,
  svn_log_message_receiver_t,
  svn_commit_callback2_t,
  svn_commit_callback_t,
  svn_cancel_func_t,
  svn_location_segment_receiver_t
};
%apply CALLABLE_CALLBACK * {
  svn_log_entry_receiver_t *,
  svn_log_message_receiver_t *,
  svn_commit_callback2_t *,
  svn_commit_callback_t *,
  svn_cancel_func_t *,
  svn_location_segment_receiver_t *
};
#endif

/* Baton typemaps */
#ifdef SWIGPYTHON
%apply void *PY_AS_VOID {
  void *baton,
  void *cancel_baton
};
#endif

/* Includes */
%{
#include "svn_types.h"
%}
%include svn_types.h

/* Structure definitions */
#ifdef SWIGPYTHON
%proxy(svn_error_t);
%opaque_proxy(svn_version_t);
%proxy(svn_dirent_t);
%proxy(svn_commit_info_t);
%proxy(svn_log_changed_path2_t);
%proxy(svn_log_changed_path_t);
%proxy(svn_log_entry_t);
%proxy(svn_lock_t);
%proxy(svn_merge_range_t);
%proxy(svn_location_segment_t);
#endif

/* Callbacks */

%inline %{
static svn_error_t * svn_log_invoke_entry_receiver(
  svn_log_entry_receiver_t _obj, void *baton, svn_log_entry_t *log_entry, apr_pool_t *pool) {
  return _obj(baton, log_entry, pool);
}

static svn_error_t * svn_log_invoke_message_receiver(
  svn_log_message_receiver_t _obj, void *baton, apr_hash_t *changed_paths, svn_revnum_t revision, const char *author, const char *date, const char *message, apr_pool_t *pool) {
  return _obj(baton, changed_paths, revision, author, date, message, pool);
}

static svn_error_t * svn_commit_invoke_callback2(
  svn_commit_callback2_t _obj, const svn_commit_info_t *commit_info, void *baton, apr_pool_t *pool) {
  return _obj(commit_info, baton, pool);
}

static svn_error_t * svn_commit_invoke_callback(
  svn_commit_callback_t _obj, svn_revnum_t new_revision, const char *date, const char *author, void *baton) {
  return _obj(new_revision, date, author, baton);
}

static svn_error_t * svn_cancel_invoke_func(
  svn_cancel_func_t _obj, void *cancel_baton) {
  return _obj(cancel_baton);
}

static svn_error_t * svn_location_invoke_segment_receiver(
  svn_location_segment_receiver_t _obj, svn_location_segment_t *segment, void *baton, apr_pool_t *pool) {
  return _obj(segment, baton, pool);
}

%}

#ifdef SWIGPYTHON
%funcptr_proxy(svn_log_entry_receiver_t, svn_log_invoke_entry_receiver);
%funcptr_proxy(svn_log_message_receiver_t, svn_log_invoke_message_receiver);
%funcptr_proxy(svn_commit_callback2_t, svn_commit_invoke_callback2);
%funcptr_proxy(svn_commit_callback_t, svn_commit_invoke_callback);
%funcptr_proxy(svn_cancel_func_t, svn_cancel_invoke_func);
%funcptr_proxy(svn_location_segment_receiver_t, svn_location_invoke_segment_receiver);

#endif

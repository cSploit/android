/* Proxy classes for svn_fs.h
 * DO NOT EDIT -- AUTOMATICALLY GENERATED */

/* No default constructors for opaque structs */
#ifdef SWIGPYTHON
%nodefault svn_fs_t;
%nodefault svn_fs_access_t;
%nodefault svn_fs_id_t;
%nodefault svn_fs_txn_t;
%nodefault svn_fs_root_t;
%nodefault svn_fs_history_t;
#endif

/* Callback typemaps */
#ifdef SWIGPYTHON
%apply CALLABLE_CALLBACK {
  svn_fs_warning_callback_t,
  svn_fs_get_locks_callback_t,
  svn_fs_pack_notify_t
};
%apply CALLABLE_CALLBACK * {
  svn_fs_warning_callback_t *,
  svn_fs_get_locks_callback_t *,
  svn_fs_pack_notify_t *
};
#endif

/* Baton typemaps */
#ifdef SWIGPYTHON
%apply void *PY_AS_VOID {
  void *baton,
  void *warning_baton,
  void *cancel_baton,
  void *get_locks_baton,
  void *notify_baton
};
#endif

/* Includes */
%{
#include "svn_fs.h"
%}
%include svn_fs.h

/* Structure definitions */
#ifdef SWIGPYTHON
%opaque_proxy(svn_fs_t);
%opaque_proxy(svn_fs_access_t);
%opaque_proxy(svn_fs_id_t);
%opaque_proxy(svn_fs_txn_t);
%opaque_proxy(svn_fs_root_t);
%proxy(svn_fs_path_change2_t);
%proxy(svn_fs_path_change_t);
%opaque_proxy(svn_fs_history_t);
%proxy(svn_fs_dirent_t);
#endif

/* Callbacks */

%inline %{
static void svn_fs_invoke_warning_callback(
  svn_fs_warning_callback_t _obj, void *baton, svn_error_t *err) {
  _obj(baton, err);
}

static svn_error_t * svn_fs_invoke_get_locks_callback(
  svn_fs_get_locks_callback_t _obj, void *baton, svn_lock_t *lock, apr_pool_t *pool) {
  return _obj(baton, lock, pool);
}

static svn_error_t * svn_fs_invoke_pack_notify(
  svn_fs_pack_notify_t _obj, void *baton, apr_int64_t shard, svn_fs_pack_notify_action_t action, apr_pool_t *pool) {
  return _obj(baton, shard, action, pool);
}

%}

#ifdef SWIGPYTHON
%funcptr_proxy(svn_fs_warning_callback_t, svn_fs_invoke_warning_callback);
%funcptr_proxy(svn_fs_get_locks_callback_t, svn_fs_invoke_get_locks_callback);
%funcptr_proxy(svn_fs_pack_notify_t, svn_fs_invoke_pack_notify);

#endif

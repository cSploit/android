/*
 * swigutil_pl.h :  utility functions and stuff for the SWIG Perl bindings
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


#ifndef SVN_SWIG_SWIGUTIL_PL_H
#define SVN_SWIG_SWIGUTIL_PL_H

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

#include <apr.h>
#include <apr_pools.h>
#include <apr_strings.h>
#include <apr_hash.h>
#include <apr_tables.h>

#include "svn_types.h"
#include "svn_string.h"
#include "svn_delta.h"
#include "svn_client.h"
#include "svn_repos.h"
#include "svn_private_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if defined(_MSC_VER)
#  if _MSC_VER >= 1300 && _INTEGRAL_MAX_BITS >= 64
#    define strtoll _strtoi64
#  else
#    define strtoll(str, endptr, base) _atoi64(str)
#  endif
#endif



#if defined(SVN_AVOID_CIRCULAR_LINKAGE_AT_ALL_COSTS_HACK)
typedef apr_pool_t *(*svn_swig_pl_get_current_pool_t)(void);
typedef void (*svn_swig_pl_set_current_pool_t)(apr_pool_t *pool);

void svn_swig_pl_bind_current_pool_fns(svn_swig_pl_get_current_pool_t get,
                                       svn_swig_pl_set_current_pool_t set);
#endif

apr_pool_t *svn_swig_pl_make_pool(SV *obj);

typedef enum perl_func_invoker {
    CALL_METHOD,
    CALL_SV
} perl_func_invoker_t;

svn_error_t *svn_swig_pl_callback_thunk(perl_func_invoker_t caller_func,
                                        void *func,
                                        SV **result,
                                        const char *fmt, ...);

SV *svn_swig_pl_prophash_to_hash(apr_hash_t *hash);
SV *svn_swig_pl_convert_hash(apr_hash_t *hash, swig_type_info *tinfo);

SV *svn_swig_pl_convert_hash_of_revnum_t(apr_hash_t *hash);

const apr_array_header_t *svn_swig_pl_strings_to_array(SV *source,
                                                       apr_pool_t *pool);

apr_hash_t *svn_swig_pl_strings_to_hash(SV *source,
                                        apr_pool_t *pool);
apr_hash_t *svn_swig_pl_objs_to_hash(SV *source, swig_type_info *tinfo,
                                     apr_pool_t *pool);
apr_hash_t *svn_swig_pl_objs_to_hash_by_name(SV *source,
                                             const char *typename,
                                             apr_pool_t *pool);
apr_hash_t *svn_swig_pl_objs_to_hash_of_revnum_t(SV *source,
                                                 apr_pool_t *pool);
const apr_array_header_t *svn_swig_pl_objs_to_array(SV *source,
                                                    swig_type_info *tinfo,
                                                    apr_pool_t *pool);

SV *svn_swig_pl_array_to_list(const apr_array_header_t *array);
/* Formerly used by pre-1.0 APIs. Now unused
SV *svn_swig_pl_ints_to_list(const apr_array_header_t *array);
*/
SV *svn_swig_pl_convert_array(const apr_array_header_t *array,
                              swig_type_info *tinfo);

/* thunked log receiver function.  */
svn_error_t * svn_swig_pl_thunk_log_receiver(void *py_receiver,
                                             apr_hash_t *changed_paths,
                                             svn_revnum_t rev,
                                             const char *author,
                                             const char *date,
                                             const char *msg,
                                             apr_pool_t *pool);

/* thunked diff summarize callback.  */
svn_error_t * svn_swig_pl_thunk_client_diff_summarize_func(
                     const svn_client_diff_summarize_t *diff,
                     void *baton,
                     apr_pool_t *pool);

/* thunked commit editor callback. */
svn_error_t *svn_swig_pl_thunk_commit_callback(svn_revnum_t new_revision,
					       const char *date,
					       const char *author,
					       void *baton);

/* thunked repos_history callback. */
svn_error_t *svn_swig_pl_thunk_history_func(void *baton,
                                            const char *path,
                                            svn_revnum_t revision,
                                            apr_pool_t *pool);

/* thunked dir_delta authz read function. */
svn_error_t *svn_swig_pl_thunk_authz_func(svn_boolean_t *allowed,
                                          svn_fs_root_t *root,
                                          const char *path,
                                          void *baton,
                                          apr_pool_t *pool);

/* ra callbacks. */
svn_error_t *svn_ra_make_callbacks(svn_ra_callbacks_t **cb,
				   void **c_baton,
				   SV *perl_callbacks,
				   apr_pool_t *pool);

/* thunked simple_prompt callback function */
svn_error_t *svn_swig_pl_thunk_simple_prompt(svn_auth_cred_simple_t **cred,
                                             void *baton,
                                             const char *realm,
                                             const char *username,
                                             svn_boolean_t may_save,
                                             apr_pool_t *pool);

/* thunked username_prompt callback function */
svn_error_t *svn_swig_pl_thunk_username_prompt(svn_auth_cred_username_t **cred,
                                               void *baton,
                                               const char *realm,
                                               svn_boolean_t may_save,
                                               apr_pool_t *pool);

/* thunked ssl_server_trust_prompt callback function */
svn_error_t *svn_swig_pl_thunk_ssl_server_trust_prompt
  (svn_auth_cred_ssl_server_trust_t **cred,
   void *baton,
   const char *realm,
   apr_uint32_t failures,
   const svn_auth_ssl_server_cert_info_t *cert_info,
   svn_boolean_t may_save,
   apr_pool_t *pool);

/* thunked ssl_client_cert callback function */
svn_error_t *svn_swig_pl_thunk_ssl_client_cert_prompt
  (svn_auth_cred_ssl_client_cert_t **cred,
   void *baton,
   const char *realm,
   svn_boolean_t may_save,
   apr_pool_t *pool);

/* thunked ssl_client_cert_pw callback function */
svn_error_t *svn_swig_pl_thunk_ssl_client_cert_pw_prompt
  (svn_auth_cred_ssl_client_cert_pw_t **cred,
   void *baton,
   const char *realm,
   svn_boolean_t may_save,
   apr_pool_t *pool);

/* thunked callback for svn_ra_get_wc_prop_func_t */
svn_error_t *thunk_get_wc_prop(void *baton,
                               const char *relpath,
                               const char *name,
                               const svn_string_t **value,
                               apr_pool_t *pool);

/* Thunked version of svn_wc_notify_func_t callback type */
void svn_swig_pl_notify_func(void * baton,
                             const char *path,
		             svn_wc_notify_action_t action,
			     svn_node_kind_t kind,
			     const char *mime_type,
			     svn_wc_notify_state_t content_state,
			     svn_wc_notify_state_t prop_state,
			     svn_revnum_t revision);


/* Thunked version of svn_client_get_commit_log3_t callback type. */
svn_error_t *svn_swig_pl_get_commit_log_func(const char **log_msg,
                                             const char **tmp_file,
                                             const apr_array_header_t *
                                             commit_items,
                                             void *baton,
                                             apr_pool_t *pool);

/* Thunked version of svn_client_info_t callback type. */
svn_error_t *svn_swig_pl_info_receiver(void *baton,
                                       const char *path,
                                       const svn_info_t *info,
                                       apr_pool_t *pool);

/* Thunked version of svn_wc_cancel_func_t callback type. */
svn_error_t *svn_swig_pl_cancel_func(void *cancel_baton);

/* Thunked version of svn_wc_status_func_t callback type. */
void svn_swig_pl_status_func(void *baton,
                             const char *path,
                             svn_wc_status_t *status);
/* Thunked version of svn_client_blame_receiver_t callback type. */
svn_error_t *svn_swig_pl_blame_func(void *baton,
                                    apr_int64_t line_no,
                                    svn_revnum_t revision,
                                    const char *author,
                                    const char *date,
                                    const char *line,
                                    apr_pool_t *pool);

/* Thunked config enumerator */
svn_boolean_t svn_swig_pl_thunk_config_enumerator(const char *name, const char *value, void *baton);

/* helper for making the editor */
void svn_delta_make_editor(svn_delta_editor_t **editor,
                           void **edit_baton,
                           SV *perl_editor,
                           apr_pool_t *pool);

void svn_delta_wrap_window_handler(svn_txdelta_window_handler_t *handler,
                                   void **h_baton,
                                   SV *callback,
                                   apr_pool_t *pool);

/* svn_stream_t helpers */
svn_error_t *svn_swig_pl_make_stream(svn_stream_t **stream, SV *obj);
SV *svn_swig_pl_from_stream(svn_stream_t *stream);

/* apr_file_t * */
apr_file_t *svn_swig_pl_make_file(SV *file, apr_pool_t *pool);

void svn_swig_pl_hold_ref_in_pool(apr_pool_t *pool, SV *sv);

/* md5 access class */
SV *svn_swig_pl_from_md5(unsigned char *digest);

svn_error_t *svn_swig_pl_ra_lock_callback(void *baton,
                                          const char *path,
                                          svn_boolean_t do_lock,
                                          const svn_lock_t *lock,
                                          svn_error_t *ra_err,
                                          apr_pool_t *pool);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* SVN_SWIG_SWIGUTIL_PL_H */

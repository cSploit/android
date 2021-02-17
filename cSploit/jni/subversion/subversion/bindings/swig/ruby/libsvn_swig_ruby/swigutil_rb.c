/*
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

/* -*- c-file-style: "ruby" -*- */
/* Tell swigutil_rb.h that we're inside the implementation */
#define SVN_SWIG_SWIGUTIL_RB_C

#include "swig_ruby_external_runtime.swg"
#include "swigutil_rb.h"
#include <st.h>

#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef _

#include "svn_private_config.h"

#ifndef RE_OPTION_IGNORECASE
#  ifdef ONIG_OPTION_IGNORECASE
#    define RE_OPTION_IGNORECASE ONIG_OPTION_IGNORECASE
#  endif
#endif

#ifndef RSTRING_LEN
#  define RSTRING_LEN(str) (RSTRING(str)->len)
#endif

#ifndef RSTRING_PTR
#  define RSTRING_PTR(str) (RSTRING(str)->ptr)
#endif

#include <locale.h>
#include <math.h>

#include "svn_nls.h"
#include "svn_pools.h"
#include "svn_props.h"
#include "svn_time.h"
#include "svn_utf.h"


#if APR_HAS_LARGE_FILES
#  define AOFF2NUM(num) LL2NUM(num)
#else
#  define AOFF2NUM(num) LONG2NUM(num)
#endif

#if SIZEOF_LONG_LONG == 8
#  define AI642NUM(num) LL2NUM(num)
#else
#  define AI642NUM(num) LONG2NUM(num)
#endif

#define EMPTY_CPP_ARGUMENT

#define POOL_P(obj) (RTEST(rb_obj_is_kind_of(obj, rb_svn_core_pool())))
#define CONTEXT_P(obj) (RTEST(rb_obj_is_kind_of(obj, rb_svn_client_context())))
#define SVN_ERR_P(obj) (RTEST(rb_obj_is_kind_of(obj, rb_svn_error())))

static VALUE mSvn = Qnil;
static VALUE mSvnClient = Qnil;
static VALUE mSvnUtil = Qnil;
static VALUE cSvnClientContext = Qnil;
static VALUE mSvnCore = Qnil;
static VALUE cSvnCorePool = Qnil;
static VALUE cSvnCoreStream = Qnil;
static VALUE cSvnDelta = Qnil;
static VALUE cSvnDeltaEditor = Qnil;
static VALUE cSvnDeltaTextDeltaWindowHandler = Qnil;
static VALUE cSvnError = Qnil;
static VALUE cSvnErrorSvnError = Qnil;
static VALUE cSvnFs = Qnil;
static VALUE cSvnFsFileSystem = Qnil;
static VALUE cSvnRa = Qnil;
static VALUE cSvnRaReporter3 = Qnil;

static apr_pool_t *swig_rb_pool;
static apr_allocator_t *swig_rb_allocator;

#define DECLARE_ID(key) static ID id_ ## key
#define DEFINE_ID(key) DEFINE_ID_WITH_NAME(key, #key)
#define DEFINE_ID_WITH_NAME(key, name) id_ ## key = rb_intern(name)

DECLARE_ID(code);
DECLARE_ID(message);
DECLARE_ID(call);
DECLARE_ID(read);
DECLARE_ID(write);
DECLARE_ID(eqq);
DECLARE_ID(baton);
DECLARE_ID(new);
DECLARE_ID(new_corresponding_error);
DECLARE_ID(set_target_revision);
DECLARE_ID(open_root);
DECLARE_ID(delete_entry);
DECLARE_ID(add_directory);
DECLARE_ID(open_directory);
DECLARE_ID(change_dir_prop);
DECLARE_ID(close_directory);
DECLARE_ID(absent_directory);
DECLARE_ID(add_file);
DECLARE_ID(open_file);
DECLARE_ID(apply_textdelta);
DECLARE_ID(change_file_prop);
DECLARE_ID(absent_file);
DECLARE_ID(close_file);
DECLARE_ID(close_edit);
DECLARE_ID(abort_edit);
DECLARE_ID(__pool__);
DECLARE_ID(__pools__);
DECLARE_ID(name);
DECLARE_ID(value);
DECLARE_ID(swig_type_regex);
DECLARE_ID(open_tmp_file);
DECLARE_ID(get_wc_prop);
DECLARE_ID(set_wc_prop);
DECLARE_ID(push_wc_prop);
DECLARE_ID(invalidate_wc_props);
DECLARE_ID(progress_func);
DECLARE_ID(auth_baton);
DECLARE_ID(found_entry);
DECLARE_ID(file_changed);
DECLARE_ID(file_added);
DECLARE_ID(file_deleted);
DECLARE_ID(dir_added);
DECLARE_ID(dir_deleted);
DECLARE_ID(dir_props_changed);
DECLARE_ID(handler);
DECLARE_ID(handler_baton);
DECLARE_ID(__batons__);
DECLARE_ID(destroy);
DECLARE_ID(filename_to_temp_file);
DECLARE_ID(inspect);
DECLARE_ID(handle_error);
DECLARE_ID(set_path);
DECLARE_ID(delete_path);
DECLARE_ID(link_path);
DECLARE_ID(finish_report);
DECLARE_ID(abort_report);
DECLARE_ID(to_s);
DECLARE_ID(upcase);


typedef void *(*r2c_func)(VALUE value, void *ctx, apr_pool_t *pool);
typedef VALUE (*c2r_func)(void *value, void *ctx);
typedef struct hash_to_apr_hash_data_t
{
  apr_hash_t *apr_hash;
  r2c_func func;
  void *ctx;
  apr_pool_t *pool;
} hash_to_apr_hash_data_t;

static void r2c_swig_type2(VALUE value, const char *type_name, void **result);
static const char *r2c_inspect(VALUE object);



/* constant getter */
static VALUE
rb_svn(void)
{
  if (NIL_P(mSvn)) {
    mSvn = rb_const_get(rb_cObject, rb_intern("Svn"));
  }
  return mSvn;
}

static VALUE
rb_svn_util(void)
{
  if (NIL_P(mSvnUtil)) {
    mSvnUtil = rb_const_get(rb_svn(), rb_intern("Util"));
  }
  return mSvnUtil;
}

static VALUE
rb_svn_client(void)
{
  if (NIL_P(mSvnClient)) {
    mSvnClient = rb_const_get(rb_svn(), rb_intern("Client"));
  }
  return mSvnClient;
}

static VALUE
rb_svn_client_context(void)
{
  if (NIL_P(cSvnClientContext)) {
    cSvnClientContext = rb_const_get(rb_svn_client(), rb_intern("Context"));
  }
  return cSvnClientContext;
}

static VALUE
rb_svn_core(void)
{
  if (NIL_P(mSvnCore)) {
    mSvnCore = rb_const_get(rb_svn(), rb_intern("Core"));
  }
  return mSvnCore;
}

static VALUE
rb_svn_core_pool(void)
{
  if (NIL_P(cSvnCorePool)) {
    cSvnCorePool = rb_const_get(rb_svn_core(), rb_intern("Pool"));
    rb_ivar_set(cSvnCorePool, id___pools__, rb_hash_new());
  }
  return cSvnCorePool;
}

static VALUE
rb_svn_core_stream(void)
{
  if (NIL_P(cSvnCoreStream)) {
    cSvnCoreStream = rb_const_get(rb_svn_core(), rb_intern("Stream"));
  }
  return cSvnCoreStream;
}

static VALUE
rb_svn_delta(void)
{
  if (NIL_P(cSvnDelta)) {
    cSvnDelta = rb_const_get(rb_svn(), rb_intern("Delta"));
  }
  return cSvnDelta;
}

VALUE
svn_swig_rb_svn_delta_editor(void)
{
  if (NIL_P(cSvnDeltaEditor)) {
    cSvnDeltaEditor =
      rb_const_get(rb_svn_delta(), rb_intern("Editor"));
  }
  return cSvnDeltaEditor;
}

VALUE
svn_swig_rb_svn_delta_text_delta_window_handler(void)
{
  if (NIL_P(cSvnDeltaTextDeltaWindowHandler)) {
    cSvnDeltaTextDeltaWindowHandler =
      rb_const_get(rb_svn_delta(), rb_intern("TextDeltaWindowHandler"));
  }
  return cSvnDeltaTextDeltaWindowHandler;
}

static VALUE
rb_svn_error(void)
{
  if (NIL_P(cSvnError)) {
    cSvnError = rb_const_get(rb_svn(), rb_intern("Error"));
  }
  return cSvnError;
}

static VALUE
rb_svn_error_svn_error(void)
{
  if (NIL_P(cSvnErrorSvnError)) {
    cSvnErrorSvnError = rb_const_get(rb_svn_error(), rb_intern("SvnError"));
  }
  return cSvnErrorSvnError;
}

static VALUE
rb_svn_fs(void)
{
  if (NIL_P(cSvnFs)) {
    cSvnFs = rb_const_get(rb_svn(), rb_intern("Fs"));
  }
  return cSvnFs;
}

static VALUE
rb_svn_fs_file_system(void)
{
  if (NIL_P(cSvnFsFileSystem)) {
    cSvnFsFileSystem = rb_const_get(rb_svn_fs(), rb_intern("FileSystem"));
    rb_ivar_set(cSvnFsFileSystem, id___batons__, rb_hash_new());
  }
  return cSvnFsFileSystem;
}

static VALUE
rb_svn_ra(void)
{
  if (NIL_P(cSvnRa)) {
    cSvnRa = rb_const_get(rb_svn(), rb_intern("Ra"));
  }
  return cSvnRa;
}

static VALUE
rb_svn_ra_reporter3(void)
{
  if (NIL_P(cSvnRaReporter3)) {
    cSvnRaReporter3 = rb_const_get(rb_svn_ra(), rb_intern("Reporter3"));
  }
  return cSvnRaReporter3;
}


/* constant resolver */
static VALUE
resolve_constant(VALUE parent, const char *prefix, VALUE name)
{
    VALUE const_name;

    const_name = rb_str_new2(prefix);
    rb_str_concat(const_name,
                  rb_funcall(rb_funcall(name, id_to_s, 0),
                             id_upcase, 0));
    return rb_const_get(parent, rb_intern(StringValuePtr(const_name)));
}


/* initialize */
static VALUE
svn_swig_rb_converter_to_locale_encoding(VALUE self, VALUE str)
{
  apr_pool_t *pool;
  svn_error_t *err;
  const char *dest;
  VALUE result;

  pool = svn_pool_create(NULL);
  err = svn_utf_cstring_from_utf8(&dest, StringValueCStr(str), pool);
  if (err) {
    svn_pool_destroy(pool);
    svn_swig_rb_handle_svn_error(err);
  }

  result = rb_str_new2(dest);
  svn_pool_destroy(pool);
  return result;
}

static VALUE
svn_swig_rb_locale_set(int argc, VALUE *argv, VALUE self)
{
  char *result;
  int category;
  const char *locale;
  VALUE rb_category, rb_locale;

  rb_scan_args(argc, argv, "02", &rb_category, &rb_locale);

  if (NIL_P(rb_category))
    category = LC_ALL;
  else
    category = NUM2INT(rb_category);

  if (NIL_P(rb_locale))
    locale = "";
  else
    locale = StringValueCStr(rb_locale);

  result = setlocale(category, locale);

  return result ? rb_str_new2(result) : Qnil;
}

static VALUE
svn_swig_rb_gettext_bindtextdomain(VALUE self, VALUE path)
{
#ifdef ENABLE_NLS
  bindtextdomain(PACKAGE_NAME, StringValueCStr(path));
#endif
  return Qnil;
}

static VALUE
svn_swig_rb_gettext__(VALUE self, VALUE message)
{
#ifdef ENABLE_NLS
  return rb_str_new2(_(StringValueCStr(message)));
#else
  return message;
#endif
}

static void
svn_swig_rb_initialize_ids(void)
{
  DEFINE_ID(code);
  DEFINE_ID(message);
  DEFINE_ID(call);
  DEFINE_ID(read);
  DEFINE_ID(write);
  DEFINE_ID_WITH_NAME(eqq, "===");
  DEFINE_ID(baton);
  DEFINE_ID(new);
  DEFINE_ID(new_corresponding_error);
  DEFINE_ID(set_target_revision);
  DEFINE_ID(open_root);
  DEFINE_ID(delete_entry);
  DEFINE_ID(add_directory);
  DEFINE_ID(open_directory);
  DEFINE_ID(change_dir_prop);
  DEFINE_ID(close_directory);
  DEFINE_ID(absent_directory);
  DEFINE_ID(add_file);
  DEFINE_ID(open_file);
  DEFINE_ID(apply_textdelta);
  DEFINE_ID(change_file_prop);
  DEFINE_ID(absent_file);
  DEFINE_ID(close_file);
  DEFINE_ID(close_edit);
  DEFINE_ID(abort_edit);
  DEFINE_ID(__pool__);
  DEFINE_ID(__pools__);
  DEFINE_ID(name);
  DEFINE_ID(value);
  DEFINE_ID(swig_type_regex);
  DEFINE_ID(open_tmp_file);
  DEFINE_ID(get_wc_prop);
  DEFINE_ID(set_wc_prop);
  DEFINE_ID(push_wc_prop);
  DEFINE_ID(invalidate_wc_props);
  DEFINE_ID(progress_func);
  DEFINE_ID(auth_baton);
  DEFINE_ID(found_entry);
  DEFINE_ID(file_changed);
  DEFINE_ID(file_added);
  DEFINE_ID(file_deleted);
  DEFINE_ID(dir_added);
  DEFINE_ID(dir_deleted);
  DEFINE_ID(dir_props_changed);
  DEFINE_ID(handler);
  DEFINE_ID(handler_baton);
  DEFINE_ID(__batons__);
  DEFINE_ID(destroy);
  DEFINE_ID(filename_to_temp_file);
  DEFINE_ID(inspect);
  DEFINE_ID(handle_error);
  DEFINE_ID(set_path);
  DEFINE_ID(delete_path);
  DEFINE_ID(link_path);
  DEFINE_ID(finish_report);
  DEFINE_ID(abort_report);
  DEFINE_ID(to_s);
  DEFINE_ID(upcase);
}

static void
check_apr_status(apr_status_t status, VALUE exception_class, const char *format)
{
    if (status != APR_SUCCESS) {
	char buffer[1024];
	apr_strerror(status, buffer, sizeof(buffer) - 1);
	rb_raise(exception_class, format, buffer);
    }
}

static VALUE swig_type_re = Qnil;

static VALUE
swig_type_regex(void)
{
  if (NIL_P(swig_type_re)) {
    char reg_str[] = "\\A(?:SWIG|Svn::Ext)::";
    swig_type_re = rb_reg_new(reg_str, strlen(reg_str), 0);
    rb_ivar_set(rb_svn(), id_swig_type_regex, swig_type_re);
  }
  return swig_type_re;
}

static VALUE
find_swig_type_object(int num, VALUE *objects)
{
  VALUE re;
  int i;

  re = swig_type_regex();
  for (i = 0; i < num; i++) {
    if (RTEST(rb_reg_match(re,
                           rb_funcall(rb_obj_class(objects[i]),
                                      id_name,
                                      0)))) {
      return objects[i];
    }
  }

  return Qnil;
}

static VALUE
svn_swig_rb_destroyer_destroy(VALUE self, VALUE target)
{
    VALUE objects[1];

    objects[0] = target;
    if (find_swig_type_object(1, objects) && DATA_PTR(target)) {
	svn_swig_rb_destroy_internal_pool(target);
	DATA_PTR(target) = NULL;
    }

    return Qnil;
}

void
svn_swig_rb_initialize(void)
{
  VALUE mSvnConverter, mSvnLocale, mSvnGetText, mSvnDestroyer;

  check_apr_status(apr_initialize(), rb_eLoadError, "cannot initialize APR: %s");

  if (atexit(apr_terminate)) {
    rb_raise(rb_eLoadError, "atexit registration failed");
  }

  check_apr_status(apr_allocator_create(&swig_rb_allocator),
		   rb_eLoadError, "failed to create allocator: %s");
  apr_allocator_max_free_set(swig_rb_allocator,
			     SVN_ALLOCATOR_RECOMMENDED_MAX_FREE);

  swig_rb_pool = svn_pool_create_ex(NULL, swig_rb_allocator);
  apr_pool_tag(swig_rb_pool, "svn-ruby-pool");
#if APR_HAS_THREADS
  {
    apr_thread_mutex_t *mutex;

    check_apr_status(apr_thread_mutex_create(&mutex, APR_THREAD_MUTEX_DEFAULT,
					     swig_rb_pool),
		     rb_eLoadError, "failed to create allocator: %s");
    apr_allocator_mutex_set(swig_rb_allocator, mutex);
  }
#endif
  apr_allocator_owner_set(swig_rb_allocator, swig_rb_pool);

  svn_utf_initialize(swig_rb_pool);

  svn_swig_rb_initialize_ids();

  mSvnConverter = rb_define_module_under(rb_svn(), "Converter");
  rb_define_module_function(mSvnConverter, "to_locale_encoding",
                            svn_swig_rb_converter_to_locale_encoding, 1);

  mSvnLocale = rb_define_module_under(rb_svn(), "Locale");
  rb_define_const(mSvnLocale, "ALL", INT2NUM(LC_ALL));
  rb_define_const(mSvnLocale, "COLLATE", INT2NUM(LC_COLLATE));
  rb_define_const(mSvnLocale, "CTYPE", INT2NUM(LC_CTYPE));
#ifdef LC_MESSAGES
  rb_define_const(mSvnLocale, "MESSAGES", INT2NUM(LC_MESSAGES));
#endif
  rb_define_const(mSvnLocale, "MONETARY", INT2NUM(LC_MONETARY));
  rb_define_const(mSvnLocale, "NUMERIC", INT2NUM(LC_NUMERIC));
  rb_define_const(mSvnLocale, "TIME", INT2NUM(LC_TIME));
  rb_define_module_function(mSvnLocale, "set", svn_swig_rb_locale_set, -1);

  mSvnGetText = rb_define_module_under(rb_svn(), "GetText");
  rb_define_module_function(mSvnGetText, "bindtextdomain",
                            svn_swig_rb_gettext_bindtextdomain, 1);
  rb_define_module_function(mSvnGetText, "_", svn_swig_rb_gettext__, 1);

  mSvnDestroyer = rb_define_module_under(rb_svn(), "Destroyer");
  rb_define_module_function(mSvnDestroyer, "destroy",
			    svn_swig_rb_destroyer_destroy, 1);
}

apr_pool_t *
svn_swig_rb_pool(void)
{
    return swig_rb_pool;
}

apr_allocator_t *
svn_swig_rb_allocator(void)
{
    return swig_rb_allocator;
}


/* pool holder */
static VALUE
rb_svn_pool_holder(void)
{
  return rb_ivar_get(rb_svn_core_pool(), id___pools__);
}

static VALUE
rb_svn_fs_warning_callback_baton_holder(void)
{
  return rb_ivar_get(rb_svn_fs_file_system(), id___batons__);
}

static VALUE
rb_holder_push(VALUE holder, VALUE obj)
{
  VALUE key, objs;

  key = rb_obj_id(obj);
  objs = rb_hash_aref(holder, key);

  if (NIL_P(objs)) {
    objs = rb_ary_new();
    rb_hash_aset(holder, key, objs);
  }

  rb_ary_push(objs, obj);

  return Qnil;
}

static VALUE
rb_holder_pop(VALUE holder, VALUE obj)
{
  VALUE key, objs;
  VALUE result = Qnil;

  key = rb_obj_id(obj);
  objs = rb_hash_aref(holder, key);

  if (!NIL_P(objs)) {
    result = rb_ary_pop(objs);
    if (RARRAY_LEN(objs) == 0) {
      rb_hash_delete(holder, key);
    }
  }

  return result;
}


/* pool */
static VALUE
rb_get_pool(VALUE self)
{
  return rb_ivar_get(self, id___pool__);
}

static VALUE
rb_pools(VALUE self)
{
  VALUE pools = rb_ivar_get(self, id___pools__);

  if (NIL_P(pools)) {
    pools = rb_hash_new();
    rb_ivar_set(self, id___pools__, pools);
  }

  return pools;
}

static VALUE
rb_set_pool(VALUE self, VALUE pool)
{
  if (NIL_P(pool)) {
    VALUE old_pool = rb_ivar_get(self, id___pool__);
    rb_hash_aset(rb_pools(self), rb_obj_id(old_pool), old_pool);
    rb_ivar_set(self, id___pool__, Qnil);
  } else {
    if (NIL_P(rb_ivar_get(self, id___pool__))) {
      rb_ivar_set(self, id___pool__, pool);
    } else {
      rb_hash_aset(rb_pools(self), rb_obj_id(pool), pool);
    }
  }

  return Qnil;
}

static VALUE
rb_pool_new(VALUE parent)
{
  return rb_funcall(rb_svn_core_pool(), id_new, 1, parent);
}

void
svn_swig_rb_get_pool(int argc, VALUE *argv, VALUE self,
                     VALUE *rb_pool, apr_pool_t **pool)
{
  *rb_pool = Qnil;

  if (argc > 0) {
    if (POOL_P(argv[argc - 1])) {
      *rb_pool = rb_pool_new(argv[argc - 1]);
      argc -= 1;
    }
  }

  if (NIL_P(*rb_pool) && !NIL_P(self)) {
    *rb_pool = rb_get_pool(self);
    if (POOL_P(*rb_pool)) {
      *rb_pool = rb_pool_new(*rb_pool);
    } else {
      *rb_pool = Qnil;
    }
  }

  if (NIL_P(*rb_pool)) {
    VALUE target;
    target = find_swig_type_object(argc, argv);
    *rb_pool = rb_pool_new(rb_get_pool(target));
  }

  if (pool) {
    apr_pool_wrapper_t *pool_wrapper;
    apr_pool_wrapper_t **pool_wrapper_p;

    pool_wrapper_p = &pool_wrapper;
    r2c_swig_type2(*rb_pool, "apr_pool_wrapper_t *", (void **)pool_wrapper_p);
    *pool = pool_wrapper->pool;
  }
}

static svn_boolean_t
rb_set_pool_if_swig_type_object(VALUE target, VALUE pool)
{
  VALUE targets[1] = {target};

  if (!NIL_P(find_swig_type_object(1, targets))) {
    rb_set_pool(target, pool);
    return TRUE;
  } else {
    return FALSE;
  }
}

struct rb_set_pool_for_hash_arg {
  svn_boolean_t set;
  VALUE pool;
};

static int
rb_set_pool_for_hash_callback(VALUE key, VALUE value,
                              struct rb_set_pool_for_hash_arg *arg)
{
  if (svn_swig_rb_set_pool(value, arg->pool))
    arg->set = TRUE;
  return ST_CONTINUE;
}

svn_boolean_t
svn_swig_rb_set_pool(VALUE target, VALUE pool)
{
  if (NIL_P(target)) {
    return FALSE;
  }

  if (RTEST(rb_obj_is_kind_of(target, rb_cArray))) {
    long i;
    svn_boolean_t set = FALSE;

    for (i = 0; i < RARRAY_LEN(target); i++) {
      if (svn_swig_rb_set_pool(RARRAY_PTR(target)[i], pool))
        set = TRUE;
    }
    return set;
  } else if (RTEST(rb_obj_is_kind_of(target, rb_cHash))) {
    struct rb_set_pool_for_hash_arg arg;
    arg.set = FALSE;
    arg.pool = pool;
    rb_hash_foreach(target, rb_set_pool_for_hash_callback, (VALUE)&arg);
    return arg.set;
  } else {
    return rb_set_pool_if_swig_type_object(target, pool);
  }
}

void
svn_swig_rb_set_pool_for_no_swig_type(VALUE target, VALUE pool)
{
  if (NIL_P(target)) {
    return;
  }

  if (!RTEST(rb_obj_is_kind_of(target, rb_cArray))) {
    target = rb_ary_new3(1, target);
  }

  rb_iterate(rb_each, target, rb_set_pool, pool);
}

void
svn_swig_rb_push_pool(VALUE pool)
{
  if (!NIL_P(pool)) {
    rb_holder_push(rb_svn_pool_holder(), pool);
  }
}

void
svn_swig_rb_pop_pool(VALUE pool)
{
  if (!NIL_P(pool)) {
    rb_holder_pop(rb_svn_pool_holder(), pool);
  }
}

void
svn_swig_rb_destroy_pool(VALUE pool)
{
  if (!NIL_P(pool)) {
    rb_funcall(pool, id_destroy, 0);
  }
}

void
svn_swig_rb_destroy_internal_pool(VALUE object)
{
  svn_swig_rb_destroy_pool(rb_get_pool(object));
}


/* error */
void
svn_swig_rb_raise_svn_fs_already_close(void)
{
  static VALUE rb_svn_error_fs_already_close = 0;

  if (!rb_svn_error_fs_already_close) {
    rb_svn_error_fs_already_close =
      rb_const_get(rb_svn_error(), rb_intern("FsAlreadyClose"));
  }

  rb_raise(rb_svn_error_fs_already_close, "closed file system");
}

void
svn_swig_rb_raise_svn_repos_already_close(void)
{
  static VALUE rb_svn_error_repos_already_close = 0;

  if (!rb_svn_error_repos_already_close) {
    rb_svn_error_repos_already_close =
      rb_const_get(rb_svn_error(), rb_intern("ReposAlreadyClose"));
  }

  rb_raise(rb_svn_error_repos_already_close, "closed repository");
}

VALUE
svn_swig_rb_svn_error_new(VALUE code, VALUE message, VALUE file, VALUE line,
			  VALUE child)
{
  return rb_funcall(rb_svn_error_svn_error(),
                    id_new_corresponding_error,
                    5, code, message, file, line, child);
}

VALUE
svn_swig_rb_svn_error_to_rb_error(svn_error_t *error)
{
  VALUE error_code = INT2NUM(error->apr_err);
  VALUE message;
  VALUE file = Qnil;
  VALUE line = Qnil;
  VALUE child = Qnil;

  if (error->file)
    file = rb_str_new2(error->file);
  if (error->line)
    line = LONG2NUM(error->line);

  message = rb_str_new2(error->message ? error->message : "");

  if (error->child)
      child = svn_swig_rb_svn_error_to_rb_error(error->child);

  return svn_swig_rb_svn_error_new(error_code, message, file, line, child);
}

void
svn_swig_rb_handle_svn_error(svn_error_t *error)
{
  VALUE rb_error = svn_swig_rb_svn_error_to_rb_error(error);
  svn_error_clear(error);
  rb_exc_raise(rb_error);
}


static VALUE inited = Qnil;
/* C -> Ruby */
VALUE
svn_swig_rb_from_swig_type(void *value, void *ctx)
{
  swig_type_info *info;

  if (NIL_P(inited)) {
    SWIG_InitRuntime();
    inited = Qtrue;
  }

  info = SWIG_TypeQuery((char *)ctx);
  if (info) {
    return SWIG_NewPointerObj(value, info, 0);
  } else {
    rb_raise(rb_eArgError, "invalid SWIG type: %s", (char *)ctx);
  }
}
#define c2r_swig_type svn_swig_rb_from_swig_type

svn_depth_t
svn_swig_rb_to_depth(VALUE value)
{
  if (NIL_P(value)) {
    return svn_depth_infinity;
  } else if (value == Qtrue) {
    return SVN_DEPTH_INFINITY_OR_FILES(TRUE);
  } else if (value == Qfalse) {
    return SVN_DEPTH_INFINITY_OR_FILES(FALSE);
  } else if (RTEST(rb_obj_is_kind_of(value, rb_cString)) ||
             RTEST(rb_obj_is_kind_of(value, rb_cSymbol))) {
    value = rb_funcall(value, id_to_s, 0);
    return svn_depth_from_word(StringValueCStr(value));
  } else if (RTEST(rb_obj_is_kind_of(value, rb_cInteger))) {
    return NUM2INT(value);
  } else {
    rb_raise(rb_eArgError,
             "'%s' must be DEPTH_STRING (e.g. \"infinity\" or :infinity) "
             "or Svn::Core::DEPTH_*",
             r2c_inspect(value));
  }
}

svn_mergeinfo_inheritance_t
svn_swig_rb_to_mergeinfo_inheritance(VALUE value)
{
  if (NIL_P(value)) {
    return svn_mergeinfo_inherited;
  } else if (RTEST(rb_obj_is_kind_of(value, rb_cString)) ||
             RTEST(rb_obj_is_kind_of(value, rb_cSymbol))) {
    value = rb_funcall(value, id_to_s, 0);
    return svn_inheritance_from_word(StringValueCStr(value));
  } else if (RTEST(rb_obj_is_kind_of(value, rb_cInteger))) {
    return NUM2INT(value);
  } else {
    rb_raise(rb_eArgError,
       "'%s' must be MERGEINFO_STRING (e.g. \"explicit\" or :explicit) "
       "or Svn::Core::MERGEINFO_*",
       r2c_inspect(value));
  }
}

static VALUE
c2r_string(void *value, void *ctx)
{
  if (value) {
    return rb_str_new2((const char *)value);
  } else {
    return Qnil;
  }
}

static VALUE
c2r_string2(const char *cstr)
{
  return c2r_string((void *)cstr, NULL);
}

#define c2r_bool2(bool) (bool ? Qtrue : Qfalse)

VALUE
svn_swig_rb_svn_date_string_to_time(const char *date)
{
  if (date) {
    apr_time_t tm;
    svn_error_t *error;
    apr_pool_t *pool;

    pool = svn_pool_create(NULL);
    error = svn_time_from_cstring(&tm, date, pool);
    svn_pool_destroy(pool);
    if (error)
      svn_swig_rb_handle_svn_error(error);
    return rb_time_new((time_t)apr_time_sec(tm), (time_t)apr_time_usec(tm));
  } else {
    return Qnil;
  }
}
#define c2r_svn_date_string2 svn_swig_rb_svn_date_string_to_time

static VALUE
c2r_long(void *value, void *ctx)
{
  return INT2NUM(*(long *)value);
}

static VALUE
c2r_svn_string(void *value, void *ctx)
{
  const svn_string_t *s = (svn_string_t *)value;

  return c2r_string2(s->data);
}

typedef struct prop_hash_each_arg_t {
  apr_array_header_t *array;
  apr_pool_t *pool;
} prop_hash_each_arg_t;

static int
svn_swig_rb_to_apr_array_row_prop_callback(VALUE key, VALUE value,
                                           prop_hash_each_arg_t *arg)
{
  svn_prop_t *prop;

  prop = apr_array_push(arg->array);
  prop->name = apr_pstrdup(arg->pool, StringValueCStr(key));
  prop->value = svn_string_ncreate(RSTRING_PTR(value), RSTRING_LEN(value),
                                   arg->pool);
  return ST_CONTINUE;
}

apr_array_header_t *
svn_swig_rb_to_apr_array_row_prop(VALUE array_or_hash, apr_pool_t *pool)
{
  if (RTEST(rb_obj_is_kind_of(array_or_hash, rb_cArray))) {
    int i, len;
    apr_array_header_t *result;

    len = RARRAY_LEN(array_or_hash);
    result = apr_array_make(pool, len, sizeof(svn_prop_t));
    result->nelts = len;
    for (i = 0; i < len; i++) {
      VALUE name, value, item;
      svn_prop_t *prop;

      item = rb_ary_entry(array_or_hash, i);
      name = rb_funcall(item, id_name, 0);
      value = rb_funcall(item, id_value, 0);
      prop = &APR_ARRAY_IDX(result, i, svn_prop_t);
      prop->name = apr_pstrdup(pool, StringValueCStr(name));
      prop->value = svn_string_ncreate(RSTRING_PTR(value), RSTRING_LEN(value),
                                       pool);
    }
    return result;
  } else if (RTEST(rb_obj_is_kind_of(array_or_hash, rb_cHash))) {
    apr_array_header_t *result;
    prop_hash_each_arg_t arg;

    result = apr_array_make(pool, 0, sizeof(svn_prop_t));
    arg.array = result;
    arg.pool = pool;
    rb_hash_foreach(array_or_hash, svn_swig_rb_to_apr_array_row_prop_callback,
                    (VALUE)&arg);
    return result;
  } else {
    rb_raise(rb_eArgError,
             "'%s' must be [Svn::Core::Prop, ...] or {'name' => 'value', ...}",
             r2c_inspect(array_or_hash));
  }
}

static int
svn_swig_rb_to_apr_array_prop_callback(VALUE key, VALUE value,
                                       prop_hash_each_arg_t *arg)
{
  svn_prop_t *prop;

  prop = apr_palloc(arg->pool, sizeof(svn_prop_t));
  prop->name = apr_pstrdup(arg->pool, StringValueCStr(key));
  prop->value = svn_string_ncreate(RSTRING_PTR(value), RSTRING_LEN(value),
                                   arg->pool);
  APR_ARRAY_PUSH(arg->array, svn_prop_t *) = prop;
  return ST_CONTINUE;
}

apr_array_header_t *
svn_swig_rb_to_apr_array_prop(VALUE array_or_hash, apr_pool_t *pool)
{
  if (RTEST(rb_obj_is_kind_of(array_or_hash, rb_cArray))) {
    int i, len;
    apr_array_header_t *result;

    len = RARRAY_LEN(array_or_hash);
    result = apr_array_make(pool, len, sizeof(svn_prop_t *));
    result->nelts = len;
    for (i = 0; i < len; i++) {
      VALUE name, value, item;
      svn_prop_t *prop;

      item = rb_ary_entry(array_or_hash, i);
      name = rb_funcall(item, id_name, 0);
      value = rb_funcall(item, id_value, 0);
      prop = apr_palloc(pool, sizeof(svn_prop_t));
      prop->name = apr_pstrdup(pool, StringValueCStr(name));
      prop->value = svn_string_ncreate(RSTRING_PTR(value), RSTRING_LEN(value),
                                       pool);
      APR_ARRAY_IDX(result, i, svn_prop_t *) = prop;
    }
    return result;
  } else if (RTEST(rb_obj_is_kind_of(array_or_hash, rb_cHash))) {
    apr_array_header_t *result;
    prop_hash_each_arg_t arg;

    result = apr_array_make(pool, 0, sizeof(svn_prop_t *));
    arg.array = result;
    arg.pool = pool;
    rb_hash_foreach(array_or_hash, svn_swig_rb_to_apr_array_prop_callback,
                    (VALUE)&arg);
    return result;
  } else {
    rb_raise(rb_eArgError,
             "'%s' must be [Svn::Core::Prop, ...] or {'name' => 'value', ...}",
             r2c_inspect(array_or_hash));
  }
}


/* C -> Ruby (dup) */
#define DEFINE_DUP_BASE(type, dup_func, type_prefix)                         \
static VALUE                                                                 \
c2r_ ## type ## _dup(void *type, void *ctx)                                  \
{                                                                            \
  apr_pool_t *pool;                                                          \
  VALUE rb_pool;                                                             \
  svn_ ## type ## _t *copied_item;                                           \
  VALUE rb_copied_item;                                                      \
                                                                             \
  if (!type)                                                                 \
    return Qnil;                                                             \
                                                                             \
  svn_swig_rb_get_pool(0, (VALUE *)0, 0, &rb_pool, &pool);                   \
  copied_item = svn_ ## dup_func((type_prefix svn_ ## type ## _t *)type,     \
                                  pool);                                     \
  rb_copied_item = c2r_swig_type((void *)copied_item,                        \
                                 (void *)"svn_" # type "_t *");              \
  rb_set_pool(rb_copied_item, rb_pool);                                      \
                                                                             \
  return rb_copied_item;                                                     \
}

#define DEFINE_DUP_BASE_WITH_CONVENIENCE(type, dup_func, type_prefix)   \
DEFINE_DUP_BASE(type, dup_func, type_prefix)                            \
static VALUE                                                            \
c2r_ ## type ## __dup(type_prefix svn_ ## type ## _t *type)             \
{                                                                       \
  void *void_type;                                                      \
  void_type = (void *)type;                                             \
  return c2r_ ## type ## _dup(void_type, NULL);                         \
}

#define DEFINE_DUP_WITH_FUNCTION_NAME(type, dup_func) \
  DEFINE_DUP_BASE_WITH_CONVENIENCE(type, dup_func, const)
#define DEFINE_DUP(type) \
  DEFINE_DUP_WITH_FUNCTION_NAME(type, type ## _dup)

#define DEFINE_DUP_NO_CONVENIENCE_WITH_FUNCTION_NAME(type, dup_func) \
  DEFINE_DUP_BASE(type, dup_func, const)
#define DEFINE_DUP_NO_CONVENIENCE(type) \
  DEFINE_DUP_NO_CONVENIENCE_WITH_FUNCTION_NAME(type, type ## _dup)

#define DEFINE_DUP_NO_CONST_WITH_FUNCTION_NAME(type, dup_func) \
  DEFINE_DUP_BASE_WITH_CONVENIENCE(type, dup_func,)
#define DEFINE_DUP_NO_CONST(type) \
  DEFINE_DUP_NO_CONST_WITH_FUNCTION_NAME(type, type ## _dup)

#define DEFINE_DUP_NO_CONST_NO_CONVENIENCE_WITH_FUNCTION_NAME(type, dup_func) \
  DEFINE_DUP_BASE(type, dup_func,)
#define DEFINE_DUP_NO_CONST_NO_CONVENIENCE(type) \
  DEFINE_DUP_NO_CONST_NO_CONVENIENCE_WITH_FUNCTION_NAME(type, type ## _dup)


DEFINE_DUP_WITH_FUNCTION_NAME(wc_notify, wc_dup_notify)
DEFINE_DUP(txdelta_window)
DEFINE_DUP(info)
DEFINE_DUP(commit_info)
DEFINE_DUP(lock)
DEFINE_DUP(auth_ssl_server_cert_info)
DEFINE_DUP(wc_entry)
DEFINE_DUP(client_diff_summarize)
DEFINE_DUP(dirent)
DEFINE_DUP_NO_CONVENIENCE(client_commit_item3)
DEFINE_DUP_NO_CONVENIENCE(client_proplist_item)
DEFINE_DUP_NO_CONVENIENCE(wc_external_item2)
DEFINE_DUP_NO_CONVENIENCE(log_changed_path)
DEFINE_DUP_NO_CONST_WITH_FUNCTION_NAME(wc_status2, wc_dup_status2)
DEFINE_DUP_NO_CONST_NO_CONVENIENCE(merge_range)


/* Ruby -> C */
static const char *
r2c_inspect(VALUE object)
{
  VALUE inspected;
  inspected = rb_funcall(object, id_inspect, 0);
  return StringValueCStr(inspected);
}

static void *
r2c_string(VALUE value, void *ctx, apr_pool_t *pool)
{
  return (void *)apr_pstrdup(pool, StringValuePtr(value));
}

static void *
r2c_svn_string(VALUE value, void *ctx, apr_pool_t *pool)
{
  return (void *)svn_string_create(StringValuePtr(value), pool);
}

void *
svn_swig_rb_to_swig_type(VALUE value, void *ctx, apr_pool_t *pool)
{
  void **result = NULL;
  result = apr_palloc(pool, sizeof(void *));
  r2c_swig_type2(value, (const char *)ctx, result);
  return *result;
}
#define r2c_swig_type svn_swig_rb_to_swig_type

static void
r2c_swig_type2(VALUE value, const char *type_name, void **result)
{
#ifdef SWIG_IsOK
  int res;
  res = SWIG_ConvertPtr(value, result, SWIG_TypeQuery(type_name),
                        SWIG_POINTER_EXCEPTION);
  if (!SWIG_IsOK(res)) {
    VALUE message = rb_funcall(value, rb_intern("inspect"), 0);
    rb_str_cat2(message, "must be ");
    rb_str_cat2(message, type_name);
    SWIG_Error(SWIG_ArgError(res), StringValuePtr(message));
  }
#endif
}

static void *
r2c_long(VALUE value, void *ctx, apr_pool_t *pool)
{
  return (void *)NUM2LONG(value);
}

static void *
r2c_svn_err(VALUE rb_svn_err, void *ctx, apr_pool_t *pool)
{
  VALUE message;
  svn_error_t *err;

  message = rb_funcall(rb_svn_err, id_message, 0);
  err = svn_error_create(NUM2INT(rb_funcall(rb_svn_err, id_code, 0)),
                         NULL,
                         StringValuePtr(message));
  return (void *)err;
}

static void *
r2c_revnum(VALUE value, void *ctx, apr_pool_t *pool)
{
  svn_revnum_t *revnum;
  revnum = apr_palloc(pool, sizeof(svn_revnum_t));
  *revnum = NUM2INT(value);
  return revnum;
}

static void *
r2c_merge_range(VALUE value, void *ctx, apr_pool_t *pool)
{
  return svn_swig_rb_array_to_apr_array_merge_range(value, pool);
}


/* apr_array_t -> Ruby Array */
#define DEFINE_APR_ARRAY_TO_ARRAY(return_type, name, conv, amp, type, ctx)  \
return_type                                                                 \
name(const apr_array_header_t *apr_ary)                                     \
{                                                                           \
  VALUE ary = rb_ary_new();                                                 \
  int i;                                                                    \
                                                                            \
  for (i = 0; i < apr_ary->nelts; i++) {                                    \
    rb_ary_push(ary, conv((void *)amp(APR_ARRAY_IDX(apr_ary, i, type)),     \
                          ctx));                                            \
  }                                                                         \
                                                                            \
  return ary;                                                               \
}

DEFINE_APR_ARRAY_TO_ARRAY(VALUE, svn_swig_rb_apr_array_to_array_string,
                          c2r_string, EMPTY_CPP_ARGUMENT, const char *, NULL)
DEFINE_APR_ARRAY_TO_ARRAY(VALUE, svn_swig_rb_apr_array_to_array_svn_string,
                          c2r_svn_string, &, svn_string_t, NULL)
DEFINE_APR_ARRAY_TO_ARRAY(static VALUE, c2r_commit_item3_array,
                          c2r_client_commit_item3_dup, EMPTY_CPP_ARGUMENT,
                          svn_client_commit_item3_t *, NULL)
DEFINE_APR_ARRAY_TO_ARRAY(VALUE, svn_swig_rb_apr_array_to_array_svn_rev,
                          c2r_long, &, svn_revnum_t, NULL)
DEFINE_APR_ARRAY_TO_ARRAY(VALUE, svn_swig_rb_apr_array_to_array_proplist_item,
                          c2r_client_proplist_item_dup, EMPTY_CPP_ARGUMENT,
                          svn_client_proplist_item_t *, NULL)
DEFINE_APR_ARRAY_TO_ARRAY(VALUE, svn_swig_rb_apr_array_to_array_external_item2,
                          c2r_wc_external_item2_dup, EMPTY_CPP_ARGUMENT,
                          svn_wc_external_item2_t *, NULL)
DEFINE_APR_ARRAY_TO_ARRAY(VALUE, svn_swig_rb_apr_array_to_array_merge_range,
                          c2r_merge_range_dup, EMPTY_CPP_ARGUMENT,
                          svn_merge_range_t *, NULL)
DEFINE_APR_ARRAY_TO_ARRAY(VALUE, svn_swig_rb_apr_array_to_array_auth_provider_object,
                          c2r_swig_type, EMPTY_CPP_ARGUMENT,
                          svn_auth_provider_object_t *, "svn_auth_provider_object_t*")

static VALUE
c2r_merge_range_array(void *value, void *ctx)
{
  return svn_swig_rb_apr_array_to_array_merge_range(value);
}

VALUE
svn_swig_rb_prop_apr_array_to_hash_prop(const apr_array_header_t *apr_ary)
{
  VALUE hash;
  int i;

  hash = rb_hash_new();
  for (i = 0; i < apr_ary->nelts; i++) {
    svn_prop_t prop;
    prop = APR_ARRAY_IDX(apr_ary, i, svn_prop_t);
    rb_hash_aset(hash,
                 prop.name ? rb_str_new2(prop.name) : Qnil,
                 prop.value && prop.value->data ?
                   rb_str_new2(prop.value->data) : Qnil);
  }

  return hash;
}

apr_array_header_t *
svn_swig_rb_array_to_apr_array_revision_range(VALUE array, apr_pool_t *pool)
{
  int i, len;
  apr_array_header_t *apr_ary;

  Check_Type(array, T_ARRAY);
  len = RARRAY_LEN(array);
  apr_ary = apr_array_make(pool, len, sizeof(svn_opt_revision_range_t *));
  apr_ary->nelts = len;
  for (i = 0; i < len; i++) {
    VALUE value;
    svn_opt_revision_range_t *range;

    value = rb_ary_entry(array, i);
    if (RTEST(rb_obj_is_kind_of(value, rb_cArray))) {
      if (RARRAY_LEN(value) != 2)
        rb_raise(rb_eArgError,
                 "revision range should be [start, end]: %s",
                 r2c_inspect(value));
      range = apr_palloc(pool, sizeof(*range));
      svn_swig_rb_set_revision(&range->start, rb_ary_entry(value, 0));
      svn_swig_rb_set_revision(&range->end, rb_ary_entry(value, 1));
    } else {
      range = r2c_swig_type(value, (void *)"svn_opt_revision_range_t *", pool);
    }
    APR_ARRAY_IDX(apr_ary, i, svn_opt_revision_range_t *) = range;
  }
  return apr_ary;
}



/* Ruby Array -> apr_array_t */
#define DEFINE_ARRAY_TO_APR_ARRAY(type, name, converter, context) \
apr_array_header_t *                                              \
name(VALUE array, apr_pool_t *pool)                               \
{                                                                 \
  int i, len;                                                     \
  apr_array_header_t *apr_ary;                                    \
                                                                  \
  Check_Type(array, T_ARRAY);                                     \
  len = RARRAY_LEN(array);                                       \
  apr_ary = apr_array_make(pool, len, sizeof(type));              \
  apr_ary->nelts = len;                                           \
  for (i = 0; i < len; i++) {                                     \
    VALUE value;                                                  \
    type val;                                                     \
    value = rb_ary_entry(array, i);                               \
    val = (type)converter(value, context, pool);                  \
    APR_ARRAY_IDX(apr_ary, i, type) = val;                        \
  }                                                               \
  return apr_ary;                                                 \
}

DEFINE_ARRAY_TO_APR_ARRAY(const char *, svn_swig_rb_strings_to_apr_array,
                          r2c_string, NULL)
DEFINE_ARRAY_TO_APR_ARRAY(svn_auth_provider_object_t *,
                          svn_swig_rb_array_to_auth_provider_object_apr_array,
                          r2c_swig_type, (void *)"svn_auth_provider_object_t *")
DEFINE_ARRAY_TO_APR_ARRAY(svn_revnum_t,
                          svn_swig_rb_array_to_apr_array_revnum,
                          r2c_long, NULL)
DEFINE_ARRAY_TO_APR_ARRAY(svn_merge_range_t *,
                          svn_swig_rb_array_to_apr_array_merge_range,
                          r2c_swig_type, (void *)"svn_merge_range_t *")
DEFINE_ARRAY_TO_APR_ARRAY(svn_client_copy_source_t *,
                          svn_swig_rb_array_to_apr_array_copy_source,
                          r2c_swig_type, (void *)"svn_client_copy_source_t *")


/* apr_hash_t -> Ruby Hash */
static VALUE
c2r_hash_with_key_convert(apr_hash_t *hash,
                          c2r_func key_conv,
                          void *key_ctx,
                          c2r_func value_conv,
                          void *value_ctx)
{
  apr_hash_index_t *hi;
  VALUE r_hash;

  if (!hash)
    return Qnil;

  r_hash = rb_hash_new();

  for (hi = apr_hash_first(NULL, hash); hi; hi = apr_hash_next(hi)) {
    const void *key;
    void *val;
    VALUE v = Qnil;

    apr_hash_this(hi, &key, NULL, &val);
    if (val) {
      v = (*value_conv)(val, value_ctx);
    }
    rb_hash_aset(r_hash, (*key_conv)((void *)key, key_ctx), v);
  }

  return r_hash;
}

static VALUE
c2r_hash(apr_hash_t *hash,
         c2r_func value_conv,
         void *ctx)
{
  return c2r_hash_with_key_convert(hash, c2r_string, NULL, value_conv, ctx);
}

VALUE
svn_swig_rb_apr_hash_to_hash_string(apr_hash_t *hash)
{
  return c2r_hash(hash, c2r_string, NULL);
}

VALUE
svn_swig_rb_apr_hash_to_hash_svn_string(apr_hash_t *hash)
{
  return c2r_hash(hash, c2r_svn_string, NULL);
}

VALUE
svn_swig_rb_apr_hash_to_hash_swig_type(apr_hash_t *hash, const char *type_name)
{
  return c2r_hash(hash, c2r_swig_type, (void *)type_name);
}

VALUE
svn_swig_rb_apr_hash_to_hash_merge_range(apr_hash_t *hash)
{
  return c2r_hash(hash, c2r_merge_range_array, NULL);
}

static VALUE
c2r_merge_range_hash(void *value, void *ctx)
{
  apr_hash_t *hash = value;

  return svn_swig_rb_apr_hash_to_hash_merge_range(hash);
}

VALUE
svn_swig_rb_apr_hash_to_hash_merge_range_hash(apr_hash_t *hash)
{
  return c2r_hash(hash, c2r_merge_range_hash, NULL);
}

VALUE
svn_swig_rb_prop_hash_to_hash(apr_hash_t *prop_hash)
{
  return svn_swig_rb_apr_hash_to_hash_svn_string(prop_hash);
}

static VALUE
c2r_revnum(void *value, void *ctx)
{
  svn_revnum_t *num = value;
  return INT2NUM(*num);
}

VALUE
svn_swig_rb_apr_revnum_key_hash_to_hash_string(apr_hash_t *hash)
{
  return c2r_hash_with_key_convert(hash, c2r_revnum, NULL, c2r_string, NULL);
}


/* Ruby Hash -> apr_hash_t */
static int
r2c_hash_i(VALUE key, VALUE value, hash_to_apr_hash_data_t *data)
{
  if (key != Qundef) {
    void *val = data->func(value, data->ctx, data->pool);
    apr_hash_set(data->apr_hash,
                 apr_pstrdup(data->pool, StringValuePtr(key)),
                 APR_HASH_KEY_STRING,
                 val);
  }
  return ST_CONTINUE;
}

static apr_hash_t *
r2c_hash(VALUE hash, r2c_func func, void *ctx, apr_pool_t *pool)
{
  if (NIL_P(hash)) {
    return NULL;
  } else {
    apr_hash_t *apr_hash;
    hash_to_apr_hash_data_t data = {
      NULL,
      func,
      ctx,
      pool
    };

    apr_hash = apr_hash_make(pool);
    data.apr_hash = apr_hash;
    rb_hash_foreach(hash, r2c_hash_i, (VALUE)&data);

    return apr_hash;
  }
}


apr_hash_t *
svn_swig_rb_hash_to_apr_hash_string(VALUE hash, apr_pool_t *pool)
{
  return r2c_hash(hash, r2c_string, NULL, pool);
}

apr_hash_t *
svn_swig_rb_hash_to_apr_hash_svn_string(VALUE hash, apr_pool_t *pool)
{
  return r2c_hash(hash, r2c_svn_string, NULL, pool);
}

apr_hash_t *
svn_swig_rb_hash_to_apr_hash_swig_type(VALUE hash, const char *typename, apr_pool_t *pool)
{
  return r2c_hash(hash, r2c_swig_type, (void *)typename, pool);
}

apr_hash_t *
svn_swig_rb_hash_to_apr_hash_revnum(VALUE hash, apr_pool_t *pool)
{
  return r2c_hash(hash, r2c_revnum, NULL, pool);
}

apr_hash_t *
svn_swig_rb_hash_to_apr_hash_merge_range(VALUE hash, apr_pool_t *pool)
{
  return r2c_hash(hash, r2c_merge_range, NULL, pool);
}


/* callback */
typedef struct callback_baton_t {
  VALUE pool;
  VALUE receiver;
  ID message;
  VALUE args;
} callback_baton_t;

typedef struct callback_rescue_baton_t {
  svn_error_t **err;
  VALUE pool;
} callback_rescue_baton_t;

typedef struct callback_handle_error_baton_t {
  callback_baton_t *callback_baton;
  callback_rescue_baton_t *rescue_baton;
} callback_handle_error_baton_t;

static VALUE
callback(VALUE baton)
{
  callback_baton_t *cbb = (callback_baton_t *)baton;
  VALUE result;

  result = rb_apply(cbb->receiver, cbb->message, cbb->args);
  svn_swig_rb_push_pool(cbb->pool);

  return result;
}

static VALUE
callback_rescue(VALUE baton)
{
  callback_rescue_baton_t *rescue_baton = (callback_rescue_baton_t*)baton;

  *(rescue_baton->err) = r2c_svn_err(
#ifdef HAVE_RB_ERRINFO
                                     rb_errinfo(),
#else
                                     ruby_errinfo,
#endif
                                     NULL, NULL);
  svn_swig_rb_push_pool(rescue_baton->pool);

  return Qnil;
}

static VALUE
callback_ensure(VALUE pool)
{
  svn_swig_rb_pop_pool(pool);

  return Qnil;
}

static VALUE
invoke_callback(VALUE baton, VALUE pool)
{
  callback_baton_t *cbb = (callback_baton_t *)baton;
  VALUE sub_pool;
  VALUE argv[] = {pool};

  svn_swig_rb_get_pool(1, argv, Qnil, &sub_pool, NULL);
  cbb->pool = sub_pool;
  return rb_ensure(callback, baton, callback_ensure, sub_pool);
}

static VALUE
callback_handle_error(VALUE baton)
{
  callback_handle_error_baton_t *handle_error_baton;
  handle_error_baton = (callback_handle_error_baton_t *)baton;

  return rb_rescue2(callback,
                    (VALUE)(handle_error_baton->callback_baton),
                    callback_rescue,
                    (VALUE)(handle_error_baton->rescue_baton),
                    rb_svn_error(),
                    (VALUE)0);
}

static VALUE
invoke_callback_handle_error(VALUE baton, VALUE pool, svn_error_t **err)
{
  callback_baton_t *cbb = (callback_baton_t *)baton;
  callback_handle_error_baton_t handle_error_baton;
  callback_rescue_baton_t rescue_baton;

  rescue_baton.err = err;
  rescue_baton.pool = pool;
  cbb->pool = pool;
  handle_error_baton.callback_baton = cbb;
  handle_error_baton.rescue_baton = &rescue_baton;

  return rb_ensure(callback_handle_error, (VALUE)&handle_error_baton,
                   callback_ensure, pool);
}


/* svn_delta_editor_t */
typedef struct item_baton {
  VALUE editor;
  VALUE baton;
} item_baton;

static void
add_baton(VALUE editor, VALUE baton)
{
  if (NIL_P((rb_ivar_get(editor, id_baton)))) {
    rb_ivar_set(editor, id_baton, rb_ary_new());
  }

  rb_ary_push(rb_ivar_get(editor, id_baton), baton);
}

static item_baton *
make_baton(apr_pool_t *pool, VALUE editor, VALUE baton)
{
  item_baton *newb = apr_palloc(pool, sizeof(*newb));

  newb->editor = editor;
  newb->baton = baton;
  add_baton(editor, baton);

  return newb;
}

static VALUE
add_baton_if_delta_editor(VALUE target, VALUE baton)
{
  if (RTEST(rb_obj_is_kind_of(target, svn_swig_rb_svn_delta_editor()))) {
    add_baton(target, baton);
  }

  return Qnil;
}

void
svn_swig_rb_set_baton(VALUE target, VALUE baton)
{
  if (NIL_P(baton)) {
    return;
  }

  if (!RTEST(rb_obj_is_kind_of(target, rb_cArray))) {
    target = rb_ary_new3(1, target);
  }

  rb_iterate(rb_each, target, add_baton_if_delta_editor, baton);
}


static svn_error_t *
delta_editor_set_target_revision(void *edit_baton,
                                 svn_revnum_t target_revision,
                                 apr_pool_t *pool)
{
  item_baton *ib = edit_baton;
  svn_error_t *err = SVN_NO_ERROR;
  callback_baton_t cbb;

  cbb.receiver = ib->editor;
  cbb.message = id_set_target_revision;
  cbb.args = rb_ary_new3(1, INT2NUM(target_revision));
  invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
  return err;
}

static svn_error_t *
delta_editor_open_root(void *edit_baton,
                       svn_revnum_t base_revision,
                       apr_pool_t *dir_pool,
                       void **root_baton)
{
  item_baton *ib = edit_baton;
  svn_error_t *err = SVN_NO_ERROR;
  callback_baton_t cbb;
  VALUE result;

  cbb.receiver = ib->editor;
  cbb.message = id_open_root;
  cbb.args = rb_ary_new3(1, INT2NUM(base_revision));
  result = invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
  *root_baton = make_baton(dir_pool, ib->editor, result);
  return err;
}

static svn_error_t *
delta_editor_delete_entry(const char *path,
                          svn_revnum_t revision,
                          void *parent_baton,
                          apr_pool_t *pool)
{
  item_baton *ib = parent_baton;
  svn_error_t *err = SVN_NO_ERROR;
  callback_baton_t cbb;

  cbb.receiver = ib->editor;
  cbb.message = id_delete_entry;
  cbb.args = rb_ary_new3(3, c2r_string2(path), INT2NUM(revision), ib->baton);
  invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
  return err;
}

static svn_error_t *
delta_editor_add_directory(const char *path,
                           void *parent_baton,
                           const char *copyfrom_path,
                           svn_revnum_t copyfrom_revision,
                           apr_pool_t *dir_pool,
                           void **child_baton)
{
  item_baton *ib = parent_baton;
  svn_error_t *err = SVN_NO_ERROR;
  callback_baton_t cbb;
  VALUE result;

  cbb.receiver = ib->editor;
  cbb.message = id_add_directory;
  cbb.args = rb_ary_new3(4,
                         c2r_string2(path),
                         ib->baton,
                         c2r_string2(copyfrom_path),
                         INT2NUM(copyfrom_revision));
  result = invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
  *child_baton = make_baton(dir_pool, ib->editor, result);
  return err;
}

static svn_error_t *
delta_editor_open_directory(const char *path,
                            void *parent_baton,
                            svn_revnum_t base_revision,
                            apr_pool_t *dir_pool,
                            void **child_baton)
{
  item_baton *ib = parent_baton;
  svn_error_t *err = SVN_NO_ERROR;
  callback_baton_t cbb;
  VALUE result;

  cbb.receiver = ib->editor;
  cbb.message = id_open_directory;
  cbb.args = rb_ary_new3(3,
                         c2r_string2(path),
                         ib->baton,
                         INT2NUM(base_revision));
  result = invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
  *child_baton = make_baton(dir_pool, ib->editor, result);
  return err;
}

static svn_error_t *
delta_editor_change_dir_prop(void *dir_baton,
                             const char *name,
                             const svn_string_t *value,
                             apr_pool_t *pool)
{
  item_baton *ib = dir_baton;
  svn_error_t *err = SVN_NO_ERROR;
  callback_baton_t cbb;

  cbb.receiver = ib->editor;
  cbb.message = id_change_dir_prop;
  cbb.args = rb_ary_new3(3,
                         ib->baton,
                         c2r_string2(name),
                         value ? rb_str_new(value->data, value->len) : Qnil);
  invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
  return err;
}

static svn_error_t *
delta_editor_close_baton(void *baton, ID method_id)
{
  item_baton *ib = baton;
  svn_error_t *err = SVN_NO_ERROR;
  callback_baton_t cbb;

  cbb.receiver = ib->editor;
  cbb.message = method_id;
  cbb.args = rb_ary_new3(1, ib->baton);
  invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
  return err;
}

static svn_error_t *
delta_editor_close_directory(void *dir_baton, apr_pool_t *pool)
{
  return delta_editor_close_baton(dir_baton, id_close_directory);
}

static svn_error_t *
delta_editor_absent_directory(const char *path,
                              void *parent_baton,
                              apr_pool_t *pool)
{
  item_baton *ib = parent_baton;
  svn_error_t *err = SVN_NO_ERROR;
  callback_baton_t cbb;

  cbb.receiver = ib->editor;
  cbb.message = id_absent_directory;
  cbb.args = rb_ary_new3(2, c2r_string2(path), ib->baton);
  invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
  return err;
}

static svn_error_t *
delta_editor_add_file(const char *path,
                      void *parent_baton,
                      const char *copyfrom_path,
                      svn_revnum_t copyfrom_revision,
                      apr_pool_t *file_pool,
                      void **file_baton)
{
  item_baton *ib = parent_baton;
  svn_error_t *err = SVN_NO_ERROR;
  callback_baton_t cbb;
  VALUE result;

  cbb.receiver = ib->editor;
  cbb.message = id_add_file;
  cbb.args = rb_ary_new3(4,
                         c2r_string2(path),
                         ib->baton,
                         c2r_string2(copyfrom_path),
                         INT2NUM(copyfrom_revision));
  result = invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
  *file_baton = make_baton(file_pool, ib->editor, result);
  return err;
}

static svn_error_t *
delta_editor_open_file(const char *path,
                       void *parent_baton,
                       svn_revnum_t base_revision,
                       apr_pool_t *file_pool,
                       void **file_baton)
{
  item_baton *ib = parent_baton;
  svn_error_t *err = SVN_NO_ERROR;
  callback_baton_t cbb;
  VALUE result;

  cbb.receiver = ib->editor;
  cbb.message = id_open_file;
  cbb.args = rb_ary_new3(3,
                         c2r_string2(path),
                         ib->baton,
                         INT2NUM(base_revision));
  result = invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
  *file_baton = make_baton(file_pool, ib->editor, result);
  return err;
}

static svn_error_t *
delta_editor_window_handler(svn_txdelta_window_t *window, void *baton)
{
  VALUE handler = (VALUE)baton;
  callback_baton_t cbb;
  VALUE result;
  svn_error_t *err = SVN_NO_ERROR;

  cbb.receiver = handler;
  cbb.message = id_call;
  cbb.args = rb_ary_new3(1, c2r_txdelta_window__dup(window));
  result = invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
  return err;
}

static svn_error_t *
delta_editor_apply_textdelta(void *file_baton,
                             const char *base_checksum,
                             apr_pool_t *pool,
                             svn_txdelta_window_handler_t *handler,
                             void **h_baton)
{
  item_baton *ib = file_baton;
  svn_error_t *err = SVN_NO_ERROR;
  callback_baton_t cbb;
  VALUE result;

  cbb.receiver = ib->editor;
  cbb.message = id_apply_textdelta;
  cbb.args = rb_ary_new3(2, ib->baton, c2r_string2(base_checksum));
  result = invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
  if (NIL_P(result)) {
    *handler = svn_delta_noop_window_handler;
    *h_baton = NULL;
  } else {
    *handler = delta_editor_window_handler;
    *h_baton = (void *)result;
  }

  return err;
}

static svn_error_t *
delta_editor_change_file_prop(void *file_baton,
                              const char *name,
                              const svn_string_t *value,
                              apr_pool_t *pool)
{
  item_baton *ib = file_baton;
  svn_error_t *err = SVN_NO_ERROR;
  callback_baton_t cbb;

  cbb.receiver = ib->editor;
  cbb.message = id_change_file_prop;
  cbb.args = rb_ary_new3(3,
                         ib->baton,
                         c2r_string2(name),
                         value ? rb_str_new(value->data, value->len) : Qnil);
  invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);

  return err;
}

static svn_error_t *
delta_editor_close_file(void *file_baton,
                        const char *text_checksum,
                        apr_pool_t *pool)
{
  item_baton *ib = file_baton;
  svn_error_t *err = SVN_NO_ERROR;
  callback_baton_t cbb;

  cbb.receiver = ib->editor;
  cbb.message = id_close_file;
  cbb.args = rb_ary_new3(2, ib->baton, c2r_string2(text_checksum));
  invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);

  return err;
}

static svn_error_t *
delta_editor_absent_file(const char *path,
                         void *parent_baton,
                         apr_pool_t *pool)
{
  item_baton *ib = parent_baton;
  svn_error_t *err = SVN_NO_ERROR;
  callback_baton_t cbb;

  cbb.receiver = ib->editor;
  cbb.message = id_absent_file;
  cbb.args = rb_ary_new3(2, c2r_string2(path), ib->baton);
  invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);

  return err;
}

static svn_error_t *
delta_editor_close_edit(void *edit_baton, apr_pool_t *pool)
{
  item_baton *ib = edit_baton;
  svn_error_t *err = delta_editor_close_baton(edit_baton, id_close_edit);
  rb_ary_clear(rb_ivar_get(ib->editor, id_baton));
  return err;
}

static svn_error_t *
delta_editor_abort_edit(void *edit_baton, apr_pool_t *pool)
{
  item_baton *ib = edit_baton;
  svn_error_t *err = delta_editor_close_baton(edit_baton, id_abort_edit);
  rb_ary_clear(rb_ivar_get(ib->editor, id_baton));
  return err;
}

void
svn_swig_rb_make_delta_editor(svn_delta_editor_t **editor,
                              void **edit_baton,
                              VALUE rb_editor,
                              apr_pool_t *pool)
{
  svn_delta_editor_t *thunk_editor = svn_delta_default_editor(pool);

  thunk_editor->set_target_revision = delta_editor_set_target_revision;
  thunk_editor->open_root = delta_editor_open_root;
  thunk_editor->delete_entry = delta_editor_delete_entry;
  thunk_editor->add_directory = delta_editor_add_directory;
  thunk_editor->open_directory = delta_editor_open_directory;
  thunk_editor->change_dir_prop = delta_editor_change_dir_prop;
  thunk_editor->close_directory = delta_editor_close_directory;
  thunk_editor->absent_directory = delta_editor_absent_directory;
  thunk_editor->add_file = delta_editor_add_file;
  thunk_editor->open_file = delta_editor_open_file;
  thunk_editor->apply_textdelta = delta_editor_apply_textdelta;
  thunk_editor->change_file_prop = delta_editor_change_file_prop;
  thunk_editor->close_file = delta_editor_close_file;
  thunk_editor->absent_file = delta_editor_absent_file;
  thunk_editor->close_edit = delta_editor_close_edit;
  thunk_editor->abort_edit = delta_editor_abort_edit;

  *editor = thunk_editor;
  rb_ivar_set(rb_editor, id_baton, rb_ary_new());
  *edit_baton = make_baton(pool, rb_editor, Qnil);
}


VALUE
svn_swig_rb_make_baton(VALUE proc, VALUE pool)
{
  if (NIL_P(proc)) {
    return Qnil;
  } else {
    return rb_ary_new3(2, proc, pool);
  }
}

void
svn_swig_rb_from_baton(VALUE baton, VALUE *proc, VALUE *pool)
{
  if (NIL_P(baton)) {
    *proc = Qnil;
    *pool = Qnil;
  } else {
    *proc = rb_ary_entry(baton, 0);
    *pool = rb_ary_entry(baton, 1);
  }
}

svn_error_t *
svn_swig_rb_log_receiver(void *baton,
                         apr_hash_t *changed_paths,
                         svn_revnum_t revision,
                         const char *author,
                         const char *date,
                         const char *message,
                         apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;
    VALUE rb_changed_paths = Qnil;

    if (changed_paths) {
      rb_changed_paths = c2r_hash(changed_paths,
                                  c2r_log_changed_path_dup,
                                  NULL);
    }

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(5,
                           rb_changed_paths,
                           c2r_long(&revision, NULL),
                           c2r_string2(author),
                           c2r_svn_date_string2(date),
                           c2r_string2(message));
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }
  return err;
}

svn_error_t *
svn_swig_rb_log_entry_receiver(void *baton,
                               svn_log_entry_t *entry,
                               apr_pool_t *pool)
{
    svn_error_t *err = SVN_NO_ERROR;
    VALUE proc, rb_pool;

    svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

    if (!NIL_P(proc)) {
        callback_baton_t cbb;

        cbb.receiver = proc;
        cbb.message = id_call;
        cbb.args = rb_ary_new3(1,
                               c2r_swig_type((void *)entry,
                                             (void *)"svn_log_entry_t *"));
        invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
    }
    return err;
}


svn_error_t *
svn_swig_rb_repos_authz_func(svn_boolean_t *allowed,
                             svn_fs_root_t *root,
                             const char *path,
                             void *baton,
                             apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  *allowed = TRUE;

  if (!NIL_P(proc)) {
    callback_baton_t cbb;
    VALUE result;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(2,
                           c2r_swig_type((void *)root,
                                         (void *)"svn_fs_root_t *"),
                           c2r_string2(path));
    result = invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);

    *allowed = RTEST(result);
  }
  return err;
}

svn_error_t *
svn_swig_rb_repos_authz_callback(svn_repos_authz_access_t required,
                                 svn_boolean_t *allowed,
                                 svn_fs_root_t *root,
                                 const char *path,
                                 void *baton,
                                 apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  *allowed = TRUE;

  if (!NIL_P(proc)) {
    callback_baton_t cbb;
    VALUE result;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(3,
                           INT2NUM(required),
                           c2r_swig_type((void *)root,
                                         (void *)"svn_fs_root_t *"),
                           c2r_string2(path));
    result = invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);

    *allowed = RTEST(result);
  }
  return err;
}

svn_error_t *
svn_swig_rb_get_commit_log_func(const char **log_msg,
                                const char **tmp_file,
                                const apr_array_header_t *commit_items,
                                void *baton,
                                apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  *log_msg = NULL;
  *tmp_file = NULL;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;
    VALUE result;
    VALUE is_message;
    VALUE value;
    char *ret;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(1, c2r_commit_item3_array(commit_items));
    result = invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);

    if (!err) {
      char error_message[] =
        "log_msg_func should return an array not '%s': "
        "[TRUE_IF_IT_IS_MESSAGE, MESSAGE_OR_FILE_AS_STRING]";

      if (!RTEST(rb_obj_is_kind_of(result, rb_cArray)))
        rb_raise(rb_eTypeError, error_message, r2c_inspect(result));
      is_message = rb_ary_entry(result, 0);
      value = rb_ary_entry(result, 1);

      if (!RTEST(rb_obj_is_kind_of(value, rb_cString)))
        rb_raise(rb_eTypeError, error_message, r2c_inspect(result));
      ret = (char *)r2c_string(value, NULL, pool);
      if (RTEST(is_message)) {
        *log_msg = ret;
      } else {
        *tmp_file = ret;
      }
    }
  }
  return err;
}


void
svn_swig_rb_notify_func2(void *baton,
                         const svn_wc_notify_t *notify,
                         apr_pool_t *pool)
{
  VALUE proc, rb_pool;
  callback_baton_t cbb;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(1, c2r_wc_notify__dup(notify));
  }

  if (!NIL_P(proc))
    invoke_callback((VALUE)(&cbb), rb_pool);
}

svn_error_t *
svn_swig_rb_conflict_resolver_func
    (svn_wc_conflict_result_t **result,
     const svn_wc_conflict_description_t *description,
     void *baton,
     apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (NIL_P(proc)) {
    *result = svn_wc_create_conflict_result(svn_wc_conflict_choose_postpone,
                                            description->merged_file,
                                            pool);
  } else {
    callback_baton_t cbb;
    VALUE fret;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(
                   1,
                   c2r_swig_type((void *)description,
                                 (void *)"svn_wc_conflict_description_t *") );
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
    fret = invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
    *result = svn_wc_create_conflict_result(NUM2INT(fret),
                                            description->merged_file,
                                            pool);
  }

  return err;
}

svn_error_t *
svn_swig_rb_commit_callback(svn_revnum_t new_revision,
                            const char *date,
                            const char *author,
                            void *baton)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(3,
                           INT2NUM(new_revision),
                           c2r_svn_date_string2(date),
                           c2r_string2(author));
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}

svn_error_t *
svn_swig_rb_commit_callback2(const svn_commit_info_t *commit_info,
                             void *baton, apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(1, c2r_commit_info__dup(commit_info));
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}

svn_error_t *
svn_swig_rb_cancel_func(void *cancel_baton)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)cancel_baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(0);
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}

svn_error_t *
svn_swig_rb_info_receiver(void *baton,
                          const char *path,
                          const svn_info_t *info,
                          apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(2, c2r_string2(path), c2r_info__dup(info));
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}

svn_boolean_t
svn_swig_rb_config_enumerator(const char *name,
                              const char *value,
                              void *baton,
                              apr_pool_t *pool)
{
  svn_boolean_t result = FALSE;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(2, c2r_string2(name), c2r_string2(value));
    result = RTEST(invoke_callback((VALUE)(&cbb), rb_pool));
  }

  return result;
}

svn_boolean_t
svn_swig_rb_config_section_enumerator(const char *name,
                                      void *baton,
                                      apr_pool_t *pool)
{
  svn_boolean_t result = FALSE;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(1, c2r_string2(name));
    result = RTEST(invoke_callback((VALUE)(&cbb), rb_pool));
  }

  return result;
}

svn_error_t *
svn_swig_rb_delta_path_driver_cb_func(void **dir_baton,
                                      void *parent_baton,
                                      void *callback_baton,
                                      const char *path,
                                      apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)callback_baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;
    VALUE result;
    item_baton *ib = (item_baton *)parent_baton;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(2, ib->baton, c2r_string2(path));
    result = invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
    *dir_baton = make_baton(pool, ib->editor, result);
  }

  return err;
}

svn_error_t *
svn_swig_rb_txdelta_window_handler(svn_txdelta_window_t *window,
                                   void *baton)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(1, c2r_txdelta_window__dup(window));
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}

void
svn_swig_rb_fs_warning_callback(void *baton, svn_error_t *err)
{
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(1, svn_swig_rb_svn_error_to_rb_error(err));
    invoke_callback((VALUE)(&cbb), rb_pool);
  }
}

static apr_status_t
cleanup_fs_warning_callback_baton(void *baton)
{
  rb_holder_pop(rb_svn_fs_warning_callback_baton_holder(), (VALUE)baton);
  return APR_SUCCESS;
}

void
svn_swig_rb_fs_warning_callback_baton_register(VALUE baton, apr_pool_t *pool)
{
  rb_holder_push(rb_svn_fs_warning_callback_baton_holder(), (VALUE)baton);
  apr_pool_cleanup_register(pool, (void *)baton,
                            cleanup_fs_warning_callback_baton,
                            apr_pool_cleanup_null);
}

svn_error_t *
svn_swig_rb_fs_get_locks_callback(void *baton,
                                  svn_lock_t *lock,
                                  apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(1, c2r_lock__dup(lock));
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}


/* svn_ra_callbacks_t */
static svn_error_t *
ra_callbacks_open_tmp_file(apr_file_t **fp,
                           void *callback_baton,
                           apr_pool_t *pool)
{
  VALUE callbacks = (VALUE)callback_baton;
  svn_error_t *err = SVN_NO_ERROR;

  if (!NIL_P(callbacks)) {
    callback_baton_t cbb;
    VALUE result;

    cbb.receiver = callbacks;
    cbb.message = id_open_tmp_file;
    cbb.args = rb_ary_new3(0);

    result = invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
    *fp = svn_swig_rb_make_file(result, pool);
  }

  return err;
}

static svn_error_t *
ra_callbacks_get_wc_prop(void *baton,
                         const char *relpath,
                         const char *name,
                         const svn_string_t **value,
                         apr_pool_t *pool)
{
  VALUE callbacks = (VALUE)baton;
  svn_error_t *err = SVN_NO_ERROR;

  if (!NIL_P(callbacks)) {
    callback_baton_t cbb;
    VALUE result;

    cbb.receiver = callbacks;
    cbb.message = id_get_wc_prop;
    cbb.args = rb_ary_new3(2, c2r_string2(relpath), c2r_string2(name));
    result = invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
    if (NIL_P(result)) {
      *value = NULL;
    } else {
      *value = r2c_svn_string(result, NULL, pool);
    }
  }

  return err;
}

static svn_error_t *
ra_callbacks_set_wc_prop(void *baton,
                         const char *path,
                         const char *name,
                         const svn_string_t *value,
                         apr_pool_t *pool)
{
  VALUE callbacks = (VALUE)baton;
  svn_error_t *err = SVN_NO_ERROR;

  if (!NIL_P(callbacks)) {
    callback_baton_t cbb;

    cbb.receiver = callbacks;
    cbb.message = id_set_wc_prop;
    cbb.args = rb_ary_new3(3,
                           c2r_string2(path),
                           c2r_string2(name),
                           c2r_svn_string((void *)value, NULL));
    invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
  }

  return err;
}

static svn_error_t *
ra_callbacks_push_wc_prop(void *baton,
                          const char *path,
                          const char *name,
                          const svn_string_t *value,
                          apr_pool_t *pool)
{
  VALUE callbacks = (VALUE)baton;
  svn_error_t *err = SVN_NO_ERROR;

  if (!NIL_P(callbacks)) {
    callback_baton_t cbb;

    cbb.receiver = callbacks;
    cbb.message = id_push_wc_prop;
    cbb.args = rb_ary_new3(3,
                           c2r_string2(path),
                           c2r_string2(name),
                           c2r_svn_string((void *)value, NULL));
    invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
  }

  return err;
}

static svn_error_t *
ra_callbacks_invalidate_wc_props(void *baton,
                                 const char *path,
                                 const char *name,
                                 apr_pool_t *pool)
{
  VALUE callbacks = (VALUE)baton;
  svn_error_t *err = SVN_NO_ERROR;

  if (!NIL_P(callbacks)) {
    callback_baton_t cbb;

    cbb.receiver = callbacks;
    cbb.message = id_invalidate_wc_props;
    cbb.args = rb_ary_new3(2, c2r_string2(path), c2r_string2(name));
    invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
  }

  return err;
}


static void
ra_callbacks_progress_func(apr_off_t progress,
                           apr_off_t total,
                           void *baton,
                           apr_pool_t *pool)
{
  VALUE callbacks = (VALUE)baton;
  if (!NIL_P(callbacks)) {
    callback_baton_t cbb;

    cbb.receiver = callbacks;
    cbb.message = id_progress_func;
    cbb.args = rb_ary_new3(2, AOFF2NUM(progress), AOFF2NUM(total));
    invoke_callback((VALUE)(&cbb), Qnil);
  }
}

void
svn_swig_rb_setup_ra_callbacks(svn_ra_callbacks2_t **callbacks,
                               void **baton,
                               VALUE rb_callbacks,
                               apr_pool_t *pool)
{
  void *auth_baton = NULL;

  if (!NIL_P(rb_callbacks)) {
    VALUE rb_auth_baton = Qnil;
    rb_auth_baton = rb_funcall(rb_callbacks, id_auth_baton, 0);
    auth_baton = r2c_swig_type(rb_auth_baton,
                               (void *)"svn_auth_baton_t *",
                               pool);
  }

  *callbacks = apr_pcalloc(pool, sizeof(**callbacks));
  *baton = (void *)rb_callbacks;

  (*callbacks)->open_tmp_file = ra_callbacks_open_tmp_file;
  (*callbacks)->auth_baton = auth_baton;
  (*callbacks)->get_wc_prop = ra_callbacks_get_wc_prop;
  (*callbacks)->set_wc_prop = ra_callbacks_set_wc_prop;
  (*callbacks)->push_wc_prop = ra_callbacks_push_wc_prop;
  (*callbacks)->invalidate_wc_props = ra_callbacks_invalidate_wc_props;
  (*callbacks)->progress_func = ra_callbacks_progress_func;
  (*callbacks)->progress_baton = (void *)rb_callbacks;
}


svn_error_t *
svn_swig_rb_ra_lock_callback(void *baton,
                             const char *path,
                             svn_boolean_t do_lock,
                             const svn_lock_t *lock,
                             svn_error_t *ra_err,
                             apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(4,
                           c2r_string2(path),
                           do_lock ? Qtrue : Qfalse,
                           c2r_lock__dup(lock),
                           ra_err ?
                             svn_swig_rb_svn_error_to_rb_error(ra_err) :
                             Qnil);
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}

svn_error_t *
svn_swig_rb_just_call(void *baton)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(0);
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}

svn_error_t *
svn_swig_rb_ra_file_rev_handler(void *baton,
                                const char *path,
                                svn_revnum_t rev,
                                apr_hash_t *rev_props,
                                svn_txdelta_window_handler_t *delta_handler,
                                void **delta_baton,
                                apr_array_header_t *prop_diffs,
                                apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(4,
                           c2r_string2(path),
                           c2r_long(&rev, NULL),
                           svn_swig_rb_apr_hash_to_hash_svn_string(rev_props),
                           svn_swig_rb_prop_apr_array_to_hash_prop(prop_diffs));
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}

svn_error_t *
svn_swig_rb_repos_history_func(void *baton,
                               const char *path,
                               svn_revnum_t revision,
                               apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;
    VALUE result = Qnil;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(2,
                           c2r_string2(path),
                           c2r_long(&revision, NULL));
    result = invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);

    if (!err && SVN_ERR_P(result)) {
      err = r2c_svn_err(result, NULL, NULL);
    }
  }

  return err;
}

svn_error_t *
svn_swig_rb_repos_file_rev_handler(void *baton,
                                   const char *path,
                                   svn_revnum_t rev,
                                   apr_hash_t *rev_props,
                                   svn_txdelta_window_handler_t *delta_handler,
                                   void **delta_baton,
                                   apr_array_header_t *prop_diffs,
                                   apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(4,
                           c2r_string2(path),
                           c2r_long(&rev, NULL),
                           svn_swig_rb_apr_hash_to_hash_svn_string(rev_props),
                           svn_swig_rb_prop_apr_array_to_hash_prop(prop_diffs));
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}

svn_error_t *
svn_swig_rb_wc_relocation_validator3(void *baton,
                                     const char *uuid,
                                     const char *url,
                                     const char *root_url,
                                     apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(3,
                           c2r_string2(uuid),
                           c2r_string2(url),
                           c2r_string2(root_url));
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}



/* auth provider callbacks */
svn_error_t *
svn_swig_rb_auth_simple_prompt_func(svn_auth_cred_simple_t **cred,
                                    void *baton,
                                    const char *realm,
                                    const char *username,
                                    svn_boolean_t may_save,
                                    apr_pool_t *pool)
{
  svn_auth_cred_simple_t *new_cred = NULL;
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;
    VALUE result;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(3,
                           c2r_string2(realm),
                           c2r_string2(username),
                           RTEST(may_save) ? Qtrue : Qfalse);
    result = invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);

    if (!NIL_P(result)) {
      void *result_cred = NULL;
      svn_auth_cred_simple_t *tmp_cred = NULL;

      r2c_swig_type2(result, "svn_auth_cred_simple_t *", &result_cred);
      tmp_cred = (svn_auth_cred_simple_t *)result_cred;
      new_cred = apr_pcalloc(pool, sizeof(*new_cred));
      new_cred->username = tmp_cred->username ?
        apr_pstrdup(pool, tmp_cred->username) : NULL;
      new_cred->password = tmp_cred->password ?
        apr_pstrdup(pool, tmp_cred->password) : NULL;
      new_cred->may_save = tmp_cred->may_save;
    }
  }

  *cred = new_cred;
  return err;
}

svn_error_t *
svn_swig_rb_auth_username_prompt_func(svn_auth_cred_username_t **cred,
                                      void *baton,
                                      const char *realm,
                                      svn_boolean_t may_save,
                                      apr_pool_t *pool)
{
  svn_auth_cred_username_t *new_cred = NULL;
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;
    VALUE result;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(2,
                           c2r_string2(realm),
                           RTEST(may_save) ? Qtrue : Qfalse);
    result = invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);

    if (!NIL_P(result)) {
      void *result_cred = NULL;
      svn_auth_cred_username_t *tmp_cred = NULL;

      r2c_swig_type2(result, "svn_auth_cred_username_t *", &result_cred);
      tmp_cred = (svn_auth_cred_username_t *)result_cred;
      new_cred = apr_pcalloc(pool, sizeof(*new_cred));
      new_cred->username = tmp_cred->username ?
        apr_pstrdup(pool, tmp_cred->username) : NULL;
      new_cred->may_save = tmp_cred->may_save;
    }
  }

  *cred = new_cred;
  return err;
}

svn_error_t *
svn_swig_rb_auth_ssl_server_trust_prompt_func(
  svn_auth_cred_ssl_server_trust_t **cred,
  void *baton,
  const char *realm,
  apr_uint32_t failures,
  const svn_auth_ssl_server_cert_info_t *cert_info,
  svn_boolean_t may_save,
  apr_pool_t *pool)
{
  svn_auth_cred_ssl_server_trust_t *new_cred = NULL;
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;
    VALUE result;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(4,
                           c2r_string2(realm),
                           UINT2NUM(failures),
                           c2r_auth_ssl_server_cert_info__dup(cert_info),
                           RTEST(may_save) ? Qtrue : Qfalse);
    result = invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);

    if (!NIL_P(result)) {
      void *result_cred;
      svn_auth_cred_ssl_server_trust_t *tmp_cred = NULL;

      r2c_swig_type2(result, "svn_auth_cred_ssl_server_trust_t *",
                     &result_cred);
      tmp_cred = (svn_auth_cred_ssl_server_trust_t *)result_cred;
      new_cred = apr_pcalloc(pool, sizeof(*new_cred));
      *new_cred = *tmp_cred;
    }
  }

  *cred = new_cred;
  return err;
}

svn_error_t *
svn_swig_rb_auth_ssl_client_cert_prompt_func(
  svn_auth_cred_ssl_client_cert_t **cred,
  void *baton,
  const char *realm,
  svn_boolean_t may_save,
  apr_pool_t *pool)
{
  svn_auth_cred_ssl_client_cert_t *new_cred = NULL;
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;
    VALUE result;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(2,
                           c2r_string2(realm),
                           RTEST(may_save) ? Qtrue : Qfalse);
    result = invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);

    if (!NIL_P(result)) {
      void *result_cred = NULL;
      svn_auth_cred_ssl_client_cert_t *tmp_cred = NULL;

      r2c_swig_type2(result, "svn_auth_cred_ssl_client_cert_t *",
                     &result_cred);
      tmp_cred = (svn_auth_cred_ssl_client_cert_t *)result_cred;
      new_cred = apr_pcalloc(pool, sizeof(*new_cred));
      new_cred->cert_file = tmp_cred->cert_file ?
        apr_pstrdup(pool, tmp_cred->cert_file) : NULL;
      new_cred->may_save = tmp_cred->may_save;
    }
  }

  *cred = new_cred;
  return err;
}

svn_error_t *
svn_swig_rb_auth_ssl_client_cert_pw_prompt_func(
  svn_auth_cred_ssl_client_cert_pw_t **cred,
  void *baton,
  const char *realm,
  svn_boolean_t may_save,
  apr_pool_t *pool)
{
  svn_auth_cred_ssl_client_cert_pw_t *new_cred = NULL;
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;
    VALUE result;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(2,
                           c2r_string2(realm),
                           RTEST(may_save) ? Qtrue : Qfalse);
    result = invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);

    if (!NIL_P(result)) {
      void *result_cred = NULL;
      svn_auth_cred_ssl_client_cert_pw_t *tmp_cred = NULL;

      r2c_swig_type2(result, "svn_auth_cred_ssl_client_cert_pw_t *",
                     &result_cred);
      tmp_cred = (svn_auth_cred_ssl_client_cert_pw_t *)result_cred;
      new_cred = apr_pcalloc(pool, sizeof(*new_cred));
      new_cred->password = tmp_cred->password ?
        apr_pstrdup(pool, tmp_cred->password) : NULL;
      new_cred->may_save = tmp_cred->may_save;
    }
  }

  *cred = new_cred;
  return err;
}


apr_file_t *
svn_swig_rb_make_file(VALUE file, apr_pool_t *pool)
{
  apr_file_t *apr_file = NULL;

  apr_file_open(&apr_file, StringValuePtr(file),
                APR_CREATE | APR_READ | APR_WRITE,
                APR_OS_DEFAULT,
                pool);

  return apr_file;
}


static svn_error_t *
read_handler_rbio(void *baton, char *buffer, apr_size_t *len)
{
  VALUE result;
  VALUE io = (VALUE)baton;
  svn_error_t *err = SVN_NO_ERROR;

  result = rb_funcall(io, id_read, 1, INT2NUM(*len));
  if (NIL_P(result)) {
    *len = 0;
  } else {
    memcpy(buffer, StringValuePtr(result), RSTRING_LEN(result));
    *len = RSTRING_LEN(result);
  }

  return err;
}

static svn_error_t *
write_handler_rbio(void *baton, const char *data, apr_size_t *len)
{
  VALUE io = (VALUE)baton;
  svn_error_t *err = SVN_NO_ERROR;

  rb_funcall(io, id_write, 1, rb_str_new(data, *len));

  return err;
}

svn_stream_t *
svn_swig_rb_make_stream(VALUE io)
{
  svn_stream_t *stream;

  if (RTEST(rb_funcall(rb_svn_core_stream(), id_eqq, 1, io))) {
    svn_stream_t **stream_p;
    stream_p = &stream;
    r2c_swig_type2(io, "svn_stream_t *", (void **)stream_p);
  } else {
    VALUE rb_pool = rb_pool_new(Qnil);
    apr_pool_wrapper_t *pool_wrapper;
    apr_pool_wrapper_t **pool_wrapper_p;

    rb_set_pool(io, rb_pool);
    pool_wrapper_p = &pool_wrapper;
    r2c_swig_type2(rb_pool, "apr_pool_wrapper_t *", (void **)pool_wrapper_p);
    stream = svn_stream_create((void *)io, pool_wrapper->pool);
    svn_stream_set_read(stream, read_handler_rbio);
    svn_stream_set_write(stream, write_handler_rbio);
  }

  return stream;
}

VALUE
svn_swig_rb_filename_to_temp_file(const char *file_name)
{
  return rb_funcall(rb_svn_util(), id_filename_to_temp_file,
                    1, rb_str_new2(file_name));
}

void
svn_swig_rb_set_revision(svn_opt_revision_t *rev, VALUE value)
{
  switch (TYPE(value)) {
  case T_NIL:
    rev->kind = svn_opt_revision_unspecified;
    break;
  case T_FIXNUM:
    rev->kind = svn_opt_revision_number;
    rev->value.number = NUM2LONG(value);
    break;
  case T_STRING:
    if (RTEST(rb_reg_match(rb_reg_new("^BASE$",
                                      strlen("^BASE$"),
                                      RE_OPTION_IGNORECASE),
                           value)))
      rev->kind = svn_opt_revision_base;
    else if (RTEST(rb_reg_match(rb_reg_new("^HEAD$",
                                           strlen("^HEAD$"),
                                           RE_OPTION_IGNORECASE),
                                value)))
      rev->kind = svn_opt_revision_head;
    else if (RTEST(rb_reg_match(rb_reg_new("^WORKING$",
                                           strlen("^WORKING$"),
                                           RE_OPTION_IGNORECASE),
                                value)))
      rev->kind = svn_opt_revision_working;
    else if (RTEST(rb_reg_match(rb_reg_new("^COMMITTED$",
                                           strlen("^COMMITTED$"),
                                           RE_OPTION_IGNORECASE),
                                value)))
      rev->kind = svn_opt_revision_committed;
    else if (RTEST(rb_reg_match(rb_reg_new("^PREV$",
                                           strlen("^PREV$"),
                                           RE_OPTION_IGNORECASE),
                                value)))
      rev->kind = svn_opt_revision_previous;
    else
      rb_raise(rb_eArgError,
               "invalid value: %s",
               StringValuePtr(value));
    break;
  default:
    if (rb_obj_is_kind_of(value,
                          rb_const_get(rb_cObject, rb_intern("Time")))) {
      double sec;
      double whole_sec;
      double frac_sec;

      sec = NUM2DBL(rb_funcall(value, rb_intern("to_f"), 0));
      frac_sec = modf(sec, &whole_sec);

      rev->kind = svn_opt_revision_date;
      rev->value.date = apr_time_make(whole_sec, frac_sec*APR_USEC_PER_SEC);
    } else {
      rb_raise(rb_eArgError,
               "invalid type: %s",
               rb_class2name(CLASS_OF(value)));
    }
    break;
  }
}

void
svn_swig_rb_adjust_arg_for_client_ctx_and_pool(int *argc, VALUE **argv)
{
  if (*argc > 1) {
    VALUE last_arg = (*argv)[*argc - 1];
    if (NIL_P(last_arg) || POOL_P(last_arg)) {
      *argv += *argc - 2;
      *argc = 2;
    } else {
      if (CONTEXT_P(last_arg)) {
        *argv += *argc - 1;
        *argc = 1;
      } else {
        *argv += *argc - 2;
        *argc = 2;
      }
    }
  }
}

void
svn_swig_rb_wc_status_func(void *baton,
                           const char *path,
                           svn_wc_status2_t *status)
{
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(2, c2r_string2(path), c2r_wc_status2__dup(status));
    invoke_callback((VALUE)(&cbb), rb_pool);
  }
}

svn_error_t *
svn_swig_rb_client_blame_receiver_func(void *baton,
                                       apr_int64_t line_no,
                                       svn_revnum_t revision,
                                       const char *author,
                                       const char *date,
                                       const char *line,
                                       apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(5,
                           AI642NUM(line_no),
                           INT2NUM(revision),
                           c2r_string2(author),
                           c2r_svn_date_string2(date),
                           c2r_string2(line));
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}



/* svn_wc_entry_callbacks2_t */
static svn_error_t *
wc_entry_callbacks2_found_entry(const char *path,
                                const svn_wc_entry_t *entry,
                                void *walk_baton,
                                apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE callbacks, rb_pool;

  svn_swig_rb_from_baton((VALUE)walk_baton, &callbacks, &rb_pool);

  if (!NIL_P(callbacks)) {
    callback_baton_t cbb;

    cbb.receiver = callbacks;
    cbb.message = id_found_entry;
    cbb.args = rb_ary_new3(2,
                           c2r_string2(path),
                           c2r_wc_entry__dup(entry));
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}

static svn_error_t *
wc_entry_callbacks2_handle_error(const char *path,
                                 svn_error_t *err,
                                 void *walk_baton,
                                 apr_pool_t *pool)
{
  VALUE callbacks, rb_pool;

  svn_swig_rb_from_baton((VALUE)walk_baton, &callbacks, &rb_pool);

  if (!NIL_P(callbacks)) {
    callback_baton_t cbb;
    ID message;

    message = id_handle_error;
    if (rb_obj_respond_to(callbacks, message, FALSE)) {
      VALUE rb_err;

      cbb.receiver = callbacks;
      cbb.message = id_handle_error;
      rb_err = err ? svn_swig_rb_svn_error_to_rb_error(err) : Qnil;
      if (err)
        svn_error_clear(err);
      err = NULL;
      cbb.args = rb_ary_new3(2, c2r_string2(path), rb_err);
      invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
    }
  }

  return err;
}

svn_wc_entry_callbacks2_t *
svn_swig_rb_wc_entry_callbacks2(void)
{
  static svn_wc_entry_callbacks2_t wc_entry_callbacks = {
    wc_entry_callbacks2_found_entry,
    wc_entry_callbacks2_handle_error,
  };

  return &wc_entry_callbacks;
}



/* svn_wc_diff_callbacks2_t */
static svn_error_t *
wc_diff_callbacks_file_changed(svn_wc_adm_access_t *adm_access,
                               svn_wc_notify_state_t *contentstate,
                               svn_wc_notify_state_t *propstate,
                               const char *path,
                               const char *tmpfile1,
                               const char *tmpfile2,
                               svn_revnum_t rev1,
                               svn_revnum_t rev2,
                               const char *mimetype1,
                               const char *mimetype2,
                               const apr_array_header_t *propchanges,
                               apr_hash_t *originalprops,
                               void *diff_baton)
{
  VALUE callbacks, rb_pool;
  svn_error_t *err = SVN_NO_ERROR;

  svn_swig_rb_from_baton((VALUE)diff_baton, &callbacks, &rb_pool);
  if (!NIL_P(callbacks)) {
    callback_baton_t cbb;
    VALUE result = Qnil;

    cbb.receiver = callbacks;
    cbb.message = id_file_changed;
    cbb.args = rb_ary_new3(10,
                           c2r_swig_type((void *)adm_access,
                                         (void *)"svn_wc_adm_access_t *"),
                           c2r_string2(path),
                           c2r_string2(tmpfile1),
                           c2r_string2(tmpfile2),
                           INT2NUM(rev1),
                           INT2NUM(rev2),
                           c2r_string2(mimetype1),
                           c2r_string2(mimetype2),
                           svn_swig_rb_prop_apr_array_to_hash_prop(propchanges),
                           svn_swig_rb_prop_hash_to_hash(originalprops));
    result = invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);

    if (contentstate)
      *contentstate = NUM2INT(rb_ary_entry(result, 0));
    if (propstate)
      *propstate = NUM2INT(rb_ary_entry(result, 1));
  }

  return err;
}

static svn_error_t *
wc_diff_callbacks_file_added(svn_wc_adm_access_t *adm_access,
                             svn_wc_notify_state_t *contentstate,
                             svn_wc_notify_state_t *propstate,
                             const char *path,
                             const char *tmpfile1,
                             const char *tmpfile2,
                             svn_revnum_t rev1,
                             svn_revnum_t rev2,
                             const char *mimetype1,
                             const char *mimetype2,
                             const apr_array_header_t *propchanges,
                             apr_hash_t *originalprops,
                             void *diff_baton)
{
  VALUE callbacks, rb_pool;
  svn_error_t *err = SVN_NO_ERROR;

  svn_swig_rb_from_baton((VALUE)diff_baton, &callbacks, &rb_pool);
  if (!NIL_P(callbacks)) {
    callback_baton_t cbb;
    VALUE result = Qnil;

    cbb.receiver = callbacks;
    cbb.message = id_file_added;
    cbb.args = rb_ary_new3(10,
                           c2r_swig_type((void *)adm_access,
                                         (void *)"svn_wc_adm_access_t *"),
                           c2r_string2(path),
                           c2r_string2(tmpfile1),
                           c2r_string2(tmpfile2),
                           INT2NUM(rev1),
                           INT2NUM(rev2),
                           c2r_string2(mimetype1),
                           c2r_string2(mimetype2),
                           svn_swig_rb_prop_apr_array_to_hash_prop(propchanges),
                           svn_swig_rb_prop_hash_to_hash(originalprops));
    result = invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);

    if (contentstate)
      *contentstate = NUM2INT(rb_ary_entry(result, 0));
    if (propstate)
      *propstate = NUM2INT(rb_ary_entry(result, 1));
  }

  return err;
}

static svn_error_t *
wc_diff_callbacks_file_deleted(svn_wc_adm_access_t *adm_access,
                               svn_wc_notify_state_t *state,
                               const char *path,
                               const char *tmpfile1,
                               const char *tmpfile2,
                               const char *mimetype1,
                               const char *mimetype2,
                               apr_hash_t *originalprops,
                               void *diff_baton)
{
  VALUE callbacks, rb_pool;
  svn_error_t *err = SVN_NO_ERROR;

  svn_swig_rb_from_baton((VALUE)diff_baton, &callbacks, &rb_pool);
  if (!NIL_P(callbacks)) {
    callback_baton_t cbb;
    VALUE result = Qnil;

    cbb.receiver = callbacks;
    cbb.message = id_file_deleted;
    cbb.args = rb_ary_new3(7,
                           c2r_swig_type((void *)adm_access,
                                         (void *)"svn_wc_adm_access_t *"),
                           c2r_string2(path),
                           c2r_string2(tmpfile1),
                           c2r_string2(tmpfile2),
                           c2r_string2(mimetype1),
                           c2r_string2(mimetype2),
                           svn_swig_rb_prop_hash_to_hash(originalprops));
    result = invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
    if (state)
      *state = NUM2INT(result);
  }

  return err;
}

static svn_error_t *
wc_diff_callbacks_dir_added(svn_wc_adm_access_t *adm_access,
                            svn_wc_notify_state_t *state,
                            const char *path,
                            svn_revnum_t rev,
                            void *diff_baton)
{
  VALUE callbacks, rb_pool;
  svn_error_t *err = SVN_NO_ERROR;

  svn_swig_rb_from_baton((VALUE)diff_baton, &callbacks, &rb_pool);
  if (!NIL_P(callbacks)) {
    callback_baton_t cbb;
    VALUE result = Qnil;

    cbb.receiver = callbacks;
    cbb.message = id_dir_added;
    cbb.args = rb_ary_new3(3,
                           c2r_swig_type((void *)adm_access,
                                         (void *)"svn_wc_adm_access_t *"),
                           c2r_string2(path),
                           INT2NUM(rev));
    result = invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
    if (state)
      *state = NUM2INT(result);
  }

  return err;
}

static svn_error_t *
wc_diff_callbacks_dir_deleted(svn_wc_adm_access_t *adm_access,
                              svn_wc_notify_state_t *state,
                              const char *path,
                              void *diff_baton)
{
  VALUE callbacks, rb_pool;
  svn_error_t *err = SVN_NO_ERROR;

  svn_swig_rb_from_baton((VALUE)diff_baton, &callbacks, &rb_pool);
  if (!NIL_P(callbacks)) {
    callback_baton_t cbb;
    VALUE result = Qnil;

    cbb.receiver = callbacks;
    cbb.message = id_dir_deleted;
    cbb.args = rb_ary_new3(2,
                           c2r_swig_type((void *)adm_access,
                                         (void *)"svn_wc_adm_access_t *"),
                           c2r_string2(path));
    result = invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);
    if (state)
      *state = NUM2INT(result);
  }

  return err;
}

static svn_error_t *
wc_diff_callbacks_dir_props_changed(svn_wc_adm_access_t *adm_access,
                                    svn_wc_notify_state_t *state,
                                    const char *path,
                                    const apr_array_header_t *propchanges,
                                    apr_hash_t *originalprops,
                                    void *diff_baton)
{
  VALUE callbacks, rb_pool;
  svn_error_t *err = SVN_NO_ERROR;

  svn_swig_rb_from_baton((VALUE)diff_baton, &callbacks, &rb_pool);
  if (!NIL_P(callbacks)) {
    callback_baton_t cbb;
    VALUE result = Qnil;

    cbb.receiver = callbacks;
    cbb.message = id_dir_props_changed;
    cbb.args = rb_ary_new3(4,
                           c2r_swig_type((void *)adm_access,
                                         (void *)"svn_wc_adm_access_t *"),
                           c2r_string2(path),
                           svn_swig_rb_prop_apr_array_to_hash_prop(propchanges),
                           svn_swig_rb_prop_hash_to_hash(originalprops));
    result = invoke_callback_handle_error((VALUE)(&cbb), Qnil, &err);

    if (state)
      *state = NUM2INT(result);
  }

  return err;
}

svn_wc_diff_callbacks2_t *
svn_swig_rb_wc_diff_callbacks2(void)
{
  static svn_wc_diff_callbacks2_t wc_diff_callbacks2 = {
    wc_diff_callbacks_file_changed,
    wc_diff_callbacks_file_added,
    wc_diff_callbacks_file_deleted,
    wc_diff_callbacks_dir_added,
    wc_diff_callbacks_dir_deleted,
    wc_diff_callbacks_dir_props_changed
  };

  return &wc_diff_callbacks2;
}


VALUE
svn_swig_rb_make_txdelta_window_handler_wrapper(VALUE *rb_handler_pool,
                                                apr_pool_t **handler_pool,
                                                svn_txdelta_window_handler_t **handler,
                                                void ***handler_baton)
{
  VALUE obj;

  obj = rb_class_new_instance(0, NULL, rb_cObject);
  svn_swig_rb_get_pool(0, NULL, obj, rb_handler_pool, handler_pool);
  svn_swig_rb_set_pool_for_no_swig_type(obj, *rb_handler_pool);
  *handler = apr_palloc(*handler_pool, sizeof(svn_txdelta_window_handler_t));
  *handler_baton = apr_palloc(*handler_pool, sizeof(void *));

  return obj;
}

VALUE
svn_swig_rb_setup_txdelta_window_handler_wrapper(VALUE obj,
                                                 svn_txdelta_window_handler_t handler,
                                                 void *handler_baton)
{
  rb_ivar_set(obj, id_handler,
              c2r_swig_type((void *)handler,
                            (void *)"svn_txdelta_window_handler_t"));
  rb_ivar_set(obj, id_handler_baton,
              c2r_swig_type(handler_baton, (void *)"void *"));
  return obj;
}

svn_error_t *
svn_swig_rb_invoke_txdelta_window_handler_wrapper(VALUE obj,
                                                  svn_txdelta_window_t *window,
                                                  apr_pool_t *pool)
{
  svn_txdelta_window_handler_t handler;
  svn_txdelta_window_handler_t *handler_p;
  void *handler_baton;

  handler_p = &handler;
  r2c_swig_type2(rb_ivar_get(obj, id_handler),
                 "svn_txdelta_window_handler_t", (void **)handler_p);
  r2c_swig_type2(rb_ivar_get(obj, id_handler_baton),
                 "void *", &handler_baton);

  return handler(window, handler_baton);
}


VALUE
svn_swig_rb_txdelta_window_t_ops_get(svn_txdelta_window_t *window)
{
  VALUE ops;
  const svn_txdelta_op_t *op;
  int i;

  ops = rb_ary_new2(window->num_ops);

  for (i = 0; i < window->num_ops; i++) {
    op = window->ops + i;
    rb_ary_push(ops, c2r_swig_type((void *)op, (void *)"svn_txdelta_op_t *"));
  }

  return ops;
}


svn_error_t *
svn_swig_rb_client_diff_summarize_func(const svn_client_diff_summarize_t *diff,
                                       void *baton,
                                       apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(1, c2r_client_diff_summarize__dup(diff));
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}

svn_error_t *
svn_swig_rb_client_list_func(void *baton,
                             const char *path,
                             const svn_dirent_t *dirent,
                             const svn_lock_t *lock,
                             const char *abs_path,
                             apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);

  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(4,
                           c2r_string2(path),
                           c2r_dirent__dup(dirent),
                           c2r_lock__dup(lock),
                           c2r_string2(abs_path));
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}

svn_error_t *
svn_swig_rb_proplist_receiver(void *baton,
                              const char *path,
                              apr_hash_t *prop_hash,
                              apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);
  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(2,
                           c2r_string2(path),
                           svn_swig_rb_prop_hash_to_hash(prop_hash));
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}

svn_error_t *
svn_swig_rb_changelist_receiver(void *baton,
                                const char *path,
                                const char *changelist,
                                apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE proc, rb_pool;

  svn_swig_rb_from_baton((VALUE)baton, &proc, &rb_pool);
  if (!NIL_P(proc)) {
    callback_baton_t cbb;

    cbb.receiver = proc;
    cbb.message = id_call;
    cbb.args = rb_ary_new3(2,
                           c2r_string2(path),
                           c2r_string2(changelist));
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}


/* svn_ra_reporter3_t */
static void
c2r_ra_reporter3(VALUE rb_reporter, svn_ra_reporter3_t **reporter, void **baton,
                 apr_pool_t *pool)
{
    VALUE rb_baton;

    r2c_swig_type2(rb_reporter, "svn_ra_reporter3_t *", (void **)reporter);

    rb_baton = rb_funcall(rb_reporter, id_baton, 0);
    r2c_swig_type2(rb_baton, "void *", baton);
}

static svn_error_t *
svn_swig_rb_ra_reporter_set_path(void *report_baton, const char *path,
                                 svn_revnum_t revision, svn_depth_t depth,
                                 svn_boolean_t start_empty,
                                 const char *lock_token, apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE reporter, rb_pool;

  svn_swig_rb_from_baton((VALUE)report_baton, &reporter, &rb_pool);
  if (rb_obj_is_kind_of(reporter, rb_svn_ra_reporter3())) {
    svn_ra_reporter3_t *svn_reporter;
    void *baton;

    c2r_ra_reporter3(reporter, &svn_reporter, &baton, pool);
    err = svn_reporter->set_path(baton, path, revision, depth,
                                 start_empty, lock_token, pool);
  } else if (!NIL_P(reporter)) {
    callback_baton_t cbb;

    cbb.receiver = reporter;
    cbb.message = id_set_path;
    cbb.args = rb_ary_new3(4,
                           c2r_string2(path),
                           INT2NUM(revision),
                           INT2NUM(depth),
                           c2r_bool2(start_empty));
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}

static svn_error_t *
svn_swig_rb_ra_reporter_delete_path(void *report_baton, const char *path,
                                    apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE reporter, rb_pool;

  svn_swig_rb_from_baton((VALUE)report_baton, &reporter, &rb_pool);
  if (rb_obj_is_kind_of(reporter, rb_svn_ra_reporter3())) {
    svn_ra_reporter3_t *svn_reporter;
    void *baton;

    c2r_ra_reporter3(reporter, &svn_reporter, &baton, pool);
    err = svn_reporter->delete_path(baton, path, pool);
  } else if (!NIL_P(reporter)) {
    callback_baton_t cbb;

    cbb.receiver = reporter;
    cbb.message = id_delete_path;
    cbb.args = rb_ary_new3(1,
                           c2r_string2(path));
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}

static svn_error_t *
svn_swig_rb_ra_reporter_link_path(void *report_baton, const char *path,
                                  const char *url, svn_revnum_t revision,
                                  svn_depth_t depth, svn_boolean_t start_empty,
                                  const char *lock_token, apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE reporter, rb_pool;

  svn_swig_rb_from_baton((VALUE)report_baton, &reporter, &rb_pool);
  if (rb_obj_is_kind_of(reporter, rb_svn_ra_reporter3())) {
    svn_ra_reporter3_t *svn_reporter;
    void *baton;

    c2r_ra_reporter3(reporter, &svn_reporter, &baton, pool);
    err = svn_reporter->link_path(baton, path, url, revision, depth,
                                  start_empty, lock_token, pool);
  } else if (!NIL_P(reporter)) {
    callback_baton_t cbb;

    cbb.receiver = reporter;
    cbb.message = id_link_path;
    cbb.args = rb_ary_new3(5,
                           c2r_string2(path),
                           c2r_string2(url),
                           INT2NUM(revision),
                           INT2NUM(depth),
                           c2r_bool2(start_empty));
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}

static svn_error_t *
svn_swig_rb_ra_reporter_finish_report(void *report_baton, apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE reporter, rb_pool;

  svn_swig_rb_from_baton((VALUE)report_baton, &reporter, &rb_pool);
  if (rb_obj_is_kind_of(reporter, rb_svn_ra_reporter3())) {
    svn_ra_reporter3_t *svn_reporter;
    void *baton;

    c2r_ra_reporter3(reporter, &svn_reporter, &baton, pool);
    err = svn_reporter->finish_report(baton, pool);
  } else if (!NIL_P(reporter)) {
    callback_baton_t cbb;

    cbb.receiver = reporter;
    cbb.message = id_finish_report;
    cbb.args = rb_ary_new();
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}

static svn_error_t *
svn_swig_rb_ra_reporter_abort_report(void *report_baton, apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  VALUE reporter, rb_pool;

  svn_swig_rb_from_baton((VALUE)report_baton, &reporter, &rb_pool);
  if (rb_obj_is_kind_of(reporter, rb_svn_ra_reporter3())) {
    svn_ra_reporter3_t *svn_reporter;
    void *baton;

    c2r_ra_reporter3(reporter, &svn_reporter, &baton, pool);
    err = svn_reporter->abort_report(baton, pool);
  } else if (!NIL_P(reporter)) {
    callback_baton_t cbb;

    cbb.receiver = reporter;
    cbb.message = id_abort_report;
    cbb.args = rb_ary_new();
    invoke_callback_handle_error((VALUE)(&cbb), rb_pool, &err);
  }

  return err;
}

static svn_ra_reporter3_t rb_ra_reporter3 = {
  svn_swig_rb_ra_reporter_set_path,
  svn_swig_rb_ra_reporter_delete_path,
  svn_swig_rb_ra_reporter_link_path,
  svn_swig_rb_ra_reporter_finish_report,
  svn_swig_rb_ra_reporter_abort_report
};

svn_ra_reporter3_t *svn_swig_rb_ra_reporter3 = &rb_ra_reporter3;

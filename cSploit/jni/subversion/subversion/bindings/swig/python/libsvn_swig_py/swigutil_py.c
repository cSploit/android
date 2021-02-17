/*
 * swigutil_py.c: utility functions for the SWIG Python bindings
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

/* Tell swigutil_py.h that we're inside the implementation */
#define SVN_SWIG_SWIGUTIL_PY_C

#include <Python.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <apr_pools.h>
#include <apr_hash.h>
#include <apr_portable.h>
#include <apr_thread_proc.h>

#include "svn_client.h"
#include "svn_string.h"
#include "svn_opt.h"
#include "svn_delta.h"
#include "svn_auth.h"
#include "svn_props.h"
#include "svn_pools.h"
#include "svn_mergeinfo.h"
#include "svn_types.h"

#include "svn_private_config.h" /* for SVN_APR_INT64_T_PYCFMT */

#include "swig_python_external_runtime.swg"
#include "swigutil_py.h"

/* Py_ssize_t for old Pythons */
/* This code is as recommended by: */
/* http://www.python.org/dev/peps/pep-0353/#conversion-guidelines */
#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
# define PY_SSIZE_T_MAX INT_MAX
# define PY_SSIZE_T_MIN INT_MIN
#endif


/*** Manage the Global Interpreter Lock ***/

/* If both Python and APR have threads available, we can optimize ourselves
 * by releasing the global interpreter lock when we drop into our SVN calls.
 *
 * In svn_types.i, svn_swig_py_release_py_lock is called before every
 * function, then svn_swig_py_acquire_py_lock is called after every
 * function.  So, if these functions become no-ops, then Python will
 * start to block...
 *
 * The Subversion libraries can be assumed to be thread-safe *only* when
 * APR_HAS_THREAD is 1.  The APR pool allocations aren't thread-safe unless
 * APR_HAS_THREAD is 1.
 */

#if defined(WITH_THREAD) && APR_HAS_THREADS
#define ACQUIRE_PYTHON_LOCK
#endif

#ifdef ACQUIRE_PYTHON_LOCK
static apr_threadkey_t *_saved_thread_key = NULL;
static apr_pool_t *_saved_thread_pool = NULL;
#endif

void svn_swig_py_release_py_lock(void)
{
#ifdef ACQUIRE_PYTHON_LOCK
  PyThreadState *thread_state;

  if (_saved_thread_key == NULL)
    {
      /* Obviously, creating a top-level pool for this is pretty stupid. */
      apr_pool_create(&_saved_thread_pool, NULL);
      apr_threadkey_private_create(&_saved_thread_key, NULL,
                                   _saved_thread_pool);
    }

  thread_state = PyEval_SaveThread();
  apr_threadkey_private_set(thread_state, _saved_thread_key);
#endif
}

void svn_swig_py_acquire_py_lock(void)
{
#ifdef ACQUIRE_PYTHON_LOCK
  void *val;
  PyThreadState *thread_state;
  apr_threadkey_private_get(&val, _saved_thread_key);
  thread_state = val;
  PyEval_RestoreThread(thread_state);
#endif
}



/*** Automatic Pool Management Functions ***/

/* The application pool */
static apr_pool_t *application_pool = NULL;
static PyObject *application_py_pool = NULL;
static char assertValid[] = "assert_valid";
static char markValid[] = "_mark_valid";
static char parentPool[] = "_parent_pool";
static char wrap[] = "_wrap";
static char unwrap[] = "_unwrap";
static char setParentPool[] = "set_parent_pool";
static char emptyTuple[] = "()";
static char objectTuple[] = "(O)";


apr_status_t svn_swig_py_initialize(void)
{
  apr_status_t status;

  if ((status = apr_initialize()) != APR_SUCCESS)
    return status;
  if (atexit(apr_terminate) != 0)
    return APR_EGENERAL;
  return APR_SUCCESS;
}

int svn_swig_py_get_pool_arg(PyObject *args, swig_type_info *type,
    PyObject **py_pool, apr_pool_t **pool)
{
  int argnum = PyTuple_GET_SIZE(args) - 1;

  if (argnum >= 0)
    {
      PyObject *input = PyTuple_GET_ITEM(args, argnum);
      if (input != Py_None && PyObject_HasAttrString(input, markValid))
        {
          *pool = svn_swig_MustGetPtr(input, type, argnum+1);
          if (*pool == NULL)
            return 1;
          *py_pool = input;
          Py_INCREF(input);
          return 0;
        }
    }

  /* We couldn't find a pool argument, so we'll create a subpool */
  *pool = svn_pool_create(application_pool);
  *py_pool = svn_swig_NewPointerObj(*pool, type, application_py_pool,
                                    NULL);
  if (*py_pool == NULL)
    return 1;

  return 0;
}

int svn_swig_py_get_parent_pool(PyObject *args, swig_type_info *type,
    PyObject **py_pool, apr_pool_t **pool)
{
  PyObject *proxy = PyTuple_GetItem(args, 0);

  if (proxy == NULL)
    return 1;

  *py_pool = PyObject_GetAttrString(proxy, parentPool);

  if (*py_pool == NULL)
    {
      PyErr_SetString(PyExc_TypeError,
             "Unexpected NULL parent pool on proxy object");
      return 1;
    }

  Py_DECREF(*py_pool);

  *pool = svn_swig_MustGetPtr(*py_pool, type, 1);

  if (*pool == NULL)
    return 1;

  return 0;
}

/* Set the application pool */
void svn_swig_py_set_application_pool(PyObject *py_pool, apr_pool_t *pool)
{
  application_pool = pool;
  application_py_pool = py_pool;
}

/* Clear the application pool */
void svn_swig_py_clear_application_pool(void)
{
  application_pool = NULL;
  application_py_pool = NULL;
}

/* Set the parent pool of a proxy object */
static int proxy_set_pool(PyObject **proxy, PyObject *pool)
{
  PyObject *result;

  if (*proxy != NULL)
    {
      if (pool == NULL)
        {
          if (PyObject_HasAttrString(*proxy, setParentPool))
            {
              result = PyObject_CallMethod(*proxy, setParentPool, emptyTuple);
              if (result == NULL)
                return 1;
              Py_DECREF(result);
            }
        }
      else
        {
          result = PyObject_CallMethod(pool, wrap, objectTuple, *proxy);
          Py_DECREF(*proxy);
          *proxy = result;
        }
    }

  return 0;
}


/* Wrapper for SWIG_TypeQuery */
#define svn_swig_TypeQuery(x) SWIG_TypeQuery(x)

/** Wrapper for SWIG_NewPointerObj */
PyObject *svn_swig_NewPointerObj(void *obj, swig_type_info *type,
                                 PyObject *pool, PyObject *args)
{
  PyObject *proxy = SWIG_NewPointerObj(obj, type, 0);

  if (proxy == NULL)
    return NULL;

  if (pool == NULL && args != NULL)
    {
      apr_pool_t *tmp;
      if (svn_swig_py_get_parent_pool(args,
            svn_swig_TypeQuery("apr_pool_t *"), &pool, &tmp))
        PyErr_Clear();
    }

  if (proxy_set_pool(&proxy, pool))
    {
      Py_DECREF(proxy);
      return NULL;
    }

  return proxy;
}

/** svn_swig_NewPointerObj, except a string is used to describe the type */
static PyObject *svn_swig_NewPointerObjString(void *ptr, const char *type,
                                              PyObject *py_pool)
{
  swig_type_info *typeinfo = svn_swig_TypeQuery(type);
  if (typeinfo == NULL)
    {
      PyErr_SetString(PyExc_TypeError, "Cannot find required typeobject");
      return NULL;
    }

  /* ### cache the swig_type_info at some point? */
  return svn_swig_NewPointerObj(ptr, typeinfo, py_pool, NULL);
}

/** Wrapper for SWIG_ConvertPtr */
int svn_swig_ConvertPtr(PyObject *input, void **obj, swig_type_info *type)
{
  if (PyObject_HasAttrString(input, assertValid))
    {
      PyObject *result = PyObject_CallMethod(input, assertValid, emptyTuple);
      if (result == NULL)
        return 1;
      Py_DECREF(result);
    }
  if (PyObject_HasAttrString(input, unwrap))
    {
      input = PyObject_CallMethod(input, unwrap, emptyTuple);
      if (input == NULL)
        return 1;
      Py_DECREF(input);
    }

  return SWIG_ConvertPtr(input, obj, type, SWIG_POINTER_EXCEPTION | 0);
}

/** svn_swig_ConvertPtr, except a string is used to describe the type */
static int svn_swig_ConvertPtrString(PyObject *input,
    void **obj, const char *type)
{
  return svn_swig_ConvertPtr(input, obj, svn_swig_TypeQuery(type));
}

/** Wrapper for SWIG_MustGetPtr */
void *svn_swig_MustGetPtr(void *input, swig_type_info *type, int argnum)
{
  if (PyObject_HasAttrString(input, assertValid))
    {
      PyObject *result = PyObject_CallMethod(input, assertValid, emptyTuple);
      if (result == NULL)
        return NULL;
      Py_DECREF(result);
    }

  if (PyObject_HasAttrString(input, unwrap))
    {
      input = PyObject_CallMethod(input, unwrap, emptyTuple);
      if (input == NULL)
        return NULL;
      Py_DECREF((PyObject *) input);
    }

  return SWIG_MustGetPtr(input, type, argnum, SWIG_POINTER_EXCEPTION | 0);
}



/*** Custom SubversionException stuffs. ***/

void svn_swig_py_svn_exception(svn_error_t *error_chain)
{
  PyObject *args_list, *args, *apr_err_ob, *message_ob, *file_ob, *line_ob;
  PyObject *svn_module, *exc_class, *exc_ob;
  svn_error_t *err;

  if (error_chain == NULL)
    return;

  /* Start with no references. */
  args_list = args = apr_err_ob = message_ob = file_ob = line_ob = NULL;
  svn_module = exc_class = exc_ob = NULL;

  if ((args_list = PyList_New(0)) == NULL)
    goto finished;

  for (err = error_chain; err; err = err->child)
    {
      int i;

      if ((args = PyTuple_New(4)) == NULL)
        goto finished;

      /* Convert the fields of the svn_error_t to Python objects. */
      if ((apr_err_ob = PyInt_FromLong(err->apr_err)) == NULL)
        goto finished;
      if (err->message == NULL)
        {
          Py_INCREF(Py_None);
          message_ob = Py_None;
        }
      else if ((message_ob = PyString_FromString(err->message)) == NULL)
        goto finished;
      if (err->file == NULL)
        {
          Py_INCREF(Py_None);
          file_ob = Py_None;
        }
      else if ((file_ob = PyString_FromString(err->file)) == NULL)
        goto finished;
      if ((line_ob = PyInt_FromLong(err->line)) == NULL)
        goto finished;

      /* Store the objects in the tuple. */
      i = 0;
#define append(item)                                            \
      if (PyTuple_SetItem(args, i++, item) == 0)                \
        /* tuple stole our reference, so don't DECREF */        \
        item = NULL;                                            \
      else                                                      \
        goto finished;
      append(apr_err_ob);
      append(message_ob);
      append(file_ob);
      append(line_ob);
#undef append

      /* Append the tuple to the args list. */
      if (PyList_Append(args_list, args) == -1)
        goto finished;
      /* The list takes its own reference, so release ours. */
      Py_DECREF(args);
      /* Let's not decref in 'finished:' after the final iteration. */
      args = NULL;
    }
  svn_error_clear(error_chain);

  /* Create the exception object chain. */
  if ((svn_module = PyImport_ImportModule((char *)"svn.core")) == NULL)
    goto finished;
  if ((exc_class = PyObject_GetAttrString(svn_module,
                                       (char *)"SubversionException")) == NULL)
    goto finished;
  if ((exc_ob = PyObject_CallMethod(exc_class, (char *)"_new_from_err_list",
                                    (char *)"O", args_list)) == NULL)
    goto finished;

  /* Raise the exception. */
  PyErr_SetObject(exc_class, exc_ob);

 finished:
  /* Release any references. */
  Py_XDECREF(args_list);
  Py_XDECREF(args);
  Py_XDECREF(apr_err_ob);
  Py_XDECREF(message_ob);
  Py_XDECREF(file_ob);
  Py_XDECREF(line_ob);
  Py_XDECREF(svn_module);
  Py_XDECREF(exc_class);
  Py_XDECREF(exc_ob);
}



/*** Helper/Conversion Routines ***/

/* Functions for making Python wrappers around Subversion structs */
static PyObject *make_ob_pool(void *pool)
{
  /* Return a brand new default pool to Python. This pool isn't
   * normally used for anything. It's just here for compatibility
   * with Subversion 1.2. */
  apr_pool_t *new_pool = svn_pool_create(application_pool);
  PyObject *new_py_pool = svn_swig_NewPointerObj(new_pool,
    svn_swig_TypeQuery("apr_pool_t *"), application_py_pool, NULL);
  (void) pool; /* Silence compiler warnings about unused parameter. */
  return new_py_pool;
}
static PyObject *make_ob_fs_root(svn_fs_root_t *ptr, PyObject *py_pool)
{
  return svn_swig_NewPointerObjString(ptr, "svn_fs_root_t *", py_pool);
}
static PyObject *make_ob_wc_adm_access(void *adm_access)
{
  return svn_swig_NewPointerObjString(adm_access,
                                      "svn_wc_adm_access_t *",
                                      NULL);
}


/***/

/* Conversion from Python single objects (not hashes/lists/etc.) to
   Subversion types. */
static const char *make_string_from_ob(PyObject *ob, apr_pool_t *pool)
{
  if (ob == Py_None)
    return NULL;
  if (! PyString_Check(ob))
    {
      PyErr_SetString(PyExc_TypeError, "not a string");
      return NULL;
    }
  return apr_pstrdup(pool, PyString_AS_STRING(ob));
}
static svn_string_t *make_svn_string_from_ob(PyObject *ob, apr_pool_t *pool)
{
  if (ob == Py_None)
    return NULL;
  if (! PyString_Check(ob))
    {
      PyErr_SetString(PyExc_TypeError, "not a string");
      return NULL;
    }
  return svn_string_create(PyString_AS_STRING(ob), pool);
}


/***/

static PyObject *convert_hash(apr_hash_t *hash,
                              PyObject * (*converter_func)(void *value,
                                                           void *ctx,
                                                           PyObject *py_pool),
                              void *ctx, PyObject *py_pool)
{
    apr_hash_index_t *hi;
    PyObject *dict;

    if (hash == NULL)
        Py_RETURN_NONE;

    if ((dict = PyDict_New()) == NULL)
        return NULL;

    for (hi = apr_hash_first(NULL, hash); hi; hi = apr_hash_next(hi))
      {
        const void *key;
        void *val;
        PyObject *value;

        apr_hash_this(hi, &key, NULL, &val);
        value = (*converter_func)(val, ctx, py_pool);
        if (value == NULL)
          {
            Py_DECREF(dict);
            return NULL;
          }
        /* ### gotta cast this thing cuz Python doesn't use "const" */
        if (PyDict_SetItemString(dict, (char *)key, value) == -1)
          {
            Py_DECREF(value);
            Py_DECREF(dict);
            return NULL;
          }
        Py_DECREF(value);
      }

    return dict;
}

static PyObject *convert_to_swigtype(void *value, void *ctx, PyObject *py_pool)
{
  /* ctx is a 'swig_type_info *' */
  return svn_swig_NewPointerObj(value, ctx, py_pool, NULL);
}

static PyObject *convert_svn_string_t(void *value, void *ctx,
                                      PyObject *py_pool)
{
  /* ctx is unused */

  const svn_string_t *s = value;

  /* ### gotta cast this thing cuz Python doesn't use "const" */
  return PyString_FromStringAndSize((void *)s->data, s->len);
}

static PyObject *convert_svn_client_commit_item3_t(void *value, void *ctx)
{
  PyObject *list;
  PyObject *path, *kind, *url, *rev, *cf_url, *cf_rev, *state,
      *incoming_prop_changes, *outgoing_prop_changes;
  svn_client_commit_item3_t *item = value;

  /* ctx is unused */

  list = PyList_New(9);

  if (item->path)
    path = PyString_FromString(item->path);
  else
    {
      path = Py_None;
      Py_INCREF(Py_None);
    }

  if (item->url)
    url = PyString_FromString(item->url);
  else
    {
      url = Py_None;
      Py_INCREF(Py_None);
    }

  if (item->copyfrom_url)
    cf_url = PyString_FromString(item->copyfrom_url);
  else
    {
      cf_url = Py_None;
      Py_INCREF(Py_None);
    }

  kind = PyInt_FromLong(item->kind);
  rev = PyInt_FromLong(item->revision);
  cf_rev = PyInt_FromLong(item->copyfrom_rev);
  state = PyInt_FromLong(item->state_flags);

  if (item->incoming_prop_changes)
    incoming_prop_changes =
      svn_swig_py_array_to_list(item->incoming_prop_changes);
  else
    {
      incoming_prop_changes = Py_None;
      Py_INCREF(Py_None);
    }

  if (item->outgoing_prop_changes)
    outgoing_prop_changes =
      svn_swig_py_array_to_list(item->outgoing_prop_changes);
  else
    {
      outgoing_prop_changes = Py_None;
      Py_INCREF(Py_None);
    }

  if (! (list && path && kind && url && rev && cf_url && cf_rev && state &&
         incoming_prop_changes && outgoing_prop_changes))
    {
      Py_XDECREF(list);
      Py_XDECREF(path);
      Py_XDECREF(kind);
      Py_XDECREF(url);
      Py_XDECREF(rev);
      Py_XDECREF(cf_url);
      Py_XDECREF(cf_rev);
      Py_XDECREF(state);
      Py_XDECREF(incoming_prop_changes);
      Py_XDECREF(outgoing_prop_changes);
      return NULL;
    }

  PyList_SET_ITEM(list, 0, path);
  PyList_SET_ITEM(list, 1, kind);
  PyList_SET_ITEM(list, 2, url);
  PyList_SET_ITEM(list, 3, rev);
  PyList_SET_ITEM(list, 4, cf_url);
  PyList_SET_ITEM(list, 5, cf_rev);
  PyList_SET_ITEM(list, 6, state);
  PyList_SET_ITEM(list, 7, incoming_prop_changes);
  PyList_SET_ITEM(list, 8, outgoing_prop_changes);
  return list;
}

PyObject *svn_swig_py_prophash_to_dict(apr_hash_t *hash)
{
  return convert_hash(hash, convert_svn_string_t, NULL, NULL);
}

static PyObject *convert_string(void *value, void *ctx,
                                PyObject *py_pool)
{
  /* ### gotta cast this thing cuz Python doesn't use "const" */
  return PyString_FromString((const char *)value);
}

PyObject *svn_swig_py_stringhash_to_dict(apr_hash_t *hash)
{
  return convert_hash(hash, convert_string, NULL, NULL);
}

static PyObject *convert_pointerlist(void *value, void *ctx, PyObject *py_pool)
{
  int i;
  PyObject *list;
  apr_array_header_t *array = value;

  list = PyList_New(0);
  if (list == NULL)
    return NULL;

  for (i = 0; i < array->nelts; i++)
    {
      void *ptr = APR_ARRAY_IDX(array, i, void *);
      PyObject *obj;
      int result;

      obj = convert_to_swigtype(ptr, ctx, py_pool);
      if (obj == NULL)
        goto error;

      result = PyList_Append(list, obj);
      Py_DECREF(obj);
      if (result == -1)
        goto error;
    }
  return list;
 error:
  Py_DECREF(list);
  return NULL;
}

PyObject *svn_swig_py_pointerlist_to_list(apr_array_header_t *list,
                                          swig_type_info *type,
                                          PyObject *py_pool)
{
  return convert_pointerlist(list, type, py_pool);
}

PyObject *svn_swig_py_mergeinfo_to_dict(apr_hash_t *hash,
                                        swig_type_info *type,
                                        PyObject *py_pool)
{
  return convert_hash(hash, convert_pointerlist, type, py_pool);
}

static PyObject *convert_mergeinfo_hash(void *value, void *ctx,
                                         PyObject *py_pool)
{
  return svn_swig_py_mergeinfo_to_dict(value, ctx, py_pool);
}

PyObject *svn_swig_py_mergeinfo_catalog_to_dict(apr_hash_t *hash,
                                                swig_type_info *type,
                                                PyObject *py_pool)
{
  return convert_hash(hash, convert_mergeinfo_hash, type, py_pool);
}

PyObject *svn_swig_py_proparray_to_dict(const apr_array_header_t *array)
{
    PyObject *dict = PyDict_New();
    int i;

    if (dict == NULL)
      return NULL;

    for (i = 0; i < array->nelts; ++i)
      {
        svn_prop_t prop;
        PyObject *py_key, *py_value;

        prop = APR_ARRAY_IDX(array, i, svn_prop_t);

        py_key = PyString_FromString(prop.name);
        if (py_key == NULL)
          goto error;

        if (prop.value == NULL)
          {
             py_value = Py_None;
             Py_INCREF(Py_None);
          }
        else
          {
             py_value = PyString_FromStringAndSize((void *)prop.value->data,
                                                   prop.value->len);
             if (py_value == NULL)
               {
                 Py_DECREF(py_key);
                  goto error;
               }
          }

        PyDict_SetItem(dict, py_key, py_value);
    }

    return dict;

  error:
    Py_DECREF(dict);
    return NULL;

}

PyObject *svn_swig_py_locationhash_to_dict(apr_hash_t *hash)
{
    /* Need special code for this because of the darned svn_revnum_t
       keys. */
    apr_hash_index_t *hi;
    PyObject *dict = PyDict_New();

    if (dict == NULL)
        return NULL;

    for (hi = apr_hash_first(NULL, hash); hi; hi = apr_hash_next(hi))
      {
        const void *k;
        void *v;
        PyObject *key, *value;

        apr_hash_this(hi, &k, NULL, &v);
        key = PyLong_FromLong(*(svn_revnum_t *)k);
        if (key == NULL)
          {
            Py_DECREF(dict);
            return NULL;
          }
        value = PyString_FromString((char *)v);
        if (value == NULL)
          {
            Py_DECREF(key);
            Py_DECREF(dict);
            return NULL;
          }
        if (PyDict_SetItem(dict, key, value) == -1)
          {
            Py_DECREF(value);
            Py_DECREF(dict);
            return NULL;
          }
        Py_DECREF(value);
        Py_DECREF(key);
      }
    return dict;
}

PyObject *svn_swig_py_convert_hash(apr_hash_t *hash, swig_type_info *type,
                                   PyObject *py_pool)
{
  return convert_hash(hash, convert_to_swigtype, type, py_pool);
}

#define DECLARE_SWIG_CONSTRUCTOR(type, dup) \
static PyObject *make_ob_##type(void *value) \
{ \
  apr_pool_t *new_pool = svn_pool_create(application_pool); \
  PyObject *new_py_pool = svn_swig_NewPointerObj(new_pool, \
    svn_swig_TypeQuery("apr_pool_t *"), application_py_pool, NULL); \
  svn_##type##_t *new_value = dup(value, new_pool); \
  PyObject *obj = svn_swig_NewPointerObjString(new_value, "svn_" #type "_t *", \
                                               new_py_pool); \
  Py_XDECREF(new_py_pool); \
  return obj; \
}

DECLARE_SWIG_CONSTRUCTOR(txdelta_window, svn_txdelta_window_dup)
DECLARE_SWIG_CONSTRUCTOR(log_changed_path, svn_log_changed_path_dup)
DECLARE_SWIG_CONSTRUCTOR(log_changed_path2, svn_log_changed_path2_dup)
DECLARE_SWIG_CONSTRUCTOR(wc_status, svn_wc_dup_status)
DECLARE_SWIG_CONSTRUCTOR(lock, svn_lock_dup)
DECLARE_SWIG_CONSTRUCTOR(auth_ssl_server_cert_info,
    svn_auth_ssl_server_cert_info_dup)
DECLARE_SWIG_CONSTRUCTOR(info, svn_info_dup)
DECLARE_SWIG_CONSTRUCTOR(location_segment, svn_location_segment_dup)
DECLARE_SWIG_CONSTRUCTOR(commit_info, svn_commit_info_dup)
DECLARE_SWIG_CONSTRUCTOR(wc_notify, svn_wc_dup_notify)

static PyObject *convert_log_changed_path(void *value, void *ctx,
                                          PyObject *py_pool)
{
  return make_ob_log_changed_path(value);
}

PyObject *svn_swig_py_c_strings_to_list(char **strings)
{
    PyObject *list = PyList_New(0);
    char *s;

    while ((s = *strings++) != NULL)
      {
        PyObject *ob = PyString_FromString(s);

        if (ob == NULL)
            goto error;
        if (PyList_Append(list, ob) == -1)
            goto error;
      }

    return list;

  error:
    Py_DECREF(list);
    return NULL;
}

PyObject *svn_swig_py_changed_path_hash_to_dict(apr_hash_t *hash)
{
  apr_hash_index_t *hi;
  PyObject *dict;

  if (hash == NULL)
    Py_RETURN_NONE;

  if ((dict = PyDict_New()) == NULL)
    return NULL;

  for (hi = apr_hash_first(NULL, hash); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      void *val;
      PyObject *value;

      apr_hash_this(hi, &key, NULL, &val);
      value = make_ob_log_changed_path(val);
      if (value == NULL)
        {
            Py_DECREF(dict);
            return NULL;
        }
      if (PyDict_SetItemString(dict, (char *)key, value) == -1)
        {
          Py_DECREF(value);
          Py_DECREF(dict);
          return NULL;
        }
      Py_DECREF(value);
    }

  return dict;
}

PyObject *svn_swig_py_changed_path2_hash_to_dict(apr_hash_t *hash)
{
  apr_hash_index_t *hi;
  PyObject *dict;

  if (hash == NULL)
    Py_RETURN_NONE;

  if ((dict = PyDict_New()) == NULL)
    return NULL;

  for (hi = apr_hash_first(NULL, hash); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      void *val;
      PyObject *value;

      apr_hash_this(hi, &key, NULL, &val);
      value = make_ob_log_changed_path2(val);
      if (value == NULL)
        {
            Py_DECREF(dict);
            return NULL;
        }
      if (PyDict_SetItemString(dict, (char *)key, value) == -1)
        {
          Py_DECREF(value);
          Py_DECREF(dict);
          return NULL;
        }
      Py_DECREF(value);
    }

  return dict;
}

apr_hash_t *svn_swig_py_stringhash_from_dict(PyObject *dict,
                                             apr_pool_t *pool)
{
  apr_hash_t *hash;
  PyObject *keys;
  int i, num_keys;

  if (dict == Py_None)
    return NULL;

  if (!PyDict_Check(dict))
    {
      PyErr_SetString(PyExc_TypeError, "not a dictionary");
      return NULL;
    }

  hash = apr_hash_make(pool);
  keys = PyDict_Keys(dict);
  num_keys = PyList_Size(keys);
  for (i = 0; i < num_keys; i++)
    {
      PyObject *key = PyList_GetItem(keys, i);
      PyObject *value = PyDict_GetItem(dict, key);
      const char *propname = make_string_from_ob(key, pool);
      const char *propval = make_string_from_ob(value, pool);
      if (! (propname && propval))
        {
          PyErr_SetString(PyExc_TypeError,
                          "dictionary keys/values aren't strings");
          Py_DECREF(keys);
          return NULL;
        }
      apr_hash_set(hash, propname, APR_HASH_KEY_STRING, propval);
    }
  Py_DECREF(keys);
  return hash;
}

apr_hash_t *svn_swig_py_mergeinfo_from_dict(PyObject *dict,
                                            apr_pool_t *pool)
{
  apr_hash_t *hash;
  PyObject *keys;
  int i, num_keys;

  if (dict == Py_None)
    return NULL;

  if (!PyDict_Check(dict)) {
    PyErr_SetString(PyExc_TypeError, "not a dictionary");
    return NULL;
  }

  hash = apr_hash_make(pool);
  keys = PyDict_Keys(dict);
  num_keys = PyList_Size(keys);
  for (i = 0; i < num_keys; i++)
    {
      PyObject *key = PyList_GetItem(keys, i);
      PyObject *value = PyDict_GetItem(dict, key);
      const char *pathname = make_string_from_ob(key, pool);
      const apr_array_header_t *ranges = svn_swig_py_seq_to_array(value,
        sizeof(const svn_merge_range_t *),
        svn_swig_py_unwrap_struct_ptr,
        svn_swig_TypeQuery("svn_merge_range_t *"),
        pool
      );

      if (! (pathname && ranges))
        {
          PyErr_SetString(PyExc_TypeError,
                          "dictionary keys aren't strings or values aren't svn_merge_range_t *'s");
          Py_DECREF(keys);
          return NULL;
        }
      apr_hash_set(hash, pathname, APR_HASH_KEY_STRING, ranges);
    }
  Py_DECREF(keys);
  return hash;
}

apr_array_header_t *svn_swig_py_proparray_from_dict(PyObject *dict,
                                                    apr_pool_t *pool)
{
  apr_array_header_t *array;
  PyObject *keys;
  int i, num_keys;

  if (dict == Py_None)
    return NULL;

  if (!PyDict_Check(dict))
    {
      PyErr_SetString(PyExc_TypeError, "not a dictionary");
      return NULL;
    }

  keys = PyDict_Keys(dict);
  num_keys = PyList_Size(keys);
  array = apr_array_make(pool, num_keys, sizeof(svn_prop_t *));
  for (i = 0; i < num_keys; i++)
    {
      PyObject *key = PyList_GetItem(keys, i);
      PyObject *value = PyDict_GetItem(dict, key);
      svn_prop_t *prop = apr_palloc(pool, sizeof(*prop));
      prop->name = make_string_from_ob(key, pool);
      prop->value = make_svn_string_from_ob(value, pool);
      if (! (prop->name && prop->value))
        {
          PyErr_SetString(PyExc_TypeError,
                          "dictionary keys/values aren't strings");
          Py_DECREF(keys);
          return NULL;
        }
      APR_ARRAY_PUSH(array, svn_prop_t *) = prop;
    }
  Py_DECREF(keys);
  return array;
}

apr_hash_t *svn_swig_py_prophash_from_dict(PyObject *dict,
                                           apr_pool_t *pool)
{
  apr_hash_t *hash;
  PyObject *keys;
  int i, num_keys;

  if (dict == Py_None)
    return NULL;

  if (!PyDict_Check(dict))
    {
      PyErr_SetString(PyExc_TypeError, "not a dictionary");
      return NULL;
    }

  hash = apr_hash_make(pool);
  keys = PyDict_Keys(dict);
  num_keys = PyList_Size(keys);
  for (i = 0; i < num_keys; i++)
    {
      PyObject *key = PyList_GetItem(keys, i);
      PyObject *value = PyDict_GetItem(dict, key);
      const char *propname = make_string_from_ob(key, pool);
      svn_string_t *propval = make_svn_string_from_ob(value, pool);
      if (! (propname && propval))
        {
          PyErr_SetString(PyExc_TypeError,
                          "dictionary keys/values aren't strings");
          Py_DECREF(keys);
          return NULL;
        }
      apr_hash_set(hash, propname, APR_HASH_KEY_STRING, propval);
    }
  Py_DECREF(keys);
  return hash;
}

apr_hash_t *svn_swig_py_path_revs_hash_from_dict(PyObject *dict,
                                                 apr_pool_t *pool)
{
  apr_hash_t *hash;
  PyObject *keys;
  int i, num_keys;

  if (dict == Py_None)
    return NULL;

  if (!PyDict_Check(dict))
    {
      PyErr_SetString(PyExc_TypeError, "not a dictionary");
      return NULL;
    }

  hash = apr_hash_make(pool);
  keys = PyDict_Keys(dict);
  num_keys = PyList_Size(keys);
  for (i = 0; i < num_keys; i++)
    {
      PyObject *key = PyList_GetItem(keys, i);
      PyObject *value = PyDict_GetItem(dict, key);
      const char *path = make_string_from_ob(key, pool);
      svn_revnum_t *revnum;

      if (!(path))
        {
          PyErr_SetString(PyExc_TypeError,
                          "dictionary keys aren't strings");
          Py_DECREF(keys);
          return NULL;
        }

      revnum = apr_palloc(pool, sizeof(svn_revnum_t));

      if (PyInt_Check(value))
        *revnum = PyInt_AsLong(value);
      else if (PyLong_Check(value))
        *revnum = PyLong_AsLong(value);
      else
        {
          PyErr_SetString(PyExc_TypeError, "dictionary values aren't revnums");
          Py_DECREF(keys);
          return NULL;
        }

      apr_hash_set(hash, path, APR_HASH_KEY_STRING, revnum);
    }
  Py_DECREF(keys);
  return hash;
}

apr_hash_t *svn_swig_py_struct_ptr_hash_from_dict(PyObject *dict,
                                                  swig_type_info *type,
                                                  apr_pool_t *pool)
{
  apr_hash_t *hash;
  PyObject *keys;
  int i, num_keys;

  if (dict == Py_None)
    return NULL;

  if (!PyDict_Check(dict))
    {
      PyErr_SetString(PyExc_TypeError, "not a dictionary");
      return NULL;
    }

  hash = apr_hash_make(pool);
  keys = PyDict_Keys(dict);
  num_keys = PyList_Size(keys);
  for (i = 0; i < num_keys; i++)
    {
      PyObject *key = PyList_GetItem(keys, i);
      PyObject *value = PyDict_GetItem(dict, key);
      const char *c_key = make_string_from_ob(key, pool);
      void *struct_ptr;
      int status;

      if (!c_key)
        {
          PyErr_SetString(PyExc_TypeError,
                          "dictionary keys aren't strings");
          Py_DECREF(keys);
          return NULL;
        }
      status = svn_swig_ConvertPtr(value, &struct_ptr, type);
      if (status != 0)
        {
          PyErr_SetString(PyExc_TypeError,
            "dictionary values aren't SWIG proxies of correct type");
          Py_DECREF(keys);
          return NULL;
        }
      apr_hash_set(hash, c_key, APR_HASH_KEY_STRING, struct_ptr);
    }
  Py_DECREF(keys);
  return hash;
}

int
svn_swig_py_unwrap_string(PyObject *source,
                          void *destination,
                          void *baton)
{
    const char **ptr_dest = destination;
    *ptr_dest = PyString_AsString(source);

    if (*ptr_dest != NULL)
        return 0;
    else
        return -1;
}

int
svn_swig_py_unwrap_revnum(PyObject *source,
                          void *destination,
                          void *baton)
{
    svn_revnum_t *revnum_dest = destination;

    if (PyInt_Check(source))
      {
        *revnum_dest = PyInt_AsLong(source);
        if (PyErr_Occurred()) return -1;
        return 0;
      }
    if (PyLong_Check(source))
      {
        *revnum_dest = PyLong_AsLong(source);
        if (PyErr_Occurred()) return -1;
        return 0;
      }

    PyErr_SetString(PyExc_TypeError, "not an integer type");
    return -1;
}

int
svn_swig_py_unwrap_struct_ptr(PyObject *source,
                          void *destination,
                          void *baton)
{
    void **ptr_dest = destination;
    swig_type_info *type_descriptor = baton;

    int status = svn_swig_ConvertPtr(source, ptr_dest, type_descriptor);

    if (status != 0)
      {
        PyErr_SetString(PyExc_TypeError, "not a SWIG proxy of correct type");
        return -1;
      }

    return 0;
}


const apr_array_header_t *
svn_swig_py_seq_to_array(PyObject *seq,
                         int element_size,
                         svn_swig_py_object_unwrap_t unwrap_func,
                         void *unwrap_baton,
                         apr_pool_t *pool)
{
    Py_ssize_t inputlen;
    int targlen, i;
    apr_array_header_t *temp;

    if (seq == Py_None)
        return NULL;

    if (!PySequence_Check(seq))
      {
        PyErr_SetString(PyExc_TypeError, "not a sequence");
        return NULL;
      }

    inputlen = PySequence_Length(seq);

    if (inputlen < 0)
        return NULL;

    if (inputlen > INT_MAX)
      {
        PyErr_SetString(PyExc_ValueError, "too many elements");
        return NULL;
      }

    targlen = (int) inputlen;
    temp = apr_array_make(pool, targlen, element_size);

    for (i = 0; i < targlen; ++i)
      {
        int status;
        void * elt_ptr;
        PyObject *o = PySequence_GetItem(seq, i);

        if (o == NULL)
            return NULL;

        elt_ptr = apr_array_push(temp);
        status = unwrap_func(o, elt_ptr, unwrap_baton);
        Py_DECREF(o);

        if (status < 0)
            return NULL;
      }

    return temp;
}


/*** apr_array_header_t conversions.  To create a new type of
     converter, simply copy-n-paste one of these function and tweak
     the creation of the PyObject *ob.  ***/

PyObject *svn_swig_py_array_to_list(const apr_array_header_t *array)
{
    PyObject *list = PyList_New(array->nelts);
    int i;

    for (i = 0; i < array->nelts; ++i)
      {
        PyObject *ob =
          PyString_FromString(APR_ARRAY_IDX(array, i, const char *));
        if (ob == NULL)
          goto error;
        PyList_SET_ITEM(list, i, ob);
      }
    return list;

  error:
    Py_DECREF(list);
    return NULL;
}

PyObject *svn_swig_py_revarray_to_list(const apr_array_header_t *array)
{
    PyObject *list = PyList_New(array->nelts);
    int i;

    for (i = 0; i < array->nelts; ++i)
      {
        PyObject *ob
          = PyInt_FromLong(APR_ARRAY_IDX(array, i, svn_revnum_t));
        if (ob == NULL)
          goto error;
        PyList_SET_ITEM(list, i, ob);
      }
    return list;

  error:
    Py_DECREF(list);
    return NULL;
}

static PyObject *
commit_item_array_to_list(const apr_array_header_t *array)
{
    PyObject *list = PyList_New(array->nelts);
    int i;

    for (i = 0; i < array->nelts; ++i)
      {
        PyObject *ob = convert_svn_client_commit_item3_t
          (APR_ARRAY_IDX(array, i, svn_client_commit_item3_t *), NULL);
        if (ob == NULL)
          goto error;
        PyList_SET_ITEM(list, i, ob);
      }
    return list;

  error:
    Py_DECREF(list);
    return NULL;
}



/*** Errors ***/

/* Convert a given SubversionException to an svn_error_t. On failure returns
   NULL and sets a Python exception. */
static svn_error_t *exception_to_error(PyObject * exc)
{
	const char *message, *file = NULL;
	apr_status_t apr_err;
	long line = 0;
	PyObject *apr_err_ob = NULL, *child_ob = NULL, *message_ob = NULL;
	PyObject *file_ob = NULL, *line_ob = NULL;
    svn_error_t *rv = NULL, *child = NULL;

	if ((apr_err_ob = PyObject_GetAttrString(exc, "apr_err")) == NULL)
	    goto finished;
	apr_err = (apr_status_t) PyInt_AsLong(apr_err_ob);
	if (PyErr_Occurred()) goto finished;

	if ((message_ob = PyObject_GetAttrString(exc, "message")) == NULL)
	    goto finished;
	message = PyString_AsString(message_ob);
	if (PyErr_Occurred()) goto finished;

	if ((file_ob = PyObject_GetAttrString(exc, "file")) == NULL)
	    goto finished;
	if (file_ob != Py_None)
	    file = PyString_AsString(file_ob);
	if (PyErr_Occurred()) goto finished;

	if ((line_ob = PyObject_GetAttrString(exc, "line")) == NULL)
	    goto finished;
	if (line_ob != Py_None)
	    line = PyInt_AsLong(line_ob);
	if (PyErr_Occurred()) goto finished;

	if ((child_ob = PyObject_GetAttrString(exc, "child")) == NULL)
	    goto finished;
	/* We could check if the child is a Subversion exception too,
	   but let's just apply duck typing. */
	if (child_ob != Py_None)
	    child = exception_to_error(child_ob);
	if (PyErr_Occurred()) goto finished;

	rv = svn_error_create(apr_err, child, message);
	/* Somewhat hacky, but we need to preserve original file/line info. */
	rv->file = file ? apr_pstrdup(rv->pool, file) : NULL;
	rv->line = line;

finished:
	Py_XDECREF(child_ob);
	Py_XDECREF(line_ob);
	Py_XDECREF(file_ob);
	Py_XDECREF(message_ob);
	Py_XDECREF(apr_err_ob);
	return rv;
}

/* If the currently set Python exception is a valid SubversionException,
   clear exception state and transform it into a Subversion error.
   Otherwise, return a Subversion error about an exception in a callback. */
static svn_error_t *callback_exception_error(void)
{
  PyObject *svn_module = NULL, *svn_exc = NULL;
  PyObject *exc, *exc_type, *exc_traceback;
  svn_error_t *rv = NULL;

  PyErr_Fetch(&exc_type, &exc, &exc_traceback);

  if ((svn_module = PyImport_ImportModule("svn.core")) == NULL)
      goto finished;

  svn_exc = PyObject_GetAttrString(svn_module, "SubversionException");
  Py_DECREF(svn_module);

  if (svn_exc == NULL)
      goto finished;

  if (PyErr_GivenExceptionMatches(exc_type, svn_exc))
    {
      rv = exception_to_error(exc);
    }
  else
    {
      PyErr_Restore(exc_type, exc, exc_traceback);
      exc_type = exc = exc_traceback = NULL;
    }

finished:
  Py_XDECREF(svn_exc);
  Py_XDECREF(exc_type);
  Py_XDECREF(exc);
  Py_XDECREF(exc_traceback);
  /* By now, either rv is set and the exception is cleared, or rv is NULL
     and an exception is pending (possibly a new one). */
  return rv ? rv : svn_error_create(SVN_ERR_SWIG_PY_EXCEPTION_SET, NULL,
                                    "Python callback raised an exception");
}

/* Raise a TypeError exception with MESSAGE, and return a Subversion
   error about an invalid return from a callback. */
static svn_error_t *callback_bad_return_error(const char *message)
{
  PyErr_SetString(PyExc_TypeError, message);
  return svn_error_create(APR_EGENERAL, NULL,
                          "Python callback returned an invalid object");
}

/* Return a generic error about not being able to map types. */
static svn_error_t *type_conversion_error(const char *datatype)
{
  return svn_error_createf(APR_EGENERAL, NULL,
                           "Error converting object of type '%s'", datatype);
}



/*** Editor Wrapping ***/

/* this baton is used for the editor, directory, and file batons. */
typedef struct item_baton {
  PyObject *editor;     /* the editor handling the callbacks */
  PyObject *baton;      /* the dir/file baton (or NULL for edit baton) */
} item_baton;

static item_baton *make_baton(apr_pool_t *pool,
                              PyObject *editor,
                              PyObject *baton)
{
  item_baton *newb = apr_palloc(pool, sizeof(*newb));

  /* Note: We steal the caller's reference to 'baton'. Also, to avoid
     memory leaks, we borrow the caller's reference to 'editor'. In this
     case, borrowing the reference to 'editor' is safe because the contents
     of an item_baton struct are only used by functino calls which operate on
     the editor itself. */
  newb->editor = editor;
  newb->baton = baton;

  return newb;
}

static svn_error_t *close_baton(void *baton,
                                const char *method)
{
  item_baton *ib = baton;
  PyObject *result;
  svn_error_t *err;

  svn_swig_py_acquire_py_lock();

  /* If there is no baton object, then it is an edit_baton, and we should
     not bother to pass an object. Note that we still shove a NULL onto
     the stack, but the format specified just won't reference it.  */
  /* ### python doesn't have 'const' on the method name and format */
  if ((result = PyObject_CallMethod(ib->editor, (char *)method,
                                    ib->baton ? (char *)"(O)" : NULL,
                                    ib->baton)) == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }

  /* there is no return value, so just toss this object (probably Py_None) */
  Py_DECREF(result);

  /* We're now done with the baton. Since there isn't really a free, all
     we need to do is note that its objects are no longer referenced by
     the baton.  */
  Py_XDECREF(ib->baton);

#ifdef SVN_DEBUG
  ib->editor = ib->baton = NULL;
#endif

  err = SVN_NO_ERROR;

 finished:
  svn_swig_py_release_py_lock();
  return err;
}

static svn_error_t *set_target_revision(void *edit_baton,
                                        svn_revnum_t target_revision,
                                        apr_pool_t *pool)
{
  item_baton *ib = edit_baton;
  PyObject *result;
  svn_error_t *err;

  svn_swig_py_acquire_py_lock();

  /* ### python doesn't have 'const' on the method name and format */
  if ((result = PyObject_CallMethod(ib->editor, (char *)"set_target_revision",
                                    (char *)"l", target_revision)) == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }

  /* there is no return value, so just toss this object (probably Py_None) */
  Py_DECREF(result);
  err = SVN_NO_ERROR;

 finished:
  svn_swig_py_release_py_lock();
  return err;
}

static svn_error_t *open_root(void *edit_baton,
                              svn_revnum_t base_revision,
                              apr_pool_t *dir_pool,
                              void **root_baton)
{
  item_baton *ib = edit_baton;
  PyObject *result;
  svn_error_t *err;

  svn_swig_py_acquire_py_lock();

  /* ### python doesn't have 'const' on the method name and format */
  if ((result = PyObject_CallMethod(ib->editor, (char *)"open_root",
                                    (char *)"lO&", base_revision,
                                    make_ob_pool, dir_pool)) == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }

  /* make_baton takes our 'result' reference */
  *root_baton = make_baton(dir_pool, ib->editor, result);
  err = SVN_NO_ERROR;

 finished:
  svn_swig_py_release_py_lock();
  return err;
}

static svn_error_t *delete_entry(const char *path,
                                 svn_revnum_t revision,
                                 void *parent_baton,
                                 apr_pool_t *pool)
{
  item_baton *ib = parent_baton;
  PyObject *result;
  svn_error_t *err;

  svn_swig_py_acquire_py_lock();

  /* ### python doesn't have 'const' on the method name and format */
  if ((result = PyObject_CallMethod(ib->editor, (char *)"delete_entry",
                                    (char *)"slOO&", path, revision, ib->baton,
                                    make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }

  /* there is no return value, so just toss this object (probably Py_None) */
  Py_DECREF(result);
  err = SVN_NO_ERROR;

 finished:
  svn_swig_py_release_py_lock();
  return err;
}

static svn_error_t *add_directory(const char *path,
                                  void *parent_baton,
                                  const char *copyfrom_path,
                                  svn_revnum_t copyfrom_revision,
                                  apr_pool_t *dir_pool,
                                  void **child_baton)
{
  item_baton *ib = parent_baton;
  PyObject *result;
  svn_error_t *err;

  svn_swig_py_acquire_py_lock();

  /* ### python doesn't have 'const' on the method name and format */
  if ((result = PyObject_CallMethod(ib->editor, (char *)"add_directory",
                                    (char *)"sOslO&", path, ib->baton,
                                    copyfrom_path, copyfrom_revision,
                                    make_ob_pool, dir_pool)) == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }

  /* make_baton takes our 'result' reference */
  *child_baton = make_baton(dir_pool, ib->editor, result);
  err = SVN_NO_ERROR;

 finished:
  svn_swig_py_release_py_lock();
  return err;
}

static svn_error_t *open_directory(const char *path,
                                   void *parent_baton,
                                   svn_revnum_t base_revision,
                                   apr_pool_t *dir_pool,
                                   void **child_baton)
{
  item_baton *ib = parent_baton;
  PyObject *result;
  svn_error_t *err;

  svn_swig_py_acquire_py_lock();

  /* ### python doesn't have 'const' on the method name and format */
  if ((result = PyObject_CallMethod(ib->editor, (char *)"open_directory",
                                    (char *)"sOlO&", path, ib->baton,
                                    base_revision,
                                    make_ob_pool, dir_pool)) == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }

  /* make_baton takes our 'result' reference */
  *child_baton = make_baton(dir_pool, ib->editor, result);
  err = SVN_NO_ERROR;

 finished:
  svn_swig_py_release_py_lock();
  return err;
}

static svn_error_t *change_dir_prop(void *dir_baton,
                                    const char *name,
                                    const svn_string_t *value,
                                    apr_pool_t *pool)
{
  item_baton *ib = dir_baton;
  PyObject *result;
  svn_error_t *err;

  svn_swig_py_acquire_py_lock();

  /* ### python doesn't have 'const' on the method name and format */
  if ((result = PyObject_CallMethod(ib->editor, (char *)"change_dir_prop",
                                    (char *)"Oss#O&", ib->baton, name,
                                    value ? value->data : NULL,
                                    value ? value->len : 0,
                                    make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }

  /* there is no return value, so just toss this object (probably Py_None) */
  Py_DECREF(result);
  err = SVN_NO_ERROR;

 finished:
  svn_swig_py_release_py_lock();
  return err;
}

static svn_error_t *close_directory(void *dir_baton,
                                    apr_pool_t *pool)
{
  return close_baton(dir_baton, "close_directory");
}

static svn_error_t *add_file(const char *path,
                             void *parent_baton,
                             const char *copyfrom_path,
                             svn_revnum_t copyfrom_revision,
                             apr_pool_t *file_pool,
                             void **file_baton)
{
  item_baton *ib = parent_baton;
  PyObject *result;
  svn_error_t *err;

  svn_swig_py_acquire_py_lock();

  /* ### python doesn't have 'const' on the method name and format */
  if ((result = PyObject_CallMethod(ib->editor, (char *)"add_file",
                                    (char *)"sOslO&", path, ib->baton,
                                    copyfrom_path, copyfrom_revision,
                                    make_ob_pool, file_pool)) == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }

  /* make_baton takes our 'result' reference */
  *file_baton = make_baton(file_pool, ib->editor, result);

  err = SVN_NO_ERROR;

 finished:
  svn_swig_py_release_py_lock();
  return err;
}

static svn_error_t *open_file(const char *path,
                              void *parent_baton,
                              svn_revnum_t base_revision,
                              apr_pool_t *file_pool,
                              void **file_baton)
{
  item_baton *ib = parent_baton;
  PyObject *result;
  svn_error_t *err;

  svn_swig_py_acquire_py_lock();

  /* ### python doesn't have 'const' on the method name and format */
  if ((result = PyObject_CallMethod(ib->editor, (char *)"open_file",
                                    (char *)"sOlO&", path, ib->baton,
                                    base_revision,
                                    make_ob_pool, file_pool)) == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }

  /* make_baton takes our 'result' reference */
  *file_baton = make_baton(file_pool, ib->editor, result);
  err = SVN_NO_ERROR;

 finished:
  svn_swig_py_release_py_lock();
  return err;
}

static svn_error_t *window_handler(svn_txdelta_window_t *window,
                                   void *baton)
{
  PyObject *handler = baton;
  PyObject *result;
  svn_error_t *err;

  svn_swig_py_acquire_py_lock();

  if (window == NULL)
    {
      /* the last call; it closes the handler */

      /* invoke the handler with None for the window */
      /* ### python doesn't have 'const' on the format */
      result = PyObject_CallFunction(handler, (char *)"O", Py_None);

      /* we no longer need to refer to the handler object */
      Py_DECREF(handler);
    }
  else
    {
      /* invoke the handler with the window */
      /* ### python doesn't have 'const' on the format */
      result = PyObject_CallFunction(handler, (char *)"O&",
        make_ob_txdelta_window, window);
    }

  if (result == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }

  /* there is no return value, so just toss this object (probably Py_None) */
  Py_DECREF(result);
  err = SVN_NO_ERROR;

 finished:
  svn_swig_py_release_py_lock();
  return err;
}

static svn_error_t *apply_textdelta(void *file_baton,
                                    const char *base_checksum,
                                    apr_pool_t *pool,
                                    svn_txdelta_window_handler_t *handler,
                                    void **h_baton)
{
  item_baton *ib = file_baton;
  PyObject *result;
  svn_error_t *err;

  svn_swig_py_acquire_py_lock();

  /* ### python doesn't have 'const' on the method name and format */
  if ((result = PyObject_CallMethod(ib->editor, (char *)"apply_textdelta",
                                    (char *)"(Os)", ib->baton,
                                    base_checksum)) == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }

  /* Interpret None to mean svn_delta_noop_window_handler. This is much
     easier/faster than making code always have to write a NOOP handler
     in Python.  */
  if (result == Py_None)
    {
      Py_DECREF(result);

      *handler = svn_delta_noop_window_handler;
      *h_baton = NULL;
    }
  else
    {
      /* return the thunk for invoking the handler. the baton takes our
         'result' reference, which is the handler. */
      *handler = window_handler;
      *h_baton = result;
    }

  err = SVN_NO_ERROR;

 finished:
  svn_swig_py_release_py_lock();
  return err;
}

static svn_error_t *change_file_prop(void *file_baton,
                                     const char *name,
                                     const svn_string_t *value,
                                     apr_pool_t *pool)
{
  item_baton *ib = file_baton;
  PyObject *result;
  svn_error_t *err;

  svn_swig_py_acquire_py_lock();

  /* ### python doesn't have 'const' on the method name and format */
  if ((result = PyObject_CallMethod(ib->editor, (char *)"change_file_prop",
                                    (char *)"Oss#O&", ib->baton, name,
                                    value ? value->data : NULL,
                                    value ? value->len : 0,
                                    make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }

  /* there is no return value, so just toss this object (probably Py_None) */
  Py_DECREF(result);
  err = SVN_NO_ERROR;

 finished:
  svn_swig_py_release_py_lock();
  return err;
}

static svn_error_t *close_file(void *file_baton,
                               const char *text_checksum,
                               apr_pool_t *pool)
{
  item_baton *ib = file_baton;
  PyObject *result;
  svn_error_t *err;

  svn_swig_py_acquire_py_lock();

  /* ### python doesn't have 'const' on the method name and format */
  if ((result = PyObject_CallMethod(ib->editor, (char *)"close_file",
                                    (char *)"(Os)", ib->baton,
                                    text_checksum)) == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }

  /* there is no return value, so just toss this object (probably Py_None) */
  Py_DECREF(result);

  /* We're now done with the baton. Since there isn't really a free, all
     we need to do is note that its objects are no longer referenced by
     the baton.  */
  Py_XDECREF(ib->baton);

#ifdef SVN_DEBUG
  ib->editor = ib->baton = NULL;
#endif

  err = SVN_NO_ERROR;

 finished:
  svn_swig_py_release_py_lock();
  return err;
}

static svn_error_t *close_edit(void *edit_baton,
                               apr_pool_t *pool)
{
  return close_baton(edit_baton, "close_edit");
}

static svn_error_t *abort_edit(void *edit_baton,
                               apr_pool_t *pool)
{
  return close_baton(edit_baton, "abort_edit");
}

void svn_swig_py_make_editor(const svn_delta_editor_t **editor,
                             void **edit_baton,
                             PyObject *py_editor,
                             apr_pool_t *pool)
{
  svn_delta_editor_t *thunk_editor = svn_delta_default_editor(pool);

  thunk_editor->set_target_revision = set_target_revision;
  thunk_editor->open_root = open_root;
  thunk_editor->delete_entry = delete_entry;
  thunk_editor->add_directory = add_directory;
  thunk_editor->open_directory = open_directory;
  thunk_editor->change_dir_prop = change_dir_prop;
  thunk_editor->close_directory = close_directory;
  thunk_editor->add_file = add_file;
  thunk_editor->open_file = open_file;
  thunk_editor->apply_textdelta = apply_textdelta;
  thunk_editor->change_file_prop = change_file_prop;
  thunk_editor->close_file = close_file;
  thunk_editor->close_edit = close_edit;
  thunk_editor->abort_edit = abort_edit;

  *editor = thunk_editor;
  *edit_baton = make_baton(pool, py_editor, NULL);
}



/*** Other Wrappers for SVN Functions ***/


apr_file_t *svn_swig_py_make_file(PyObject *py_file,
                                  apr_pool_t *pool)
{
  apr_file_t *apr_file = NULL;
  apr_status_t apr_err;

  if (py_file == NULL || py_file == Py_None)
    return NULL;

  if (PyString_Check(py_file))
    {
      /* input is a path -- just open an apr_file_t */
      char* fname = PyString_AS_STRING(py_file);
      apr_err = apr_file_open(&apr_file, fname,
                              APR_CREATE | APR_READ | APR_WRITE,
                              APR_OS_DEFAULT, pool);
      if (apr_err)
        {
          char buf[256];
          apr_strerror(apr_err, buf, sizeof(buf));
          PyErr_Format(PyExc_IOError, "apr_file_open failed: %s: '%s'",
                       buf, fname);
          return NULL;
        }
    }
  else if (PyFile_Check(py_file))
    {
      FILE *file;
      apr_os_file_t osfile;

      /* input is a file object -- convert to apr_file_t */
      file = PyFile_AsFile(py_file);
#ifdef WIN32
      osfile = (apr_os_file_t)_get_osfhandle(_fileno(file));
#else
      osfile = (apr_os_file_t)fileno(file);
#endif
      apr_err = apr_os_file_put(&apr_file, &osfile, O_CREAT | O_WRONLY, pool);
      if (apr_err)
        {
          char buf[256];
          apr_strerror(apr_err, buf, sizeof(buf));
          PyErr_Format(PyExc_IOError, "apr_os_file_put failed: %s", buf);
          return NULL;
        }
    }
  return apr_file;
}


static svn_error_t *
read_handler_pyio(void *baton, char *buffer, apr_size_t *len)
{
  PyObject *result;
  PyObject *py_io = baton;
  apr_size_t bytes;
  svn_error_t *err = SVN_NO_ERROR;

  if (py_io == Py_None)
    {
      /* Return the empty string to indicate a short read */
      *buffer = '\0';
      *len = 0;
      return SVN_NO_ERROR;
    }

  svn_swig_py_acquire_py_lock();
  if ((result = PyObject_CallMethod(py_io, (char *)"read",
                                    (char *)"i", *len)) == NULL)
    {
      err = callback_exception_error();
    }
  else if (PyString_Check(result))
    {
      bytes = PyString_GET_SIZE(result);
      if (bytes > *len)
        {
          err = callback_bad_return_error("Too many bytes");
        }
      else
        {
          /* Writeback, in case this was a short read, indicating EOF */
          *len = bytes;
          memcpy(buffer, PyString_AS_STRING(result), *len);
        }
    }
  else
    {
      err = callback_bad_return_error("Not a string");
    }
  Py_XDECREF(result);
  svn_swig_py_release_py_lock();

  return err;
}

static svn_error_t *
write_handler_pyio(void *baton, const char *data, apr_size_t *len)
{
  PyObject *result;
  PyObject *py_io = baton;
  svn_error_t *err = SVN_NO_ERROR;

  if (data != NULL && py_io != Py_None)
    {
      svn_swig_py_acquire_py_lock();
      if ((result = PyObject_CallMethod(py_io, (char *)"write",
                                        (char *)"s#", data, *len)) == NULL)
        {
          err = callback_exception_error();
        }
      Py_XDECREF(result);
      svn_swig_py_release_py_lock();
    }

  return err;
}

static svn_error_t *
close_handler_pyio(void *baton)
{
  PyObject *py_io = baton;
  PyObject *result;
  svn_error_t *err = NULL;

  svn_swig_py_acquire_py_lock();
  if ((result = PyObject_CallMethod(py_io, (char *)"close", NULL)) == NULL)
    {
      err = callback_exception_error();
    }
  Py_XDECREF(result);
  svn_swig_py_release_py_lock();

  return err;
}

static apr_status_t
svn_swig_py_stream_destroy(void *py_io)
{
  svn_swig_py_acquire_py_lock();
  Py_DECREF((PyObject*)py_io);
  svn_swig_py_release_py_lock();
  return APR_SUCCESS;
}

svn_stream_t *
svn_swig_py_make_stream(PyObject *py_io, apr_pool_t *pool)
{
  svn_stream_t *stream;

  stream = svn_stream_create(py_io, pool);
  svn_stream_set_read(stream, read_handler_pyio);
  svn_stream_set_write(stream, write_handler_pyio);
  svn_stream_set_close(stream, close_handler_pyio);
  apr_pool_cleanup_register(pool, py_io, svn_swig_py_stream_destroy,
                            apr_pool_cleanup_null);
  Py_INCREF(py_io);

  return stream;
}


void svn_swig_py_notify_func(void *baton,
                             const char *path,
                             svn_wc_notify_action_t action,
                             svn_node_kind_t kind,
                             const char *mime_type,
                             svn_wc_notify_state_t content_state,
                             svn_wc_notify_state_t prop_state,
                             svn_revnum_t revision)
{
  PyObject *function = baton;
  PyObject *result;
  svn_error_t *err = SVN_NO_ERROR;

  if (function == NULL || function == Py_None)
    return;

  svn_swig_py_acquire_py_lock();
  if ((result = PyObject_CallFunction(function,
                                      (char *)"(siisiii)",
                                      path, action, kind,
                                      mime_type,
                                      content_state, prop_state,
                                      revision)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      /* The callback shouldn't be returning anything. */
      if (result != Py_None)
        err = callback_bad_return_error("Not None");
      Py_DECREF(result);
    }

  /* Our error has no place to go. :-( */
  svn_error_clear(err);

  svn_swig_py_release_py_lock();
}


void svn_swig_py_notify_func2(void *baton,
                              const svn_wc_notify_t *notify,
                              apr_pool_t *pool)
{
  PyObject *function = baton;
  PyObject *result;
  svn_error_t *err = SVN_NO_ERROR;

  if (function == NULL || function == Py_None)
    return;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallFunction(function,
                                      (char *)"(O&O&)",
                                      make_ob_wc_notify, notify,
                                      make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      /* The callback shouldn't be returning anything. */
      if (result != Py_None)
        err = callback_bad_return_error("Not None");
      Py_DECREF(result);
    }

  /* Our error has no place to go. :-( */
  svn_error_clear(err);

  svn_swig_py_release_py_lock();
}

void svn_swig_py_status_func(void *baton,
                             const char *path,
                             svn_wc_status_t *status)
{
  PyObject *function = baton;
  PyObject *result;
  svn_error_t *err = SVN_NO_ERROR;

  if (function == NULL || function == Py_None)
    return;

  svn_swig_py_acquire_py_lock();
  if ((result = PyObject_CallFunction(function, (char *)"sO&", path,
                                      make_ob_wc_status, status)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      /* The callback shouldn't be returning anything. */
      if (result != Py_None)
        err = callback_bad_return_error("Not None");
      Py_DECREF(result);
    }

  /* Our error has no place to go. :-( */
  svn_error_clear(err);

  svn_swig_py_release_py_lock();
}


svn_error_t *svn_swig_py_delta_path_driver_cb_func(void **dir_baton,
                                                   void *parent_baton,
                                                   void *callback_baton,
                                                   const char *path,
                                                   apr_pool_t *pool)
{
  PyObject *function = callback_baton;
  PyObject *result, *py_parent_baton;
  svn_error_t *err = SVN_NO_ERROR;

  if (function == NULL || function == Py_None)
    return err;

  svn_swig_py_acquire_py_lock();

  py_parent_baton = svn_swig_NewPointerObjString(parent_baton,
                                                 "void *",
                                                 NULL);

  result = PyObject_CallFunction(function, (char *)"OsO&",
                                 py_parent_baton,
                                 path, make_ob_pool, pool);


  if (result == NULL)
    {
      err = callback_exception_error();
    }
  else if (result == Py_None)
    {
      *dir_baton = NULL;
    }
  else
    {
      if (svn_swig_ConvertPtrString(result, dir_baton, "void *") == -1)
        {
          err = type_conversion_error("void *");
        }
    }

  Py_XDECREF(result);
  Py_XDECREF(py_parent_baton);
  svn_swig_py_release_py_lock();
  return err;
}


void svn_swig_py_status_func2(void *baton,
                              const char *path,
                              svn_wc_status2_t *status)
{
  PyObject *function = baton;
  PyObject *result;
  svn_error_t *err = SVN_NO_ERROR;

  if (function == NULL || function == Py_None)
    return;

  svn_swig_py_acquire_py_lock();
  if ((result = PyObject_CallFunction(function, (char *)"sO&", path,
                                      make_ob_wc_status, status)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      /* The callback shouldn't be returning anything. */
      if (result != Py_None)
        err = callback_bad_return_error("Not None");
      Py_DECREF(result);
    }

  /* Our error has no place to go. :-( */
  if (err)
    svn_error_clear(err);

  svn_swig_py_release_py_lock();
}


svn_error_t *svn_swig_py_cancel_func(void *cancel_baton)
{
  PyObject *function = cancel_baton;
  PyObject *result;
  svn_error_t *err = SVN_NO_ERROR;

  if (function == NULL || function == Py_None)
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();
  if ((result = PyObject_CallFunction(function, NULL)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      if (PyInt_Check(result))
        {
          if (PyInt_AsLong(result))
            err = svn_error_create(SVN_ERR_CANCELLED, 0, NULL);
        }
      else if (PyLong_Check(result))
        {
          if (PyLong_AsLong(result))
            err = svn_error_create(SVN_ERR_CANCELLED, 0, NULL);
        }
      else if (result != Py_None)
        {
          err = callback_bad_return_error("Not an integer or None");
        }
      Py_DECREF(result);
    }
  svn_swig_py_release_py_lock();
  return err;
}

svn_error_t *svn_swig_py_fs_get_locks_func(void *baton,
                                           svn_lock_t *lock,
                                           apr_pool_t *pool)
{
  PyObject *function = baton;
  PyObject *result;
  svn_error_t *err = SVN_NO_ERROR;

  if (function == NULL || function == Py_None)
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallFunction(function, (char *)"O&O&",
                                      make_ob_lock, lock,
                                      make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      /* The callback shouldn't be returning anything. */
      if (result != Py_None)
        err = callback_bad_return_error("Not None");
      Py_DECREF(result);
    }

  svn_swig_py_release_py_lock();
  return err;
}

svn_error_t *svn_swig_py_get_commit_log_func(const char **log_msg,
                                             const char **tmp_file,
                                             const apr_array_header_t *
                                             commit_items,
                                             void *baton,
                                             apr_pool_t *pool)
{
  PyObject *function = baton;
  PyObject *result;
  PyObject *cmt_items;
  svn_error_t *err;

  *log_msg = NULL;
  *tmp_file = NULL;

  /* ### todo: for now, just ignore the whole tmp_file thing.  */

  if ((function == NULL) || (function == Py_None))
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if (commit_items)
    {
      cmt_items = commit_item_array_to_list(commit_items);
    }
  else
    {
      cmt_items = Py_None;
      Py_INCREF(Py_None);
    }

  if ((result = PyObject_CallFunction(function,
                                      (char *)"OO&",
                                      cmt_items,
                                      make_ob_pool, pool)) == NULL)
    {
      Py_DECREF(cmt_items);
      err = callback_exception_error();
      goto finished;
    }

  Py_DECREF(cmt_items);

  if (result == Py_None)
    {
      Py_DECREF(result);
      *log_msg = NULL;
      err = SVN_NO_ERROR;
    }
  else if (PyString_Check(result))
    {
      *log_msg = apr_pstrdup(pool, PyString_AS_STRING(result));
      Py_DECREF(result);
      err = SVN_NO_ERROR;
    }
  else
    {
      Py_DECREF(result);
      err = callback_bad_return_error("Not a string");
    }

 finished:
  svn_swig_py_release_py_lock();
  return err;
}


svn_error_t *svn_swig_py_repos_authz_func(svn_boolean_t *allowed,
                                          svn_fs_root_t *root,
                                          const char *path,
                                          void *baton,
                                          apr_pool_t *pool)
{
  PyObject *function = baton;
  PyObject *result;
  PyObject *py_pool, *py_root;
  svn_error_t *err = SVN_NO_ERROR;

  *allowed = TRUE;

  if (function == NULL || function == Py_None)
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  py_pool = make_ob_pool(pool);
  if (py_pool == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }
  py_root = make_ob_fs_root(root, py_pool);
  if (py_root == NULL)
    {
      Py_DECREF(py_pool);
      err = callback_exception_error();
      goto finished;
    }

  if ((result = PyObject_CallFunction(function,
                                      (char *)"OsO",
                                      py_root, path, py_pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      if (PyInt_Check(result))
        *allowed = PyInt_AsLong(result);
      else if (PyLong_Check(result))
        *allowed = PyLong_AsLong(result);
      else
        err = callback_bad_return_error("Not an integer");
      Py_DECREF(result);
    }
  Py_DECREF(py_root);
  Py_DECREF(py_pool);
finished:
  svn_swig_py_release_py_lock();
  return err;
}


svn_error_t *svn_swig_py_repos_history_func(void *baton,
                                            const char *path,
                                            svn_revnum_t revision,
                                            apr_pool_t *pool)
{
  PyObject *function = baton;
  PyObject *result;
  svn_error_t *err = SVN_NO_ERROR;

  if (function == NULL || function == Py_None)
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();
  if ((result = PyObject_CallFunction(function,
                                      (char *)"slO&",
                                      path, revision,
                                      make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      if (result != Py_None)
        err = callback_bad_return_error("Not None");
      Py_DECREF(result);
    }
  svn_swig_py_release_py_lock();
  return err;
}


svn_error_t *svn_swig_py_log_receiver(void *baton,
                                      apr_hash_t *changed_paths,
                                      svn_revnum_t rev,
                                      const char *author,
                                      const char *date,
                                      const char *msg,
                                      apr_pool_t *pool)
{
  PyObject *receiver = baton;
  PyObject *result, *py_pool;
  PyObject *chpaths;
  svn_error_t *err = SVN_NO_ERROR;

  if ((receiver == NULL) || (receiver == Py_None))
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  py_pool = make_ob_pool(pool);
  if (py_pool == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }

  if (changed_paths)
    {
      chpaths = convert_hash(changed_paths, convert_log_changed_path,
                             NULL, NULL);
    }
  else
    {
      chpaths = Py_None;
      Py_INCREF(Py_None);
    }

  if ((result = PyObject_CallFunction(receiver,
                                      (char *)"OlsssO",
                                      chpaths, rev, author, date, msg,
                                      py_pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      if (result != Py_None)
        err = callback_bad_return_error("Not None");
      Py_DECREF(result);
    }

  Py_DECREF(chpaths);
  Py_DECREF(py_pool);
finished:
  svn_swig_py_release_py_lock();
  return err;
}

svn_error_t *svn_swig_py_log_entry_receiver(void *baton,
                                            svn_log_entry_t *log_entry,
                                            apr_pool_t *pool)
{
  PyObject *receiver = baton;
  PyObject *result, *py_pool;
  svn_error_t *err = SVN_NO_ERROR;
  PyObject *py_log_entry;

  if ((receiver == NULL) || (receiver == Py_None))
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  py_pool = make_ob_pool(pool);
  if (py_pool == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }

  py_log_entry = svn_swig_NewPointerObjString(log_entry, "svn_log_entry_t *",
                                              py_pool);
  if ((result = PyObject_CallFunction(receiver,
                                      (char *)"OO", py_log_entry,
                                      py_pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      if (result != Py_None)
        err = callback_bad_return_error("Not None");
      Py_DECREF(result);
    }

  Py_DECREF(py_log_entry);
  Py_DECREF(py_pool);
finished:
  svn_swig_py_release_py_lock();
  return err;
}

svn_error_t *svn_swig_py_info_receiver_func(void *baton,
                                            const char *path,
                                            const svn_info_t *info,
                                            apr_pool_t *pool)
{
  PyObject *receiver = baton;
  PyObject *result;
  svn_error_t *err = SVN_NO_ERROR;

  if ((receiver == NULL) || (receiver == Py_None))
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallFunction(receiver,
                                      (char *)"sO&O&",
                                      path, make_ob_info, info,
                                      make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      if (result != Py_None)
        err = callback_bad_return_error("Not None");
      Py_DECREF(result);
    }

  svn_swig_py_release_py_lock();

  return err;
}

svn_error_t *
svn_swig_py_location_segment_receiver_func(svn_location_segment_t *segment,
                                           void *baton,
                                           apr_pool_t *pool)
{
  PyObject *receiver = baton;
  PyObject *result;
  svn_error_t *err = SVN_NO_ERROR;

  if ((receiver == NULL) || (receiver == Py_None))
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallFunction(receiver,
                                      (char *)"O&O&",
                                      make_ob_location_segment, segment,
                                      make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      if (result != Py_None)
        err = callback_bad_return_error("Not None");
      Py_DECREF(result);
    }

  svn_swig_py_release_py_lock();

  return err;
}

svn_error_t *svn_swig_py_client_blame_receiver_func(void *baton,
                                                    apr_int64_t line_no,
                                                    svn_revnum_t revision,
                                                    const char *author,
                                                    const char *date,
                                                    const char *line,
                                                    apr_pool_t *pool)
{
  PyObject *receiver = baton;
  PyObject *result;
  svn_error_t *err = SVN_NO_ERROR;

  if ((receiver == NULL) || (receiver == Py_None))
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallFunction(receiver,
                                      (char *)
                                      (SVN_APR_INT64_T_PYCFMT "lsssO&"),
                                      line_no, revision, author, date, line,
                                      make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      if (result != Py_None)
        err = callback_bad_return_error("Not None");
      Py_DECREF(result);
    }

  svn_swig_py_release_py_lock();
  return err;
}

svn_error_t *svn_swig_py_changelist_receiver_func(void *baton,
                                                  const char *path,
                                                  const char *changelist,
                                                  apr_pool_t *pool)
{
  PyObject *receiver = baton;
  PyObject *result;
  svn_error_t *err = SVN_NO_ERROR;

  if ((receiver == NULL) || (receiver == Py_None))
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallFunction(receiver,
                                      (char *)"ssO&",
                                      path, changelist,
                                      make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      if (result != Py_None)
        err = callback_bad_return_error("Not None");
      Py_DECREF(result);
    }

  svn_swig_py_release_py_lock();
  return err;
}

svn_error_t *
svn_swig_py_auth_simple_prompt_func(svn_auth_cred_simple_t **cred,
                                    void *baton,
                                    const char *realm,
                                    const char *username,
                                    svn_boolean_t may_save,
                                    apr_pool_t *pool)
{
  PyObject *function = baton;
  PyObject *result;
  svn_auth_cred_simple_t *creds = NULL;
  svn_error_t *err = SVN_NO_ERROR;

  if ((function == NULL) || (function == Py_None))
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallFunction(function,
                                      (char *)"sslO&",
                                      realm, username, may_save,
                                      make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      if (result != Py_None)
        {
          svn_auth_cred_simple_t *tmp_creds = NULL;
          if (svn_swig_ConvertPtrString(result, (void **)&tmp_creds,
                "svn_auth_cred_simple_t *"))
            {
              err = type_conversion_error("svn_auth_cred_simple_t *");
            }
          else
            {
              creds = apr_pcalloc(pool, sizeof(*creds));
              creds->username = tmp_creds->username ?
                apr_pstrdup(pool, tmp_creds->username) : NULL;
              creds->password = tmp_creds->password ?
                apr_pstrdup(pool, tmp_creds->password) : NULL;
              creds->may_save = tmp_creds->may_save;
            }
        }
      Py_DECREF(result);
    }
  svn_swig_py_release_py_lock();
  *cred = creds;
  return err;
}

svn_error_t *
svn_swig_py_auth_username_prompt_func(svn_auth_cred_username_t **cred,
                                      void *baton,
                                      const char *realm,
                                      svn_boolean_t may_save,
                                      apr_pool_t *pool)
{
  PyObject *function = baton;
  PyObject *result;
  svn_auth_cred_username_t *creds = NULL;
  svn_error_t *err = SVN_NO_ERROR;

  if ((function == NULL) || (function == Py_None))
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallFunction(function,
                                      (char *)"slO&",
                                      realm, may_save,
                                      make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      if (result != Py_None)
        {
          svn_auth_cred_username_t *tmp_creds = NULL;
          if (svn_swig_ConvertPtrString(result, (void **)&tmp_creds,
                "svn_auth_cred_username_t *"))
            {
              err = type_conversion_error("svn_auth_cred_username_t *");
            }
          else
            {
              creds = apr_pcalloc(pool, sizeof(*creds));
              creds->username = tmp_creds->username ?
                apr_pstrdup(pool, tmp_creds->username) : NULL;
              creds->may_save = tmp_creds->may_save;
            }
        }
      Py_DECREF(result);
    }
  svn_swig_py_release_py_lock();
  *cred = creds;
  return err;
}


svn_error_t *
svn_swig_py_auth_ssl_server_trust_prompt_func(
    svn_auth_cred_ssl_server_trust_t **cred,
    void *baton,
    const char *realm,
    apr_uint32_t failures,
    const svn_auth_ssl_server_cert_info_t *cert_info,
    svn_boolean_t may_save,
    apr_pool_t *pool)
{
  PyObject *function = baton;
  PyObject *result;
  svn_auth_cred_ssl_server_trust_t *creds = NULL;
  svn_error_t *err = SVN_NO_ERROR;

  if ((function == NULL) || (function == Py_None))
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallFunction(function, (char *)"slO&lO&",
                  realm, failures, make_ob_auth_ssl_server_cert_info,
                  cert_info, may_save, make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      if (result != Py_None)
        {
          svn_auth_cred_ssl_server_trust_t *tmp_creds = NULL;
          if (svn_swig_ConvertPtrString
              (result, (void **)&tmp_creds,
               "svn_auth_cred_ssl_server_trust_t *"))
            {
              err = type_conversion_error
                ("svn_auth_cred_ssl_server_trust_t *");
            }
          else
            {
              creds = apr_pcalloc(pool, sizeof(*creds));
              *creds = *tmp_creds;
            }
        }
      Py_DECREF(result);
    }

  svn_swig_py_release_py_lock();
  *cred = creds;
  return err;
}

svn_error_t *
svn_swig_py_auth_ssl_client_cert_prompt_func(
    svn_auth_cred_ssl_client_cert_t **cred,
    void *baton,
    const char *realm,
    svn_boolean_t may_save,
    apr_pool_t *pool)
{
  PyObject *function = baton;
  PyObject *result;
  svn_auth_cred_ssl_client_cert_t *creds = NULL;
  svn_error_t *err = SVN_NO_ERROR;

  if ((function == NULL) || (function == Py_None))
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallFunction(function,
                                      (char *)"slO&",
                                      realm, may_save,
                                      make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      if (result != Py_None)
        {
          svn_auth_cred_ssl_client_cert_t *tmp_creds = NULL;
          if (svn_swig_ConvertPtrString
              (result, (void **)&tmp_creds,
               "svn_auth_cred_ssl_client_cert_t *"))
            {
              err = type_conversion_error("svn_auth_cred_ssl_client_cert_t *");
            }
          else
            {
              creds = apr_pcalloc(pool, sizeof(*creds));
              creds->cert_file = tmp_creds->cert_file ?
                apr_pstrdup(pool, tmp_creds->cert_file) : NULL;
              creds->may_save = tmp_creds->may_save;
            }
        }
      Py_DECREF(result);
    }
  svn_swig_py_release_py_lock();
  *cred = creds;
  return err;
}

svn_error_t *
svn_swig_py_auth_ssl_client_cert_pw_prompt_func(
    svn_auth_cred_ssl_client_cert_pw_t **cred,
    void *baton,
    const char *realm,
    svn_boolean_t may_save,
    apr_pool_t *pool)
{
  PyObject *function = baton;
  PyObject *result;
  svn_auth_cred_ssl_client_cert_pw_t *creds = NULL;
  svn_error_t *err = SVN_NO_ERROR;

  if ((function == NULL) || (function == Py_None))
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallFunction(function,
                                      (char *)"slO&",
                                      realm, may_save,
                                      make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      if (result != Py_None)
        {
          svn_auth_cred_ssl_client_cert_pw_t *tmp_creds = NULL;
          if (svn_swig_ConvertPtrString
              (result, (void **)&tmp_creds,
               "svn_auth_cred_ssl_client_cert_pw_t *"))
            {
              err = type_conversion_error
                ("svn_auth_cred_ssl_client_cert_pw_t *");
            }
          else
            {
              creds = apr_pcalloc(pool, sizeof(*creds));
              creds->password = tmp_creds->password ?
                apr_pstrdup(pool, tmp_creds->password) : NULL;
              creds->may_save = tmp_creds->may_save;
            }
        }
      Py_DECREF(result);
    }
  svn_swig_py_release_py_lock();
  *cred = creds;
  return err;
}

/* svn_ra_callbacks_t */
static svn_error_t *
ra_callbacks_open_tmp_file(apr_file_t **fp,
                           void *callback_baton,
                           apr_pool_t *pool)
{
  PyObject *callbacks = (PyObject *)callback_baton;
  PyObject *py_callback, *result;
  svn_error_t *err = SVN_NO_ERROR;

  *fp = NULL;

  svn_swig_py_acquire_py_lock();

  py_callback = PyObject_GetAttrString(callbacks, (char *)"open_tmp_file");
  if (py_callback == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }
  else if (py_callback == Py_None)
    {
      goto finished;
    }

  if ((result = PyObject_CallFunction(py_callback,
                                      (char *)"O&",
                                      make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else if (result != Py_None)
    {
      *fp = svn_swig_py_make_file(result, pool);
      if (*fp == NULL)
       {
      	  err = callback_exception_error();
       }
    }

  Py_XDECREF(result);
finished:
  Py_XDECREF(py_callback);
  svn_swig_py_release_py_lock();
  return err;
}

/* svn_ra_callbacks_t */
static svn_error_t *
ra_callbacks_get_wc_prop(void *baton,
                         const char *path,
                         const char *name,
                         const svn_string_t **value,
                         apr_pool_t *pool)
{
  PyObject *callbacks = (PyObject *)baton;
  PyObject *py_callback, *result;
  svn_error_t *err = SVN_NO_ERROR;

  *value = NULL;

  svn_swig_py_acquire_py_lock();

  py_callback = PyObject_GetAttrString(callbacks, (char *)"get_wc_prop");
  if (py_callback == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }
  else if (py_callback == Py_None)
    {
      goto finished;
    }

  if ((result = PyObject_CallFunction(py_callback,
                                      (char *)"ssO&", path, name,
                                      make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else if (result != Py_None)
    {
      char *buf;
      Py_ssize_t len;
      if (PyString_AsStringAndSize(result, &buf, &len) == -1)
        {
      	  err = callback_exception_error();
        }
      else
        {
          *value = svn_string_ncreate(buf, len, pool);
        }
    }

  Py_XDECREF(result);
finished:
  Py_XDECREF(py_callback);
  svn_swig_py_release_py_lock();
  return err;
}

/* svn_ra_callbacks_t */
static svn_error_t *
ra_callbacks_push_or_set_wc_prop(const char *callback,
                                 void *baton,
                                 const char *path,
                                 const char *name,
                                 const svn_string_t *value,
                                 apr_pool_t *pool)
{
  PyObject *callbacks = (PyObject *)baton;
  PyObject *py_callback, *py_value, *result;
  svn_error_t *err = SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  py_callback = PyObject_GetAttrString(callbacks, (char *)callback);
  if (py_callback == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }
  else if (py_callback == Py_None)
    {
      goto finished;
    }

  if ((py_value = PyString_FromStringAndSize(value->data, value->len)) == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }

  if ((result = PyObject_CallFunction(py_callback,
                                      (char *)"ssOO&", path, name, py_value,
                                      make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }

  Py_XDECREF(result);
finished:
  Py_XDECREF(py_callback);
  svn_swig_py_release_py_lock();
  return err;
}

/* svn_ra_callbacks_t */
static svn_error_t *
ra_callbacks_set_wc_prop(void *baton,
                         const char *path,
                         const char *name,
                         const svn_string_t *value,
                         apr_pool_t *pool)
{
  return ra_callbacks_push_or_set_wc_prop("set_wc_prop", baton, path,
                                          name, value, pool);
}

/* svn_ra_callbacks_t */
static svn_error_t *
ra_callbacks_push_wc_prop(void *baton,
                          const char *path,
                          const char *name,
                          const svn_string_t *value,
                          apr_pool_t *pool)
{
  return ra_callbacks_push_or_set_wc_prop("push_wc_prop", baton, path,
                                          name, value, pool);
}

/* svn_ra_callbacks_t */
static svn_error_t *
ra_callbacks_invalidate_wc_props(void *baton,
                                 const char *path,
                                 const char *name,
                                 apr_pool_t *pool)
{
  PyObject *callbacks = (PyObject *)baton;
  PyObject *py_callback, *result;
  svn_error_t *err = SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  py_callback = PyObject_GetAttrString(callbacks,
                                       (char *)"invalidate_wc_props");
  if (py_callback == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }
  else if (py_callback == Py_None)
    {
      goto finished;
    }

  if ((result = PyObject_CallFunction(py_callback,
                                      (char *)"ssO&", path, name,
                                      make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }

  Py_XDECREF(result);
finished:
  Py_XDECREF(py_callback);
  svn_swig_py_release_py_lock();
  return err;
}

/* svn_ra_callbacks_t */
static void
ra_callbacks_progress_func(apr_off_t progress,
                           apr_off_t total,
                           void *baton,
                           apr_pool_t *pool)
{
  PyObject *callbacks = (PyObject *)baton;
  PyObject *py_callback, *py_progress, *py_total, *result;

  py_progress = py_total = NULL;

  svn_swig_py_acquire_py_lock();

  py_callback = PyObject_GetAttrString(callbacks,
                                       (char *)"progress_func");
  if (py_callback == NULL)
    {
      /* Ouch, no way to pass on exceptions! */
      /* err = callback_exception_error(); */
      goto finished;
    }
  else if (py_callback == Py_None)
    {
      goto finished;
    }

  /* Create PyLongs for progress and total up-front, rather than
     passing them directly, so we don't have to worry about the size
     (if apr_off_t is 4 bytes, we'd better use the l specifier; if 8
     bytes, better use L...) */
  if ((py_progress = PyLong_FromLongLong(progress)) == NULL)
    {
      /* Ouch, no way to pass on exceptions! */
      /* err = callback_exception_error(); */
      goto finished;
    }
  if ((py_total = PyLong_FromLongLong(total)) == NULL)
    {
      /* Ouch, no way to pass on exceptions! */
      /* err = callback_exception_error(); */
      goto finished;
    }
  if ((result = PyObject_CallFunction(py_callback,
                                      (char *)"OOO&", py_progress, py_total,
                                      make_ob_pool, pool)) == NULL)
    {
      /* Ouch, no way to pass on exceptions! */
      /* err = callback_exception_error(); */
    }

  Py_XDECREF(result);
finished:
  Py_XDECREF(py_callback);
  Py_XDECREF(py_progress);
  Py_XDECREF(py_total);
  svn_swig_py_release_py_lock();
  /* Sure hope nothing went wrong... */
  /* return err; */
}

/* svn_ra_callbacks_t */
static svn_error_t *
ra_callbacks_cancel_func(void *baton)
{
  PyObject *callbacks = (PyObject *)baton;
  PyObject *py_callback;
  svn_error_t *err;

  svn_swig_py_acquire_py_lock();
  py_callback = PyObject_GetAttrString(callbacks,
                                       (char *)"cancel_func");
  svn_swig_py_release_py_lock();

  err = svn_swig_py_cancel_func(py_callback);

  svn_swig_py_acquire_py_lock();
  Py_XDECREF(py_callback);
  svn_swig_py_release_py_lock();

  return err;
}

/* svn_ra_callbacks_t */
static svn_error_t *
ra_callbacks_get_client_string(void *baton,
                               const char **name,
                               apr_pool_t *pool)
{
  PyObject *callbacks = (PyObject *)baton;
  PyObject *py_callback, *result;
  svn_error_t *err = SVN_NO_ERROR;

  *name = NULL;

  svn_swig_py_acquire_py_lock();

  py_callback = PyObject_GetAttrString(callbacks, (char *)"get_client_string");
  if (py_callback == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }
  else if (py_callback == Py_None)
    {
      goto finished;
    }

  if ((result = PyObject_CallFunction(py_callback,
                                      (char *)"O&",
                                      make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else if (result != Py_None)
    {
      if ((*name = PyString_AsString(result)) == NULL)
        {
      	  err = callback_exception_error();
        }
    }

  Py_XDECREF(result);
finished:
  Py_XDECREF(py_callback);
  svn_swig_py_release_py_lock();
  return err;
}

void
svn_swig_py_setup_ra_callbacks(svn_ra_callbacks2_t **callbacks,
                               void **baton,
                               PyObject *py_callbacks,
                               apr_pool_t *pool)
{
  svn_error_t *err = svn_ra_create_callbacks(callbacks, pool);
  PyObject *py_auth_baton;

  if (err)
    {
      svn_swig_py_svn_exception(err);
      return;
    }

  (*callbacks)->open_tmp_file = ra_callbacks_open_tmp_file;

  py_auth_baton = PyObject_GetAttrString(py_callbacks, (char *)"auth_baton");

  if (svn_swig_ConvertPtrString(py_auth_baton,
                                (void **)&((*callbacks)->auth_baton),
                                "svn_auth_baton_t *"))
    {
      err = type_conversion_error("svn_auth_baton_t *");
      svn_swig_py_svn_exception(err);
      Py_XDECREF(py_auth_baton);
      return;
    }

  Py_XDECREF(py_auth_baton);

  (*callbacks)->get_wc_prop = ra_callbacks_get_wc_prop;
  (*callbacks)->set_wc_prop = ra_callbacks_set_wc_prop;
  (*callbacks)->push_wc_prop = ra_callbacks_push_wc_prop;
  (*callbacks)->invalidate_wc_props = ra_callbacks_invalidate_wc_props;
  (*callbacks)->progress_func = ra_callbacks_progress_func;
  (*callbacks)->progress_baton = py_callbacks;
  (*callbacks)->cancel_func = ra_callbacks_cancel_func;
  (*callbacks)->get_client_string = ra_callbacks_get_client_string;

  *baton = py_callbacks;
}

svn_error_t *svn_swig_py_commit_callback2(const svn_commit_info_t *commit_info,
                                          void *baton,
                                          apr_pool_t *pool)
{
  PyObject *receiver = baton;
  PyObject *result;
  svn_error_t *err = SVN_NO_ERROR;

  if ((receiver == NULL) || (receiver == Py_None))
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallFunction(receiver,
                                      (char *)"O&O&",
                                      make_ob_commit_info, commit_info,
                                      make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      if (result != Py_None)
        err = callback_bad_return_error("Not None");
      Py_DECREF(result);
    }

  svn_swig_py_release_py_lock();

  return err;
}

svn_error_t *svn_swig_py_commit_callback(svn_revnum_t new_revision,
                                         const char *date,
                                         const char *author,
                                         void *baton)
{
  PyObject *receiver = baton;
  PyObject *result;
  svn_error_t *err = SVN_NO_ERROR;

  if ((receiver == NULL) || (receiver == Py_None))
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallFunction(receiver,
                                      (char *)"lss",
                                      new_revision, date, author)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      if (result != Py_None)
        err = callback_bad_return_error("Not None");
      Py_DECREF(result);
    }

  svn_swig_py_release_py_lock();

  return err;
}

svn_error_t *svn_swig_py_ra_file_rev_handler_func(
                    void *baton,
                    const char *path,
                    svn_revnum_t rev,
                    apr_hash_t *rev_props,
                    svn_txdelta_window_handler_t *delta_handler,
                    void **delta_baton,
                    apr_array_header_t *prop_diffs,
                    apr_pool_t *pool)
{
  PyObject *handler = baton;
  PyObject *result, *py_rev_props = NULL, *py_prop_diffs = NULL;
  svn_error_t *err = SVN_NO_ERROR;

  if ((handler == NULL) || (handler == Py_None))
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  py_rev_props = svn_swig_py_prophash_to_dict(rev_props);
  if (py_rev_props == NULL)
    {
      err = type_conversion_error("apr_hash_t *");
      goto error;
    }

  py_prop_diffs = svn_swig_py_proparray_to_dict(prop_diffs);

  if (py_prop_diffs == NULL)
    {
      err = type_conversion_error("apr_array_header_t *");
      goto error;
    }

  if ((result = PyObject_CallFunction(handler,
                                      (char *)"slOOO&",
                                      path, rev, py_rev_props, py_prop_diffs,
                                      make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else
    {
      if (result != Py_None)
        err = callback_bad_return_error("Not None");

      /* FIXME: Support returned TxDeltaWindow object and
       * set delta_handler and delta_baton */
      *delta_handler = NULL;
      *delta_baton = NULL;

      Py_XDECREF(result);
    }

error:

  Py_XDECREF(py_rev_props);
  Py_XDECREF(py_prop_diffs);

  svn_swig_py_release_py_lock();

  return err;
}

svn_error_t *svn_swig_py_ra_lock_callback(
                    void *baton,
                    const char *path,
                    svn_boolean_t do_lock,
                    const svn_lock_t *lock,
                    svn_error_t *ra_err,
                    apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  PyObject *py_callback = baton, *result;

  if (py_callback == NULL || py_callback == Py_None)
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallFunction(py_callback,
                                     (char *)"sbO&O&",
                                     path, do_lock,
                                     make_ob_lock, lock,
                                     make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else if (result != Py_None)
    {
      err = callback_bad_return_error("Not None");
    }

  Py_XDECREF(result);

  svn_swig_py_release_py_lock();

  return err;
}

static svn_error_t *reporter_set_path(void *report_baton,
                           const char *path,
                           svn_revnum_t revision,
                           svn_boolean_t start_empty,
                           const char *lock_token,
                           apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  PyObject *py_reporter = report_baton, *result;

  if (py_reporter == NULL || py_reporter == Py_None)
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallMethod(py_reporter,
                                    (char *)"set_path",
                                    (char *)"slbsO&",
                                    path, revision,
                                    start_empty, lock_token,
                                    make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else if (result != Py_None)
    {
      err = callback_bad_return_error("Not None");
    }

  Py_XDECREF(result);

  svn_swig_py_release_py_lock();

  return err;
}

static svn_error_t *reporter_delete_path(void *report_baton,
                         const char *path,
                        apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  PyObject *py_reporter = report_baton, *result;

  if (py_reporter == NULL || py_reporter == Py_None)
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallMethod(py_reporter,
                                    (char *)"delete_path",
                                    (char *)"sO&",
                                    path,
                                    make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else if (result != Py_None)
    {
      err = callback_bad_return_error("Not None");
    }

  Py_XDECREF(result);

  svn_swig_py_release_py_lock();

  return err;
}

static svn_error_t *reporter_link_path(void *report_baton,
                            const char *path,
                            const char *url,
                            svn_revnum_t revision,
                            svn_boolean_t start_empty,
                            const char *lock_token,
                            apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  PyObject *py_reporter = report_baton, *result;

  if (py_reporter == NULL || py_reporter == Py_None)
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallMethod(py_reporter,
                                    (char *)"link_path",
                                    (char *)"sslbsO&",
                                    path, url, revision,
                                    start_empty, lock_token,
                                    make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else if (result != Py_None)
    {
      err = callback_bad_return_error("Not None");
    }

  Py_XDECREF(result);

  svn_swig_py_release_py_lock();

  return err;
}

static svn_error_t *reporter_finish_report(void *report_baton,
                                apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;

  PyObject *py_reporter = report_baton, *result;

  if (py_reporter == NULL || py_reporter == Py_None)
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallMethod(py_reporter,
                                    (char *)"finish_report",
                                    (char *)"O&",
                                    make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else if (result != Py_None)
    {
      err = callback_bad_return_error("Not None");
    }

  Py_XDECREF(result);

  svn_swig_py_release_py_lock();

  return err;
}

static svn_error_t *reporter_abort_report(void *report_baton,
                               apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;

  PyObject *py_reporter = report_baton, *result;

  if (py_reporter == NULL || py_reporter == Py_None)
    return SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  if ((result = PyObject_CallMethod(py_reporter,
                                    (char *)"abort_report",
                                    (char *)"O&",
                                    make_ob_pool, pool)) == NULL)
    {
      err = callback_exception_error();
    }
  else if (result != Py_None)
    {
      err = callback_bad_return_error("Not None");
    }

  Py_XDECREF(result);

  svn_swig_py_release_py_lock();

  return err;
}

const svn_ra_reporter2_t swig_py_ra_reporter2 = {
    reporter_set_path,
    reporter_delete_path,
    reporter_link_path,
    reporter_finish_report,
    reporter_abort_report
};

/* svn_wc_diff_callbacks2_t */
static svn_error_t *
wc_diff_callbacks2_file_changed_or_added(const char *callback,
                                         svn_wc_adm_access_t *adm_access,
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
  PyObject *callbacks = (PyObject *)diff_baton;
  PyObject *py_callback;
  PyObject *result = NULL;
  int py_contentstate, py_propstate;
  svn_error_t *err = SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  py_callback = PyObject_GetAttrString(callbacks, (char *)callback);
  if (py_callback == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }
  else if (py_callback == Py_None)
    {
      goto finished;
    }

  result = PyObject_CallFunction(py_callback,
                                 (char *)"O&sssllssO&O&",
                                 make_ob_wc_adm_access, adm_access,
                                 path,
                                 tmpfile1, tmpfile2,
                                 rev1, rev2,
                                 mimetype1, mimetype2,
                                 svn_swig_py_proparray_to_dict, propchanges,
                                 svn_swig_py_prophash_to_dict, originalprops);
  if (result == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }
  if (!PyArg_ParseTuple(result, (char *)"ii", &py_contentstate, &py_propstate))
    {
      err = callback_exception_error();
      goto finished;
    }
  if (contentstate != NULL)
    *contentstate = py_contentstate;
  if (propstate != NULL)
    *propstate = py_propstate;

finished:
  Py_XDECREF(result);
  Py_XDECREF(py_callback);
  svn_swig_py_release_py_lock();
  return err;
}

/* svn_wc_diff_callbacks2_t */
static svn_error_t *
wc_diff_callbacks2_file_changed(svn_wc_adm_access_t *adm_access,
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
  return wc_diff_callbacks2_file_changed_or_added("file_changed",
                                                  adm_access,
                                                  contentstate,
                                                  propstate,
                                                  path,
                                                  tmpfile1,
                                                  tmpfile2,
                                                  rev1, rev2,
                                                  mimetype1,
                                                  mimetype2,
                                                  propchanges,
                                                  originalprops,
                                                  diff_baton);
}

/* svn_wc_diff_callbacks2_t */
static svn_error_t *
wc_diff_callbacks2_file_added(svn_wc_adm_access_t *adm_access,
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
  return wc_diff_callbacks2_file_changed_or_added("file_added",
                                                  adm_access,
                                                  contentstate,
                                                  propstate,
                                                  path,
                                                  tmpfile1,
                                                  tmpfile2,
                                                  rev1, rev2,
                                                  mimetype1,
                                                  mimetype2,
                                                  propchanges,
                                                  originalprops,
                                                  diff_baton);
}

/* svn_wc_diff_callbacks2_t */
static svn_error_t *
wc_diff_callbacks2_file_deleted(svn_wc_adm_access_t *adm_access,
                                svn_wc_notify_state_t *state,
                                const char *path,
                                const char *tmpfile1,
                                const char *tmpfile2,
                                const char *mimetype1,
                                const char *mimetype2,
                                apr_hash_t *originalprops,
                                void *diff_baton)
{
  PyObject *callbacks = (PyObject *)diff_baton;
  PyObject *py_callback, *result = NULL;
  long py_state;
  svn_error_t *err = SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  py_callback = PyObject_GetAttrString(callbacks, (char *)"file_deleted");
  if (py_callback == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }
  else if (py_callback == Py_None)
    {
      goto finished;
    }

  result = PyObject_CallFunction(py_callback,
                                 (char *)"O&sssssO&",
                                 make_ob_wc_adm_access, adm_access,
                                 path,
                                 tmpfile1, tmpfile2,
                                 mimetype1, mimetype2,
                                 svn_swig_py_prophash_to_dict, originalprops);
  if (result == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }
  py_state = PyInt_AsLong(result);
  if (py_state == -1 && PyErr_Occurred())
    {
      err = callback_exception_error();
      goto finished;
    }
  if (state != NULL)
    *state = py_state;

finished:
  Py_XDECREF(result);
  Py_XDECREF(py_callback);
  svn_swig_py_release_py_lock();
  return err;
}

/* svn_wc_diff_callbacks2_t */
static svn_error_t *
wc_diff_callbacks2_dir_added(svn_wc_adm_access_t *adm_access,
                             svn_wc_notify_state_t *state,
                             const char *path,
                             svn_revnum_t rev,
                             void *diff_baton)
{
  PyObject *callbacks = (PyObject *)diff_baton;
  PyObject *py_callback, *result = NULL;
  long py_state;
  svn_error_t *err = SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  py_callback = PyObject_GetAttrString(callbacks, (char *)"dir_added");
  if (py_callback == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }
  else if (py_callback == Py_None)
    {
      goto finished;
    }

  result = PyObject_CallFunction(py_callback,
                                 (char *)"O&sl",
                                 make_ob_wc_adm_access, adm_access,
                                 path, rev);
  if (result == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }
  py_state = PyInt_AsLong(result);
  if (py_state == -1 && PyErr_Occurred())
    {
      err = callback_exception_error();
      goto finished;
    }
  if (state != NULL)
    *state = py_state;

finished:
  Py_XDECREF(result);
  Py_XDECREF(py_callback);
  svn_swig_py_release_py_lock();
  return err;
}

/* svn_wc_diff_callbacks2_t */
static svn_error_t *
wc_diff_callbacks2_dir_deleted(svn_wc_adm_access_t *adm_access,
                               svn_wc_notify_state_t *state,
                               const char *path,
                               void *diff_baton)
{
  PyObject *callbacks = (PyObject *)diff_baton;
  PyObject *py_callback, *result = NULL;
  long py_state;
  svn_error_t *err = SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  py_callback = PyObject_GetAttrString(callbacks, (char *)"dir_deleted");
  if (py_callback == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }
  else if (py_callback == Py_None)
    {
      goto finished;
    }

  result = PyObject_CallFunction(py_callback,
                                 (char *)"O&s",
                                 make_ob_wc_adm_access, adm_access, path);
  if (result == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }
  py_state = PyInt_AsLong(result);
  if (py_state == -1 && PyErr_Occurred())
    {
      err = callback_exception_error();
      goto finished;
    }
  if (state != NULL)
    *state = py_state;

finished:
  Py_XDECREF(result);
  Py_XDECREF(py_callback);
  svn_swig_py_release_py_lock();
  return err;
}

/* svn_wc_diff_callbacks2_t */
static svn_error_t *
wc_diff_callbacks2_dir_props_changed(svn_wc_adm_access_t *adm_access,
                                     svn_wc_notify_state_t *state,
                                     const char *path,
                                     const apr_array_header_t *propchanges,
                                     apr_hash_t *originalprops,
                                     void *diff_baton)
{
  PyObject *callbacks = (PyObject *)diff_baton;
  PyObject *py_callback;
  PyObject *result = NULL;
  long py_state;
  svn_error_t *err = SVN_NO_ERROR;

  svn_swig_py_acquire_py_lock();

  py_callback = PyObject_GetAttrString(callbacks, (char *)"dir_props_changed");
  if (py_callback == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }
  else if (py_callback == Py_None)
    {
      goto finished;
    }

  result = PyObject_CallFunction(py_callback,
                                 (char *)"O&sO&O&",
                                 make_ob_wc_adm_access, adm_access,
                                 path,
                                 svn_swig_py_proparray_to_dict, propchanges,
                                 svn_swig_py_prophash_to_dict, originalprops);
  if (result == NULL)
    {
      err = callback_exception_error();
      goto finished;
    }
  py_state = PyInt_AsLong(result);
  if (py_state == -1 && PyErr_Occurred())
    {
      err = callback_exception_error();
      goto finished;
    }
  if (state != NULL)
    *state = py_state;

finished:
  Py_XDECREF(result);
  Py_XDECREF(py_callback);
  svn_swig_py_release_py_lock();
  return err;
}

svn_wc_diff_callbacks2_t *
svn_swig_py_setup_wc_diff_callbacks2(void **baton,
                                     PyObject *py_callbacks,
                                     apr_pool_t *pool)
{
  svn_wc_diff_callbacks2_t *callbacks = apr_palloc(pool, sizeof(*callbacks));
  *baton = py_callbacks;
  callbacks->file_changed       = wc_diff_callbacks2_file_changed;
  callbacks->file_added         = wc_diff_callbacks2_file_added;
  callbacks->file_deleted       = wc_diff_callbacks2_file_deleted;
  callbacks->dir_added          = wc_diff_callbacks2_dir_added;
  callbacks->dir_deleted        = wc_diff_callbacks2_dir_deleted;
  callbacks->dir_props_changed  = wc_diff_callbacks2_dir_props_changed;
  return callbacks;
}

PyObject *
svn_swig_py_txdelta_window_t_ops_get(svn_txdelta_window_t *window,
                                     swig_type_info * op_type_info,
                                     PyObject *window_pool)
{
  PyObject *result = PyList_New(window->num_ops);
  int i;

  for (i = 0; i < window->num_ops; ++i)
      PyList_SET_ITEM(result, i,
                      svn_swig_NewPointerObj(window->ops + i, op_type_info,
                                             window_pool, NULL));

  return result;
}

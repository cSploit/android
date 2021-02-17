/*
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
 *
 * svn_string.swg: This is a child file of svn_types.swg, and should
 *   not be included directly.  This file should contain typemaps that
 *   deal with svn_string_t, svn_stringbuf_t and char * string types.
 */

typedef struct svn_stringbuf_t svn_stringbuf_t;
typedef struct svn_string_t svn_string_t;

/* -----------------------------------------------------------------------
   generic OUT param typemap for svn_string(buf)_t. we can share these
   because we only refer to the ->data and ->len values.
*/
#ifdef SWIGPYTHON
%typemap(argout) RET_STRING {
    PyObject *s;
    if (*$1 == NULL) {
        Py_INCREF(Py_None);
        s = Py_None;
    }
    else {
        s = PyString_FromStringAndSize((*$1)->data, (*$1)->len);
        if (s == NULL)
            SWIG_fail;
    }
    %append_output(s);
}
#endif
#ifdef SWIGPERL
%typemap(argout) RET_STRING {
  if (*$1) {
    %append_output(sv_2mortal(newSVpvn((*$1)->data, (*$1)->len)));
  } else {
    %append_output(&PL_sv_undef);
  }
}
#endif
#ifdef SWIGRUBY
%typemap(argout) RET_STRING {
  if (*$1) {
    %append_output(rb_str_new((*$1)->data, (*$1)->len));
  } else {
    %append_output(Qnil);
  }
}
#endif

%apply RET_STRING {
  svn_string_t **,
  svn_stringbuf_t **
};

/* -----------------------------------------------------------------------
   TYPE: svn_stringbuf_t
*/

#ifdef SWIGPYTHON
%typemap(in) svn_stringbuf_t * {
    if (!PyString_Check($input)) {
        PyErr_SetString(PyExc_TypeError, "not a string");
        SWIG_fail;
    }
    $1 = svn_stringbuf_ncreate(PyString_AS_STRING($input),
                               PyString_GET_SIZE($input),
                               /* ### gah... what pool to use? */
                               _global_pool);
}
#endif

#ifdef SWIGPERL
%typemap(in) svn_stringbuf_t * {
    apr_size_t len;
    char *buf;

    if (!SvOK($input)) {
        $1 = NULL;
    } else if (SvPOK($input)) {
        buf = SvPV($input, len);
        /* Another case of ugly pool handling, this should use the current
           default pool, or make a new one if it doesn't exist yet */
        $1 = svn_stringbuf_ncreate(buf,len,
                                   svn_swig_pl_make_pool ((SV *)NULL));
    } else {
        croak("Not a string");
    }
}
#endif

#ifdef SWIGRUBY
%typemap(in) svn_stringbuf_t *
{
  if (NIL_P($input)) {
    $1 = NULL;
  } else {
    $1 = svn_stringbuf_ncreate(StringValuePtr($input),
                               RSTRING_LEN($input),
                               _global_pool);
  }
}

%typemap(in) svn_stringbuf_t *node_name
{
  if (NIL_P($input)) {
    $1 = NULL;
  } else {
    VALUE rb_pool;
    apr_pool_t *pool;

    svn_swig_rb_get_pool(argc, argv, self, &rb_pool, &pool);

    $1 = svn_stringbuf_ncreate(StringValuePtr($input),
                               RSTRING_LEN($input),
                               pool);
  }
}
#endif


#ifdef SWIGPYTHON
%typemap(out) svn_stringbuf_t * {
    $result = PyString_FromStringAndSize($1->data, $1->len);
}
#endif

#ifdef SWIGPERL
%typemap(out) svn_stringbuf_t * {
    SV *sv = sv_newmortal();
    sv_setpvn(sv,$1->data,$1->len);
    $result = sv;
    argvi++;
}
#endif

#ifdef SWIGRUBY
%typemap(out) svn_stringbuf_t *
{
  $result = rb_str_new($1->data, $1->len);
}

%typemap(argout) svn_stringbuf_t *output
{
  %append_output(rb_str_new($1->data, $1->len));
}
#endif

/* -----------------------------------------------------------------------
   TYPE: svn_string_t
*/

/* const svn_string_t * is always an input parameter */
#ifdef SWIGPYTHON
%typemap(in) const svn_string_t * (svn_string_t value) {
    if ($input == Py_None)
        $1 = NULL;
    else {
        if (!PyString_Check($input)) {
            PyErr_SetString(PyExc_TypeError, "not a string");
            SWIG_fail;
        }
        value.data = PyString_AS_STRING($input);
        value.len = PyString_GET_SIZE($input);
        $1 = &value;
    }
}
#endif
#ifdef SWIGPERL
%typemap(in) const svn_string_t * (svn_string_t value) {
    if (SvOK($input)) {
        value.data = SvPV($input, value.len);
        $1 = &value;
    }
    else {
        $1 = NULL;
    }
}
#endif
#ifdef SWIGRUBY
%typemap(in) const svn_string_t * (svn_string_t value)
{
  if (NIL_P($input)) {
    $1 = NULL;
  } else {
    value.data = StringValuePtr($input);
    value.len = RSTRING_LEN($input);
    $1 = &value;
  }
}
#endif

/* when storing an svn_string_t* into a structure, we must allocate the
   svn_string_t structure on the heap. */
#ifdef SWIGPYTHON
%typemap(memberin) const svn_string_t * {
    $1 = svn_string_dup($input, _global_pool);
}
#endif
#ifdef SWIGPERL
%typemap(memberin) const svn_string_t * {
    $1 = svn_string_dup($input, _global_pool);
}
#endif
#ifdef SWIGRUBY
%typemap(memberin) const svn_string_t * {
    $1 = svn_string_dup($input, _global_pool);
}
#endif

#ifdef SWIGPYTHON
%typemap(out) svn_string_t * {
    $result = PyString_FromStringAndSize($1->data, $1->len);
}
#endif
#ifdef SWIGPERL
%typemap(out) svn_string_t * {
    $result = sv_2mortal(newSVpv($1->data, $1->len));
    ++argvi;
}
#endif
#ifdef SWIGRUBY
%typemap(out) svn_string_t * {
  if ($1) {
    $result = rb_str_new($1->data, $1->len);
  } else {
    $result = Qnil;
  }
}
#endif

/* -----------------------------------------------------------------------
   define a way to return a 'const char *'
*/
#ifdef SWIGPYTHON
%typemap(argout) const char **OUTPUT {
    PyObject *s;
    if (*$1 == NULL) {
        Py_INCREF(Py_None);
        s = Py_None;
    }
    else {
        s = PyString_FromString(*$1);
        if (s == NULL)
            SWIG_fail;
    }
    %append_output(s);
}
#endif

#ifdef SWIGPERL
%typemap(argout) const char **OUTPUT {
  if (*$1 == NULL) {
    %append_output(&PL_sv_undef);
  } else {
    %append_output(sv_2mortal(newSVpv(*$1, 0)));
  }
}
#endif

#ifdef SWIGRUBY
%typemap(argout) const char **OUTPUT
{
  if (*$1) {
    %append_output(rb_str_new2(*$1));
  } else {
    %append_output(Qnil);
  }
}
#endif

/* svn_wc_get_ancestry() lacks a 'const' */
%apply const char **OUTPUT { const char **, char **url };

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
 * svn_containers.swg: This is a child file of svn_types.swg, and
 *   should not be included directly.  This file contains typemaps
 *   that deal with "container" data types (e.g. apr_hash_t,
 *   apr_array_header_t, etc.).
 */

/* =======================================================================
   %typemap(argout) apr_hash_t **
*/

#ifdef SWIGPYTHON
%define %hash_argout_typemap(ParamName, ValueType)
%typemap(argout) apr_hash_t **ParamName {
  %append_output(svn_swig_py_convert_hash(*$1,
                                          $descriptor(ValueType),
                                          _global_py_pool));
}
%enddef
#endif
#ifdef SWIGPERL
%define %hash_argout_typemap(ParamName, ValueType)
%typemap(argout) apr_hash_t **ParamName {
  %append_output(svn_swig_pl_convert_hash(*$1, $descriptor(ValueType)));
}
%enddef
#endif
#ifdef SWIGRUBY
%define %hash_argout_typemap(ParamName, ValueType)
%typemap(argout) apr_hash_t **ParamName {
  %append_output(svn_swig_rb_apr_hash_to_hash_swig_type(*$1,
                                                        #ValueType));
}
%enddef
#endif

%hash_argout_typemap(dirents, svn_dirent_t *)
%hash_argout_typemap(locks, svn_lock_t *)
%hash_argout_typemap(cfg_hash, svn_config_t *)

/* -----------------------------------------------------------------------
   Output of apr_hash_t * <const char *, svn_string_t *>
   (commonly used for property name and value pairs)
*/

#ifdef SWIGPYTHON
%typemap(argout) apr_hash_t **HASH_SVN_STRING {
  %append_output(svn_swig_py_prophash_to_dict(*$1));
}
%typemap(argout) apr_hash_t **HASH_SVN_STRING_MAYBENULL {
  %append_output(!*$1 ? Py_None : svn_swig_py_prophash_to_dict(*$1));
}
#endif
#ifdef SWIGPERL
%typemap(argout) apr_hash_t **HASH_SVN_STRING {
  %append_output(svn_swig_pl_prophash_to_hash(*$1));
}
%typemap(argout) apr_hash_t **HASH_SVN_STRING_MAYBENULL {
  %append_output(!*$1 ? &PL_sv_undef : svn_swig_pl_prophash_to_hash(*$1));
}
#endif
#ifdef SWIGRUBY
%typemap(argout) apr_hash_t **HASH_SVN_STRING {
  %append_output(svn_swig_rb_apr_hash_to_hash_svn_string(*$1));
}
%typemap(argout) apr_hash_t **HASH_SVN_STRING_MAYBENULL {
  %append_output(!*$1 ? Qnil : svn_swig_rb_apr_hash_to_hash_svn_string(*$1));
}
#endif
%apply apr_hash_t **HASH_SVN_STRING {
  apr_hash_t **props,
  apr_hash_t **original_props,
  apr_hash_t **table_p,
  apr_hash_t **revprops
};
%apply apr_hash_t **HASH_SVN_STRING_MAYBENULL {
  /* svn_config_read_auth_data() */
  apr_hash_t **hash
};


/* -----------------------------------------------------------------------
   Output of apr_hash_t * <const char *, const char *>
*/

#ifdef SWIGRUBY
%typemap(argout) apr_hash_t **HASH_CSTRING {
  %append_output(svn_swig_rb_apr_hash_to_hash_string(*$1));
}
#endif
#ifdef SWIGPYTHON
%typemap(argout) apr_hash_t **HASH_CSTRING {
  %append_output(svn_swig_py_stringhash_to_dict(*$1));
}
#endif

%apply apr_hash_t **HASH_CSTRING {
  apr_hash_t **type_map,
  apr_hash_t **externals_old,
  apr_hash_t **externals_new,
  apr_hash_t **depths
};

/* -----------------------------------------------------------------------
   apr_hash_t **locations
   svn_repos_trace_node_locations()
   svn_ra_get_locations()
*/

#ifdef SWIGPYTHON
%typemap(argout) apr_hash_t **locations {
  %append_output(svn_swig_py_locationhash_to_dict(*$1));
}
#endif
#ifdef SWIGRUBY
%typemap(argout) apr_hash_t **locations
{
  %append_output(svn_swig_rb_apr_revnum_key_hash_to_hash_string(*$1));
}
#endif

/* -----------------------------------------------------------------------
   apr_hash_t **externals_old
   apr_hash_t **externals_new
   svn_wc_edited_externals()
*/

#ifdef SWIGRUBY
%typemap(argout) apr_hash_t **externals_old, apr_hash_t **externals_new {
  %append_output(svn_swig_rb_apr_hash_to_hash_string(*$1));
}
#endif


/* =======================================================================
   %typemap(in) apr_hash_t *
*/

/* -----------------------------------------------------------------------
   Input of apr_hash_t * <const char *, svn_string_t *>
   (commonly used for property name and value pairs)
*/

#ifdef SWIGPYTHON
%typemap(in) apr_hash_t *PROPHASH
  (apr_pool_t *_global_pool = NULL, PyObject *_global_py_pool = NULL)
{
  if (_global_pool == NULL)
  {
    if (svn_swig_py_get_parent_pool(args, $descriptor(apr_pool_t *),
                                    &_global_py_pool, &_global_pool))
      SWIG_fail;
  }

  $1 = svn_swig_py_prophash_from_dict($input, _global_pool);
  if (PyErr_Occurred()) {
    SWIG_fail;
  }
}

%typemap(out) apr_hash_t *PROPHASH
{
  %append_output(svn_swig_py_prophash_to_dict($1));
}
#endif

#ifdef SWIGPYTHON
%typemap(in) apr_hash_t *changed_paths
  (apr_pool_t *_global_pool = NULL, PyObject *_global_py_pool = NULL)
{
  if (_global_pool == NULL)
  {
    if (svn_swig_py_get_parent_pool(args, $descriptor(apr_pool_t *),
                                    &_global_py_pool, &_global_pool))
      SWIG_fail;
  }

  $1 = svn_swig_py_struct_ptr_hash_from_dict($input,
    $descriptor(svn_log_changed_path_t *), _global_pool);
  if (PyErr_Occurred()) {
    SWIG_fail;
  }
}
%typemap(out) apr_hash_t *changed_paths
{
  %append_output(svn_swig_py_changed_path_hash_to_dict($1));
}

%typemap(in) apr_hash_t *changed_paths2
  (apr_pool_t *_global_pool = NULL, PyObject *_global_py_pool = NULL)
{
  if (_global_pool == NULL)
  {
    if (svn_swig_py_get_parent_pool(args, $descriptor(apr_pool_t *),
                                    &_global_py_pool, &_global_pool))
      SWIG_fail;
  }

  $1 = svn_swig_py_struct_ptr_hash_from_dict($input,
    $descriptor(svn_log_changed_path2_t *), _global_pool);
  if (PyErr_Occurred()) {
    SWIG_fail;
  }
}
%typemap(out) apr_hash_t *changed_paths2
{
  %append_output(svn_swig_py_changed_path2_hash_to_dict($1));
}

/* apr_hash_t * <const char *, svn_config_t *> */
%typemap(in) apr_hash_t *config
  (apr_pool_t *_global_pool = NULL, PyObject *_global_py_pool = NULL)
{
  if (_global_pool == NULL)
  {
    if (svn_swig_py_get_parent_pool(args, $descriptor(apr_pool_t *),
                                    &_global_py_pool, &_global_pool))
      SWIG_fail;
  }

  $1 = svn_swig_py_struct_ptr_hash_from_dict($input,
    $descriptor(svn_config_t *), _global_pool);
  if (PyErr_Occurred())
    SWIG_fail;
}

%typemap(out) apr_hash_t *config
{
  /* HACK. We don't know which pool the config comes from, and we
     can't copy it, because svn_config_t is not copyable. So we'll just
     assume that it's the parent struct's pool. It shouldn't matter
     anyway, because the only way a non-null config can end up in
     svn_client_ctx_t is if we assigned it ourselves, in which case the copy
     that's stored in the Python object will become the result; so this is just
     a safety net. */
  apr_pool_t *parent_pool;
  PyObject *parent_py_pool;
  
  if (svn_swig_py_get_parent_pool(args, $descriptor(apr_pool_t *),
                                  &parent_py_pool, &parent_pool))
    SWIG_fail;
  
  %append_output(svn_swig_py_convert_hash($1, $descriptor(svn_config_t *),
    parent_py_pool));
}

#endif

#ifdef SWIGRUBY
%typemap(in) apr_hash_t *PROPHASH
{
  VALUE rb_pool = Qnil;
  if (!_global_pool) {
    svn_swig_rb_get_pool(argc, argv, self, &rb_pool, &_global_pool);
    svn_swig_rb_push_pool(rb_pool);
  }
  $1 = svn_swig_rb_hash_to_apr_hash_svn_string($input, _global_pool);
  _global_pool = NULL;
  if (!NIL_P(rb_pool)) {
    if (NIL_P($1)) {
      svn_swig_rb_destroy_pool(rb_pool);
    } else {
      svn_swig_rb_set_pool_for_no_swig_type($input, rb_pool);
    }
    svn_swig_rb_pop_pool(_global_svn_swig_rb_pool);
  }
}

%typemap(out) apr_hash_t *PROPHASH
{
  %append_output(svn_swig_rb_apr_hash_to_hash_svn_string($1));
}
#endif

#if defined(SWIGPYTHON) || defined(SWIGRUBY)
%apply apr_hash_t *PROPHASH {
  apr_hash_t *target_props,
  apr_hash_t *source_props,
  apr_hash_t *hash,
  apr_hash_t *props,
  apr_hash_t *revprop_table,
  apr_hash_t *revprops
};
#endif

#ifdef SWIGRUBY
%typemap(out) apr_hash_t *CHANGED_PATH_HASH
{
  VALUE rb_changed_path_hash;
  rb_changed_path_hash =
    svn_swig_rb_apr_hash_to_hash_swig_type($1, "svn_log_changed_path_t *");
  %append_output(rb_changed_path_hash);
}

%apply apr_hash_t *CHANGED_PATH_HASH {
  apr_hash_t *changed_paths
};
#endif

/* -----------------------------------------------------------------------
   Input of scripting hash/dictionary (string => string)
   converted to apr_hash_t (const char * => const char *)
*/
#ifdef SWIGPYTHON
%typemap(in) apr_hash_t *HASH_CSTRING {
  $1 = svn_swig_py_stringhash_from_dict($input, _global_pool);
}
%typemap(in) apr_hash_t *HASH_CSTRING_MAYBENULL {
  /* PYTHON-FIXME: Handle None -> NULL. */
  $1 = svn_swig_py_stringhash_from_dict($input, _global_pool);
}
#endif
#ifdef SWIGPERL
%typemap(in) apr_hash_t *HASH_CSTRING {
  $1 = svn_swig_pl_strings_to_hash($input, _global_pool);
}
%typemap(in) apr_hash_t *HASH_CSTRING_MAYBENULL {
  /* PERL-FIXME: Handle undef -> NULL. */
  $1 = svn_swig_pl_strings_to_hash($input, _global_pool);
}
#endif
#ifdef SWIGRUBY
%typemap(in) apr_hash_t *HASH_CSTRING {
  $1 = svn_swig_rb_hash_to_apr_hash_string($input, _global_pool);
}
%typemap(in) apr_hash_t *HASH_CSTRING_MAYBENULL {
  VALUE rb_pool = Qnil;
  if (!_global_pool) {
    svn_swig_rb_get_pool(argc, argv, self, &rb_pool, &_global_pool);
    svn_swig_rb_push_pool(rb_pool);
  }
  $1 = (NIL_P($input)) ? NULL :
    svn_swig_rb_hash_to_apr_hash_string($input, _global_pool);
  _global_pool = NULL;
  if (!NIL_P(rb_pool)) {
    if (NIL_P($1)) {
      svn_swig_rb_destroy_pool(rb_pool);
    } else {
      svn_swig_rb_set_pool_for_no_swig_type($input, rb_pool);
    }
    svn_swig_rb_pop_pool(_global_svn_swig_rb_pool);
  }
}

%typemap(out) apr_hash_t *HASH_CSTRING_MAYBENULL {
  $result = $1 ? svn_swig_rb_apr_hash_to_hash_string($1) : Qnil;
}
#endif

%apply apr_hash_t *HASH_CSTRING {
  apr_hash_t *path_tokens
};
%apply apr_hash_t *HASH_CSTRING_MAYBENULL {
  apr_hash_t *fs_config,
  apr_hash_t *lock_tokens
};

/* -----------------------------------------------------------------------
   handle config and fs_config in svn_{fs,repos}_create
*/

#ifdef SWIGPERL
%typemap(in) apr_hash_t *config {
    $1 = svn_swig_pl_objs_to_hash_by_name ($input, "svn_config_t *",
                                           _global_pool);
}
#endif
#ifdef SWIGRUBY
%typemap(in) apr_hash_t *config
{
  if (NIL_P($input)) {
    $1 = NULL;
  } else {
    $1 = svn_swig_rb_hash_to_apr_hash_swig_type($input, "svn_config_t *", _global_pool);
  }
}
#endif

#ifdef SWIGRUBY
%typemap(in) apr_hash_t *path_revs
{
  $1 = svn_swig_rb_hash_to_apr_hash_revnum($input, _global_pool);
}
#endif

#ifdef SWIGPYTHON
%typemap(in) apr_hash_t *path_revs
{
  $1 = svn_swig_py_path_revs_hash_from_dict($input, _global_pool);
}
#endif

#ifdef SWIGPERL
%typemap(in) apr_hash_t *path_revs
{
  $1 = svn_swig_pl_objs_to_hash_of_revnum_t($input, _global_pool);
}
#endif

/* =======================================================================
   %typemap(argout) apr_array_header_t **
*/

/* -----------------------------------------------------------------------
   Output of apr_array_header_t * <svn_prop_t *>
*/
#ifdef SWIGRUBY
%typemap(argout) apr_array_header_t **OUTPUT_OF_PROP {
  %append_output(svn_swig_rb_prop_apr_array_to_hash_prop(*$1));
}

%apply apr_array_header_t **OUTPUT_OF_PROP {
  apr_array_header_t **entry_props,
  apr_array_header_t **wc_props,
  apr_array_header_t **regular_props,
  apr_array_header_t **propdiffs
};
#endif

#ifdef SWIGPYTHON
%typemap(argout) apr_array_header_t **OUTPUT_OF_PROP {
  %append_output(svn_swig_py_proparray_to_dict(*$1));
}

%apply apr_array_header_t **OUTPUT_OF_PROP {
  apr_array_header_t **entry_props,
  apr_array_header_t **wc_props,
  apr_array_header_t **regular_props,
  apr_array_header_t **propdiffs
};
#endif

/* -----------------------------------------------------------------------
   Output of apr_array_header_t * <const char *>
*/

#ifdef SWIGPYTHON
%typemap(argout) apr_array_header_t **OUTPUT_OF_CONST_CHAR_P {
  %append_output(svn_swig_py_array_to_list(*$1));
}
#endif
#ifdef SWIGPERL
%typemap(argout) apr_array_header_t **OUTPUT_OF_CONST_CHAR_P {
  %append_output(svn_swig_pl_array_to_list(*$1));
}
#endif
#ifdef SWIGRUBY
%typemap(argout) apr_array_header_t **OUTPUT_OF_CONST_CHAR_P {
  %append_output(svn_swig_rb_apr_array_to_array_string(*$1));
}
#endif

%apply apr_array_header_t **OUTPUT_OF_CONST_CHAR_P {
  apr_array_header_t **patterns,
  apr_array_header_t **logfiles,
  apr_array_header_t **names_p,
  apr_array_header_t **targets_p,
  apr_array_header_t **args_p
};

/* -----------------------------------------------------------------------
   apr_array_header_t **externals_p
   svn_wc_parse_externals_description3()
*/

#ifdef SWIGRUBY
%typemap(argout) apr_array_header_t **externals_p {
  %append_output(svn_swig_rb_apr_array_to_array_external_item2(*$1));
}
#endif

/* svn_client_update3/svn_client_update2 */
#ifdef SWIGPYTHON
%typemap(argout) apr_array_header_t **result_revs {
  %append_output(svn_swig_py_revarray_to_list(*$1));
  if (PyErr_Occurred()) {
    SWIG_fail;
  }
}
#endif

#ifdef SWIGRUBY
%typemap(argout) apr_array_header_t **result_revs {
  %append_output(svn_swig_rb_apr_array_to_array_svn_rev(*$1));
}
#endif

/* =======================================================================
   %typemap(in) apr_array_header_t *
*/

/* -----------------------------------------------------------------------
   apr_array_header_t *location_revisions
   svn_repos_trace_node_locations()
   svn_ra_get_locations()
*/

#ifdef SWIGPYTHON
%typemap(in) apr_array_header_t *location_revisions {
    $1 = (apr_array_header_t *) svn_swig_py_seq_to_array($input,
      sizeof(svn_revnum_t),
      svn_swig_py_unwrap_revnum,
      NULL,
      _global_pool);
    if (PyErr_Occurred())
      SWIG_fail;
}
#endif
#ifdef SWIGRUBY
%typemap(in) apr_array_header_t *location_revisions {
  $1 = svn_swig_rb_array_to_apr_array_revnum($input, _global_pool);
}
#endif

/* -----------------------------------------------------------------------
   define a general INPUT param of an array of const char * items.
 */

#ifdef SWIGPYTHON
%typemap(in) const apr_array_header_t *STRINGLIST {
    $1 = (apr_array_header_t *) svn_swig_py_seq_to_array($input,
      sizeof(const char *),
      svn_swig_py_unwrap_string,
      NULL,
      _global_pool);
    if (PyErr_Occurred())
      SWIG_fail;
}
#endif
#ifdef SWIGPERL
%typemap(in) const apr_array_header_t *STRINGLIST {
    $1 = (apr_array_header_t *) svn_swig_pl_strings_to_array($input,
                                                             _global_pool);
}
#endif
#ifdef SWIGRUBY
%typemap(in) const apr_array_header_t *STRINGLIST {
  $1 = svn_swig_rb_strings_to_apr_array($input, _global_pool);
}
%typemap(in) const apr_array_header_t *STRINGLIST_MAY_BE_NULL {
  $1 = (NIL_P($input)) ? NULL :
    svn_swig_rb_strings_to_apr_array($input, _global_pool);
}
#endif

/* svn_delta_path_driver() mutates its 'paths' argument (by sorting it),
   despite the fact that it is notionally an input parameter - hence, the
   lack of 'const' in that one case.

   svn_wc_get_update_editor3() and svn_wc_get_switch_editor3() aren't changing
   their 'preserved_exts' argument, but it is forwarded to
   svn_cstring_match_glob_list which also doesn't modify it, but does not have
   const in its prototype.  */
%apply const apr_array_header_t *STRINGLIST {
  const apr_array_header_t *args,
  const apr_array_header_t *diff_options,
  apr_array_header_t *paths,
  apr_array_header_t *revprops,
  const apr_array_header_t *targets,
  apr_array_header_t *preserved_exts
};

#ifdef SWIGRUBY
%apply const apr_array_header_t *STRINGLIST_MAY_BE_NULL {
  apr_array_header_t *revprops
};
#endif

/* -----------------------------------------------------------------------
   auth provider convertors
*/
#ifdef SWIGPERL
%typemap(in) apr_array_header_t *providers {
    $1 = (apr_array_header_t *) svn_swig_pl_objs_to_array($input,
      $descriptor(svn_auth_provider_object_t *), _global_pool);
}
#endif

#ifdef SWIGPYTHON
%typemap(in) apr_array_header_t *providers {
    $1 = (apr_array_header_t *) svn_swig_py_seq_to_array($input,
      sizeof(const svn_auth_provider_object_t *),
      svn_swig_py_unwrap_struct_ptr,
      $descriptor(svn_auth_provider_object_t *),
      _global_pool);
    if (PyErr_Occurred())
      SWIG_fail;
}
#endif

#ifdef SWIGRUBY
%typemap(in) apr_array_header_t *providers
{
  $1 = svn_swig_rb_array_to_auth_provider_object_apr_array($input, _global_pool);
}
#endif

/* -----------------------------------------------------------------------
   An array of svn_prop_t.
*/

#ifdef SWIGRUBY
%typemap(in) apr_array_header_t *ROW_PROP_LIST
{
  $1 = svn_swig_rb_to_apr_array_row_prop($input, _global_pool);
}

%apply apr_array_header_t *ROW_PROP_LIST {
  apr_array_header_t *proplist
};
#endif

/* -----------------------------------------------------------------------
   An array of svn_prop_t *.
*/

#ifdef SWIGRUBY
%typemap(in) apr_array_header_t *PROP_LIST
{
  VALUE rb_pool;
  apr_pool_t *pool;

  svn_swig_rb_get_pool(argc, argv, self, &rb_pool, &pool);

  $1 = svn_swig_rb_to_apr_array_prop($input, pool);
}

%typemap(out) apr_array_header_t *PROP_LIST
{
  %append_output(svn_swig_rb_prop_apr_array_to_hash_prop($1));
}

%typemap(in) apr_array_header_t *PROP_LIST_MAY_BE_NULL
{
  if (NIL_P($input)) {
    $1 = NULL;
  } else {
    VALUE rb_pool;
    apr_pool_t *pool;

    svn_swig_rb_get_pool(argc, argv, self, &rb_pool, &pool);
    $1 = svn_swig_rb_to_apr_array_prop($input, pool);
  }
}

%typemap(out) apr_array_header_t *PROP_LIST_MAY_BE_NULL
{
  %append_output($1 ? svn_swig_rb_prop_apr_array_to_hash_prop($1) : Qnil);
}

%apply apr_array_header_t *PROP_LIST {
  apr_array_header_t *wcprop_changes,
  apr_array_header_t *propchanges
};

%apply apr_array_header_t *PROP_LIST_MAY_BE_NULL {
  apr_array_header_t *incoming_prop_changes,
  apr_array_header_t *outgoing_prop_changes,
  apr_array_header_t *merge_options
};

#endif

/* -----------------------------------------------------------------------
   Input of apr_array_header_t * <svn_merge_range_t *>
*/
#ifdef SWIGPYTHON
%typemap(in) apr_array_header_t *RANGELIST {
    $1 = (apr_array_header_t *) svn_swig_py_seq_to_array($input,
      sizeof(const svn_merge_range_t *),
      svn_swig_py_unwrap_struct_ptr,
      $descriptor(svn_merge_range_t *),
      _global_pool);
    if (PyErr_Occurred()) {
      SWIG_fail;
    }
}
#endif

#ifdef SWIGRUBY
%typemap(in) apr_array_header_t *RANGELIST {
  $1 = svn_swig_rb_array_to_apr_array_merge_range($input, _global_pool);
}
#endif

/* -----------------------------------------------------------------------
   Input of apr_array_header_t * <svn_client_copy_source_t *>
*/
#ifdef SWIGRUBY
%typemap(in) apr_array_header_t *SOURCES {
  $1 = svn_swig_rb_array_to_apr_array_copy_source($input, _global_pool);
}
#endif

/* -----------------------------------------------------------------------
   Input of apr_array_header_t * <svn_opt_revision_range_t *>
*/
#ifdef SWIGPYTHON
%typemap(in) apr_array_header_t *REVISION_RANGE_LIST {
    $1 = (apr_array_header_t *) svn_swig_py_seq_to_array($input,
      sizeof(const svn_opt_revision_range_t *),
      svn_swig_py_unwrap_struct_ptr,
      $descriptor(svn_opt_revision_range_t *),
      _global_pool);
    if (PyErr_Occurred()) {
      SWIG_fail;
    }
}
#endif

#ifdef SWIGRUBY
%typemap(in) apr_array_header_t *REVISION_RANGE_LIST {
  $1 = svn_swig_rb_array_to_apr_array_revision_range($input, _global_pool);
}
#endif

/* -----------------------------------------------------------------------
   Input of
   apr_hash_t * <const char *, apr_array_header_t * <svn_merge_range_t *>>
*/
#ifdef SWIGPYTHON
%typemap(in) apr_hash_t *MERGEINFO {
  $1 = svn_swig_py_mergeinfo_from_dict($input, _global_pool);
  if (PyErr_Occurred()) {
    SWIG_fail;
  }
}
#endif

#ifdef SWIGRUBY
%typemap(in) apr_hash_t *MERGEINFO {
  $1 = svn_swig_rb_hash_to_apr_hash_merge_range($input, _global_pool);
}
#endif

/* -----------------------------------------------------------------------
   Output of
   apr_hash_t * <const char *, apr_array_header_t * <svn_merge_range_t *>>
*/
#ifdef SWIGPYTHON
%typemap(argout) apr_hash_t **MERGEINFO
{
  %append_output
    (svn_swig_py_mergeinfo_to_dict(*$1, $descriptor(svn_merge_range_t *),
                                    _global_py_pool));

  if (PyErr_Occurred()) {
    SWIG_fail;
  }
}

%typemap(in) apr_hash_t **MERGEINFO_INOUT ($*1_ltype temp)
{
  $1 = &temp;
  *$1 = svn_swig_py_mergeinfo_from_dict($input, _global_pool);
  if (PyErr_Occurred()) {
    SWIG_fail;
  }
}
#endif

#ifdef SWIGRUBY
%typemap(argout) apr_hash_t **MERGEINFO
{
  %append_output(svn_swig_rb_apr_hash_to_hash_merge_range(*$1));
}

%typemap(in) apr_hash_t **MERGEINFO_INOUT ($*1_ltype temp)
{
  $1 = &temp;
  *$1 = svn_swig_rb_hash_to_apr_hash_merge_range($input, _global_pool);
}
#endif

#if defined(SWIGPYTHON) || defined(SWIGRUBY)
%typemap(argout) apr_hash_t **MERGEINFO_INOUT = apr_hash_t **MERGEINFO;
#endif

/* -----------------------------------------------------------------------
   Output of
   apr_hash_t * <const char *,
                 apr_hash_t * <const char *,
                               array_header_t * <svn_merge_range_t *>>>
*/
#ifdef SWIGRUBY
%typemap(argout) apr_hash_t **MERGEINFO_CATALOG
{
  %append_output(svn_swig_rb_apr_hash_to_hash_merge_range_hash(*$1));
}
#endif
#ifdef SWIGPYTHON
%typemap(argout) apr_hash_t **MERGEINFO_CATALOG
{
  %append_output(svn_swig_py_mergeinfo_catalog_to_dict(*$1,
                                            $descriptor(svn_merge_range_t *),
                                            _global_py_pool));
  if (PyErr_Occurred()) {
    SWIG_fail;
  }
}
#endif

/* -----------------------------------------------------------------------
   Output of apr_array_header_t * <svn_merge_range_t *>
*/
#ifdef SWIGPYTHON
%typemap(argout) apr_array_header_t **RANGELIST {
  %append_output
    (svn_swig_py_pointerlist_to_list(*$1, $descriptor(svn_merge_range_t *),
                                     _global_py_pool));
  if (PyErr_Occurred()) {
    SWIG_fail;
  }
}

%typemap(in) apr_array_header_t **RANGELIST_INOUT ($*1_ltype temp)
{
  $1 = &temp;
  *$1 = (apr_array_header_t *) svn_swig_py_seq_to_array($input,
    sizeof(const svn_merge_range_t *),
    svn_swig_py_unwrap_struct_ptr,
    $descriptor(svn_merge_range_t *),
    _global_pool);
      
  if (PyErr_Occurred()) {
    SWIG_fail;
  }
}
#endif

#ifdef SWIGRUBY
%typemap(argout) apr_array_header_t **RANGELIST {
  %append_output(svn_swig_rb_apr_array_to_array_merge_range(*$1));
}

%typemap(in) apr_array_header_t **RANGELIST_INOUT ($*1_ltype temp)
{
  $1 = &temp;
  *$1 = svn_swig_rb_array_to_apr_array_merge_range($input, _global_pool);
}
#endif

#if defined(SWIGPYTHON) || defined(SWIGRUBY)
%typemap(argout) apr_array_header_t **RANGELIST_INOUT =
   apr_array_header_t **RANGELIST;
#endif

/* -----------------------------------------------------------------------
   Output of apr_array_header_t * <svn_auth_provider_object_t *>
*/
#ifdef SWIGPERL
%typemap(argout) apr_array_header_t **providers {
  %append_output(svn_swig_pl_convert_array(*$1,
                    $descriptor(svn_auth_provider_object_t *)));
}
#endif

#ifdef SWIGPYTHON
%typemap(argout) apr_array_header_t **providers {
  %append_output
    (svn_swig_py_pointerlist_to_list(*$1, $descriptor(svn_auth_provider_object_t *),
                                     _global_py_pool));
  if (PyErr_Occurred()) {
    SWIG_fail;
  }
}
#endif

#ifdef SWIGRUBY
%typemap(argout) apr_array_header_t **providers {
  %append_output(svn_swig_rb_apr_array_to_array_auth_provider_object(*$1));
}
#endif

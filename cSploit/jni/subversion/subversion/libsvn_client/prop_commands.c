/*
 * prop_commands.c:  Implementation of propset, propget, and proplist.
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

/* ==================================================================== */



/*** Includes. ***/

#define APR_WANT_STRFUNC
#include <apr_want.h>

#include "svn_error.h"
#include "svn_client.h"
#include "client.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_pools.h"
#include "svn_props.h"
#include "svn_hash.h"

#include "svn_private_config.h"
#include "private/svn_wc_private.h"
#include "private/svn_client_private.h"


/*** Code. ***/

/* Check whether NAME is a revision property name.
 *
 * Return TRUE if it is.
 * Return FALSE if it is not.
 */
static svn_boolean_t
is_revision_prop_name(const char *name)
{
  apr_size_t i;
  static const char *revision_props[] =
    {
      SVN_PROP_REVISION_ALL_PROPS
    };

  for (i = 0; i < sizeof(revision_props) / sizeof(revision_props[0]); i++)
    {
      if (strcmp(name, revision_props[i]) == 0)
        return TRUE;
    }
  return FALSE;
}


/* Return an SVN_ERR_CLIENT_PROPERTY_NAME error if NAME is a wcprop,
   else return SVN_NO_ERROR. */
static svn_error_t *
error_if_wcprop_name(const char *name)
{
  if (svn_property_kind(NULL, name) == svn_prop_wc_kind)
    {
      return svn_error_createf
        (SVN_ERR_CLIENT_PROPERTY_NAME, NULL,
         _("'%s' is a wcprop, thus not accessible to clients"),
         name);
    }

  return SVN_NO_ERROR;
}


struct getter_baton
{
  svn_ra_session_t *ra_session;
  svn_revnum_t base_revision_for_url;
};


static svn_error_t *
get_file_for_validation(const svn_string_t **mime_type,
                        svn_stream_t *stream,
                        void *baton,
                        apr_pool_t *pool)
{
  struct getter_baton *gb = baton;
  svn_ra_session_t *ra_session = gb->ra_session;
  apr_hash_t *props;

  SVN_ERR(svn_ra_get_file(ra_session, "", gb->base_revision_for_url,
                          stream, NULL,
                          (mime_type ? &props : NULL),
                          pool));

  if (mime_type)
    *mime_type = apr_hash_get(props, SVN_PROP_MIME_TYPE, APR_HASH_KEY_STRING);

  return SVN_NO_ERROR;
}


static
svn_error_t *
do_url_propset(const char *propname,
               const svn_string_t *propval,
               const svn_node_kind_t kind,
               const svn_revnum_t base_revision_for_url,
               const svn_delta_editor_t *editor,
               void *edit_baton,
               apr_pool_t *pool)
{
  void *root_baton;

  SVN_ERR(editor->open_root(edit_baton, base_revision_for_url, pool,
                            &root_baton));

  if (kind == svn_node_file)
    {
      void *file_baton;
      SVN_ERR(editor->open_file("", root_baton, base_revision_for_url,
                                pool, &file_baton));
      SVN_ERR(editor->change_file_prop(file_baton, propname, propval, pool));
      SVN_ERR(editor->close_file(file_baton, NULL, pool));
    }
  else
    {
      SVN_ERR(editor->change_dir_prop(root_baton, propname, propval, pool));
    }

  return editor->close_directory(root_baton, pool);
}

static svn_error_t *
propset_on_url(const char *propname,
               const svn_string_t *propval,
               const char *target,
               svn_boolean_t skip_checks,
               svn_revnum_t base_revision_for_url,
               const apr_hash_t *revprop_table,
               svn_commit_callback2_t commit_callback,
               void *commit_baton,
               svn_client_ctx_t *ctx,
               apr_pool_t *pool)
{
  enum svn_prop_kind prop_kind = svn_property_kind(NULL, propname);
  svn_ra_session_t *ra_session;
  svn_node_kind_t node_kind;
  const char *message;
  const svn_delta_editor_t *editor;
  void *edit_baton;
  apr_hash_t *commit_revprops;
  svn_error_t *err;

  if (prop_kind != svn_prop_regular_kind)
    return svn_error_createf
      (SVN_ERR_BAD_PROP_KIND, NULL,
       _("Property '%s' is not a regular property"), propname);

  /* Open an RA session for the URL. Note that we don't have a local
     directory, nor a place to put temp files. */
  SVN_ERR(svn_client__open_ra_session_internal(&ra_session, NULL, target,
                                               NULL, NULL, FALSE, TRUE,
                                               ctx, pool));

  SVN_ERR(svn_ra_check_path(ra_session, "", base_revision_for_url,
                            &node_kind, pool));
  if (node_kind == svn_node_none)
    return svn_error_createf
      (SVN_ERR_FS_NOT_FOUND, NULL,
       _("Path '%s' does not exist in revision %ld"),
       target, base_revision_for_url);

  /* Setting an inappropriate property is not allowed (unless
     overridden by 'skip_checks', in some circumstances).  Deleting an
     inappropriate property is allowed, however, since older clients
     allowed (and other clients possibly still allow) setting it in
     the first place. */
  if (propval && svn_prop_is_svn_prop(propname))
    {
      const svn_string_t *new_value;
      struct getter_baton gb;

      gb.ra_session = ra_session;
      gb.base_revision_for_url = base_revision_for_url;
      SVN_ERR(svn_wc_canonicalize_svn_prop(&new_value, propname, propval,
                                           target, node_kind, skip_checks,
                                           get_file_for_validation, &gb, pool));
      propval = new_value;
    }

  /* Create a new commit item and add it to the array. */
  if (SVN_CLIENT__HAS_LOG_MSG_FUNC(ctx))
    {
      svn_client_commit_item3_t *item;
      const char *tmp_file;
      apr_array_header_t *commit_items = apr_array_make(pool, 1, sizeof(item));

      item = svn_client_commit_item3_create(pool);
      item->url = target;
      item->state_flags = SVN_CLIENT_COMMIT_ITEM_PROP_MODS;
      APR_ARRAY_PUSH(commit_items, svn_client_commit_item3_t *) = item;
      SVN_ERR(svn_client__get_log_msg(&message, &tmp_file, commit_items,
                                      ctx, pool));
      if (! message)
        return SVN_NO_ERROR;
    }
  else
    message = "";

  SVN_ERR(svn_client__ensure_revprop_table(&commit_revprops, revprop_table,
                                           message, ctx, pool));

  /* Fetch RA commit editor. */
  SVN_ERR(svn_ra_get_commit_editor3(ra_session, &editor, &edit_baton,
                                    commit_revprops,
                                    commit_callback,
                                    commit_baton,
                                    NULL, TRUE, /* No lock tokens */
                                    pool));

  err = do_url_propset(propname, propval, node_kind, base_revision_for_url,
                       editor, edit_baton, pool);

  if (err)
    {
      /* At least try to abort the edit (and fs txn) before throwing err. */
      svn_error_clear(editor->abort_edit(edit_baton, pool));
      return svn_error_trace(err);
    }

  /* Close the edit. */
  return editor->close_edit(edit_baton, pool);
}

/* Baton for set_props_cb */
struct set_props_baton
{
  svn_client_ctx_t *ctx;
  const char *local_abspath;
  svn_depth_t depth;
  svn_node_kind_t kind;
  const char *propname;
  const svn_string_t *propval;
  svn_boolean_t skip_checks;
  const apr_array_header_t *changelist_filter;
};

/* Working copy lock callback for svn_client_propset4 */
static svn_error_t *
set_props_cb(void *baton,
             apr_pool_t *result_pool,
             apr_pool_t *scratch_pool)
{
  struct set_props_baton *bt = baton;

  SVN_ERR(svn_wc_prop_set4(bt->ctx->wc_ctx, bt->local_abspath, bt->propname,
                           bt->propval, bt->depth, bt->skip_checks,
                           bt->changelist_filter,
                           bt->ctx->cancel_func, bt->ctx->cancel_baton,
                           bt->ctx->notify_func2, bt->ctx->notify_baton2,
                           scratch_pool));

  return SVN_NO_ERROR;
}

/* Check that PROPNAME is a valid name for a versioned property.  Return an
 * error if it is not valid, specifically if it is:
 *   - the name of a standard Subversion rev-prop; or
 *   - in the namespace of WC-props; or
 *   - not a well-formed property name (except if PROPVAL is NULL: in other
 *     words we do allow deleting a prop with an ill-formed name).
 *
 * Since Subversion controls the "svn:" property namespace, we don't honor
 * a 'skip_checks' flag here.  Checks for unusual property combinations such
 * as svn:eol-style with a non-text svn:mime-type might understandably be
 * skipped, but things such as using a property name reserved for revprops
 * on a local target are never allowed.
 */
static svn_error_t *
check_prop_name(const char *propname,
                const svn_string_t *propval)
{
  if (is_revision_prop_name(propname))
    return svn_error_createf(SVN_ERR_CLIENT_PROPERTY_NAME, NULL,
                             _("Revision property '%s' not allowed "
                               "in this context"), propname);

  SVN_ERR(error_if_wcprop_name(propname));

  if (propval && ! svn_prop_name_is_valid(propname))
    return svn_error_createf(SVN_ERR_CLIENT_PROPERTY_NAME, NULL,
                             _("Bad property name: '%s'"), propname);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_client_propset_local(const char *propname,
                         const svn_string_t *propval,
                         const apr_array_header_t *targets,
                         svn_depth_t depth,
                         svn_boolean_t skip_checks,
                         const apr_array_header_t *changelists,
                         svn_client_ctx_t *ctx,
                         apr_pool_t *scratch_pool)
{
  apr_pool_t *iterpool = svn_pool_create(scratch_pool);
  svn_boolean_t targets_are_urls;
  int i;

  if (targets->nelts == 0)
    return SVN_NO_ERROR;

  /* Check for homogeneity among our targets. */
  targets_are_urls = svn_path_is_url(APR_ARRAY_IDX(targets, 0, const char *));
  SVN_ERR(svn_client__assert_homogeneous_target_type(targets));

  if (targets_are_urls)
    return svn_error_create(SVN_ERR_ILLEGAL_TARGET, NULL,
                            _("Targets must be working copy paths"));

  SVN_ERR(check_prop_name(propname, propval));

  for (i = 0; i < targets->nelts; i++)
    {
      svn_node_kind_t kind;
      const char *target_abspath;
      svn_error_t *err;
      struct set_props_baton baton;
      const char *target = APR_ARRAY_IDX(targets, i, const char *);

      svn_pool_clear(iterpool);

      /* Check for cancellation */
      if (ctx->cancel_func)
        SVN_ERR(ctx->cancel_func(ctx->cancel_baton));

      SVN_ERR(svn_dirent_get_absolute(&target_abspath, target, iterpool));

      err = svn_wc_read_kind(&kind, ctx->wc_ctx, target_abspath, FALSE,
                             iterpool);

      if ((err && err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
          || (!err && (kind == svn_node_unknown || kind == svn_node_none)))
        {
          if (ctx->notify_func2)
            {
              svn_wc_notify_t *notify = svn_wc_create_notify(
                                          target_abspath,
                                          svn_wc_notify_path_nonexistent,
                                          iterpool);

              ctx->notify_func2(ctx->notify_baton2, notify, iterpool);
            }

          svn_error_clear(err);
        }
      else
        SVN_ERR(err);

      baton.ctx = ctx;
      baton.local_abspath = target_abspath;
      baton.depth = depth;
      baton.kind = kind;
      baton.propname = propname;
      baton.propval = propval;
      baton.skip_checks = skip_checks;
      baton.changelist_filter = changelists;

      SVN_ERR(svn_wc__call_with_write_lock(set_props_cb, &baton,
                                           ctx->wc_ctx, target_abspath,
                                           FALSE, iterpool, iterpool));
    }
  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_client_propset_remote(const char *propname,
                          const svn_string_t *propval,
                          const char *url,
                          svn_boolean_t skip_checks,
                          svn_revnum_t base_revision_for_url,
                          const apr_hash_t *revprop_table,
                          svn_commit_callback2_t commit_callback,
                          void *commit_baton,
                          svn_client_ctx_t *ctx,
                          apr_pool_t *scratch_pool)
{
  if (!svn_path_is_url(url))
    return svn_error_create(SVN_ERR_ILLEGAL_TARGET, NULL,
                            _("Targets must be URLs"));

  SVN_ERR(check_prop_name(propname, propval));

  /* The rationale for requiring the base_revision_for_url
     argument is that without it, it's too easy to possibly
     overwrite someone else's change without noticing.  (See also
     tools/examples/svnput.c). */
  if (! SVN_IS_VALID_REVNUM(base_revision_for_url))
    return svn_error_create(SVN_ERR_CLIENT_BAD_REVISION, NULL,
                            _("Setting property on non-local targets "
                              "needs a base revision"));

  /* ### When you set svn:eol-style or svn:keywords on a wc file,
     ### Subversion sends a textdelta at commit time to properly
     ### normalize the file in the repository.  If we want to
     ### support editing these properties on URLs, then we should
     ### generate the same textdelta; for now, we won't support
     ### editing these properties on URLs.  (Admittedly, this
     ### means that all the machinery with get_file_for_validation
     ### is unused.)
   */
  if ((strcmp(propname, SVN_PROP_EOL_STYLE) == 0) ||
      (strcmp(propname, SVN_PROP_KEYWORDS) == 0))
    return svn_error_createf(SVN_ERR_UNSUPPORTED_FEATURE, NULL,
                             _("Setting property '%s' on non-local "
                               "targets is not supported"), propname);

  SVN_ERR(propset_on_url(propname, propval, url, skip_checks,
                         base_revision_for_url, revprop_table,
                         commit_callback, commit_baton, ctx, scratch_pool));

  return SVN_NO_ERROR;
}

static svn_error_t *
check_and_set_revprop(svn_revnum_t *set_rev,
                      svn_ra_session_t *ra_session,
                      const char *propname,
                      const svn_string_t *original_propval,
                      const svn_string_t *propval,
                      apr_pool_t *pool)
{
  if (original_propval)
    {
      /* Ensure old value hasn't changed behind our back. */
      svn_string_t *current;
      SVN_ERR(svn_ra_rev_prop(ra_session, *set_rev, propname, &current, pool));

      if (original_propval->data && (! current))
        {
          return svn_error_createf(
                  SVN_ERR_RA_OUT_OF_DATE, NULL,
                  _("revprop '%s' in r%ld is unexpectedly absent "
                    "in repository (maybe someone else deleted it?)"),
                  propname, *set_rev);
        }
      else if (original_propval->data
               && (! svn_string_compare(original_propval, current)))
        {
          return svn_error_createf(
                  SVN_ERR_RA_OUT_OF_DATE, NULL,
                  _("revprop '%s' in r%ld has unexpected value "
                    "in repository (maybe someone else changed it?)"),
                  propname, *set_rev);
        }
      else if ((! original_propval->data) && current)
        {
          return svn_error_createf(
                  SVN_ERR_RA_OUT_OF_DATE, NULL,
                  _("revprop '%s' in r%ld is unexpectedly present "
                    "in repository (maybe someone else set it?)"),
                  propname, *set_rev);
        }
    }

  SVN_ERR(svn_ra_change_rev_prop2(ra_session, *set_rev, propname,
                                  NULL, propval, pool));

  return SVN_NO_ERROR;
}

svn_error_t *
svn_client_revprop_set2(const char *propname,
                        const svn_string_t *propval,
                        const svn_string_t *original_propval,
                        const char *URL,
                        const svn_opt_revision_t *revision,
                        svn_revnum_t *set_rev,
                        svn_boolean_t force,
                        svn_client_ctx_t *ctx,
                        apr_pool_t *pool)
{
  svn_ra_session_t *ra_session;
  svn_boolean_t be_atomic;

  if ((strcmp(propname, SVN_PROP_REVISION_AUTHOR) == 0)
      && propval
      && strchr(propval->data, '\n') != NULL
      && (! force))
    return svn_error_create(SVN_ERR_CLIENT_REVISION_AUTHOR_CONTAINS_NEWLINE,
                            NULL, _("Author name should not contain a newline;"
                                    " value will not be set unless forced"));

  if (propval && ! svn_prop_name_is_valid(propname))
    return svn_error_createf(SVN_ERR_CLIENT_PROPERTY_NAME, NULL,
                             _("Bad property name: '%s'"), propname);

  /* Open an RA session for the URL. Note that we don't have a local
     directory, nor a place to put temp files. */
  SVN_ERR(svn_client__open_ra_session_internal(&ra_session, NULL, URL, NULL,
                                               NULL, FALSE, TRUE, ctx, pool));

  /* Resolve the revision into something real, and return that to the
     caller as well. */
  SVN_ERR(svn_client__get_revision_number(set_rev, NULL, ctx->wc_ctx, NULL,
                                          ra_session, revision, pool));

  SVN_ERR(svn_ra_has_capability(ra_session, &be_atomic,
                                SVN_RA_CAPABILITY_ATOMIC_REVPROPS, pool));
  if (be_atomic)
    {
      /* Convert ORIGINAL_PROPVAL to an OLD_VALUE_P. */
      const svn_string_t *const *old_value_p;
      const svn_string_t *unset = NULL;

      if (original_propval == NULL)
      	old_value_p = NULL;
      else if (original_propval->data == NULL)
      	old_value_p = &unset;
      else
      	old_value_p = &original_propval;

      /* The actual RA call. */
      SVN_ERR(svn_ra_change_rev_prop2(ra_session, *set_rev, propname,
                                      old_value_p, propval, pool));
    }
  else
    {
      /* The actual RA call. */
      SVN_ERR(check_and_set_revprop(set_rev, ra_session, propname,
                                    original_propval, propval, pool));
    }

  if (ctx->notify_func2)
    {
      svn_wc_notify_t *notify = svn_wc_create_notify_url(URL,
                                             propval == NULL
                                               ? svn_wc_notify_revprop_deleted
                                               : svn_wc_notify_revprop_set,
                                             pool);
      notify->prop_name = propname;
      notify->revision = *set_rev;

      (*ctx->notify_func2)(ctx->notify_baton2, notify, pool);
    }

  return SVN_NO_ERROR;
}


/* Set *PROPS to the pristine (base) properties at LOCAL_ABSPATH, if PRISTINE
 * is true, or else the working value if PRISTINE is false.
 *
 * The keys of *PROPS will be 'const char *' property names, and the
 * values 'const svn_string_t *' property values.  Allocate *PROPS
 * and its contents in RESULT_POOL.  Use SCRATCH_POOL for temporary
 * allocations.
 */
static svn_error_t *
pristine_or_working_props(apr_hash_t **props,
                          svn_wc_context_t *wc_ctx,
                          const char *local_abspath,
                          svn_boolean_t pristine,
                          apr_pool_t *result_pool,
                          apr_pool_t *scratch_pool)
{
  if (pristine)
    {
      return svn_error_trace(svn_wc_get_pristine_props(props,
                                                       wc_ctx,
                                                       local_abspath,
                                                       result_pool,
                                                       scratch_pool));
    }

  /* ### until svn_wc_prop_list2() returns a NULL value for locally-deleted
     ### nodes, then let's check manually.  */
  {
    svn_boolean_t deleted;

    SVN_ERR(svn_wc__node_is_status_deleted(&deleted, wc_ctx, local_abspath,
                                           scratch_pool));
    if (deleted)
      {
        *props = NULL;
        return SVN_NO_ERROR;
      }
  }

  return svn_error_trace(svn_wc_prop_list2(props, wc_ctx, local_abspath,
                                           result_pool, scratch_pool));
}


/* Helper for the remote case of svn_client_propget.
 *
 * Get the value of property PROPNAME in REVNUM, using RA_LIB and
 * SESSION.  Store the value ('svn_string_t *') in PROPS, under the
 * path key "TARGET_PREFIX/TARGET_RELATIVE" ('const char *').
 *
 * Recurse according to DEPTH, similarly to svn_client_propget3().
 *
 * KIND is the kind of the node at "TARGET_PREFIX/TARGET_RELATIVE".
 * Yes, caller passes this; it makes the recursion more efficient :-).
 *
 * Allocate the keys and values in PERM_POOL, but do all temporary
 * work in WORK_POOL.  The two pools can be the same; recursive
 * calls may use a different WORK_POOL, however.
 */
static svn_error_t *
remote_propget(apr_hash_t *props,
               const char *propname,
               const char *target_prefix,
               const char *target_relative,
               svn_node_kind_t kind,
               svn_revnum_t revnum,
               svn_ra_session_t *ra_session,
               svn_depth_t depth,
               apr_pool_t *perm_pool,
               apr_pool_t *work_pool)
{
  apr_hash_t *dirents;
  apr_hash_t *prop_hash;
  const svn_string_t *val;
  const char *target_full_url =
    svn_path_url_add_component2(target_prefix, target_relative, work_pool);

  if (kind == svn_node_dir)
    {
      SVN_ERR(svn_ra_get_dir2(ra_session,
                              (depth >= svn_depth_files ? &dirents : NULL),
                              NULL, &prop_hash, target_relative, revnum,
                              SVN_DIRENT_KIND, work_pool));
    }
  else if (kind == svn_node_file)
    {
      SVN_ERR(svn_ra_get_file(ra_session, target_relative, revnum,
                              NULL, NULL, &prop_hash, work_pool));
    }
  else if (kind == svn_node_none)
    {
      return svn_error_createf(SVN_ERR_ENTRY_NOT_FOUND, NULL,
                               _("'%s' does not exist in revision %ld"),
                               target_full_url, revnum);
    }
  else
    {
      return svn_error_createf(SVN_ERR_NODE_UNKNOWN_KIND, NULL,
                               _("Unknown node kind for '%s'"),
                               target_full_url);
    }

  if ((val = apr_hash_get(prop_hash, propname, APR_HASH_KEY_STRING)))
    {
      apr_hash_set(props, apr_pstrdup(perm_pool, target_full_url),
                   APR_HASH_KEY_STRING, svn_string_dup(val, perm_pool));
    }

  if (depth >= svn_depth_files
      && kind == svn_node_dir
      && apr_hash_count(dirents) > 0)
    {
      apr_hash_index_t *hi;
      apr_pool_t *iterpool = svn_pool_create(work_pool);

      for (hi = apr_hash_first(work_pool, dirents);
           hi;
           hi = apr_hash_next(hi))
        {
          const char *this_name = svn__apr_hash_index_key(hi);
          svn_dirent_t *this_ent = svn__apr_hash_index_val(hi);
          const char *new_target_relative;
          svn_depth_t depth_below_here = depth;

          svn_pool_clear(iterpool);

          if (depth == svn_depth_files && this_ent->kind == svn_node_dir)
            continue;

          if (depth == svn_depth_files || depth == svn_depth_immediates)
            depth_below_here = svn_depth_empty;

          new_target_relative = svn_relpath_join(target_relative, this_name,
                                                 iterpool);

          SVN_ERR(remote_propget(props,
                                 propname,
                                 target_prefix,
                                 new_target_relative,
                                 this_ent->kind,
                                 revnum,
                                 ra_session,
                                 depth_below_here,
                                 perm_pool, iterpool));
        }

      svn_pool_destroy(iterpool);
    }

  return SVN_NO_ERROR;
}

/* Baton for recursive_propget_receiver(). */
struct recursive_propget_receiver_baton
{
  apr_hash_t *props; /* Hash to collect props. */
  apr_pool_t *pool; /* Pool to allocate additions to PROPS. */
  svn_wc_context_t *wc_ctx;  /* Working copy context. */
};

/* An implementation of svn_wc__proplist_receiver_t. */
static svn_error_t *
recursive_propget_receiver(void *baton,
                           const char *local_abspath,
                           apr_hash_t *props,
                           apr_pool_t *scratch_pool)
{
  struct recursive_propget_receiver_baton *b = baton;

  if (apr_hash_count(props))
    {
      apr_hash_index_t *hi = apr_hash_first(scratch_pool, props);
      apr_hash_set(b->props, apr_pstrdup(b->pool, local_abspath),
                   APR_HASH_KEY_STRING,
                   svn_string_dup(svn__apr_hash_index_val(hi), b->pool));
    }

  return SVN_NO_ERROR;
}

/* Return the property value for any PROPNAME set on TARGET in *PROPS,
   with WC paths of char * for keys and property values of
   svn_string_t * for values.  Assumes that PROPS is non-NULL.  Additions
   to *PROPS are allocated in RESULT_POOL, temporary allocations happen in
   SCRATCH_POOL.

   CHANGELISTS is an array of const char * changelist names, used as a
   restrictive filter on items whose properties are set; that is,
   don't set properties on any item unless it's a member of one of
   those changelists.  If CHANGELISTS is empty (or altogether NULL),
   no changelist filtering occurs.

   Treat DEPTH as in svn_client_propget3().
*/
static svn_error_t *
get_prop_from_wc(apr_hash_t *props,
                 const char *propname,
                 const char *target_abspath,
                 svn_boolean_t pristine,
                 svn_node_kind_t kind,
                 svn_depth_t depth,
                 const apr_array_header_t *changelists,
                 svn_client_ctx_t *ctx,
                 apr_pool_t *result_pool,
                 apr_pool_t *scratch_pool)
{
  struct recursive_propget_receiver_baton rb;

  /* Technically, svn_depth_unknown just means use whatever depth(s)
     we find in the working copy.  But this is a walk over extant
     working copy paths: if they're there at all, then by definition
     the local depth reaches them, so let's just use svn_depth_infinity
     to get there. */
  if (depth == svn_depth_unknown)
    depth = svn_depth_infinity;

  rb.props = props;
  rb.pool = result_pool;
  rb.wc_ctx = ctx->wc_ctx;

  SVN_ERR(svn_wc__prop_list_recursive(ctx->wc_ctx, target_abspath,
                                      propname, depth, FALSE, pristine,
                                      changelists,
                                      recursive_propget_receiver, &rb,
                                      ctx->cancel_func, ctx->cancel_baton,
                                      scratch_pool));

  return SVN_NO_ERROR;
}

/* Note: this implementation is very similar to svn_client_proplist. */
svn_error_t *
svn_client_propget4(apr_hash_t **props,
                    const char *propname,
                    const char *target,
                    const svn_opt_revision_t *peg_revision,
                    const svn_opt_revision_t *revision,
                    svn_revnum_t *actual_revnum,
                    svn_depth_t depth,
                    const apr_array_header_t *changelists,
                    svn_client_ctx_t *ctx,
                    apr_pool_t *result_pool,
                    apr_pool_t *scratch_pool)
{
  svn_revnum_t revnum;

  SVN_ERR(error_if_wcprop_name(propname));
  if (!svn_path_is_url(target))
    SVN_ERR_ASSERT(svn_dirent_is_absolute(target));

  peg_revision = svn_cl__rev_default_to_head_or_working(peg_revision,
                                                        target);
  revision = svn_cl__rev_default_to_peg(revision, peg_revision);

  *props = apr_hash_make(result_pool);

  if (! svn_path_is_url(target)
      && SVN_CLIENT__REVKIND_IS_LOCAL_TO_WC(peg_revision->kind)
      && SVN_CLIENT__REVKIND_IS_LOCAL_TO_WC(revision->kind))
    {
      svn_node_kind_t kind;
      svn_boolean_t pristine;
      svn_error_t *err;

      /* If FALSE, we want the working revision. */
      pristine = (revision->kind == svn_opt_revision_committed
                  || revision->kind == svn_opt_revision_base);

      SVN_ERR(svn_wc_read_kind(&kind, ctx->wc_ctx, target, FALSE,
                               scratch_pool));

      if (kind == svn_node_unknown || kind == svn_node_none)
        {
          /* svn uses SVN_ERR_UNVERSIONED_RESOURCE as warning only
             for this function. */
          return svn_error_createf(SVN_ERR_UNVERSIONED_RESOURCE, NULL,
                                   _("'%s' is not under version control"),
                                   svn_dirent_local_style(target,
                                                          scratch_pool));
        }

      err = svn_client__get_revision_number(&revnum, NULL, ctx->wc_ctx,
                                            target, NULL, revision,
                                            scratch_pool);
      if (err && err->apr_err == SVN_ERR_CLIENT_BAD_REVISION)
        {
          svn_error_clear(err);
          revnum = SVN_INVALID_REVNUM;
        }
      else if (err)
        return svn_error_trace(err);

      SVN_ERR(get_prop_from_wc(*props, propname, target,
                               pristine, kind,
                               depth, changelists, ctx, scratch_pool,
                               result_pool));
    }
  else
    {
      const char *url;
      svn_ra_session_t *ra_session;
      svn_node_kind_t kind;

      /* Get an RA plugin for this filesystem object. */
      SVN_ERR(svn_client__ra_session_from_path(&ra_session, &revnum,
                                               &url, target, NULL,
                                               peg_revision,
                                               revision, ctx, scratch_pool));

      SVN_ERR(svn_ra_check_path(ra_session, "", revnum, &kind, scratch_pool));

      SVN_ERR(remote_propget(*props, propname, url, "",
                             kind, revnum, ra_session,
                             depth, result_pool, scratch_pool));
    }

  if (actual_revnum)
    *actual_revnum = revnum;
  return SVN_NO_ERROR;
}

svn_error_t *
svn_client_revprop_get(const char *propname,
                       svn_string_t **propval,
                       const char *URL,
                       const svn_opt_revision_t *revision,
                       svn_revnum_t *set_rev,
                       svn_client_ctx_t *ctx,
                       apr_pool_t *pool)
{
  svn_ra_session_t *ra_session;

  /* Open an RA session for the URL. Note that we don't have a local
     directory, nor a place to put temp files. */
  SVN_ERR(svn_client__open_ra_session_internal(&ra_session, NULL, URL, NULL,
                                               NULL, FALSE, TRUE, ctx, pool));

  /* Resolve the revision into something real, and return that to the
     caller as well. */
  SVN_ERR(svn_client__get_revision_number(set_rev, NULL, ctx->wc_ctx, NULL,
                                          ra_session, revision, pool));

  /* The actual RA call. */
  return svn_ra_rev_prop(ra_session, *set_rev, propname, propval, pool);
}


/* Call RECEIVER for the given PATH and PROP_HASH.
 *
 * If PROP_HASH is null or has zero count, do nothing.
 */
static svn_error_t*
call_receiver(const char *path,
              apr_hash_t *prop_hash,
              svn_proplist_receiver_t receiver,
              void *receiver_baton,
              apr_pool_t *pool)
{
  if (prop_hash && apr_hash_count(prop_hash))
    SVN_ERR(receiver(receiver_baton, path, prop_hash, pool));

  return SVN_NO_ERROR;
}


/* Helper for the remote case of svn_client_proplist.
 *
 * Push a new 'svn_client_proplist_item_t *' item onto PROPLIST,
 * containing the properties for "TARGET_PREFIX/TARGET_RELATIVE" in
 * REVNUM, obtained using RA_LIB and SESSION.  The item->node_name
 * will be "TARGET_PREFIX/TARGET_RELATIVE", and the value will be a
 * hash mapping 'const char *' property names onto 'svn_string_t *'
 * property values.
 *
 * Allocate the new item and its contents in POOL.
 * Do all looping, recursion, and temporary work in SCRATCHPOOL.
 *
 * KIND is the kind of the node at "TARGET_PREFIX/TARGET_RELATIVE".
 *
 * If the target is a directory, only fetch properties for the files
 * and directories at depth DEPTH.
 */
static svn_error_t *
remote_proplist(const char *target_prefix,
                const char *target_relative,
                svn_node_kind_t kind,
                svn_revnum_t revnum,
                svn_ra_session_t *ra_session,
                svn_depth_t depth,
                svn_proplist_receiver_t receiver,
                void *receiver_baton,
                apr_pool_t *pool,
                apr_pool_t *scratchpool)
{
  apr_hash_t *dirents;
  apr_hash_t *prop_hash, *final_hash;
  apr_hash_index_t *hi;
  const char *target_full_url =
    svn_path_url_add_component2(target_prefix, target_relative, scratchpool);

  if (kind == svn_node_dir)
    {
      SVN_ERR(svn_ra_get_dir2(ra_session,
                              (depth > svn_depth_empty) ? &dirents : NULL,
                              NULL, &prop_hash, target_relative, revnum,
                              SVN_DIRENT_KIND, scratchpool));
    }
  else if (kind == svn_node_file)
    {
      SVN_ERR(svn_ra_get_file(ra_session, target_relative, revnum,
                              NULL, NULL, &prop_hash, scratchpool));
    }
  else
    {
      return svn_error_createf(SVN_ERR_NODE_UNKNOWN_KIND, NULL,
                               _("Unknown node kind for '%s'"),
                               target_full_url);
    }

  /* Filter out non-regular properties, since the RA layer returns all
     kinds.  Copy regular properties keys/vals from the prop_hash
     allocated in SCRATCHPOOL to the "final" hash allocated in POOL. */
  final_hash = apr_hash_make(pool);
  for (hi = apr_hash_first(scratchpool, prop_hash);
       hi;
       hi = apr_hash_next(hi))
    {
      const char *name = svn__apr_hash_index_key(hi);
      apr_ssize_t klen = svn__apr_hash_index_klen(hi);
      svn_string_t *value = svn__apr_hash_index_val(hi);
      svn_prop_kind_t prop_kind;

      prop_kind = svn_property_kind(NULL, name);

      if (prop_kind == svn_prop_regular_kind)
        {
          name = apr_pstrdup(pool, name);
          value = svn_string_dup(value, pool);
          apr_hash_set(final_hash, name, klen, value);
        }
    }

  SVN_ERR(call_receiver(target_full_url, final_hash, receiver, receiver_baton,
                        pool));

  if (depth > svn_depth_empty
      && (kind == svn_node_dir) && (apr_hash_count(dirents) > 0))
    {
      apr_pool_t *subpool = svn_pool_create(scratchpool);

      for (hi = apr_hash_first(scratchpool, dirents);
           hi;
           hi = apr_hash_next(hi))
        {
          const char *this_name = svn__apr_hash_index_key(hi);
          svn_dirent_t *this_ent = svn__apr_hash_index_val(hi);
          const char *new_target_relative;

          svn_pool_clear(subpool);

          new_target_relative = svn_relpath_join(target_relative,
                                                 this_name, subpool);

          if (this_ent->kind == svn_node_file
              || depth > svn_depth_files)
            {
              svn_depth_t depth_below_here = depth;

              if (depth == svn_depth_immediates)
                depth_below_here = svn_depth_empty;

              SVN_ERR(remote_proplist(target_prefix,
                                      new_target_relative,
                                      this_ent->kind,
                                      revnum,
                                      ra_session,
                                      depth_below_here,
                                      receiver,
                                      receiver_baton,
                                      pool,
                                      subpool));
            }
        }

      svn_pool_destroy(subpool);
    }

  return SVN_NO_ERROR;
}


/* Baton for recursive_proplist_receiver(). */
struct recursive_proplist_receiver_baton
{
  svn_wc_context_t *wc_ctx;  /* Working copy context. */
  svn_proplist_receiver_t wrapped_receiver;  /* Proplist receiver to call. */
  void *wrapped_receiver_baton;    /* Baton for the proplist receiver. */

  /* Anchor, anchor_abspath pair for converting to relative paths */
  const char *anchor;
  const char *anchor_abspath;
};

/* An implementation of svn_wc__proplist_receiver_t. */
static svn_error_t *
recursive_proplist_receiver(void *baton,
                            const char *local_abspath,
                            apr_hash_t *props,
                            apr_pool_t *scratch_pool)
{
  struct recursive_proplist_receiver_baton *b = baton;
  const char *path;

  /* Attempt to convert absolute paths to relative paths for
   * presentation purposes, if needed. */
  if (b->anchor && b->anchor_abspath)
    {
      path = svn_dirent_join(b->anchor,
                             svn_dirent_skip_ancestor(b->anchor_abspath,
                                                      local_abspath),
                             scratch_pool);
    }
  else
    path = local_abspath;

  return svn_error_trace(b->wrapped_receiver(b->wrapped_receiver_baton,
                                             path, props, scratch_pool));
}

svn_error_t *
svn_client_proplist3(const char *path_or_url,
                     const svn_opt_revision_t *peg_revision,
                     const svn_opt_revision_t *revision,
                     svn_depth_t depth,
                     const apr_array_header_t *changelists,
                     svn_proplist_receiver_t receiver,
                     void *receiver_baton,
                     svn_client_ctx_t *ctx,
                     apr_pool_t *pool)
{
  const char *url;

  peg_revision = svn_cl__rev_default_to_head_or_working(peg_revision,
                                                        path_or_url);
  revision = svn_cl__rev_default_to_peg(revision, peg_revision);

  if (depth == svn_depth_unknown)
    depth = svn_depth_empty;

  if (! svn_path_is_url(path_or_url)
      && SVN_CLIENT__REVKIND_IS_LOCAL_TO_WC(peg_revision->kind)
      && SVN_CLIENT__REVKIND_IS_LOCAL_TO_WC(revision->kind))
    {
      svn_boolean_t pristine;
      svn_node_kind_t kind;
      apr_hash_t *changelist_hash = NULL;
      const char *local_abspath;

      SVN_ERR(svn_dirent_get_absolute(&local_abspath, path_or_url, pool));

      pristine = ((revision->kind == svn_opt_revision_committed)
                  || (revision->kind == svn_opt_revision_base));

      SVN_ERR(svn_wc_read_kind(&kind, ctx->wc_ctx, local_abspath, FALSE,
                               pool));

      if (kind == svn_node_unknown || kind == svn_node_none)
        {
          /* svn uses SVN_ERR_UNVERSIONED_RESOURCE as warning only
             for this function. */
          return svn_error_createf(SVN_ERR_UNVERSIONED_RESOURCE, NULL,
                                   _("'%s' is not under version control"),
                                   svn_dirent_local_style(local_abspath,
                                                          pool));
        }

      if (changelists && changelists->nelts)
        SVN_ERR(svn_hash_from_cstring_keys(&changelist_hash,
                                           changelists, pool));

      /* Fetch, recursively or not. */
      if (kind == svn_node_dir)
        {
          struct recursive_proplist_receiver_baton rb;

          rb.wc_ctx = ctx->wc_ctx;
          rb.wrapped_receiver = receiver;
          rb.wrapped_receiver_baton = receiver_baton;

          if (strcmp(path_or_url, local_abspath) != 0)
            {
              rb.anchor = path_or_url;
              rb.anchor_abspath = local_abspath;
            }
          else
            {
              rb.anchor = NULL;
              rb.anchor_abspath = NULL;
            }

          SVN_ERR(svn_wc__prop_list_recursive(ctx->wc_ctx, local_abspath, NULL,
                                              depth,
                                              FALSE, pristine, changelists,
                                              recursive_proplist_receiver, &rb,
                                              ctx->cancel_func,
                                              ctx->cancel_baton, pool));
        }
      else if (svn_wc__changelist_match(ctx->wc_ctx, local_abspath,
                                        changelist_hash, pool))
        {
          apr_hash_t *hash;

          SVN_ERR(pristine_or_working_props(&hash, ctx->wc_ctx, local_abspath,
                                            pristine, pool, pool));
          SVN_ERR(call_receiver(path_or_url, hash,
                                receiver, receiver_baton, pool));

        }
    }
  else /* remote target */
    {
      svn_ra_session_t *ra_session;
      svn_node_kind_t kind;
      apr_pool_t *subpool = svn_pool_create(pool);
      svn_revnum_t revnum;

      /* Get an RA session for this URL. */
      SVN_ERR(svn_client__ra_session_from_path(&ra_session, &revnum,
                                               &url, path_or_url, NULL,
                                               peg_revision,
                                               revision, ctx, pool));

      SVN_ERR(svn_ra_check_path(ra_session, "", revnum, &kind, pool));

      SVN_ERR(remote_proplist(url, "", kind, revnum, ra_session, depth,
                              receiver, receiver_baton, pool, subpool));
      svn_pool_destroy(subpool);
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_client_revprop_list(apr_hash_t **props,
                        const char *URL,
                        const svn_opt_revision_t *revision,
                        svn_revnum_t *set_rev,
                        svn_client_ctx_t *ctx,
                        apr_pool_t *pool)
{
  svn_ra_session_t *ra_session;
  apr_hash_t *proplist;

  /* Open an RA session for the URL. Note that we don't have a local
     directory, nor a place to put temp files. */
  SVN_ERR(svn_client__open_ra_session_internal(&ra_session, NULL, URL, NULL,
                                               NULL, FALSE, TRUE, ctx, pool));

  /* Resolve the revision into something real, and return that to the
     caller as well. */
  SVN_ERR(svn_client__get_revision_number(set_rev, NULL, ctx->wc_ctx, NULL,
                                          ra_session, revision, pool));

  /* The actual RA call. */
  SVN_ERR(svn_ra_rev_proplist(ra_session, *set_rev, &proplist, pool));

  *props = proplist;
  return SVN_NO_ERROR;
}

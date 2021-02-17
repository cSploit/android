/* fs-wrap.c --- filesystem interface wrappers.
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "svn_pools.h"
#include "svn_error.h"
#include "svn_fs.h"
#include "svn_path.h"
#include "svn_props.h"
#include "svn_repos.h"
#include "svn_time.h"
#include "repos.h"
#include "svn_private_config.h"
#include "private/svn_repos_private.h"
#include "private/svn_utf_private.h"


/*** Commit wrappers ***/

svn_error_t *
svn_repos_fs_commit_txn(const char **conflict_p,
                        svn_repos_t *repos,
                        svn_revnum_t *new_rev,
                        svn_fs_txn_t *txn,
                        apr_pool_t *pool)
{
  svn_error_t *err, *err2;
  const char *txn_name;

  *new_rev = SVN_INVALID_REVNUM;

  /* Run pre-commit hooks. */
  SVN_ERR(svn_fs_txn_name(&txn_name, txn, pool));
  SVN_ERR(svn_repos__hooks_pre_commit(repos, txn_name, pool));

  /* Commit. */
  err = svn_fs_commit_txn(conflict_p, new_rev, txn, pool);
  if (! SVN_IS_VALID_REVNUM(*new_rev))
    return err;

  /* Run post-commit hooks. */
  if ((err2 = svn_repos__hooks_post_commit(repos, *new_rev, pool)))
    {
      err2 = svn_error_create
               (SVN_ERR_REPOS_POST_COMMIT_HOOK_FAILED, err2,
                _("Commit succeeded, but post-commit hook failed"));
    }

  return svn_error_compose_create(err, err2);
}



/*** Transaction creation wrappers. ***/


svn_error_t *
svn_repos_fs_begin_txn_for_commit2(svn_fs_txn_t **txn_p,
                                   svn_repos_t *repos,
                                   svn_revnum_t rev,
                                   apr_hash_t *revprop_table,
                                   apr_pool_t *pool)
{
  svn_string_t *author = apr_hash_get(revprop_table, SVN_PROP_REVISION_AUTHOR,
                                      APR_HASH_KEY_STRING);
  apr_array_header_t *revprops;

  /* Run start-commit hooks. */
  SVN_ERR(svn_repos__hooks_start_commit(repos, author ? author->data : NULL,
                                        repos->client_capabilities, pool));

  /* Begin the transaction, ask for the fs to do on-the-fly lock checks. */
  SVN_ERR(svn_fs_begin_txn2(txn_p, repos->fs, rev,
                            SVN_FS_TXN_CHECK_LOCKS, pool));

  /* We pass the revision properties to the filesystem by adding them
     as properties on the txn.  Later, when we commit the txn, these
     properties will be copied into the newly created revision. */
  revprops = svn_prop_hash_to_array(revprop_table, pool);
  return svn_repos_fs_change_txn_props(*txn_p, revprops, pool);
}


svn_error_t *
svn_repos_fs_begin_txn_for_commit(svn_fs_txn_t **txn_p,
                                  svn_repos_t *repos,
                                  svn_revnum_t rev,
                                  const char *author,
                                  const char *log_msg,
                                  apr_pool_t *pool)
{
  apr_hash_t *revprop_table = apr_hash_make(pool);
  if (author)
    apr_hash_set(revprop_table, SVN_PROP_REVISION_AUTHOR,
                 APR_HASH_KEY_STRING,
                 svn_string_create(author, pool));
  if (log_msg)
    apr_hash_set(revprop_table, SVN_PROP_REVISION_LOG,
                 APR_HASH_KEY_STRING,
                 svn_string_create(log_msg, pool));
  return svn_repos_fs_begin_txn_for_commit2(txn_p, repos, rev, revprop_table,
                                            pool);
}


svn_error_t *
svn_repos_fs_begin_txn_for_update(svn_fs_txn_t **txn_p,
                                  svn_repos_t *repos,
                                  svn_revnum_t rev,
                                  const char *author,
                                  apr_pool_t *pool)
{
  /* ### someday, we might run a read-hook here. */

  /* Begin the transaction. */
  SVN_ERR(svn_fs_begin_txn2(txn_p, repos->fs, rev, 0, pool));

  /* We pass the author to the filesystem by adding it as a property
     on the txn. */

  /* User (author). */
  if (author)
    {
      svn_string_t val;
      val.data = author;
      val.len = strlen(author);
      SVN_ERR(svn_fs_change_txn_prop(*txn_p, SVN_PROP_REVISION_AUTHOR,
                                     &val, pool));
    }

  return SVN_NO_ERROR;
}



/*** Property wrappers ***/

svn_error_t *
svn_repos__validate_prop(const char *name,
                         const svn_string_t *value,
                         apr_pool_t *pool)
{
  svn_prop_kind_t kind = svn_property_kind(NULL, name);

  /* Disallow setting non-regular properties. */
  if (kind != svn_prop_regular_kind)
    return svn_error_createf
      (SVN_ERR_REPOS_BAD_ARGS, NULL,
       _("Storage of non-regular property '%s' is disallowed through the "
         "repository interface, and could indicate a bug in your client"),
       name);

  /* Validate "svn:" properties. */
  if (svn_prop_is_svn_prop(name) && value != NULL)
    {
      /* Validate that translated props (e.g., svn:log) are UTF-8 with
       * LF line endings. */
      if (svn_prop_needs_translation(name))
        {
          if (svn_utf__is_valid(value->data, value->len) == FALSE)
            {
              return svn_error_createf
                (SVN_ERR_BAD_PROPERTY_VALUE, NULL,
                 _("Cannot accept '%s' property because it is not encoded in "
                   "UTF-8"), name);
            }

          /* Disallow inconsistent line ending style, by simply looking for
           * carriage return characters ('\r'). */
          if (strchr(value->data, '\r') != NULL)
            {
              return svn_error_createf
                (SVN_ERR_BAD_PROPERTY_VALUE, NULL,
                 _("Cannot accept non-LF line endings in '%s' property"),
                   name);
            }
        }

      /* "svn:date" should be a valid date. */
      if (strcmp(name, SVN_PROP_REVISION_DATE) == 0)
        {
          apr_time_t temp;
          svn_error_t *err;

          err = svn_time_from_cstring(&temp, value->data, pool);
          if (err)
            return svn_error_create(SVN_ERR_BAD_PROPERTY_VALUE,
                                    err, NULL);
        }
    }

  return SVN_NO_ERROR;
}


/* Verify the mergeinfo property value VALUE and return an error if it
 * is invalid. The PATH on which that property is set is used for error
 * messages only.  Use SCRATCH_POOL for temporary allocations. */
static svn_error_t *
verify_mergeinfo(const svn_string_t *value,
                 const char *path,
                 apr_pool_t *scratch_pool)
{
  svn_error_t *err;
  svn_mergeinfo_t mergeinfo;

  /* It's okay to delete svn:mergeinfo. */
  if (value == NULL)
    return SVN_NO_ERROR;

  /* Mergeinfo is UTF-8 encoded so the number of bytes returned by strlen()
   * should match VALUE->LEN. Prevents trailing garbage in the property. */
  if (strlen(value->data) != value->len)
    return svn_error_createf(SVN_ERR_MERGEINFO_PARSE_ERROR, NULL,
                             _("Commit rejected because mergeinfo on '%s' "
                               "contains unexpected string terminator"),
                             path);

  err = svn_mergeinfo_parse(&mergeinfo, value->data, scratch_pool);
  if (err)
    return svn_error_createf(err->apr_err, err,
                             _("Commit rejected because mergeinfo on '%s' "
                               "is syntactically invalid"),
                             path);
  return SVN_NO_ERROR;
}


svn_error_t *
svn_repos_fs_change_node_prop(svn_fs_root_t *root,
                              const char *path,
                              const char *name,
                              const svn_string_t *value,
                              apr_pool_t *pool)
{
  if (value && strcmp(name, SVN_PROP_MERGEINFO) == 0)
    SVN_ERR(verify_mergeinfo(value, path, pool));

  /* Validate the property, then call the wrapped function. */
  SVN_ERR(svn_repos__validate_prop(name, value, pool));
  return svn_fs_change_node_prop(root, path, name, value, pool);
}


svn_error_t *
svn_repos_fs_change_txn_props(svn_fs_txn_t *txn,
                              const apr_array_header_t *txnprops,
                              apr_pool_t *pool)
{
  int i;

  for (i = 0; i < txnprops->nelts; i++)
    {
      svn_prop_t *prop = &APR_ARRAY_IDX(txnprops, i, svn_prop_t);
      SVN_ERR(svn_repos__validate_prop(prop->name, prop->value, pool));
    }

  return svn_fs_change_txn_props(txn, txnprops, pool);
}


svn_error_t *
svn_repos_fs_change_txn_prop(svn_fs_txn_t *txn,
                             const char *name,
                             const svn_string_t *value,
                             apr_pool_t *pool)
{
  apr_array_header_t *props = apr_array_make(pool, 1, sizeof(svn_prop_t));
  svn_prop_t prop;

  prop.name = name;
  prop.value = value;
  APR_ARRAY_PUSH(props, svn_prop_t) = prop;

  return svn_repos_fs_change_txn_props(txn, props, pool);
}


svn_error_t *
svn_repos_fs_change_rev_prop4(svn_repos_t *repos,
                              svn_revnum_t rev,
                              const char *author,
                              const char *name,
                              const svn_string_t *const *old_value_p,
                              const svn_string_t *new_value,
                              svn_boolean_t use_pre_revprop_change_hook,
                              svn_boolean_t use_post_revprop_change_hook,
                              svn_repos_authz_func_t authz_read_func,
                              void *authz_read_baton,
                              apr_pool_t *pool)
{
  svn_repos_revision_access_level_t readability;

  SVN_ERR(svn_repos_check_revision_access(&readability, repos, rev,
                                          authz_read_func, authz_read_baton,
                                          pool));

  if (readability == svn_repos_revision_access_full)
    {
      const svn_string_t *old_value;
      char action;

      SVN_ERR(svn_repos__validate_prop(name, new_value, pool));

      /* Fetch OLD_VALUE for svn_fs_change_rev_prop2(). */
      if (old_value_p)
        {
          old_value = *old_value_p;
        }
      else
        {
          /* Get OLD_VALUE anyway, in order for ACTION and OLD_VALUE arguments
           * to the hooks to be accurate. */
          svn_string_t *old_value2;

          SVN_ERR(svn_fs_revision_prop(&old_value2, repos->fs, rev, name, pool));
          old_value = old_value2;
        }

      /* Prepare ACTION. */
      if (! new_value)
        action = 'D';
      else if (! old_value)
        action = 'A';
      else
        action = 'M';

      /* ### currently not passing the old_value to hooks */
      if (use_pre_revprop_change_hook)
        SVN_ERR(svn_repos__hooks_pre_revprop_change(repos, rev, author, name,
                                                    new_value, action, pool));

      SVN_ERR(svn_fs_change_rev_prop2(repos->fs, rev, name,
                                      &old_value, new_value, pool));

      if (use_post_revprop_change_hook)
        SVN_ERR(svn_repos__hooks_post_revprop_change(repos, rev, author,  name,
                                                     old_value, action, pool));
    }
  else  /* rev is either unreadable or only partially readable */
    {
      return svn_error_createf
        (SVN_ERR_AUTHZ_UNREADABLE, NULL,
         _("Write denied:  not authorized to read all of revision %ld"), rev);
    }

  return SVN_NO_ERROR;
}


svn_error_t *
svn_repos_fs_revision_prop(svn_string_t **value_p,
                           svn_repos_t *repos,
                           svn_revnum_t rev,
                           const char *propname,
                           svn_repos_authz_func_t authz_read_func,
                           void *authz_read_baton,
                           apr_pool_t *pool)
{
  svn_repos_revision_access_level_t readability;

  SVN_ERR(svn_repos_check_revision_access(&readability, repos, rev,
                                          authz_read_func, authz_read_baton,
                                          pool));

  if (readability == svn_repos_revision_access_none)
    {
      /* Property?  What property? */
      *value_p = NULL;
    }
  else if (readability == svn_repos_revision_access_partial)
    {
      /* Only svn:author and svn:date are fetchable. */
      if ((strncmp(propname, SVN_PROP_REVISION_AUTHOR,
                   strlen(SVN_PROP_REVISION_AUTHOR)) != 0)
          && (strncmp(propname, SVN_PROP_REVISION_DATE,
                      strlen(SVN_PROP_REVISION_DATE)) != 0))
        *value_p = NULL;

      else
        SVN_ERR(svn_fs_revision_prop(value_p, repos->fs,
                                     rev, propname, pool));
    }
  else /* wholly readable revision */
    {
      SVN_ERR(svn_fs_revision_prop(value_p, repos->fs, rev, propname, pool));
    }

  return SVN_NO_ERROR;
}



svn_error_t *
svn_repos_fs_revision_proplist(apr_hash_t **table_p,
                               svn_repos_t *repos,
                               svn_revnum_t rev,
                               svn_repos_authz_func_t authz_read_func,
                               void *authz_read_baton,
                               apr_pool_t *pool)
{
  svn_repos_revision_access_level_t readability;

  SVN_ERR(svn_repos_check_revision_access(&readability, repos, rev,
                                          authz_read_func, authz_read_baton,
                                          pool));

  if (readability == svn_repos_revision_access_none)
    {
      /* Return an empty hash. */
      *table_p = apr_hash_make(pool);
    }
  else if (readability == svn_repos_revision_access_partial)
    {
      apr_hash_t *tmphash;
      svn_string_t *value;

      /* Produce two property hashtables, both in POOL. */
      SVN_ERR(svn_fs_revision_proplist(&tmphash, repos->fs, rev, pool));
      *table_p = apr_hash_make(pool);

      /* If they exist, we only copy svn:author and svn:date into the
         'real' hashtable being returned. */
      value = apr_hash_get(tmphash, SVN_PROP_REVISION_AUTHOR,
                           APR_HASH_KEY_STRING);
      if (value)
        apr_hash_set(*table_p, SVN_PROP_REVISION_AUTHOR,
                     APR_HASH_KEY_STRING, value);

      value = apr_hash_get(tmphash, SVN_PROP_REVISION_DATE,
                           APR_HASH_KEY_STRING);
      if (value)
        apr_hash_set(*table_p, SVN_PROP_REVISION_DATE,
                     APR_HASH_KEY_STRING, value);
    }
  else /* wholly readable revision */
    {
      SVN_ERR(svn_fs_revision_proplist(table_p, repos->fs, rev, pool));
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_repos_fs_lock(svn_lock_t **lock,
                  svn_repos_t *repos,
                  const char *path,
                  const char *token,
                  const char *comment,
                  svn_boolean_t is_dav_comment,
                  apr_time_t expiration_date,
                  svn_revnum_t current_rev,
                  svn_boolean_t steal_lock,
                  apr_pool_t *pool)
{
  svn_error_t *err;
  svn_fs_access_t *access_ctx = NULL;
  const char *username = NULL;
  const char *new_token;
  apr_array_header_t *paths;

  /* Setup an array of paths in anticipation of the ra layers handling
     multiple locks in one request (1.3 most likely).  This is only
     used by svn_repos__hooks_post_lock. */
  paths = apr_array_make(pool, 1, sizeof(const char *));
  APR_ARRAY_PUSH(paths, const char *) = path;

  SVN_ERR(svn_fs_get_access(&access_ctx, repos->fs));
  if (access_ctx)
    SVN_ERR(svn_fs_access_get_username(&username, access_ctx));

  if (! username)
    return svn_error_createf
      (SVN_ERR_FS_NO_USER, NULL,
       "Cannot lock path '%s', no authenticated username available.", path);

  /* Run pre-lock hook.  This could throw error, preventing
     svn_fs_lock() from happening. */
  SVN_ERR(svn_repos__hooks_pre_lock(repos, &new_token, path, username, comment,
                                    steal_lock, pool));
  if (*new_token)
    token = new_token;

  /* Lock. */
  SVN_ERR(svn_fs_lock(lock, repos->fs, path, token, comment, is_dav_comment,
                      expiration_date, current_rev, steal_lock, pool));

  /* Run post-lock hook. */
  if ((err = svn_repos__hooks_post_lock(repos, paths, username, pool)))
    return svn_error_create
      (SVN_ERR_REPOS_POST_LOCK_HOOK_FAILED, err,
       "Lock succeeded, but post-lock hook failed");

  return SVN_NO_ERROR;
}


svn_error_t *
svn_repos_fs_unlock(svn_repos_t *repos,
                    const char *path,
                    const char *token,
                    svn_boolean_t break_lock,
                    apr_pool_t *pool)
{
  svn_error_t *err;
  svn_fs_access_t *access_ctx = NULL;
  const char *username = NULL;
  /* Setup an array of paths in anticipation of the ra layers handling
     multiple locks in one request (1.3 most likely).  This is only
     used by svn_repos__hooks_post_lock. */
  apr_array_header_t *paths = apr_array_make(pool, 1, sizeof(const char *));
  APR_ARRAY_PUSH(paths, const char *) = path;

  SVN_ERR(svn_fs_get_access(&access_ctx, repos->fs));
  if (access_ctx)
    SVN_ERR(svn_fs_access_get_username(&username, access_ctx));

  if (! break_lock && ! username)
    return svn_error_createf
      (SVN_ERR_FS_NO_USER, NULL,
       _("Cannot unlock path '%s', no authenticated username available"),
       path);

  /* Run pre-unlock hook.  This could throw error, preventing
     svn_fs_unlock() from happening. */
  SVN_ERR(svn_repos__hooks_pre_unlock(repos, path, username, token,
                                      break_lock, pool));

  /* Unlock. */
  SVN_ERR(svn_fs_unlock(repos->fs, path, token, break_lock, pool));

  /* Run post-unlock hook. */
  if ((err = svn_repos__hooks_post_unlock(repos, paths, username, pool)))
    return svn_error_create
      (SVN_ERR_REPOS_POST_UNLOCK_HOOK_FAILED, err,
       _("Unlock succeeded, but post-unlock hook failed"));

  return SVN_NO_ERROR;
}


struct get_locks_baton_t
{
  svn_fs_t *fs;
  svn_fs_root_t *head_root;
  svn_repos_authz_func_t authz_read_func;
  void *authz_read_baton;
  apr_hash_t *locks;
};


/* This implements the svn_fs_get_locks_callback_t interface. */
static svn_error_t *
get_locks_callback(void *baton,
                   svn_lock_t *lock,
                   apr_pool_t *pool)
{
  struct get_locks_baton_t *b = baton;
  svn_boolean_t readable = TRUE;
  apr_pool_t *hash_pool = apr_hash_pool_get(b->locks);

  /* If there's auth to deal with, deal with it. */
  if (b->authz_read_func)
    SVN_ERR(b->authz_read_func(&readable, b->head_root, lock->path,
                               b->authz_read_baton, pool));

  /* If we can read this lock path, add the lock to the return hash. */
  if (readable)
    apr_hash_set(b->locks, apr_pstrdup(hash_pool, lock->path),
                 APR_HASH_KEY_STRING, svn_lock_dup(lock, hash_pool));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_repos_fs_get_locks2(apr_hash_t **locks,
                        svn_repos_t *repos,
                        const char *path,
                        svn_depth_t depth,
                        svn_repos_authz_func_t authz_read_func,
                        void *authz_read_baton,
                        apr_pool_t *pool)
{
  apr_hash_t *all_locks = apr_hash_make(pool);
  svn_revnum_t head_rev;
  struct get_locks_baton_t baton;

  SVN_ERR_ASSERT((depth == svn_depth_empty) ||
                 (depth == svn_depth_files) ||
                 (depth == svn_depth_immediates) ||
                 (depth == svn_depth_infinity));

  SVN_ERR(svn_fs_youngest_rev(&head_rev, repos->fs, pool));

  /* Populate our callback baton. */
  baton.fs = repos->fs;
  baton.locks = all_locks;
  baton.authz_read_func = authz_read_func;
  baton.authz_read_baton = authz_read_baton;
  SVN_ERR(svn_fs_revision_root(&(baton.head_root), repos->fs,
                               head_rev, pool));

  /* Get all the locks. */
  SVN_ERR(svn_fs_get_locks2(repos->fs, path, depth,
                            get_locks_callback, &baton, pool));

  *locks = baton.locks;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_repos_fs_get_mergeinfo(svn_mergeinfo_catalog_t *mergeinfo,
                           svn_repos_t *repos,
                           const apr_array_header_t *paths,
                           svn_revnum_t rev,
                           svn_mergeinfo_inheritance_t inherit,
                           svn_boolean_t include_descendants,
                           svn_repos_authz_func_t authz_read_func,
                           void *authz_read_baton,
                           apr_pool_t *pool)
{
  /* Here we cast away 'const', but won't try to write through this pointer
   * without first allocating a new array. */
  apr_array_header_t *readable_paths = (apr_array_header_t *) paths;
  svn_fs_root_t *root;
  apr_pool_t *iterpool = svn_pool_create(pool);

  if (!SVN_IS_VALID_REVNUM(rev))
    SVN_ERR(svn_fs_youngest_rev(&rev, repos->fs, pool));
  SVN_ERR(svn_fs_revision_root(&root, repos->fs, rev, pool));

  /* Filter out unreadable paths before divining merge tracking info. */
  if (authz_read_func)
    {
      int i;

      for (i = 0; i < paths->nelts; i++)
        {
          svn_boolean_t readable;
          const char *path = APR_ARRAY_IDX(paths, i, char *);
          svn_pool_clear(iterpool);
          SVN_ERR(authz_read_func(&readable, root, path, authz_read_baton,
                                  iterpool));
          if (readable && readable_paths != paths)
            APR_ARRAY_PUSH(readable_paths, const char *) = path;
          else if (!readable && readable_paths == paths)
            {
              /* Requested paths differ from readable paths.  Fork
                 list of readable paths from requested paths. */
              int j;
              readable_paths = apr_array_make(pool, paths->nelts - 1,
                                              sizeof(char *));
              for (j = 0; j < i; j++)
                {
                  path = APR_ARRAY_IDX(paths, j, char *);
                  APR_ARRAY_PUSH(readable_paths, const char *) = path;
                }
            }
        }
    }

  /* We consciously do not perform authz checks on the paths returned
     in *MERGEINFO, avoiding massive authz overhead which would allow
     us to protect the name of where a change was merged from, but not
     the change itself. */
  /* ### TODO(reint): ... but how about descendant merged-to paths? */
  if (readable_paths->nelts > 0)
    SVN_ERR(svn_fs_get_mergeinfo(mergeinfo, root, readable_paths, inherit,
                                 include_descendants, pool));
  else
    *mergeinfo = apr_hash_make(pool);

  svn_pool_destroy(iterpool);
  return SVN_NO_ERROR;
}

struct pack_notify_baton
{
  svn_repos_notify_func_t notify_func;
  void *notify_baton;
};

/* Implements svn_fs_pack_notify_t. */
static svn_error_t *
pack_notify_func(void *baton,
                 apr_int64_t shard,
                 svn_fs_pack_notify_action_t pack_action,
                 apr_pool_t *pool)
{
  struct pack_notify_baton *pnb = baton;
  svn_repos_notify_t *notify;

  notify = svn_repos_notify_create(pack_action + 3, pool);
  notify->shard = shard;
  pnb->notify_func(pnb->notify_baton, notify, pool);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_repos_fs_pack2(svn_repos_t *repos,
                   svn_repos_notify_func_t notify_func,
                   void *notify_baton,
                   svn_cancel_func_t cancel_func,
                   void *cancel_baton,
                   apr_pool_t *pool)
{
  struct pack_notify_baton pnb;

  pnb.notify_func = notify_func;
  pnb.notify_baton = notify_baton;

  return svn_fs_pack(repos->db_path,
                     notify_func ? pack_notify_func : NULL,
                     notify_func ? &pnb : NULL,
                     cancel_func, cancel_baton, pool);
}



/*
 * vim:ts=4:sw=2:expandtab:tw=80:fo=tcroq
 * vim:isk=a-z,A-Z,48-57,_,.,-,>
 * vim:cino=>1s,e0,n0,f0,{.5s,}0,^-.5s,=.5s,t0,+1s,c3,(0,u0,\:0
 */

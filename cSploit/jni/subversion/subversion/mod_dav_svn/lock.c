/*
 * lock.c: mod_dav_svn locking provider functions
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

#include <apr_uuid.h>
#include <apr_time.h>

#include <httpd.h>
#include <http_log.h>
#include <mod_dav.h>

#include "svn_fs.h"
#include "svn_repos.h"
#include "svn_dav.h"
#include "svn_time.h"
#include "svn_pools.h"
#include "svn_props.h"
#include "private/svn_log.h"

#include "dav_svn.h"


struct dav_lockdb_private
{
  /* These represent 'custom' request hearders only sent by svn clients: */
  svn_boolean_t lock_steal;
  svn_boolean_t lock_break;
  svn_boolean_t keep_locks;
  svn_revnum_t working_revnum;

  /* The original request, so we can set 'custom' output headers. */
  request_rec *r;
};


/* Helper func:  convert an svn_lock_t to a dav_lock, allocated in
   pool.  EXISTS_P indicates whether slock->path actually exists or not.
   If HIDE_AUTH_USER is set, then do not return the svn lock's 'owner'
   as dlock->auth_user.
 */
static void
svn_lock_to_dav_lock(dav_lock **dlock,
                     const svn_lock_t *slock,
                     svn_boolean_t hide_auth_user,
                     svn_boolean_t exists_p,
                     apr_pool_t *pool)
{
  dav_lock *lock = apr_pcalloc(pool, sizeof(*lock));
  dav_locktoken *token = apr_pcalloc(pool, sizeof(*token));

  lock->rectype = DAV_LOCKREC_DIRECT;
  lock->scope = DAV_LOCKSCOPE_EXCLUSIVE;
  lock->type = DAV_LOCKTYPE_WRITE;
  lock->depth = 0;
  lock->is_locknull = exists_p;

  token->uuid_str = apr_pstrdup(pool, slock->token);
  lock->locktoken = token;

  /* the svn_lock_t 'comment' field maps to the 'DAV:owner' field. */
  if (slock->comment)
    {
      if (! slock->is_dav_comment)
        {
          /* This comment was originally given to us by an svn client,
             so, we need to wrap the naked comment with <DAV:owner>,
             and xml-escape it for safe transport, lest we send back
             an invalid http response.  (mod_dav won't do it for us,
             because it assumes that it personally created every lock
             in the repository.) */
          lock->owner = apr_pstrcat(pool,
                                    "<D:owner xmlns:D=\"DAV:\">",
                                    apr_xml_quote_string(pool,
                                                         slock->comment, 1),
                                    "</D:owner>", (char *)NULL);
        }
      else
        {
          lock->owner = apr_pstrdup(pool, slock->comment);
        }
    }
  else
    lock->owner = NULL;

  /* the svn_lock_t 'owner' is the actual authenticated owner of the
     lock, and maps to the 'auth_user' field in the mod_dav lock. */

  /* (If the client ran 'svn unlock --force', then we don't want to
     return lock->auth_user.  Otherwise mod_dav will throw an error
     when lock->auth_user and r->user don't match.) */
  if (! hide_auth_user)
    lock->auth_user = apr_pstrdup(pool, slock->owner);

  /* This is absurd.  apr_time.h has an apr_time_t->time_t func,
     but not the reverse?? */
  if (slock->expiration_date)
    lock->timeout = (time_t) (slock->expiration_date / APR_USEC_PER_SEC);
  else
    lock->timeout = DAV_TIMEOUT_INFINITE;

  *dlock = lock;
}


/* Helper func for dav_lock_to_svn_lock:  take an incoming
   "<D:owner>&lt;foo&gt;</D:owner>" tag and convert it to
   "<foo>". */
static dav_error *
unescape_xml(const char **output,
             const char *input,
             apr_pool_t *pool)
{
  apr_xml_parser *xml_parser = apr_xml_parser_create(pool);
  apr_xml_doc *xml_doc;
  apr_status_t apr_err;
  const char *xml_input = apr_pstrcat
    (pool, "<?xml version=\"1.0\" encoding=\"utf-8\"?>", input, (char *)NULL);

  apr_err = apr_xml_parser_feed(xml_parser, xml_input, strlen(xml_input));
  if (!apr_err)
    apr_err = apr_xml_parser_done(xml_parser, &xml_doc);

  if (apr_err)
    {
      char errbuf[1024];
      (void)apr_xml_parser_geterror(xml_parser, errbuf, sizeof(errbuf));
      return dav_svn__new_error(pool, HTTP_INTERNAL_SERVER_ERROR,
                                DAV_ERR_LOCK_SAVE_LOCK, errbuf);
    }

  apr_xml_to_text(pool, xml_doc->root, APR_XML_X2T_INNER,
                  xml_doc->namespaces, NULL, output, NULL);
  return SVN_NO_ERROR;
}


/* Helper func:  convert a dav_lock to an svn_lock_t, allocated in pool. */
static dav_error *
dav_lock_to_svn_lock(svn_lock_t **slock,
                     const dav_lock *dlock,
                     const char *path,
                     dav_lockdb_private *info,
                     svn_boolean_t is_svn_client,
                     apr_pool_t *pool)
{
  svn_lock_t *lock;

  /* Sanity checks */
  if (dlock->type != DAV_LOCKTYPE_WRITE)
    return dav_svn__new_error(pool, HTTP_BAD_REQUEST,
                              DAV_ERR_LOCK_SAVE_LOCK,
                              "Only 'write' locks are supported.");

  if (dlock->scope != DAV_LOCKSCOPE_EXCLUSIVE)
    return dav_svn__new_error(pool, HTTP_BAD_REQUEST,
                              DAV_ERR_LOCK_SAVE_LOCK,
                              "Only exclusive locks are supported.");

  lock = svn_lock_create(pool);
  lock->path = apr_pstrdup(pool, path);
  lock->token = apr_pstrdup(pool, dlock->locktoken->uuid_str);

  /* DAV has no concept of lock creationdate, so assume 'now' */
  lock->creation_date = apr_time_now();

  if (dlock->auth_user)
    lock->owner = apr_pstrdup(pool, dlock->auth_user);

  /* We need to be very careful about stripping the <D:owner> tag away
     from the cdata.  It's okay to do for svn clients, but not other
     DAV clients! */
  if (dlock->owner)
    {
      if (is_svn_client)
        {
          /* mod_dav has forcibly xml-escaped the comment before
             handing it to us; we need to xml-unescape it (and remove
             the <D:owner> wrapper) when storing in the repository, so
             it looks reasonable to the rest of svn. */
          dav_error *derr;
          lock->is_dav_comment = 0;  /* comment is NOT xml-wrapped. */
          derr = unescape_xml(&(lock->comment), dlock->owner, pool);
          if (derr)
            return derr;
        }
      else
        {
          /* The comment comes from a non-svn client;  don't touch
             this data at all. */
          lock->comment = apr_pstrdup(pool, dlock->owner);
          lock->is_dav_comment = 1; /* comment IS xml-wrapped. */
        }
    }

  if (dlock->timeout == DAV_TIMEOUT_INFINITE)
    lock->expiration_date = 0; /* never expires */
  else
    lock->expiration_date = ((apr_time_t)dlock->timeout) * APR_USEC_PER_SEC;

  *slock = lock;
  return 0;
}


/* ---------------------------------------------------------------- */
/* mod_dav locking vtable starts here: */


/* Return the supportedlock property for a resource */
static const char *
get_supportedlock(const dav_resource *resource)
{
  /* This is imitating what mod_dav_fs is doing.  Note that unlike
     mod_dav_fs, however, we don't support "shared" locks, only
     "exclusive" ones.  Nor do we support locks on collections. */
  static const char supported[] = DEBUG_CR
    "<D:lockentry>" DEBUG_CR
    "<D:lockscope><D:exclusive/></D:lockscope>" DEBUG_CR
    "<D:locktype><D:write/></D:locktype>" DEBUG_CR
    "</D:lockentry>" DEBUG_CR;

  if (resource->collection)
    return NULL;
  else
    return supported;
}


/* Parse a lock token URI, returning a lock token object allocated
 * in the given pool.
 */
static dav_error *
parse_locktoken(apr_pool_t *pool,
                const char *char_token,
                dav_locktoken **locktoken_p)
{
  dav_locktoken *token = apr_pcalloc(pool, sizeof(*token));

  /* libsvn_fs already produces a valid locktoken URI. */
  token->uuid_str = apr_pstrdup(pool, char_token);

  *locktoken_p = token;
  return 0;
}


/* Format a lock token object into a URI string, allocated in
 * the given pool.
 *
 * Always returns non-NULL.
 */
static const char *
format_locktoken(apr_pool_t *p, const dav_locktoken *locktoken)
{
  /* libsvn_fs already produces a valid locktoken URI. */
  return apr_pstrdup(p, locktoken->uuid_str);
}


/* Compare two lock tokens.
 *
 * Result < 0  => lt1 < lt2
 * Result == 0 => lt1 == lt2
 * Result > 0  => lt1 > lt2
 */
static int
compare_locktoken(const dav_locktoken *lt1, const dav_locktoken *lt2)
{
  return strcmp(lt1->uuid_str, lt2->uuid_str);
}


/* Open the provider's lock database.
 *
 * The provider may or may not use a "real" database for locks
 * (a lock could be an attribute on a resource, for example).
 *
 * The provider may choose to use the value of the DAVLockDB directive
 * (as returned by dav_get_lockdb_path()) to decide where to place
 * any storage it may need.
 *
 * The request storage pool should be associated with the lockdb,
 * so it can be used in subsequent operations.
 *
 * If ro != 0, only readonly operations will be performed.
 * If force == 0, the open can be "lazy"; no subsequent locking operations
 * may occur.
 * If force != 0, locking operations will definitely occur.
 */
static dav_error *
open_lockdb(request_rec *r, int ro, int force, dav_lockdb **lockdb)
{
  const char *svn_client_options, *version_name;
  dav_lockdb *db = apr_pcalloc(r->pool, sizeof(*db));
  dav_lockdb_private *info = apr_pcalloc(r->pool, sizeof(*info));

  info->r = r;

  /* Is this an svn client? */

  /* Check to see if an svn client sent any custom X-SVN-* headers in
     the request. */
  svn_client_options = apr_table_get(r->headers_in, SVN_DAV_OPTIONS_HEADER);
  if (svn_client_options)
    {
      /* 'svn [lock | unlock] --force' */
      if (ap_strstr_c(svn_client_options, SVN_DAV_OPTION_LOCK_BREAK))
        info->lock_break = TRUE;
      if (ap_strstr_c(svn_client_options, SVN_DAV_OPTION_LOCK_STEAL))
        info->lock_steal = TRUE;
      if (ap_strstr_c(svn_client_options, SVN_DAV_OPTION_KEEP_LOCKS))
        info->keep_locks = TRUE;
    }

  /* 'svn lock' wants to make svn_fs_lock() do an out-of-dateness check. */
  version_name = apr_table_get(r->headers_in, SVN_DAV_VERSION_NAME_HEADER);
  info->working_revnum = version_name ?
                         SVN_STR_TO_REV(version_name): SVN_INVALID_REVNUM;

  /* The generic lockdb structure.  */
  db->hooks = &dav_svn__hooks_locks;
  db->ro = ro;
  db->info = info;

  *lockdb = db;
  return 0;
}


/* Indicates completion of locking operations */
static void
close_lockdb(dav_lockdb *lockdb)
{
  /* nothing to do here. */
  return;
}


/* Take a resource out of the lock-null state. */
static dav_error *
remove_locknull_state(dav_lockdb *lockdb, const dav_resource *resource)
{

  /* mod_dav_svn supports RFC2518bis which does not actually require
     the server to create lock-null resources.  Instead, we create
     zero byte files when a lock comes in on a non-existent path.
     mod_dav_svn never creates any lock-null resources, so this
     function is never called by mod_dav. */

  return 0;  /* Just to suppress compiler warnings. */
}


/*
** Create a (direct) lock structure for the given resource. A locktoken
** will be created.
**
** The lock provider may store private information into lock->info.
*/
static dav_error *
create_lock(dav_lockdb *lockdb, const dav_resource *resource, dav_lock **lock)
{
  svn_error_t *serr;
  dav_locktoken *token = apr_pcalloc(resource->pool, sizeof(*token));
  dav_lock *dlock = apr_pcalloc(resource->pool, sizeof(*dlock));

  dlock->rectype = DAV_LOCKREC_DIRECT;
  dlock->is_locknull = resource->exists;
  dlock->scope = DAV_LOCKSCOPE_UNKNOWN;
  dlock->type = DAV_LOCKTYPE_UNKNOWN;
  dlock->depth = 0;

  serr = svn_fs_generate_lock_token(&(token->uuid_str),
                                    resource->info->repos->fs,
                                    resource->pool);
  if (serr)
    return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                "Failed to generate a lock token.",
                                resource->pool);
  dlock->locktoken = token;

  /* allowing mod_dav to fill in dlock->timeout, owner, auth_user. */
  /* dlock->info and dlock->next are NULL by default. */

  *lock = dlock;
  return 0;
}


/*
** Get the locks associated with the specified resource.
**
** If resolve_locks is true (non-zero), then any indirect locks are
** resolved to their actual, direct lock (i.e. the reference to followed
** to the original lock).
**
** The locks, if any, are returned as a linked list in no particular
** order. If no locks are present, then *locks will be NULL.
**
** #define DAV_GETLOCKS_RESOLVED   0    -- resolve indirects to directs
** #define DAV_GETLOCKS_PARTIAL    1    -- leave indirects partially filled
** #define DAV_GETLOCKS_COMPLETE   2    -- fill out indirect locks
*/
static dav_error *
get_locks(dav_lockdb *lockdb,
          const dav_resource *resource,
          int calltype,
          dav_lock **locks)
{
  dav_lockdb_private *info = lockdb->info;
  svn_error_t *serr;
  svn_lock_t *slock;
  dav_lock *lock = NULL;

  /* We only support exclusive locks, not shared ones.  So this
     function always returns a "list" of exactly one lock, or just a
     NULL list.  The 'calltype' arg is also meaningless, since we
     don't support locks on collections.  */

  /* Sanity check:  if the resource has no associated path in the fs,
     then there's nothing to do.  */
  if (! resource->info->repos_path)
    {
      *locks = NULL;
      return 0;
    }

  /* The Big Lie: if the client ran 'svn lock', then we have
     to pretend that there's no existing lock.  Otherwise mod_dav will
     throw '403 Locked' without even attempting to create a new
     lock.  For the --force case, this is required and for the non-force case,
     we allow the filesystem to produce a better error for svn clients.
  */
  if (info->r->method_number == M_LOCK)
    {
      *locks = NULL;
      return 0;
    }

  /* If the resource's fs path is unreadable, we don't want to say
     anything about locks attached to it.*/
  if (! dav_svn__allow_read_resource(resource, SVN_INVALID_REVNUM,
                                     resource->pool))
    return dav_svn__new_error(resource->pool, HTTP_FORBIDDEN,
                              DAV_ERR_LOCK_SAVE_LOCK,
                              "Path is not accessible.");

  serr = svn_fs_get_lock(&slock,
                         resource->info->repos->fs,
                         resource->info->repos_path,
                         resource->pool);
  if (serr)
    return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                "Failed to check path for a lock.",
                                resource->pool);

  if (slock != NULL)
    {
      svn_lock_to_dav_lock(&lock, slock, info->lock_break,
                           resource->exists, resource->pool);

      /* Let svn clients know the creationdate of the slock. */
      apr_table_setn(info->r->headers_out, SVN_DAV_CREATIONDATE_HEADER,
                     svn_time_to_cstring(slock->creation_date,
                                         resource->pool));

      /* Let svn clients know who "owns" the slock. */
      apr_table_setn(info->r->headers_out, SVN_DAV_LOCK_OWNER_HEADER,
                     slock->owner);
    }

  *locks = lock;
  return 0;
}


/*
** Find a particular lock on a resource (specified by its locktoken).
**
** *lock will be set to NULL if the lock is not found.
**
** Note that the provider can optimize the unmarshalling -- only one
** lock (or none) must be constructed and returned.
**
** If partial_ok is true (non-zero), then an indirect lock can be
** partially filled in. Otherwise, another lookup is done and the
** lock structure will be filled out as a DAV_LOCKREC_INDIRECT.
*/
static dav_error *
find_lock(dav_lockdb *lockdb,
          const dav_resource *resource,
          const dav_locktoken *locktoken,
          int partial_ok,
          dav_lock **lock)
{
  dav_lockdb_private *info = lockdb->info;
  svn_error_t *serr;
  svn_lock_t *slock;
  dav_lock *dlock = NULL;

  /* If the resource's fs path is unreadable, we don't want to say
     anything about locks attached to it.*/
  if (! dav_svn__allow_read_resource(resource, SVN_INVALID_REVNUM,
                                     resource->pool))
    return dav_svn__new_error(resource->pool, HTTP_FORBIDDEN,
                              DAV_ERR_LOCK_SAVE_LOCK,
                              "Path is not accessible.");

  serr = svn_fs_get_lock(&slock,
                         resource->info->repos->fs,
                         resource->info->repos_path,
                         resource->pool);
  if (serr)
    return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                "Failed to look up lock by path.",
                                resource->pool);

  if (slock != NULL)
    {
      /* Sanity check. */
      if (strcmp(locktoken->uuid_str, slock->token) != 0)
        return dav_svn__new_error(resource->pool, HTTP_BAD_REQUEST,
                                  DAV_ERR_LOCK_SAVE_LOCK,
                                  "Incoming token doesn't match existing "
                                  "lock.");

      svn_lock_to_dav_lock(&dlock, slock, FALSE,
                           resource->exists, resource->pool);

      /* Let svn clients know the creationdate of the slock. */
      apr_table_setn(info->r->headers_out, SVN_DAV_CREATIONDATE_HEADER,
                     svn_time_to_cstring(slock->creation_date,
                                         resource->pool));

      /* Let svn clients know the 'owner' of the slock. */
      apr_table_setn(info->r->headers_out, SVN_DAV_LOCK_OWNER_HEADER,
                     slock->owner);
    }

  *lock = dlock;
  return 0;
}


/*
** Quick test to see if the resource has *any* locks on it.
**
** This is typically used to determine if a non-existent resource
** has a lock and is (therefore) a locknull resource.
**
** WARNING: this function may return TRUE even when timed-out locks
**          exist (i.e. it may not perform timeout checks).
*/
static dav_error *
has_locks(dav_lockdb *lockdb, const dav_resource *resource, int *locks_present)
{
  dav_lockdb_private *info = lockdb->info;
  svn_error_t *serr;
  svn_lock_t *slock;

  /* Sanity check:  if the resource has no associated path in the fs,
     then there's nothing to do.  */
  if (! resource->info->repos_path)
    {
      *locks_present = 0;
      return 0;
    }

  /* The Big Lie: if the client ran 'svn lock', then we have
     to pretend that there's no existing lock.  Otherwise mod_dav will
     throw '403 Locked' without even attempting to create a new
     lock.  For the --force case, this is required and for the non-force case,
     we allow the filesystem to produce a better error for svn clients.
  */
  if (info->r->method_number == M_LOCK)
    {
      *locks_present = 0;
      return 0;
    }

  /* If the resource's fs path is unreadable, we don't want to say
     anything about locks attached to it.*/
  if (! dav_svn__allow_read_resource(resource, SVN_INVALID_REVNUM,
                                     resource->pool))
    return dav_svn__new_error(resource->pool, HTTP_FORBIDDEN,
                              DAV_ERR_LOCK_SAVE_LOCK,
                              "Path is not accessible.");

  serr = svn_fs_get_lock(&slock,
                         resource->info->repos->fs,
                         resource->info->repos_path,
                         resource->pool);
  if (serr)
    return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                "Failed to check path for a lock.",
                                resource->pool);

  *locks_present = slock ? 1 : 0;
  return 0;
}


/*
** Append the specified lock(s) to the set of locks on this resource.
**
** If "make_indirect" is true (non-zero), then the specified lock(s)
** should be converted to an indirect lock (if it is a direct lock)
** before appending. Note that the conversion to an indirect lock does
** not alter the passed-in lock -- the change is internal the
** append_locks function.
**
** Multiple locks are specified using the lock->next links.
*/
static dav_error *
append_locks(dav_lockdb *lockdb,
             const dav_resource *resource,
             int make_indirect,
             const dav_lock *lock)
{
  dav_lockdb_private *info = lockdb->info;
  svn_lock_t *slock;
  svn_error_t *serr;
  dav_error *derr;
  dav_svn_repos *repos = resource->info->repos;
      
  /* We don't allow anonymous locks */
  if (! repos->username)
    return dav_svn__new_error(resource->pool, HTTP_UNAUTHORIZED,
                              DAV_ERR_LOCK_SAVE_LOCK,
                              "Anonymous lock creation is not allowed.");

  /* Not a path in the repository so can't lock it. */
  if (! resource->info->repos_path)
    return dav_svn__new_error(resource->pool, HTTP_BAD_REQUEST,
                              DAV_ERR_LOCK_SAVE_LOCK,
                              "Attempted to lock path not in repository.");

  /* If the resource's fs path is unreadable, we don't allow a lock to
     be created on it. */
  if (! dav_svn__allow_read_resource(resource, SVN_INVALID_REVNUM,
                                     resource->pool))
    return dav_svn__new_error(resource->pool, HTTP_FORBIDDEN,
                              DAV_ERR_LOCK_SAVE_LOCK,
                              "Path is not accessible.");

  if (lock->next)
    return dav_svn__new_error(resource->pool, HTTP_BAD_REQUEST,
                              DAV_ERR_LOCK_SAVE_LOCK,
                              "Tried to attach multiple locks to a resource.");

  /* RFC2518bis (section 7.4) doesn't require us to support
     'lock-null' resources at all.  Instead, it asks that we treat
     'LOCK nonexistentURL' as a PUT (followed by a LOCK) of a 0-byte file.  */
  if (! resource->exists)
    {
      svn_revnum_t rev, new_rev;
      svn_fs_txn_t *txn;
      svn_fs_root_t *txn_root;
      const char *conflict_msg;
      apr_hash_t *revprop_table = apr_hash_make(resource->pool);
      apr_hash_set(revprop_table, SVN_PROP_REVISION_AUTHOR,
                   APR_HASH_KEY_STRING, svn_string_create(repos->username,
                                                          resource->pool));

      if (resource->info->repos->is_svn_client)
        return dav_svn__new_error(resource->pool, HTTP_METHOD_NOT_ALLOWED,
                                  DAV_ERR_LOCK_SAVE_LOCK,
                                  "Subversion clients may not lock "
                                  "nonexistent paths.");

      else if (! resource->info->repos->autoversioning)
        return dav_svn__new_error(resource->pool, HTTP_METHOD_NOT_ALLOWED,
                                  DAV_ERR_LOCK_SAVE_LOCK,
                                  "Attempted to lock non-existent path; "
                                  "turn on autoversioning first.");

      /* Commit a 0-byte file: */

      if ((serr = svn_fs_youngest_rev(&rev, repos->fs, resource->pool)))
        return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                    "Could not determine youngest revision",
                                    resource->pool);

      if ((serr = svn_repos_fs_begin_txn_for_commit2(&txn, repos->repos, rev,
                                                     revprop_table,
                                                     resource->pool)))
        return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                    "Could not begin a transaction",
                                    resource->pool);

      if ((serr = svn_fs_txn_root(&txn_root, txn, resource->pool)))
        return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                    "Could not begin a transaction",
                                    resource->pool);

      if ((serr = svn_fs_make_file(txn_root, resource->info->repos_path,
                                   resource->pool)))
        return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                    "Could not create empty file.",
                                    resource->pool);

      if ((serr = dav_svn__attach_auto_revprops(txn,
                                                resource->info->repos_path,
                                                resource->pool)))
        return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                    "Could not create empty file.",
                                    resource->pool);

      serr = svn_repos_fs_commit_txn(&conflict_msg, repos->repos,
                                     &new_rev, txn, resource->pool);
      if (SVN_IS_VALID_REVNUM(new_rev))
        {
          /* ### Log an error in post commit FS processing? */
          svn_error_clear(serr);
        }
      else
        {
          svn_error_clear(svn_fs_abort_txn(txn, resource->pool));
          if (serr)
            return dav_svn__convert_err(serr, HTTP_CONFLICT,
                                        apr_psprintf(resource->pool,
                                                     "Conflict when "
                                                     "committing '%s'.",
                                                     conflict_msg),
                                        resource->pool);
          else
            return dav_svn__new_error(resource->pool,
                                      HTTP_INTERNAL_SERVER_ERROR,
                                      0,
                                      "Commit failed but there was no error "
                                      "provided.");
        }
    }

  /* Convert the dav_lock into an svn_lock_t. */
  derr = dav_lock_to_svn_lock(&slock, lock, resource->info->repos_path,
                              info, repos->is_svn_client,
                              resource->pool);
  if (derr)
    return derr;

  /* Now use the svn_lock_t to actually perform the lock. */
  serr = svn_repos_fs_lock(&slock,
                           repos->repos,
                           slock->path,
                           slock->token,
                           slock->comment,
                           slock->is_dav_comment,
                           slock->expiration_date,
                           info->working_revnum,
                           info->lock_steal,
                           resource->pool);

  if (serr && serr->apr_err == SVN_ERR_FS_NO_USER)
    {
      svn_error_clear(serr);
      return dav_svn__new_error(resource->pool, HTTP_UNAUTHORIZED,
                                DAV_ERR_LOCK_SAVE_LOCK,
                                "Anonymous lock creation is not allowed.");
    }
  else if (serr)
    return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                "Failed to create new lock.",
                                resource->pool);


  /* A standard webdav LOCK response doesn't include any information
     about the creation date.  We send it in a custom header, so that
     svn clients can fill in svn_lock_t->creation_date.  A generic DAV
     client should just ignore the header. */
  apr_table_setn(info->r->headers_out, SVN_DAV_CREATIONDATE_HEADER,
                 svn_time_to_cstring(slock->creation_date, resource->pool));

  /* A standard webdav LOCK response doesn't include any information
     about the owner of the lock.  ('DAV:owner' has nothing to do with
     authorization, it's just a comment that we map to
     svn_lock_t->comment.)  We send the owner in a custom header, so
     that svn clients can fill in svn_lock_t->owner.  A generic DAV
     client should just ignore the header. */
  apr_table_setn(info->r->headers_out, SVN_DAV_LOCK_OWNER_HEADER,
                 slock->owner);

  /* Log the locking as a 'high-level' action. */
  dav_svn__operational_log(resource->info,
                           svn_log__lock_one_path(slock->path, info->lock_steal,
                                                  resource->info->r->pool));

  return 0;
}


/*
** Remove any lock that has the specified locktoken.
**
** If locktoken == NULL, then ALL locks are removed.
*/
static dav_error *
remove_lock(dav_lockdb *lockdb,
            const dav_resource *resource,
            const dav_locktoken *locktoken)
{
  dav_lockdb_private *info = lockdb->info;
  svn_error_t *serr;
  svn_lock_t *slock;
  const char *token = NULL;

  /* Sanity check:  if the resource has no associated path in the fs,
     then there's nothing to do.  */
  if (! resource->info->repos_path)
    return 0;

  /* Another easy out: if an svn client sent a 'keep_locks' header
     (typically in a DELETE request, as part of 'svn commit
     --no-unlock'), then ignore dav_method_delete()'s attempt to
     unconditionally remove the lock.  */
  if (info->keep_locks)
    return 0;

  /* If the resource's fs path is unreadable, we don't allow a lock to
     be removed from it. */
  if (! dav_svn__allow_read_resource(resource, SVN_INVALID_REVNUM,
                                     resource->pool))
    return dav_svn__new_error(resource->pool, HTTP_FORBIDDEN,
                              DAV_ERR_LOCK_SAVE_LOCK,
                              "Path is not accessible.");

  if (locktoken == NULL)
    {
      /* Need to manually discover any lock on the resource. */
      serr = svn_fs_get_lock(&slock,
                             resource->info->repos->fs,
                             resource->info->repos_path,
                             resource->pool);
      if (serr)
        return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                    "Failed to check path for a lock.",
                                    resource->pool);
      if (slock)
        token = slock->token;
    }
  else
    {
      token = locktoken->uuid_str;
    }

  if (token)
    {
      /* Notice that a generic DAV client is unable to forcibly
         'break' a lock, because info->lock_break will always be
         FALSE.  An svn client, however, can request a 'forced' break.*/
      serr = svn_repos_fs_unlock(resource->info->repos->repos,
                                 resource->info->repos_path,
                                 token,
                                 info->lock_break,
                                 resource->pool);

      if (serr && serr->apr_err == SVN_ERR_FS_NO_USER)
        {
          svn_error_clear(serr);
          return dav_svn__new_error(resource->pool, HTTP_UNAUTHORIZED,
                                    DAV_ERR_LOCK_SAVE_LOCK,
                                    "Anonymous lock removal is not allowed.");
        }
      else if (serr)
        return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                    "Failed to remove a lock.",
                                    resource->pool);

      /* Log the unlocking as a 'high-level' action. */
      dav_svn__operational_log(resource->info,
                               svn_log__unlock_one_path(
                                   resource->info->repos_path,
                                   info->lock_break,
                                   resource->info->r->pool));
    }

  return 0;
}


/*
** Refresh all locks, found on the specified resource, which has a
** locktoken in the provided list.
**
** If the lock is indirect, then the direct lock is referenced and
** refreshed.
**
** Each lock that is updated is returned in the <locks> argument.
** Note that the locks will be fully resolved.
*/
static dav_error *
refresh_locks(dav_lockdb *lockdb,
              const dav_resource *resource,
              const dav_locktoken_list *ltl,
              time_t new_time,
              dav_lock **locks)
{
  /* We're not looping over a list of locks, since we only support one
     lock per resource. */
  dav_locktoken *token = ltl->locktoken;
  svn_error_t *serr;
  svn_lock_t *slock;
  dav_lock *dlock;

  /* If the resource's fs path is unreadable, we don't want to say
     anything about locks attached to it.*/
  if (! dav_svn__allow_read_resource(resource, SVN_INVALID_REVNUM,
                                     resource->pool))
    return dav_svn__new_error(resource->pool, HTTP_FORBIDDEN,
                              DAV_ERR_LOCK_SAVE_LOCK,
                              "Path is not accessible.");

  /* Convert the path into an svn_lock_t. */
  serr = svn_fs_get_lock(&slock,
                         resource->info->repos->fs,
                         resource->info->repos_path,
                         resource->pool);
  if (serr)
    return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                "Token doesn't point to a lock.",
                                resource->pool);

  /* Sanity check: does the incoming token actually represent the
     current lock on the incoming resource? */
  if ((! slock)
      || (strcmp(token->uuid_str, slock->token) != 0))
    return dav_svn__new_error(resource->pool, HTTP_UNAUTHORIZED,
                              DAV_ERR_LOCK_SAVE_LOCK,
                              "Lock refresh request doesn't match existing "
                              "lock.");

  /* Now use the tweaked svn_lock_t to 'refresh' the existing lock. */
  serr = svn_repos_fs_lock(&slock,
                           resource->info->repos->repos,
                           slock->path,
                           slock->token,
                           slock->comment,
                           slock->is_dav_comment,
                           (new_time == DAV_TIMEOUT_INFINITE)
                             ? 0 : (apr_time_t)new_time * APR_USEC_PER_SEC,
                           SVN_INVALID_REVNUM,
                           TRUE, /* forcibly steal existing lock */
                           resource->pool);

  if (serr && serr->apr_err == SVN_ERR_FS_NO_USER)
    {
      svn_error_clear(serr);
      return dav_svn__new_error(resource->pool, HTTP_UNAUTHORIZED,
                                DAV_ERR_LOCK_SAVE_LOCK,
                                "Anonymous lock refreshing is not allowed.");
    }
  else if (serr)
    return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                "Failed to refresh existing lock.",
                                resource->pool);

  /* Convert the refreshed lock into a dav_lock and return it. */
  svn_lock_to_dav_lock(&dlock, slock, FALSE, resource->exists, resource->pool);
  *locks = dlock;

  return 0;
}


/* The main locking vtable, provided to mod_dav */
const dav_hooks_locks dav_svn__hooks_locks = {
  get_supportedlock,
  parse_locktoken,
  format_locktoken,
  compare_locktoken,
  open_lockdb,
  close_lockdb,
  remove_locknull_state,
  create_lock,
  get_locks,
  find_lock,
  has_locks,
  append_locks,
  remove_lock,
  refresh_locks,
  NULL,
  NULL                          /* hook structure context */
};

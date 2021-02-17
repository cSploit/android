/**
 * @copyright
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
 * @endcopyright
 *
 * @file SVNRepos.cpp
 * @brief Implementation of the class SVNRepos
 */

#include "SVNRepos.h"
#include "CreateJ.h"
#include "ReposNotifyCallback.h"
#include "JNIUtil.h"
#include "svn_error_codes.h"
#include "svn_repos.h"
#include "svn_config.h"
#include "svn_props.h"
#include "svn_pools.h"
#include "svn_path.h"
#include "svn_utf.h"
#include "svn_private_config.h"

SVNRepos::SVNRepos()
    : m_cancelOperation(false)
{
}

SVNRepos::~SVNRepos()
{
}

SVNRepos *SVNRepos::getCppObject(jobject jthis)
{
  static jfieldID fid = 0;
  jlong cppAddr = SVNBase::findCppAddrForJObject(jthis, &fid,
                                                 JAVA_PACKAGE"/SVNRepos");
  return (cppAddr == 0 ? NULL : reinterpret_cast<SVNRepos *>(cppAddr));
}

void SVNRepos::dispose()
{
  static jfieldID fid = 0;
  SVNBase::dispose(&fid, JAVA_PACKAGE"/SVNRepos");
}

void SVNRepos::cancelOperation()
{
  m_cancelOperation = true;
}

svn_error_t *
SVNRepos::checkCancel(void *cancelBaton)
{
  SVNRepos *that = (SVNRepos *)cancelBaton;
  if (that->m_cancelOperation)
    return svn_error_create(SVN_ERR_CANCELLED, NULL,
                            _("Operation cancelled"));
  else
    return SVN_NO_ERROR;
}

void SVNRepos::create(File &path, bool disableFsyncCommits,
                      bool keepLogs, File &configPath,
                      const char *fstype)
{
  SVN::Pool requestPool;
  svn_repos_t *repos;
  apr_hash_t *config;
  apr_hash_t *fs_config = apr_hash_make(requestPool.getPool());

  if (path.isNull())
    {
      JNIUtil::throwNullPointerException("path");
      return;
    }

  apr_hash_set(fs_config, SVN_FS_CONFIG_BDB_TXN_NOSYNC,
               APR_HASH_KEY_STRING,
               (disableFsyncCommits? "1" : "0"));

  apr_hash_set(fs_config, SVN_FS_CONFIG_BDB_LOG_AUTOREMOVE,
               APR_HASH_KEY_STRING,
               (keepLogs ? "0" : "1"));
  apr_hash_set(fs_config, SVN_FS_CONFIG_FS_TYPE,
               APR_HASH_KEY_STRING, fstype);

  SVN_JNI_ERR(svn_config_get_config(&config,
                                    configPath.getInternalStyle(requestPool),
                                    requestPool.getPool()),);
  SVN_JNI_ERR(svn_repos_create(&repos, path.getInternalStyle(requestPool),
                               NULL, NULL, config, fs_config,
                               requestPool.getPool()), );
}

void SVNRepos::deltify(File &path, Revision &revStart, Revision &revEnd)
{
  SVN::Pool requestPool;
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_revnum_t start = SVN_INVALID_REVNUM, end = SVN_INVALID_REVNUM;
  svn_revnum_t youngest, revision;
  SVN::Pool revisionPool;

  if (path.isNull())
    {
      JNIUtil::throwNullPointerException("path");
      return;
    }

  SVN_JNI_ERR(svn_repos_open2(&repos, path.getInternalStyle(requestPool),
                              NULL, requestPool.getPool()), );
  fs = svn_repos_fs(repos);
  SVN_JNI_ERR(svn_fs_youngest_rev(&youngest, fs, requestPool.getPool()), );

  if (revStart.revision()->kind == svn_opt_revision_number)
    /* ### We only handle revision numbers right now, not dates. */
    start = revStart.revision()->value.number;
  else if (revStart.revision()->kind == svn_opt_revision_head)
    start = youngest;
  else
    start = SVN_INVALID_REVNUM;

  if (revEnd.revision()->kind == svn_opt_revision_number)
    end = revEnd.revision()->value.number;
  else if (revEnd.revision()->kind == svn_opt_revision_head)
    end = youngest;
  else
    end = SVN_INVALID_REVNUM;

  /* Fill in implied revisions if necessary. */
  if (start == SVN_INVALID_REVNUM)
    start = youngest;
  if (end == SVN_INVALID_REVNUM)
    end = start;

  if (start > end)
    {
      SVN_JNI_ERR(svn_error_create
                  (SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                   _("First revision cannot be higher than second")), );
    }
  if ((start > youngest) || (end > youngest))
    {
      SVN_JNI_ERR(svn_error_createf
                  (SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                   _("Revisions must not be greater than the youngest revision"
                     " (%ld)"), youngest), );
    }

  /* Loop over the requested revision range, performing the
   * predecessor deltification on paths changed in each. */
  for (revision = start; revision <= end; ++revision)
    {
      revisionPool.clear();
      SVN_JNI_ERR(svn_fs_deltify_revision (fs, revision, revisionPool.getPool()),
                  );
    }

  return;
}

void SVNRepos::dump(File &path, OutputStream &dataOut,
                    Revision &revsionStart, Revision &revisionEnd,
                    bool incremental, bool useDeltas,
                    ReposNotifyCallback *notifyCallback)
{
  SVN::Pool requestPool;
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_revnum_t lower = SVN_INVALID_REVNUM, upper = SVN_INVALID_REVNUM;
  svn_revnum_t youngest;

  if (path.isNull())
    {
      JNIUtil::throwNullPointerException("path");
      return;
    }

  SVN_JNI_ERR(svn_repos_open2(&repos, path.getInternalStyle(requestPool),
                              NULL, requestPool.getPool()), );
  fs = svn_repos_fs(repos);
  SVN_JNI_ERR(svn_fs_youngest_rev(&youngest, fs, requestPool.getPool()), );

  /* ### We only handle revision numbers right now, not dates. */
  if (revsionStart.revision()->kind == svn_opt_revision_number)
    lower = revsionStart.revision()->value.number;
  else if (revsionStart.revision()->kind == svn_opt_revision_head)
    lower = youngest;
  else
    lower = SVN_INVALID_REVNUM;

  if (revisionEnd.revision()->kind == svn_opt_revision_number)
    upper = revisionEnd.revision()->value.number;
  else if (revisionEnd.revision()->kind == svn_opt_revision_head)
    upper = youngest;
  else
    upper = SVN_INVALID_REVNUM;

  /* Fill in implied revisions if necessary. */
  if (lower == SVN_INVALID_REVNUM)
    {
      lower = 0;
      upper = youngest;
    }
  else if (upper == SVN_INVALID_REVNUM)
    {
      upper = lower;
    }

  if (lower > upper)
    {
      SVN_JNI_ERR(svn_error_create
                  (SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                   _("First revision cannot be higher than second")), );
    }
  if ((lower > youngest) || (upper > youngest))
    {
      SVN_JNI_ERR(svn_error_createf
                  (SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                   _("Revisions must not be greater than the youngest revision"
                     " (%ld)"), youngest), );
    }

  SVN_JNI_ERR(svn_repos_dump_fs3(repos, dataOut.getStream(requestPool),
                                 lower, upper, incremental, useDeltas,
                                 notifyCallback != NULL
                                    ? ReposNotifyCallback::notify
                                    : NULL,
                                 notifyCallback,
                                 checkCancel, this, requestPool.getPool()), );
}

void SVNRepos::hotcopy(File &path, File &targetPath,
                       bool cleanLogs)
{
  SVN::Pool requestPool;

  if (path.isNull())
    {
      JNIUtil::throwNullPointerException("path");
      return;
    }

  if (targetPath.isNull())
    {
      JNIUtil::throwNullPointerException("targetPath");
      return;
    }

  SVN_JNI_ERR(svn_repos_hotcopy(path.getInternalStyle(requestPool),
                                targetPath.getInternalStyle(requestPool),
                                cleanLogs, requestPool.getPool()), );
}

static void
list_dblogs (File &path, MessageReceiver &receiver, bool only_unused)
{
  SVN::Pool requestPool;
  apr_array_header_t *logfiles;

  if (path.isNull())
    {
      JNIUtil::throwNullPointerException("path");
      return;
    }

  SVN_JNI_ERR(svn_repos_db_logfiles(&logfiles,
                                    path.getInternalStyle(requestPool),
                                    only_unused, requestPool.getPool()), );

  /* Loop, printing log files.  We append the log paths to the
   * repository path, making sure to return everything to the native
   * style and encoding before printing. */
  for (int i = 0; i < logfiles->nelts; ++i)
    {
      const char *log_utf8;
      log_utf8 = svn_dirent_join(path.getInternalStyle(requestPool),
                                 APR_ARRAY_IDX(logfiles, i, const char *),
                                 requestPool.getPool());
      log_utf8 = svn_dirent_local_style (log_utf8, requestPool.getPool());
      receiver.receiveMessage(log_utf8);
    }
}

void SVNRepos::listDBLogs(File &path, MessageReceiver &messageReceiver)
{
  list_dblogs(path, messageReceiver, false);
}

void SVNRepos::listUnusedDBLogs(File &path,
                                MessageReceiver &messageReceiver)
{
  list_dblogs(path, messageReceiver, true);
}

void SVNRepos::load(File &path,
                    InputStream &dataIn,
                    bool ignoreUUID,
                    bool forceUUID,
                    bool usePreCommitHook,
                    bool usePostCommitHook,
                    const char *relativePath,
                    ReposNotifyCallback *notifyCallback)
{
  SVN::Pool requestPool;
  svn_repos_t *repos;
  enum svn_repos_load_uuid uuid_action = svn_repos_load_uuid_default;
  if (ignoreUUID)
    uuid_action = svn_repos_load_uuid_ignore;
  else if (forceUUID)
    uuid_action = svn_repos_load_uuid_force;

  if (path.isNull())
    {
      JNIUtil::throwNullPointerException("path");
      return;
    }

  SVN_JNI_ERR(svn_repos_open2(&repos, path.getInternalStyle(requestPool),
                              NULL, requestPool.getPool()), );

  SVN_JNI_ERR(svn_repos_load_fs3(repos, dataIn.getStream(requestPool),
                                 uuid_action, relativePath,
                                 usePreCommitHook, usePostCommitHook,
                                 FALSE,
                                 notifyCallback != NULL
                                    ? ReposNotifyCallback::notify
                                    : NULL,
                                 notifyCallback,
                                 checkCancel, this, requestPool.getPool()), );
}

void SVNRepos::lstxns(File &path, MessageReceiver &messageReceiver)
{
  SVN::Pool requestPool;
  svn_repos_t *repos;
  svn_fs_t *fs;
  apr_array_header_t *txns;

  if (path.isNull())
    {
      JNIUtil::throwNullPointerException("path");
      return;
    }

  SVN_JNI_ERR(svn_repos_open2(&repos, path.getInternalStyle(requestPool),
                              NULL, requestPool.getPool()), );
  fs = svn_repos_fs (repos);
  SVN_JNI_ERR(svn_fs_list_transactions(&txns, fs, requestPool.getPool()), );

  /* Loop, printing revisions. */
  for (int i = 0; i < txns->nelts; ++i)
    {
      messageReceiver.receiveMessage(APR_ARRAY_IDX (txns, i, const char *));
    }


}

jlong SVNRepos::recover(File &path, ReposNotifyCallback *notifyCallback)
{
  SVN::Pool requestPool;
  svn_revnum_t youngest_rev;
  svn_repos_t *repos;

  if (path.isNull())
    {
      JNIUtil::throwNullPointerException("path");
      return -1;
    }

  SVN_JNI_ERR(svn_repos_recover4(path.getInternalStyle(requestPool), FALSE,
                                 notifyCallback != NULL
                                    ? ReposNotifyCallback::notify
                                    : NULL,
                                 notifyCallback,
                                 checkCancel, this, requestPool.getPool()),
              -1);

  /* Since db transactions may have been replayed, it's nice to tell
   * people what the latest revision is.  It also proves that the
   * recovery actually worked. */
  SVN_JNI_ERR(svn_repos_open2(&repos, path.getInternalStyle(requestPool),
                              NULL, requestPool.getPool()), -1);
  SVN_JNI_ERR(svn_fs_youngest_rev(&youngest_rev, svn_repos_fs (repos),
                                  requestPool.getPool()),
              -1);
  return youngest_rev;
}

void SVNRepos::rmtxns(File &path, StringArray &transactions)
{
  SVN::Pool requestPool;
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  const apr_array_header_t *args;
  int i;
  SVN::Pool transactionPool;

  if (path.isNull())
    {
      JNIUtil::throwNullPointerException("path");
      return;
    }

  SVN_JNI_ERR(svn_repos_open2(&repos, path.getInternalStyle(requestPool),
                              NULL, requestPool.getPool()), );
  fs = svn_repos_fs (repos);

  args = transactions.array(requestPool);
  /* All the rest of the arguments are transaction names. */
  for (i = 0; i < args->nelts; ++i)
    {
      const char *txn_name = APR_ARRAY_IDX (args, i, const char *);
      svn_error_t *err;

      /* Try to open the txn.  If that succeeds, try to abort it. */
      err = svn_fs_open_txn(&txn, fs, txn_name, transactionPool.getPool());
      if (! err)
        err = svn_fs_abort_txn(txn, transactionPool.getPool());

      /* If either the open or the abort of the txn fails because that
       * transaction is dead, just try to purge the thing.  Else,
       * there was either an error worth reporting, or not error at
       * all.  */
      if (err && (err->apr_err == SVN_ERR_FS_TRANSACTION_DEAD))
        {
          svn_error_clear (err);
          err = svn_fs_purge_txn(fs, txn_name, transactionPool.getPool());
        }

      /* If we had a real from the txn open, abort, or purge, we clear
       * that error and just report to the user that we had an issue
       * with this particular txn. */
      SVN_JNI_ERR(err, );
      transactionPool.clear();
    }

}

void SVNRepos::setRevProp(File &path, Revision &revision,
                          const char *propName, const char *propValue,
                          bool usePreRevPropChangeHook,
                          bool usePostRevPropChangeHook)
{
  SVN::Pool requestPool;
  SVN_JNI_NULL_PTR_EX(propName, "propName", );
  SVN_JNI_NULL_PTR_EX(propValue, "propValue", );
  if (revision.revision()->kind != svn_opt_revision_number)
    {
      SVN_JNI_ERR(svn_error_createf(SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                                    _("Missing revision")), );
    }

  if (path.isNull())
    {
      JNIUtil::throwNullPointerException("path");
      return;
    }

  /* Open the filesystem. */
  svn_repos_t *repos;
  SVN_JNI_ERR(svn_repos_open2(&repos, path.getInternalStyle(requestPool),
                              NULL, requestPool.getPool()), );

  /* If we are bypassing the hooks system, we just hit the filesystem
   * directly. */
  svn_error_t *err;
  svn_string_t *propValStr = svn_string_create(propValue,
                                               requestPool.getPool());
  if (usePreRevPropChangeHook || usePostRevPropChangeHook)
    {
      err = svn_repos_fs_change_rev_prop4(repos,
                                          revision.revision()->value.number,
                                          NULL, propName, NULL, propValStr,
                                          usePreRevPropChangeHook,
                                          usePostRevPropChangeHook, NULL,
                                          NULL, requestPool.getPool());
    }
  else
    {
      svn_fs_t *fs = svn_repos_fs (repos);
      err = svn_fs_change_rev_prop2(fs, revision.revision()->value.number,
                                    propName, NULL, propValStr,
                                    requestPool.getPool());
    }
  SVN_JNI_ERR(err, );
}

/* Set *REVNUM to the revision specified by REVISION (or to
   SVN_INVALID_REVNUM if that has the type 'unspecified'),
   possibly making use of the YOUNGEST revision number in REPOS. */
svn_error_t *
SVNRepos::getRevnum(svn_revnum_t *revnum, const svn_opt_revision_t *revision,
                    svn_revnum_t youngest, svn_repos_t *repos,
                    apr_pool_t *pool)
{
  if (revision->kind == svn_opt_revision_number)
    *revnum = revision->value.number;
  else if (revision->kind == svn_opt_revision_head)
    *revnum = youngest;
  else if (revision->kind == svn_opt_revision_date)
    SVN_ERR(svn_repos_dated_revision
            (revnum, repos, revision->value.date, pool));
  else if (revision->kind == svn_opt_revision_unspecified)
    *revnum = SVN_INVALID_REVNUM;
  else
    return svn_error_create
      (SVN_ERR_INCORRECT_PARAMS, NULL, _("Invalid revision specifier"));

  if (*revnum > youngest)
    return svn_error_createf
      (SVN_ERR_INCORRECT_PARAMS, NULL,
       _("Revisions must not be greater than the youngest revision (%ld)"),
       youngest);

  return SVN_NO_ERROR;
}

void
SVNRepos::verify(File &path, Revision &revisionStart, Revision &revisionEnd,
                 ReposNotifyCallback *notifyCallback)
{
  SVN::Pool requestPool;
  svn_repos_t *repos;
  svn_revnum_t lower = SVN_INVALID_REVNUM, upper = SVN_INVALID_REVNUM;
  svn_revnum_t youngest;

  if (path.isNull())
    {
      JNIUtil::throwNullPointerException("path");
      return;
    }

  /* This whole process is basically just a dump of the repository
   * with no interest in the output. */
  SVN_JNI_ERR(svn_repos_open2(&repos, path.getInternalStyle(requestPool),
                              NULL, requestPool.getPool()), );
  SVN_JNI_ERR(svn_fs_youngest_rev(&youngest, svn_repos_fs (repos),
                                  requestPool.getPool()), );

  /* Find the revision numbers at which to start and end. */
  SVN_JNI_ERR(getRevnum(&lower, revisionStart.revision(),
                        youngest, repos, requestPool.getPool()), );
  SVN_JNI_ERR(getRevnum(&upper, revisionEnd.revision(),
                        youngest, repos, requestPool.getPool()), );

  // Fill in implied revisions if necessary.
  if (lower == SVN_INVALID_REVNUM)
    {
      lower = 0;
      upper = youngest;
    }
  else if (upper == SVN_INVALID_REVNUM)
    {
      upper = lower;
    }

  if (lower > upper)
    SVN_JNI_ERR(svn_error_create
      (SVN_ERR_INCORRECT_PARAMS, NULL,
       _("Start revision cannot be higher than end revision")), );

  SVN_JNI_ERR(svn_repos_verify_fs2(repos, lower, upper,
                                   notifyCallback != NULL
                                    ? ReposNotifyCallback::notify
                                    : NULL,
                                   notifyCallback,
                                   checkCancel, this /* cancel callback/baton */,
                                   requestPool.getPool()), );
}

void SVNRepos::pack(File &path, ReposNotifyCallback *notifyCallback)
{
  SVN::Pool requestPool;
  svn_repos_t *repos;

  if (path.isNull())
    {
      JNIUtil::throwNullPointerException("path");
      return;
    }

  SVN_JNI_ERR(svn_repos_open2(&repos, path.getInternalStyle(requestPool),
                              NULL, requestPool.getPool()), );

  SVN_JNI_ERR(svn_repos_fs_pack2(repos,
                                 notifyCallback != NULL
                                    ? ReposNotifyCallback::notify
                                    : NULL,
                                 notifyCallback,
                                 checkCancel, this,
                                 requestPool.getPool()),
              );
}

void SVNRepos::upgrade(File &path, ReposNotifyCallback *notifyCallback)
{
  SVN::Pool requestPool;

  if (path.isNull())
    {
      JNIUtil::throwNullPointerException("path");
      return;
    }

  SVN_JNI_ERR(svn_repos_upgrade2(path.getInternalStyle(requestPool), FALSE,
                                 notifyCallback != NULL
                                    ? ReposNotifyCallback::notify
                                    : NULL,
                                 notifyCallback,
                                 requestPool.getPool()),
              );
}

jobject SVNRepos::lslocks(File &path, svn_depth_t depth)
{
  SVN::Pool requestPool;
  svn_repos_t *repos;
  apr_hash_t *locks;
  apr_hash_index_t *hi;

  if (path.isNull())
    {
      JNIUtil::throwNullPointerException("path");
      return NULL;
    }

  SVN_JNI_ERR(svn_repos_open2(&repos, path.getInternalStyle(requestPool),
                              NULL, requestPool.getPool()), NULL);
  /* Fetch all locks on or below the root directory. */
  SVN_JNI_ERR(svn_repos_fs_get_locks2(&locks, repos, "/", depth, NULL, NULL,
                                      requestPool.getPool()),
              NULL);

  JNIEnv *env = JNIUtil::getEnv();
  jclass clazz = env->FindClass(JAVA_PACKAGE"/types/Lock");
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  std::vector<jobject> jlocks;

  for (hi = apr_hash_first (requestPool.getPool(), locks);
       hi;
       hi = apr_hash_next (hi))
    {
      void *val;
      apr_hash_this (hi, NULL, NULL, &val);
      svn_lock_t *lock = (svn_lock_t *)val;
      jobject jLock = CreateJ::Lock(lock);

      jlocks.push_back(jLock);
    }

  env->DeleteLocalRef(clazz);

  return CreateJ::Set(jlocks);
}

void SVNRepos::rmlocks(File &path, StringArray &locks)
{
  SVN::Pool requestPool;
  apr_pool_t *pool = requestPool.getPool();
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_fs_access_t *access;

  if (path.isNull())
    {
      JNIUtil::throwNullPointerException("path");
      return;
    }

  SVN_JNI_ERR(svn_repos_open2(&repos, path.getInternalStyle(requestPool),
                              NULL, requestPool.getPool()), );
  fs = svn_repos_fs (repos);
  const char *username;

  /* svn_fs_unlock() demands that some username be associated with the
   * filesystem, so just use the UID of the person running 'svnadmin'.*/
  {
    apr_uid_t uid;
    apr_gid_t gid;
    char *un;
    if (apr_uid_current (&uid, &gid, pool) == APR_SUCCESS &&
        apr_uid_name_get (&un, uid, pool) == APR_SUCCESS)
      {
        svn_error_t *err = svn_utf_cstring_to_utf8(&username, un, pool);
        svn_error_clear (err);
        if (err)
          username = "administrator";
      }
  }

  /* Create an access context describing the current user. */
  SVN_JNI_ERR(svn_fs_create_access(&access, username, pool), );

  /* Attach the access context to the filesystem. */
  SVN_JNI_ERR(svn_fs_set_access(fs, access), );

  SVN::Pool subpool;
  const apr_array_header_t *args = locks.array(requestPool);
  for (int i = 0; i < args->nelts; ++i)
    {
      const char *lock_path = APR_ARRAY_IDX (args, i, const char *);
      svn_lock_t *lock;

      /* Fetch the path's svn_lock_t. */
      svn_error_t *err = svn_fs_get_lock(&lock, fs, lock_path, subpool.getPool());
      if (err)
        goto move_on;
      if (! lock)
        continue;

      /* Now forcibly destroy the lock. */
      err = svn_fs_unlock (fs, lock_path,
                           lock->token, 1 /* force */, subpool.getPool());
      if (err)
        goto move_on;

    move_on:
      svn_error_clear (err);
      subpool.clear();
    }

  return;
}

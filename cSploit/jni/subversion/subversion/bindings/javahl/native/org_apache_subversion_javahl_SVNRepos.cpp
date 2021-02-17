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
 * @file org_apache_subversion_javahl_SVNRepos.cpp
 * @brief Implementation of the native methods in the Java class SVNRepos
 */

#include "../include/org_apache_subversion_javahl_SVNRepos.h"
#include "JNIUtil.h"
#include "JNIStackElement.h"
#include "JNIStringHolder.h"
#include "JNIByteArray.h"
#include "SVNRepos.h"
#include "EnumMapper.h"
#include "Revision.h"
#include "InputStream.h"
#include "OutputStream.h"
#include "MessageReceiver.h"
#include "File.h"
#include "ReposNotifyCallback.h"
#include "svn_props.h"
#include "svn_private_config.h"

JNIEXPORT jlong JNICALL
Java_org_apache_subversion_javahl_SVNRepos_ctNative
(JNIEnv *env, jobject jthis)
{
  JNIEntry(SVNRepos, ctNative);
  SVNRepos *obj = new SVNRepos;
  return obj->getCppAddr();
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNRepos_dispose
(JNIEnv *env, jobject jthis)
{
  JNIEntry(SVNRepos, dispose);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  cl->dispose();
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNRepos_finalize
(JNIEnv *env, jobject jthis)
{
  JNIEntry(SVNRepos, finalize);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl != NULL)
    cl->finalize();
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNRepos_create
(JNIEnv *env, jobject jthis, jobject jpath, jboolean jdisableFsyncCommit,
 jboolean jkeepLog, jobject jconfigpath, jstring jfstype)
{
  JNIEntry(SVNRepos, create);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }

  File path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  File configpath(jconfigpath);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder fstype(jfstype);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->create(path, jdisableFsyncCommit? true : false, jkeepLog? true : false,
             configpath, fstype);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNRepos_deltify
(JNIEnv *env, jobject jthis, jobject jpath, jobject jrevisionStart,
 jobject jrevisionStop)
{
  JNIEntry(SVNRepos, deltify);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }

  File path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revisionStart(jrevisionStart);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revisionEnd(jrevisionStop);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->deltify(path, revisionStart, revisionEnd);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNRepos_dump
(JNIEnv *env, jobject jthis, jobject jpath, jobject jdataout,
 jobject jrevisionStart, jobject jrevisionEnd, jboolean jincremental,
 jboolean juseDeltas, jobject jnotifyCallback)
{
  JNIEntry(SVNRepos, dump);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }

  File path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  OutputStream dataOut(jdataout);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revisionStart(jrevisionStart);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revisionEnd(jrevisionEnd);
  if (JNIUtil::isExceptionThrown())
    return;

  ReposNotifyCallback notifyCallback(jnotifyCallback);

  cl->dump(path, dataOut, revisionStart, revisionEnd,
           jincremental ? true : false, juseDeltas ? true : false,
           jnotifyCallback != NULL ? &notifyCallback : NULL);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNRepos_hotcopy
(JNIEnv *env, jobject jthis, jobject jpath, jobject jtargetPath,
 jboolean jcleanLogs)
{
  JNIEntry(SVNRepos, hotcopy);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }

  File path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  File targetPath(jtargetPath);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->hotcopy(path, targetPath, jcleanLogs ? true : false);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNRepos_listDBLogs
(JNIEnv *env, jobject jthis, jobject jpath, jobject jreceiver)
{
  JNIEntry(SVNRepos, listDBLogs);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }

  File path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  MessageReceiver mr(jreceiver);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->listDBLogs(path, mr);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNRepos_listUnusedDBLogs
(JNIEnv *env, jobject jthis, jobject jpath, jobject jreceiver)
{
  JNIEntry(SVNRepos, listUnusedDBLogs);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }

  File path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  MessageReceiver mr(jreceiver);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->listUnusedDBLogs(path, mr);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNRepos_load
(JNIEnv *env, jobject jthis, jobject jpath, jobject jinputData,
 jboolean jignoreUUID, jboolean jforceUUID, jboolean jusePreCommitHook,
 jboolean jusePostCommitHook, jstring jrelativePath, jobject jnotifyCallback)
{
  JNIEntry(SVNRepos, load);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }

  File path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  InputStream inputData(jinputData);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder relativePath(jrelativePath);
  if (JNIUtil::isExceptionThrown())
    return;

  ReposNotifyCallback notifyCallback(jnotifyCallback);

  cl->load(path, inputData, jignoreUUID ? true : false,
           jforceUUID ? true : false, jusePreCommitHook ? true : false,
           jusePostCommitHook ? true : false, relativePath,
           jnotifyCallback != NULL ? &notifyCallback : NULL);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNRepos_lstxns
(JNIEnv *env, jobject jthis, jobject jpath, jobject jmessageReceiver)
{
  JNIEntry(SVNRepos, lstxns);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }

  File path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  MessageReceiver mr(jmessageReceiver);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->lstxns(path, mr);
}

JNIEXPORT jlong JNICALL
Java_org_apache_subversion_javahl_SVNRepos_recover
(JNIEnv *env, jobject jthis, jobject jpath, jobject jnotifyCallback)
{
  JNIEntry(SVNRepos, recover);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return -1;
    }

  File path(jpath);
  if (JNIUtil::isExceptionThrown())
    return -1;

  ReposNotifyCallback callback(jnotifyCallback);

  return cl->recover(path, jnotifyCallback != NULL ? &callback : NULL);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNRepos_rmtxns
(JNIEnv *env, jobject jthis, jobject jpath, jobjectArray jtransactions)
{
  JNIEntry(SVNRepos, rmtxns);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }

  File path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  StringArray transactions(jtransactions);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->rmtxns(path, transactions);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNRepos_setRevProp
(JNIEnv *env, jobject jthis, jobject jpath, jobject jrevision,
 jstring jpropName, jstring jpropValue, jboolean jusePreRevPropChangeHook,
 jboolean jusePostRevPropChangeHook)
{
  JNIEntry(SVNRepos, setRevProp);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }

  File path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revision(jrevision);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder propName(jpropName);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder propValue(jpropValue);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->setRevProp(path, revision, propName, propValue,
                 jusePreRevPropChangeHook ? true : false,
                 jusePostRevPropChangeHook ? true : false);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNRepos_verify
(JNIEnv *env, jobject jthis, jobject jpath, jobject jrevisionStart,
 jobject jrevisionEnd, jobject jcallback)
{
  JNIEntry(SVNRepos, verify);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }

  File path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revisionStart(jrevisionStart);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revisionEnd(jrevisionEnd);
  if (JNIUtil::isExceptionThrown())
    return;

  ReposNotifyCallback callback(jcallback);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->verify(path, revisionStart, revisionEnd,
             jcallback != NULL ? &callback : NULL);
}

JNIEXPORT jobject JNICALL
Java_org_apache_subversion_javahl_SVNRepos_lslocks
(JNIEnv *env, jobject jthis, jobject jpath, jobject jdepth)
{
  JNIEntry(SVNRepos, lslocks);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return NULL;
    }

  File path(jpath);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  return cl->lslocks(path, EnumMapper::toDepth(jdepth));
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNRepos_rmlocks
(JNIEnv *env, jobject jthis, jobject jpath, jobjectArray jlocks)
{
  JNIEntry(SVNRepos, rmlocks);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }

  File path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  StringArray locks(jlocks);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->rmlocks(path, locks);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNRepos_upgrade
(JNIEnv *env, jobject jthis, jobject jpath, jobject jnotifyCallback)
{
  JNIEntry(SVNRepos, upgrade);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }

  File path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  ReposNotifyCallback callback(jnotifyCallback);

  cl->upgrade(path, jnotifyCallback != NULL ? &callback : NULL);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNRepos_pack
(JNIEnv *env, jobject jthis, jobject jpath, jobject jnotifyCallback)
{
  JNIEntry(SVNRepos, pack);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }

  File path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  ReposNotifyCallback callback(jnotifyCallback);

  cl->pack(path, jnotifyCallback != NULL ? &callback : NULL);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNRepos_cancelOperation
(JNIEnv *env, jobject jthis)
{
  JNIEntry(SVNRepos, cancelOperation);
  SVNRepos *cl = SVNRepos::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError("bad C++ this");
      return;
    }
  cl->cancelOperation();
}

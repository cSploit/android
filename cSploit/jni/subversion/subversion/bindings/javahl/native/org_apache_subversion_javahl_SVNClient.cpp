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
 * @file org_apache_subversion_javahl_SVNClient.cpp
 * @brief Implementation of the native methods in the Java class SVNClient
 */

#include "../include/org_apache_subversion_javahl_SVNClient.h"
#include "JNIUtil.h"
#include "JNIStackElement.h"
#include "JNIStringHolder.h"
#include "JNIByteArray.h"
#include "SVNClient.h"
#include "Revision.h"
#include "RevisionRange.h"
#include "OutputStream.h"
#include "EnumMapper.h"
#include "CommitMessage.h"
#include "Prompter.h"
#include "Targets.h"
#include "CopySources.h"
#include "DiffSummaryReceiver.h"
#include "BlameCallback.h"
#include "ProplistCallback.h"
#include "PatchCallback.h"
#include "CommitCallback.h"
#include "LogMessageCallback.h"
#include "InfoCallback.h"
#include "StatusCallback.h"
#include "ListCallback.h"
#include "ChangelistCallback.h"
#include "StringArray.h"
#include "RevpropTable.h"
#include "svn_version.h"
#include "svn_private_config.h"
#include "version.h"
#include <iostream>

JNIEXPORT jlong JNICALL
Java_org_apache_subversion_javahl_SVNClient_ctNative
(JNIEnv *env, jobject jthis)
{
  JNIEntry(SVNClient, ctNative);
  SVNClient *obj = new SVNClient(jthis);
  return obj->getCppAddr();
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_dispose
(JNIEnv *env, jobject jthis)
{
  JNIEntry(SVNClient, dispose);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  cl->dispose();
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_finalize
(JNIEnv *env, jobject jthis)
{
  JNIEntry(SVNClient, finalize);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl != NULL)
    cl->finalize();
}

JNIEXPORT jstring JNICALL
Java_org_apache_subversion_javahl_SVNClient_getAdminDirectoryName
(JNIEnv *env, jobject jthis)
{
  JNIEntry(SVNClient, getAdminDirectoryName);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return NULL;
    }
  return cl->getAdminDirectoryName();
}

JNIEXPORT jboolean JNICALL
Java_org_apache_subversion_javahl_SVNClient_isAdminDirectory
(JNIEnv *env, jobject jthis, jstring jname)
{
  JNIEntry(SVNClient, isAdminDirectory);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return JNI_FALSE;
    }
  JNIStringHolder name(jname);
  if (JNIUtil::isExceptionThrown())
    return JNI_FALSE;

  return cl->isAdminDirectory(name);
}

JNIEXPORT jstring JNICALL
Java_org_apache_subversion_javahl_SVNClient_getLastPath
(JNIEnv *env, jobject jthis)
{
  JNIEntry(SVNClient, getLastPath);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return NULL;
    }
  const char *ret = cl->getLastPath();
  return JNIUtil::makeJString(ret);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_list
(JNIEnv *env, jobject jthis, jstring jurl, jobject jrevision,
 jobject jpegRevision, jobject jdepth, jint jdirentFields,
 jboolean jfetchLocks, jobject jcallback)
{
  JNIEntry(SVNClient, list);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    return;

  JNIStringHolder url(jurl);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revision(jrevision);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision pegRevision(jpegRevision);
  if (JNIUtil::isExceptionThrown())
    return;

  ListCallback callback(jcallback);
  cl->list(url, revision, pegRevision, EnumMapper::toDepth(jdepth),
           (int)jdirentFields, jfetchLocks ? true : false, &callback);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_status
(JNIEnv *env, jobject jthis, jstring jpath, jobject jdepth,
 jboolean jonServer, jboolean jgetAll, jboolean jnoIgnore,
 jboolean jignoreExternals, jobject jchangelists,
 jobject jstatusCallback)
{
  JNIEntry(SVNClient, status);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    return;

  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  StringArray changelists(jchangelists);
  if (JNIUtil::isExceptionThrown())
    return;

  StatusCallback callback(jstatusCallback);
  cl->status(path, EnumMapper::toDepth(jdepth),
             jonServer ? true:false,
             jgetAll ? true:false, jnoIgnore ? true:false,
             jignoreExternals ? true:false, changelists, &callback);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_username
(JNIEnv *env, jobject jthis, jstring jusername)
{
  JNIEntry(SVNClient, username);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  if (jusername == NULL)
    {
      JNIUtil::raiseThrowable("java/lang/IllegalArgumentException",
                              _("Provide a username (null is not supported)"));
      return;
    }
  JNIStringHolder username(jusername);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->getClientContext().username(username);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_password
(JNIEnv *env, jobject jthis, jstring jpassword)
{
  JNIEntry(SVNClient, password);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  if (jpassword == NULL)
    {
      JNIUtil::raiseThrowable("java/lang/IllegalArgumentException",
                              _("Provide a password (null is not supported)"));
      return;
    }
  JNIStringHolder password(jpassword);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->getClientContext().password(password);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_setPrompt
(JNIEnv *env, jobject jthis, jobject jprompter)
{
  JNIEntry(SVNClient, setPrompt);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  Prompter *prompter = Prompter::makeCPrompter(jprompter);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->getClientContext().setPrompt(prompter);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_logMessages
(JNIEnv *env, jobject jthis, jstring jpath, jobject jpegRevision,
 jobject jranges, jboolean jstopOnCopy, jboolean jdisoverPaths,
 jboolean jincludeMergedRevisions, jobject jrevProps, jlong jlimit,
 jobject jlogMessageCallback)
{
  JNIEntry(SVNClient, logMessages);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  Revision pegRevision(jpegRevision, true);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  LogMessageCallback callback(jlogMessageCallback);

  StringArray revProps(jrevProps);
  if (JNIUtil::isExceptionThrown())
    return;

  // Build the revision range vector from the Java array.
  Array ranges(jranges);
  if (JNIUtil::isExceptionThrown())
    return;

  std::vector<RevisionRange> revisionRanges;
  std::vector<jobject> rangeVec = ranges.vector();

  for (std::vector<jobject>::const_iterator it = rangeVec.begin();
        it < rangeVec.end(); ++it)
    {
      RevisionRange revisionRange(*it);
      if (JNIUtil::isExceptionThrown())
        return;

      revisionRanges.push_back(revisionRange);
    }

  cl->logMessages(path, pegRevision, revisionRanges,
                  jstopOnCopy ? true: false, jdisoverPaths ? true : false,
                  jincludeMergedRevisions ? true : false,
                  revProps, jlimit, &callback);
}

JNIEXPORT jlong JNICALL
Java_org_apache_subversion_javahl_SVNClient_checkout
(JNIEnv *env, jobject jthis, jstring jmoduleName, jstring jdestPath,
 jobject jrevision, jobject jpegRevision, jobject jdepth,
 jboolean jignoreExternals, jboolean jallowUnverObstructions)
{
  JNIEntry(SVNClient, checkout);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return -1;
    }
  Revision revision(jrevision, true);
  if (JNIUtil::isExceptionThrown())
    return -1;

  Revision pegRevision(jpegRevision, true);
  if (JNIUtil::isExceptionThrown())
    return -1;

  JNIStringHolder moduleName(jmoduleName);
  if (JNIUtil::isExceptionThrown())
    return -1;

  JNIStringHolder destPath(jdestPath);
  if (JNIUtil::isExceptionThrown())
    return -1;

  return cl->checkout(moduleName, destPath, revision, pegRevision,
                      EnumMapper::toDepth(jdepth),
                      jignoreExternals ? true : false,
                      jallowUnverObstructions ? true : false);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_remove
(JNIEnv *env, jobject jthis, jobject jtargets, jboolean jforce,
 jboolean keepLocal, jobject jrevpropTable, jobject jmessage,
 jobject jcallback)
{
  JNIEntry(SVNClient, remove);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  SVN::Pool tmpPool;
  StringArray targetsArr(jtargets);
  Targets targets(targetsArr, tmpPool);
  if (JNIUtil::isExceptionThrown())
    return;

  CommitMessage message(jmessage);
  if (JNIUtil::isExceptionThrown())
    return;

  RevpropTable revprops(jrevpropTable);
  if (JNIUtil::isExceptionThrown())
    return;

  CommitCallback callback(jcallback);
  cl->remove(targets, &message, jforce ? true : false,
             keepLocal ? true : false, revprops, jcallback ? &callback : NULL);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_revert
(JNIEnv *env, jobject jthis, jstring jpath, jobject jdepth,
 jobject jchangelists)
{
  JNIEntry(SVNClient, revert);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  StringArray changelists(jchangelists);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->revert(path, EnumMapper::toDepth(jdepth), changelists);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_add
(JNIEnv *env, jobject jthis, jstring jpath, jobject jdepth,
 jboolean jforce, jboolean jnoIgnore, jboolean jaddParents)
{
  JNIEntry(SVNClient, add);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->add(path, EnumMapper::toDepth(jdepth), jforce ? true : false,
          jnoIgnore ? true : false, jaddParents ? true : false);
}

JNIEXPORT jlongArray JNICALL
Java_org_apache_subversion_javahl_SVNClient_update
(JNIEnv *env, jobject jthis, jobject jtargets, jobject jrevision,
 jobject jdepth, jboolean jdepthIsSticky, jboolean jmakeParents,
 jboolean jignoreExternals, jboolean jallowUnverObstructions)
{
  JNIEntry(SVNClient, update);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return NULL;
    }
  Revision revision(jrevision);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  SVN::Pool tmpPool;
  StringArray targetsArr(jtargets);
  Targets targets(targetsArr, tmpPool);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  return cl->update(targets, revision, EnumMapper::toDepth(jdepth),
                    jdepthIsSticky ? true : false,
                    jmakeParents ? true : false,
                    jignoreExternals ? true : false,
                    jallowUnverObstructions ? true : false);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_commit
(JNIEnv *env, jobject jthis, jobject jtargets, jobject jdepth,
 jboolean jnoUnlock, jboolean jkeepChangelist, jobject jchangelists,
 jobject jrevpropTable, jobject jmessage, jobject jcallback)
{
  JNIEntry(SVNClient, commit);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  SVN::Pool tmpPool;
  StringArray targetsArr(jtargets);
  Targets targets(targetsArr, tmpPool);
  if (JNIUtil::isExceptionThrown())
    return;

  CommitMessage message(jmessage);
  if (JNIUtil::isExceptionThrown())
    return;

  // Build the changelist vector from the Java array.
  StringArray changelists(jchangelists);
  if (JNIUtil::isExceptionThrown())
    return;

  RevpropTable revprops(jrevpropTable);
  if (JNIUtil::isExceptionThrown())
    return;

  CommitCallback callback(jcallback);
  cl->commit(targets, &message, EnumMapper::toDepth(jdepth),
             jnoUnlock ? true : false, jkeepChangelist ? true : false,
             changelists, revprops,
             jcallback ? &callback : NULL);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_copy
(JNIEnv *env, jobject jthis, jobject jcopySources, jstring jdestPath,
 jboolean jcopyAsChild, jboolean jmakeParents, jboolean jignoreExternals,
 jobject jrevpropTable, jobject jmessage, jobject jcallback)
{
  JNIEntry(SVNClient, copy);

  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  Array copySrcArray(jcopySources);
  if (JNIUtil::isExceptionThrown())
    return;

  CopySources copySources(copySrcArray);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder destPath(jdestPath);
  if (JNIUtil::isExceptionThrown())
    return;

  CommitMessage message(jmessage);
  if (JNIUtil::isExceptionThrown())
    return;

  RevpropTable revprops(jrevpropTable);
  if (JNIUtil::isExceptionThrown())
    return;

  CommitCallback callback(jcallback);
  cl->copy(copySources, destPath, &message, jcopyAsChild ? true : false,
           jmakeParents ? true : false, jignoreExternals ? true : false,
           revprops, jcallback ? &callback : NULL);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_move
(JNIEnv *env, jobject jthis, jobject jsrcPaths, jstring jdestPath,
 jboolean jforce, jboolean jmoveAsChild, jboolean jmakeParents,
 jobject jrevpropTable, jobject jmessage, jobject jcallback)
{
  JNIEntry(SVNClient, move);

  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  SVN::Pool tmpPool;
  StringArray srcPathArr(jsrcPaths);
  Targets srcPaths(srcPathArr, tmpPool);
  if (JNIUtil::isExceptionThrown())
    return;
  JNIStringHolder destPath(jdestPath);
  if (JNIUtil::isExceptionThrown())
    return;

  CommitMessage message(jmessage);
  if (JNIUtil::isExceptionThrown())
    return;

  RevpropTable revprops(jrevpropTable);
  if (JNIUtil::isExceptionThrown())
    return;

  CommitCallback callback(jcallback);
  cl->move(srcPaths, destPath, &message, jforce ? true : false,
           jmoveAsChild ? true : false, jmakeParents ? true : false,
           revprops, jcallback ? &callback : NULL);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_mkdir
(JNIEnv *env, jobject jthis, jobject jtargets, jboolean jmakeParents,
 jobject jrevpropTable, jobject jmessage, jobject jcallback)
{
  JNIEntry(SVNClient, mkdir);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  SVN::Pool tmpPool;
  StringArray targetsArr(jtargets);
  Targets targets(targetsArr, tmpPool);
  if (JNIUtil::isExceptionThrown())
    return;

  CommitMessage message(jmessage);
  if (JNIUtil::isExceptionThrown())
    return;

  RevpropTable revprops(jrevpropTable);
  if (JNIUtil::isExceptionThrown())
    return;

  CommitCallback callback(jcallback);
  cl->mkdir(targets, &message, jmakeParents ? true : false, revprops,
            jcallback ? &callback : NULL);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_cleanup
(JNIEnv *env, jobject jthis, jstring jpath)
{
  JNIEntry(SVNClient, cleanup);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->cleanup(path);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_resolve
(JNIEnv *env, jobject jthis, jstring jpath, jobject jdepth, jobject jchoice)
{
  JNIEntry(SVNClient, resolve);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->resolve(path, EnumMapper::toDepth(jdepth),
              EnumMapper::toConflictChoice(jchoice));
}

JNIEXPORT jlong JNICALL
Java_org_apache_subversion_javahl_SVNClient_doExport
(JNIEnv *env, jobject jthis, jstring jsrcPath, jstring jdestPath,
 jobject jrevision, jobject jpegRevision, jboolean jforce,
 jboolean jignoreExternals, jobject jdepth, jstring jnativeEOL)
{
  JNIEntry(SVNClient, doExport);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return -1;
    }
  Revision revision(jrevision);
  if (JNIUtil::isExceptionThrown())
    return -1;

  Revision pegRevision(jpegRevision);
  if (JNIUtil::isExceptionThrown())
    return -1;

  JNIStringHolder srcPath(jsrcPath);
  if (JNIUtil::isExceptionThrown())
    return -1;

  JNIStringHolder destPath(jdestPath);
  if (JNIUtil::isExceptionThrown())
    return -1;

  JNIStringHolder nativeEOL(jnativeEOL);
  if (JNIUtil::isExceptionThrown())
    return -1;

  return cl->doExport(srcPath, destPath, revision, pegRevision,
                      jforce ? true : false, jignoreExternals ? true : false,
                      EnumMapper::toDepth(jdepth), nativeEOL);
}

JNIEXPORT jlong JNICALL
Java_org_apache_subversion_javahl_SVNClient_doSwitch
(JNIEnv *env, jobject jthis, jstring jpath, jstring jurl, jobject jrevision,
 jobject jPegRevision, jobject jdepth, jboolean jdepthIsSticky,
 jboolean jignoreExternals, jboolean jallowUnverObstructions,
 jboolean jignoreAncestry)
{
  JNIEntry(SVNClient, doSwitch);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return -1;
    }
  Revision revision(jrevision);
  if (JNIUtil::isExceptionThrown())
    return -1;

  Revision pegRevision(jPegRevision);
  if (JNIUtil::isExceptionThrown())
    return -1;

  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return -1;

  JNIStringHolder url(jurl);
  if (JNIUtil::isExceptionThrown())
    return -1;

  return cl->doSwitch(path, url, revision, pegRevision,
                      EnumMapper::toDepth(jdepth),
                      jdepthIsSticky ? true : false,
                      jignoreExternals ? true : false,
                      jallowUnverObstructions ? true : false,
                      jignoreAncestry ? true : false);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_doImport
(JNIEnv *env, jobject jthis, jstring jpath, jstring jurl, jobject jdepth,
 jboolean jnoIgnore, jboolean jignoreUnknownNodeTypes, jobject jrevpropTable,
 jobject jmessage, jobject jcallback)
{
  JNIEntry(SVNClient, doImport);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder url(jurl);
  if (JNIUtil::isExceptionThrown())
    return;

  CommitMessage message(jmessage);
  if (JNIUtil::isExceptionThrown())
    return;

  RevpropTable revprops(jrevpropTable);
  if (JNIUtil::isExceptionThrown())
    return;

  CommitCallback callback(jcallback);
  cl->doImport(path, url, &message, EnumMapper::toDepth(jdepth),
               jnoIgnore ? true : false,
               jignoreUnknownNodeTypes ? true : false, revprops,
               jcallback ? &callback : NULL);
}

JNIEXPORT jobject JNICALL
Java_org_apache_subversion_javahl_SVNClient_suggestMergeSources
(JNIEnv *env, jobject jthis, jstring jpath, jobject jpegRevision)
{
  JNIEntry(SVNClient, suggestMergeSources);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return NULL;
    }

  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  Revision pegRevision(jpegRevision);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  return cl->suggestMergeSources(path, pegRevision);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_merge__Ljava_lang_String_2Lorg_apache_subversion_javahl_types_Revision_2Ljava_lang_String_2Lorg_apache_subversion_javahl_types_Revision_2Ljava_lang_String_2ZLorg_apache_subversion_javahl_types_Depth_2ZZZ
(JNIEnv *env, jobject jthis, jstring jpath1, jobject jrevision1,
 jstring jpath2, jobject jrevision2, jstring jlocalPath, jboolean jforce,
 jobject jdepth, jboolean jignoreAncestry, jboolean jdryRun,
 jboolean jrecordOnly)
{
  JNIEntry(SVNClient, merge);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  Revision revision1(jrevision1);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder path1(jpath1);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revision2(jrevision2);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder path2(jpath2);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder localPath(jlocalPath);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->merge(path1, revision1, path2, revision2, localPath,
            jforce ? true:false, EnumMapper::toDepth(jdepth),
            jignoreAncestry ? true:false, jdryRun ? true:false,
            jrecordOnly ? true:false);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_merge__Ljava_lang_String_2Lorg_apache_subversion_javahl_types_Revision_2Ljava_util_List_2Ljava_lang_String_2ZLorg_apache_subversion_javahl_types_Depth_2ZZZ
(JNIEnv *env, jobject jthis, jstring jpath, jobject jpegRevision,
 jobject jranges, jstring jlocalPath, jboolean jforce, jobject jdepth,
 jboolean jignoreAncestry, jboolean jdryRun, jboolean jrecordOnly)
{
  JNIEntry(SVNClient, merge);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }

  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision pegRevision(jpegRevision);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder localPath(jlocalPath);
  if (JNIUtil::isExceptionThrown())
    return;

  // Build the revision range vector from the Java array.
  Array ranges(jranges);
  if (JNIUtil::isExceptionThrown())
    return;

  std::vector<RevisionRange> revisionRanges;
  std::vector<jobject> rangeVec = ranges.vector();

  for (std::vector<jobject>::const_iterator it = rangeVec.begin();
        it < rangeVec.end(); ++it)
    {
      RevisionRange revisionRange(*it);
      if (JNIUtil::isExceptionThrown())
        return;

      revisionRanges.push_back(revisionRange);
    }

  cl->merge(path, pegRevision, revisionRanges, localPath,
            jforce ? true:false, EnumMapper::toDepth(jdepth),
            jignoreAncestry ? true:false, jdryRun ? true:false,
            jrecordOnly ? true:false);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_mergeReintegrate
(JNIEnv *env, jobject jthis, jstring jpath, jobject jpegRevision,
 jstring jlocalPath, jboolean jdryRun)
{
  JNIEntry(SVNClient, mergeReintegrate);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }

  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision pegRevision(jpegRevision);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder localPath(jlocalPath);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->mergeReintegrate(path, pegRevision, localPath,
                       jdryRun ? true:false);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_properties
(JNIEnv *env, jobject jthis, jstring jpath, jobject jrevision,
 jobject jpegRevision, jobject jdepth, jobject jchangelists,
 jobject jproplistCallback)
{
  JNIEntry(SVNClient, properties);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revision(jrevision);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision pegRevision(jpegRevision);
  if (JNIUtil::isExceptionThrown())
    return;

  StringArray changelists(jchangelists);
  if (JNIUtil::isExceptionThrown())
    return;

  ProplistCallback callback(jproplistCallback);
  cl->properties(path, revision, pegRevision, EnumMapper::toDepth(jdepth),
                 changelists, &callback);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_propertySetRemote
(JNIEnv *env, jobject jthis, jstring jpath, jlong jbaseRev, jstring jname,
 jbyteArray jvalue, jobject jmessage, jboolean jforce, jobject jrevpropTable,
 jobject jcallback)
{
  JNIEntry(SVNClient, propertySet);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder name(jname);
  if (JNIUtil::isExceptionThrown())
    return;

  CommitMessage message(jmessage);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIByteArray value(jvalue);
  if (JNIUtil::isExceptionThrown())
    return;

  RevpropTable revprops(jrevpropTable);
  if (JNIUtil::isExceptionThrown())
    return;

  CommitCallback callback(jcallback);
  cl->propertySetRemote(path, jbaseRev, name, &message, value,
                        jforce ? true:false,
                        revprops, jcallback ? &callback : NULL);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_propertySetLocal
(JNIEnv *env, jobject jthis, jobject jtargets, jstring jname,
 jbyteArray jvalue, jobject jdepth, jobject jchangelists, jboolean jforce)
{
  JNIEntry(SVNClient, propertySet);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  SVN::Pool tmpPool;
  StringArray targetsArr(jtargets);
  Targets targets(targetsArr, tmpPool);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder name(jname);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIByteArray value(jvalue);
  if (JNIUtil::isExceptionThrown())
    return;

  StringArray changelists(jchangelists);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->propertySetLocal(targets, name, value, EnumMapper::toDepth(jdepth),
                       changelists, jforce ? true:false);
}

JNIEXPORT jbyteArray JNICALL
Java_org_apache_subversion_javahl_SVNClient_revProperty
(JNIEnv *env, jobject jthis, jstring jpath, jstring jname, jobject jrevision)
{
  JNIEntry(SVNClient, revProperty);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return NULL;
    }
  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  JNIStringHolder name(jname);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  Revision revision(jrevision);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  return cl->revProperty(path, name, revision);
}

JNIEXPORT jobject JNICALL
Java_org_apache_subversion_javahl_SVNClient_revProperties
(JNIEnv *env, jobject jthis, jstring jpath, jobject jrevision)
{
  JNIEntry(SVNClient, revProperty);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return NULL;
    }
  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  Revision revision(jrevision);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  return cl->revProperties(path, revision);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_setRevProperty
(JNIEnv *env, jobject jthis, jstring jpath, jstring jname, jobject jrevision,
 jstring jvalue, jstring joriginalValue, jboolean jforce)
{
  JNIEntry(SVNClient, setRevProperty);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder name(jname);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revision(jrevision);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder value(jvalue);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder original_value(joriginalValue);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->setRevProperty(path, name, revision, value, original_value,
                     jforce ? true: false);
}

JNIEXPORT jbyteArray JNICALL
Java_org_apache_subversion_javahl_SVNClient_propertyGet
(JNIEnv *env, jobject jthis, jstring jpath, jstring jname, jobject jrevision,
 jobject jpegRevision)
{
  JNIEntry(SVNClient, propertyGet);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return NULL;
    }
  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  JNIStringHolder name(jname);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  Revision revision(jrevision);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  Revision pegRevision(jpegRevision);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  return cl->propertyGet(path, name, revision, pegRevision);
}

JNIEXPORT jobject JNICALL
Java_org_apache_subversion_javahl_SVNClient_getMergeinfo
(JNIEnv *env, jobject jthis, jstring jtarget, jobject jpegRevision)
{
  JNIEntry(SVNClient, getMergeinfo);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return NULL;
    }
  JNIStringHolder target(jtarget);
  if (JNIUtil::isExceptionThrown())
    return NULL;
  Revision pegRevision(jpegRevision);
  if (JNIUtil::isExceptionThrown())
    return NULL;
  return cl->getMergeinfo(target, pegRevision);
}

JNIEXPORT void JNICALL Java_org_apache_subversion_javahl_SVNClient_getMergeinfoLog
(JNIEnv *env, jobject jthis, jobject jkind, jstring jpathOrUrl,
 jobject jpegRevision, jstring jmergeSourceUrl, jobject jsrcPegRevision,
 jboolean jdiscoverChangedPaths, jobject jdepth, jobject jrevProps,
 jobject jlogMessageCallback)
{
  JNIEntry(SVNClient, getMergeinfoLog);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }

  Revision pegRevision(jpegRevision, true);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision srcPegRevision(jsrcPegRevision, true);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder pathOrUrl(jpathOrUrl);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder mergeSourceUrl(jmergeSourceUrl);
  if (JNIUtil::isExceptionThrown())
    return;

  StringArray revProps(jrevProps);
  if (JNIUtil::isExceptionThrown())
    return;

  LogMessageCallback callback(jlogMessageCallback);

  cl->getMergeinfoLog(EnumMapper::toMergeinfoLogKind(jkind),
                      pathOrUrl, pegRevision, mergeSourceUrl,
                      srcPegRevision, jdiscoverChangedPaths ? true : false,
                      EnumMapper::toDepth(jdepth), revProps, &callback);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_diff__Ljava_lang_String_2Lorg_apache_subversion_javahl_types_Revision_2Ljava_lang_String_2Lorg_apache_subversion_javahl_types_Revision_2Ljava_lang_String_2Ljava_lang_String_2Lorg_apache_subversion_javahl_types_Depth_2Ljava_util_Collection_2ZZZZ
(JNIEnv *env, jobject jthis, jstring jtarget1, jobject jrevision1,
 jstring jtarget2, jobject jrevision2, jstring jrelativeToDir,
 jstring joutfileName, jobject jdepth, jobject jchangelists,
 jboolean jignoreAncestry, jboolean jnoDiffDeleted, jboolean jforce,
 jboolean jcopiesAsAdds)
{
  JNIEntry(SVNClient, diff);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  JNIStringHolder target1(jtarget1);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revision1(jrevision1);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder target2(jtarget2);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revision2(jrevision2);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder relativeToDir(jrelativeToDir);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder outfileName(joutfileName);
  if (JNIUtil::isExceptionThrown())
    return;

  StringArray changelists(jchangelists);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->diff(target1, revision1, target2, revision2, relativeToDir, outfileName,
           EnumMapper::toDepth(jdepth), changelists,
           jignoreAncestry ? true:false,
           jnoDiffDeleted ? true:false, jforce ? true:false,
           jcopiesAsAdds ? true:false);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_diff__Ljava_lang_String_2Lorg_apache_subversion_javahl_types_Revision_2Lorg_apache_subversion_javahl_types_Revision_2Lorg_apache_subversion_javahl_types_Revision_2Ljava_lang_String_2Ljava_lang_String_2Lorg_apache_subversion_javahl_types_Depth_2Ljava_util_Collection_2ZZZZ
(JNIEnv *env, jobject jthis, jstring jtarget, jobject jpegRevision,
 jobject jstartRevision, jobject jendRevision, jstring jrelativeToDir,
 jstring joutfileName, jobject jdepth, jobject jchangelists,
 jboolean jignoreAncestry, jboolean jnoDiffDeleted, jboolean jforce,
 jboolean jcopiesAsAdds)
{
  JNIEntry(SVNClient, diff);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  JNIStringHolder target(jtarget);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision pegRevision(jpegRevision);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision startRevision(jstartRevision);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision endRevision(jendRevision);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder outfileName(joutfileName);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder relativeToDir(jrelativeToDir);
  if (JNIUtil::isExceptionThrown())
    return;

  StringArray changelists(jchangelists);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->diff(target, pegRevision, startRevision, endRevision, relativeToDir,
           outfileName, EnumMapper::toDepth(jdepth), changelists,
           jignoreAncestry ? true:false,
           jnoDiffDeleted ? true:false, jforce ? true:false,
           jcopiesAsAdds ? true:false);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_diffSummarize__Ljava_lang_String_2Lorg_apache_subversion_javahl_types_Revision_2Ljava_lang_String_2Lorg_apache_subversion_javahl_types_Revision_2Lorg_apache_subversion_javahl_types_Depth_2Ljava_util_Collection_2ZLorg_apache_subversion_javahl_callback_DiffSummaryCallback_2
(JNIEnv *env, jobject jthis, jstring jtarget1, jobject jrevision1,
 jstring jtarget2, jobject jrevision2, jobject jdepth, jobject jchangelists,
 jboolean jignoreAncestry, jobject jdiffSummaryReceiver)
{
  JNIEntry(SVNClient, diffSummarize);

  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  JNIStringHolder target1(jtarget1);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revision1(jrevision1);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder target2(jtarget2);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revision2(jrevision2);
  if (JNIUtil::isExceptionThrown())
    return;

  DiffSummaryReceiver receiver(jdiffSummaryReceiver);
  if (JNIUtil::isExceptionThrown())
    return;

  StringArray changelists(jchangelists);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->diffSummarize(target1, revision1, target2, revision2,
                    EnumMapper::toDepth(jdepth), changelists,
                    jignoreAncestry ? true: false, receiver);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_diffSummarize__Ljava_lang_String_2Lorg_apache_subversion_javahl_types_Revision_2Lorg_apache_subversion_javahl_types_Revision_2Lorg_apache_subversion_javahl_types_Revision_2Lorg_apache_subversion_javahl_types_Depth_2Ljava_util_Collection_2ZLorg_apache_subversion_javahl_callback_DiffSummaryCallback_2
(JNIEnv *env, jobject jthis, jstring jtarget, jobject jPegRevision,
 jobject jStartRevision, jobject jEndRevision, jobject jdepth,
 jobject jchangelists, jboolean jignoreAncestry,
 jobject jdiffSummaryReceiver)
{
  JNIEntry(SVNClient, diffSummarize);

  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  JNIStringHolder target(jtarget);
  if (JNIUtil::isExceptionThrown())
    return;
  Revision pegRevision(jPegRevision);
  if (JNIUtil::isExceptionThrown())
    return;
  Revision startRevision(jStartRevision);
  if (JNIUtil::isExceptionThrown())
    return;
  Revision endRevision(jEndRevision);
  if (JNIUtil::isExceptionThrown())
    return;
  DiffSummaryReceiver receiver(jdiffSummaryReceiver);
  if (JNIUtil::isExceptionThrown())
    return;

  StringArray changelists(jchangelists);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->diffSummarize(target, pegRevision, startRevision, endRevision,
                    EnumMapper::toDepth(jdepth), changelists,
                    jignoreAncestry ? true : false, receiver);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_streamFileContent
(JNIEnv *env, jobject jthis, jstring jpath, jobject jrevision,
 jobject jpegRevision, jobject jstream)
{
  JNIEntry(SVNClient, streamFileContent);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revision(jrevision);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision pegRevision(jpegRevision);
  if (JNIUtil::isExceptionThrown())
    return;

  OutputStream dataOut(jstream);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->streamFileContent(path, revision, pegRevision, dataOut);
}

JNIEXPORT jstring JNICALL
Java_org_apache_subversion_javahl_SVNClient_getVersionInfo
(JNIEnv *env, jobject jthis, jstring jpath, jstring jtrailUrl,
 jboolean jlastChanged)
{
  JNIEntry(SVNClient, getVersionInfo);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return NULL;
    }
  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  JNIStringHolder trailUrl(jtrailUrl);
  return cl->getVersionInfo(path, trailUrl, jlastChanged ? true:false);
}

JNIEXPORT void JNICALL Java_org_apache_subversion_javahl_SVNClient_upgrade
  (JNIEnv *env, jobject jthis, jstring jpath)
{
  JNIEntry(SVNClient, upgrade);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->upgrade(path);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_enableLogging
(JNIEnv *env, jclass jclazz, jobject jlogLevel, jstring jpath)
{
  JNIEntryStatic(SVNClient, enableLogging);
  JNIUtil::initLogFile(EnumMapper::toLogLevel(jlogLevel), jpath);

}

JNIEXPORT jstring JNICALL
Java_org_apache_subversion_javahl_SVNClient_version
(JNIEnv *env, jclass jclazz)
{
  JNIEntryStatic(SVNClient, version);
  const char *version = "svn:" SVN_VERSION "\njni:" JNI_VERSION;
  return JNIUtil::makeJString(version);
}

JNIEXPORT jint JNICALL
Java_org_apache_subversion_javahl_SVNClient_versionMajor
(JNIEnv *env, jclass jclazz)
{
  JNIEntryStatic(SVNClient, versionMajor);
  return JNI_VER_MAJOR;
}

JNIEXPORT jint JNICALL
Java_org_apache_subversion_javahl_SVNClient_versionMinor
(JNIEnv *env, jclass jclazz)
{
  JNIEntryStatic(SVNClient, versionMinor);
  return JNI_VER_MINOR;
}

JNIEXPORT jint JNICALL
Java_org_apache_subversion_javahl_SVNClient_versionMicro
(JNIEnv *env, jclass jclazz)
{
  JNIEntryStatic(SVNClient, versionMicro);
  return JNI_VER_MICRO;
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_relocate
(JNIEnv *env, jobject jthis, jstring jfrom, jstring jto, jstring jpath,
 jboolean jignoreExternals)
{
  JNIEntry(SVNClient, relocate);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  JNIStringHolder from(jfrom);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder to(jto);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->relocate(from, to, path, jignoreExternals ? true : false);
  return;
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_blame
(JNIEnv *env, jobject jthis, jstring jpath, jobject jpegRevision,
 jobject jrevisionStart, jobject jrevisionEnd, jboolean jignoreMimeType,
 jboolean jincludeMergedRevisions, jobject jblameCallback)
{
  JNIEntry(SVNClient, blame);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }
  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision pegRevision(jpegRevision, false, true);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revisionStart(jrevisionStart, false, true);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revisionEnd(jrevisionEnd, true);
  if (JNIUtil::isExceptionThrown())
    return;

  BlameCallback callback(jblameCallback);
  cl->blame(path, pegRevision, revisionStart, revisionEnd,
            jignoreMimeType ? true : false,
            jincludeMergedRevisions ? true : false, &callback);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_setConfigDirectory
(JNIEnv *env, jobject jthis, jstring jconfigDir)
{
  JNIEntry(SVNClient, setConfigDirectory);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return;
    }

  JNIStringHolder configDir(jconfigDir);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->getClientContext().setConfigDirectory(configDir);
}

JNIEXPORT jstring JNICALL
Java_org_apache_subversion_javahl_SVNClient_getConfigDirectory
(JNIEnv *env, jobject jthis)
{
  JNIEntry(SVNClient, getConfigDirectory);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError(_("bad C++ this"));
      return NULL;
    }

  const char *configDir = cl->getClientContext().getConfigDirectory();
  return JNIUtil::makeJString(configDir);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_cancelOperation
(JNIEnv *env, jobject jthis)
{
  JNIEntry(SVNClient, cancelOperation);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError("bad C++ this");
      return;
    }
  cl->getClientContext().cancelOperation();
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_addToChangelist
(JNIEnv *env, jobject jthis, jobject jtargets, jstring jchangelist,
 jobject jdepth, jobject jchangelists)
{
  JNIEntry(SVNClient, addToChangelist);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError("bad C++ this");
      return;
    }
  SVN::Pool tmpPool;
  StringArray targetsArr(jtargets);
  Targets targets(targetsArr, tmpPool);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder changelist_name(jchangelist);
  if (JNIUtil::isExceptionThrown())
    return;

  StringArray changelists(jchangelists);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->addToChangelist(targets, changelist_name, EnumMapper::toDepth(jdepth),
                      changelists);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_removeFromChangelists
(JNIEnv *env, jobject jthis, jobject jtargets, jobject jdepth,
 jobject jchangelists)
{
  JNIEntry(SVNClient, removeFromChangelist);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError("bad C++ this");
      return;
    }
  SVN::Pool tmpPool;
  StringArray targetsArr(jtargets);
  Targets targets(targetsArr, tmpPool);
  if (JNIUtil::isExceptionThrown())
    return;

  StringArray changelists(jchangelists);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->removeFromChangelists(targets, EnumMapper::toDepth(jdepth), changelists);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_getChangelists
(JNIEnv *env, jobject jthis, jstring jroot_path, jobject jchangelists,
 jobject jdepth, jobject jchangelistCallback)
{
  JNIEntry(SVNClient, getChangelist);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError("bad C++ this");
      return;
    }

  JNIStringHolder root_path(jroot_path);
  if (JNIUtil::isExceptionThrown())
    return;

  StringArray changelists(jchangelists);
  if (JNIUtil::isExceptionThrown())
    return;

  ChangelistCallback callback(jchangelistCallback);
  cl->getChangelists(root_path, changelists, EnumMapper::toDepth(jdepth),
                     &callback);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_lock
(JNIEnv *env, jobject jthis, jobject jtargets, jstring jcomment,
 jboolean jforce)
{
  JNIEntry(SVNClient, lock);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError("bad C++ this");
      return;
    }
  SVN::Pool tmpPool;
  StringArray targetsArr(jtargets);
  Targets targets(targetsArr, tmpPool);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder comment(jcomment);
  if (JNIUtil::isExceptionThrown())
    return;

  cl->lock(targets, comment, jforce ? true : false);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_unlock
(JNIEnv *env, jobject jthis, jobject jtargets, jboolean jforce)
{
  JNIEntry(SVNClient, unlock);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError("bad C++ this");
      return;
    }

  SVN::Pool tmpPool;
  StringArray targetsArr(jtargets);
  Targets targets(targetsArr, tmpPool);
  if (JNIUtil::isExceptionThrown())
    return;


  cl->unlock(targets, jforce ? true : false);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_info2
(JNIEnv *env, jobject jthis, jstring jpath, jobject jrevision,
 jobject jpegRevision, jobject jdepth, jobject jchangelists,
 jobject jinfoCallback)
{
  JNIEntry(SVNClient, info2);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError("bad C++ this");
      return;
    }
  JNIStringHolder path(jpath);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision revision(jrevision);
  if (JNIUtil::isExceptionThrown())
    return;

  Revision pegRevision(jpegRevision);
  if (JNIUtil::isExceptionThrown())
    return;

  StringArray changelists(jchangelists);
  if (JNIUtil::isExceptionThrown())
    return;

  InfoCallback callback(jinfoCallback);
  cl->info2(path, revision, pegRevision, EnumMapper::toDepth(jdepth),
            changelists, &callback);
}

JNIEXPORT void JNICALL
Java_org_apache_subversion_javahl_SVNClient_patch
(JNIEnv *env, jobject jthis, jstring jpatchPath, jstring jtargetPath,
 jboolean jdryRun, jint jstripCount, jboolean jreverse,
 jboolean jignoreWhitespace, jboolean jremoveTempfiles, jobject jcallback)
{
  JNIEntry(SVNClient, patch);
  SVNClient *cl = SVNClient::getCppObject(jthis);
  if (cl == NULL)
    {
      JNIUtil::throwError("bad C++ this");
      return;
    }

  JNIStringHolder patchPath(jpatchPath);
  if (JNIUtil::isExceptionThrown())
    return;

  JNIStringHolder targetPath(jtargetPath);
  if (JNIUtil::isExceptionThrown())
    return;

  PatchCallback callback(jcallback);
  cl->patch(patchPath, targetPath, jdryRun ? true : false, (int) jstripCount,
            jreverse ? true : false, jignoreWhitespace ? true : false,
            jremoveTempfiles ? true : false, &callback);
}

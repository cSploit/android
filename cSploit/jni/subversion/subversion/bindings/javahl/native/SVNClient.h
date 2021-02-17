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
 * @file SVNClient.h
 * @brief Interface for the SVNClient class
 */

#ifndef SVNCLIENT_H
#define SVNCLIENT_H

#include <vector>
#include <string>
#include <jni.h>
#include "Path.h"
#include "ClientContext.h"

class Revision;
class RevisionRange;
class Targets;
class JNIByteArray;
class Prompter;
class CopySources;
class DiffSummaryReceiver;
class BlameCallback;
class ProplistCallback;
class LogMessageCallback;
class InfoCallback;
class CommitCallback;
class ListCallback;
class StatusCallback;
class OutputStream;
class PatchCallback;
class ChangelistCallback;
class CommitMessage;
class StringArray;
class RevpropTable;
#include "svn_types.h"
#include "svn_client.h"
#include "SVNBase.h"

class SVNClient :public SVNBase
{
 public:
  void patch(const char *patchPath, const char *targetPath, bool dryRun,
             int stripCount, bool reverse, bool ignoreWhitespace,
             bool removeTempfiles, PatchCallback *callback);
  void info2(const char *path, Revision &revision, Revision &pegRevision,
             svn_depth_t depth, StringArray &changelists,
             InfoCallback *callback);
  void unlock(Targets &targets, bool force);
  void lock(Targets &targets, const char *comment, bool force);
  jobject revProperties(const char *path, Revision &revision);

  void blame(const char *path, Revision &pegRevision,
             Revision &revisionStart, Revision &revisionEnd,
             bool ignoreMimeType, bool includeMergedRevisions,
             BlameCallback *callback);
  void relocate(const char *from, const char *to, const char *path,
                bool ignoreExternals);
  void streamFileContent(const char *path, Revision &revision,
                         Revision &pegRevision, OutputStream &outputStream);
  void propertySetLocal(Targets &targets, const char *name, JNIByteArray &value,
                        svn_depth_t depth, StringArray &changelists,
                        bool force);
  void propertySetRemote(const char *path, long base_rev, const char *name,
                         CommitMessage *message,
                         JNIByteArray &value, bool force,
                         RevpropTable &revprops, CommitCallback *callback);
  void properties(const char *path, Revision &revision,
                  Revision &pegRevision, svn_depth_t depth,
                  StringArray &changelists, ProplistCallback *callback);
  jobject getMergeinfo(const char *target, Revision &pegRevision);
  void getMergeinfoLog(int type, const char *pathOrURL,
                       Revision &pegRevision, const char *mergeSourceURL,
                       Revision &srcPegRevision, bool discoverChangedPaths,
                       svn_depth_t depth, StringArray &revProps,
                       LogMessageCallback *callback);
  jobject suggestMergeSources(const char *path, Revision &pegRevision);
  void merge(const char *path1, Revision &revision1, const char *path2,
             Revision &revision2, const char *localPath, bool force,
             svn_depth_t depth, bool ignoreAncestry, bool dryRun,
             bool recordOnly);
  void merge(const char *path, Revision &pegRevision,
             std::vector<RevisionRange> &rangesToMerge,
             const char *localPath, bool force, svn_depth_t depth,
             bool ignoreAncestry, bool dryRun, bool recordOnly);
  void mergeReintegrate(const char *path, Revision &pegRevision,
                        const char *localPath, bool dryRun);
  void doImport(const char *path, const char *url, CommitMessage *message,
                svn_depth_t depth, bool noIgnore, bool ignoreUnknownNodeTypes,
                RevpropTable &revprops, CommitCallback *callback);
  jlong doSwitch(const char *path, const char *url, Revision &revision,
                 Revision &pegRevision, svn_depth_t depth,
                 bool depthIsSticky, bool ignoreExternals,
                 bool allowUnverObstructions, bool ignoreAncestry);
  jlong doExport(const char *srcPath, const char *destPath,
                 Revision &revision, Revision &pegRevision, bool force,
                 bool ignoreExternals, svn_depth_t depth,
                 const char *nativeEOL);
  void resolve(const char *path, svn_depth_t depth,
               svn_wc_conflict_choice_t choice);
  void cleanup(const char *path);
  void mkdir(Targets &targets, CommitMessage *message, bool makeParents,
             RevpropTable &revprops, CommitCallback *callback);
  void move(Targets &srcPaths, const char *destPath,
            CommitMessage *message, bool force, bool moveAsChild,
            bool makeParents, RevpropTable &revprops, CommitCallback *callback);
  void copy(CopySources &copySources, const char *destPath,
            CommitMessage *message, bool copyAsChild, bool makeParents,
            bool ignoreExternals, RevpropTable &revprops,
            CommitCallback *callback);
  void commit(Targets &targets, CommitMessage *message, svn_depth_t depth,
              bool noUnlock, bool keepChangelist,
              StringArray &changelists, RevpropTable &revprops,
              CommitCallback *callback);
  jlongArray update(Targets &targets, Revision &revision, svn_depth_t depth,
                    bool depthIsSticky, bool makeParents, bool ignoreExternals,
                    bool allowUnverObstructions);
  void add(const char *path, svn_depth_t depth, bool force, bool no_ignore,
           bool add_parents);
  void revert(const char *path, svn_depth_t depth, StringArray &changelists);
  void remove(Targets &targets, CommitMessage *message, bool force,
              bool keep_local, RevpropTable &revprops,
              CommitCallback *callback);
  jlong checkout(const char *moduleName, const char *destPath,
                 Revision &revision, Revision &pegRevsion, svn_depth_t depth,
                 bool ignoreExternals, bool allowUnverObstructions);
  void logMessages(const char *path, Revision &pegRevision,
                   std::vector<RevisionRange> &ranges, bool stopOnCopy,
                   bool discoverPaths, bool includeMergedRevisions,
                   StringArray &revProps,
                   long limit, LogMessageCallback *callback);
  jstring getAdminDirectoryName();
  jboolean isAdminDirectory(const char *name);
  void addToChangelist(Targets &srcPaths, const char *changelist,
                       svn_depth_t depth, StringArray &changelists);
  void removeFromChangelists(Targets &srcPaths, svn_depth_t depth,
                             StringArray &changelists);
  void getChangelists(const char *path, StringArray &changelists,
                      svn_depth_t depth, ChangelistCallback *callback);
  void status(const char *path, svn_depth_t depth, bool onServer,
              bool getAll, bool noIgnore, bool ignoreExternals,
              StringArray &changelists, StatusCallback *callback);
  void list(const char *url, Revision &revision, Revision &pegRevision,
            svn_depth_t depth, int direntFields, bool fetchLocks,
            ListCallback *callback);
  jbyteArray revProperty(const char *path, const char *name, Revision &rev);
  void setRevProperty(const char *path, const char *name,
                      Revision &rev, const char *value,
                      const char *original_value, bool force);
  jstring getVersionInfo(const char *path, const char *trailUrl,
                         bool lastChanged);
  void upgrade(const char *path);
  jbyteArray propertyGet(const char *path, const char *name,
                         Revision &revision, Revision &pegRevision);
  void diff(const char *target1, Revision &revision1,
            const char *target2, Revision &revision2,
            const char *relativeToDir, const char *outfileName,
            svn_depth_t depth, StringArray &changelists,
            bool ignoreAncestry, bool noDiffDelete, bool force,
            bool showCopiesAsAdds);
  void diff(const char *target, Revision &pegevision,
            Revision &startRevision, Revision &endRevision,
            const char *relativeToDir, const char *outfileName,
            svn_depth_t depth, StringArray &changelists,
            bool ignoreAncestry, bool noDiffDelete, bool force,
            bool showCopiesAsAdds);
  void diffSummarize(const char *target1, Revision &revision1,
                     const char *target2, Revision &revision2,
                     svn_depth_t depth, StringArray &changelists,
                     bool ignoreAncestry, DiffSummaryReceiver &receiver);
  void diffSummarize(const char *target, Revision &pegRevision,
                     Revision &startRevision, Revision &endRevision,
                     svn_depth_t depth, StringArray &changelists,
                     bool ignoreAncestry, DiffSummaryReceiver &receiver);

  ClientContext &getClientContext();

  const char *getLastPath();
  void dispose();
  static SVNClient *getCppObject(jobject jthis);
  SVNClient(jobject jthis_in);
  virtual ~SVNClient();
 private:
  /**
   * Shared implementation for diff() APIs. When pegRevision is
   * provided, revision1 and revision2 equate to startRevision and
   * endRevision (respectively), and target2 is ignored.
   */
  void diff(const char *target1, Revision &revision1,
            const char *target2, Revision &revision2,
            Revision *pegRevision, const char *relativeToDir,
            const char *outfileName, svn_depth_t depth, StringArray &changelists,
            bool ignoreAncestry, bool noDiffDelete, bool force,
            bool showCopiesAsAdds);

  Path m_lastPath;
  ClientContext context;
};

#endif // SVNCLIENT_H

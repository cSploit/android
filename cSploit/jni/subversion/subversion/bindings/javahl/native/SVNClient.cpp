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
 * @file SVNClient.cpp
 * @brief: Implementation of the SVNClient class
 */

#include "SVNClient.h"
#include "JNIUtil.h"
#include "CopySources.h"
#include "DiffSummaryReceiver.h"
#include "ClientContext.h"
#include "Prompter.h"
#include "Pool.h"
#include "Targets.h"
#include "Revision.h"
#include "OutputStream.h"
#include "RevisionRange.h"
#include "BlameCallback.h"
#include "ProplistCallback.h"
#include "LogMessageCallback.h"
#include "InfoCallback.h"
#include "PatchCallback.h"
#include "CommitCallback.h"
#include "StatusCallback.h"
#include "ChangelistCallback.h"
#include "ListCallback.h"
#include "JNIByteArray.h"
#include "CommitMessage.h"
#include "EnumMapper.h"
#include "StringArray.h"
#include "RevpropTable.h"
#include "CreateJ.h"
#include "svn_auth.h"
#include "svn_dso.h"
#include "svn_types.h"
#include "svn_client.h"
#include "svn_sorts.h"
#include "svn_time.h"
#include "svn_diff.h"
#include "svn_config.h"
#include "svn_io.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_utf.h"
#include "svn_private_config.h"
#include "JNIStringHolder.h"
#include <vector>
#include <iostream>
#include <sstream>


SVNClient::SVNClient(jobject jthis_in)
    : context(jthis_in, pool), m_lastPath("", pool)
{
}

SVNClient::~SVNClient()
{
}

SVNClient *SVNClient::getCppObject(jobject jthis)
{
    static jfieldID fid = 0;
    jlong cppAddr = SVNBase::findCppAddrForJObject(jthis, &fid,
                                                   JAVA_PACKAGE"/SVNClient");
    return (cppAddr == 0 ? NULL : reinterpret_cast<SVNClient *>(cppAddr));
}

void SVNClient::dispose()
{
    static jfieldID fid = 0;
    SVNBase::dispose(&fid, JAVA_PACKAGE"/SVNClient");
}

jstring SVNClient::getAdminDirectoryName()
{
    SVN::Pool subPool(pool);
    jstring name =
        JNIUtil::makeJString(svn_wc_get_adm_dir(subPool.getPool()));
    if (JNIUtil::isJavaExceptionThrown())
        return NULL;

    return name;
}

jboolean SVNClient::isAdminDirectory(const char *name)
{
    SVN::Pool subPool(pool);
    return svn_wc_is_adm_dir(name, subPool.getPool()) ? JNI_TRUE : JNI_FALSE;
}

const char *SVNClient::getLastPath()
{
    return m_lastPath.c_str();
}

/**
 * List directory entries of a URL.
 */
void SVNClient::list(const char *url, Revision &revision,
                     Revision &pegRevision, svn_depth_t depth,
                     int direntFields, bool fetchLocks,
                     ListCallback *callback)
{
    SVN::Pool subPool(pool);
    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    SVN_JNI_NULL_PTR_EX(url, "path or url", );

    Path urlPath(url, subPool);
    SVN_JNI_ERR(urlPath.error_occured(), );

    SVN_JNI_ERR(svn_client_list2(urlPath.c_str(),
                                 pegRevision.revision(),
                                 revision.revision(),
                                 depth,
                                 direntFields,
                                 fetchLocks,
                                 ListCallback::callback,
                                 callback,
                                 ctx, subPool.getPool()), );
}

void
SVNClient::status(const char *path, svn_depth_t depth,
                  bool onServer, bool getAll, bool noIgnore,
                  bool ignoreExternals, StringArray &changelists,
                  StatusCallback *callback)
{
    SVN::Pool subPool(pool);
    svn_revnum_t youngest = SVN_INVALID_REVNUM;
    svn_opt_revision_t rev;

    SVN_JNI_NULL_PTR_EX(path, "path", );

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;
    callback->setWcCtx(ctx->wc_ctx);

    Path checkedPath(path, subPool);
    SVN_JNI_ERR(checkedPath.error_occured(), );

    rev.kind = svn_opt_revision_unspecified;

    SVN_JNI_ERR(svn_client_status5(&youngest, ctx, checkedPath.c_str(),
                                   &rev,
                                   depth,
                                   getAll, onServer, noIgnore, ignoreExternals,
                                   FALSE,
                                   changelists.array(subPool),
                                   StatusCallback::callback, callback,
                                   subPool.getPool()), );
}

void SVNClient::logMessages(const char *path, Revision &pegRevision,
                            std::vector<RevisionRange> &logRanges,
                            bool stopOnCopy, bool discoverPaths,
                            bool includeMergedRevisions, StringArray &revProps,
                            long limit, LogMessageCallback *callback)
{
    SVN::Pool subPool(pool);

    SVN_JNI_NULL_PTR_EX(path, "path", );

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    Targets target(path, subPool);
    const apr_array_header_t *targets = target.array(subPool);
    SVN_JNI_ERR(target.error_occured(), );

    apr_array_header_t *ranges =
        apr_array_make(subPool.getPool(), logRanges.size(),
                       sizeof(svn_opt_revision_range_t *));

    std::vector<RevisionRange>::const_iterator it;
    for (it = logRanges.begin(); it != logRanges.end(); ++it)
    {
        if (it->toRange(subPool)->start.kind
            == svn_opt_revision_unspecified
            && it->toRange(subPool)->end.kind
            == svn_opt_revision_unspecified)
        {
            svn_opt_revision_range_t *range =
                (svn_opt_revision_range_t *)apr_pcalloc(subPool.getPool(),
                                                        sizeof(*range));
            range->start.kind = svn_opt_revision_number;
            range->start.value.number = 1;
            range->end.kind = svn_opt_revision_head;
            APR_ARRAY_PUSH(ranges, const svn_opt_revision_range_t *) = range;
        }
        else
        {
            APR_ARRAY_PUSH(ranges, const svn_opt_revision_range_t *) =
                it->toRange(subPool);
        }
        if (JNIUtil::isExceptionThrown())
            return;
    }

    SVN_JNI_ERR(svn_client_log5(targets, pegRevision.revision(), ranges,
                                limit, discoverPaths, stopOnCopy,
                                includeMergedRevisions,
                                revProps.array(subPool),
                                LogMessageCallback::callback, callback, ctx,
                                subPool.getPool()), );
}

jlong SVNClient::checkout(const char *moduleName, const char *destPath,
                          Revision &revision, Revision &pegRevision,
                          svn_depth_t depth, bool ignoreExternals,
                          bool allowUnverObstructions)
{
    SVN::Pool subPool;

    SVN_JNI_NULL_PTR_EX(moduleName, "moduleName", -1);
    SVN_JNI_NULL_PTR_EX(destPath, "destPath", -1);

    Path url(moduleName, subPool);
    Path path(destPath, subPool);
    SVN_JNI_ERR(url.error_occured(), -1);
    SVN_JNI_ERR(path.error_occured(), -1);
    svn_revnum_t rev;

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return -1;

    SVN_JNI_ERR(svn_client_checkout3(&rev, url.c_str(),
                                     path.c_str(),
                                     pegRevision.revision(),
                                     revision.revision(),
                                     depth,
                                     ignoreExternals,
                                     allowUnverObstructions,
                                     ctx,
                                     subPool.getPool()),
                -1);

    return rev;
}

void SVNClient::remove(Targets &targets, CommitMessage *message, bool force,
                       bool keep_local, RevpropTable &revprops,
                       CommitCallback *callback)
{
    SVN::Pool subPool(pool);
    svn_client_ctx_t *ctx = context.getContext(message, subPool);
    if (ctx == NULL)
        return;

    const apr_array_header_t *targets2 = targets.array(subPool);
    SVN_JNI_ERR(targets.error_occured(), );

    SVN_JNI_ERR(svn_client_delete4(targets2, force, keep_local,
                                   revprops.hash(subPool),
                                   CommitCallback::callback, callback,
                                   ctx, subPool.getPool()), );
}

void SVNClient::revert(const char *path, svn_depth_t depth,
                       StringArray &changelists)
{
    SVN::Pool subPool(pool);

    SVN_JNI_NULL_PTR_EX(path, "path", );

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    Targets target(path, subPool);
    const apr_array_header_t *targets = target.array(subPool);
    SVN_JNI_ERR(target.error_occured(), );
    SVN_JNI_ERR(svn_client_revert2(targets, depth,
                                   changelists.array(subPool), ctx,
                                   subPool.getPool()), );
}

void SVNClient::add(const char *path,
                    svn_depth_t depth, bool force, bool no_ignore,
                    bool add_parents)
{
    SVN::Pool subPool(pool);

    SVN_JNI_NULL_PTR_EX(path, "path", );

    Path intPath(path, subPool);
    SVN_JNI_ERR(intPath.error_occured(), );
    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    SVN_JNI_ERR(svn_client_add4(intPath.c_str(), depth, force,
                                no_ignore, add_parents, ctx,
                                subPool.getPool()), );
}

jlongArray SVNClient::update(Targets &targets, Revision &revision,
                             svn_depth_t depth, bool depthIsSticky,
                             bool makeParents, bool ignoreExternals,
                             bool allowUnverObstructions)
{
    SVN::Pool subPool(pool);

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    apr_array_header_t *revs;
    if (ctx == NULL)
        return NULL;

    const apr_array_header_t *array = targets.array(subPool);
    SVN_JNI_ERR(targets.error_occured(), NULL);
    SVN_JNI_ERR(svn_client_update4(&revs, array,
                                   revision.revision(),
                                   depth,
                                   depthIsSticky,
                                   ignoreExternals,
                                   allowUnverObstructions,
                                   TRUE /* adds_as_modification */,
                                   makeParents,
                                   ctx, subPool.getPool()),
                NULL);

    JNIEnv *env = JNIUtil::getEnv();
    jlongArray jrevs = env->NewLongArray(revs->nelts);
    if (JNIUtil::isJavaExceptionThrown())
        return NULL;
    jlong *jrevArray = env->GetLongArrayElements(jrevs, NULL);
    if (JNIUtil::isJavaExceptionThrown())
        return NULL;
    for (int i = 0; i < revs->nelts; ++i)
    {
        jlong rev = APR_ARRAY_IDX(revs, i, svn_revnum_t);
        jrevArray[i] = rev;
    }
    env->ReleaseLongArrayElements(jrevs, jrevArray, 0);

    return jrevs;
}

void SVNClient::commit(Targets &targets, CommitMessage *message,
                       svn_depth_t depth, bool noUnlock, bool keepChangelist,
                       StringArray &changelists, RevpropTable &revprops,
                       CommitCallback *callback)
{
    SVN::Pool subPool(pool);
    const apr_array_header_t *targets2 = targets.array(subPool);
    SVN_JNI_ERR(targets.error_occured(), );
    svn_client_ctx_t *ctx = context.getContext(message, subPool);
    if (ctx == NULL)
        return;

    SVN_JNI_ERR(svn_client_commit5(targets2, depth,
                                   noUnlock, keepChangelist, TRUE,
                                   changelists.array(subPool),
                                   revprops.hash(subPool),
                                   CommitCallback::callback, callback,
                                   ctx, subPool.getPool()),
                );
}

void SVNClient::copy(CopySources &copySources, const char *destPath,
                     CommitMessage *message, bool copyAsChild,
                     bool makeParents, bool ignoreExternals,
                     RevpropTable &revprops, CommitCallback *callback)
{
    SVN::Pool subPool(pool);

    apr_array_header_t *srcs = copySources.array(subPool);
    if (srcs == NULL)
    {
        JNIUtil::throwNativeException(JAVA_PACKAGE "/ClientException",
                                      "Invalid copy sources");
        return;
    }
    SVN_JNI_NULL_PTR_EX(destPath, "destPath", );
    Path destinationPath(destPath, subPool);
    SVN_JNI_ERR(destinationPath.error_occured(), );

    svn_client_ctx_t *ctx = context.getContext(message, subPool);
    if (ctx == NULL)
        return;

    SVN_JNI_ERR(svn_client_copy6(srcs, destinationPath.c_str(),
                                 copyAsChild, makeParents, ignoreExternals,
                                 revprops.hash(subPool),
                                 CommitCallback::callback, callback,
                                 ctx, subPool.getPool()), );
}

void SVNClient::move(Targets &srcPaths, const char *destPath,
                     CommitMessage *message, bool force, bool moveAsChild,
                     bool makeParents, RevpropTable &revprops,
                     CommitCallback *callback)
{
    SVN::Pool subPool(pool);

    const apr_array_header_t *srcs = srcPaths.array(subPool);
    SVN_JNI_ERR(srcPaths.error_occured(), );
    SVN_JNI_NULL_PTR_EX(destPath, "destPath", );
    Path destinationPath(destPath, subPool);
    SVN_JNI_ERR(destinationPath.error_occured(), );

    svn_client_ctx_t *ctx = context.getContext(message, subPool);
    if (ctx == NULL)
        return;

    SVN_JNI_ERR(svn_client_move6((apr_array_header_t *) srcs,
                                 destinationPath.c_str(), moveAsChild,
                                 makeParents, revprops.hash(subPool),
                                 CommitCallback::callback, callback, ctx,
                                 subPool.getPool()), );
}

void SVNClient::mkdir(Targets &targets, CommitMessage *message,
                      bool makeParents, RevpropTable &revprops,
                      CommitCallback *callback)
{
    SVN::Pool subPool(pool);
    svn_client_ctx_t *ctx = context.getContext(message, subPool);
    if (ctx == NULL)
        return;

    const apr_array_header_t *targets2 = targets.array(subPool);
    SVN_JNI_ERR(targets.error_occured(), );

    SVN_JNI_ERR(svn_client_mkdir4(targets2, makeParents,
                                  revprops.hash(subPool),
                                  CommitCallback::callback, callback,
                                  ctx, subPool.getPool()), );
}

void SVNClient::cleanup(const char *path)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(path, "path", );
    Path intPath(path, subPool);
    SVN_JNI_ERR(intPath.error_occured(), );

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    SVN_JNI_ERR(svn_client_cleanup(intPath.c_str(), ctx, subPool.getPool()),);
}

void SVNClient::resolve(const char *path, svn_depth_t depth,
                        svn_wc_conflict_choice_t choice)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(path, "path", );
    Path intPath(path, subPool);
    SVN_JNI_ERR(intPath.error_occured(), );
    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    SVN_JNI_ERR(svn_client_resolve(intPath.c_str(), depth, choice,
                                   ctx, subPool.getPool()), );
}

jlong SVNClient::doExport(const char *srcPath, const char *destPath,
                          Revision &revision, Revision &pegRevision,
                          bool force, bool ignoreExternals,
                          svn_depth_t depth, const char *nativeEOL)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(srcPath, "srcPath", -1);
    SVN_JNI_NULL_PTR_EX(destPath, "destPath", -1);
    Path sourcePath(srcPath, subPool);
    SVN_JNI_ERR(sourcePath.error_occured(), -1);
    Path destinationPath(destPath, subPool);
    SVN_JNI_ERR(destinationPath.error_occured(), -1);
    svn_revnum_t rev;
    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return -1;

    SVN_JNI_ERR(svn_client_export5(&rev, sourcePath.c_str(),
                                   destinationPath.c_str(),
                                   pegRevision.revision(),
                                   revision.revision(), force,
                                   ignoreExternals, FALSE,
                                   depth,
                                   nativeEOL, ctx,
                                   subPool.getPool()),
                -1);

    return rev;

}

jlong SVNClient::doSwitch(const char *path, const char *url,
                          Revision &revision, Revision &pegRevision,
                          svn_depth_t depth, bool depthIsSticky,
                          bool ignoreExternals,
                          bool allowUnverObstructions,
                          bool ignoreAncestry)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(path, "path", -1);
    SVN_JNI_NULL_PTR_EX(url, "url", -1);
    Path intUrl(url, subPool);
    SVN_JNI_ERR(intUrl.error_occured(), -1);
    Path intPath(path, subPool);
    SVN_JNI_ERR(intPath.error_occured(), -1);

    svn_revnum_t rev;
    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return -1;

    SVN_JNI_ERR(svn_client_switch3(&rev, intPath.c_str(),
                                   intUrl.c_str(),
                                   pegRevision.revision(),
                                   revision.revision(),
                                   depth,
                                   depthIsSticky,
                                   ignoreExternals,
                                   allowUnverObstructions,
                                   ignoreAncestry,
                                   ctx,
                                   subPool.getPool()),
                -1);

    return rev;
}

void SVNClient::doImport(const char *path, const char *url,
                         CommitMessage *message, svn_depth_t depth,
                         bool noIgnore, bool ignoreUnknownNodeTypes,
                         RevpropTable &revprops, CommitCallback *callback)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(path, "path", );
    SVN_JNI_NULL_PTR_EX(url, "url", );
    Path intPath(path, subPool);
    SVN_JNI_ERR(intPath.error_occured(), );
    Path intUrl(url, subPool);
    SVN_JNI_ERR(intUrl.error_occured(), );

    svn_client_ctx_t *ctx = context.getContext(message, subPool);
    if (ctx == NULL)
        return;

    SVN_JNI_ERR(svn_client_import4(intPath.c_str(), intUrl.c_str(), depth,
                                   noIgnore, ignoreUnknownNodeTypes,
                                   revprops.hash(subPool),
                                   CommitCallback::callback, callback,
                                   ctx, subPool.getPool()), );
}

jobject
SVNClient::suggestMergeSources(const char *path, Revision &pegRevision)
{
    SVN::Pool subPool(pool);
    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return NULL;

    apr_array_header_t *sources;
    SVN_JNI_ERR(svn_client_suggest_merge_sources(&sources, path,
                                                 pegRevision.revision(),
                                                 ctx, subPool.getPool()),
                NULL);

    return CreateJ::StringSet(sources);
}

void SVNClient::merge(const char *path1, Revision &revision1,
                      const char *path2, Revision &revision2,
                      const char *localPath, bool force, svn_depth_t depth,
                      bool ignoreAncestry, bool dryRun, bool recordOnly)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(path1, "path1", );
    SVN_JNI_NULL_PTR_EX(path2, "path2", );
    SVN_JNI_NULL_PTR_EX(localPath, "localPath", );
    Path intLocalPath(localPath, subPool);
    SVN_JNI_ERR(intLocalPath.error_occured(), );

    Path srcPath1(path1, subPool);
    SVN_JNI_ERR(srcPath1.error_occured(), );

    Path srcPath2(path2, subPool);
    SVN_JNI_ERR(srcPath2.error_occured(), );

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    SVN_JNI_ERR(svn_client_merge4(srcPath1.c_str(), revision1.revision(),
                                  srcPath2.c_str(), revision2.revision(),
                                  intLocalPath.c_str(),
                                  depth,
                                  ignoreAncestry, force, recordOnly, dryRun,
                                  TRUE, NULL, ctx, subPool.getPool()), );
}

void SVNClient::merge(const char *path, Revision &pegRevision,
                      std::vector<RevisionRange> &rangesToMerge,
                      const char *localPath, bool force, svn_depth_t depth,
                      bool ignoreAncestry, bool dryRun, bool recordOnly)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(path, "path", );
    SVN_JNI_NULL_PTR_EX(localPath, "localPath", );
    Path intLocalPath(localPath, subPool);
    SVN_JNI_ERR(intLocalPath.error_occured(), );

    Path srcPath(path, subPool);
    SVN_JNI_ERR(srcPath.error_occured(), );

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    apr_array_header_t *ranges =
      apr_array_make(subPool.getPool(), rangesToMerge.size(),
                     sizeof(const svn_opt_revision_range_t *));

    std::vector<RevisionRange>::const_iterator it;
    for (it = rangesToMerge.begin(); it != rangesToMerge.end(); ++it)
    {
        if (it->toRange(subPool)->start.kind
            == svn_opt_revision_unspecified
            && it->toRange(subPool)->end.kind
            == svn_opt_revision_unspecified)
        {
            svn_opt_revision_range_t *range =
                (svn_opt_revision_range_t *)apr_pcalloc(subPool.getPool(),
                                                        sizeof(*range));
            range->start.kind = svn_opt_revision_number;
            range->start.value.number = 1;
            range->end.kind = svn_opt_revision_head;
            APR_ARRAY_PUSH(ranges, const svn_opt_revision_range_t *) = range;
        }
        else
        {
            APR_ARRAY_PUSH(ranges, const svn_opt_revision_range_t *) =
                it->toRange(subPool);
        }
        if (JNIUtil::isExceptionThrown())
            return;
    }

    SVN_JNI_ERR(svn_client_merge_peg4(srcPath.c_str(),
                                      ranges,
                                      pegRevision.revision(),
                                      intLocalPath.c_str(),
                                      depth,
                                      ignoreAncestry, force, recordOnly,
                                      dryRun, TRUE, NULL, ctx,
                                      subPool.getPool()), );
}

void SVNClient::mergeReintegrate(const char *path, Revision &pegRevision,
                                 const char *localPath, bool dryRun)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(path, "path", );
    SVN_JNI_NULL_PTR_EX(localPath, "localPath", );
    Path intLocalPath(localPath, subPool);
    SVN_JNI_ERR(intLocalPath.error_occured(), );

    Path srcPath(path, subPool);
    SVN_JNI_ERR(srcPath.error_occured(), );

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    SVN_JNI_ERR(svn_client_merge_reintegrate(srcPath.c_str(),
                                             pegRevision.revision(),
                                             intLocalPath.c_str(),
                                             dryRun, NULL, ctx,
                                             subPool.getPool()), );
}

jobject
SVNClient::getMergeinfo(const char *target, Revision &pegRevision)
{
    SVN::Pool subPool(pool);
    JNIEnv *env = JNIUtil::getEnv();

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return NULL;

    svn_mergeinfo_t mergeinfo;
    Path intLocalTarget(target, subPool);
    SVN_JNI_ERR(intLocalTarget.error_occured(), NULL);
    SVN_JNI_ERR(svn_client_mergeinfo_get_merged(&mergeinfo,
                                                intLocalTarget.c_str(),
                                                pegRevision.revision(), ctx,
                                                subPool.getPool()),
                NULL);
    if (mergeinfo == NULL)
        return NULL;

    // Transform mergeinfo into Java Mergeinfo object.
    jclass clazz = env->FindClass(JAVA_PACKAGE "/types/Mergeinfo");
    if (JNIUtil::isJavaExceptionThrown())
        return NULL;

    static jmethodID ctor = 0;
    if (ctor == 0)
    {
        ctor = env->GetMethodID(clazz, "<init>", "()V");
        if (JNIUtil::isJavaExceptionThrown())
            return NULL;
    }

    static jmethodID addRevisions = 0;
    if (addRevisions == 0)
    {
        addRevisions = env->GetMethodID(clazz, "addRevisions",
                                        "(Ljava/lang/String;"
                                        "Ljava/util/List;)V");
        if (JNIUtil::isJavaExceptionThrown())
            return NULL;
    }

    jobject jmergeinfo = env->NewObject(clazz, ctor);
    if (JNIUtil::isJavaExceptionThrown())
        return NULL;

    apr_hash_index_t *hi;
    for (hi = apr_hash_first(subPool.getPool(), mergeinfo);
         hi;
         hi = apr_hash_next(hi))
    {
        const void *path;
        void *val;
        apr_hash_this(hi, &path, NULL, &val);

        jstring jpath = JNIUtil::makeJString((const char *) path);
        jobject jranges =
            CreateJ::RevisionRangeList((apr_array_header_t *) val);

        env->CallVoidMethod(jmergeinfo, addRevisions, jpath, jranges);

        env->DeleteLocalRef(jranges);
        env->DeleteLocalRef(jpath);
    }

    env->DeleteLocalRef(clazz);

    return jmergeinfo;
}

void SVNClient::getMergeinfoLog(int type, const char *pathOrURL,
                                Revision &pegRevision,
                                const char *mergeSourceURL,
                                Revision &srcPegRevision,
                                bool discoverChangedPaths,
                                svn_depth_t depth,
                                StringArray &revProps,
                                LogMessageCallback *callback)
{
    SVN::Pool subPool(pool);

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    SVN_JNI_NULL_PTR_EX(pathOrURL, "path or url", );
    Path urlPath(pathOrURL, subPool);
    SVN_JNI_ERR(urlPath.error_occured(), );

    SVN_JNI_NULL_PTR_EX(mergeSourceURL, "merge source url", );
    Path srcURL(mergeSourceURL, subPool);
    SVN_JNI_ERR(srcURL.error_occured(), );

    SVN_JNI_ERR(svn_client_mergeinfo_log((type == 1),
                                         urlPath.c_str(),
                                         pegRevision.revision(),
                                         srcURL.c_str(),
                                         srcPegRevision.revision(),
                                         LogMessageCallback::callback,
                                         callback,
                                         discoverChangedPaths,
                                         depth,
                                         revProps.array(subPool),
                                         ctx,
                                         subPool.getPool()), );

    return;
}

/**
 * Get a property.
 */
jbyteArray SVNClient::propertyGet(const char *path, const char *name,
                                  Revision &revision, Revision &pegRevision)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(path, "path", NULL);
    SVN_JNI_NULL_PTR_EX(name, "name", NULL);
    Path intPath(path, subPool);
    SVN_JNI_ERR(intPath.error_occured(), NULL);

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return NULL;

    apr_hash_t *props;
    SVN_JNI_ERR(svn_client_propget3(&props, name,
                                    intPath.c_str(), pegRevision.revision(),
                                    revision.revision(), NULL, svn_depth_empty,
                                    NULL, ctx, subPool.getPool()),
                NULL);

    apr_hash_index_t *hi;
    // only one element since we disabled recurse
    hi = apr_hash_first(subPool.getPool(), props);
    if (hi == NULL)
        return NULL; // no property with this name

    svn_string_t *propval;
    apr_hash_this(hi, NULL, NULL, (void**)&propval);

    if (propval == NULL)
        return NULL;

    return JNIUtil::makeJByteArray((const signed char *)propval->data,
                                   propval->len);
}

void SVNClient::properties(const char *path, Revision &revision,
                           Revision &pegRevision, svn_depth_t depth,
                           StringArray &changelists, ProplistCallback *callback)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(path, "path", );
    Path intPath(path, subPool);
    SVN_JNI_ERR(intPath.error_occured(), );

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    SVN_JNI_ERR(svn_client_proplist3(intPath.c_str(), pegRevision.revision(),
                                     revision.revision(), depth,
                                     changelists.array(subPool),
                                     ProplistCallback::callback, callback,
                                     ctx, subPool.getPool()), );

    return;
}

void SVNClient::propertySetLocal(Targets &targets, const char *name,
                                 JNIByteArray &value, svn_depth_t depth,
                                 StringArray &changelists, bool force)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(name, "name", );

    svn_string_t *val;
    if (value.isNull())
      val = NULL;
    else
      val = svn_string_ncreate((const char *)value.getBytes(), value.getLength(),
                               subPool.getPool());

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    const apr_array_header_t *targetsApr = targets.array(subPool);
    SVN_JNI_ERR(svn_client_propset_local(name, val, targetsApr,
                                         depth, force,
                                         changelists.array(subPool),
                                         ctx, subPool.getPool()), );
}

void SVNClient::propertySetRemote(const char *path, long base_rev,
                                  const char *name,
                                  CommitMessage *message,
                                  JNIByteArray &value, bool force,
                                  RevpropTable &revprops,
                                  CommitCallback *callback)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(name, "name", );

    svn_string_t *val;
    if (value.isNull())
      val = NULL;
    else
      val = svn_string_ncreate((const char *)value.getBytes(), value.getLength(),
                               subPool.getPool());

    Path intPath(path, subPool);
    SVN_JNI_ERR(intPath.error_occured(), );

    svn_client_ctx_t *ctx = context.getContext(message, subPool);
    if (ctx == NULL)
        return;

    SVN_JNI_ERR(svn_client_propset_remote(name, val, intPath.c_str(),
                                          force, base_rev,
                                          revprops.hash(subPool),
                                          CommitCallback::callback, callback,
                                          ctx, subPool.getPool()), );
}

void SVNClient::diff(const char *target1, Revision &revision1,
                     const char *target2, Revision &revision2,
                     Revision *pegRevision, const char *relativeToDir,
                     const char *outfileName, svn_depth_t depth,
                     StringArray &changelists,
                     bool ignoreAncestry, bool noDiffDelete, bool force,
                     bool showCopiesAsAdds)
{
    svn_error_t *err;
    SVN::Pool subPool(pool);
    const char *c_relToDir = relativeToDir ?
      svn_dirent_canonicalize(relativeToDir, subPool.getPool()) :
      relativeToDir;

    SVN_JNI_NULL_PTR_EX(target1, "target", );
    // target2 is ignored when pegRevision is provided.
    if (pegRevision == NULL)
        SVN_JNI_NULL_PTR_EX(target2, "target2", );

    SVN_JNI_NULL_PTR_EX(outfileName, "outfileName", );
    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    Path path1(target1, subPool);
    SVN_JNI_ERR(path1.error_occured(), );

    apr_file_t *outfile = NULL;
    apr_status_t rv =
        apr_file_open(&outfile,
                      svn_dirent_internal_style(outfileName,
                                                subPool.getPool()),
                      APR_CREATE|APR_WRITE|APR_TRUNCATE , APR_OS_DEFAULT,
                      subPool.getPool());
    if (rv != APR_SUCCESS)
    {
        SVN_JNI_ERR(svn_error_createf(rv, NULL, _("Cannot open file '%s'"),
                                      outfileName), );
    }

    // We don't use any options to diff.
    apr_array_header_t *diffOptions = apr_array_make(subPool.getPool(),
                                                     0, sizeof(char *));

    if (pegRevision)
    {
        err = svn_client_diff_peg5(diffOptions,
                                   path1.c_str(),
                                   pegRevision->revision(),
                                   revision1.revision(),
                                   revision2.revision(),
                                   c_relToDir,
                                   depth,
                                   ignoreAncestry,
                                   noDiffDelete,
                                   showCopiesAsAdds,
                                   force,
                                   FALSE,
                                   SVN_APR_LOCALE_CHARSET,
                                   outfile,
                                   NULL /* error file */,
                                   changelists.array(subPool),
                                   ctx,
                                   subPool.getPool());
    }
    else
    {
        // "Regular" diff (without a peg revision).
        Path path2(target2, subPool);
        err = path2.error_occured();
        if (err)
        {
            if (outfile)
                goto cleanup;

            SVN_JNI_ERR(err, );
        }

        err = svn_client_diff5(diffOptions,
                               path1.c_str(),
                               revision1.revision(),
                               path2.c_str(),
                               revision2.revision(),
                               c_relToDir,
                               depth,
                               ignoreAncestry,
                               noDiffDelete,
                               showCopiesAsAdds,
                               force,
                               FALSE,
                               SVN_APR_LOCALE_CHARSET,
                               outfile,
                               NULL /* error file */,
                               changelists.array(subPool),
                               ctx,
                               subPool.getPool());
    }

cleanup:
    rv = apr_file_close(outfile);
    if (rv != APR_SUCCESS)
    {
        svn_error_clear(err);

        SVN_JNI_ERR(svn_error_createf(rv, NULL, _("Cannot close file '%s'"),
                                      outfileName), );
    }

    SVN_JNI_ERR(err, );
}

void SVNClient::diff(const char *target1, Revision &revision1,
                     const char *target2, Revision &revision2,
                     const char *relativeToDir, const char *outfileName,
                     svn_depth_t depth, StringArray &changelists,
                     bool ignoreAncestry, bool noDiffDelete, bool force,
                     bool showCopiesAsAdds)
{
    diff(target1, revision1, target2, revision2, NULL, relativeToDir,
         outfileName, depth, changelists, ignoreAncestry, noDiffDelete, force,
         showCopiesAsAdds);
}

void SVNClient::diff(const char *target, Revision &pegRevision,
                     Revision &startRevision, Revision &endRevision,
                     const char *relativeToDir, const char *outfileName,
                     svn_depth_t depth, StringArray &changelists,
                     bool ignoreAncestry, bool noDiffDelete, bool force,
                     bool showCopiesAsAdds)
{
    diff(target, startRevision, NULL, endRevision, &pegRevision,
         relativeToDir, outfileName, depth, changelists,
         ignoreAncestry, noDiffDelete, force, showCopiesAsAdds);
}

void
SVNClient::diffSummarize(const char *target1, Revision &revision1,
                         const char *target2, Revision &revision2,
                         svn_depth_t depth, StringArray &changelists,
                         bool ignoreAncestry,
                         DiffSummaryReceiver &receiver)
{
    SVN::Pool subPool(pool);

    SVN_JNI_NULL_PTR_EX(target1, "target1", );
    SVN_JNI_NULL_PTR_EX(target2, "target2", );

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    Path path1(target1, subPool);
    SVN_JNI_ERR(path1.error_occured(), );
    Path path2(target2, subPool);
    SVN_JNI_ERR(path2.error_occured(), );

    SVN_JNI_ERR(svn_client_diff_summarize2(path1.c_str(), revision1.revision(),
                                           path2.c_str(), revision2.revision(),
                                           depth,
                                           ignoreAncestry,
                                           changelists.array(subPool),
                                           DiffSummaryReceiver::summarize,
                                           &receiver,
                                           ctx, subPool.getPool()), );
}

void
SVNClient::diffSummarize(const char *target, Revision &pegRevision,
                         Revision &startRevision, Revision &endRevision,
                         svn_depth_t depth, StringArray &changelists,
                         bool ignoreAncestry, DiffSummaryReceiver &receiver)
{
    SVN::Pool subPool(pool);

    SVN_JNI_NULL_PTR_EX(target, "target", );

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    Path path(target, subPool);
    SVN_JNI_ERR(path.error_occured(), );

    SVN_JNI_ERR(svn_client_diff_summarize_peg2(path.c_str(),
                                               pegRevision.revision(),
                                               startRevision.revision(),
                                               endRevision.revision(),
                                               depth,
                                               ignoreAncestry,
                                               changelists.array(subPool),
                                               DiffSummaryReceiver::summarize,
                                               &receiver, ctx,
                                               subPool.getPool()), );
}

void SVNClient::streamFileContent(const char *path, Revision &revision,
                                  Revision &pegRevision,
                                  OutputStream &outputStream)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(path, "path", );
    Path intPath(path, subPool);
    SVN_JNI_ERR(intPath.error_occured(), );

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    SVN_JNI_ERR(svn_client_cat2(outputStream.getStream(subPool),
                                path, pegRevision.revision(),
                                revision.revision(), ctx, subPool.getPool()),
                );
}

jbyteArray SVNClient::revProperty(const char *path,
                                  const char *name, Revision &rev)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(path, "path", NULL);
    SVN_JNI_NULL_PTR_EX(name, "name", NULL);
    Path intPath(path, subPool);
    SVN_JNI_ERR(intPath.error_occured(), NULL);

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return NULL;

    const char *URL;
    svn_string_t *propval;
    svn_revnum_t set_rev;
    SVN_JNI_ERR(svn_client_url_from_path2(&URL, intPath.c_str(), ctx,
                                          subPool.getPool(),
                                          subPool.getPool()),
                NULL);

    if (URL == NULL)
    {
        SVN_JNI_ERR(svn_error_create(SVN_ERR_UNVERSIONED_RESOURCE, NULL,
                                     _("Either a URL or versioned item is required.")),
                    NULL);
    }

    SVN_JNI_ERR(svn_client_revprop_get(name, &propval, URL,
                                       rev.revision(), &set_rev, ctx,
                                       subPool.getPool()),
                NULL);
    if (propval == NULL)
        return NULL;

    return JNIUtil::makeJByteArray((const signed char *)propval->data,
                                   propval->len);
}
void SVNClient::relocate(const char *from, const char *to, const char *path,
                         bool ignoreExternals)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(path, "path", );
    SVN_JNI_NULL_PTR_EX(from, "from", );
    SVN_JNI_NULL_PTR_EX(to, "to", );
    Path intPath(path, subPool);
    SVN_JNI_ERR(intPath.error_occured(), );

    Path intFrom(from, subPool);
    SVN_JNI_ERR(intFrom.error_occured(), );

    Path intTo(to, subPool);
    SVN_JNI_ERR(intTo.error_occured(), );

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    SVN_JNI_ERR(svn_client_relocate2(intPath.c_str(), intFrom.c_str(),
                                     intTo.c_str(), ignoreExternals, ctx,
                                     subPool.getPool()), );
}

void SVNClient::blame(const char *path, Revision &pegRevision,
                      Revision &revisionStart, Revision &revisionEnd,
                      bool ignoreMimeType, bool includeMergedRevisions,
                      BlameCallback *callback)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(path, "path", );
    apr_pool_t *pool = subPool.getPool();
    Path intPath(path, subPool);
    SVN_JNI_ERR(intPath.error_occured(), );

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    SVN_JNI_ERR(svn_client_blame5(intPath.c_str(), pegRevision.revision(),
                                  revisionStart.revision(),
                                  revisionEnd.revision(),
                                  svn_diff_file_options_create(pool),
                                  ignoreMimeType, includeMergedRevisions,
                                  BlameCallback::callback, callback, ctx,
                                  pool),
        );
}

void SVNClient::addToChangelist(Targets &srcPaths, const char *changelist,
                                svn_depth_t depth, StringArray &changelists)
{
    SVN::Pool subPool(pool);
    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);

    const apr_array_header_t *srcs = srcPaths.array(subPool);
    SVN_JNI_ERR(srcPaths.error_occured(), );

    SVN_JNI_ERR(svn_client_add_to_changelist(srcs, changelist, depth,
                                             changelists.array(subPool),
                                             ctx, subPool.getPool()), );
}

void SVNClient::removeFromChangelists(Targets &srcPaths, svn_depth_t depth,
                                      StringArray &changelists)
{
    SVN::Pool subPool(pool);
    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);

    const apr_array_header_t *srcs = srcPaths.array(subPool);
    SVN_JNI_ERR(srcPaths.error_occured(), );

    SVN_JNI_ERR(svn_client_remove_from_changelists(srcs, depth,
                                                changelists.array(subPool),
                                                ctx, subPool.getPool()), );
}

void SVNClient::getChangelists(const char *rootPath,
                               StringArray &changelists,
                               svn_depth_t depth,
                               ChangelistCallback *callback)
{
    SVN::Pool subPool(pool);
    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);

    SVN_JNI_ERR(svn_client_get_changelists(rootPath,
                                           changelists.array(subPool),
                                           depth, ChangelistCallback::callback,
                                           callback, ctx, subPool.getPool()),
                );
}

void SVNClient::lock(Targets &targets, const char *comment, bool force)
{
    SVN::Pool subPool(pool);
    const apr_array_header_t *targetsApr = targets.array(subPool);
    SVN_JNI_ERR(targets.error_occured(), );
    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);

    SVN_JNI_ERR(svn_client_lock(targetsApr, comment, force, ctx,
                                subPool.getPool()), );
}

void SVNClient::unlock(Targets &targets, bool force)
{
    SVN::Pool subPool(pool);

    const apr_array_header_t *targetsApr = targets.array(subPool);
    SVN_JNI_ERR(targets.error_occured(), );
    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    SVN_JNI_ERR(svn_client_unlock((apr_array_header_t*)targetsApr, force,
                                  ctx, subPool.getPool()), );
}
void SVNClient::setRevProperty(const char *path,
                               const char *name, Revision &rev,
                               const char *value, const char *original_value,
                               bool force)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(path, "path", );
    SVN_JNI_NULL_PTR_EX(name, "name", );
    Path intPath(path, subPool);
    SVN_JNI_ERR(intPath.error_occured(), );

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    const char *URL;
    SVN_JNI_ERR(svn_client_url_from_path2(&URL, intPath.c_str(), ctx,
                                          subPool.getPool(),
                                          subPool.getPool()), );

    if (URL == NULL)
    {
        SVN_JNI_ERR(svn_error_create(SVN_ERR_UNVERSIONED_RESOURCE, NULL,
                                     _("Either a URL or versioned item is required.")),
            );
    }

    svn_string_t *val = svn_string_create(value, subPool.getPool());
    svn_string_t *orig_val;
    if (original_value != NULL)
      orig_val = svn_string_create(original_value, subPool.getPool());
    else
      orig_val = NULL;

    svn_revnum_t set_revision;
    SVN_JNI_ERR(svn_client_revprop_set2(name, val, orig_val, URL, rev.revision(),
                                        &set_revision, force, ctx,
                                        subPool.getPool()), );
}

jstring SVNClient::getVersionInfo(const char *path, const char *trailUrl,
                                  bool lastChanged)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(path, "path", NULL);

    Path intPath(path, subPool);
    SVN_JNI_ERR(intPath.error_occured(), NULL);

    int wc_format;
    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return NULL;
    SVN_JNI_ERR(svn_wc_check_wc2(&wc_format, ctx->wc_ctx, intPath.c_str(),
                                 subPool.getPool()),
                NULL);

    if (! wc_format)
    {
        svn_node_kind_t kind;
        SVN_JNI_ERR(svn_io_check_path(intPath.c_str(), &kind,
                                      subPool.getPool()),
                    NULL);
        if (kind == svn_node_dir)
        {
            return JNIUtil::makeJString("exported");
        }
        else
        {
            char *message = JNIUtil::getFormatBuffer();
            apr_snprintf(message, JNIUtil::formatBufferSize,
                         _("'%s' not versioned, and not exported\n"), path);
            return JNIUtil::makeJString(message);
        }
    }

    svn_wc_revision_status_t *result;
    const char *local_abspath;

    SVN_JNI_ERR(svn_dirent_get_absolute(&local_abspath, intPath.c_str(),
                                        subPool.getPool()), NULL);
    SVN_JNI_ERR(svn_wc_revision_status2(&result, ctx->wc_ctx, local_abspath,
                                        trailUrl, lastChanged,
                                        ctx->cancel_func, ctx->cancel_baton,
                                        subPool.getPool(),
                                        subPool.getPool()), NULL);

    std::ostringstream value;
    value << result->min_rev;
    if (result->min_rev != result->max_rev)
    {
        value << ":";
        value << result->max_rev;
    }
    if (result->modified)
        value << "M";
    if (result->switched)
        value << "S";
    if (result->sparse_checkout)
        value << "P";

    return JNIUtil::makeJString(value.str().c_str());
}

void SVNClient::upgrade(const char *path)
{
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(path, "path", );

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    Path checkedPath(path, subPool);
    SVN_JNI_ERR(checkedPath.error_occured(), );

    SVN_JNI_ERR(svn_client_upgrade(path, ctx, subPool.getPool()), );
}

jobject SVNClient::revProperties(const char *path, Revision &revision)
{
    apr_hash_t *props;
    SVN::Pool subPool(pool);
    SVN_JNI_NULL_PTR_EX(path, "path", NULL);
    Path intPath(path, subPool);
    SVN_JNI_ERR(intPath.error_occured(), NULL);

    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    const char *URL;
    svn_revnum_t set_rev;
    SVN_JNI_ERR(svn_client_url_from_path2(&URL, intPath.c_str(), ctx,
                                          subPool.getPool(),
                                          subPool.getPool()),
                NULL);

    if (ctx == NULL)
        return NULL;

    SVN_JNI_ERR(svn_client_revprop_list(&props, URL, revision.revision(),
                                        &set_rev, ctx, subPool.getPool()),
                NULL);

    return CreateJ::PropertyMap(props);
}

struct info_baton
{
    std::vector<info_entry> infoVect;
    apr_pool_t *pool;
};

void
SVNClient::info2(const char *path, Revision &revision, Revision &pegRevision,
                 svn_depth_t depth, StringArray &changelists,
                 InfoCallback *callback)
{
    SVN_JNI_NULL_PTR_EX(path, "path", );

    SVN::Pool subPool(pool);
    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    Path checkedPath(path, subPool);
    SVN_JNI_ERR(checkedPath.error_occured(), );

    SVN_JNI_ERR(svn_client_info3(checkedPath.c_str(),
                                 pegRevision.revision(),
                                 revision.revision(),
                                 depth, FALSE, TRUE,
                                 changelists.array(subPool),
                                 InfoCallback::callback, callback,
                                 ctx, subPool.getPool()), );
}

void
SVNClient::patch(const char *patchPath, const char *targetPath, bool dryRun,
                 int stripCount, bool reverse, bool ignoreWhitespace,
                 bool removeTempfiles, PatchCallback *callback)
{
    SVN_JNI_NULL_PTR_EX(patchPath, "patchPath", );
    SVN_JNI_NULL_PTR_EX(targetPath, "targetPath", );

    SVN::Pool subPool(pool);
    svn_client_ctx_t *ctx = context.getContext(NULL, subPool);
    if (ctx == NULL)
        return;

    Path checkedPatchPath(patchPath, subPool);
    SVN_JNI_ERR(checkedPatchPath.error_occured(), );
    Path checkedTargetPath(targetPath, subPool);
    SVN_JNI_ERR(checkedTargetPath.error_occured(), );

    // Should parameterize the following, instead of defaulting to FALSE
    SVN_JNI_ERR(svn_client_patch(checkedPatchPath.c_str(),
                                 checkedTargetPath.c_str(),
                                 dryRun, stripCount, reverse,
                                 ignoreWhitespace, removeTempfiles,
                                 PatchCallback::callback, callback,
                                 ctx, subPool.getPool()), );
}

ClientContext &
SVNClient::getClientContext()
{
    return context;
}

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
 */

package org.apache.subversion.javahl;

import java.util.Map;
import java.util.EventObject;
import org.apache.subversion.javahl.callback.ClientNotifyCallback;
import org.apache.subversion.javahl.types.*;

/**
 * The event passed to the {@link ClientNotifyCallback#onNotify}
 * API to notify {@link ISVNClient} of relevant events.
 */
public class ClientNotifyInformation extends EventObject
{
    // Update the serialVersionUID when there is a incompatible change
    // made to this class.  See any of the following, depending upon
    // the Java release.
    // http://java.sun.com/j2se/1.3/docs/guide/serialization/spec/version.doc7.html
    // http://java.sun.com/j2se/1.4/pdf/serial-spec.pdf
    // http://java.sun.com/j2se/1.5.0/docs/guide/serialization/spec/version.html#6678
    // http://java.sun.com/javase/6/docs/platform/serialization/spec/version.html#6678
    private static final long serialVersionUID = 1L;

    /**
     * The {@link Action} which triggered this event.
     */
    private Action action;

    /**
     * The {@link NodeKind} of the item.
     */
    private NodeKind kind;

    /**
     * The MIME type of the item.
     */
    private String mimeType;

    /**
     * Any lock for the item.
     */
    private Lock lock;

    /**
     * Any error message for the item.
     */
    private String errMsg;

    /**
     * The {@link Status} of the content of the item.
     */
    private Status contentState;

    /**
     * The {@link Status} of the properties of the item.
     */
    private Status propState;

    /**
     * The {@link LockStatus} of the lock of the item.
     */
    private LockStatus lockState;

    /**
     * The revision of the item.
     */
    private long revision;

    /**
     * The name of the changelist.
     */
    private String changelistName;

    /**
     * The range of the merge just beginning to occur.
     */
    private RevisionRange mergeRange;

    /**
     * A common absolute path prefix that can be subtracted from .path.
     */
    private String pathPrefix;

    private String propName;

    private Map<String, String> revProps;

    long oldRevision;

    long hunkOriginalStart;

    long hunkOriginalLength;

    long hunkModifiedStart;

    long hunkModifiedLength;

    long hunkMatchedLine;

    int hunkFuzz;

    /**
     * This constructor is to be used by the native code.
     *
     * @param path The path of the item, which is the source of the event.
     * @param action The {@link Action} which triggered this event.
     * @param kind The {@link NodeKind} of the item.
     * @param mimeType The MIME type of the item.
     * @param lock Any lock for the item.
     * @param errMsg Any error message for the item.
     * @param contentState The {@link Status} of the content of
     * the item.
     * @param propState The {@link Status} of the properties of
     * the item.
     * @param lockState The {@link LockStatus} of the lock of the item.
     * @param revision The revision of the item.
     * @param changelistName The name of the changelist.
     * @param mergeRange The range of the merge just beginning to occur.
     * @param pathPrefix A common path prefix.
     */
    public ClientNotifyInformation(String path, Action action, NodeKind kind,
                             String mimeType, Lock lock, String errMsg,
                             Status contentState, Status propState,
                             LockStatus lockState, long revision,
                             String changelistName, RevisionRange mergeRange,
                             String pathPrefix, String propName,
                             Map<String, String> revProps, long oldRevision,
                             long hunkOriginalStart, long hunkOriginalLength,
                             long hunkModifiedStart, long hunkModifiedLength,
                             long hunkMatchedLine, int hunkFuzz)
    {
        super(path == null ? "" : path);
        this.action = action;
        this.kind = kind;
        this.mimeType = mimeType;
        this.lock = lock;
        this.errMsg = errMsg;
        this.contentState = contentState;
        this.propState = propState;
        this.lockState = lockState;
        this.revision = revision;
        this.changelistName = changelistName;
        this.mergeRange = mergeRange;
        this.pathPrefix = pathPrefix;
        this.propName = propName;
        this.revProps = revProps;
        this.oldRevision = oldRevision;
        this.hunkOriginalStart = hunkOriginalStart;
        this.hunkOriginalLength = hunkOriginalLength;
        this.hunkModifiedStart = hunkModifiedStart;
        this.hunkModifiedLength = hunkModifiedLength;
        this.hunkMatchedLine = hunkMatchedLine;
        this.hunkFuzz = hunkFuzz;
    }

    /**
     * @return The path of the item, which is the source of the event.
     */
    public String getPath()
    {
        return (String) super.source;
    }

    /**
     * @return The {@link Action} which triggered this event.
     */
    public Action getAction()
    {
        return action;
    }

    /**
     * @return The {@link NodeKind} of the item.
     */
    public NodeKind getKind()
    {
        return kind;
    }

    /**
     * @return The MIME type of the item.
     */
    public String getMimeType()
    {
        return mimeType;
    }

    /**
     * @return Any lock for the item.
     */
    public Lock getLock()
    {
        return lock;
    }

    /**
     * @return Any error message for the item.
     */
    public String getErrMsg()
    {
        return errMsg;
    }

    /**
     * @return The {@link Status} of the content of the item.
     */
    public Status getContentState()
    {
        return contentState;
    }

    /**
     * @return The {@link Status} of the properties of the item.
     */
    public Status getPropState()
    {
        return propState;
    }

    /**
     * @return The {@link LockStatus} of the lock of the item.
     */
    public LockStatus getLockState()
    {
        return lockState;
    }

    /**
     * @return The revision of the item.
     */
    public long getRevision()
    {
        return revision;
    }

    /**
     * @return The name of the changelist.
     */
    public String getChangelistName()
    {
        return changelistName;
    }

    /**
     * @return The range of the merge just beginning to occur.
     */
    public RevisionRange getMergeRange()
    {
        return mergeRange;
    }

    /**
     * @return The common absolute path prefix.
     */
    public String getPathPrefix()
    {
        return pathPrefix;
    }

    public String getPropName()
    {
        return propName;
    }

    public Map<String, String> getRevProps()
    {
        return revProps;
    }

    public long getOldRevision()
    {
        return oldRevision;
    }

    public long getHunkOriginalStart()
    {
        return hunkOriginalStart;
    }

    public long getHunkOriginalLength()
    {
        return hunkOriginalLength;
    }

    public long getHunkModifiedStart()
    {
        return hunkModifiedStart;
    }

    public long getHunkModifiedLength()
    {
        return hunkModifiedLength;
    }

    public long getHunkMatchedLine()
    {
        return hunkMatchedLine;
    }

    public int getHunkFuzz()
    {
        return hunkFuzz;
    }

    /**
     * The type of action triggering the notification
     */
    public enum Action
    {
        /** Adding a path to revision control. */
        add             ("add"),

        /** Copying a versioned path. */
        copy            ("copy"),

        /** Deleting a versioned path. */
        delete          ("delete"),

        /** Restoring a missing path from the pristine text-base. */
        restore         ("restore"),

        /** Reverting a modified path. */
        revert          ("revert"),

        /** A revert operation has failed. */
        failed_revert   ("failed revert"),

        /** Resolving a conflict. */
        resolved        ("resolved"),

        /** Skipping a path. */
        skip            ("skip"),

        /* The update actions are also used for checkouts, switches, and
           merges. */

        /** Got a delete in an update. */
        update_delete   ("update delete"),

        /** Got an add in an update. */
        update_add      ("update add"),

        /** Got any other action in an update. */
        update_update   ("update modified"),

        /** The last notification in an update */
        update_completed ("update completed"),

        /** About to update an external module, use for checkouts and switches
         *  too, end with @c svn_wc_update_completed.
         */
        update_external ("update external"),

        /** The last notification in a status (including status on externals).
         */
        status_completed ("status completed"),

        /** Running status on an external module. */
        status_external ("status external"),

        /** Committing a modification. */
        commit_modified ("sending modified"),

        /** Committing an addition. */
        commit_added    ("sending added"),

        /** Committing a deletion. */
        commit_deleted  ("sending deleted"),

        /** Committing a replacement. */
        commit_replaced ("sending replaced"),

        /** Transmitting post-fix text-delta data for a file. */
        commit_postfix_txdelta ("transfer"),

        /** Processed a single revision's blame. */
        blame_revision  ("blame revision processed"),

        /** Locking a path */
        locked          ("locked"),

        /** Unlocking a path */
        unlocked        ("unlocked"),

        /** Failed to lock a path */
        failed_lock     ("locking failed"),

        /** Failed to unlock a path */
        failed_unlock   ("unlocking failed"),

        /** Tried adding a path that already exists.  */
        exists          ("path exists"),

        /** Set the changelist for a path.  */
        changelist_set  ("changelist set"),

        /** Clear the changelist for a path.  */
        changelist_clear ("changelist cleared"),

        /** A path has moved to another changelist.  */
        changelist_moved    ("changelist moved"),

        /** A merge operation has begun.  */
        merge_begin     ("merge begin"),

        /** A merge operation from a foreign repository has begun.  */
        foreign_merge_begin ("foreign merge begin"),

        /** Got a replaced in an update.  */
        update_replaced ("replaced"),

        /** Property added.  */
        property_added  ("property added"),

        /** Property modified.  */
        property_modified ("property modified"),

        /** Property deleted.  */
        property_deleted ("property deleted"),

        /** Property delete nonexistent.  */
        property_deleted_nonexistent ("nonexistent property deleted"),

        /** Revision property set.  */
        revprop_set     ("revprop set"),

        /** Revision property deleted.  */
        revprop_deleted ("revprop deleted"),

        /** The last notification in a merge.  */
        merge_completed ("merge completed"),

        /** The path is a tree-conflict victim of the intended action */
        tree_conflict   ("tree conflict"),

        /** The path is a subdirectory referenced in an externals definition
          * which is unable to be operated on.  */
        failed_external ("failed external"),

        /** Starting an update operation */
        update_started ("update started"),

        /** Skipping an obstruction working copy */
        update_skip_obstruction ("update skip obstruction"),

        /** Skipping a working only node */
        update_skip_working_only ("update skip working only"),

        /** Skipped a file or directory to which access couldn't be obtained */
        update_skip_access_denied ("update skip access denied"),

        /** An update operation removed an external working copy.  */
        update_external_removed ("update external removed"),

        /** Applying a shadowed add */
        update_shadowed_add ("update shadowed add"),

        /** Applying a shadowed update */
        update_shadowed_update ("update shadowed update"),

        /** Applying a shadowed delete */
        update_shadowed_delete ("update shadowed delete"),

        /** The mergeinfo on path was updated.  */
        merge_record_info   ("merge record info"),

        /** An working copy directory was upgraded to the latest format.  */
        upgraded_path       ("upgraded path"),

        /** Mergeinfo describing a merge was recorded.  */
        merge_record_info_begin     ("merge record info begin"),

        /** Mergeinfo was removed due to elision.  */
        merge_elide_info    ("Merge elide info"),

        /** A file in the working copy was patched.  */
        patch       ("patch"),

        /** A hunk from a patch was applied.  */
        patch_applied_hunk  ("patch applied hunk"),

        /** A hunk from a patch was rejected.  */
        patch_rejected_hunk ("patch rejected hunk"),

        /** A hunk from a patch was found to be already applied. */
        patch_hunk_already_applied ("patch hunk already applied"),

        /** Committing a non-overwriting copy (path is the target of the
          * copy, not the source). */
        commit_copied   ("commit copied"),

        /** Committing an overwriting (replace) copy (path is the target of
          * the copy, not the source).  */
        commit_copied_replaced  ("commit copied replaced"),

        /** The server has instructed the client to follow a URL
          * redirection. */
        url_redirect    ("url redirect"),

        /** The operation was attempted on a path which doesn't exist. */
        path_nonexistent ("path nonexistent"),

        /** Removing a path by excluding it. */
        exclude ("exclude"),

        /** Operation failed because the node remains in conflict */
        failed_conflict ("failed conflict"),

        /** Operation failed because an added node is missing */
        failed_missing ("failed missing"),

        /** Operation failed because a node is out of date */
        failed_out_of_date ("failed out of date"),

        /** Operation failed because an added parent is not selected */
        failed_no_parent ("failed no parent"),

        /** Operation failed because a node is locked */
        failed_locked ("failed by lock"),

        /** Operation failed because the operation was forbidden */
        failed_forbidden_by_server ("failed forbidden by server"),

        /** Operation skipped the path because it was conflicted */
        skip_conflicted ("skipped conflicted path");

        /**
         * The description of the action.
         */
        private String description;

        Action(String description)
        {
            this.description = description;
        }

        public String toString()
        {
            return description;
        }
    }

    public enum Status
    {
        /** It not applicable*/
        inapplicable    ("inapplicable"),

        /** Notifier doesn't know or isn't saying. */
        unknown         ("unknown"),

        /** The state did not change. */
        unchanged       ("unchanged"),

        /** The item wasn't present. */
        missing         ("missing"),

        /** An unversioned item obstructed work. */
        obstructed      ("obstructed"),

        /** Pristine state was modified. */
        changed         ("changed"),

        /** Modified state had mods merged in. */
        merged          ("merged"),

        /** Modified state got conflicting mods. */
        conflicted      ("conflicted");

        /**
         * The description of the action.
         */
        private String description;

        Status(String description)
        {
            this.description = description;
        }

        public String toString()
        {
            return description;
        }
    }

    public enum LockStatus
    {
        /** does not make sense for this operation */
        inapplicable    ("inapplicable"),

        /** unknown lock state */
        unknown         ("unknown"),

        /** the lock change did not change */
        unchanged       ("unchanged"),

        /** the item was locked */
        locked          ("locked"),

        /** the item was unlocked */
        unlocked        ("unlocked");

        /**
         * The description of the action.
         */
        private String description;

        LockStatus(String description)
        {
            this.description = description;
        }

        public String toString()
        {
            return description;
        }
    }
}

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

package org.apache.subversion.javahl.types;

import java.util.Date;

import org.apache.subversion.javahl.ConflictDescriptor;

/**
 * Subversion status API.
 * This describes the status of one subversion item (file or directory) in
 * the working copy. Will be returned by SVNClient.status or
 * SVNClient.singleStatus
 */
public class Status implements java.io.Serializable
{
    // Update the serialVersionUID when there is a incompatible change
    // made to this class.  See any of the following, depending upon
    // the Java release.
    // http://java.sun.com/j2se/1.3/docs/guide/serialization/spec/version.doc7.html
    // http://java.sun.com/j2se/1.4/pdf/serial-spec.pdf
    // http://java.sun.com/j2se/1.5.0/docs/guide/serialization/spec/version.html#6678
    // http://java.sun.com/javase/6/docs/platform/serialization/spec/version.html#6678
    private static final long serialVersionUID = 2L;

    /**
     * the url for accessing the item
     */
    private String url;

    /**
     * the path in the working copy
     */
    private String path;

    /**
     * kind of the item (file, directory or unknonw)
     */
    private NodeKind nodeKind;

    /**
     * the base revision of the working copy
     */
    private long revision;

    /**
     * the last revision the item was changed before base
     */
    private long lastChangedRevision;

    /**
     * the last date the item was changed before base (represented in
     * microseconds since the epoch)
     */
    private long lastChangedDate;

    /**
     * the last author of the last change before base
     */
    private String lastCommitAuthor;

    /**
     * the file or directory status (See StatusKind)
     */
    private Kind textStatus;

    /**
     * the status of the properties (See StatusKind)
     */
    private Kind propStatus;

    /**
     * flag is this item is locked locally by subversion
     * (running or aborted operation)
     */
    private boolean locked;

    /**
     * has this item be copied from another item
     */
    private boolean copied;

    /**
     * has the url of the item be switch
     */
    private boolean switched;

    /**
     * has the item is a file external
     */
    private boolean fileExternal;

    /**
     * is this item in a conflicted state
     */
    private boolean isConflicted;

    /**
     * the file or directory status of base (See StatusKind)
     */
    private Kind repositoryTextStatus;

    /**
     * the status of the properties base (See StatusKind)
     */
    private Kind repositoryPropStatus;

    /**
     * the current lock
     */
    private Lock localLock;

    /**
     * the lock in the repository
     */
    private Lock reposLock;

    /**
     * Set to the youngest committed revision, or {@link
     * Revision#SVN_INVALID_REVNUM} if not out of date.
     */
    private long reposLastCmtRevision = Revision.SVN_INVALID_REVNUM;

    /**
     * Set to the most recent commit date, or 0 if not out of date.
     */
    private long reposLastCmtDate = 0;

    /**
     * Set to the node kind of the youngest commit, or {@link
     * NodeKind#none} if not out of date.
     */
    private NodeKind reposKind = NodeKind.none;

    /**
     * Set to the user name of the youngest commit, or
     * <code>null</code> if not out of date.
     */
    private String reposLastCmtAuthor;

    /**
     * Set to the changelist of the item, or <code>null</code> if not under
     * version control.
     */
    private String changelist;

    /**
     * this constructor should only called from JNI code
     * @param path                  the file system path of item
     * @param url                   the url of the item
     * @param nodeKind              kind of item (directory, file or unknown
     * @param revision              the revision number of the base
     * @param lastChangedRevision   the last revision this item was changed
     * @param lastChangedDate       the last date this item was changed
     * @param lastCommitAuthor      the author of the last change
     * @param textStatus            the file or directory status (See
     *                              StatusKind)
     * @param propStatus            the property status (See StatusKind)
     * @param repositoryTextStatus  the file or directory status of the base
     * @param repositoryPropStatus  the property status of the base
     * @param locked                if the item is locked (running or aborted
     *                              operation)
     * @param copied                if the item is copy
     * @param isConflicted          if the item is part of a conflict
     * @param conflictDescriptor    the description of the tree conflict
     * @param conflictOld           in case of conflict, the file name of the
     *                              the common base version
     * @param conflictNew           in case of conflict, the file name of new
     *                              repository version
     * @param conflictWorking       in case of conflict, the file name of the
     *                              former working copy version
     * @param switched              flag if the node has been switched in the
     *                              path
     * @param fileExternal          flag if the node is a file external
     * @param localLock             the current lock
     * @param reposLock             the lock as stored in the repository if
     *                              any
     * @param reposLastCmtRevision  the youngest revision, if out of date
     * @param reposLastCmtDate      the last commit date, if out of date
     * @param reposKind             the kind of the youngest revision, if
     *                              out of date
     * @param reposLastCmtAuthor    the author of the last commit, if out of
     *                              date
     * @param changelist            the changelist the item is a member of
     */
    public Status(String path, String url, NodeKind nodeKind, long revision,
                  long lastChangedRevision, long lastChangedDate,
                  String lastCommitAuthor, Kind textStatus, Kind propStatus,
                  Kind repositoryTextStatus, Kind repositoryPropStatus,
                  boolean locked, boolean copied, boolean isConflicted,
                  boolean switched, boolean fileExternal, Lock localLock,
                  Lock reposLock, long reposLastCmtRevision,
                  long reposLastCmtDate, NodeKind reposKind,
                  String reposLastCmtAuthor, String changelist)
    {
        this.path = path;
        this.url = url;
        this.nodeKind = (nodeKind != null ? nodeKind : NodeKind.unknown);
        this.revision = revision;
        this.lastChangedRevision = lastChangedRevision;
        this.lastChangedDate = lastChangedDate;
        this.lastCommitAuthor = lastCommitAuthor;
        this.textStatus = textStatus;
        this.propStatus = propStatus;
        this.locked = locked;
        this.copied = copied;
        this.isConflicted = isConflicted;
        this.repositoryTextStatus = repositoryTextStatus;
        this.repositoryPropStatus = repositoryPropStatus;
        this.switched = switched;
        this.fileExternal = fileExternal;
        this.localLock = localLock;
        this.reposLock = reposLock;
        this.reposLastCmtRevision = reposLastCmtRevision;
        this.reposLastCmtDate = reposLastCmtDate;
        this.reposKind = reposKind;
        this.reposLastCmtAuthor = reposLastCmtAuthor;
        this.changelist = changelist;
    }

    /**
     * Returns the file system path of the item
     * @return path of status entry
     */
    public String getPath()
    {
        return path;
    }

    /**
     * Returns the revision as a Revision object
     * @return revision if versioned, otherwise SVN_INVALID_REVNUM
     */
    public Revision.Number getRevision()
    {
        return Revision.createNumber(revision);
    }

    /**
     * Returns the revision as a long integer
     * @return revision if versioned, otherwise SVN_INVALID_REVNUM
     */
    public long getRevisionNumber()
    {
        return revision;
    }

    /**
     * Returns the last date the item was changed or null
     * @return the last time the item was changed or null if not
     * available
     */
    public Date getLastChangedDate()
    {
        return microsecondsToDate(lastChangedDate);
    }

    /**
     * Returns the last date the item was changed measured in the
     * number of microseconds since 00:00:00 January 1, 1970 UTC.
     * @return the last time the item was changed.
     */
    public long getLastChangedDateMicros()
    {
        return lastChangedDate;
    }

    /**
     * Returns the author of the last changed or null
     * @return name of author if versioned, null otherwise
     */
    public String getLastCommitAuthor()
    {
        return lastCommitAuthor;
    }

    /**
     * Returns the status of the item (See StatusKind)
     * @return file status property enum of the "textual" component.
     */
    public Kind getTextStatus()
    {
        return textStatus;
    }

    /**
     * Returns the status of the item as text.
     * @return english text
     */
    public String getTextStatusDescription()
    {
        return textStatus.toString();
    }

    /**
     * Returns the status of the properties (See Status Kind)
     * @return file status property enum of the "property" component.
     */
    public Kind getPropStatus()
    {
        return propStatus;
    }

    /**
     * Returns the status of the properties as text
     * @return english text
     */
    public String getPropStatusDescription()
    {
        return propStatus.toString();
    }

    /**
     * Returns the status of the item in the repository (See StatusKind)
     * @return file status property enum of the "textual" component in the
     * repository.
     */
    public Kind getRepositoryTextStatus()
    {
        return repositoryTextStatus;
    }

    /**
     * Returns test status of the properties in the repository (See StatusKind)
     * @return file status property enum of the "property" component im the
     * repository.
     */
    public Kind getRepositoryPropStatus()
    {
        return repositoryPropStatus;
    }

    /**
     * Returns if the item is locked (running or aborted subversion operation)
     * @return true if locked
     */
    public boolean isLocked()
    {
        return locked;
    }

    /**
     * Returns if the item has been copied
     * @return true if copied
     */
    public boolean isCopied()
    {
        return copied;
    }

    /**
     * Returns the URI to where the item might exist in the
     * repository.  We say "might" because the item might exist in
     * your working copy, but have been deleted from the repository.
     * Or it might exist in the repository, but your working copy
     * might not yet contain it (because the WC is not up to date).
     * @return URI in repository, or <code>null</code> if the item
     * exists in neither the repository nor the WC.
     */
    public String getUrl()
    {
        return url;
    }


    /**
     * Returns the last revision the file was changed as a Revision object
     * @return last changed revision
     */
    public Revision.Number getLastChangedRevision()
    {
        return Revision.createNumber(lastChangedRevision);
    }

    /**
     * Returns the last revision the file was changed as a long integer
     * @return last changed revision
     */
    public long getLastChangedRevisionNumber()
    {
        return lastChangedRevision;
    }

    /**
     * Returns the kind of the node (file, directory or unknown, see NodeKind)
     * @return the node kind
     */
    public NodeKind getNodeKind()
    {
        return nodeKind;
    }

    /**
     * Returns if the repository url has been switched
     * @return is the item has been switched
     */
    public boolean isSwitched()
    {
        return switched;
    }

    /**
     * Returns if the item is a file external
     * @return is the item is a file external
     */
    public boolean isFileExternal()
    {
        return fileExternal;
    }

    /**
     * Returns if is managed by svn (added, normal, modified ...)
     * @return if managed by svn
     */
    public boolean isManaged()
    {
        Kind status = getTextStatus();
        return (status != Status.Kind.unversioned &&
                status != Status.Kind.none &&
                status != Status.Kind.ignored);
    }

    /**
     * Returns if the resource has a remote counter-part
     * @return has version in repository
     */
    public boolean hasRemote()
    {
        return (isManaged() && getTextStatus() != Status.Kind.added);
    }

    /**
     * Returns if the resource just has been added
     * @return if added
     */
    public boolean isAdded()
    {
        return getTextStatus() == Status.Kind.added;
    }

    /**
     * Returns if the resource is schedules for delete
     * @return if deleted
     */
    public boolean isDeleted()
    {
        return getTextStatus() == Status.Kind.deleted;
    }

    /**
     * Returns if the resource has been merged
     * @return if merged
     */
    public boolean isMerged()
    {
        return getTextStatus() == Status.Kind.merged;
    }

    /**
     * Returns if the resource is ignored by svn (only returned if noIgnore
     * is set on SVNClient.list)
     * @return if ignore
     */
    public boolean isIgnored()
    {
        return getTextStatus() == Status.Kind.ignored;
    }

    /**
     * Returns if the resource itself is modified
     * @return if modified
     */
    public boolean isModified()
    {
        return getTextStatus() == Status.Kind.modified;
    }

    /**
     * Returns the local lock
     * @return the local lock
     */
    public Lock getLocalLock()
    {
        return localLock;
    }

    /**
     * Returns the lock as in the repository
     * @return the lock as in the repository
     */
    public Lock getReposLock()
    {
        return reposLock;
    }

    /**
     * @return The last committed revision, or {@link
     * Revision#SVN_INVALID_REVNUM} if up to date.
     */
    public Revision.Number getReposLastCmtRevision()
    {
        return Revision.createNumber(reposLastCmtRevision);
    }

    /**
     * @return The last committed revision as a long integer, or
     * <code>-1</code> if up to date.
     */
    public long getReposLastCmtRevisionNumber()
    {
        return reposLastCmtRevision;
    }

    /**
     * @return The last committed date, or <code>null</code> if up to
     * date.
     */
    public Date getReposLastCmtDate()
    {
        return microsecondsToDate(reposLastCmtDate);
    }

    /**
     * Return the last committed date measured in the number of
     * microseconds since 00:00:00 January 1, 1970 UTC.
     * @return the last committed date
     */
    public long getReposLastCmtDateMicros()
    {
        return reposLastCmtDate;
    }

    /**
     * @return The node kind (e.g. file, directory, etc.), or
     * <code>null</code> if up to date.
     */
    public NodeKind getReposKind()
    {
        return reposKind;
    }

    /**
     * @return The author of the last commit, or <code>null</code> if
     * up to date.
     */
    public String getReposLastCmtAuthor()
    {
        return reposLastCmtAuthor;
    }

    /**
     * @return the changelist name
     */
    public String getChangelist()
    {
        return changelist;
    }

    /**
     * @return the conflicted state
     */
    public boolean isConflicted()
    {
        return isConflicted;
    }

    /**
     * Converts microseconds since the epoch to a Date object.
     *
     * @param micros Microseconds since the epoch.
     * @return A Date object, or <code>null</code> if
     * <code>micros</code> was zero.
     */
    private static Date microsecondsToDate(long micros)
    {
        return (micros == 0 ? null : new Date(micros / 1000));
    }

    /**
     * class for kind status of the item or its properties
     * the constants are defined in the interface StatusKind for building
     * reasons
     */
    public enum Kind
    {
        /** does not exist */
        none        ("non-svn"),

        /** is not a versioned thing in this wc */
        unversioned ("unversioned"),

        /** exists, but uninteresting */
        normal      ("normal"),

        /** is scheduled for additon */
        added       ("added"),

        /** under v.c., but is missing */
        missing     ("missing"),

        /** scheduled for deletion */
        deleted     ("deleted"),

        /** was deleted and then re-added */
        replaced    ("replaced"),

        /** text or props have been modified */
        modified    ("modified"),

        /** local mods received repos mods */
        merged      ("merged"),

        /** local mods received conflicting repos mods */
        conflicted  ("conflicted"),

        /** a resource marked as ignored */
        ignored     ("ignored"),

        /** an unversioned resource is in the way of the versioned resource */
        obstructed  ("obstructed"),

        /** an unversioned path populated by an svn:externals property */
        external    ("external"),

        /** a directory doesn't contain a complete entries list */
        incomplete  ("incomplete");

        /**
         * The description of the action.
         */
        private String description;

        Kind(String description)
        {
            this.description = description;
        }

        public String toString()
        {
            return description;
        }
    }
}

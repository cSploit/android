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

package org.tigris.subversion.javahl;

import java.util.Date;

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
    private int nodeKind;

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
    private int textStatus;

    /**
     * the status of the properties (See StatusKind)
     */
    private int propStatus;

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
     * @since 1.6
     * is this item in a tree conflicted state
     */
    private boolean treeConflicted;

    /**
     * @since 1.6
     * description of the tree conflict
     */
    private ConflictDescriptor conflictDescriptor;

    /**
     * the file or directory status of base (See StatusKind)
     */
    private int repositoryTextStatus;

    /**
     * the status of the properties base (See StatusKind)
     */
    private int repositoryPropStatus;

    /**
     * if there is a conflict, the filename of the new version
     * from the repository
     */
    private String conflictNew;

    /**
     * if there is a conflict, the filename of the common base version
     * from the repository
     */
    private String conflictOld;

    /**
     * if there is a conflict, the filename of the former working copy
     * version
     */
    private String conflictWorking;

    /**
     * if copied, the url of the copy source
     */
    private String urlCopiedFrom;

    /**
     * if copied, the revision number of the copy source
     */
    private long revisionCopiedFrom;

    /**
     * @since 1.2
     * token specified for the lock (null if not locked)
     */
    private String lockToken;

    /**
     * @since 1.2
     * owner of the lock (null if not locked)
     */
    private String lockOwner;

    /**
     * @since 1.2
     * comment specified for the lock (null if not locked)
     */
    private String lockComment;

    /**
     * @since 1.2
     * date of the creation of the lock (represented in microseconds
     * since the epoch)
     */
    private long lockCreationDate;

    /**
     * @since 1.2
     * the lock in the repository
     */
    private Lock reposLock;

    /**
     * @since 1.3
     * Set to the youngest committed revision, or {@link
     * Revision#SVN_INVALID_REVNUM} if not out of date.
     */
    private long reposLastCmtRevision = Revision.SVN_INVALID_REVNUM;

    /**
     * @since 1.3
     * Set to the most recent commit date, or 0 if not out of date.
     */
    private long reposLastCmtDate = 0;

    /**
     * @since 1.3
     * Set to the node kind of the youngest commit, or {@link
     * NodeKind#none} if not out of date.
     */
    private int reposKind = NodeKind.none;

    /**
     * @since 1.3
     * Set to the user name of the youngest commit, or
     * <code>null</code> if not out of date.
     */
    private String reposLastCmtAuthor;

    /**
     * @since 1.5
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
     * @param treeConflicted        if the item is part of a tree conflict
     * @param conflictDescriptor    the description of the tree conflict
     * @param conflictOld           in case of conflict, the file name of the
     *                              the common base version
     * @param conflictNew           in case of conflict, the file name of new
     *                              repository version
     * @param conflictWorking       in case of conflict, the file name of the
     *                              former working copy version
     * @param urlCopiedFrom         if copied, the url of the copy source
     * @param revisionCopiedFrom    if copied, the revision number of the copy
     *                              source
     * @param switched              flag if the node has been switched in the
     *                              path
     * @param fileExternal          flag if the node is a file external
     * @param lockToken             the token for the current lock if any
     * @param lockOwner             the owner of the current lock is any
     * @param lockComment           the comment of the current lock if any
     * @param lockCreationDate      the date, the lock was created if any
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
    public Status(String path, String url, int nodeKind, long revision,
                  long lastChangedRevision, long lastChangedDate,
                  String lastCommitAuthor, int textStatus, int propStatus,
                  int repositoryTextStatus, int repositoryPropStatus,
                  boolean locked, boolean copied, boolean treeConflicted,
                  ConflictDescriptor conflictDescriptor, String conflictOld,
                  String conflictNew, String conflictWorking,
                  String urlCopiedFrom, long revisionCopiedFrom,
                  boolean switched, boolean fileExternal, String lockToken,
                  String lockOwner, String lockComment, long lockCreationDate,
                  Lock reposLock, long reposLastCmtRevision,
                  long reposLastCmtDate, int reposKind,
                  String reposLastCmtAuthor, String changelist)
    {
        this.path = path;
        this.url = url;
        this.nodeKind = nodeKind;
        this.revision = revision;
        this.lastChangedRevision = lastChangedRevision;
        this.lastChangedDate = lastChangedDate;
        this.lastCommitAuthor = lastCommitAuthor;
        this.textStatus = textStatus;
        this.propStatus = propStatus;
        this.locked = locked;
        this.copied = copied;
        this.treeConflicted = treeConflicted;
        this.conflictDescriptor = conflictDescriptor;
        this.repositoryTextStatus = repositoryTextStatus;
        this.repositoryPropStatus = repositoryPropStatus;
        this.conflictOld = conflictOld;
        this.conflictNew = conflictNew;
        this.conflictWorking = conflictWorking;
        this.urlCopiedFrom = urlCopiedFrom;
        this.revisionCopiedFrom = revisionCopiedFrom;
        this.switched = switched;
        this.fileExternal = fileExternal;
        this.lockToken = lockToken;
        this.lockOwner = lockOwner;
        this.lockComment = lockComment;
        this.lockCreationDate = lockCreationDate;
        this.reposLock = reposLock;
        this.reposLastCmtRevision = reposLastCmtRevision;
        this.reposLastCmtDate = reposLastCmtDate;
        this.reposKind = reposKind;
        this.reposLastCmtAuthor = reposLastCmtAuthor;
        this.changelist = changelist;
    }

    /** Create an empty status struct */
    public Status(String path)
    {
        this(path, null, NodeKind.fromApache(null), Revision.SVN_INVALID_REVNUM,
             Revision.SVN_INVALID_REVNUM, 0, null, fromAStatusKind(null),
             fromAStatusKind(null), fromAStatusKind(null),
             fromAStatusKind(null), false, false, false, null,
             null, null, null, null, Revision.SVN_INVALID_REVNUM,
             false, false, null, null, null, 0, null,
             Revision.SVN_INVALID_REVNUM, 0, NodeKind.fromApache(null),
             null, null);
    }

    private void
    populateFromInfo(org.apache.subversion.javahl.SVNClient aClient,
                     String path)
        throws org.apache.subversion.javahl.ClientException
    {
        class MyInfoCallback
                implements org.apache.subversion.javahl.callback.InfoCallback
        {
          org.apache.subversion.javahl.types.Info info;

          public void singleInfo(org.apache.subversion.javahl.types.Info aInfo)
          {
            info = aInfo;
          }

          public org.apache.subversion.javahl.types.Info getInfo()
          {
            return info;
          }
        }

        MyInfoCallback callback = new MyInfoCallback();

        aClient.info2(path, null, null,
                      org.apache.subversion.javahl.types.Depth.empty, null,
                      callback);

        org.apache.subversion.javahl.types.Info aInfo = callback.getInfo();
        if (aInfo == null)
            return;

        if (aInfo.getConflicts() != null)
            for (org.apache.subversion.javahl.ConflictDescriptor conflict
                    : aInfo.getConflicts())
            {
               switch (conflict.getKind())
               {
                 case tree:
                   this.treeConflicted = true;
                   this.conflictDescriptor = new ConflictDescriptor(conflict);
                   break;

                 case text:
                   this.conflictOld = conflict.getBasePath();
                   this.conflictWorking = conflict.getMergedPath();
                   this.conflictNew = conflict.getMyPath();
                   break;

                 case property:
                   // Ignore
                   break;
               }
            }

        this.urlCopiedFrom = aInfo.getCopyFromUrl();
        this.revisionCopiedFrom = aInfo.getCopyFromRev();
    }

    void
    populateLocalLock(org.apache.subversion.javahl.types.Lock aLock)
    {
        if (aLock == null)
            return;

        this.lockToken = aLock.getToken();
        this.lockOwner = aLock.getOwner();
        this.lockComment = aLock.getComment();
        this.lockCreationDate = aLock.getCreationDate().getTime() * 1000;;
    }

    /**
     * A backward-compat wrapper.
     */
    public Status(org.apache.subversion.javahl.SVNClient aClient,
                  org.apache.subversion.javahl.types.Status aStatus)
    {
        this(aStatus.getPath(), aStatus.getUrl(),
             NodeKind.fromApache(aStatus.getNodeKind()),
             aStatus.getRevisionNumber(),
             aStatus.getLastChangedRevisionNumber(),
             aStatus.getLastChangedDateMicros(), aStatus.getLastCommitAuthor(),
             fromAStatusKind(aStatus.getTextStatus()),
             fromAStatusKind(aStatus.getPropStatus()),
             fromAStatusKind(aStatus.getRepositoryTextStatus()),
             fromAStatusKind(aStatus.getRepositoryPropStatus()),
             aStatus.isLocked(), aStatus.isCopied(), false,
             null, null, null, null, null, Revision.SVN_INVALID_REVNUM,
             aStatus.isSwitched(),
             aStatus.isFileExternal(), null, null, null, 0,
             aStatus.getReposLock() == null ? null
                : new Lock(aStatus.getReposLock()),
             aStatus.getReposLastCmtRevisionNumber(),
             aStatus.getReposLastCmtDateMicros(),
             NodeKind.fromApache(aStatus.getReposKind()),
             aStatus.getReposLastCmtAuthor(), aStatus.getChangelist());

        try {
            populateFromInfo(aClient, aStatus.getPath());
            if (aStatus.getLocalLock() != null)
                populateLocalLock(aStatus.getLocalLock());
        } catch (org.apache.subversion.javahl.ClientException ex) {
            // Ignore
        }
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
     * @since 1.5
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
    public int getTextStatus()
    {
        return textStatus;
    }

    /**
     * Returns the status of the item as text.
     * @return english text
     */
    public String getTextStatusDescription()
    {
        return Kind.getDescription(textStatus);
    }

    /**
     * Returns the status of the properties (See Status Kind)
     * @return file status property enum of the "property" component.
     */
    public int getPropStatus()
    {
        return propStatus;
    }

    /**
     * Returns the status of the properties as text
     * @return english text
     */
    public String getPropStatusDescription()
    {
        return Kind.getDescription(propStatus);
    }

    /**
     * Returns the status of the item in the repository (See StatusKind)
     * @return file status property enum of the "textual" component in the
     * repository.
     */
    public int getRepositoryTextStatus()
    {
        return repositoryTextStatus;
    }

    /**
     * Returns test status of the properties in the repository (See StatusKind)
     * @return file status property enum of the "property" component im the
     * repository.
     */
    public int getRepositoryPropStatus()
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
     * Returns in case of conflict, the filename of the most recent repository
     * version
     * @return the filename of the most recent repository version
     */
    public String getConflictNew()
    {
        return conflictNew;
    }

    /**
     * Returns in case of conflict, the filename of the common base version
     * @return the filename of the common base version
     */
    public String getConflictOld()
    {
        return conflictOld;
    }

    /**
     * Returns in case of conflict, the filename of the former working copy
     * version
     * @return the filename of the former working copy version
     */
    public String getConflictWorking()
    {
        return conflictWorking;
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
    public int getNodeKind()
    {
        return nodeKind;
    }

    /**
     * Returns if copied the copy source url or null
     * @return the source url
     */
    public String getUrlCopiedFrom()
    {
        return urlCopiedFrom;
    }

    /**
     * Returns if copied the source revision as a Revision object
     * @return the source revision
     */
    public Revision.Number getRevisionCopiedFrom()
    {
        return Revision.createNumber(revisionCopiedFrom);
    }

    /**
     * Returns if copied the source revision as s long integer
     * @return the source revision
     */
    public long getRevisionCopiedFromNumber()
    {
        return revisionCopiedFrom;
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
        int status = getTextStatus();
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
     * Returns the lock token
     * @return the lock token
     * @since 1.2
     */
    public String getLockToken()
    {
        return lockToken;
    }

    /**
     * Returns the lock  owner
     * @return the lock owner
     * @since 1.2
     */
    public String getLockOwner()
    {
        return lockOwner;
    }

    /**
     * Returns the lock comment
     * @return the lock comment
     * @since 1.2
     */
    public String getLockComment()
    {
        return lockComment;
    }

    /**
     * Returns the lock creation date
     * @return the lock creation date
     * @since 1.2
     */
    public Date getLockCreationDate()
    {
        return microsecondsToDate(lockCreationDate);
    }

    /**
     * Returns the lock creation date measured in the number of
     * microseconds since 00:00:00 January 1, 1970 UTC.
     * @return the lock creation date
     * @since 1.5
     */
    public long getLockCreationDateMicros()
    {
        return lockCreationDate;
    }

    /**
     * Returns the lock as in the repository
     * @return the lock as in the repository
     * @since 1.2
     */
    public Lock getReposLock()
    {
        return reposLock;
    }

    /**
     * @return The last committed revision, or {@link
     * Revision#SVN_INVALID_REVNUM} if up to date.
     * @since 1.3
     */
    public Revision.Number getReposLastCmtRevision()
    {
        return Revision.createNumber(reposLastCmtRevision);
    }

    /**
     * @return The last committed revision as a long integer, or
     * <code>-1</code> if up to date.
     * @since 1.3
     */
    public long getReposLastCmtRevisionNumber()
    {
        return reposLastCmtRevision;
    }

    /**
     * @return The last committed date, or <code>null</code> if up to
     * date.
     * @since 1.3
     */
    public Date getReposLastCmtDate()
    {
        return microsecondsToDate(reposLastCmtDate);
    }

    /**
     * Return the last committed date measured in the number of
     * microseconds since 00:00:00 January 1, 1970 UTC.
     * @return the last committed date
     * @since 1.5
     */
    public long getReposLastCmtDateMicros()
    {
        return reposLastCmtDate;
    }

    /**
     * @return The node kind (e.g. file, directory, etc.), or
     * <code>null</code> if up to date.
     * @since 1.3
     */
    public int getReposKind()
    {
        return reposKind;
    }

    /**
     * @return The author of the last commit, or <code>null</code> if
     * up to date.
     * @since 1.3
     */
    public String getReposLastCmtAuthor()
    {
        return reposLastCmtAuthor;
    }

    /**
     * @return the changelist name
     * @since 1.5
     */
    public String getChangelist()
    {
        return changelist;
    }

    /**
     * @return the tree conflicted state
     * @since 1.6
     */
    public boolean hasTreeConflict()
    {
        return treeConflicted;
    }

    /**
     * @return the conflict descriptor for the tree conflict
     * @since 1.6
     */
    public ConflictDescriptor getConflictDescriptor()
    {
        return conflictDescriptor;
    }

    /**
     * class for kind status of the item or its properties
     * the constants are defined in the interface StatusKind for building
     * reasons
     */
    public static final class Kind implements StatusKind
    {
        /**
         * Returns the textual representation of the status
         * @param kind of status
         * @return english status
         */
        public static final String getDescription(int kind)
        {
            switch (kind)
            {
            case StatusKind.none:
                return "non-svn";
            case StatusKind.normal:
                return "normal";
            case StatusKind.added:
                return "added";
            case StatusKind.missing:
                return "missing";
            case StatusKind.deleted:
                return "deleted";
            case StatusKind.replaced:
                return "replaced";
            case StatusKind.modified:
                return "modified";
            case StatusKind.merged:
                return "merged";
            case StatusKind.conflicted:
                return "conflicted";
            case StatusKind.ignored:
                return "ignored";
            case StatusKind.incomplete:
                return "incomplete";
            case StatusKind.external:
                return "external";
            case StatusKind.unversioned:
            default:
                return "unversioned";
            }
        }
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

    private static int fromAStatusKind(
                            org.apache.subversion.javahl.types.Status.Kind aKind)
    {
        if (aKind == null)
            return StatusKind.none;

        switch (aKind)
        {
            default:
            case none:
                return StatusKind.none;
            case normal:
                return StatusKind.normal;
            case modified:
                return StatusKind.modified;
            case added:
                return StatusKind.added;
            case deleted:
                return StatusKind.deleted;
            case unversioned:
                return StatusKind.unversioned;
            case missing:
                return StatusKind.missing;
            case replaced:
                return StatusKind.replaced;
            case merged:
                return StatusKind.merged;
            case conflicted:
                return StatusKind.conflicted;
            case obstructed:
                return StatusKind.obstructed;
            case ignored:
                return StatusKind.ignored;
            case incomplete:
                return StatusKind.incomplete;
            case external:
                return StatusKind.external;
        }
    }
}

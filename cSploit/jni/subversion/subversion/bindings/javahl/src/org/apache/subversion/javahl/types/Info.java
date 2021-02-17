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
import java.util.Set;

import org.apache.subversion.javahl.ConflictDescriptor;
import org.apache.subversion.javahl.ISVNClient;

/**
 * this class is returned by {@link ISVNClient#info2} and contains information
 * about items in the repository or working copy
 */
public class Info implements java.io.Serializable
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
     * the path of the item
     */
    private String path;

    /**
     * the working copy root
     */
    private String wcroot;

    /**
     * the url of the item
     */
    private String url;

    /**
     * the revision of the item
     */
    private long rev;

    /**
     * the item kinds
     */
    private NodeKind kind;

    /**
     * the root URL of the repository
     */
    private String reposRootUrl;

    /**
     * the UUID of the repository
     */
    private String reposUUID;

    /**
     * the revision of the last change
     */
    private long lastChangedRev;

    /**
     * the date of the last change in ns
     */
    private long lastChangedDate;

    /**
     * the author of the last change
     */
    private String lastChangedAuthor;

    /**
     * the information about any lock (may be null)
     */
    private Lock lock;

    /**
     * the flag if the remaining fields are set
     */
    private boolean hasWcInfo;

    /**
     * the scheduled operation at next commit
     */
    private ScheduleKind schedule;

    /**
     * if the item was copied, the source url
     */
    private String copyFromUrl;

    /**
     * if the item was copied, the source rev
     */
    private long copyFromRev;

    /**
     * the last time the item was changed in ns
     */
    private long textTime;

    /**
     * the checksum of the item
     */
    private Checksum checksum;

    /**
     * The name of the changelist.
     */
    private String changelistName;

    /**
     * The size of the file after being translated into its local
     * representation, or <code>-1</code> if unknown.  Not applicable
     * for directories.
     */
    private long workingSize;

    /**
     * The size of the file in the repository (untranslated,
     * e.g. without adjustment of line endings and keyword
     * expansion). Only applicable for file -- not directory -- URLs.
     * For working copy paths, size will be <code>-1</code>.
     */
    private long reposSize;

    /**
     * The depth of the item.
     */
    private Depth depth;

    /**
     * Info on any tree conflict of which this node is a victim.
     */
    private Set<ConflictDescriptor> conflict;

    /**
     * constructor to build the object by native code. See fields for
     * parameters
     * @param path
     * @param wcroot
     * @param url
     * @param rev
     * @param kind
     * @param reposRootUrl
     * @param reposUUID
     * @param lastChangedRev
     * @param lastChangedDate
     * @param lastChangedAuthor
     * @param lock
     * @param hasWcInfo
     * @param schedule
     * @param copyFromUrl
     * @param copyFromRev
     * @param textTime
     * @param checksum
     * @param depth
     * @param treeConflict
     */
    public Info(String path, String wcroot, String url, long rev, NodeKind kind,
          String reposRootUrl, String reposUUID, long lastChangedRev,
          long lastChangedDate, String lastChangedAuthor, Lock lock,
          boolean hasWcInfo, ScheduleKind schedule, String copyFromUrl,
          long copyFromRev, long textTime, Checksum checksum,
          String changelistName, long workingSize, long reposSize, Depth depth,
          Set<ConflictDescriptor> conflict)
    {
        this.path = path;
        this.wcroot = wcroot;
        this.url = url;
        this.rev = rev;
        this.kind = kind;
        this.reposRootUrl = reposRootUrl;
        this.reposUUID = reposUUID;
        this.lastChangedRev = lastChangedRev;
        this.lastChangedDate = lastChangedDate;
        this.lastChangedAuthor = lastChangedAuthor;
        this.lock = lock;
        this.hasWcInfo = hasWcInfo;
        this.schedule = schedule;
        this.copyFromUrl = copyFromUrl;
        this.copyFromRev = copyFromRev;
        this.textTime = textTime;
        this.checksum = checksum;
        this.changelistName = changelistName;
        this.workingSize = workingSize;
        this.reposSize = reposSize;
        this.depth = depth;
        this.conflict = conflict;
    }

    /**
     * return the path of the item
     */
    public String getPath()
    {
        return path;
    }

    /**
     * return the workgin copy root
     */
    public String getWcroot()
    {
        return wcroot;
    }

    /**
     * return the url of the item
     */
    public String getUrl()
    {
        return url;
    }

    /**
     * return the revision of the item
     */
    public long getRev()
    {
        return rev;
    }

    /**
     * return the item kinds (see NodeKind)
     */
    public NodeKind getKind()
    {
        return kind;
    }

    /**
     * return the root URL of the repository
     */
    public String getReposRootUrl()
    {
        return reposRootUrl;
    }

    /**
     * return the UUID of the repository
     */
    public String getReposUUID()
    {
        return reposUUID;
    }

    /**
     * return the revision of the last change
     */
    public long getLastChangedRev()
    {
        return lastChangedRev;
    }

    /**
     * return the date of the last change
     */
    public Date getLastChangedDate()
    {
        if (lastChangedDate == 0)
            return null;
        else
            return new Date(lastChangedDate/1000);
    }

    /**
     * return the author of the last change
     */
    public String getLastChangedAuthor()
    {
        return lastChangedAuthor;
    }

    /**
     * return the information about any lock (may be null)
     */
    public Lock getLock()
    {
        return lock;
    }

    /**
     * return the flag if the working copy fields are set
     */
    public boolean isHasWcInfo()
    {
        return hasWcInfo;
    }

    /**
     * return the scheduled operation at next commit (see ScheduleKind)
     */
    public ScheduleKind getSchedule()
    {
        return schedule;
    }

    /**
     * return if the item was copied, the source url
     */
    public String getCopyFromUrl()
    {
        return copyFromUrl;
    }

    /**
     * return if the item was copied, the source rev
     */
    public long getCopyFromRev()
    {
        return copyFromRev;
    }

    /**
     * return the last time the item was changed
     */
    public Date getTextTime()
    {
        if (textTime == 0)
            return null;
        else
            return new Date(textTime/1000);
    }

    /**
     * return the checksum of the item
     */
    public Checksum getChecksum()
    {
        return checksum;
    }

    /**
     * @return The name of the changelist.
     */
    public String getChangelistName()
    {
        return changelistName;
    }

    /**
     * @return The size of the file after being translated into its
     * local representation, or <code>-1</code> if unknown.  Not
     * applicable for directories.
     */
    public long getWorkingSize()
    {
        return workingSize;
    }

    /**
     * @return The size of the file in the repository (untranslated,
     * e.g. without adjustment of line endings and keyword
     * expansion). Only applicable for file -- not directory -- URLs.
     * For working copy paths, size will be <code>-1</code>.
     */
    public long getReposSize()
    {
        return reposSize;
    }

    /**
     * @return The depth of the directory or <code>null</code> if the
     * item is a file.
     */
    public Depth getDepth()
    {
        return depth;
    }

    /**
     * @return the tree conflict of which this node is a victim, or null if none
     */
    public Set<ConflictDescriptor> getConflicts()
    {
        return conflict;
    }

    /**
     * @return A string representation of this info.
     */
    public String toString()
    {
        return getUrl();
    }

    public enum ScheduleKind
    {
        /** exists, but uninteresting */
        normal,

        /** Slated for addition */
        add,

        /** Slated for deletion */
        delete,

        /** Slated for replacement (delete + add) */
        replace;
    }
}

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
import java.io.File;

/**
 * Give information about one subversion item (file or directory) in the
 * working copy
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

    /** the name of the item */
    private String name;

    /** the url of the item */
    private String url;

    /** the uuid of the repository */
    private String uuid;

    /** the repository url */
    private String repository;

    /** the schedule on the next commit (see NodeKind) */
    private int schedule;

    /** the kind of node (file or directory or unknown */
    private int nodeKind;

    /** the author of the last commit before base */
    private String author;

    /** the last revision this item was updated */
    private long revision;

    /** the last revision the item before base */
    private long lastChangedRevision;

    /** the date of the last commit */
    private Date lastChangedDate;

    /** the last up-to-date time for the text context */
    private Date lastDateTextUpdate;

    /** the last up-to-date time for the properties */
    private Date lastDatePropsUpdate;

    /** the item was copied */
    private boolean copied;

    /** the item was deleted */
    private boolean deleted;

    /** the item is absent */
    private boolean absent;

    /** the item is incomplete */
    private boolean incomplete;

    /** the copy source revision */
    private long copyRev;

    /** the copy source url */
    private String copyUrl;

    /**
     * Constructor to be called only by the native code
     * @param name                  name of the item
     * @param url                   url of the item
     * @param uuid                  uuid of the repository
     * @param repository            url of the repository
     * @param author                author of the last change
     * @param revision              revision of the last update
     * @param lastChangedRevision   revision of the last change
     * @param lastChangedDate       the date of the last change
     * @param lastDateTextUpdate    the date of the last text change
     * @param lastDatePropsUpdate   the date of the last property change
     * @param copied                is the item copied
     * @param deleted               is the item deleted
     * @param absent                is the item absent
     * @param incomplete            is the item incomplete
     * @param copyRev               copy source revision
     * @param copyUrl               copy source url
     */
    Info(String name, String url, String uuid, String repository, int schedule,
         int nodeKind, String author, long revision, long lastChangedRevision,
         Date lastChangedDate, Date lastDateTextUpdate,
         Date lastDatePropsUpdate, boolean copied,
         boolean deleted, boolean absent, boolean incomplete, long copyRev,
         String copyUrl)
    {
        this.name = name;
        this.url = url;
        this.uuid = uuid;
        this.repository = repository;
        this.schedule = schedule;
        this.nodeKind = nodeKind;
        this.author = author;
        this.revision = revision;
        this.lastChangedRevision = lastChangedRevision;
        this.lastChangedDate = lastChangedDate;
        this.lastDateTextUpdate = lastDateTextUpdate;
        this.lastDatePropsUpdate = lastDatePropsUpdate;
        this.copied = copied;
        this.deleted = deleted;
        this.absent = absent;
        this.incomplete = incomplete;
        this.copyRev = copyRev;
        this.copyUrl = copyUrl;
    }

    /**
     * A backward-compat constructor
     */
    public Info(org.apache.subversion.javahl.types.Info aInfo)
    {
        this((new File(aInfo.getPath())).getName(), aInfo.getUrl(),
             aInfo.getReposUUID(), aInfo.getReposRootUrl(),
             aInfo.getSchedule() == null ? 0 : aInfo.getSchedule().ordinal(),
             NodeKind.fromApache(aInfo.getKind()),
             aInfo.getLastChangedAuthor(), aInfo.getRev(),
             aInfo.getLastChangedRev(), aInfo.getLastChangedDate(),
             aInfo.getTextTime(), null, aInfo.getCopyFromUrl() != null,
             aInfo.getSchedule() == org.apache.subversion.javahl.types.Info.ScheduleKind.delete,
             checkAbsent(aInfo.getPath()), checkIncomplete(aInfo.getPath()),
             aInfo.getCopyFromRev(), aInfo.getCopyFromUrl());
    }

    private static boolean checkAbsent(String path)
    {
    	File f = new File(path);
    	return !f.exists();
    }

    /** See if the path is incomplete.  We currently have no way of getting
     *  this information from the existing info struct, so just return false.
     */
    private static boolean checkIncomplete(String path)
    {
    	return false;
    }

    /**
     * Retrieves the name of the item
     * @return name of the item
     */
    public String getName()
    {
        return name;
    }

    /**
     * Retrieves the url of the item
     * @return url of the item
     */
    public String getUrl()
    {
        return url;
    }

    /**
     * Retrieves the uuid of the repository
     * @return  uuid of the repository
     */
    public String getUuid()
    {
        return uuid;
    }

    /**
     * Retrieves the url of the repository
     * @return url of the repository
     */
    public String getRepository()
    {
        return repository;
    }

    /**
     * Retrieves the schedule of the next commit
     * @return schedule of the next commit
     */
    public int getSchedule()
    {
        return schedule;
    }

    /**
     * Retrieves the nodeKind
     * @return nodeKind
     */
    public int getNodeKind()
    {
        return nodeKind;
    }

    /**
     * Retrieves the author of the last commit
     * @return author of the last commit
     */
    public String getAuthor()
    {
        return author;
    }

    /**
     * Retrieves the last revision the item was updated to
     * @return last revision the item was updated to
     */
    public long getRevision()
    {
        return revision;
    }

    /**
     * Retrieves the revision of the last commit
     * @return the revision of the last commit
     */
    public long getLastChangedRevision()
    {
        return lastChangedRevision;
    }

    /**
     * Retrieves the date of the last commit
     * @return the date of the last commit
     */
    public Date getLastChangedDate()
    {
        return lastChangedDate;
    }

    /**
     * Retrieves the last date the text content was changed
     * @return last date the text content was changed
     */
    public Date getLastDateTextUpdate()
    {
        return lastDateTextUpdate;
    }

    /**
     * Retrieves the last date the properties were changed
     * @return last date the properties were changed
     */
    public Date getLastDatePropsUpdate()
    {
        return lastDatePropsUpdate;
    }

    /**
     * Retrieve if the item was copied
     * @return the item was copied
     */
    public boolean isCopied()
    {
        return copied;
    }

    /**
     * Retrieve if the item was deleted
     * @return the item was deleted
     */
    public boolean isDeleted()
    {
        return deleted;
    }

    /**
     * Retrieve if the item is absent
     * @return the item is absent
     */
    public boolean isAbsent()
    {
        return absent;
    }

    /**
     * Retrieve if the item is incomplete
     * @return the item is incomplete
     */
    public boolean isIncomplete()
    {
        return incomplete;
    }

    /**
     * Retrieves the copy source revision
     * @return copy source revision
     */
    public long getCopyRev()
    {
        return copyRev;
    }

    /**
     * Retrieves the copy source url
     * @return copy source url
     */
    public String getCopyUrl()
    {
        return copyUrl;
    }
}

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
 * A general subversion directory entry. Used for SVNClientInterface.list
 */
public class DirEntry implements java.io.Serializable
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
     * The various field values which can be passed to list()
     */
    public class Fields
    {
        /**
         * An indication that you are interested in the nodeKind field
         */
        public static final int nodeKind = 0x00001;

        /**
         * An indication that you are interested in the size field
         */
        public static final int size = 0x00002;

        /**
         * An indication that you are interested in the hasProps field
         */
        public static final int hasProps = 0x00004;

        /**
         * An indication that you are interested in the lastChangedRevision
         * field
         */
        public static final int lastChangeRevision = 0x00008;

        /**
         * An indication that you are interested in the lastChanged field
         */
        public static final int lastChanged = 0x00010;

        /**
         * An indication that you are interested in the lastAuthor field
         */
        public static final int lastAuthor = 0x00020;

        /**
         * A combination of all the DirEntry fields
         */
        public static final int all = ~0;
    }

    /**
     * the date of the last change in nanoseconds since 01/01/1970
     */
    private long lastChanged;

    /**
     * the revision number of the last change
     */
    private long lastChangedRevision;

    /**
     * flag if the item has properties managed by subversion
     */
    private boolean hasProps;

    /**
     * the name of the author of the last change
     */
    private String lastAuthor;

    /**
     * the kind of the node (directory or file)
     */
    private int nodeKind;

    /**
     * the size of the file
     */
    private long size;

    /**
     * the pathname of the entry
     */
    private String path;

    /**
     * the absolute path of the entry
     */
    private String absPath;

    /**
     * this constructor is only called from the JNI code
     * @param path                  the pathname of the entry
     * @param absPath               the absolute path of the entry
     * @param nodeKind              the kind of entry (file or directory)
     * @param size                  the size of the file
     * @param hasProps              if the entry has properties managed by
     *                              subversion
     * @param lastChangedRevision   the revision number of the last change
     * @param lastChanged           the date of the last change
     * @param lastAuthor            the author of the last change
     */
    DirEntry(String path, String absPath, int nodeKind, long size,
             boolean hasProps, long lastChangedRevision, long lastChanged,
             String lastAuthor)
    {
        this.path = path;
        this.absPath = absPath;
        this.nodeKind = nodeKind;
        this.size = size;
        this.hasProps = hasProps;
        this.lastChangedRevision = lastChangedRevision;
        this.lastChanged = lastChanged;
        this.lastAuthor = lastAuthor;
    }

    /**
     * A backward-compat constructor
     */
    DirEntry(org.apache.subversion.javahl.types.DirEntry aEntry)
    {
        this(aEntry.getPath(), aEntry.getAbsPath(),
             NodeKind.fromApache(aEntry.getNodeKind()),
             aEntry.getSize(), aEntry.getHasProps(),
             aEntry.getLastChangedRevisionNumber(),
             aEntry.getLastChanged().getTime() * 1000,
             aEntry.getLastAuthor());
    }

    /**
     * Returns the path of the entry.
     * @return the path of the entry.
     */
    public String getPath()
    {
        return path;
    }

    /**
     * Returns the absolute path of the entry.
     * @return the absolute path of the entry.
     */
    public String getAbsPath()
    {
        return absPath;
    }

    /**
     * Returns the last time the file was changed.
     * @return the last time the file was changed.
     */
    public Date getLastChanged()
    {
        return new Date(lastChanged/1000);
    }

    /**
     * Returns the revision of the last change.
     * @return revision of the last change as a Revision object.
     */
    public Revision.Number getLastChangedRevision()
    {
        return Revision.createNumber(lastChangedRevision);
    }

    /**
     * Returns the revision number of the last change.
     * @return revision number of the last change.
     */
    public long getLastChangedRevisionNumber()
    {
        return lastChangedRevision;
    }

    /**
     * Returns if the entry has properties managed by Subversion.
     * @return if the entry has properties managed by subversion.
     */
    public boolean getHasProps()
    {
        return hasProps;
    }

    /**
     * Returns the author of the last change.
     * @return the author of the last change.
     */
    public String getLastAuthor()
    {
        return lastAuthor;
    }

    /**
     * Return the kind of entry (file or directory)
     * @return the kind of the entry (file or directory) see NodeKind class
     */
    public int getNodeKind()
    {
        return nodeKind;
    }

    /**
     * Return the length of file test or 0 for directories
     * @return length of file text, or 0 for directories
     */
    public long getSize()
    {
        return size;
    }

    /**
     * Set the path.  This should only be used by compatibility wrapper.
     */
    public void setPath(String path)
    {
        this.path = path;
    }

    /**
     * @return The path at its last changed revision.
     */
    public String toString()
    {
        return getPath() + '@' + getLastChangedRevision();
    }
}

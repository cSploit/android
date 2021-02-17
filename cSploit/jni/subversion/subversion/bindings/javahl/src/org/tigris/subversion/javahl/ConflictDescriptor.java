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

/**
 * The description of a merge conflict, encountered during
 * merge/update/switch operations.
 *
 * @since 1.5
 */
public class ConflictDescriptor
{
    private String path;

    /**
     * @see .Kind
     */
    private int conflictKind;

    /**
     * @see NodeKind
     */
    private int nodeKind;

    private String propertyName;

    private boolean isBinary;
    private String mimeType;

    // svn_wc_conflict_description_t also provides us with an
    // svn_wc_adm_access_t *.  However, that is only useful to
    // JNI-based APIs written against svn_wc.h.  So, we don't (yet)
    // expose that to JavaHL.  We could expose it is a long
    // representing the memory address of the struct, which could be
    // passed off to other JNI APIs.

    /**
     * @see #Action
     */
    private int action;

    /**
     * @see #Reason
     */
    private int reason;

    // File paths, present only when the conflict involves the merging
    // of two files descended from a common ancestor, here are the
    // paths of up to four fulltext files that can be used to
    // interactively resolve the conflict.  NOTE: The content of these
    // files will be in repository-normal form (LF line endings and
    // contracted keywords).
    private String basePath;
    private String theirPath;
    private String myPath;
    private String mergedPath;

    /**
     * @see Operation
     */
    private int operation;

    /**
     * @see ConflictVersion
     */
    private ConflictVersion srcLeftVersion;

    /**
     * @see ConflictVersion
     */
    private ConflictVersion srcRightVersion;

    /** This constructor should only be called from JNI code. */
    ConflictDescriptor(String path, int conflictKind, int nodeKind,
                       String propertyName, boolean isBinary, String mimeType,
                       int action, int reason, int operation,
                       String basePath, String theirPath,
                       String myPath, String mergedPath,
                       ConflictVersion srcLeft, ConflictVersion srcRight)
    {
        this.path = path;
        this.conflictKind = conflictKind;
        this.nodeKind = nodeKind;
        this.propertyName = propertyName;
        this.isBinary = isBinary;
        this.mimeType = mimeType;
        this.action = action;
        this.reason = reason;
        this.basePath = basePath;
        this.theirPath = theirPath;
        this.myPath = myPath;
        this.mergedPath = mergedPath;
        this.operation = operation;
        this.srcLeftVersion = srcLeft;
        this.srcRightVersion = srcRight;
    }

    public ConflictDescriptor(org.apache.subversion.javahl.ConflictDescriptor
                                                                aDesc)
    {
        this(aDesc.getPath(), aDesc.getKind().ordinal(),
             NodeKind.fromApache(aDesc.getNodeKind()),
             aDesc.getPropertyName(), aDesc.isBinary(), aDesc.getMIMEType(),
             aDesc.getAction().ordinal(), aDesc.getReason().ordinal(),
             aDesc.getOperation().ordinal(), aDesc.getBasePath(),
             aDesc.getTheirPath(), aDesc.getMyPath(), aDesc.getMergedPath(),
             new ConflictVersion(aDesc.getSrcLeftVersion()),
             new ConflictVersion(aDesc.getSrcRightVersion()));
    }

    public String getPath()
    {
        return path;
    }

    /**
     * @see .Kind
     */
    public int getKind()
    {
        return conflictKind;
    }

    /**
     * @see NodeKind
     */
    public int getNodeKind()
    {
        return nodeKind;
    }

    public String getPropertyName()
    {
        return propertyName;
    }

    public boolean isBinary()
    {
        return isBinary;
    }

    public String getMIMEType()
    {
        return mimeType;
    }

    /**
     * @see .Action
     */
    public int getAction()
    {
        return action;
    }

    /**
     * @see .Reason
     */
    public int getReason()
    {
        return reason;
    }

    public String getBasePath()
    {
        return basePath;
    }

    public String getTheirPath()
    {
        return theirPath;
    }

    public String getMyPath()
    {
        return myPath;
    }

    public String getMergedPath()
    {
        return mergedPath;
    }

    public int getOperation()
    {
        return operation;
    }

    public ConflictVersion getSrcLeftVersion()
    {
        return srcLeftVersion;
    }

    public ConflictVersion getSrcRightVersion()
    {
        return srcRightVersion;
    }

    /**
     * Poor man's enum for <code>svn_wc_conflict_kind_t</code>.
     */
    public final class Kind
    {
        /**
         * Attempting to change text or props.
         */
        public static final int text = 0;

        /**
         * Attempting to add object.
         */
        public static final int property = 1;
    }

    /**
     * Poor man's enum for <code>svn_wc_conflict_action_t</code>.
     */
    public final class Action
    {
        /**
         * Attempting to change text or props.
         */
        public static final int edit = 0;

        /**
         * Attempting to add object.
         */
        public static final int add = 1;

        /**
         * Attempting to delete object.
         */
        public static final int delete = 2;
    }

    /**
     * Poor man's enum for <code>svn_wc_conflict_reason_t</code>.
     */
    public final class Reason
    {
        /**
         * Local edits are already present.
         */
        public static final int edited = 0;

        /**
         * Another object is in the way.
         */
        public static final int obstructed = 1;

        /**
         * Object is already schedule-delete.
         */
        public static final int deleted = 2;

        /**
         * Object is unknown or missing.
         */
        public static final int missing = 3;

        /**
         * Object is unversioned.
         */
        public static final int unversioned = 4;

        /**
         * Object is already added or schedule-add.
         * @since New in 1.6.
         */
        public static final int added = 5;
    }
}

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

import java.util.EventObject;

/**
 * The event passed to the {@link
 * DiffSummaryReceiver#onSummary(DiffSummary)} API in response to path
 * differences reported by {@link SVNClientInterface#diffSummarize}.
 *
 * @since 1.5
 */
public class DiffSummary extends EventObject
{
    // Update the serialVersionUID when there is a incompatible change
    // made to this class.  See any of the following, depending upon
    // the Java release.
    // http://java.sun.com/j2se/1.3/docs/guide/serialization/spec/version.doc7.html
    // http://java.sun.com/j2se/1.4/pdf/serial-spec.pdf
    // http://java.sun.com/j2se/1.5.0/docs/guide/serialization/spec/version.html#6678
    // http://java.sun.com/javase/6/docs/platform/serialization/spec/version.html#6678
    private static final long serialVersionUID = 1L;

    private DiffKind diffKind;
    private boolean propsChanged;
    private int nodeKind;

    /**
     * This constructor is to be used by the native code.
     *
     * @param path The path we have a diff for.
     * @param diffKind The kind of diff this describes.
     * @param propChanged Whether any properties have changed.
     * @param nodeKind The type of node which changed (corresponds to
     * the {@link NodeKind} enumeration).
     */
    DiffSummary(String path, int diffKind, boolean propsChanged,
                int nodeKind)
    {
        super(path);
        this.diffKind = DiffKind.getInstance(diffKind);
        this.propsChanged = propsChanged;
        this.nodeKind = nodeKind;
    }

    /**
     * This constructor is for backward compat.
     */
    DiffSummary(org.apache.subversion.javahl.DiffSummary aSummary)
    {
        this(aSummary.getPath(), aSummary.getDiffKind().ordinal(),
             aSummary.propsChanged(),
             NodeKind.fromApache(aSummary.getNodeKind()));
    }

    /**
     * @return The path we have a diff for.
     */
    public String getPath()
    {
        return (String) super.source;
    }

    /**
     * @return The kind of summary this describes.
     */
    public DiffKind getDiffKind()
    {
        return this.diffKind;
    }

    /**
     * @return Whether any properties have changed.
     */
    public boolean propsChanged()
    {
        return this.propsChanged;
    }

    /**
     * @return The type of node which changed (corresponds to the
     * {@link NodeKind} enumeration).
     */
    public int getNodeKind()
    {
        return this.nodeKind;
    }

    /**
     * @return The path.
     */
    public String toString()
    {
        return getPath();
    }

    /**
     * The type of difference being summarized.
     */
    public static class DiffKind
    {
        // Corresponds to the svn_client_diff_summarize_kind_t enum.
        public static DiffKind NORMAL = new DiffKind(0);
        public static DiffKind ADDED = new DiffKind(1);
        public static DiffKind MODIFIED = new DiffKind(2);
        public static DiffKind DELETED = new DiffKind(3);

        private int kind;

        private DiffKind(int kind)
        {
            this.kind = kind;
        }

        /**
         * @return The appropriate instance.
         * @throws IllegalArgumentException If the diff kind is not
         * recognized.
         */
        public static DiffKind getInstance(int diffKind)
            throws IllegalArgumentException
        {
            switch (diffKind)
            {
            case 0:
                return NORMAL;
            case 1:
                return ADDED;
            case 2:
                return MODIFIED;
            case 3:
                return DELETED;
            default:
                throw new IllegalArgumentException("Diff kind " + diffKind +
                                                   " not recognized");
            }
        }

        public static DiffKind fromApache(
            org.apache.subversion.javahl.DiffSummary.DiffKind aKind)
        {
            /* This is cheating... */
            return getInstance(aKind.hashCode());
        }

        /**
         * @param diffKind A DiffKind for comparison.
         * @return Whether both DiffKinds are of the same type.
         */
        public boolean equals(Object diffKind)
        {
            return (((DiffKind) diffKind).kind == this.kind);
        }

        public int hashCode()
        {
            // Equivalent to new Integer(this.kind).hashCode().
            return this.kind;
        }

        /**
         * @return A textual representation of the type of diff.
         */
        public String toString()
        {
            switch (this.kind)
            {
            case 0:
                return "normal";
            case 1:
                return "added";
            case 2:
                return "modified";
            case 3:
                return "deleted";
            default:
                return "unknown";
            }
        }
    }
}

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

import java.util.EventObject;
import org.apache.subversion.javahl.callback.DiffSummaryCallback;
import org.apache.subversion.javahl.types.NodeKind;

/**
 * The event passed to the {@link DiffSummaryCallback#onSummary} API
 * in response to path differences reported by {@link ISVNClient#diffSummarize}.
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
    private NodeKind nodeKind;

    /**
     * This constructor is to be used by the native code.
     *
     * @param path The path we have a diff for.
     * @param diffKind The kind of diff this describes.
     * @param propsChanged Whether any properties have changed.
     * @param nodeKind The type of node which changed (corresponds to
     * the {@link NodeKind} enumeration).
     */
    public DiffSummary(String path, DiffKind diffKind, boolean propsChanged,
                NodeKind nodeKind)
    {
        super(path);
        this.diffKind = diffKind;
        this.propsChanged = propsChanged;
        this.nodeKind = nodeKind;
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
    public NodeKind getNodeKind()
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
    public enum DiffKind
    {
        normal      ("normal"),
        added       ("added"),
        modified    ("modified"),
        deleted     ("deleted");

        /**
         * The description of the action.
         */
        private String description;

        DiffKind(String description)
        {
            this.description = description;
        }

        public String toString()
        {
            return description;
        }
    }
}

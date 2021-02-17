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

public class ChangePath implements java.io.Serializable
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
     * Constructor to be called from the native code
     * @param path              path of the commit item
     * @param copySrcRevision   copy source revision (if any)
     * @param copySrcPath       copy source path (if any)
     * @param action            action performed
     * @param nodeKind          the kind of the changed path
     */
    public ChangePath(String path, long copySrcRevision, String copySrcPath,
               Action action, NodeKind nodeKind, Tristate textMods,
               Tristate propMods)
    {
        this.path = path;
        this.copySrcRevision = copySrcRevision;
        this.copySrcPath = copySrcPath;
        this.action = action;
        this.nodeKind = nodeKind;
        this.textMods = textMods;
        this.propMods = propMods;
    }

    /** Path of committed item */
    private String path;

    /** Source revision of copy (if any). */
    private long copySrcRevision;

    /** Source path of copy (if any). */
    private String copySrcPath;

    /** 'A'dd, 'D'elete, 'R'eplace, 'M'odify */
    private Action action;

    /** The kind of the changed path. */
    private NodeKind nodeKind;

    /** Whether the text was modified. */
    private Tristate textMods;

    /** Whether the properties were modified. */
    private Tristate propMods;

    /**
     * Retrieve the path to the committed item
     * @return  the path to the committed item
     */
    public String getPath()
    {
        return path;
    }

    /**
     * Retrieve the copy source revision (if any)
     * @return  the copy source revision (if any)
     */
    public long getCopySrcRevision()
    {
        return copySrcRevision;
    }

    /**
     * Retrieve the copy source path (if any)
     * @return  the copy source path (if any)
     */
    public String getCopySrcPath()
    {
        return copySrcPath;
    }

    /**
     * Retrieve action performed
     * @return  action performed
     */
    public Action getAction()
    {
        return action;
    }

    /**
     * Retrieve the node kind
     * @return  the node kind
     */
    public NodeKind getNodeKind()
    {
        return nodeKind;
    }

    /**
     * Retrieve whether the text was modified.
     * @return   the text mod state
     */
    public Tristate getTextMods()
    {
        return textMods;
    }

    /**
     * Retrieve whether the properties were modified.
     * @return   the prop mod state
     */
    public Tristate getPropMods()
    {
        return propMods;
    }

    /**
     * Actions which may have occurred to this path.
     */
    public enum Action
    {
        /** Path was added. */
        add,

        /** Path was deleted. */
        delete,

        /** Path was replaced. */
        replace,

        /** Path was modified. */
        modify;
    }
}

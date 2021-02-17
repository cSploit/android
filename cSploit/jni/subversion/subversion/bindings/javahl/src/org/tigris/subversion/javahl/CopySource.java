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
 * A description of a copy source.
 */
public class CopySource implements java.io.Serializable
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
     * The source path or URL.
     */
    private String path;

    /**
     * The source revision.
     */
    private Revision revision;

    /**
     * The peg revision.
     */
    private Revision pegRevision;

    /**
     * Create a new instance.
     *
     * @param path
     * @param revision The source revision.
     * @param pegRevision The peg revision.  Typically interpreted as
     * {@link org.tigris.subversion.javahl.Revision#HEAD} when
     * <code>null</code>.
     */
    public CopySource(String path, Revision revision, Revision pegRevision)
    {
        this.path = path;
        this.revision = revision;
        this.pegRevision = pegRevision;
    }

    public org.apache.subversion.javahl.types.CopySource toApache()
    {
        return new org.apache.subversion.javahl.types.CopySource(path,
                revision == null ? null : revision.toApache(),
                pegRevision == null ? null : pegRevision.toApache());
    }

    /**
     * @return The source path or URL.
     */
    public String getPath()
    {
        return this.path;
    }

    /**
     * @return The source revision.
     */
    public Revision getRevision()
    {
        return this.revision;
    }

    /**
     * @return The peg revision.
     */
    public Revision getPegRevision()
    {
        return this.pegRevision;
    }
}

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
 * The event passed to the {@link
 * ProgressListener#onProgress(ProgressEvent)} API to inform {@link
 * SVNClientInterface} of command progress (in terms of bytes).
 *
 * @since 1.5
 */
public class ProgressEvent implements java.io.Serializable
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
     * The number of bytes already transferred.
     */
    private long progress = -1;

    /**
     * The total number of bytes, or <code>-1</code> if not known.
     */
    private long total = -1;

    /**
     * This constructor is to be used by the native code.
     *
     * @param progress The number of bytes already transferred.
     * @param total The total number of bytes, or <code>-1</code> if
     * not known.
     */
    ProgressEvent(long progress, long total)
    {
        this.progress = progress;
        this.total = total;
    }

    /**
     * A backward-compat constructor.
     */
    public ProgressEvent(org.apache.subversion.javahl.ProgressEvent aEvent)
    {
        this(aEvent.getProgress(), aEvent.getTotal());
    }

    /**
     * @return The number of bytes already transferred.
     */
    public long getProgress()
    {
        return this.progress;
    }

    /**
     * @return The total number of bytes, or <code>-1</code> if not
     * known.
     */
    public long getTotal()
    {
        return this.total;
    }
}

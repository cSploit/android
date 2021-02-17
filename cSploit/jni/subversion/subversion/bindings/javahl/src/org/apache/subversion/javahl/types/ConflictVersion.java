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


/**
 * The description of a merge conflict, encountered during
 * merge/update/switch operations.
 */
public class ConflictVersion
{
    private String reposURL;
    private long pegRevision;
    private String pathInRepos;

    private NodeKind nodeKind;

    /** This constructor should only be called from JNI code. */
    public ConflictVersion(String reposURL, long pegRevision, String pathInRepos,
                    NodeKind nodeKind)
    {
        this.reposURL = reposURL;
        this.pegRevision = pegRevision;
        this.pathInRepos = pathInRepos;
        this.nodeKind = nodeKind;
    }

    public String getReposURL()
    {
        return reposURL;
    }

    public long getPegRevision()
    {
        return pegRevision;
    }

    public String getPathInRepos()
    {
        return pathInRepos;
    }

    /**
     * @see NodeKind
     */
    public NodeKind getNodeKind()
    {
        return nodeKind;
    }

    public String toString() {
        return "(" + nodeKind + ") " + reposURL +
        "/" + pathInRepos + "@" + pegRevision;
    }
}

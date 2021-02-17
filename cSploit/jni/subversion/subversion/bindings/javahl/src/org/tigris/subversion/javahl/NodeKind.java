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
 * Poor mans enum for svn_node_kind_t
 */
public final class NodeKind
{
    /* absent */
    public static final int none = 0;

    /* regular file */
    public static final int file = 1;

    /* directory */
    public static final int dir = 2;

    /* something's here, but we don't know what */
    public static final int unknown = 3;

    /**
     * mapping for the constants to text
     */
    private static final String[] statusNames =
    {
        "none",
        "file",
        "dir ",
        "unknown",
    };

    /**
     * Returns the textual representation for a NodeKind
     * @param kind  kind of node
     * @return english text
     */
    public static final String getNodeKindName(int kind)
    {
        return statusNames[kind];
    }

    public static int fromApache(org.apache.subversion.javahl.types.NodeKind aKind)
    {
        switch(aKind)
        {
        case none:
            return none;
        case file:
            return file;
        case dir:
            return dir;
        case unknown:
        default:
            return unknown;
        }
    }
}

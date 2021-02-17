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
 * Rich man's enum for svn_node_kind_t
 */
public enum NodeKind
{
    /* absent */
    none    ("none"),

    /* regular file */
    file    ("file"),

    /* directory */
    dir     ("dir"),

    /* something's here, but we don't know what */
    unknown ("unknown");

    /**
     * The description of the node kind.
     */
    private String description;

    NodeKind(String description)
    {
        this.description = description;
    }

    public String toString()
    {
        return description;
    }
}

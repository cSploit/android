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
 * This class describes a item which will be committed.
 */
public class CommitItem implements java.io.Serializable
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
     * the pathname of the item to be commit
     */
    String path;

    /**
     * the kind node (file or directory)
     */
    int nodeKind;

    /**
     * the kind of change to be committed (See CommitItemStateFlages)
     */
    int stateFlags;

    /**
     * the url of the item
     */
    String url;

    /**
     * the source of the copy
     */
    String copyUrl;

    /**
     * the revision
     */
    long revision;

    /**
     * This constructor will be only called from the jni code.
     * @param p     path to the commit item
     * @param nk    kind of node (see NodeKind)
     * @param sf    state flags (see StateFlags)
     * @param u     url of the item
     * @param cu    copy source url
     * @param r     revision number
     */
    CommitItem(String p, int nk, int sf, String u, String cu, long r)
    {
        path = p;
        nodeKind = nk;
        stateFlags = sf;
        url = u;
        copyUrl = cu;
        revision = r;
    }

    /**
     * A backward-compat constructor.
     */
    public CommitItem(org.apache.subversion.javahl.CommitItem aItem)
    {
        this(aItem.getPath(), NodeKind.fromApache(aItem.getNodeKind()),
             aItem.getStateFlags(), aItem.getUrl(), aItem.getCopyUrl(),
             aItem.getRevision());
    }

    /**
     *  retrieve the path of the commit item
     * @return the path
     */
    public String getPath()
    {
        return path;
    }

    /**
     * return the node kind of the commit item
     * @return the node kind. Look at the NodeKind class.
     */
    public int getNodeKind()
    {
        return nodeKind;
    }

    /**
     * return the kind of change for the commit item.
     * @return  the state flags. Look at the CommitItemStateFlags interface.
     */
    public int getStateFlags()
    {
        return stateFlags;
    }

    /**
     * the class for the commit item state flags. The values are ored together
     * the values are CommitItemStateFlags for building reasons.
     */
    public static class StateFlags implements CommitItemStateFlags
    {
    }

    /**
     * Returns the url of the item
     * @return url
     */
    public String getUrl()
    {
        return url;
    }

    /**
     * Returns the source url if the item is copied
     * @return source url
     */
    public String getCopyUrl()
    {
        return copyUrl;
    }

    /**
     * Returns the revision number
     * @return revision number
     */
    public long getRevision()
    {
        return revision;
    }
}

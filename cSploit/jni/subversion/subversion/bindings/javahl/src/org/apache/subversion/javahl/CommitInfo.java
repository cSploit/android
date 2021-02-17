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

import java.util.Date;

import org.apache.subversion.javahl.types.LogDate;

/**
 * This class describes a item which will be committed.
 */
public class CommitInfo implements java.io.Serializable
{
    // Update the serialVersionUID when there is a incompatible change
    // made to this class.  See any of the following, depending upon
    // the Java release.
    // http://java.sun.com/j2se/1.3/docs/guide/serialization/spec/version.doc7.html
    // http://java.sun.com/j2se/1.4/pdf/serial-spec.pdf
    // http://java.sun.com/j2se/1.5.0/docs/guide/serialization/spec/version.html#6678
    // http://java.sun.com/javase/6/docs/platform/serialization/spec/version.html#6678
    private static final long serialVersionUID = 1L;

    /** the revision committed */
    long revision;

    /** the date of the revision */
    Date date;

    /** the author of the revision */
    String author;

    /** post commit error (or NULL) */
    String postCommitError;

    /** repos root (or NULL) */
    String reposRoot;

    /** This constructor will be only called from the jni code.  */
    public CommitInfo(long rev, String d, String a, String pce, String rr)
            throws java.text.ParseException
    {
        revision = rev;
        date = (new LogDate(d)).getDate();
        author = a;
        postCommitError = pce;
        reposRoot = rr;
    }

    /**
     * retrieve the revision of the commit
     */
    public long getRevision()
    {
        return revision;
    }

    /**
     * return the date of the commit
     */
    public Date getDate()
    {
        return date;
    }

    /**
     * return the author of the commit
     */
    public String getAuthor()
    {
        return author;
    }

    /**
     * return any post commit error for the commit
     */
    public String getPostCommitError()
    {
        return postCommitError;
    }

    /**
     * return the repos root
     */
    public String getReposRoot()
    {
        return reposRoot;
    }
}

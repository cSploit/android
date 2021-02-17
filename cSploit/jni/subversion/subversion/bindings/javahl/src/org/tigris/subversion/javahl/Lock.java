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
 *
 * @endcopyright
 */

package org.tigris.subversion.javahl;

import java.util.Date;

/**
 * Class to describe a lock.  It is returned by the lock operation.
 * @since 1.2
 */
public class Lock implements java.io.Serializable
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
     * the owner of the lock
     */
    private String owner;

    /**
     * the path of the locked item
     */
    private String path;

    /**
     * the token provided during the lock operation
     */
    private String token;

    /**
     * the comment provided during the lock operation
     */
    private String comment;

    /**
     * the date when the lock was created
     */
    private long creationDate;

    /**
     * the date when the lock will expire
     */
    private long expirationDate;

    /**
     * this constructor should only called from JNI code
     * @param owner             the owner of the lock
     * @param path              the path of the locked item
     * @param token             the lock token
     * @param comment           the lock comment
     * @param creationDate      the date when the lock was created
     * @param expirationDate    the date when the lock will expire
     */
    Lock(String owner, String path, String token, String comment,
         long creationDate, long expirationDate)
    {
        this.owner = owner;
        this.path = path;
        this.token = token;
        this.comment = comment;
        this.creationDate = creationDate;
        this.expirationDate = expirationDate;
    }

    Lock(org.apache.subversion.javahl.types.Lock aLock)
    {
        this(aLock.getOwner(), aLock.getPath(), aLock.getToken(),
             aLock.getComment(),
             aLock.getCreationDate() == null ? 0
                : aLock.getCreationDate().getTime() * 1000,
             aLock.getExpirationDate() == null ? 0
                : aLock.getExpirationDate().getTime() * 1000);
    }

    /**
     * @return the owner of the lock
     */
    public String getOwner()
    {
        return owner;
    }

    /**
     * @return the path of the locked item
     */
    public String getPath()
    {
        return path;
    }

    /**
     * @return the token provided during the lock operation
     */
    public String getToken()
    {
        return token;
    }

    /**
     * @return the comment provided during the lock operation
     */
    public String getComment()
    {
        return comment;
    }

    /**
     * @return the date the lock was created
     */
    public Date getCreationDate()
    {
        if (creationDate == 0)
            return null;
        else
            return new Date(creationDate/1000);
    }

    /**
     * @return the date when the lock will expire
     */
    public Date getExpirationDate()
    {
        if (expirationDate == 0)
            return null;
        else
            return new Date(expirationDate/1000);
    }

}

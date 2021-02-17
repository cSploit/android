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
 * This class describes one property managed by Subversion.
 */
public class PropertyData
{
    /**
     * the name of the property
     */
    private String name;

    /**
     * the string value of the property
     */
    private String value;

    /**
     * the byte array value of the property
     */
    private byte[] data;

    /**
     * path of the subversion to change or delete this property
     */
    private String path;

    /**
     * reference to the creating SVNClient object to change or delete this
     * property
     */
    private SVNClientInterface client;

    /**
     * Standard subversion known properties
     */

    /**
     * mime type of the entry, used to flag binary files
     */
    public static final String MIME_TYPE = "svn:mime-type";

    /**
     * list of filenames with wildcards which should be ignored by add and
     * status
     */
    public static final String IGNORE = "svn:ignore";

    /**
     * how the end of line code should be treated during retrieval
     */
    public static final String EOL_STYLE = "svn:eol-style";

    /**
     * list of keywords to be expanded during retrieval
     */
    public static final String KEYWORDS = "svn:keywords";

    /**
     * flag if the file should be made excutable during retrieval
     */
    public static final String EXECUTABLE = "svn:executable";

    /**
     * value for svn:executable
     */
    public static final String EXECUTABLE_VALUE = "*";

    /**
     * list of directory managed outside of this working copy
     */
    public static final String EXTERNALS = "svn:externals";

    /**
     * the author of the revision
     */
    public static final String REV_AUTHOR = "svn:author";

    /**
     * the log message of the revision
     */
    public static final String REV_LOG = "svn:log";

    /**
     * the date of the revision
     */
    public static final String REV_DATE = "svn:date";

    /**
     * the original date of the revision
     */
    public static final String REV_ORIGINAL_DATE = "svn:original-date";

    /**
     * @since 1.2
     * flag property if a lock is needed to modify this node
     */
    public static final String NEEDS_LOCK = "svn:needs-lock";

    /**
     * this constructor is only used by the JNI code
     * @param cl    the client object, which created this object
     * @param p     the path of the item owning this property
     * @param n     the name of the property
     * @param v     the string value of the property
     * @param d     the byte array value of the property
     */
    PropertyData(SVNClientInterface cl, String p, String n, String v, byte[] d)
    {
        path = p;
        name = n;
        value = v;
        client = cl;
        data = d;
    }

    /**
     * this contructor is used when building a thin wrapper around other
     * property retrieval methods
     * @param p     the path of the item owning this property
     * @param n     the name of the property
     * @param v     the string value of the property
     */
    PropertyData(String p, String n, String v)
    {
        path = p;
        name = n;
        value = v;
        client = null;
        data = v.getBytes();
    }

    /**
     * Returns the name of the property
     * @return the name
     */
    public String getName()
    {
        return name;
    }

    /**
     * Returns the string value of the property.
     * There is no protocol if a property is a string or a binary value
     * @return the string value
     */
    public String getValue()
    {
        return value;
    }

    /**
     * Return the path of the item which owns this property
     * @return the path
     */
    public String getPath()
    {
        return path;
    }

    /**
     * Returns the byte array value of the property
     * There is no protocol if a property is a string or a binary value
     * @return the byte array value
     */
    public byte[] getData()
    {
        return data;
    }

    /**
     * modify the string value of a property
     * The byte array value is cleared
     * @param newValue          the new string value
     * @param recurse           if operation should recurse directories
     * @throws ClientException
     */
    public void setValue(String newValue, boolean recurse)
            throws ClientException
    {
        client.propertySet(path, name, newValue, recurse);
        value = newValue;
        data = null;
    }

    /**
     * modify the byte array value of a property
     * The string array value is cleared
     * @param newValue          the new byte array value
     * @param recurse           if operation should recurse directories
     * @throws ClientException
     */
    public void setValue(byte[] newValue, boolean recurse)
            throws ClientException
    {
        client.propertySet(path, name, newValue, recurse);
        data = newValue;
        value = null;
    }

    /**
     * remove this property from subversion
     * @param recurse           if operation should recurse directories
     * @throws ClientException
     */
    public void remove(boolean recurse) throws ClientException
    {
        client.propertyRemove(path, name, recurse);
    }
}

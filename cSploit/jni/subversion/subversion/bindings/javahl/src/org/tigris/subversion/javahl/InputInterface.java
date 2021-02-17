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

import java.io.IOException;

/**
 * Interface for data to be received from subversion
 * used for SVNAdmin.load and SVNAdmin.dump
 */
public interface InputInterface
{
    /**
     * read the number of data.length bytes from input.
     * @param data          array to store the read bytes.
     * @throws IOException  throw in case of problems.
     */
    public int read(byte [] data) throws IOException;

    /**
     * close the input
     * @throws IOException throw in case of problems.
     */
    public void close() throws IOException;
}

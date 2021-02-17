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

import java.io.*;

/**
 * This class connects a java.io.PipedOutputStream to a InputInterface.
 * The other side of the Pipe must written by another thread, or deadlocks
 * will occur.
 */
public class SVNOutputStream extends PipedOutputStream
{
    /**
     * My connection to receive data into subversion.
     */
    Inputer myInputer;

    /**
     * Creates a SVNOutputStream so that it is connected with an internal
     * PipedInputStream
     * @throws IOException
     */
    public SVNOutputStream() throws IOException
    {
        myInputer = new Inputer(this);
    }

    /**
     * Closes this piped output stream and releases any system resources
     * associated with this stream. This stream may no longer be used for
     * writing bytes.
     *
     * @throws  IOException  if an I/O error occurs.
     */
    public void close() throws IOException
    {
        myInputer.closed = true;
        super.close();
    }

    /**
     * Get the Interface to connect to SVNAdmin
     * @return the connection interface
     */
    public InputInterface getInputer()
    {
        return myInputer;
    }

    /**
     * this class implements the connection to SVNAdmin
     */
    public class Inputer implements InputInterface
    {
        /**
         * my side of the pipe
         */
        PipedInputStream myStream;

        /**
         * flag that the other side of the pipe has been closed
         */
        boolean closed;

        /**
         * build a new connection object
         * @param myMaster  the other side of the pipe
         * @throws IOException
         */
        Inputer(SVNOutputStream myMaster) throws IOException
        {
            myStream = new PipedInputStream(myMaster);
        }

        /**
         * read the number of data.length bytes from input.
         * @param data          array to store the read bytes.
         * @throws IOException  throw in case of problems.
         */
        public int read(byte[] data) throws IOException
        {
            if (closed)
                throw new IOException("stream has been closed");
            return myStream.read();
        }

        /**
         * close the input
         * @throws IOException throw in case of problems.
         */
        public void close() throws IOException
        {
            myStream.close();
        }
    }
}

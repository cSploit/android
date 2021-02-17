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
 * Partial interface for receiving callbacks for authentication.  New
 * applications should use PromptUserPassword3 instead.
 */
public interface PromptUserPassword2 extends PromptUserPassword
{
    /**
     * Reject the connection to the server.
     */
    public static final int Reject = 0;

    /**
     * Accept the connection to the server <i>once</i>.
     */
    public static final int AcceptTemporary = 1;

    /**
     * @deprecated Use the correctly spelled "AcceptTemporary"
     * constant instead.
     */
    public static final int AccecptTemporary = AcceptTemporary;

    /**
     * Accept the connection to the server <i>forever</i>.
     */
    public static final int AcceptPermanently = 2;

    /**
     * If there are problems with the certifcate of the SSL-server, this
     * callback will be used to deside if the connection will be used.
     * @param info              the probblems with the certificate.
     * @param allowPermanently  if AcceptPermantly is a legal answer
     * @return                  one of Reject/AcceptTemporary/AcceptPermanently
     */
    public int askTrustSSLServer(String info, boolean allowPermanently);
}

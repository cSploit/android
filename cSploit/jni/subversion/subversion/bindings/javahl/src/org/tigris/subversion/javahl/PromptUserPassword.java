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
 * The original interface for receiving callbacks for authentication.
 * Consider this code deprecated -- new applications should use
 * PromptUserPassword3 instead.
 */
public interface PromptUserPassword
{
    /**
     * Ask the user for username and password
     * The entered username/password is retrieved by the getUsername
     * getPasswort methods.
     *
     * @param realm     for which server realm this information is requested.
     * @param username  the default username
     * @return Whether the prompt for authentication credentials was
     * successful (e.g. in a GUI application whether the dialog box
     * was canceled).
     */
    public boolean prompt(String realm, String username);

    /**
     * ask the user a yes/no question
     * @param realm         for which server realm this information is
     *                      requested.
     * @param question      question to be asked
     * @param yesIsDefault  if yes should be the default
     * @return              the answer
     */
    public boolean askYesNo(String realm, String question,
                            boolean yesIsDefault);

    /**
     * ask the user a question where she answers with a text.
     * @param realm         for which server realm this information is
     *                      requested.
     * @param question      question to be asked
     * @param showAnswer    if the answer is shown or hidden
     * @return              the entered text or null if canceled
     */
    public String askQuestion(String realm, String question,
                              boolean showAnswer);

    /**
     * retrieve the username entered during the prompt call
     * @return the username
     */
    public String getUsername();

    /**
     * retrieve the password entered during the prompt call
     * @return the password
     */
    public String getPassword();
}

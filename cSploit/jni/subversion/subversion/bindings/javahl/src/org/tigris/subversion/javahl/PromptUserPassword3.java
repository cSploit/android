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
 * <p>The interface for requesting authentication credentials from the
 * user.  Should the javahl bindings need the matching information,
 * these methodes will be called.</p>
 *
 * <p>This callback can also be used to provide the equivalent of the
 * <code>--no-auth-cache</code> and <code>--non-interactive</code>
 * arguments accepted by the command-line client.</p>
 */
public interface PromptUserPassword3 extends PromptUserPassword2
{
    /**
     * Request a user name and password from the user, and (usually)
     * store the auth credential caching preference specified by
     * <code>maySave</code> (used by {@link #userAllowedSave()}).
     * Applications wanting to emulate the behavior of
     * <code>--non-interactive</code> will implement this method in a
     * manner which does not require user interaction (e.g. a no-op
     * which assumes pre-cached auth credentials).
     *
     * @param realm The realm from which the question originates.
     * @param username The name of the user in <code>realm</code>.
     * @param maySave Whether caching of credentials is allowed.
     * Usually affects the return value of the {@link
     * #userAllowedSave()} method.
     * @return Whether the prompt for authentication credentials was
     * successful (e.g. in a GUI application whether the dialog box
     * was canceled).
     */
    public boolean prompt(String realm, String username, boolean maySave);

    /**
     * Ask the user a question, and (usually) store the auth
     * credential caching preference specified by <code>maySave</code>
     * (used by {@link #userAllowedSave()}).  Applications wanting to
     * emulate the behavior of <code>--non-interactive</code> will
     * implement this method in a manner which does not require user
     * interaction (e.g. a no-op).
     *
     * @param realm The realm from which the question originates.
     * @param question The text of the question.
     * @param showAnswer Whether the answer may be displayed.
     * @param maySave Whether caching of credentials is allowed.
     * Usually affects the return value of the {@link
     * #userAllowedSave()} method.
     * @return              answer as entered or null if canceled
     */
    public String askQuestion(String realm, String question,
                              boolean showAnswer, boolean maySave);

    /**
     * @return Whether the caller allowed caching of credentials the
     * last time {@link #prompt(String, String, boolean)} was called.
     * Applications wanting to emulate the behavior of
     * <code>--no-auth-cache</code> will probably always return
     * <code>false</code>.
     */
    public boolean userAllowedSave();
}

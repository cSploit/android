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

package org.apache.subversion.javahl.callback;

import org.apache.subversion.javahl.ISVNClient;

/**
 * This interface is invoked before each patch in a
 * {@link ISVNClient#patch} call.
 */
public interface PatchCallback
{
    /**
     * the method will be called for every patch.
     * @param pathFromPatchfile        the path in the path file
     * @param patchPath                the path of the patch
     * @param rejectPath               the path of the reject file
     * @return            return TRUE to filter out the prospective patch
     */
    public boolean singlePatch(String pathFromPatchfile, String patchPath,
                               String rejectPath);
}

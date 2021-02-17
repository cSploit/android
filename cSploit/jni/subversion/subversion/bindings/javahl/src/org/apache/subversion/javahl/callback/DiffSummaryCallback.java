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

import org.apache.subversion.javahl.DiffSummary;
import org.apache.subversion.javahl.ISVNClient;

/**
 * Subversion diff summarization interface.  An implementation which
 * simply collects all summaries could look like:
 *
 * <blockquote><code><pre>
 * public class DiffSummaries extends ArrayList implements DiffSummaryCallback
 * {
 *     public void onSummary(DiffSummary descriptor)
 *     {
 *         add(descriptor);
 *     }
 * }
 * </pre></code></blockquote>
 */
public interface DiffSummaryCallback
{
    /**
     * Implement this interface to receive diff summaries from the
     * {@link ISVNClient#diffSummarize} API.
     *
     * @param descriptor A summary of the diff.
     */
    void onSummary(DiffSummary descriptor);
}

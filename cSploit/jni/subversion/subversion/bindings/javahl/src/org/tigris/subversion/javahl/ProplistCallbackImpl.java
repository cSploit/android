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

import java.util.HashMap;
import java.util.Map;

/**
 * Implementation of {@link ProplistCallback} interface.
 *
 * @since 1.5
 */
public class ProplistCallbackImpl implements ProplistCallback
{
    Map<String, Map<String, byte[]>> propMap =
                                new HashMap<String, Map<String, byte[]>>();

    public void singlePath(String path, Map<String, byte[]> props)
    {
        propMap.put(path, props);
    }

    public Map<String, String> getProperties(String path)
    {
        Map<String, String> props = new HashMap<String, String>();
        Map<String, byte[]> pdata = propMap.get(path);

        for (String key : pdata.keySet())
        {
            props.put(key, new String(pdata.get(key)));
        }

        return props;
    }
}

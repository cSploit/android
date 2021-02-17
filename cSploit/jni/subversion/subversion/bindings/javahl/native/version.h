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

#define JNI_VER_MAJOR    0
#define JNI_VER_MINOR    9
#define JNI_VER_MICRO    0

#define JNI_VER_NUM APR_STRINGIFY(JNI_VER_MAJOR) "." \
        APR_STRINGIFY(JNI_VER_MINOR) "." APR_STRINGIFY(JNI_VER_MICRO)

/** Version number with tag (contains no whitespace) */
#define JNI_VER_NUMBER     JNI_VER_NUM

/** Complete version string */
#define JNI_VERSION        JNI_VER_NUM

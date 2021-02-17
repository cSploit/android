/* rep-cache-db.sql -- schema for use in rep-caching
 *   This is intended for use with SQLite 3
 *
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
 */

-- STMT_CREATE_SCHEMA
PRAGMA AUTO_VACUUM = 1;

/* A table mapping representation hashes to locations in a rev file. */
CREATE TABLE rep_cache (
  hash TEXT NOT NULL PRIMARY KEY,
  revision INTEGER NOT NULL,
  offset INTEGER NOT NULL,
  size INTEGER NOT NULL,
  expanded_size INTEGER NOT NULL
  );

PRAGMA USER_VERSION = 1;


-- STMT_GET_REP
SELECT revision, offset, size, expanded_size
FROM rep_cache
WHERE hash = ?1


-- STMT_SET_REP
INSERT OR FAIL INTO rep_cache (hash, revision, offset, size, expanded_size)
VALUES (?1, ?2, ?3, ?4, ?5)

-- STMT_DEL_REPS_YOUNGER_THAN_REV
DELETE FROM rep_cache
WHERE revision > ?1

#
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
#
# A handle object for convenience in opening a svn repository

import sys

# We need a bsddb linked to the same version of Berkeley DB as Subversion is
try:
  import bsddb3 as bsddb
except ImportError:
  import bsddb

# Publish the result
sys.modules['svnfs_bsddb'] = bsddb

from svnfs_bsddb.db import *

class Ctx:
  def __init__(self, dbhome, readonly=None):
    self.env = self.uuids_db = self.revs_db = self.txns_db = self.changes_db \
        = self.copies_db = self.nodes_db = self.reps_db = self.strings_db = \
        None
    try:
      self.env = DBEnv()
      self.env.set_lk_detect(DB_LOCK_RANDOM)
      self.env.set_get_returns_none(1)
      self.env.open(dbhome, DB_CREATE | DB_INIT_MPOOL | DB_INIT_TXN \
          | DB_INIT_LOCK | DB_INIT_LOG)
      def open_db(dbname):
        db = DB(self.env)
        dbflags = 0
        if readonly:
          dbflags = DB_RDONLY
        db.open(dbname, flags=dbflags)
        return db
      self.uuids_db   = open_db('uuids')
      self.revs_db    = open_db('revisions')
      self.txns_db    = open_db('transactions')
      self.changes_db = open_db('changes')
      self.copies_db  = open_db('copies')
      self.nodes_db   = open_db('nodes')
      self.reps_db    = open_db('representations')
      self.strings_db = open_db('strings')
    except:
      self.close()
      raise

  def close(self):
    def close_if_not_None(i):
      if i is not None:
        i.close()
    close_if_not_None(self.uuids_db   )
    close_if_not_None(self.revs_db    )
    close_if_not_None(self.txns_db    )
    close_if_not_None(self.changes_db )
    close_if_not_None(self.copies_db  )
    close_if_not_None(self.nodes_db   )
    close_if_not_None(self.reps_db    )
    close_if_not_None(self.strings_db )
    close_if_not_None(self.env        )
    self.env = self.uuids_db = self.revs_db = self.txns_db = self.changes_db \
        = self.copies_db = self.nodes_db = self.reps_db = self.strings_db = \
        None

  # And now, some utility functions
  def get_whole_string(self, key):
    cur = self.strings_db.cursor()
    try:
      rec = cur.set(key)
      if rec is None:
        raise DBNotFoundError
      str = ""
      while rec:
        str = str + (rec[1] or "")
        rec = cur.next_dup()
    finally:
      cur.close()
    return str


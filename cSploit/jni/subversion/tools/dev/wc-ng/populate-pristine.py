#!/usr/bin/env python

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

"""
A script that takes a .svn/pristine/ hierarchy, with its existing
.svn/wc.db database, and populates the database's PRISTINE table
accordingly.  (Use 'svn cleanup' to remove unreferenced pristines.)

Usage:

  %s /path/to/wc [...]
"""

# TODO: resolve the NotImplemented() in __main__

# TODO: increment refcount upon collision
# TODO: add <given file>, not just argv[1]/.svn/pristine/??/*

import hashlib
import os
import re
import sqlite3
import sys

# ### This could require any other format that has the same PRISTINE schema
# ### and semantics.
FORMAT = 22
BUFFER_SIZE = 4 * 1024

class UnknownFormat(Exception):
  def __init__(self, formatno):
    self.formatno = formatno

def open_db(wc_path):
  wc_db = os.path.join(wc_path, '.svn', 'wc.db')
  conn = sqlite3.connect(wc_db)
  curs = conn.cursor()
  curs.execute('pragma user_version;')
  formatno = int(curs.fetchone()[0])
  if formatno > FORMAT:
    raise UnknownFormat(formatno)
  return conn

_sha1_re = re.compile(r'^[0-9a-f]{40}$')

def md5_of(path):
  fd = os.open(path, os.O_RDONLY)
  ctx = hashlib.md5()
  while True:
    s = os.read(fd, BUFFER_SIZE)
    if len(s):
      ctx.update(s)
    else:
      os.close(fd)
      return ctx.hexdigest()

INSERT_QUERY = """
  INSERT OR REPLACE
  INTO pristine(checksum,compression,size,refcount,md5_checksum)
  VALUES (?,?,?,?,?)
"""

def populate(wc_path):
  conn = open_db(wc_path)
  sys.stdout.write("Updating '%s': " % wc_path)
  for dirname, dirs, files in os.walk(os.path.join(wc_path, '.svn/pristine/')):
    # skip everything but .svn/pristine/xx/
    if os.path.basename(os.path.dirname(dirname)) == 'pristine':
      sys.stdout.write("'%s', " % os.path.basename(dirname))
      for f in filter(lambda x: _sha1_re.match(x), files):
        fullpath = os.path.join(dirname, f)
        conn.execute(INSERT_QUERY,
                     ('$sha1$'+f, None, os.stat(fullpath).st_size, 1,
                      '$md5 $'+md5_of(fullpath)))
      # periodic transaction commits, for efficiency
      conn.commit()
  else:
    sys.stdout.write(".\n")

if __name__ == '__main__':
  raise NotImplemented("""Subversion does not know yet to avoid fetching
  a file when a file with matching sha1 appears in the PRISTINE table.""")

  paths = sys.argv[1:]
  if not paths:
    paths = ['.']
  for wc_path in paths:
    try:
      populate(wc_path)
    except UnknownFormat, e:
      sys.stderr.write("Don't know how to handle '%s' (format %d)'\n"
                       % (wc_path, e.formatno))

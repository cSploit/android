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

import os
import sqlite3
import sys

MIN_SINGLE_DB_FORMAT = 19

def get_format(wc_path):
  entries = os.path.join(wc_path, '.svn', 'entries')
  wc_db = os.path.join(wc_path, '.svn', 'wc.db')

  formatno = 'not under version control'

  if os.path.exists(wc_db):
    conn = sqlite3.connect(wc_db)
    curs = conn.cursor()
    curs.execute('pragma user_version;')
    formatno = curs.fetchone()[0]
  elif os.path.exists(entries):
    formatno = int(open(entries).readline())
  elif os.path.exists(wc_path):
    parent_path = os.path.dirname(os.path.abspath(wc_path))
    if wc_path != parent_path:
      formatno = get_format(parent_path)
      if formatno >= MIN_SINGLE_DB_FORMAT:
      	return formatno

  return formatno

def print_format(wc_path):
  # see subversion/libsvn_wc/wc.h for format values and information
  #   1.0.x -> 1.3.x: format 4
  #   1.4.x: format 8
  #   1.5.x: format 9
  #   1.6.x: format 10
  #   1.7.x: format XXX
  formatno = get_format(wc_path)
  print '%s: %s' % (wc_path, formatno)


if __name__ == '__main__':
  paths = sys.argv[1:]
  if not paths:
    paths = ['.']
  for wc_path in paths:
    print_format(wc_path)

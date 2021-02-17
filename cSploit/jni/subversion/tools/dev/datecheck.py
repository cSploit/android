#!/usr/bin/env python
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

### This is a debugging script to test date-ordering in an SVN repository.

'''Tell which revisions are out of order w.r.t. date in a repository.
Takes "svn log -q -r1:HEAD" output, prints results like this:

      $ svn log -q -r1:HEAD | ./datecheck.py
      [...]
      r42           OK  2003-06-02 22:20:31 -0500
      r43           OK  2003-06-02 22:20:31 -0500
      r44           OK  2003-06-02 23:29:14 -0500
      r45           OK  2003-06-02 23:29:14 -0500
      r46           OK  2003-06-02 23:33:13 -0500
      r47           OK  2003-06-10 15:19:47 -0500
      r48       NOT OK  2003-06-02 23:33:13 -0500
      r49           OK  2003-06-10 15:19:48 -0500
      r50       NOT OK  2003-06-02 23:33:13 -0500
      [...]
'''

import sys
import time

log_msg_separator = "-" * 72 + "\n"

line = sys.stdin.readline()
last_date = 0
while line:

  if not line:
    break

  if line == log_msg_separator:
    line = sys.stdin.readline()
    continue

  # We're looking at a revision line like this:
  #
  # "r1 | svn | 2001-08-30 23:24:14 -0500 (Thu, 30 Aug 2001)"
  #
  # Parse out

  rev, ignored, date_full = line.split("|")
  rev = rev.strip()
  date_full = date_full.strip()

  # We only need the machine-readable portion of the date, so ignore
  # the parenthesized part on the end, which is meant for humans.

  # Get the "2004-06-02 00:15:08" part of "2004-06-02 00:15:08 -0500".
  date = date_full[0:19]
  # Get the "-0500" part of "2004-06-02 00:15:08 -0500".
  offset = date_full[20:25]

  # Parse the offset by hand and adjust the date accordingly, because
  # http://docs.python.org/lib/module-time.html doesn't seem to offer
  # a standard way to parse "-0500", "-0600", etc, suffixes.  Arggh.
  offset_sign    = offset[0:1]
  offset_hours   = int(offset[1:3])
  offset_minutes = int(offset[3:5])

  # Get a first draft of the date...
  date_as_int = time.mktime(time.strptime(date, "%Y-%m-%d %H:%M:%S"))
  # ... but it's still not correct, we must adjust for the offset.
  if offset_sign == "-":
    date_as_int -= (offset_hours * 3600)
    date_as_int -= (offset_minutes * 60)
  elif offset_sign == "+":
    date_as_int += (offset_hours * 3600)
    date_as_int += (offset_minutes * 60)
  else:
    sys.stderr.write("Error: unknown offset sign '%s'.\n" % offset_sign)
    sys.exit(1)

  ok_not_ok = "    OK"
  if last_date > date_as_int:
    ok_not_ok = "NOT OK"

  print("%-8s  %s  %s %s" % (rev, ok_not_ok, date, offset))
  last_date = date_as_int
  line = sys.stdin.readline()

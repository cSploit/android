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
#
# mailer-tweak.py: tweak the svn:date and svn:author properties
#                  on all revisions
#
# We need constant dates and authors for the revisions so that we can
# consistently compare an output against a known quantity.
#
# USAGE: ./mailer-tweak.py REPOS
#


import sys
import os
import getopt

from svn import fs, core

DATE_BASE = 1000000000
DATE_INCR = 10000


def tweak_dates(pool, home='.'):
  db_path = os.path.join(home, 'db')
  if not os.path.exists(db_path):
    db_path = home

  fsob = fs.new(None, pool)
  fs.open_berkeley(fsob, db_path)

  for i in range(fs.youngest_rev(fsob, pool)):
    # convert secs into microseconds, then a string
    date = core.svn_time_to_cstring((DATE_BASE+i*DATE_INCR) * 1000000L, pool)
    #print date
    fs.change_rev_prop(fsob, i+1, core.SVN_PROP_REVISION_DATE, date, pool)
    fs.change_rev_prop(fsob, i+1, core.SVN_PROP_REVISION_AUTHOR, 'mailer test', pool)

def main():
  if len(sys.argv) != 2:
    print('USAGE: %s REPOS' % sys.argv[0])
    sys.exit(1)

  core.run_app(tweak_dates, sys.argv[1])

if __name__ == '__main__':
  main()

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

import sys
import os

progname = os.path.basename(sys.argv[0])

def usage():
  print("Usage: %s SOURCEURL WCPATH [r]REVNUM[,] [...]" % progname)
  print("Try '%s --help' for more information" % progname)

def help():
  val = """This script is meant to ease the pain of merging and
reviewing revision(s) on a release branch (although it can be used to
merge and review revisions from any line of development to another).

To allow cutting and pasting from the STATUS file, revision numbers
can be space or comma-separated, and may also include the prefix
'r'.

Lastly, a file (named 'rev1-rev2-rev3.log') is created for you.
This file contains each merge command that was run, the log of the
revision that was merged, and the diff from the previous revision.

Examples:

  %s http://svn.apache.org/repos/asf/subversion/trunk svn-1.2.x-branch \
    r14041, r14149, r14186, r14194, r14238, r14273

  %s http://svn.apache.org/repos/asf/subversion/trunk svn-1.2.x-branch \
    14041 14149 14186 14194 14238 14273""" % (progname, progname)
  print(val)


if len(sys.argv) > 1 and sys.argv[1] == '--help':
  help()
  sys.exit(0)

if len(sys.argv) < 4:
  usage()
  sys.exit(255)

src_url = sys.argv[1]
wc_path = sys.argv[2]

# Tolerate comma separated lists of revs (e.g. "r234, r245, r251")
revs = []
for rev in sys.argv[3:]:
  orig_rev = rev
  if rev[-1:] == ',':
    rev = rev[:-1]

  if rev[:1] == 'r':
    rev = rev[1:]

  try:
    rev = int(rev)
  except ValueError:
    print("Encountered non integer revision '%s'" % orig_rev)
    usage()
    sys.exit(254)
  revs.append(rev)

# Make an easily reviewable logfile
logfile = "-".join([str(x) for x in revs]) + ".log"
log = open(logfile, 'w')

for rev in revs:
  merge_cmd = ("svn merge -r%i:%i %s %s" % (rev - 1, rev, src_url, wc_path))
  log_cmd = 'svn log -v -r%i %s' % (rev, src_url)
  diff_cmd = 'svn diff -r%i:%i %s' % (rev -1, rev, src_url)

  # Do the merge
  os.system(merge_cmd)

  # Write our header
  log.write("=" * 72 + '\n')
  log.write(merge_cmd + '\n')

  # Get our log
  fh = os.popen(log_cmd)
  while True:
    line = fh.readline()
    if not line:
      break
    log.write(line)
  fh.close()

  # Get our diff
  fh = os.popen(diff_cmd)
  while True:
    line = fh.readline()
    if not line:
      break
    log.write(line)

  # Write our footer
  log.write("=" * 72 + '\n' * 10)


log.close()
print("\nYour logfile is '%s'" % logfile)

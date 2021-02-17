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

### Repository lock checker.  Gets an exclusive lock on the provided
### repository, then runs db_stat to see if the lock counts have been
### reset to 0.  If not, prints the timestamp of the run and a message
### about accumulation.

DB_STAT = 'db_stat'


import sys
import os
import os.path
import time
import fcntl
import getopt
try:
  my_getopt = getopt.gnu_getopt
except AttributeError:
  my_getopt = getopt.getopt

def usage_and_exit(retval):
  if retval:
    out = sys.stderr
  else:
    out = sys.stdout
  out.write("""Usage: %s [OPTIONS] REPOS-PATH

Options:
  --help (-h)    : Show this usage message
  --non-blocking : Don't wait for a lock that can't be immediately obtained

Obtain an exclusive lock (waiting for one unless --non-blocking is
passed) on REPOS-PATH, then check its lock usage counts.  If there is
any accumulation present, report that accumulation to stdout.
""" % (os.path.basename(sys.argv[0])))
  sys.exit(retval)

def main():
  now_time = time.asctime()
  repos_path = None
  nonblocking = 0

  # Parse the options.
  optlist, args = my_getopt(sys.argv[1:], "h", ['non-blocking', 'help'])
  for opt, arg in optlist:
    if opt == '--help' or opt == '-h':
      usage_and_exit(0)
    if opt == '--non-blocking':
      nonblocking = 1
    else:
      usage_and_exit(1)

  # We need at least a path to work with, here.
  argc = len(args)
  if argc < 1 or argc > 1:
    usage_and_exit(1)
  repos_path = args[0]

  fd = open(os.path.join(repos_path, 'locks', 'db.lock'), 'a')
  try:
    # Get an exclusive lock on the repository lock file, but maybe
    # don't wait for it.
    try:
      mode = fcntl.LOCK_EX
      if nonblocking:
        mode = mode | fcntl.LOCK_NB
      fcntl.lockf(fd, mode)
    except IOError:
      sys.stderr.write("Error obtaining exclusive lock.\n")
      sys.exit(1)

    # Grab the db_stat results.
    lines = os.popen('%s -ch %s' % (DB_STAT, os.path.join(repos_path, 'db')))
    log_lines = []
    for line in lines:
      pieces = line.split('\t')
      if (pieces[1].find('current lock') != -1) and (int(pieces[0]) > 0):
        log = ''
        if not len(log_lines):
          log = log + "[%s] Lock accumulation for '%s'\n" \
                % (now_time, repos_path)
        log = log + ' ' * 27
        log = log + "%s\t%s" % (pieces[0], pieces[1])
        log_lines.append(log)
    if len(log_lines):
      sys.stdout.write(''.join(log_lines))
  finally:
    # Unlock the lockfile
    fcntl.lockf(fd, fcntl.LOCK_UN)
  fd.close()

if __name__ == "__main__":
  main()

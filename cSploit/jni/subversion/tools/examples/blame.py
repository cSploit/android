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
# USAGE: blame.py [-r REV] repos-path file
#

import sys
import os
import getopt
try:
  my_getopt = getopt.gnu_getopt
except AttributeError:
  my_getopt = getopt.getopt
import difflib
from svn import fs, core, repos

CHUNK_SIZE = 100000

def blame(path, filename, rev=None):

  annotresult = {}
  path = core.svn_path_canonicalize(path)

  repos_ptr = repos.open(path)
  fsob = repos.fs(repos_ptr)

  if rev is None:
    rev = fs.youngest_rev(fsob)
  filedata = ''
  for i in range(0, rev+1):
    root = fs.revision_root(fsob, i)
    if fs.check_path(root, filename) != core.svn_node_none:
      first = i
      break
  print("First revision is %d" % first)
  print("Last revision is %d" % rev)
  for i in range(first, rev+1):
    previousroot = root
    root = fs.revision_root(fsob, i)
    if i != first:
      if not fs.contents_changed(root, filename, previousroot, filename):
        continue

    file = fs.file_contents(root, filename)
    previousdata = filedata
    filedata = ''
    while True:
      data = core.svn_stream_read(file, CHUNK_SIZE)
      if not data:
        break
      filedata = filedata + data

    print("Current revision is %d" % i)
    diffresult = difflib.ndiff(previousdata.splitlines(1),
                               filedata.splitlines(1))
    #    print ''.join(diffresult)
    k = 0
    for j in diffresult:
      if j[0] == ' ':
        if k in annotresult:
          k = k + 1
          continue
        else:
          annotresult[k] = (i, j[2:])
          k = k + 1
          continue
      elif j[0] == '?':
        continue
      annotresult[k] = (i, j[2:])
      if j[0] != '-':
        k = k + 1
#    print ''.join(diffresult)
#  print annotresult
  for x in range(len(annotresult.keys())):
     sys.stdout.write("Line %d (rev %d):%s" % (x,
                                               annotresult[x][0],
                                               annotresult[x][1]))

def usage():
  print("USAGE: blame.py [-r REV] repos-path file")
  sys.exit(1)

def main():
  opts, args = getopt.getopt(sys.argv[1:], 'r:')
  if len(args) != 2:
    usage()
  rev = None
  for name, value in opts:
    if name == '-r':
      rev = int(value)
  blame(args[0], args[1], rev)

if __name__ == '__main__':
  main()

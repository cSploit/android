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
# USAGE: getfile.py [-r REV] repos-path file
#
# gets a file from an SVN repository, puts it to sys.stdout
#

import sys
import os
import getopt
try:
  my_getopt = getopt.gnu_getopt
except AttributeError:
  my_getopt = getopt.getopt

from svn import fs, core, repos

CHUNK_SIZE = 16384

def getfile(path, filename, rev=None):
  path = core.svn_path_canonicalize(path)
  repos_ptr = repos.open(path)
  fsob = repos.fs(repos_ptr)

  if rev is None:
    rev = fs.youngest_rev(fsob)
    print("Using youngest revision %s" % rev)

  root = fs.revision_root(fsob, rev)
  file = fs.file_contents(root, filename)
  while True:
    data = core.svn_stream_read(file, CHUNK_SIZE)
    if not data:
      break
    sys.stdout.write(data)

def usage():
  print("USAGE: getfile.py [-r REV] repos-path file")
  sys.exit(1)

def main():
  opts, args = my_getopt(sys.argv[1:], 'r:')
  if len(args) != 2:
    usage()
  rev = None
  for name, value in opts:
    if name == '-r':
      rev = int(value)
  getfile(args[0], args[1], rev)

if __name__ == '__main__':
  main()

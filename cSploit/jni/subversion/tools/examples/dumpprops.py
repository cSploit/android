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
# USAGE: dumprops.py [-r REV] repos-path [file]
#
# dump out the properties on a given path (recursively if given a dir)
#

import sys
import os
import getopt
try:
  my_getopt = getopt.gnu_getopt
except AttributeError:
  my_getopt = getopt.getopt
import pprint

from svn import fs, core, repos


def dumpprops(path, filename='', rev=None):
  path = core.svn_path_canonicalize(path)
  repos_ptr = repos.open(path)
  fsob = repos.fs(repos_ptr)

  if rev is None:
    rev = fs.youngest_rev(fsob)

  root = fs.revision_root(fsob, rev)
  print_props(root, filename)
  if fs.is_dir(root, filename):
    walk_tree(root, filename)

def print_props(root, path):
  raw_props = fs.node_proplist(root, path)
  # need to massage some buffers into strings for printing
  props = { }
  for key, value in raw_props.items():
    props[key] = str(value)

  print('--- %s' % path)
  pprint.pprint(props)

def walk_tree(root, path):
  for name in fs.dir_entries(root, path).keys():
    full = path + '/' + name
    print_props(root, full)
    if fs.is_dir(root, full):
      walk_tree(root, full)

def usage():
  print("USAGE: dumpprops.py [-r REV] repos-path [file]")
  sys.exit(1)

def main():
  opts, args = my_getopt(sys.argv[1:], 'r:')
  rev = None
  for name, value in opts:
    if name == '-r':
      rev = int(value)
  if len(args) == 2:
    dumpprops(args[0], args[1], rev)
  elif len(args) == 1:
    dumpprops(args[0], "", rev)
  else:
    usage()

if __name__ == '__main__':
  main()

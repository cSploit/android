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

import os, sys

SKIP = ['deprecated.c',
        'entries.c',
        'entries.h',
        'old-and-busted.c']

TERMS = ['svn_wc_adm_access_t',
         'svn_wc_entry_t',
         'svn_wc__node_',
         'svn_wc__db_temp_',
         'svn_wc__db_node_hidden',
         'svn_wc__loggy',
         'svn_wc__db_wq_add',
         ]


def get_files_in(path):
  names = os.listdir(path)
  for skip in SKIP:
    try:
      names.remove(skip)
    except ValueError:
      pass
  return [os.path.join(path, fname) for fname in names
          if fname.endswith('.c') or fname.endswith('.h')]


def count_terms_in(path):
  files = get_files_in(path)
  counts = {}
  for term in TERMS:
    counts[term] = 0
  for filepath in get_files_in(path):
    contents = open(filepath).read()
    for term in TERMS:
      counts[term] += contents.count(term)
  return counts


def print_report(wcroot):
  client = count_terms_in(os.path.join(wcroot, 'subversion', 'libsvn_client'))
  wc = count_terms_in(os.path.join(wcroot, 'subversion', 'libsvn_wc'))

  client_total = 0
  wc_total = 0

  FMT = '%22s |%14s |%10s |%6s'
  SEP = '%s+%s+%s+%s' % (23*'-', 15*'-', 11*'-', 7*'-')

  print(FMT % ('', 'libsvn_client', 'libsvn_wc', 'Total'))
  print(SEP)
  for term in TERMS:
    print(FMT % (term, client[term], wc[term], client[term] + wc[term]))
    client_total += client[term]
    wc_total += wc[term]
  print(SEP)
  print(FMT % ('Total', client_total, wc_total, client_total + wc_total))


def usage():
  print("""\
Usage: %s [WCROOT]
       %s --help

Show statistics related to outstanding WC-NG code conversion work
items in working copy branch root WCROOT.  If WCROOT is omitted, this
program will attempt to guess it using the assumption that it is being
run from within the working copy of interest."""
% (sys.argv[0], sys.argv[0]))

  sys.exit(0)


if __name__ == '__main__':
  if len(sys.argv) > 1:
    if '--help' in sys.argv[1:]:
      usage()

    print_report(sys.argv[1])
  else:
    cwd = os.path.abspath(os.getcwd())
    idx = cwd.rfind(os.sep + 'subversion')
    if idx > 0:
      wcroot = cwd[:idx]
    else:
      idx = cwd.rfind(os.sep + 'tools')
      if idx > 0:
        wcroot = cwd[:idx]
      elif os.path.exists(os.path.join(cwd, 'subversion')):
        wcroot = cwd
      else:
        print("ERROR: the root of 'trunk' cannot be located -- please provide")
        sys.exit(1)
    print_report(wcroot)

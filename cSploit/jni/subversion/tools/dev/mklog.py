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
# Read a diff from stdin, and output a log message template to stdout.
# Hint: It helps if the diff was generated using 'svn diff -x -p'
#
# Note: Don't completely trust the generated log message.  This script
# depends on the correct output of 'diff -x -p', which can sometimes get
# confused.

import sys, re

rm = re.compile('@@.*@@ (.*)\(.*$')

def main():
  for line in sys.stdin:
    if line[0:6] == 'Index:':
      print('\n* %s' % line[7:-1])
      prev_funcname = ''
      continue
    match = rm.search(line[:-1])
    if match:
      if prev_funcname == match.group(1):
        continue
      print('  (%s):' % match.group(1))
      prev_funcname = match.group(1)


if __name__ == '__main__':
  main()

#!/usr/bin/python
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
# Find places in our code where whitespace is erroneously used before
# the open-paren on a function all. This is typically manifested like:
#
#   return svn_some_function
#     (param1, param2, param3)
#
#
# USAGE: find-bad-style.py FILE1 FILE2 ...
#

import sys
import re

re_call = re.compile(r'^\s*\(')
re_func = re.compile(r'.*[a-z0-9_]{1,}\s*$')


def scan_file(fname):
  lines = open(fname).readlines()

  prev = None
  line_num = 1

  for line in lines:
    if re_call.match(line):
      if prev and re_func.match(prev):
        print('%s:%d:%s' % (fname, line_num - 1, prev.rstrip()))

    prev = line
    line_num += 1


if __name__ == '__main__':
  for fname in sys.argv[1:]:
    scan_file(fname)

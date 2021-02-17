#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# which-error.py: Print semantic Subversion error code names mapped from
#                 their numeric error code values
#
# ====================================================================
#    Licensed to the Apache Software Foundation (ASF) under one
#    or more contributor license agreements.  See the NOTICE file
#    distributed with this work for additional information
#    regarding copyright ownership.  The ASF licenses this file
#    to you under the Apache License, Version 2.0 (the
#    "License"); you may not use this file except in compliance
#    with the License.  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing,
#    software distributed under the License is distributed on an
#    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#    KIND, either express or implied.  See the License for the
#    specific language governing permissions and limitations
#    under the License.
# ====================================================================
#
# $HeadURL: http://svn.apache.org/repos/asf/subversion/branches/1.7.x/tools/dev/which-error.py $
# $LastChangedDate: 2011-07-08 13:53:27 +0000 (Fri, 08 Jul 2011) $
# $LastChangedBy: philip $
# $LastChangedRevision: 1144315 $
#

import sys
import os.path
import re

try:
  from svn import core
except ImportError, e:
  sys.stderr.write("ERROR: Unable to import Subversion's Python bindings: '%s'\n" \
                   "Hint: Set your PYTHONPATH environment variable, or adjust your " \
                   "PYTHONSTARTUP\nfile to point to your Subversion install " \
                   "location's svn-python directory.\n" % e)
  sys.stderr.flush()
  sys.exit(1)


def usage_and_exit():
  progname = os.path.basename(sys.argv[0])
  sys.stderr.write("""Usage: 1. %s ERRNUM [...]
       2. %s parse
       3. %s list

Print numeric and semantic error code information for Subversion error
codes.  This can be done in variety of ways:

   1. For each ERRNUM, list the error code information.

   2. Parse standard input as if it was error stream from a debug-mode
      Subversion command-line client, echoing that input to stdout,
      followed by the error code information for codes found in use in
      that error stream.

   3. Simply list the error code information for all known such
      mappings.

""" % (progname, progname, progname))
  sys.exit(1)

def get_errors():
  errs = {}
  for key in vars(core):
    if key.find('SVN_ERR_') == 0:
      try:
        val = int(vars(core)[key])
        errs[val] = key
      except:
        pass
  return errs

def print_error(code):
  try:
    print('%08d  %s' % (code, __svn_error_codes[code]))
  except KeyError:
    print('%08d  *** UNKNOWN ERROR CODE ***' % (code))

if __name__ == "__main__":
  global __svn_error_codes
  __svn_error_codes = get_errors()
  codes = []
  if len(sys.argv) < 2:
    usage_and_exit()

  # Get a list of known codes
  if sys.argv[1] == 'list':
    if len(sys.argv) > 2:
      usage_and_exit()
    codes = sorted(__svn_error_codes.keys())

  # Get a list of code by parsing stdin for apr_err=CODE instances
  elif sys.argv[1] == 'parse':
    if len(sys.argv) > 2:
      usage_and_exit()
    while True:
      line = sys.stdin.readline()
      if not line:
        break
      sys.stdout.write(line)
      match = re.match(r'^.*apr_err=([0-9]+)[^0-9].*$', line)
      if match:
        codes.append(int(match.group(1)))

  # Get the list of requested codes
  else:
    for code in sys.argv[1:]:
      try:
        code = code.lstrip('EW')
        codes.append(int(code))
      except ValueError:
        usage_and_exit()

  # Print the harvest codes
  for code in codes:
    print_error(code)



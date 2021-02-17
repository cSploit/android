#!/usr/bin/env python
#
#  svneditor.py: a mock $SVN_EDITOR for the Subversion test suite
#
#  Subversion is a tool for revision control.
#  See http://subversion.apache.org for more information.
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
######################################################################

import sys
import os

def main():
    if len(sys.argv) not in [2, 6]:
        print("usage: svneditor.py file")
        print("       svneditor.py base theirs mine merged wc_path")
        print("arguments passed were: %s" % sys.argv)
        sys.exit(1)

    if len(sys.argv) == 2:
      filename = sys.argv[1]
    elif len(sys.argv) == 6:
      filename = sys.argv[4]

    # Read in the input file.
    f = open(filename)
    contents = f.read()
    f.close()

    funcname = os.environ['SVNTEST_EDITOR_FUNC']
    func = sys.modules['__main__'].__dict__[funcname]

    # Run the conversion.
    contents = func(contents)

    # Write edited version back to the file.
    f = open(filename, 'w')
    f.write(contents)
    f.close()
    return check_conflicts(contents)

def check_conflicts(contents):
    markers = ['<<<<<<<', '=======', '>>>>>>>']
    found = 0
    for line in contents.split('\n'):
      for marker in markers:
        if line.startswith(marker):
          found = found + 1
    return found >= 3

def foo_to_bar(m):
    return m.replace('foo', 'bar')

def append_foo(m):
    return m + 'foo\n'

def identity(m):
    return m

exitcode = main()
sys.exit(exitcode)

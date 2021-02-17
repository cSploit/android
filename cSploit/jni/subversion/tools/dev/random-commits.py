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
# USAGE: random-commits.py
#
# Using the FILELIST (see config below), a series of COUNT commits will be
# constructed, each changing up to MAXFILES files per commit. The commands
# will be sent to stdout (formatted as a shell script).
#
# The FILELIST can be constructed using the find-textfiles script.
#

import random

FILELIST = 'textfiles'
COUNT = 1000	# this many commits
MAXFILES = 10	# up to 10 files at a time

files = open(FILELIST).readlines()

print('#!/bin/sh')

for i in range(COUNT):
    n = random.randrange(1, MAXFILES+1)
    l = [ ]
    print("echo '--- begin commit #%d -----------------------------------'" % (i+1,))
    for j in range(n):
        fname = random.choice(files)[:-1]	# strip trailing newline
        print("echo 'part of change #%d' >> %s" % (i+1, fname))
        l.append(fname)
    print("svn commit -m 'commit #%d' %s" % (i+1, ' '.join(l)))

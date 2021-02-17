#!/usr/bin/env python
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


from csvn.repos import *
from csvn.auth import User
import csvn.core
import time, sys, gc
from optparse import OptionParser
import textwrap

usage = """python log.py [OPTION]... URL

Print all of the log messages for a repository at a given URL,
similar to 'svn log URL'. Useful for demo purposes only.
"""

parser = OptionParser(usage=usage)
parser.add_option("-u", "--username", dest="username",
                  help="commit the changes as USERNAME")
parser.add_option("-v", "--verbose", dest="verbose",
                  action='store_true', help="verbose mode",
                  default=False)
parser.add_option("", "--stop-on-copy", dest="stop_on_copy",
                  action='store_true', help="verbose mode",
                  default=False)
parser.add_option("-r", "", dest="rev",
                  help="revision range")

(options, args) = parser.parse_args()

if len(args) != 1:
    parser.print_help()
    sys.exit(1)

csvn.core.svn_cmdline_init("", csvn.core.stderr)
repos_url = args[0]
revs = options.rev
if revs and ":" in revs:
    [start_rev, end_rev] = revs.split(":")
elif revs:
    start_rev = revs
    end_rev = revs
else:
    start_rev = 1
    end_rev = "HEAD"

session = RemoteRepository(repos_url, user=User(options.username))

if end_rev == "HEAD":
    end_rev = session.latest_revnum()
if start_rev == "HEAD":
    start_rev = session.latest_revnum()
start_rev = int(start_rev)
end_rev = int(end_rev)

def count_lines(message):
    num_lines = message.count("\n") + 1
    if num_lines == 1:
        num_lines = "1 line"
    else:
        num_lines = "%d lines" % num_lines
    return num_lines

for entry in \
     session.log(start_rev, end_rev, discover_changed_paths=options.verbose,
                 stop_on_copy=options.stop_on_copy):

    num_lines = count_lines(entry.message)
    print("-" * 72)
    print("r%d | %s | %s | %s" % (entry.revision, entry.author,
                                  entry.date.as_human_string(), num_lines))
    if options.verbose:
        print("Changed paths:")
        for key, value in entry.changed_paths.items():
            value = value[0]
            if value.copyfrom_rev != SVN_INVALID_REVNUM:
                print("   %s %s (from %s:%d)" % (value.action, key,
                                                 value.copyfrom_path,
                                                 value.copyfrom_rev))
            else:
                print("   %s %s" % (value.action, key))
    print("")
    print(entry.message)

print("-" * 72)

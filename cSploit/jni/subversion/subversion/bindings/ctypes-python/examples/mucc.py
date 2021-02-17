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


# Multiple URL Command Client
#
# Combine a list of mv, cp, rm, and put commands on URLs into a single commit
#
# To read the help for this program, type python mucc.py --help

import os
from csvn.core import *
from csvn.repos import RemoteRepository, RepositoryURI
from csvn.auth import User
from optparse import OptionParser

usage = """python mucc.py [OPTION]... [ACTION]...

Actions:
  cp REV URL1 URL2      copy URL1@REV to URL2
  mkdir URL             create new directory URL
  mv URL1 URL2          move URL1 to URL2
  rm URL                delete URL
  put SRC-FILE URL      add or modify file URL with contents copied
                        from SRC-FILE
  propset NAME VAL URL  Set property NAME on URL to value VAL
  propdel NAME URL      Delete property NAME from URL
"""

# Read and parse options
parser = OptionParser(usage=usage)
parser.add_option("-m", "--message", dest="message",
                  help="use MESSAGE as a log message")
parser.add_option("-F", "--file", dest="file",
                  help="read log message from FILE")
parser.add_option("-u", "--username", dest="username",
                  help="commit the changes as USERNAME")
parser.add_option("-p", "--password", dest="password",
                  help="use password PASSWORD")
parser.add_option("-U", "--root-url", dest="root_url",
                  help="Interpret all action URLs as relative to ROOT_URL")
parser.add_option("-r", "--revision", dest="rev",
                  help="Use REV as baseline for changes")
parser.add_option("-X", "--extra-args ARG", dest="extra_args",
                  help='append arguments from file EXTRA_ARGS (one per line; '
                       'use "-" to read from standard input)')

(options, args) = parser.parse_args()

# Read any extra arguments
if options.extra_args:
    f = open(options.extra_args)
    for line in f:
        args.append(line.strip())

if not args:
    parser.print_help()
    sys.exit(1)

# Initialize variables
root_url = options.root_url
actions = []
svn_cmdline_init("", stderr)
pool = Pool()
action = None
if root_url:
    anchor = RepositoryURI(root_url)
else:
    anchor = None
states = None
ancestor = None

# A list of the arguments accepted by each command
cmds = {
    "cp": [ "rev", "url", "url" ],
    "mkdir": [ "url" ],
    "mv": [ "url", "url" ],
    "rm": [ "url" ],
    "put": [ "file", "url" ],
    "propset": [ "name", "val", "url" ],
    "propdel": [ "name", "url" ],
}

# Build up a list of the actions we want to perform
for arg in args:
    if not states:
        action = [arg]
        actions.append((arg, action))
        states = list(cmds[arg])
        states.reverse()
    else:
        state = states.pop()
        if state == "rev":
            action.append(arg.upper() != "HEAD" and int(arg) or None)
        elif state == "url":
            arg = RepositoryURI(arg)
            if anchor:
                arg = anchor.join(arg)
            action.append(arg)

            # It's legal to make a copy of the repository root,
            # so, we should treat copyfrom paths as possible
            # repository roots
            may_be_root = (len(action) == 2 and action[0] == "cp")

            if not may_be_root:
                arg = arg.dirname()

            if ancestor:
                ancestor = ancestor.longest_ancestor(arg)
            else:
                ancestor = arg
        else:
            action.append(arg)

session = RemoteRepository(ancestor, user=User(username=options.username))
txn = session.txn()

# Carry out the transaction
for action, args in actions:
    if action == "cp":
        txn.copy(src_rev=args[1], src_path=args[2], dest_path=args[3])
    elif action == "mv":
        txn.delete(str(args[1]))
        txn.copy(src_path=args[1], dest_path=args[2])
    elif action == "rm":
        txn.delete(args[1])
    elif action == "mkdir":
        txn.mkdir(args[1])
    elif action == "put":
        txn.upload(local_path=args[1], remote_path=args[2])
    elif action == "propset":
        txn.propset(key=args[1], value=args[2], path=args[3])
    elif action == "propdel":
        txn.propdel(key=args[1], path=args[2])


# Get the log message
message = options.message
if options.file:
    message = open(options.file).read()

# Finally commit
txn.commit(message)
print("r%ld committed by %s at %s" % (txn.committed_rev, options.username,
                                      txn.committed_date))

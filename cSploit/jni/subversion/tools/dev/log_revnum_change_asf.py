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

"""
Script to change old (svn.collab.net) revision numbers in subversion log
messages to new ASF subversion repository revision numbers.
"""

USAGE = """python log_revnum_change_asf.py [OPTION]... URL

Change the revision numbers relatively in the log messages of new ASF
subversion repository.
"""

from csvn.repos import RemoteRepository
from csvn.auth import User
import csvn.core
from optparse import OptionParser
import sys
import re

def repl_newrev(matchobj):
    """
    Revision to be substituted is provided here.
    """
    if matchobj.group(0):
        old_rev = int(matchobj.group(0)[1:])
        if old_rev <= 45000:
            return 'r'+str(old_rev + 840074)
        else:
            return 'r'+str(old_rev)

def main():
    """
    Script execution starts here.
    """

    parser = OptionParser(usage=USAGE)
    parser.add_option("-u", "", dest="username",
                      help="commit the changes as USERNAME")
    parser.add_option("-p", "", dest="password",
                      help="commit the changes with PASSWORD")
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

    session = RemoteRepository(repos_url, user=User(options.username,
                                                    options.password))

    if end_rev == "HEAD":
        end_rev = session.latest_revnum()
    if start_rev == "HEAD":
        start_rev = session.latest_revnum()
    start_rev = int(start_rev)
    end_rev = int(end_rev)

    for entry in session.log(start_rev, end_rev):
        new_log = re.sub(r'(r\d+)', repl_newrev, entry.message)
        session.revprop_set(propname='svn:log',
                            propval=new_log,
                            revnum=entry.revision,
                            force=True)

if __name__ == "__main__":
    main()

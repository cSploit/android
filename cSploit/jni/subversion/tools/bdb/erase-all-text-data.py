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
# Erases the text of every file in a BDB repository
#

import sys, os
import skel, svnfs

def main():
  if len(sys.argv) == 2:
    dbhome = os.path.join(sys.argv[1], 'db')
    for i in ('', 'uuids', 'revisions', 'transactions', 'representations',
        'strings', 'changes', 'copies', 'nodes'):
      if not os.path.exists(os.path.join(dbhome, i)):
        sys.stderr.write("%s: '%s' is not a valid bdb svn repository\n" %
            (sys.argv[0], sys.argv[1]))
        sys.exit(1)
  else:
    sys.stderr.write("Usage: %s <bdb-svn-repository>\n" % sys.argv[0])
    sys.exit(1)

  print("WARNING!: This program will destroy all text data in the subversion")
  print("repository '%s'" % sys.argv[1])
  print("Do not proceed unless this is a *COPY* of your real repository")
  print("If this is really what you want to do, " \
      "type 'YESERASE' and press Return")
  if sys.version_info[0] >= 3:
    # Python >=3.0
    confirmation = input("Confirmation string> ")
  else:
    # Python <3.0
    confirmation = raw_input("Confirmation string> ")
  if confirmation != "YESERASE":
    print("Cancelled - confirmation string not matched")
    sys.exit(0)
  print("Opening database environment...")
  cur = None
  ctx = svnfs.Ctx(dbhome)
  try:
    cur = ctx.nodes_db.cursor()
    nodecount = 0
    newrep = skel.Rep()
    newrep.str = "empty"
    empty_fulltext_rep_skel = newrep.unparse()
    del newrep
    ctx.strings_db['empty'] = ""
    rec = cur.first()
    while rec:
      if rec[0] != "next-key":
        if (nodecount % 10000 == 0) and nodecount != 0:
          print("Processed %d nodes..." % nodecount)
        nodecount += 1
        node = skel.Node(rec[1])
        if node.kind == "file":
          rep = skel.Rep(ctx.reps_db[node.datarep])
          if rep.kind == "fulltext":
            if rep.str in ctx.strings_db:
              del ctx.strings_db[rep.str]
            ctx.reps_db[node.datarep] = empty_fulltext_rep_skel
          else:
            for w in rep.windows:
              if w.str in ctx.strings_db:
                del ctx.strings_db[w.str]
            ctx.reps_db[node.datarep] = empty_fulltext_rep_skel
      rec = cur.next()
    print("Processed %d nodes" % nodecount)
  finally:
    if cur:
      cur.close()
    ctx.close()
  print("Done")

if __name__ == '__main__':
  main()

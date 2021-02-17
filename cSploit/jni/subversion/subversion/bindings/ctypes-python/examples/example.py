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

import csvn.core
from csvn.repos import *
from csvn.auth import User
import os

csvn.core.svn_cmdline_init("", csvn.core.stderr)

if os.path.exists("/tmp/test-repos"):
    svn_repos_delete("/tmp/test-repos", Pool())
user = User(username="joecommitter")
repos = LocalRepository("/tmp/test-repos", user=user, create=True)
print("Repos UUID: %s" % repos.uuid())

# Create a new transaction
txn = repos.txn()

# You can create a file from a Python string
open("/tmp/contents.txt", "w").write("Hello world one!")
txn.upload("file1.txt", local_path="/tmp/contents.txt")

# ... or from a Python file
open("/tmp/contents.txt", "w").write("Hello world two!")
txn.upload("file2.txt", local_path="/tmp/contents.txt")

# Create some directories
txn.mkdir("a")
txn.mkdir("a/b")
txn.mkdir("a/d")

# Commit the transaction
new_rev = txn.commit("Create file1.txt and file2.txt. "
                     "Also create some directories")
print("Committed revision %d" % new_rev)


# Transaction number 2
txn = repos.txn()

# Copy a to c, but remove one of the subdirectories
txn.copy(src_path="a", dest_path="c")
txn.delete("c/b")

# Copy files around in the repository
txn.copy(src_path="file1.txt", dest_path="file3.txt")
txn.copy(src_path="file2.txt", dest_path="file4.txt")

# Modify some files while we're at it
open("/tmp/contents.txt", "w").write("Hello world one and a half!")
txn.upload("file1.txt", local_path="/tmp/contents.txt")

# Commit our changes
new_rev = txn.commit("Create copies of file1.txt, file2.txt, and some "
                     "random directories. Also modify file1.txt.")
print("Committed revision %d" % new_rev)

# Transaction number 3
txn = repos.txn()

# Replace file3.txt with the new version of file1.txt
txn.delete("file3.txt")
txn.copy(src_path="file1.txt", src_rev=2,
         dest_path="file3.txt")

# Commit our changes
new_rev = txn.commit("Replace file3.txt with a new copy of file1.txt")
print("Committed revision %d" % new_rev)

session = RemoteRepository("file:///tmp/test-repos", user=user)

# Create a new transaction
txn = session.txn()

if session.check_path("blahdir") != svn_node_none:
    txn.delete("blahdir")
txn.mkdir("blahdir")
txn.mkdir("blahdir/dj")
txn.mkdir("blahdir/dj/a")
txn.mkdir("blahdir/dj/a/b")
txn.mkdir("blahdir/dj/a/b/c")
txn.mkdir("blahdir/dj/a/b/c/d")
txn.mkdir("blahdir/dj/a/b/c/d/e")
txn.mkdir("blahdir/dj/a/b/c/d/e/f")
txn.mkdir("blahdir/dj/a/b/c/d/e/f/g")
txn.upload("blahdir/dj/a/b/c/d/e/f/g/h.txt", "/tmp/contents.txt")

rev = txn.commit("create blahdir and descendents")
print("Committed revision %d" % rev)

def ignore(path, kind):
    basename = os.path.basename(path)
    _, ext = os.path.splitext(basename)
    return (basename == ".svn" or basename.endswith("~") or
            basename.startswith(".") or ext in (".pyc", ".pyo"))

def autoprop(txn, path, kind):

    # Ignore compiled/optimized python files
    if kind == svn_node_dir:
        txn.propset(path, "svn:ignore", "*.py[co]")

    # Set eol-style to native for python files and text files
    if kind == svn_node_file and (path.endswith(".py") or
                                  path.endswith(".txt")):
        txn.propset(path, "svn:eol-style", "native")

txn = session.txn()
txn.ignore(ignore)
txn.autoprop(autoprop)
txn.upload("csvn", local_path="csvn")
rev = txn.commit("import csvn dir")
print("Committed revision %d" % rev)

txn = session.txn()
txn.copy(src_path="csvn", dest_path="csvn2")
txn.upload("csvn2/core/functions.py","/tmp/contents.txt")
rev = txn.commit("Copied csvn to csvn2, messing around with functions.py")
print("Committed revision %d" % rev)

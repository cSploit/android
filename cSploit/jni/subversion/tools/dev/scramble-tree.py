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
# scramble-tree.py:  (See scramble-tree.py --help.)
#
# Makes multiple random file changes to a directory tree, for testing.
#
# This script will add some new files, remove some existing files, add
# text to some existing files, and delete text from some existing
# files.  It will also leave some files completely untouched.
#
# The exact set of changes made is always the same for identical trees,
# where "identical" means the names of files and directories are the
# same, and they are arranged in the same tree structure (the actual
# contents of files may differ).  If two are not identical, the sets of
# changes scramble-tree.py will make may differ arbitrarily.
#
# Directories named .svn/ and CVS/ are ignored.
#
# Example scenario, starting with a pristine Subversion working copy:
#
#   $ ls
#   foo/
#   $ svn st foo
#   $ cp -r foo bar
#   $ svn st bar
#   $ scramble-tree.py foo
#   $ svn st foo
#   [... see lots of scary status output ...]
#   $ scramble-tree.py bar
#   [... see the exact same scary status output ...]
#   $ scramble-tree.py foo
#   [... see a new bunch of scary status output ...]
#   $

import os
import sys
import getopt
try:
  my_getopt = getopt.gnu_getopt
except AttributeError:
  my_getopt = getopt.getopt
import random
try:
  # Python >=2.5
  from hashlib import md5 as hashlib_md5
except ImportError:
  # Python <2.5
  from md5 import md5 as hashlib_md5
import base64


class VCActions:
  def __init__(self):
    pass
  def add_file(self, path):
    """Add an existing file to version control."""
    pass
  def remove_file(self, path):
    """Remove an existing file from version control, and delete it."""
    pass


class NoVCActions(VCActions):
  def remove_file(self, path):
    os.unlink(path)


class CVSActions(VCActions):
  def add_file(self, path):
    cwd = os.getcwd()
    try:
      dirname, basename = os.path.split(path)
      os.chdir(os.path.join(cwd, dirname))
      os.system('cvs -Q add -m "Adding file to repository" "%s"' % (basename))
    finally:
      os.chdir(cwd)
  def remove_file(self, path):
    cwd = os.getcwd()
    try:
      dirname, basename = os.path.split(path)
      os.chdir(os.path.join(cwd, dirname))
      os.system('cvs -Q rm -f "%s"' % (basename))
    finally:
      os.chdir(cwd)


class SVNActions(VCActions):
  def add_file(self, path):
    os.system('svn add --quiet "%s"' % (path))
  def remove_file(self, path):
    os.remove(path)
    os.system('svn rm --quiet --force "%s"' % (path))


class hashDir:
  """Given a directory, creates a string containing all directories
  and files under that directory (sorted alphanumerically) and makes a
  base64-encoded md5 hash of the resulting string.  Call
  hashDir.gen_seed() to generate a seed value for this tree."""

  def __init__(self, rootdir):
    self.allfiles = []
    for dirpath, dirs, files in os.walk(rootdir):
      self.walker_callback(len(rootdir), dirpath, dirs + files)

  def gen_seed(self):
    # Return a base64-encoded (kinda ... strip the '==\n' from the
    # end) MD5 hash of sorted tree listing.
    self.allfiles.sort()
    return base64.encodestring(hashlib_md5(''.join(self.allfiles)).digest())[:-3]

  def walker_callback(self, baselen, dirname, fnames):
    if ((dirname == '.svn') or (dirname == 'CVS')):
      return
    self.allfiles.append(dirname[baselen:])
    for filename in fnames:
      path = os.path.join(dirname, filename)
      if not os.path.isdir(path):
        self.allfiles.append(path[baselen:])


class Scrambler:
  def __init__(self, seed, vc_actions, dry_run, quiet):
    if not quiet:
      print('SEED: ' + seed)

    self.rand = random.Random(seed)
    self.vc_actions = vc_actions
    self.dry_run = dry_run
    self.quiet = quiet
    self.ops = []  ### ["add" | "munge", path]
    self.greeking = """
======================================================================
This is some text that was inserted into this file by the lovely and
talented scramble-tree.py script.
======================================================================
"""

  ### Helpers
  def shrink_list(self, list, remove_count):
    if len(list) <= remove_count:
      return []
    for i in range(remove_count):
      j = self.rand.randrange(len(list) - 1)
      del list[j]
    return list

  def _make_new_file(self, dir):
    i = 0
    path = None
    for i in range(99999):
      path = os.path.join(dir, "newfile.%05d.txt" % i)
      if not os.path.exists(path):
        open(path, 'w').write(self.greeking)
        return path
    raise Exception("Ran out of unique new filenames in directory '%s'" % dir)

  ### File Mungers
  def _mod_append_to_file(self, path):
    if not self.quiet:
      print('append_to_file: %s' % path)
    if self.dry_run:
      return
    fh = open(path, "a")
    fh.write(self.greeking)
    fh.close()

  def _mod_remove_from_file(self, path):
    if not self.quiet:
      print('remove_from_file: %s' % path)
    if self.dry_run:
      return
    lines = self.shrink_list(open(path, "r").readlines(), 5)
    open(path, "w").writelines(lines)

  def _mod_delete_file(self, path):
    if not self.quiet:
      print('delete_file: %s' % path)
    if self.dry_run:
      return
    self.vc_actions.remove_file(path)

  ### Public Interfaces
  def get_randomizer(self):
    return self.rand

  def schedule_munge(self, path):
    self.ops.append(tuple(["munge", path]))

  def schedule_addition(self, dir):
    self.ops.append(tuple(["add", dir]))

  def enact(self, limit):
    num_ops = len(self.ops)
    if limit == 0:
      return
    elif limit > 0 and limit <= num_ops:
      self.ops = self.shrink_list(self.ops, num_ops - limit)
    for op, path in self.ops:
      if op == "add":
        path = self._make_new_file(path)
        if not self.quiet:
          print("add_file: %s" % path)
        if self.dry_run:
          return
        self.vc_actions.add_file(path)
      elif op == "munge":
        file_mungers = [self._mod_append_to_file,
                        self._mod_append_to_file,
                        self._mod_append_to_file,
                        self._mod_remove_from_file,
                        self._mod_remove_from_file,
                        self._mod_remove_from_file,
                        self._mod_delete_file,
                        ]
        self.rand.choice(file_mungers)(path)


def usage(retcode=255):
  print('Usage: %s [OPTIONS] DIRECTORY' % (sys.argv[0]))
  print('')
  print('Options:')
  print('    --help, -h  : Show this usage message.')
  print('    --seed ARG  : Use seed ARG to scramble the tree.')
  print('    --use-svn   : Use Subversion (as "svn") to perform file additions')
  print('                  and removals.')
  print('    --use-cvs   : Use CVS (as "cvs") to perform file additions')
  print('                  and removals.')
  print('    --dry-run   : Don\'t actually change the disk.')
  print('    --limit N   : Limit the scrambling to a maximum of N operations.')
  print('    --quiet, -q : Run in stealth mode!')
  sys.exit(retcode)


def walker_callback(scrambler, dirname, fnames):
  if ((dirname.find('.svn') != -1) or dirname.find('CVS') != -1):
    return
  rand = scrambler.get_randomizer()
  if rand.randrange(5) == 1:
    scrambler.schedule_addition(dirname)
  for filename in fnames:
    path = os.path.join(dirname, filename)
    if not os.path.isdir(path) and rand.randrange(3) == 1:
      scrambler.schedule_munge(path)


def main():
  seed = None
  vc_actions = NoVCActions()
  dry_run = 0
  quiet = 0
  limit = None

  # Mm... option parsing.
  optlist, args = my_getopt(sys.argv[1:], "hq",
                            ['seed=', 'use-svn', 'use-cvs',
                             'help', 'quiet', 'dry-run', 'limit='])
  for opt, arg in optlist:
    if opt == '--help' or opt == '-h':
      usage(0)
    if opt == '--seed':
      seed = arg
    if opt == '--use-svn':
      vc_actions = SVNActions()
    if opt == '--use-cvs':
      vc_actions = CVSActions()
    if opt == '--dry-run':
      dry_run = 1
    if opt == '--limit':
      limit = int(arg)
    if opt == '--quiet' or opt == '-q':
      quiet = 1

  # We need at least a path to work with, here.
  argc = len(args)
  if argc < 1 or argc > 1:
    usage()
  rootdir = args[0]

  # If a seed wasn't provide, calculate one.
  if seed is None:
    seed = hashDir(rootdir).gen_seed()
  scrambler = Scrambler(seed, vc_actions, dry_run, quiet)
  for dirpath, dirs, files in os.walk(rootdir):
    walker_callback(scrambler, dirpath, dirs + files)
  scrambler.enact(limit)

if __name__ == '__main__':
  main()

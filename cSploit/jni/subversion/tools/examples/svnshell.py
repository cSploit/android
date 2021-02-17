#!/usr/bin/env python
#
# svnshell.py : a Python-based shell interface for cruising 'round in
#               the filesystem.
#
######################################################################
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
#

import sys
import time
import re
from cmd import Cmd
from random import randint
from svn import fs, core, repos


class SVNShell(Cmd):
  def __init__(self, path):
    """initialize an SVNShell object"""
    Cmd.__init__(self)
    path = core.svn_path_canonicalize(path)
    self.fs_ptr = repos.fs(repos.open(path))
    self.is_rev = 1
    self.rev = fs.youngest_rev(self.fs_ptr)
    self.txn = None
    self.root = fs.revision_root(self.fs_ptr, self.rev)
    self.path = "/"
    self._setup_prompt()
    self.cmdloop()

  def precmd(self, line):
    if line == "EOF":
      # Ctrl-D is a command without a newline.  Print a newline, so the next
      # shell prompt is not on the same line as the last svnshell prompt.
      print("")
      return "exit"
    return line

  def postcmd(self, stop, line):
    self._setup_prompt()

  _errors = ["Huh?",
             "Whatchoo talkin' 'bout, Willis?",
             "Say what?",
             "Nope.  Not gonna do it.",
             "Ehh...I don't think so, chief."]

  def default(self, line):
    print(self._errors[randint(0, len(self._errors) - 1)])

  def do_cat(self, arg):
    """dump the contents of a file"""
    if not len(arg):
      print("You must supply a file path.")
      return
    catpath = self._parse_path(arg)
    kind = fs.check_path(self.root, catpath)
    if kind == core.svn_node_none:
      print("Path '%s' does not exist." % catpath)
      return
    if kind == core.svn_node_dir:
      print("Path '%s' is not a file." % catpath)
      return
    ### be nice to get some paging in here.
    stream = fs.file_contents(self.root, catpath)
    while True:
      data = core.svn_stream_read(stream, core.SVN_STREAM_CHUNK_SIZE)
      sys.stdout.write(data)
      if len(data) < core.SVN_STREAM_CHUNK_SIZE:
        break

  def do_cd(self, arg):
    """change directory"""
    newpath = self._parse_path(arg)

    # make sure that path actually exists in the filesystem as a directory
    kind = fs.check_path(self.root, newpath)
    if kind != core.svn_node_dir:
      print("Path '%s' is not a valid filesystem directory." % newpath)
      return
    self.path = newpath

  def do_ls(self, arg):
    """list the contents of the current directory or provided path"""
    parent = self.path
    if not len(arg):
      # no arg -- show a listing for the current directory.
      entries = fs.dir_entries(self.root, self.path)
    else:
      # arg?  show a listing of that path.
      newpath = self._parse_path(arg)
      kind = fs.check_path(self.root, newpath)
      if kind == core.svn_node_dir:
        parent = newpath
        entries = fs.dir_entries(self.root, parent)
      elif kind == core.svn_node_file:
        parts = self._path_to_parts(newpath)
        name = parts.pop(-1)
        parent = self._parts_to_path(parts)
        print(parent + ':' + name)
        tmpentries = fs.dir_entries(self.root, parent)
        if not tmpentries.get(name, None):
          return
        entries = {}
        entries[name] = tmpentries[name]
      else:
        print("Path '%s' not found." % newpath)
        return

    keys = sorted(entries.keys())

    print("   REV   AUTHOR  NODE-REV-ID     SIZE         DATE NAME")
    print("----------------------------------------------------------------------------")

    for entry in keys:
      fullpath = parent + '/' + entry
      size = ''
      is_dir = fs.is_dir(self.root, fullpath)
      if is_dir:
        name = entry + '/'
      else:
        size = str(fs.file_length(self.root, fullpath))
        name = entry
      node_id = fs.unparse_id(entries[entry].id)
      created_rev = fs.node_created_rev(self.root, fullpath)
      author = fs.revision_prop(self.fs_ptr, created_rev,
                                core.SVN_PROP_REVISION_AUTHOR)
      if not author:
        author = ""
      date = fs.revision_prop(self.fs_ptr, created_rev,
                              core.SVN_PROP_REVISION_DATE)
      if not date:
        date = ""
      else:
        date = self._format_date(date)

      print("%6s %8s %12s %8s %12s %s" % (created_rev, author[:8],
                                          node_id, size, date, name))

  def do_lstxns(self, arg):
    """list the transactions available for browsing"""
    txns = sorted(fs.list_transactions(self.fs_ptr))
    counter = 0
    for txn in txns:
      counter = counter + 1
      sys.stdout.write("%8s   " % txn)
      if counter == 6:
        print("")
        counter = 0
    print("")

  def do_pcat(self, arg):
    """list the properties of a path"""
    catpath = self.path
    if len(arg):
      catpath = self._parse_path(arg)
    kind = fs.check_path(self.root, catpath)
    if kind == core.svn_node_none:
      print("Path '%s' does not exist." % catpath)
      return
    plist = fs.node_proplist(self.root, catpath)
    if not plist:
      return
    for pkey, pval in plist.items():
      print('K ' + str(len(pkey)))
      print(pkey)
      print('P ' + str(len(pval)))
      print(pval)
    print('PROPS-END')

  def do_setrev(self, arg):
    """set the current revision to view"""
    try:
      if arg.lower() == 'head':
        rev = fs.youngest_rev(self.fs_ptr)
      else:
        rev = int(arg)
      newroot = fs.revision_root(self.fs_ptr, rev)
    except:
      print("Error setting the revision to '" + arg + "'.")
      return
    fs.close_root(self.root)
    self.root = newroot
    self.rev = rev
    self.is_rev = 1
    self._do_path_landing()

  def do_settxn(self, arg):
    """set the current transaction to view"""
    try:
      txnobj = fs.open_txn(self.fs_ptr, arg)
      newroot = fs.txn_root(txnobj)
    except:
      print("Error setting the transaction to '" + arg + "'.")
      return
    fs.close_root(self.root)
    self.root = newroot
    self.txn = arg
    self.is_rev = 0
    self._do_path_landing()

  def do_youngest(self, arg):
    """list the youngest revision available for browsing"""
    rev = fs.youngest_rev(self.fs_ptr)
    print(rev)

  def do_exit(self, arg):
    sys.exit(0)

  def _path_to_parts(self, path):
    return [_f for _f in path.split('/') if _f]

  def _parts_to_path(self, parts):
    return '/' + '/'.join(parts)

  def _parse_path(self, path):
    # cleanup leading, trailing, and duplicate '/' characters
    newpath = self._parts_to_path(self._path_to_parts(path))

    # if PATH is absolute, use it, else append it to the existing path.
    if path.startswith('/') or self.path == '/':
      newpath = '/' + newpath
    else:
      newpath = self.path + '/' + newpath

    # cleanup '.' and '..'
    parts = self._path_to_parts(newpath)
    finalparts = []
    for part in parts:
      if part == '.':
        pass
      elif part == '..':
        if len(finalparts) != 0:
          finalparts.pop(-1)
      else:
        finalparts.append(part)

    # finally, return the calculated path
    return self._parts_to_path(finalparts)

  def _format_date(self, date):
    date = core.svn_time_from_cstring(date)
    date = time.asctime(time.localtime(date / 1000000))
    return date[4:-8]

  def _do_path_landing(self):
    """try to land on self.path as a directory in root, failing up to '/'"""
    not_found = 1
    newpath = self.path
    while not_found:
      kind = fs.check_path(self.root, newpath)
      if kind == core.svn_node_dir:
        not_found = 0
      else:
        parts = self._path_to_parts(newpath)
        parts.pop(-1)
        newpath = self._parts_to_path(parts)
    self.path = newpath

  def _setup_prompt(self):
    """present the prompt and handle the user's input"""
    if self.is_rev:
      self.prompt = "<rev: " + str(self.rev)
    else:
      self.prompt = "<txn: " + self.txn
    self.prompt += " " + self.path + ">$ "

  def _complete(self, text, line, begidx, endidx, limit_node_kind=None):
    """Generic tab completer.  Takes the 4 standard parameters passed to a
    cmd.Cmd completer function, plus LIMIT_NODE_KIND, which should be a
    svn.core.svn_node_foo constant to restrict the returned completions to, or
    None for no limit.  Catches and displays exceptions, because otherwise
    they are silently ignored - which is quite frustrating when debugging!"""
    try:
      args = line.split()
      if len(args) > 1:
        arg = args[1]
      else:
        arg = ""
      dirs = arg.split('/')
      user_elem = dirs[-1]
      user_dir = "/".join(dirs[:-1] + [''])

      canon_dir = self._parse_path(user_dir)

      entries = fs.dir_entries(self.root, canon_dir)
      acceptable_completions = []
      for name, dirent_t in entries.items():
        if not name.startswith(user_elem):
          continue
        if limit_node_kind and dirent_t.kind != limit_node_kind:
          continue
        if dirent_t.kind == core.svn_node_dir:
          name += '/'
        acceptable_completions.append(name)
      if limit_node_kind == core.svn_node_dir or not limit_node_kind:
        if user_elem in ('.', '..'):
          for extraname in ('.', '..'):
            if extraname.startswith(user_elem):
              acceptable_completions.append(extraname + '/')
      return acceptable_completions
    except:
      ei = sys.exc_info()
      sys.stderr.write("EXCEPTION WHILST COMPLETING\n")
      import traceback
      traceback.print_tb(ei[2])
      sys.stderr.write("%s: %s\n" % (ei[0], ei[1]))
      raise

  def complete_cd(self, text, line, begidx, endidx):
    return self._complete(text, line, begidx, endidx, core.svn_node_dir)

  def complete_cat(self, text, line, begidx, endidx):
    return self._complete(text, line, begidx, endidx, core.svn_node_file)

  def complete_ls(self, text, line, begidx, endidx):
    return self._complete(text, line, begidx, endidx)

  def complete_pcat(self, text, line, begidx, endidx):
    return self._complete(text, line, begidx, endidx)


def _basename(path):
  "Return the basename for a '/'-separated path."
  idx = path.rfind('/')
  if idx == -1:
    return path
  return path[idx+1:]


def usage(exit):
  if exit:
    output = sys.stderr
  else:
    output = sys.stdout
  output.write(
    "usage: %s REPOS_PATH\n"
    "\n"
    "Once the program has started, type 'help' at the prompt for hints on\n"
    "using the shell.\n" % sys.argv[0])
  sys.exit(exit)

def main():
  if len(sys.argv) != 2:
    usage(1)

  SVNShell(sys.argv[1])

if __name__ == '__main__':
  main()

#!/usr/bin/env python
#
# svnlook.py : a Python-based replacement for svnlook
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
import os

from svn import core, fs, delta, repos

class SVNLook:
  def __init__(self, path, cmd, rev, txn):
    path = core.svn_path_canonicalize(path)
    repos_ptr = repos.open(path)
    self.fs_ptr = repos.fs(repos_ptr)

    if txn:
      self.txn_ptr = fs.open_txn(self.fs_ptr, txn)
    else:
      self.txn_ptr = None
      if rev is None:
        rev = fs.youngest_rev(self.fs_ptr)
    self.rev = rev

    getattr(self, 'cmd_' + cmd)()

  def cmd_default(self):
    self.cmd_info()
    self.cmd_tree()

  def cmd_author(self):
    # get the author property, or empty string if the property is not present
    author = self._get_property(core.SVN_PROP_REVISION_AUTHOR) or ''
    print(author)

  def cmd_changed(self):
    self._print_tree(ChangedEditor, pass_root=1)

  def cmd_date(self):
    if self.txn_ptr:
      print("")
    else:
      date = self._get_property(core.SVN_PROP_REVISION_DATE)
      if date:
        aprtime = core.svn_time_from_cstring(date)
        # ### convert to a time_t; this requires intimate knowledge of
        # ### the apr_time_t type
        secs = aprtime / 1000000  # aprtime is microseconds; make seconds

        # assume secs in local TZ, convert to tuple, and format
        ### we don't really know the TZ, do we?
        print(time.strftime('%Y-%m-%d %H:%M', time.localtime(secs)))
      else:
        print("")

  def cmd_diff(self):
    self._print_tree(DiffEditor, pass_root=1)

  def cmd_dirs_changed(self):
    self._print_tree(DirsChangedEditor)

  def cmd_ids(self):
    self._print_tree(Editor, base_rev=0, pass_root=1)

  def cmd_info(self):
    self.cmd_author()
    self.cmd_date()
    self.cmd_log(1)

  def cmd_log(self, print_size=0):
    # get the log property, or empty string if the property is not present
    log = self._get_property(core.SVN_PROP_REVISION_LOG) or ''
    if print_size:
      print(len(log))
    print(log)

  def cmd_tree(self):
    self._print_tree(Editor, base_rev=0)

  def _get_property(self, name):
    if self.txn_ptr:
      return fs.txn_prop(self.txn_ptr, name)
    return fs.revision_prop(self.fs_ptr, self.rev, name)

  def _print_tree(self, e_factory, base_rev=None, pass_root=0):
    if base_rev is None:
      # a specific base rev was not provided. use the transaction base,
      # or the previous revision
      if self.txn_ptr:
        base_rev = fs.txn_base_revision(self.txn_ptr)
      else:
        base_rev = self.rev - 1

    # get the current root
    if self.txn_ptr:
      root = fs.txn_root(self.txn_ptr)
    else:
      root = fs.revision_root(self.fs_ptr, self.rev)

    # the base of the comparison
    base_root = fs.revision_root(self.fs_ptr, base_rev)

    if pass_root:
      editor = e_factory(root, base_root)
    else:
      editor = e_factory()

    # construct the editor for printing these things out
    e_ptr, e_baton = delta.make_editor(editor)

    # compute the delta, printing as we go
    def authz_cb(root, path, pool):
      return 1
    repos.dir_delta(base_root, '', '', root, '',
		    e_ptr, e_baton, authz_cb, 0, 1, 0, 0)


class Editor(delta.Editor):
  def __init__(self, root=None, base_root=None):
    self.root = root
    # base_root ignored

    self.indent = ''

  def open_root(self, base_revision, dir_pool):
    print('/' + self._get_id('/'))
    self.indent = self.indent + ' '    # indent one space

  def add_directory(self, path, *args):
    id = self._get_id(path)
    print(self.indent + _basename(path) + '/' + id)
    self.indent = self.indent + ' '    # indent one space

  # we cheat. one method implementation for two entry points.
  open_directory = add_directory

  def close_directory(self, baton):
    # note: if indents are being performed, this slice just returns
    # another empty string.
    self.indent = self.indent[:-1]

  def add_file(self, path, *args):
    id = self._get_id(path)
    print(self.indent + _basename(path) + id)

  # we cheat. one method implementation for two entry points.
  open_file = add_file

  def _get_id(self, path):
    if self.root:
      id = fs.node_id(self.root, path)
      return ' <%s>' % fs.unparse_id(id)
    return ''

class DirsChangedEditor(delta.Editor):
  def open_root(self, base_revision, dir_pool):
    return [ 1, '' ]

  def delete_entry(self, path, revision, parent_baton, pool):
    self._dir_changed(parent_baton)

  def add_directory(self, path, parent_baton,
                    copyfrom_path, copyfrom_revision, dir_pool):
    self._dir_changed(parent_baton)
    return [ 1, path ]

  def open_directory(self, path, parent_baton, base_revision, dir_pool):
    return [ 1, path ]

  def change_dir_prop(self, dir_baton, name, value, pool):
    self._dir_changed(dir_baton)

  def add_file(self, path, parent_baton,
               copyfrom_path, copyfrom_revision, file_pool):
    self._dir_changed(parent_baton)

  def open_file(self, path, parent_baton, base_revision, file_pool):
    # some kind of change is going to happen
    self._dir_changed(parent_baton)

  def _dir_changed(self, baton):
    if baton[0]:
      # the directory hasn't been printed yet. do it.
      print(baton[1] + '/')
      baton[0] = 0

class ChangedEditor(delta.Editor):
  def __init__(self, root, base_root):
    self.root = root
    self.base_root = base_root

  def open_root(self, base_revision, dir_pool):
    return [ 1, '' ]

  def delete_entry(self, path, revision, parent_baton, pool):
    ### need more logic to detect 'replace'
    if fs.is_dir(self.base_root, '/' + path):
      print('D   ' + path + '/')
    else:
      print('D   ' + path)

  def add_directory(self, path, parent_baton,
                    copyfrom_path, copyfrom_revision, dir_pool):
    print('A   ' + path + '/')
    return [ 0, path ]

  def open_directory(self, path, parent_baton, base_revision, dir_pool):
    return [ 1, path ]

  def change_dir_prop(self, dir_baton, name, value, pool):
    if dir_baton[0]:
      # the directory hasn't been printed yet. do it.
      print('_U  ' + dir_baton[1] + '/')
      dir_baton[0] = 0

  def add_file(self, path, parent_baton,
               copyfrom_path, copyfrom_revision, file_pool):
    print('A   ' + path)
    return [ '_', ' ', None ]

  def open_file(self, path, parent_baton, base_revision, file_pool):
    return [ '_', ' ', path ]

  def apply_textdelta(self, file_baton, base_checksum):
    file_baton[0] = 'U'

    # no handler
    return None

  def change_file_prop(self, file_baton, name, value, pool):
    file_baton[1] = 'U'

  def close_file(self, file_baton, text_checksum):
    text_mod, prop_mod, path = file_baton
    # test the path. it will be None if we added this file.
    if path:
      status = text_mod + prop_mod
      # was there some kind of change?
      if status != '_ ':
        print(status + '  ' + path)


class DiffEditor(delta.Editor):
  def __init__(self, root, base_root):
    self.root = root
    self.base_root = base_root
    self.target_revision = 0

  def _do_diff(self, base_path, path):
    if base_path is None:
      print("Added: " + path)
      label = path
    elif path is None:
      print("Removed: " + base_path)
      label = base_path
    else:
      print("Modified: " + path)
      label = path
    print("===============================================================" + \
          "===============")
    args = []
    args.append("-L")
    args.append(label + "\t(original)")
    args.append("-L")
    args.append(label + "\t(new)")
    args.append("-u")
    differ = fs.FileDiff(self.base_root, base_path, self.root,
                         path, diffoptions=args)
    pobj = differ.get_pipe()
    while True:
      line = pobj.readline()
      if not line:
        break
      sys.stdout.write("%s " % line)
    print("")

  def _do_prop_diff(self, path, prop_name, prop_val, pool):
    print("Property changes on: " + path)
    print("_______________________________________________________________" + \
          "_______________")

    old_prop_val = None

    try:
      old_prop_val = fs.node_prop(self.base_root, path, prop_name, pool)
    except core.SubversionException:
      pass # Must be a new path

    if old_prop_val:
      if prop_val:
        print("Modified: " + prop_name)
        print("   - " + str(old_prop_val))
        print("   + " + str(prop_val))
      else:
        print("Deleted: " + prop_name)
        print("   - " + str(old_prop_val))
    else:
      print("Added: " + prop_name)
      print("   + " + str(prop_val))

    print("")

  def delete_entry(self, path, revision, parent_baton, pool):
    ### need more logic to detect 'replace'
    if not fs.is_dir(self.base_root, '/' + path):
      self._do_diff(path, None)

  def add_directory(self, path, parent_baton, copyfrom_path,
                    copyfrom_revision, dir_pool):
    return [ 1, path ]

  def add_file(self, path, parent_baton,
               copyfrom_path, copyfrom_revision, file_pool):
    self._do_diff(None, path)
    return [ '_', ' ', None ]

  def open_root(self, base_revision, dir_pool):
    return [ 1, '' ]

  def open_directory(self, path, parent_baton, base_revision, dir_pool):
    return [ 1, path ]

  def open_file(self, path, parent_baton, base_revision, file_pool):
    return [ '_', ' ', path ]

  def apply_textdelta(self, file_baton, base_checksum):
    if file_baton[2] is not None:
      self._do_diff(file_baton[2], file_baton[2])
    return None

  def change_file_prop(self, file_baton, name, value, pool):
    if file_baton[2] is not None:
      self._do_prop_diff(file_baton[2], name, value, pool)
    return None

  def change_dir_prop(self, dir_baton, name, value, pool):
    if dir_baton[1] is not None:
      self._do_prop_diff(dir_baton[1], name, value, pool)
    return None

  def set_target_revision(self, target_revision):
    self.target_revision = target_revision

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
     "usage: %s REPOS_PATH rev REV [COMMAND] - inspect revision REV\n"
     "       %s REPOS_PATH txn TXN [COMMAND] - inspect transaction TXN\n"
     "       %s REPOS_PATH [COMMAND] - inspect the youngest revision\n"
     "\n"
     "REV is a revision number > 0.\n"
     "TXN is a transaction name.\n"
     "\n"
     "If no command is given, the default output (which is the same as\n"
     "running the subcommands `info' then `tree') will be printed.\n"
     "\n"
     "COMMAND can be one of: \n"
     "\n"
     "   author:        print author.\n"
     "   changed:       print full change summary: all dirs & files changed.\n"
     "   date:          print the timestamp (revisions only).\n"
     "   diff:          print GNU-style diffs of changed files and props.\n"
     "   dirs-changed:  print changed directories.\n"
     "   ids:           print the tree, with nodes ids.\n"
     "   info:          print the author, data, log_size, and log message.\n"
     "   log:           print log message.\n"
     "   tree:          print the tree.\n"
     "\n"
     % (sys.argv[0], sys.argv[0], sys.argv[0]))

  sys.exit(exit)

def main():
  if len(sys.argv) < 2:
    usage(1)

  rev = txn = None

  args = sys.argv[2:]
  if args:
    cmd = args[0]
    if cmd == 'rev':
      if len(args) == 1:
        usage(1)
      try:
        rev = int(args[1])
      except ValueError:
        usage(1)
      del args[:2]
    elif cmd == 'txn':
      if len(args) == 1:
        usage(1)
      txn = args[1]
      del args[:2]

  if args:
    if len(args) > 1:
      usage(1)
    cmd = args[0].replace('-', '_')
  else:
    cmd = 'default'

  if not hasattr(SVNLook, 'cmd_' + cmd):
    usage(1)

  SVNLook(sys.argv[1], cmd, rev, txn)

if __name__ == '__main__':
  main()

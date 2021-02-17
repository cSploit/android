#!/usr/bin/env python
#  -*- coding: utf-8 -*-
#
# fsfs-reshard.py REPOS_PATH MAX_FILES_PER_SHARD
#
# Perform an offline conversion of an FSFS repository between linear (format
# 2, usable by Subversion 1.4+) and sharded (format 3, usable by Subversion
# 1.5+) layouts.
#
# The MAX_FILES_PER_SHARD argument specifies the maximum number of files
# that will be stored in each shard (directory), or zero to specify a linear
# layout.  Subversion 1.5 uses a default value of 1000 files per shard.
#
# As the repository will not be valid while the conversion is in progress,
# the repository administrator must ensure that access to the repository is
# blocked for the duration of the conversion.
#
# In the event that the conversion is interrupted, the repository will be in
# an inconsistent state.  The repository administrator should then re-run
# this tool to completion.
#
#
# Note that, currently, resharding from one sharded layout to another is
# likely to be an extremely slow process.  To reshard, we convert from a
# sharded to linear layout and then to the new sharded layout.  The problem
# is that the initial conversion to the linear layout triggers exactly the
# same 'large number of files in a directory' problem that sharding is
# intended to solve.
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
# ====================================================================
#
# $HeadURL: http://svn.apache.org/repos/asf/subversion/branches/1.7.x/tools/server-side/fsfs-reshard.py $
# $LastChangedDate: 2009-11-16 19:07:17 +0000 (Mon, 16 Nov 2009) $
# $LastChangedBy: hwright $
# $LastChangedRevision: 880911 $

import os, stat, sys

from errno import EEXIST

def usage():
  """Print a usage message and exit."""
  print("""usage: %s REPOS_PATH MAX_FILES_PER_SHARD [START END]

Perform an offline conversion of an FSFS repository between linear
(readable by Subversion 1.4 or later) and sharded (readable by
Subversion 1.5 or later) layouts.

The MAX_FILES_PER_SHARD argument specifies the maximum number of
files that will be stored in each shard (directory), or zero to
specify a linear layout.  Subversion 1.5 uses a default value of
1000 files per shard.

Convert revisions START through END inclusive if specified, or all
revisions if unspecified.
""" % sys.argv[0])
  sys.exit(1)

def incompatible_repos_format(repos_path, format):
  """Print an error saying that REPOS_PATH is a repository with an
  incompatible repository format FORMAT, then exit."""
  sys.stderr.write("""error: unable to convert repository '%s'.

This repository is not compatible with this tool.  Valid
repository formats are '3' or '5'; this repository is
format '%s'.

""" % (repos_path, format))
  sys.stderr.flush()
  sys.exit(1)

def incompatible_fs_format(repos_path, format):
  """Print an error saying that REPOS_PATH is a repository with an
  incompatible filesystem format FORMAT, then exit."""
  sys.stderr.write("""error: unable to convert repository '%s'.

This repository contains a filesystem that is not compatible with
this tool.  Valid filesystem formats are '1', '2', or '3'; this
repository contains a filesystem with format '%s'.

""" % (repos_path, format))
  sys.stderr.flush()
  sys.exit(1)

def unexpected_fs_format_options(repos_path):
  """Print an error saying that REPOS_PATH is a repository with
  unexpected filesystem format options, then exit."""
  sys.stderr.write("""error: unable to convert repository '%s'.

This repository contains a filesystem that appears to be invalid -
there is unexpected data after the filesystem format number.

""" % repos_path)
  sys.stderr.flush()
  sys.exit(1)

def incompatible_fs_format_option(repos_path, option):
  """Print an error saying that REPOS_PATH is a repository with an
  incompatible filesystem format option OPTION, then exit."""
  sys.stderr.write("""error: unable to convert repository '%s'.

This repository contains a filesystem that is not compatible with
this tool.  This tool recognises the 'layout' option but the
filesystem uses the '%s' option.

""" % (repos_path, option))
  sys.stderr.flush()
  sys.exit(1)

def warn_about_fs_format_1(repos_path, format_path):
  """Print a warning saying that REPOS_PATH contains a format 1 FSFS
  filesystem that we can't reconstruct, then exit."""
  sys.stderr.write("""warning: conversion of '%s' will be one-way.

This repository is currently readable by Subversion 1.1 or later.
This tool can convert this repository to one that is readable by
either Subversion 1.4 (or later) or Subversion 1.5 (or later),
but it is not able to convert it back to the original format - a
separate dump/load step would be required.

If you would like to upgrade this repository anyway, delete the
file '%s' and re-run this tool.

""" % (repos_path, format_path))
  sys.stderr.flush()
  sys.exit(1)

def check_repos_format(repos_path):
  """Check that REPOS_PATH contains a repository with a suitable format;
  print a message and exit if not."""
  format_path = os.path.join(repos_path, 'format')
  try:
    format_file = open(format_path)
    format = format_file.readline()
    if not format.endswith('\n'):
      incompatible_repos_format(repos_path, format + ' <missing newline>')
    format = format.rstrip('\n')
    if format == '3' or format == '5':
      pass
    else:
      incompatible_repos_format(repos_path, format)
  except IOError:
    # In all likelihood, the file doesn't exist.
    incompatible_repos_format(repos_path, '<unreadable>')

def check_fs_format(repos_path):
  """Check that REPOS_PATH contains a filesystem with a suitable format,
  or that it contains no format file; print a message and exit if neither
  is true.  Return bool whether the filesystem is sharded."""
  sharded = False
  db_path = os.path.join(repos_path, 'db')
  format_path = os.path.join(db_path, 'format')
  try:
    format_file = open(format_path)
    format = format_file.readline()
    if not format.endswith('\n'):
      incompatible_fs_format(repos_path, format + ' <missing newline>')
    format = format.rstrip('\n')
    if format == '1':
      # This is a format 1 (svndiff0 only) filesystem.  We can upgrade it,
      # but we can't downgrade again (since we can't uncompress any of the
      # svndiff1 deltas that may have been written).  Warn the user and exit.
      warn_about_fs_format_1(repos_path, format_path)
    if format == '2':
      pass
    elif format == '3':
      pass
    else:
      incompatible_fs_format(repos_path, format)

    for line in format_file:
      if format == '2':
        unexpected_fs_format_options(repos_path)

      line = line.rstrip('\n')
      if line == 'layout linear':
        pass
      elif line.startswith('layout sharded '):
        sharded = True
      else:
        incompatible_fs_format_option(repos_path, line)

    format_file.close()
  except IOError:
    # The format file might not exist if we've previously been interrupted,
    # or if the user is following our advice about upgrading a format 1
    # repository.  In both cases, we'll just assume the format was
    # compatible.
    pass

  return sharded

def current_file(repos_path):
  """Return triple of (revision, next_node_id, next_copy_id) from
  REPOS_PATH/db/current ."""
  return open(os.path.join(repos_path, 'db', 'current')).readline().split()

def remove_fs_format(repos_path):
  """Remove the filesystem format file for repository REPOS_PATH.
  Do not raise an error if the file is already missing."""
  format_path = os.path.join(repos_path, 'db', 'format')
  try:
    statinfo = os.stat(format_path)
  except OSError:
    # The file probably doesn't exist.
    return

  # On Windows, we need to ensure the file is writable before we can
  # remove it.
  os.chmod(format_path, statinfo.st_mode | stat.S_IWUSR)
  os.remove(format_path)

def write_fs_format(repos_path, contents):
  """Write a new filesystem format file for repository REPOS_PATH containing
  CONTENTS."""
  format_path = os.path.join(repos_path, 'db', 'format')
  f = open(format_path, 'wb')
  f.write(contents)
  f.close()
  os.chmod(format_path, stat.S_IRUSR | stat.S_IRGRP)

def linearise(path):
  """Move all the files in subdirectories of PATH into PATH, and remove the
  subdirectories.  Handle conflicts between subdirectory names and files
  contained in subdirectories by ensuring subdirectories have a '.shard'
  suffix prior to moving (the files are assumed not to have this suffix.
  Abort if a subdirectory is found to contain another subdirectory."""
  # First enumerate all subdirectories of DIR and rename where necessary
  # to include a .shard suffix.
  for name in os.listdir(path):
    if name.endswith('.shard'):
      continue
    subdir_path = os.path.join(path, name)
    if not os.path.isdir(subdir_path):
      continue
    os.rename(subdir_path, subdir_path + '.shard')

  # Now move all the subdirectory contents into the parent and remove
  # the subdirectories.
  for root_path, dirnames, filenames in os.walk(path):
    if root_path == path:
      continue
    if len(dirnames) > 0:
      sys.stderr.write("error: directory '%s' contains other unexpected directories.\n" \
        % root_path)
      sys.stderr.flush()
      sys.exit(1)
    for name in filenames:
      from_path = os.path.join(root_path, name)
      to_path = os.path.join(path, name)
      os.rename(from_path, to_path)
    os.rmdir(root_path)

def shard(path, max_files_per_shard, start, end):
  """Move the files for revisions START to END inclusive in PATH into
  subdirectories of PATH named such that subdirectory '0' contains at most
  MAX_FILES_PER_SHARD files, those named [0, MAX_FILES_PER_SHARD).  Abort if
  PATH is found to contain any entries with non-numeric names."""

  tmp = path + '.reshard'
  try:
    os.mkdir(tmp)
  except OSError, e:
    if e.errno != EEXIST:
      raise

  # Move all entries into shards named N.shard.
  for rev in range(start, end + 1):
    name = str(rev)
    shard = rev // max_files_per_shard
    shard_name = str(shard) + '.shard'

    from_path = os.path.join(path, name)
    to_path = os.path.join(tmp, shard_name, name)
    try:
      os.rename(from_path, to_path)
    except OSError:
      # The most likely explanation is that the shard directory doesn't
      # exist.  Let's create it and retry the rename.
      os.mkdir(os.path.join(tmp, shard_name))
      os.rename(from_path, to_path)

  # Now rename all the shards to remove the suffix.
  skipped = 0
  for name in os.listdir(tmp):
    if not name.endswith('.shard'):
      sys.stderr.write("warning: ignoring unexpected subdirectory '%s'.\n" \
        % os.path.join(tmp, name))
      sys.stderr.flush()
      skipped += 1
      continue
    from_path = os.path.join(tmp, name)
    to_path = os.path.join(path, os.path.basename(from_path)[:-6])
    os.rename(from_path, to_path)
  skipped == 0 and os.rmdir(tmp)

def main():
  if len(sys.argv) < 3:
    usage()

  repos_path = sys.argv[1]
  max_files_per_shard = sys.argv[2]
  try:
    start = int(sys.argv[3])
    end = int(sys.argv[4])
  except IndexError:
    start = 0
    end = int(current_file(repos_path)[0])

  # Validate the command-line arguments.
  db_path = os.path.join(repos_path, 'db')
  current_path = os.path.join(db_path, 'current')
  if not os.path.exists(current_path):
    sys.stderr.write("error: '%s' doesn't appear to be a Subversion FSFS repository.\n" \
      % repos_path)
    sys.stderr.flush()
    sys.exit(1)

  try:
    max_files_per_shard = int(max_files_per_shard)
  except ValueError, OverflowError:
    sys.stderr.write("error: maximum files per shard ('%s') is not a valid number.\n" \
      % max_files_per_shard)
    sys.stderr.flush()
    sys.exit(1)

  if max_files_per_shard < 0:
    sys.stderr.write("error: maximum files per shard ('%d') must not be negative.\n" \
      % max_files_per_shard)
    sys.stderr.flush()
    sys.exit(1)

  # Check the format of the repository.
  check_repos_format(repos_path)
  sharded = check_fs_format(repos_path)

  # Let the user know what's going on.
  if max_files_per_shard > 0:
    print("Converting '%s' to a sharded structure with %d files per directory" \
      % (repos_path, max_files_per_shard))
    if sharded:
      print('(will convert to a linear structure first)')
  else:
    print("Converting '%s' to a linear structure" % repos_path)

  # Prevent access to the repository for the duration of the conversion.
  # There's no clean way to do this, but since the format of the repository
  # is indeterminate, let's remove the format file while we're converting.
  print('- marking the repository as invalid')
  remove_fs_format(repos_path)

  # First, convert to a linear scheme (this makes recovery easier because
  # it's easier to reason about the behaviour on restart).
  if sharded:
    print('- linearising db/revs')
    linearise(os.path.join(repos_path, 'db', 'revs'))
    print('- linearising db/revprops')
    linearise(os.path.join(repos_path, 'db', 'revprops'))

  if max_files_per_shard == 0:
    # We're done.  Stamp the filesystem with a format 2 db/format file.
    print('- marking the repository as a valid linear repository')
    write_fs_format(repos_path, '2\n')
  else:
    print('- sharding db/revs')
    shard(os.path.join(repos_path, 'db', 'revs'), max_files_per_shard,
          start, end)
    print('- sharding db/revprops')
    shard(os.path.join(repos_path, 'db', 'revprops'), max_files_per_shard,
          start, end)

    # We're done.  Stamp the filesystem with a format 3 db/format file.
    print('- marking the repository as a valid sharded repository')
    write_fs_format(repos_path, '3\nlayout sharded %d\n' % max_files_per_shard)

  print('- done.')
  sys.exit(0)

if __name__ == '__main__':
  raise Exception("""This script is unfinished and not ready to be used on live data.
    Trust us.""")
  main()

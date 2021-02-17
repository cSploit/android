#!/usr/bin/env python

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

"""This program converts a Subversion WC from 1.7-dev format 18 to
   1.7-dev format 19 by migrating data from multiple DBs to a single DB.

   Usage: bump-to-19.py WC_ROOT_DIR
   where WC_ROOT_DIR is the path to the WC root directory.

   Skips non-WC dirs and WC dirs that are not at format 18."""

# TODO: Detect '_svn' as an alternative to '.svn'.

# TODO: Probably should remove any directory that is in state to-be-deleted
#       and doesn't have its 'keep_local' flag set.  Otherwise it will
#       become unversioned after commit, whereas format-18 and earlier would
#       have deleted it after commit.  Before deleting we should check there
#       are no unversioned things inside, and maybe even check for "local
#       mods" even though that's logically impossible.  On the other hand
#       it's not a big deal for the user to clean these up manually.


import sys, os, shutil, sqlite3

dot_svn = '.svn'

def dotsvn_path(wc_path):
  return os.path.join(wc_path, dot_svn)

def db_path(wc_path):
  return os.path.join(wc_path, dot_svn, 'wc.db')

def pristine_path(wc_path):
  return os.path.join(wc_path, dot_svn, 'pristine')

def tmp_path(wc_path):
  return os.path.join(wc_path, dot_svn, 'tmp')

class NotASubversionWC(Exception):
  def __init__(self, wc_path):
    self.wc_path = wc_path
  def __str__(self):
    return "not a Subversion WC: '" + self.wc_path +  "'"

class WrongFormatException(Exception):
  def __init__(self, wc_dir, format):
    self.wc_dir = wc_dir
    self.format = format
  def __str__(self):
    return "format is " + str(self.format) + " not 18: '" + self.wc_dir + "'"



STMT_COPY_BASE_NODE_TABLE_TO_WCROOT_DB1 = \
  "INSERT OR REPLACE INTO root.BASE_NODE ( " \
  "    wc_id, local_relpath, repos_id, repos_relpath, parent_relpath, " \
  "    presence, kind, revnum, checksum, translated_size, changed_rev, " \
  "    changed_date, changed_author, depth, symlink_target, last_mod_time, " \
  "    properties, dav_cache, incomplete_children, file_external ) " \
  "SELECT wc_id, ?1, repos_id, repos_relpath, ?2 AS parent_relpath, " \
  "    presence, kind, revnum, checksum, translated_size, changed_rev, " \
  "    changed_date, changed_author, depth, symlink_target, last_mod_time, " \
  "    properties, dav_cache, incomplete_children, file_external " \
  "FROM BASE_NODE WHERE local_relpath = ''; "

STMT_COPY_BASE_NODE_TABLE_TO_WCROOT_DB2 = \
  "INSERT INTO root.BASE_NODE ( " \
  "    wc_id, local_relpath, repos_id, repos_relpath, parent_relpath, " \
  "    presence, kind, revnum, checksum, translated_size, changed_rev, " \
  "    changed_date, changed_author, depth, symlink_target, last_mod_time, " \
  "    properties, dav_cache, incomplete_children, file_external ) " \
  "SELECT wc_id, ?1 || '/' || local_relpath, repos_id, repos_relpath, " \
  "    ?1 AS parent_relpath, " \
  "    presence, kind, revnum, checksum, translated_size, changed_rev, " \
  "    changed_date, changed_author, depth, symlink_target, last_mod_time, " \
  "    properties, dav_cache, incomplete_children, file_external " \
  "FROM BASE_NODE WHERE local_relpath != ''; "

STMT_COPY_WORKING_NODE_TABLE_TO_WCROOT_DB1 = \
  "INSERT OR REPLACE INTO root.WORKING_NODE ( " \
  "    wc_id, local_relpath, parent_relpath, presence, kind, checksum, " \
  "    translated_size, changed_rev, changed_date, changed_author, depth, " \
  "    symlink_target, copyfrom_repos_id, copyfrom_repos_path, copyfrom_revnum, " \
  "    moved_here, moved_to, last_mod_time, properties, keep_local ) " \
  "SELECT wc_id, ?1, ?2 AS parent_relpath, " \
  "    presence, kind, checksum, " \
  "    translated_size, changed_rev, changed_date, changed_author, depth, " \
  "    symlink_target, copyfrom_repos_id, copyfrom_repos_path, copyfrom_revnum, " \
  "    moved_here, moved_to, last_mod_time, properties, keep_local " \
  "FROM WORKING_NODE WHERE local_relpath = ''; "

STMT_COPY_WORKING_NODE_TABLE_TO_WCROOT_DB2 = \
  "INSERT INTO root.WORKING_NODE ( " \
  "    wc_id, local_relpath, parent_relpath, presence, kind, checksum, " \
  "    translated_size, changed_rev, changed_date, changed_author, depth, " \
  "    symlink_target, copyfrom_repos_id, copyfrom_repos_path, copyfrom_revnum, " \
  "    moved_here, moved_to, last_mod_time, properties, keep_local ) " \
  "SELECT wc_id, ?1 || '/' || local_relpath, ?1 AS parent_relpath, " \
  "    presence, kind, checksum, " \
  "    translated_size, changed_rev, changed_date, changed_author, depth, " \
  "    symlink_target, copyfrom_repos_id, copyfrom_repos_path, copyfrom_revnum, " \
  "    moved_here, moved_to, last_mod_time, properties, keep_local " \
  "FROM WORKING_NODE WHERE local_relpath != ''; "

STMT_COPY_ACTUAL_NODE_TABLE_TO_WCROOT_DB1 = \
  "INSERT OR REPLACE INTO root.ACTUAL_NODE ( " \
  "    wc_id, local_relpath, parent_relpath, properties, " \
  "    conflict_old, conflict_new, conflict_working, " \
  "    prop_reject, changelist, text_mod, tree_conflict_data, " \
  "    conflict_data, older_checksum, left_checksum, right_checksum ) " \
  "SELECT wc_id, ?1, ?2 AS parent_relpath, properties, " \
  "    conflict_old, conflict_new, conflict_working, " \
  "    prop_reject, changelist, text_mod, tree_conflict_data, " \
  "    conflict_data, older_checksum, left_checksum, right_checksum " \
  "FROM ACTUAL_NODE WHERE local_relpath = ''; "

STMT_COPY_ACTUAL_NODE_TABLE_TO_WCROOT_DB2 = \
  "INSERT INTO root.ACTUAL_NODE ( " \
  "    wc_id, local_relpath, parent_relpath, properties, " \
  "    conflict_old, conflict_new, conflict_working, " \
  "    prop_reject, changelist, text_mod, tree_conflict_data, " \
  "    conflict_data, older_checksum, left_checksum, right_checksum ) " \
  "SELECT wc_id, ?1 || '/' || local_relpath, ?1 AS parent_relpath, properties, " \
  "    conflict_old, conflict_new, conflict_working, " \
  "    prop_reject, changelist, text_mod, tree_conflict_data, " \
  "    conflict_data, older_checksum, left_checksum, right_checksum " \
  "FROM ACTUAL_NODE WHERE local_relpath != ''; "

STMT_COPY_LOCK_TABLE_TO_WCROOT_DB = \
  "INSERT INTO root.LOCK " \
  "SELECT * FROM LOCK; "

STMT_COPY_PRISTINE_TABLE_TO_WCROOT_DB = \
  "INSERT OR REPLACE INTO root.PRISTINE " \
  "SELECT * FROM PRISTINE; "

STMT_SELECT_SUBDIR = \
  "SELECT 1 FROM BASE_NODE WHERE local_relpath=?1 AND kind='subdir'" \
  "UNION " \
  "SELECT 0 FROM WORKING_NODE WHERE local_relpath=?1 AND kind='subdir';"

def copy_db_rows_to_wcroot(wc_subdir_relpath):
  """Copy all relevant table rows from the $PWD/WC_SUBDIR_RELPATH/.svn/wc.db
     into $PWD/.svn/wc.db."""

  wc_root_path = ''
  wc_subdir_path = wc_subdir_relpath
  wc_subdir_parent_relpath = os.path.dirname(wc_subdir_relpath)

  try:
    db = sqlite3.connect(db_path(wc_subdir_path))
  except:
    raise NotASubversionWC(wc_subdir_path)
  c = db.cursor()

  c.execute("ATTACH '" + db_path(wc_root_path) + "' AS 'root'")

  ### TODO: the REPOSITORY table. At present we assume there is only one
  # repository in use and its repos_id is consistent throughout the WC.
  # That's not always true - e.g. "svn switch --relocate" creates repos_id
  # 2, and then "svn mkdir" uses repos_id 1 in the subdirectory. */

  c.execute(STMT_COPY_BASE_NODE_TABLE_TO_WCROOT_DB1,
            (wc_subdir_relpath, wc_subdir_parent_relpath))
  c.execute(STMT_COPY_BASE_NODE_TABLE_TO_WCROOT_DB2,
            (wc_subdir_relpath, ))
  c.execute(STMT_COPY_WORKING_NODE_TABLE_TO_WCROOT_DB1,
            (wc_subdir_relpath, wc_subdir_parent_relpath))
  c.execute(STMT_COPY_WORKING_NODE_TABLE_TO_WCROOT_DB2,
            (wc_subdir_relpath, ))
  c.execute(STMT_COPY_ACTUAL_NODE_TABLE_TO_WCROOT_DB1,
            (wc_subdir_relpath, wc_subdir_parent_relpath))
  c.execute(STMT_COPY_ACTUAL_NODE_TABLE_TO_WCROOT_DB2,
            (wc_subdir_relpath, ))
  c.execute(STMT_COPY_LOCK_TABLE_TO_WCROOT_DB)
  c.execute(STMT_COPY_PRISTINE_TABLE_TO_WCROOT_DB)

  db.commit()
  db.close()


def move_and_shard_pristine_files(old_wc_path, new_wc_path):
  """Move all pristine text files from 'OLD_WC_PATH/.svn/pristine/'
     into 'NEW_WC_PATH/.svn/pristine/??/', creating shard dirs where
     necessary."""

  old_pristine_dir = pristine_path(old_wc_path)
  new_pristine_dir = pristine_path(new_wc_path)

  if not os.path.exists(old_pristine_dir):
    # That's fine, assuming there are no pristine texts.
    return

  for basename in os.listdir(old_pristine_dir):
    shard = basename[:2]
    if shard == basename: # already converted
      continue
    old = os.path.join(old_pristine_dir, basename)
    new = os.path.join(new_pristine_dir, shard, basename)
    os.renames(old, new)

def select_subdir(wc_subdir_path):
  """ Return True if wc_subdir_path is a known to be a versioned subdir,
      False otherwise."""

  try:
    db = sqlite3.connect(db_path(''))
  except:
    raise NotASubversionWC(wc_subdir_path)
  c = db.cursor()
  c.execute(STMT_SELECT_SUBDIR, (wc_subdir_path,))
  if c.fetchone() is None:
    return False
  else:
    return True


def migrate_wc_subdirs(wc_root_path):
  """Move Subversion metadata from the admin dir of each subdirectory
     below WC_ROOT_PATH into WC_ROOT_PATH's own admin dir."""

  old_cwd = os.getcwd()
  os.chdir(wc_root_path)

  # Keep track of which dirs we've migrated so we can delete their .svn's
  # afterwards.  Done this way because the tree walking is top-down and if
  # we deleted the .svn before walking into the subdir, it would look like
  # an unversioned subdir.
  migrated_subdirs = []

  # For each directory in the WC, try to migrate each of its subdirs (DIRS).
  # Done this way because (a) os.walk() gives us lists of subdirs, and (b)
  # it's easy to skip the WC root dir.
  for dir_path, dirs, files in os.walk('.'):

    # don't walk into the '.svn' subdirectory
    try:
      dirs.remove(dot_svn)
    except ValueError:
      # a non-WC dir: don't walk into any subdirectories
      print "skipped: ", NotASubversionWC(dir_path)
      del dirs[:]
      continue

    # Try to migrate each other subdirectory
    for dir in dirs[:]:  # copy so we can remove some
      wc_subdir_path = os.path.join(dir_path, dir)
      if wc_subdir_path.startswith('./'):
        wc_subdir_path = wc_subdir_path[2:]

      if not select_subdir(wc_subdir_path):
        print "skipped:", wc_subdir_path
        dirs.remove(dir)
        continue

      try:
        check_wc_format_number(wc_subdir_path)
        print "migrating '" + wc_subdir_path + "'"
        copy_db_rows_to_wcroot(wc_subdir_path)
        move_and_shard_pristine_files(wc_subdir_path, '.')
        migrated_subdirs += [wc_subdir_path]
      except (WrongFormatException, NotASubversionWC), e:
        print "skipped:", e
        # don't walk into it
        dirs.remove(dir)
        continue

  # Delete the remaining parts of the migrated .svn dirs
  # Make a note of any problems in deleting.
  failed_delete_subdirs = []
  for wc_subdir_path in migrated_subdirs:
    print "deleting " + dotsvn_path(wc_subdir_path)
    try:
      os.remove(db_path(wc_subdir_path))
      if os.path.exists(pristine_path(wc_subdir_path)):
        os.rmdir(pristine_path(wc_subdir_path))
      shutil.rmtree(tmp_path(wc_subdir_path))
      os.rmdir(dotsvn_path(wc_subdir_path))
    except Exception, e:
      print e
      failed_delete_subdirs += [wc_subdir_path]

  # Notify any problems in deleting
  if failed_delete_subdirs:
    print "Failed to delete the following directories. Please delete them manually."
    for wc_subdir_path in failed_delete_subdirs:
      print "  " + dotsvn_path(wc_subdir_path)

  os.chdir(old_cwd)


def check_wc_format_number(wc_path):
  """Check that the WC format of the WC dir WC_PATH is 18.
      Raise a WrongFormatException if not."""

  try:
    db = sqlite3.connect(db_path(wc_path))
  except sqlite3.OperationalError:
    raise NotASubversionWC(wc_path)
  c = db.cursor()
  c.execute("PRAGMA user_version;")
  format = c.fetchone()[0]
  db.commit()
  db.close()

  if format != 18:
    raise WrongFormatException(wc_path, format)


def bump_wc_format_number(wc_path):
  """Bump the WC format number of the WC dir WC_PATH to 19."""

  try:
    db = sqlite3.connect(db_path(wc_path))
  except sqlite3.OperationalError:
    raise NotASubversionWC(wc_path)
  c = db.cursor()
  c.execute("PRAGMA user_version = 19;")
  db.commit()
  db.close()


if __name__ == '__main__':

  if len(sys.argv) != 2:
    print __doc__
    sys.exit(1)

  wc_root_path = sys.argv[1]

  try:
    check_wc_format_number(wc_root_path)
  except (WrongFormatException, NotASubversionWC), e:
    print "error:", e
    sys.exit(1)

  print "merging subdir DBs into single DB '" + wc_root_path + "'"
  move_and_shard_pristine_files(wc_root_path, wc_root_path)
  migrate_wc_subdirs(wc_root_path)
  bump_wc_format_number(wc_root_path)


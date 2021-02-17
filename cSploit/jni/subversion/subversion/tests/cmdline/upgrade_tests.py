#!/usr/bin/env python
#
#  upgrade_tests.py:  test the working copy upgrade process
#
#  Subversion is a tool for revision control.
#  See http://subversion.apache.org for more information.
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
######################################################################

#
# These tests exercise the upgrade capabilities of 'svn upgrade' as it
# moves working copies between wc-1 and wc-ng.
#

import os
import re
import shutil
import sys
import tarfile
import tempfile

import svntest
from svntest import wc

Item = svntest.wc.StateItem
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco

wc_is_too_old_regex = (".*Working copy '.*' is too old \(format \d+.*\).*")


def get_current_format():
  # Get current format from subversion/libsvn_wc/wc.h
  format_file = open(os.path.join(os.path.dirname(__file__), "..", "..", "libsvn_wc", "wc.h")).read()
  return int(re.search("\n#define SVN_WC__VERSION (\d+)\n", format_file).group(1))


def replace_sbox_with_tarfile(sbox, tar_filename,
                              dir=None):
  try:
    svntest.main.safe_rmtree(sbox.wc_dir)
  except OSError, e:
    pass

  if not dir:
    dir = tar_filename.split('.')[0]

  tarpath = os.path.join(os.path.dirname(sys.argv[0]), 'upgrade_tests_data',
                         tar_filename)
  t = tarfile.open(tarpath, 'r:bz2')
  extract_dir = tempfile.mkdtemp(dir=svntest.main.temp_dir)
  for member in t.getmembers():
    t.extract(member, extract_dir)

  shutil.move(os.path.join(extract_dir, dir), sbox.wc_dir)

def replace_sbox_repo_with_tarfile(sbox, tar_filename, dir=None):
  try:
    svntest.main.safe_rmtree(sbox.repo_dir)
  except OSError, e:
    pass

  if not dir:
    dir = tar_filename.split('.')[0]
    
  tarpath = os.path.join(os.path.dirname(sys.argv[0]), 'upgrade_tests_data',
                         tar_filename)
  t = tarfile.open(tarpath, 'r:bz2')
  extract_dir = tempfile.mkdtemp(dir=svntest.main.temp_dir)
  for member in t.getmembers():
    t.extract(member, extract_dir)

  shutil.move(os.path.join(extract_dir, dir), sbox.repo_dir)

def check_format(sbox, expected_format):
  dot_svn = svntest.main.get_admin_name()
  for root, dirs, files in os.walk(sbox.wc_dir):
    db = svntest.sqlite3.connect(os.path.join(root, dot_svn, 'wc.db'))
    c = db.cursor()
    c.execute('pragma user_version;')
    found_format = c.fetchone()[0]
    db.close()

    if found_format != expected_format:
      raise svntest.Failure("found format '%d'; expected '%d'; in wc '%s'" %
                            (found_format, expected_format, root))

    if svntest.main.wc_is_singledb(sbox.wc_dir):
      dirs[:] = []

    if dot_svn in dirs:
      dirs.remove(dot_svn)

def check_pristine(sbox, files):
  for file in files:
    file_path = sbox.ospath(file)
    file_text = open(file_path, 'r').read()
    file_pristine = open(svntest.wc.text_base_path(file_path), 'r').read()
    if (file_text != file_pristine):
      raise svntest.Failure("pristine mismatch for '%s'" % (file))

def check_dav_cache(dir_path, wc_id, expected_dav_caches):
  dot_svn = svntest.main.get_admin_name()
  db = svntest.sqlite3.connect(os.path.join(dir_path, dot_svn, 'wc.db'))

  c = db.cursor()

  # Check if python's sqlite can read our db
  c.execute('select sqlite_version()')
  sqlite_ver = map(int, c.fetchone()[0].split('.'))

  # SQLite versions have 3 or 4 number groups
  major = sqlite_ver[0]
  minor = sqlite_ver[1]
  patch = sqlite_ver[2]

  if major < 3 or (major == 3 and minor < 6) \
     or (major == 3 and minor == 6 and patch < 18):
       return # We need a newer SQLite

  for local_relpath, expected_dav_cache in expected_dav_caches.items():
    # NODES conversion is complete enough that we can use it if it exists
    c.execute("""pragma table_info(nodes)""")
    if c.fetchone():
      c.execute('select dav_cache from nodes ' +
                'where wc_id=? and local_relpath=? and op_depth = 0',
                (wc_id, local_relpath))
      row = c.fetchone()
    else:
      c.execute('select dav_cache from base_node ' +
                'where wc_id=? and local_relpath=?',
                (wc_id, local_relpath))
      row = c.fetchone()
    if row is None:
      raise svntest.Failure("no dav cache for '%s'" % (local_relpath))
    dav_cache = str(row[0])
    if dav_cache != expected_dav_cache:
      raise svntest.Failure(
              "wrong dav cache for '%s'\n  Found:    '%s'\n  Expected: '%s'" %
                (local_relpath, dav_cache, expected_dav_cache))

  db.close()

# Very simple working copy property diff handler for single line textual properties
# Should probably be moved to svntest/actions.py after some major refactoring.
def simple_property_verify(dir_path, expected_props):

  # Shows all items in dict1 that are not also in dict2
  def diff_props(dict1, dict2, name, match):

    equal = True;
    for key in dict1:
      node = dict1[key]
      node2 = dict2.get(key, None)
      if node2:
        for prop in node:
          v1 = node[prop]
          v2 = node2.get(prop, None)

          if not v2:
            print('\'%s\' property on \'%s\' not found in %s' %
                  (prop, key, name))
            equal = False
          if match and v1 != v2:
            print('Expected \'%s\' on \'%s\' to be \'%s\', but found \'%s\'' %
                  (prop, key, v1, v2))
            equal = False
      else:
        print('\'%s\': %s not found in %s' % (key, dict1[key], name))
        equal = False

    return equal


  exit_code, output, errput = svntest.main.run_svn(None, 'proplist', '-R',
                                                   '-v', dir_path)

  actual_props = {}
  target = None
  name = None

  for i in output:
    if i.startswith('Properties on '):
      target = i[15+len(dir_path)+1:-3].replace(os.path.sep, '/')
    elif not i.startswith('    '):
      name = i.strip()
    else:
      v = actual_props.get(target, {})
      v[name] = i.strip()
      actual_props[target] = v

  v1 = diff_props(expected_props, actual_props, 'actual', True)
  v2 = diff_props(actual_props, expected_props, 'expected', False)

  if not v1 or not v2:
    print('Actual properties: %s' % actual_props)
    raise svntest.Failure("Properties unequal")

def simple_checksum_verify(expected_checksums):

  for path, checksum in expected_checksums:
    exit_code, output, errput = svntest.main.run_svn(None, 'info', path)
    if exit_code:
      raise svntest.Failure()
    if checksum:
      if not svntest.verify.RegexOutput('Checksum: ' + checksum,
                                        match_all=False).matches(output):
        raise svntest.Failure("did not get expected checksum " + checksum)
    if not checksum:
      if svntest.verify.RegexOutput('Checksum: ',
                                    match_all=False).matches(output):
        raise svntest.Failure("unexpected checksum")


def run_and_verify_status_no_server(wc_dir, expected_status):
  "same as svntest.actions.run_and_verify_status(), but without '-u'"

  exit_code, output, errput = svntest.main.run_svn(None, 'st', '-q', '-v',
                                                   wc_dir)
  actual = svntest.tree.build_tree_from_status(output)
  try:
    svntest.tree.compare_trees("status", actual, expected_status.old_tree())
  except svntest.tree.SVNTreeError:
    svntest.verify.display_trees(None, 'STATUS OUTPUT TREE',
                                 expected_status.old_tree(), actual)
    print("ACTUAL STATUS TREE:")
    svntest.tree.dump_tree_script(actual, wc_dir + os.sep)
    raise


def basic_upgrade(sbox):
  "basic upgrade behavior"

  replace_sbox_with_tarfile(sbox, 'basic_upgrade.tar.bz2')

  # Attempt to use the working copy, this should give an error
  expected_stderr = wc_is_too_old_regex
  svntest.actions.run_and_verify_svn(None, None, expected_stderr,
                                     'info', sbox.wc_dir)


  # Upgrade on something not a versioned dir gives a 'not directory' error.
  not_dir = ".*E155019.*%s'.*directory"
  os.mkdir(sbox.ospath('X'))
  svntest.actions.run_and_verify_svn(None, None, not_dir % 'X',
                                     'upgrade', sbox.ospath('X'))

  svntest.actions.run_and_verify_svn(None, None, not_dir % 'Y',
                                     'upgrade', sbox.ospath('Y'))

  svntest.actions.run_and_verify_svn(None, None, not_dir %
                                        re.escape(sbox.ospath('A/mu')),
                                     'upgrade', sbox.ospath('A/mu'))

  # Upgrade on a versioned subdir gives a 'not root' error.
  not_root = ".*E155019.*%s'.*root.*%s'"
  svntest.actions.run_and_verify_svn(None, None, not_root %
                                        ('A', re.escape(sbox.wc_dir)),
                                     'upgrade', sbox.ospath('A'))

  # Now upgrade the working copy
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'upgrade', sbox.wc_dir)

  # Actually check the format number of the upgraded working copy
  check_format(sbox, get_current_format())

  # Now check the contents of the working copy
  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)
  run_and_verify_status_no_server(sbox.wc_dir, expected_status)
  check_pristine(sbox, ['iota', 'A/mu'])

def upgrade_with_externals(sbox):
  "upgrade with externals"

  # Create wc from tarfile, uses the same structure of the wc as the tests
  # in externals_tests.py.
  replace_sbox_with_tarfile(sbox, 'upgrade_with_externals.tar.bz2')

  # Attempt to use the working copy, this should give an error
  expected_stderr = wc_is_too_old_regex
  svntest.actions.run_and_verify_svn(None, None, expected_stderr,
                                     'info', sbox.wc_dir)
  # Now upgrade the working copy
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'upgrade', sbox.wc_dir)

  # Actually check the format number of the upgraded working copy
  check_format(sbox, get_current_format())
  check_pristine(sbox, ['iota', 'A/mu',
                        'A/D/x/lambda', 'A/D/x/E/alpha'])

def upgrade_1_5_body(sbox, subcommand):
  replace_sbox_with_tarfile(sbox, 'upgrade_1_5.tar.bz2')

  # Attempt to use the working copy, this should give an error
  expected_stderr = wc_is_too_old_regex
  svntest.actions.run_and_verify_svn(None, None, expected_stderr,
                                     subcommand, sbox.wc_dir)


  # Now upgrade the working copy
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'upgrade', sbox.wc_dir)

  # Check the format of the working copy
  check_format(sbox, get_current_format())

  # Now check the contents of the working copy
  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)
  run_and_verify_status_no_server(sbox.wc_dir, expected_status)
  check_pristine(sbox, ['iota', 'A/mu'])


def upgrade_1_5(sbox):
  "test upgrading from a 1.5-era working copy"
  return upgrade_1_5_body(sbox, 'info')


def update_1_5(sbox):
  "test updating a 1.5-era working copy"

  # The 'update' printed:
  #    Skipped 'svn-test-work\working_copies\upgrade_tests-3'
  #    Summary of conflicts:
  #      Skipped paths: 1
  return upgrade_1_5_body(sbox, 'update')


def logs_left_1_5(sbox):
  "test upgrading from a 1.5-era wc with stale logs"

  replace_sbox_with_tarfile(sbox, 'logs_left_1_5.tar.bz2')

  # Try to upgrade, this should give an error
  expected_stderr = (".*Cannot upgrade with existing logs; .*")
  svntest.actions.run_and_verify_svn(None, None, expected_stderr,
                                     'upgrade', sbox.wc_dir)


def upgrade_wcprops(sbox):
  "test upgrading a working copy with wcprops"

  replace_sbox_with_tarfile(sbox, 'upgrade_wcprops.tar.bz2')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'upgrade', sbox.wc_dir)

  # Make sure that .svn/all-wcprops has disappeared
  dot_svn = svntest.main.get_admin_name()
  if os.path.exists(os.path.join(sbox.wc_dir, dot_svn, 'all-wcprops')):
    raise svntest.Failure("all-wcprops file still exists")

  # Just for kicks, let's see if the wcprops are what we'd expect them
  # to be.  (This could be smarter.)
  expected_dav_caches = {
   '' :
    '(svn:wc:ra_dav:version-url 41 /svn-test-work/local_tmp/repos/!svn/ver/1)',
   'iota' :
    '(svn:wc:ra_dav:version-url 46 /svn-test-work/local_tmp/repos/!svn/ver/1/iota)',
  }
  check_dav_cache(sbox.wc_dir, 1, expected_dav_caches)

# Poor mans relocate to fix up an 1.0 (xml style) working copy to refer to a
# valid repository, so svn upgrade can do its work on it
def xml_entries_relocate(path, from_url, to_url):
  adm_name = svntest.main.get_admin_name()
  entries = os.path.join(path, adm_name, 'entries')
  txt = open(entries).read().replace('url="' + from_url, 'url="' + to_url)
  os.chmod(entries, 0777)
  open(entries, 'w').write(txt)

  for dirent in os.listdir(path):
    item_path = os.path.join(path, dirent)

    if dirent == svntest.main.get_admin_name():
      continue

    if os.path.isdir(os.path.join(item_path, adm_name)):
      xml_entries_relocate(item_path, from_url, to_url)

# Poor mans relocate to fix up an working copy to refer to a
# valid repository, so svn upgrade can do its work on it
def simple_entries_replace(path, from_url, to_url):
  adm_name = svntest.main.get_admin_name()
  entries = os.path.join(path, adm_name, 'entries')
  txt = open(entries).read().replace(from_url, to_url)
  os.chmod(entries, 0777)
  open(entries, 'wb').write(txt)

  for dirent in os.listdir(path):
    item_path = os.path.join(path, dirent)

    if dirent == svntest.main.get_admin_name():
      continue

    if os.path.isdir(os.path.join(item_path, adm_name)):
      simple_entries_replace(item_path, from_url, to_url)


def basic_upgrade_1_0(sbox):
  "test upgrading a working copy created with 1.0.0"

  sbox.build(create_wc = False)
  replace_sbox_with_tarfile(sbox, 'upgrade_1_0.tar.bz2')

  url = sbox.repo_url

  xml_entries_relocate(sbox.wc_dir, 'file:///1.0.0/repos', url)

  # Attempt to use the working copy, this should give an error
  expected_stderr = wc_is_too_old_regex
  svntest.actions.run_and_verify_svn(None, None, expected_stderr,
                                     'info', sbox.wc_dir)


  # Now upgrade the working copy
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'upgrade', sbox.wc_dir)
  # And the separate working copy below COPIED or check_format() fails
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'upgrade',
                                     os.path.join(sbox.wc_dir, 'COPIED', 'G'))

  # Actually check the format number of the upgraded working copy
  check_format(sbox, get_current_format())

  # Now check the contents of the working copy
  # #### This working copy is not just a basic tree,
  #      fix with the right data once we get here
  expected_status = svntest.wc.State(sbox.wc_dir,
    {
      '' : Item(status='  ', wc_rev=7),
      'B'                 : Item(status='  ', wc_rev='7'),
      'B/mu'              : Item(status='  ', wc_rev='7'),
      'B/D'               : Item(status='  ', wc_rev='7'),
      'B/D/H'             : Item(status='  ', wc_rev='7'),
      'B/D/H/psi'         : Item(status='  ', wc_rev='7'),
      'B/D/H/omega'       : Item(status='  ', wc_rev='7'),
      'B/D/H/zeta'        : Item(status='MM', wc_rev='7'),
      'B/D/H/chi'         : Item(status='  ', wc_rev='7'),
      'B/D/gamma'         : Item(status='  ', wc_rev='9'),
      'B/D/G'             : Item(status='  ', wc_rev='7'),
      'B/D/G/tau'         : Item(status='  ', wc_rev='7'),
      'B/D/G/rho'         : Item(status='  ', wc_rev='7'),
      'B/D/G/pi'          : Item(status='  ', wc_rev='7'),
      'B/B'               : Item(status='  ', wc_rev='7'),
      'B/B/lambda'        : Item(status='  ', wc_rev='7'),
      'MKDIR'             : Item(status='A ', wc_rev='0'),
      'MKDIR/MKDIR'       : Item(status='A ', wc_rev='0'),
      'A'                 : Item(status='  ', wc_rev='7'),
      'A/B'               : Item(status='  ', wc_rev='7'),
      'A/B/lambda'        : Item(status='  ', wc_rev='7'),
      'A/D'               : Item(status='  ', wc_rev='7'),
      'A/D/G'             : Item(status='  ', wc_rev='7'),
      'A/D/G/rho'         : Item(status='  ', wc_rev='7'),
      'A/D/G/pi'          : Item(status='  ', wc_rev='7'),
      'A/D/G/tau'         : Item(status='  ', wc_rev='7'),
      'A/D/H'             : Item(status='  ', wc_rev='7'),
      'A/D/H/psi'         : Item(status='  ', wc_rev='7'),
      'A/D/H/omega'       : Item(status='  ', wc_rev='7'),
      'A/D/H/zeta'        : Item(status='  ', wc_rev='7'),
      'A/D/H/chi'         : Item(status='  ', wc_rev='7'),
      'A/D/gamma'         : Item(status='  ', wc_rev='7'),
      'A/mu'              : Item(status='  ', wc_rev='7'),
      'iota'              : Item(status='  ', wc_rev='7'),
      'COPIED'            : Item(status='  ', wc_rev='10'),
      'DELETED'           : Item(status='D ', wc_rev='10'),
     })
  run_and_verify_status_no_server(sbox.wc_dir, expected_status)

  expected_infos = [ {
      'Node Kind': 'directory',
      'Schedule': 'normal',
      'Revision': '7',
      'Last Changed Author' : 'Bert',
      'Last Changed Rev' : '7'
    } ]
  svntest.actions.run_and_verify_info(expected_infos, sbox.wc_dir)

  expected_infos = [ {
      'Node Kind': 'directory',
      'Schedule': 'delete',
      'Revision': '10',
      'Last Changed Author' : 'Bert',
      'Last Changed Rev' : '10'
    } ]
  svntest.actions.run_and_verify_info(expected_infos,
                                      os.path.join(sbox.wc_dir, 'DELETED'))

  check_pristine(sbox, ['iota', 'A/mu', 'A/D/H/zeta'])

# Helper function for the x3 tests.
def do_x3_upgrade(sbox, expected_error=[]):
  # Attempt to use the working copy, this should give an error
  expected_stderr = wc_is_too_old_regex
  svntest.actions.run_and_verify_svn(None, None, expected_stderr,
                                     'info', sbox.wc_dir)


  # Now upgrade the working copy
  svntest.actions.run_and_verify_svn(None, None, expected_error,
                                     'upgrade', sbox.wc_dir)

  if expected_error != []:
    return

  # Actually check the format number of the upgraded working copy
  check_format(sbox, get_current_format())

  # Now check the contents of the working copy
  expected_status = svntest.wc.State(sbox.wc_dir,
    {
      ''                  : Item(status='  ', wc_rev='2'),
      'A'                 : Item(status='  ', wc_rev='2'),
      'A/D'               : Item(status='  ', wc_rev='2'),
      'A/D/H'             : Item(status='  ', wc_rev='2'),
      'A/D/H/omega'       : Item(status='  ', wc_rev='2'),
      'A/D/H/psi'         : Item(status='D ', wc_rev='2'),
      'A/D/H/new'         : Item(status='A ', copied='+', wc_rev='-'),
      'A/D/H/chi'         : Item(status='R ', copied='+', wc_rev='-'),
      'A/D/gamma'         : Item(status='D ', wc_rev='2'),
      'A/D/G'             : Item(status='  ', wc_rev='2'),
      'A/B_new'           : Item(status='A ', copied='+', wc_rev='-'),
      'A/B_new/B'         : Item(status='A ', copied='+', wc_rev='-'),
      'A/B_new/B/E'       : Item(status=' M', copied='+', wc_rev='-'),
      'A/B_new/B/E/alpha' : Item(status='  ', copied='+', wc_rev='-'),
      'A/B_new/B/E/beta'  : Item(status='R ', copied='+', wc_rev='-'),
      'A/B_new/B/new'     : Item(status='A ', copied='+', wc_rev='-'),
      'A/B_new/B/lambda'  : Item(status='R ', copied='+', wc_rev='-'),
      'A/B_new/B/F'       : Item(status='  ', copied='+', wc_rev='-'),
      'A/B_new/E'         : Item(status=' M', copied='+', wc_rev='-'),
      'A/B_new/E/alpha'   : Item(status=' M', copied='+', wc_rev='-'),
      'A/B_new/E/beta'    : Item(status='RM', copied='+', wc_rev='-'),
      'A/B_new/lambda'    : Item(status='R ', copied='+', wc_rev='-'),
      'A/B_new/new'       : Item(status='A ', copied='+', wc_rev='-'),
      'A/B_new/F'         : Item(status='  ', copied='+', wc_rev='-'),
      'A/B'               : Item(status='  ', wc_rev='2'),
      'A/B/E'             : Item(status='  ', wc_rev='2'),
      'A/B/E/beta'        : Item(status='RM', copied='+', wc_rev='-'),
      'A/B/E/alpha'       : Item(status=' M', wc_rev='2'),
      'A/B/F'             : Item(status='  ', wc_rev='2'),
      'A/B/lambda'        : Item(status='R ', copied='+', wc_rev='-'),
      'A/B/new'           : Item(status='A ', copied='+', wc_rev='-'),
      'A/G_new'           : Item(status='A ', copied='+', wc_rev='-'),
      'A/G_new/rho'       : Item(status='R ', copied='+', wc_rev='-'),
      'iota'              : Item(status='  ', wc_rev='2'),
      'A_new'             : Item(status='A ', wc_rev='0'),
      'A_new/alpha'       : Item(status='A ', copied='+', wc_rev='-'),
    })
  run_and_verify_status_no_server(sbox.wc_dir, expected_status)

  simple_property_verify(sbox.wc_dir, {
      'A/B_new/E/beta'    : {'x3'           : '3x',
                             'svn:eol-style': 'native'},
      'A/B/E/beta'        : {'s'            : 't',
                             'svn:eol-style': 'native'},
      'A/B_new/B/E/alpha' : {'svn:eol-style': 'native'},
      'A/B/E/alpha'       : {'q': 'r',
                             'svn:eol-style': 'native'},
      'A_new/alpha'       : {'svn:eol-style': 'native'},
      'A/B_new/B/new'     : {'svn:eol-style': 'native'},
      'A/B_new/E/alpha'   : {'svn:eol-style': 'native',
                             'u': 'v'},
      'A/B_new/B/E'       : {'q': 'r'},
      'A/B_new/lambda'    : {'svn:eol-style': 'native'},
      'A/B_new/E'         : {'x3': '3x'},
      'A/B_new/new'       : {'svn:eol-style': 'native'},
      'A/B/lambda'        : {'svn:eol-style': 'native'},
      'A/B_new/B/E/beta'  : {'svn:eol-style': 'native'},
      'A/B_new/B/lambda'  : {'svn:eol-style': 'native'},
      'A/B/new'           : {'svn:eol-style': 'native'},
      'A/G_new/rho'       : {'svn:eol-style': 'native'}
  })

  svntest.actions.run_and_verify_svn(None, 'Reverted.*', [],
                                     'revert', '-R', sbox.wc_dir)

  expected_status = svntest.wc.State(sbox.wc_dir,
    {
      ''                  : Item(status='  ', wc_rev='2'),
      'A'                 : Item(status='  ', wc_rev='2'),
      'A/D'               : Item(status='  ', wc_rev='2'),
      'A/D/H'             : Item(status='  ', wc_rev='2'),
      'A/D/H/omega'       : Item(status='  ', wc_rev='2'),
      'A/D/H/psi'         : Item(status='  ', wc_rev='2'),
      'A/D/H/chi'         : Item(status='  ', wc_rev='2'),
      'A/D/gamma'         : Item(status='  ', wc_rev='2'),
      'A/D/G'             : Item(status='  ', wc_rev='2'),
      'A/B'               : Item(status='  ', wc_rev='2'),
      'A/B/F'             : Item(status='  ', wc_rev='2'),
      'A/B/E'             : Item(status='  ', wc_rev='2'),
      'A/B/E/beta'        : Item(status='  ', wc_rev='2'),
      'A/B/E/alpha'       : Item(status='  ', wc_rev='2'),
      'A/B/lambda'        : Item(status='  ', wc_rev='2'),
      'iota'              : Item(status='  ', wc_rev='2'),
    })
  run_and_verify_status_no_server(sbox.wc_dir, expected_status)

  simple_property_verify(sbox.wc_dir, {
      'A/B/E/beta'        : {'svn:eol-style': 'native'},
#      'A/B/lambda'        : {'svn:eol-style': 'native'},
      'A/B/E/alpha'       : {'svn:eol-style': 'native'}
  })

@Issue(2530)
def x3_1_4_0(sbox):
  "3x same wc upgrade 1.4.0 test"

  replace_sbox_with_tarfile(sbox, 'wc-3x-1.4.0.tar.bz2', dir='wc-1.4.0')

  do_x3_upgrade(sbox, expected_error='.*E155016: The properties of.*are in an '
                'indeterminate state and cannot be upgraded. See issue #2530.')

@Issue(3811)
def x3_1_4_6(sbox):
  "3x same wc upgrade 1.4.6 test"

  replace_sbox_with_tarfile(sbox, 'wc-3x-1.4.6.tar.bz2', dir='wc-1.4.6')

  do_x3_upgrade(sbox)

@Issue(3811)
def x3_1_6_12(sbox):
  "3x same wc upgrade 1.6.12 test"

  replace_sbox_with_tarfile(sbox, 'wc-3x-1.6.12.tar.bz2', dir='wc-1.6.12')

  do_x3_upgrade(sbox)

def missing_dirs(sbox):
  "missing directories and obstructing files"

  # tarball wc looks like:
  #   svn co URL wc
  #   svn cp wc/A/B wc/A/B_new
  #   rm -rf wc/A/B/E wc/A/D wc/A/B_new/E wc/A/B_new/F
  #   touch wc/A/D wc/A/B_new/F

  replace_sbox_with_tarfile(sbox, 'missing-dirs.tar.bz2')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'upgrade', sbox.wc_dir)
  expected_status = svntest.wc.State(sbox.wc_dir,
    {
      ''                  : Item(status='  ', wc_rev='1'),
      'A'                 : Item(status='  ', wc_rev='1'),
      'A/mu'              : Item(status='  ', wc_rev='1'),
      'A/C'               : Item(status='  ', wc_rev='1'),
      'A/D'               : Item(status='! ', wc_rev='1'),
      'A/B'               : Item(status='  ', wc_rev='1'),
      'A/B/F'             : Item(status='  ', wc_rev='1'),
      'A/B/E'             : Item(status='! ', wc_rev='1'),
      'A/B/lambda'        : Item(status='  ', wc_rev='1'),
      'iota'              : Item(status='  ', wc_rev='1'),
      'A/B_new'           : Item(status='A ', wc_rev='-', copied='+'),
      'A/B_new/E'         : Item(status='! ', wc_rev='-'),
      'A/B_new/F'         : Item(status='! ', wc_rev='-'),
      'A/B_new/lambda'    : Item(status='  ', wc_rev='-', copied='+'),
    })
  run_and_verify_status_no_server(sbox.wc_dir, expected_status)

def missing_dirs2(sbox):
  "missing directories and obstructing dirs"

  replace_sbox_with_tarfile(sbox, 'missing-dirs.tar.bz2')
  os.remove(sbox.ospath('A/D'))
  os.remove(sbox.ospath('A/B_new/F'))
  os.mkdir(sbox.ospath('A/D'))
  os.mkdir(sbox.ospath('A/B_new/F'))
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'upgrade', sbox.wc_dir)
  expected_status = svntest.wc.State(sbox.wc_dir,
    {
      ''                  : Item(status='  ', wc_rev='1'),
      'A'                 : Item(status='  ', wc_rev='1'),
      'A/mu'              : Item(status='  ', wc_rev='1'),
      'A/C'               : Item(status='  ', wc_rev='1'),
      'A/D'               : Item(status='! ', wc_rev='1'),
      'A/B'               : Item(status='  ', wc_rev='1'),
      'A/B/F'             : Item(status='  ', wc_rev='1'),
      'A/B/E'             : Item(status='! ', wc_rev='1'),
      'A/B/lambda'        : Item(status='  ', wc_rev='1'),
      'iota'              : Item(status='  ', wc_rev='1'),
      'A/B_new'           : Item(status='A ', wc_rev='-', copied='+'),
      'A/B_new/E'         : Item(status='! ', wc_rev='-'),
      'A/B_new/F'         : Item(status='! ', wc_rev='-'),
      'A/B_new/lambda'    : Item(status='  ', wc_rev='-', copied='+'),
    })
  run_and_verify_status_no_server(sbox.wc_dir, expected_status)

@Issue(3808)
def delete_and_keep_local(sbox):
  "check status delete and delete --keep-local"

  replace_sbox_with_tarfile(sbox, 'wc-delete.tar.bz2')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'upgrade', sbox.wc_dir)

  expected_status = svntest.wc.State(sbox.wc_dir,
    {
      ''                  : Item(status='  ', wc_rev='0'),
      'Normal'            : Item(status='  ', wc_rev='1'),
      'Deleted-Keep-Local': Item(status='D ', wc_rev='1'),
      'Deleted'           : Item(status='D ', wc_rev='1'),
  })

  run_and_verify_status_no_server(sbox.wc_dir, expected_status)

  # Deleted-Keep-Local should still exist after the upgrade
  if not os.path.exists(os.path.join(sbox.wc_dir, 'Deleted-Keep-Local')):
    raise svntest.Failure('wc/Deleted-Keep-Local should exist')

  # Deleted should be removed after the upgrade as it was
  # schedule delete and doesn't contain unversioned changes.
  if os.path.exists(os.path.join(sbox.wc_dir, 'Deleted')):
    raise svntest.Failure('wc/Deleted should not exist')


def dirs_only_upgrade(sbox):
  "upgrade a wc without files"

  replace_sbox_with_tarfile(sbox, 'dirs-only.tar.bz2')

  expected_output = ["Upgraded '%s'\n" % (sbox.ospath('').rstrip(os.path.sep)),
                     "Upgraded '%s'\n" % (sbox.ospath('A'))]

  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'upgrade', sbox.wc_dir)

  expected_status = svntest.wc.State(sbox.wc_dir, {
      ''                  : Item(status='  ', wc_rev='1'),
      'A'                 : Item(status='  ', wc_rev='1'),
      })
  run_and_verify_status_no_server(sbox.wc_dir, expected_status)

def read_tree_conflict_data(sbox, path):
  dot_svn = svntest.main.get_admin_name()
  db = svntest.sqlite3.connect(os.path.join(sbox.wc_dir, dot_svn, 'wc.db'))
  for row in db.execute("select tree_conflict_data from actual_node "
                        "where tree_conflict_data is not null "
                        "and local_relpath = '%s'" % path):
    return
  raise svntest.Failure("conflict expected for '%s'" % path)

def no_actual_node(sbox, path):
  dot_svn = svntest.main.get_admin_name()
  db = svntest.sqlite3.connect(os.path.join(sbox.wc_dir, dot_svn, 'wc.db'))
  for row in db.execute("select 1 from actual_node "
                        "where local_relpath = '%s'" % path):
    raise svntest.Failure("no actual node expected for '%s'" % path)

def upgrade_tree_conflict_data(sbox):
  "upgrade tree conflict data (f20->f21)"

  wc_dir = sbox.wc_dir
  replace_sbox_with_tarfile(sbox, 'upgrade_tc.tar.bz2')

  # Check and see if we can still read our tree conflicts
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.tweak('A/D/G/pi', status='D ', treeconflict='C')
  expected_status.tweak('A/D/G/tau', status='! ', treeconflict='C',
                        wc_rev=None)
  expected_status.tweak('A/D/G/rho', status='A ', copied='+',
                        treeconflict='C', wc_rev='-')

  # Look inside pre-upgrade database
  read_tree_conflict_data(sbox, 'A/D/G')
  no_actual_node(sbox, 'A/D/G/pi')
  no_actual_node(sbox, 'A/D/G/rho')
  no_actual_node(sbox, 'A/D/G/tau')

  # While the upgrade from f20 to f21 will work the upgrade from f22
  # to f23 will not, since working nodes are present, so the
  # auto-upgrade will fail.  If this happens we cannot use the
  # Subversion libraries to query the working copy.
  exit_code, output, errput = svntest.main.run_svn('format 22', 'st', wc_dir)

  if not exit_code:
    run_and_verify_status_no_server(wc_dir, expected_status)
  else:
    if not svntest.verify.RegexOutput('.*format 22 with WORKING nodes.*',
                                      match_all=False).matches(errput):
      raise svntest.Failure()

  # Look insde post-upgrade database
  read_tree_conflict_data(sbox, 'A/D/G/pi')
  read_tree_conflict_data(sbox, 'A/D/G/rho')
  read_tree_conflict_data(sbox, 'A/D/G/tau')
  # no_actual_node(sbox, 'A/D/G')  ### not removed but should be?


@Issue(3898)
def delete_in_copy_upgrade(sbox):
  "upgrade a delete within a copy"

  wc_dir = sbox.wc_dir
  replace_sbox_with_tarfile(sbox, 'delete-in-copy.tar.bz2')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'upgrade', sbox.wc_dir)

  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)
  expected_status.add({
      'A/B-copied'         : Item(status='A ', copied='+', wc_rev='-'),
      'A/B-copied/lambda'  : Item(status='  ', copied='+', wc_rev='-'),
      'A/B-copied/E'       : Item(status='D ', copied='+', wc_rev='-'),
      'A/B-copied/E/alpha' : Item(status='D ', copied='+', wc_rev='-'),
      'A/B-copied/E/beta'  : Item(status='D ', copied='+', wc_rev='-'),
      'A/B-copied/F'       : Item(status='  ', copied='+', wc_rev='-'),
      })
  run_and_verify_status_no_server(sbox.wc_dir, expected_status)

  svntest.actions.run_and_verify_svn(None, 'Reverted.*', [], 'revert', '-R',
                                     sbox.ospath('A/B-copied/E'))

  expected_status.tweak('A/B-copied/E',
                        'A/B-copied/E/alpha',
                        'A/B-copied/E/beta',
                        status='  ')
  run_and_verify_status_no_server(sbox.wc_dir, expected_status)

  simple_checksum_verify([[sbox.ospath('A/B-copied/E/alpha'),
                           'b347d1da69df9a6a70433ceeaa0d46c8483e8c03']])


def replaced_files(sbox):
  "upgrade with base and working replaced files"

  wc_dir = sbox.wc_dir
  replace_sbox_with_tarfile(sbox, 'replaced-files.tar.bz2')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'upgrade', sbox.wc_dir)

  # A is a checked-out dir containing A/f and A/g, then
  # svn cp wc/A wc/B
  # svn rm wc/A/f wc/B/f
  # svn cp wc/A/g wc/A/f     # A/f replaced by copied A/g
  # svn cp wc/A/g wc/B/f     # B/f replaced by copied A/g (working-only)
  # svn rm wc/A/g wc/B/g
  # touch wc/A/g wc/B/g
  # svn add wc/A/g wc/B/g    # A/g replaced, B/g replaced (working-only)
  # svn ps pX vX wc/A/g
  # svn ps pY vY wc/B/g
  expected_status = svntest.wc.State(sbox.wc_dir,
    {
      ''    : Item(status='  ', wc_rev='5'),
      'A'   : Item(status='  ', wc_rev='5'),
      'A/f' : Item(status='R ', wc_rev='-', copied='+'),
      'A/g' : Item(status='RM', wc_rev='5'),
      'B'   : Item(status='A ', wc_rev='-', copied='+'),
      'B/f' : Item(status='R ', wc_rev='-', copied='+'),
      'B/g' : Item(status='RM', wc_rev='-'),
  })
  run_and_verify_status_no_server(sbox.wc_dir, expected_status)

  simple_property_verify(sbox.wc_dir, {
      'A/f' : {'pAg' : 'vAg' },
      'A/g' : {'pX'  : 'vX' },
      'B/f' : {'pAg' : 'vAg' },
      'B/g' : {'pY'  : 'vY' },
      })

  simple_checksum_verify([
      [sbox.ospath('A/f'), '395dfb603d8a4e0348d0b082803f2b7426c76eb9'],
      [sbox.ospath('A/g'), None],
      [sbox.ospath('B/f'), '395dfb603d8a4e0348d0b082803f2b7426c76eb9'],
      [sbox.ospath('B/g'), None]])

  svntest.actions.run_and_verify_svn(None, 'Reverted.*', [], 'revert',
                                     sbox.ospath('A/f'), sbox.ospath('B/f'),
                                     sbox.ospath('A/g'), sbox.ospath('B/g'))

  simple_property_verify(sbox.wc_dir, {
      'A/f' : {'pAf' : 'vAf' },
      'A/g' : {'pAg' : 'vAg' },
      'B/f' : {'pAf' : 'vAf' },
      'B/g' : {'pAg' : 'vAg' },
      })

  simple_checksum_verify([
      [sbox.ospath('A/f'), '958eb2d755df2d9e0de6f7b835aec16b64d83f6f'],
      [sbox.ospath('A/g'), '395dfb603d8a4e0348d0b082803f2b7426c76eb9'],
      [sbox.ospath('B/f'), '958eb2d755df2d9e0de6f7b835aec16b64d83f6f'],
      [sbox.ospath('B/g'), '395dfb603d8a4e0348d0b082803f2b7426c76eb9']])

def upgrade_with_scheduled_change(sbox):
  "upgrade 1.6.x wc with a scheduled change"

  replace_sbox_with_tarfile(sbox, 'upgrade_with_scheduled_change.tar.bz2')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'upgrade', sbox.wc_dir)
  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)
  expected_status.add({
      'A/scheduled_file_1' : Item(status='A ', wc_rev='-'),
      })
  run_and_verify_status_no_server(sbox.wc_dir, expected_status)

@Issue(3777)
def tree_replace1(sbox):
  "upgrade 1.6 with tree replaced"

  replace_sbox_with_tarfile(sbox, 'tree-replace1.tar.bz2')

  svntest.actions.run_and_verify_svn(None, None, [], 'upgrade', sbox.wc_dir)

  expected_status = svntest.wc.State(sbox.wc_dir,
    {
      ''      : Item(status=' M', wc_rev=17),
      'B'     : Item(status='R ', copied='+', wc_rev='-'),
      'B/f'   : Item(status='R ', copied='+', wc_rev='-'),
      'B/g'   : Item(status='D ', wc_rev=17),
      'B/h'   : Item(status='A ', copied='+', wc_rev='-'),
      'B/C'   : Item(status='R ', copied='+', wc_rev='-'),
      'B/C/f' : Item(status='R ', copied='+', wc_rev='-'),
      'B/D'   : Item(status='D ', wc_rev=17),
      'B/D/f' : Item(status='D ', wc_rev=17),
      'B/E'   : Item(status='A ', copied='+', wc_rev='-'),
      'B/E/f' : Item(status='A ', copied='+', wc_rev='-'),
    })
  run_and_verify_status_no_server(sbox.wc_dir, expected_status)

@Issue(3777)
def tree_replace2(sbox):
  "upgrade 1.6 with tree replaced (2)"

  replace_sbox_with_tarfile(sbox, 'tree-replace2.tar.bz2')

  svntest.actions.run_and_verify_svn(None, None, [], 'upgrade', sbox.wc_dir)

  expected_status = svntest.wc.State(sbox.wc_dir,
    {
      ''      : Item(status=' M', wc_rev=12),
      'B'     : Item(status='R ', copied='+', wc_rev='-'),
      'B/f'   : Item(status='D ', wc_rev=12),
      'B/D'   : Item(status='D ', wc_rev=12),
      'B/g'   : Item(status='A ', copied='+', wc_rev='-'),
      'B/E'   : Item(status='A ', copied='+', wc_rev='-'),
      'C'     : Item(status='R ', copied='+', wc_rev='-'),
      'C/f'   : Item(status='A ', copied='+', wc_rev='-'),
      'C/D'   : Item(status='A ', copied='+', wc_rev='-'),
      'C/g'   : Item(status='D ', wc_rev=12),
      'C/E'   : Item(status='D ', wc_rev=12),
    })
  run_and_verify_status_no_server(sbox.wc_dir, expected_status)

def upgrade_from_format_28(sbox):
  """upgrade from format 28: rename pristines"""

  # Start with a format-28 WC that is a clean checkout of the Greek tree.
  replace_sbox_with_tarfile(sbox, 'format_28.tar.bz2')

  # Get the old and new pristine file paths for file 'iota'.
  checksum = '2c0aa9014a0cd07f01795a333d82485ef6d083e2'
  old_pristine_path = os.path.join(sbox.wc_dir, svntest.main.get_admin_name(),
                                   'pristine', checksum[0:2], checksum)
  new_pristine_path = old_pristine_path + '.svn-base'

  assert os.path.exists(old_pristine_path)
  assert not os.path.exists(new_pristine_path)

  # Touch the WC to auto-upgrade it
  svntest.actions.run_and_verify_svn(None, None, [], 'info', sbox.wc_dir)

  assert not os.path.exists(old_pristine_path)
  assert os.path.exists(new_pristine_path)

@Issue(3901)
def depth_exclude(sbox):
  "upgrade 1.6.x wc that has depth=exclude"

  replace_sbox_with_tarfile(sbox, 'depth_exclude.tar.bz2')

  svntest.actions.run_and_verify_svn(None, None, [], 'upgrade', sbox.wc_dir)

  expected_status = svntest.wc.State(sbox.wc_dir,
    {
      ''                  : Item(status='  ', wc_rev='1'),
      'A'                 : Item(status='  ', wc_rev='1'),
      'X'                 : Item(status='A ', copied='+', wc_rev='-'),
    })
  run_and_verify_status_no_server(sbox.wc_dir, expected_status)

@Issue(3901)
def depth_exclude_2(sbox):
  "1.6.x wc that has depth=exclude inside a delete"

  replace_sbox_with_tarfile(sbox, 'depth_exclude_2.tar.bz2')

  svntest.actions.run_and_verify_svn(None, None, [], 'upgrade', sbox.wc_dir)

  expected_status = svntest.wc.State(sbox.wc_dir,
    {
      ''                  : Item(status='  ', wc_rev='1'),
      'A'                 : Item(status='D ', wc_rev='1'),
    })
  run_and_verify_status_no_server(sbox.wc_dir, expected_status)

@Issue(3916)
def add_add_del_del_tc(sbox):
  "wc with add-add and del-del tree conflicts"

  replace_sbox_with_tarfile(sbox, 'add_add_del_del_tc.tar.bz2')

  svntest.actions.run_and_verify_svn(None, None, [], 'upgrade', sbox.wc_dir)

  expected_status = svntest.wc.State(sbox.wc_dir,
    {
      ''     : Item(status='  ', wc_rev='4'),
      'A'    : Item(status='  ', wc_rev='4'),
      'A/B'  : Item(status='A ', treeconflict='C', copied='+', wc_rev='-'),
      'X'    : Item(status='  ', wc_rev='3'),
      'X/Y'  : Item(status='! ', treeconflict='C')
    })
  run_and_verify_status_no_server(sbox.wc_dir, expected_status)

@Issue(3916)
def add_add_x2(sbox):
  "wc with 2 tree conflicts in same entry"

  replace_sbox_with_tarfile(sbox, 'add_add_x2.tar.bz2')

  svntest.actions.run_and_verify_svn(None, None, [], 'upgrade', sbox.wc_dir)

  expected_status = svntest.wc.State(sbox.wc_dir,
    {
      ''     : Item(status='  ', wc_rev='3'),
      'A'    : Item(status='  ', wc_rev='3'),
      'A/X'  : Item(status='A ', treeconflict='C', copied='+', wc_rev='-'),
      'A/Y'  : Item(status='A ', treeconflict='C', copied='+', wc_rev='-'),
    })
  run_and_verify_status_no_server(sbox.wc_dir, expected_status)

@Issue(3940)
def upgrade_with_missing_subdir(sbox):
  "test upgrading a working copy with missing subdir"

  sbox.build(create_wc = False)
  replace_sbox_with_tarfile(sbox, 'basic_upgrade.tar.bz2')

  simple_entries_replace(sbox.wc_dir,
                         'file:///Users/Hyrum/dev/test/greek-1.6.repo',
                         sbox.repo_url)

  svntest.main.run_svnadmin('setuuid', sbox.repo_dir,
                            'cafefeed-babe-face-dead-beeff00dfade')

  url = sbox.repo_url
  wc_dir = sbox.wc_dir

  # Attempt to use the working copy, this should give an error
  expected_stderr = wc_is_too_old_regex
  svntest.actions.run_and_verify_svn(None, None, expected_stderr,
                                     'info', sbox.wc_dir)

  # Now remove a subdirectory
  svntest.main.safe_rmtree(sbox.ospath('A/B'))

  # Now upgrade the working copy and expect a missing subdir
  expected_output = svntest.verify.UnorderedOutput([
    "Upgraded '%s'\n" % sbox.wc_dir,
    "Upgraded '%s'\n" % sbox.ospath('A'),
    "Skipped '%s'\n" % sbox.ospath('A/B'),
    "Upgraded '%s'\n" % sbox.ospath('A/C'),
    "Upgraded '%s'\n" % sbox.ospath('A/D'),
    "Upgraded '%s'\n" % sbox.ospath('A/D/G'),
    "Upgraded '%s'\n" % sbox.ospath('A/D/H'),
  ])
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'upgrade', sbox.wc_dir)

  # And now perform an update. (This used to fail with an assertion)
  expected_output = svntest.wc.State(wc_dir, {
    'A/B'               : Item(verb='Restored'),
    'A/B/E'             : Item(status='A '),
    'A/B/E/alpha'       : Item(status='A '),
    'A/B/E/beta'        : Item(status='A '),
    'A/B/lambda'        : Item(status='A '),
    'A/B/F'             : Item(status='A '),
  })

  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  # Do the update and check the results in three ways.
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

@Issue(3994)
def upgrade_locked(sbox):
  "upgrade working copy with locked files"

  replace_sbox_with_tarfile(sbox, 'upgrade_locked.tar.bz2')

  svntest.actions.run_and_verify_svn(None, None, [], 'upgrade', sbox.wc_dir)

  expected_status = svntest.wc.State(sbox.wc_dir,
    {
      ''                  : Item(status='  ', wc_rev=1),
      'A'                 : Item(status='D ', wc_rev=2),
      'A/third'           : Item(status='D ', writelocked='K', wc_rev=2),
      'other'             : Item(status='D ', writelocked='K', wc_rev=4),
      'iota'              : Item(status='  ', writelocked='K', wc_rev=3),
    })

  run_and_verify_status_no_server(sbox.wc_dir, expected_status)

@Issue(4015)
def upgrade_file_externals(sbox):
  "upgrade with file externals"

  sbox.build()
  replace_sbox_with_tarfile(sbox, 'upgrade_file_externals.tar.bz2')
  svntest.main.run_svnadmin('setuuid', sbox.repo_dir,
                            '07146bbd-0b64-4aaf-ab70-cd76a0df2d41')

  expected_output = svntest.verify.RegexOutput('r2 committed.*')
  svntest.actions.run_and_verify_svnmucc(None, expected_output, [],
                                         'propset', 'svn:externals',
                                         '^/A/B/E EX\n^/A/mu muX',
                                         sbox.repo_url + '/A/B/F')

  expected_output = svntest.verify.RegexOutput('r3 committed.*')
  svntest.actions.run_and_verify_svnmucc(None, expected_output, [],
                                         'propset', 'svn:externals',
                                         '^/A/B/F FX\n^/A/B/lambda lambdaX',
                                         sbox.repo_url + '/A/C')

  expected_output = svntest.verify.RegexOutput('r4 committed.*')
  svntest.actions.run_and_verify_svnmucc(None, expected_output, [],
                                         'propset', 'pname1', 'pvalue1',
                                         sbox.repo_url + '/A/mu',
                                         'propset', 'pname2', 'pvalue2',
                                         sbox.repo_url + '/A/B/lambda',
                                         'propset', 'pname3', 'pvalue3',
                                         sbox.repo_url + '/A/B/E/alpha')

  svntest.actions.run_and_verify_svn(None, None, [], 'upgrade', sbox.wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [], 'relocate',
                                     'file:///tmp/repo', sbox.repo_url,
                                     sbox.wc_dir)
  
  expected_output = svntest.wc.State(sbox.wc_dir, {
      'A/mu'            : Item(status=' U'),
      'A/B/lambda'      : Item(status=' U'),
      'A/B/E/alpha'     : Item(status=' U'),
      'A/C/FX/EX/alpha' : Item(status=' U'),
      'A/C/FX/muX'      : Item(status=' U'),
      'A/C/lambdaX'     : Item(status=' U'),
      'A/B/F/EX/alpha'  : Item(status=' U'),
      'A/B/F/muX'       : Item(status=' U'),
      })
  svntest.actions.run_and_verify_update(sbox.wc_dir, expected_output,
                                        None, None)

  ### simple_property_verify only sees last line of multi-line
  ### property values such as svn:externals
  simple_property_verify(sbox.wc_dir, {
      'A/mu'          : {'pname1' : 'pvalue1' },
      'A/B/lambda'    : {'pname2' : 'pvalue2' },
      'A/B/E/alpha'   : {'pname3' : 'pvalue3' },
      'A/B/F'         : {'svn:externals' : '^/A/mu muX'},
      'A/C'           : {'svn:externals' : '^/A/B/lambda lambdaX'},
      'A/B/F/muX'     : {'pname1' : 'pvalue1' },
      'A/C/lambdaX'   : {'pname2' : 'pvalue2' },
      })

  simple_property_verify(sbox.ospath('A/C/FX'), {
      ''    : {'svn:externals' : '^/A/mu muX'},
      'muX' : {'pname1' : 'pvalue1' },
      })

  simple_property_verify(sbox.ospath('A/C/FX/EX'), {
      'alpha' : {'pname3' : 'pvalue3' },
      })


@Issue(4035)
def upgrade_missing_replaced(sbox):
  "upgrade with missing replaced dir"

  sbox.build(create_wc=False)
  replace_sbox_with_tarfile(sbox, 'upgrade_missing_replaced.tar.bz2')

  svntest.actions.run_and_verify_svn(None, None, [], 'upgrade', sbox.wc_dir)
  svntest.main.run_svnadmin('setuuid', sbox.repo_dir,
                            'd7130b12-92f6-45c9-9217-b9f0472c3fab')
  svntest.actions.run_and_verify_svn(None, None, [], 'relocate',
                                     'file:///tmp/repo', sbox.repo_url,
                                     sbox.wc_dir)

  expected_output = svntest.wc.State(sbox.wc_dir, {
      'A/B/E'         : Item(status='  ', treeconflict='C'),
      'A/B/E/alpha'   : Item(status='  ', treeconflict='A'),
      'A/B/E/beta'    : Item(status='  ', treeconflict='A'),
      })
  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)
  expected_status.tweak('A/B/E', status='! ', treeconflict='C', wc_rev='-')
  expected_status.tweak('A/B/E/alpha', 'A/B/E/beta', status='D ')
  svntest.actions.run_and_verify_update(sbox.wc_dir, expected_output,
                                        None, expected_status)

  svntest.actions.run_and_verify_svn(None, 'Reverted.*', [], 'revert', '-R',
                                     sbox.wc_dir)
  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)
  svntest.actions.run_and_verify_status(sbox.wc_dir, expected_status)

@Issue(4033)
def upgrade_not_present_replaced(sbox):
  "upgrade with not-present replaced nodes"

  sbox.build(create_wc=False)
  replace_sbox_with_tarfile(sbox, 'upgrade_not_present_replaced.tar.bz2')

  svntest.actions.run_and_verify_svn(None, None, [], 'upgrade', sbox.wc_dir)
  svntest.main.run_svnadmin('setuuid', sbox.repo_dir,
                            'd7130b12-92f6-45c9-9217-b9f0472c3fab')
  svntest.actions.run_and_verify_svn(None, None, [], 'relocate',
                                     'file:///tmp/repo', sbox.repo_url,
                                     sbox.wc_dir)

  expected_output = svntest.wc.State(sbox.wc_dir, {
      'A/B/E'         : Item(status='E '),
      'A/B/E/alpha'   : Item(status='A '),
      'A/B/E/beta'    : Item(status='A '),
      'A/B/lambda'    : Item(status='E '),
      })
  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)
  svntest.actions.run_and_verify_update(sbox.wc_dir, expected_output,
                                        None, expected_status)

########################################################################
# Run the tests

  # prop states
  #
  # .base                      simple checkout
  # .base, .revert             delete, copy-here
  # .working                   add, propset
  # .base, .working            checkout, propset
  # .base, .revert, .working   delete, copy-here, propset
  # .revert, .working          delete, add, propset
  # .revert                    delete, add
  #
  # 1.3.x (f4)
  # 1.4.0 (f8, buggy)
  # 1.4.6 (f8, fixed)

# list all tests here, starting with None:
test_list = [ None,
              basic_upgrade,
              upgrade_with_externals,
              upgrade_1_5,
              update_1_5,
              logs_left_1_5,
              upgrade_wcprops,
              basic_upgrade_1_0,
              # Upgrading from 1.4.0-1.4.5 with specific states fails
              # See issue #2530
              x3_1_4_0,
              x3_1_4_6,
              x3_1_6_12,
              missing_dirs,
              missing_dirs2,
              delete_and_keep_local,
              dirs_only_upgrade,
              upgrade_tree_conflict_data,
              delete_in_copy_upgrade,
              replaced_files,
              upgrade_with_scheduled_change,
              tree_replace1,
              tree_replace2,
              upgrade_from_format_28,
              depth_exclude,
              depth_exclude_2,
              add_add_del_del_tc,
              add_add_x2,
              upgrade_with_missing_subdir,
              upgrade_locked,
              upgrade_file_externals,
              upgrade_missing_replaced,
              upgrade_not_present_replaced,
             ]


if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED

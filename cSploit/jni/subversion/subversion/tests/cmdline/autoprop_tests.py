#!/usr/bin/env python
#
#  autoprop_tests.py:  testing automatic properties
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

# General modules
import os

# Our testing module
import svntest


# (abbreviation)
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco
Item = svntest.wc.StateItem


# Helper function
def check_proplist(path, exp_out):
  """Verify that property list on PATH has a value of EXP_OUT"""

  props = svntest.tree.get_props([path]).get(path, {})
  if props != exp_out:
    print("Expected properties: %s" % exp_out)
    print("Actual properties:   %s" % props)
    raise svntest.Failure


######################################################################
# Tests

#----------------------------------------------------------------------

def create_config(config_dir, enable_flag):
  "create config directories and files"

  # contents of the file 'config'
  config_contents = '''\
[auth]
password-stores =

[miscellany]
enable-auto-props = %s

[auto-props]
*.c = cfile=yes
*.jpg = jpgfile=ja
fubar* = tarfile=si
foobar.lha = lhafile=da;lzhfile=niet
spacetest = abc = def ; ghi = ; = j
escapetest = myval=;;;;val;myprop=p
quotetest = svn:keywords="Author Date Id Rev URL";
* = auto=oui
''' % (enable_flag and 'yes' or 'no')

  svntest.main.create_config_dir(config_dir, config_contents)

#----------------------------------------------------------------------

def autoprops_test(sbox, cmd, cfgenable, clienable, subdir):
  """configurable autoprops test.

     CMD is the subcommand to test: 'import' or 'add'
     if CFGENABLE is true, enable autoprops in the config file, else disable
     if CLIENABLE == 1: --auto-props is added to the command line
                     0: nothing is added
                    -1: --no-auto-props is added to command line
     if string SUBDIR is not empty files are created in that subdir and the
       directory is added/imported"""

  # Bootstrap
  sbox.build()

  # some directories
  wc_dir = sbox.wc_dir
  tmp_dir = os.path.abspath(svntest.main.temp_dir)
  config_dir = os.path.join(tmp_dir, 'autoprops_config_' + sbox.name)
  repos_url = sbox.repo_url

  # initialize parameters
  if cmd == 'import':
    parameters = ['import', '-m', 'bla']
    files_dir = tmp_dir
  else:
    parameters = ['add']
    files_dir = wc_dir

  parameters = parameters + ['--config-dir', config_dir]

  create_config(config_dir, cfgenable)

  # add comandline flags
  if clienable == 1:
    parameters = parameters + ['--auto-props']
    enable_flag = 1
  elif clienable == -1:
    parameters = parameters + ['--no-auto-props']
    enable_flag = 0
  else:
    enable_flag = cfgenable

  # setup subdirectory if needed
  if len(subdir) > 0:
    files_dir = os.path.join(files_dir, subdir)
    files_wc_dir = os.path.join(wc_dir, subdir)
    os.makedirs(files_dir)
  else:
    files_wc_dir = wc_dir

  # create test files
  filenames = ['foo.h',
               'foo.c',
               'foo.jpg',
               'fubar.tar',
               'foobar.lha',
               'spacetest',
               'escapetest',
               'quotetest']
  for filename in filenames:
    svntest.main.file_write(os.path.join(files_dir, filename),
                            'foo\nbar\nbaz\n')

  if len(subdir) == 0:
    # add/import the files
    for filename in filenames:
      path = os.path.join(files_dir, filename)
      if cmd == 'import':
        tmp_params = parameters + [path, repos_url + '/' + filename]
      else:
        tmp_params = parameters + [path]
      svntest.main.run_svn(None, *tmp_params)
  else:
    # add/import subdirectory
    if cmd == 'import':
      parameters = parameters + [files_dir, repos_url]
    else:
      parameters = parameters + [files_wc_dir]
    svntest.main.run_svn(None, *parameters)

  # do an svn co if needed
  if cmd == 'import':
    svntest.main.run_svn(None, 'checkout', repos_url, files_wc_dir,
                        '--config-dir', config_dir)

  # check the properties
  if enable_flag:
    filename = os.path.join(files_wc_dir, 'foo.h')
    check_proplist(filename, {'auto':'oui'})
    filename = os.path.join(files_wc_dir, 'foo.c')
    check_proplist(filename, {'auto':'oui', 'cfile':'yes'})
    filename = os.path.join(files_wc_dir, 'foo.jpg')
    check_proplist(filename, {'auto':'oui', 'jpgfile':'ja'})
    filename = os.path.join(files_wc_dir, 'fubar.tar')
    check_proplist(filename, {'auto':'oui', 'tarfile':'si'})
    filename = os.path.join(files_wc_dir, 'foobar.lha')
    check_proplist(filename, {'auto':'oui', 'lhafile':'da', 'lzhfile':'niet'})
    filename = os.path.join(files_wc_dir, 'spacetest')
    check_proplist(filename, {'auto':'oui', 'abc':'def', 'ghi':''})
    filename = os.path.join(files_wc_dir, 'escapetest')
    check_proplist(filename, {'auto':'oui', 'myval':';;val', 'myprop':'p'})
    filename = os.path.join(files_wc_dir, 'quotetest')
    check_proplist(filename, {'auto':'oui',
                              'svn:keywords': 'Author Date Id Rev URL'})
  else:
    for filename in filenames:
      check_proplist(os.path.join(files_wc_dir, filename), {})


#----------------------------------------------------------------------

def autoprops_add_no_none(sbox):
  "add: config=no,  commandline=none"

  autoprops_test(sbox, 'add', 0, 0, '')

#----------------------------------------------------------------------

def autoprops_add_yes_none(sbox):
  "add: config=yes, commandline=none"

  autoprops_test(sbox, 'add', 1, 0, '')

#----------------------------------------------------------------------

def autoprops_add_no_yes(sbox):
  "add: config=no,  commandline=yes"

  autoprops_test(sbox, 'add', 0, 1, '')

#----------------------------------------------------------------------

def autoprops_add_yes_yes(sbox):
  "add: config=yes, commandline=yes"

  autoprops_test(sbox, 'add', 1, 1, '')

#----------------------------------------------------------------------

def autoprops_add_no_no(sbox):
  "add: config=no,  commandline=no"

  autoprops_test(sbox, 'add', 0, -1, '')

#----------------------------------------------------------------------

def autoprops_add_yes_no(sbox):
  "add: config=yes, commandline=no"

  autoprops_test(sbox, 'add', 1, -1, '')

#----------------------------------------------------------------------

def autoprops_imp_no_none(sbox):
  "import: config=no,  commandline=none"

  autoprops_test(sbox, 'import', 0, 0, '')

#----------------------------------------------------------------------

def autoprops_imp_yes_none(sbox):
  "import: config=yes, commandline=none"

  autoprops_test(sbox, 'import', 1, 0, '')

#----------------------------------------------------------------------

def autoprops_imp_no_yes(sbox):
  "import: config=no,  commandline=yes"

  autoprops_test(sbox, 'import', 0, 1, '')

#----------------------------------------------------------------------

def autoprops_imp_yes_yes(sbox):
  "import: config=yes, commandline=yes"

  autoprops_test(sbox, 'import', 1, 1, '')

#----------------------------------------------------------------------

def autoprops_imp_no_no(sbox):
  "import: config=no,  commandline=no"

  autoprops_test(sbox, 'import', 0, -1, '')

#----------------------------------------------------------------------

def autoprops_imp_yes_no(sbox):
  "import: config=yes, commandline=no"

  autoprops_test(sbox, 'import', 1, -1, '')

#----------------------------------------------------------------------

def autoprops_add_dir(sbox):
  "add directory"

  autoprops_test(sbox, 'add', 1, 0, 'autodir')

#----------------------------------------------------------------------

def autoprops_imp_dir(sbox):
  "import directory"

  autoprops_test(sbox, 'import', 1, 0, 'autodir')

#----------------------------------------------------------------------

# Issue #2713: adding a file with an svn:eol-style property, svn should abort
# if the file has mixed EOL style. Previously, svn aborted but had added the
# file anyway.
@Issue(2713)
def fail_add_mixed_eol_style(sbox):
  "fail to add a file with mixed EOL style"

  from svntest.actions import run_and_verify_svn, run_and_verify_unquiet_status

  # Bootstrap
  sbox.build()

  filename = 'mixed-eol.txt'
  filepath = os.path.join(sbox.wc_dir, filename)
  parameters = ['--auto-props',
                '--config-option=config:auto-props:' + filename
                + '=svn:eol-style=native']

  svntest.main.file_write(filepath, 'foo\nbar\r\nbaz\r')

  expected_stderr = "svn: E200009: File '.*" + filename + \
                    "' has inconsistent newlines" + \
                    "|" + "svn: E135000: Inconsistent line ending style\n"
  run_and_verify_svn(None, [], expected_stderr,
                     'add', filepath, *parameters)

  expected_status = svntest.wc.State(sbox.wc_dir,
    {filename : Item(status='? ')})
  run_and_verify_unquiet_status(filepath, expected_status)


########################################################################
# Run the tests


# list all tests here, starting with None:
test_list = [ None,
              autoprops_add_no_none,
              autoprops_add_yes_none,
              autoprops_add_no_yes,
              autoprops_add_yes_yes,
              autoprops_add_no_no,
              autoprops_add_yes_no,
              autoprops_imp_no_none,
              autoprops_imp_yes_none,
              autoprops_imp_no_yes,
              autoprops_imp_yes_yes,
              autoprops_imp_no_no,
              autoprops_imp_yes_no,
              autoprops_add_dir,
              autoprops_imp_dir,
              fail_add_mixed_eol_style,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.

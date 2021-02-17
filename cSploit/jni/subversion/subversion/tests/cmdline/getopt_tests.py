#!/usr/bin/env python
#
#  getopt_tests.py:  testing the svn command line processing
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
import sys, re, os.path

# Our testing module
import svntest


######################################################################
# Tests

#----------------------------------------------------------------------

# This directory contains all the expected output from svn.
getopt_output_dir = os.path.join(os.path.dirname(sys.argv[0]),
                                 'getopt_tests_data')

# Naming convention for golden files: take the svn command line as a
# single string and apply the following sed transformations:
#   echo svn option1 option2 ... | sed -e 's/ /_/g' -e 's/_--/--/g'
# Then append either _stdout or _stderr for the file descriptor to
# compare against.

def load_expected_output(basename):
  "load the expected standard output and standard error"

  stdout_filename = os.path.join(getopt_output_dir, basename + '_stdout')
  stderr_filename = os.path.join(getopt_output_dir, basename + '_stderr')

  exp_stdout = open(stdout_filename, 'r').readlines()
  exp_stderr = open(stderr_filename, 'r').readlines()

  return exp_stdout, exp_stderr

# This is a list of lines to delete.
del_lines_res = [
                 # In 'svn --version', the date line is variable, for example:
                 # "compiled Apr  5 2002, 10:08:45"
                 re.compile(r'\s+compiled\s+'),

                 # Also for 'svn --version':
                 re.compile(r"\* ra_(neon|local|svn|serf) :"),
                 re.compile(r"  - handles '(https?|file|svn)' scheme"),
                 re.compile(r"  - with Cyrus SASL authentication"),
                 re.compile(r"\* fs_(base|fs) :"),
                ]

# This is a list of lines to search and replace text on.
rep_lines_res = [
                 # In 'svn --version', this line varies, for example:
                 # "Subversion Client, version 0.10.2-dev (under development)"
                 # "Subversion Client, version 0.10.2 (r1729)"
                 (re.compile(r'version \d+\.\d+\.\d+(-[^ ]*)? \(.*\)'),
                  'version X.Y.Z '),
                 # The copyright end date keeps changing; fix forever.
                 (re.compile(r'Copyright \(C\) 20\d\d The Apache '
                              'Software Foundation\.'),
                  'Copyright (C) YYYY The Apache Software Foundation'),
                 # In 'svn --version --quiet', we print only the version
                 # number in a single line.
                 (re.compile(r'^\d+\.\d+\.\d+(-[a-zA-Z0-9]+)?$'), 'X.Y.Z\n'),
                 # 'svn --help' has a line with the version number.
                 # It can vary, for example:
                 # "Subversion command-line client, version 1.1.0."
                 # "Subversion command-line client, version 1.1.0-dev."
                 (re.compile(r'Subversion command-line client, '
                             'version \d+\.\d+\.\d+(.|-[a-zA-Z0-9]+\.)$'),
                  'Subversion command-line client, version X.Y.Z.'),
                ]

def process_lines(lines):
  "delete lines that should not be compared and search and replace the rest"
  output = [ ]
  for line in lines:
    # Skip these lines from the output list.
    delete_line = 0
    for delete_re in del_lines_res:
      if delete_re.match(line):
        delete_line = 1
        break
    if delete_line:
      continue

    # Search and replace text on the rest.
    for replace_re, replace_str in rep_lines_res:
      line = replace_re.sub(replace_str, line)

    output.append(line)

  return output

def run_one_test(sbox, basename, *varargs):
  "run svn with args and compare against the specified output files"

  ### no need to use sbox.build() -- we don't need a repos or working copy
  ### for these tests.

  exp_stdout, exp_stderr = load_expected_output(basename)

  # special case the 'svn' test so that no extra arguments are added
  if basename != 'svn':
    exit_code, actual_stdout, actual_stderr = svntest.main.run_svn(1, *varargs)
  else:
    exit_code, actual_stdout, actual_stderr = svntest.main.run_command(svntest.main.svn_binary,
                                                                       1, 0, *varargs)

  # Delete and perform search and replaces on the lines from the
  # actual and expected output that may differ between build
  # environments.
  exp_stdout    = process_lines(exp_stdout)
  exp_stderr    = process_lines(exp_stderr)
  actual_stdout = process_lines(actual_stdout)
  actual_stderr = process_lines(actual_stderr)

  if exp_stdout != actual_stdout:
    print("Standard output does not match.")
    print("Expected standard output:")
    print("=====")
    for x in exp_stdout:
      sys.stdout.write(x)
    print("=====")
    print("Actual standard output:")
    print("=====")
    for x in actual_stdout:
      sys.stdout.write(x)
    print("=====")
    raise svntest.Failure

  if exp_stderr != actual_stderr:
    print("Standard error does not match.")
    print("Expected standard error:")
    print("=====")
    for x in exp_stderr:
      sys.stdout.write(x)
    print("=====")
    print("Actual standard error:")
    print("=====")
    for x in actual_stderr:
      sys.stdout.write(x)
    print("=====")
    raise svntest.Failure

def getopt_no_args(sbox):
  "run svn with no arguments"
  run_one_test(sbox, 'svn')

def getopt__version(sbox):
  "run svn --version"
  run_one_test(sbox, 'svn--version', '--version')

def getopt__version__quiet(sbox):
  "run svn --version --quiet"
  run_one_test(sbox, 'svn--version--quiet', '--version', '--quiet')

def getopt__help(sbox):
  "run svn --help"
  run_one_test(sbox, 'svn--help', '--help')

def getopt_help(sbox):
  "run svn help"
  run_one_test(sbox, 'svn_help', 'help')

def getopt_help_log_switch(sbox):
  "run svn help log switch"
  run_one_test(sbox, 'svn_help_log_switch', 'help', 'log', 'switch')

def getopt_help_bogus_cmd(sbox):
  "run svn help bogus-cmd"
  run_one_test(sbox, 'svn_help_bogus-cmd', 'help', 'bogus-cmd')

########################################################################
# Run the tests


# list all tests here, starting with None:
test_list = [ None,
              getopt_no_args,
              getopt__version,
              getopt__version__quiet,
              getopt__help,
              getopt_help,
              getopt_help_bogus_cmd,
              getopt_help_log_switch,
            ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.

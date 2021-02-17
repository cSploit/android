#
#  factory.py:  Automatically generate a (near-)complete new cmdline test
#               from a series of shell commands.
#
#  Subversion is a tool for revision control.
#  See http://subversion.tigris.org for more information.
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


## HOW TO USE:
#
# (1) Edit the py test script you want to enhance (for example
#     cmdline/basic_tests.py), add a new test header as usual.
#     Insert a call to factory.make() into the empty test:
#
#  def my_new_test(sbox):
#    "my new test modifies iota"
#    svntest.factory.make(sbox, """
#                               echo "foo" > A/D/foo
#                               svn add A/D/foo
#                               svn st
#                               svn ci
#                               """)
#
# (2) Add the test to the tests list at the bottom of the py file.
# [...]
#              some_other_test,
#              my_new_test,
#             ]
#
#
# (3) Run the test, paste the output back into your new test,
#     replacing the factory call.
#
#  $ ./foo_tests.py my_new_test
#  OR
#  $ ./foo_tests.py my_new_test > new_test.snippet
#  OR
#  $ ./foo_tests.py my_new_test >> basic_tests.py
#  Then edit (e.g.) basic_tests.py to put the script in the right place.
#
#     Ensure that the py script (e.g. basic_tests.py) has these imports,
#     so that the composed script that you pasted back finds everything
#     that it uses:
#     import os, shutil
#     from svntest import main, wc, actions, verify
#
#     Be aware that you have to paste the result back to the .py file.
#
#     Be more aware that you have to read every single line and understand
#     that it makes sense. If the current behaviour is wrong, you need to
#     make the changes to expect the correct behaviour and XFail() your test.
#
#     factory.make() just probes the current situation and writes a test that
#     PASSES any success AND ANY FAILURE THAT IT FINDS. The resulting script
#     will never fail anything (if it works correctly), not even a failure.
#
#     ### TODO: some sort of intelligent pasting directly into the
#               right place, like looking for the factory call,
#               inserting the new test there, back-up-ing the old file.
#
#
# TROUBLESHOOTING
# If the result has a problem somewhere along the middle, you can,
# of course, only use the first part of the output result, maybe tweak
# something, and continue with another factory.make() at the end of that.
#
# Or you can first do stuff to your sbox and then call factory on it.
# Factory will notice if the sbox has already been built and calls
# sbox.build() only if you didn't already.
#
# You can also have any number of factory.make() calls scattered
# around "real" test code.
#
# Note that you can pass a prev_status and prev_disk to factory, to make
# the expected_* trees re-use a pre-existing one in the test, entirely
# for code beauty :P (has to match the wc_dir you will be using next).
#
#
# YOU ARE CORDIALLY INVITED to add/tweak/change to your needs.
# If you want to know what's going on, look at the switch()
# funtion of TestFactory below.
#
#
# DETAILS
# =======
#
# The input to factory.make(sbox, input) is not "real" shell-script.
# Factory goes at great lengths to try and understand your script, it
# parses common shell operations during tests and translates them.
#
# All arguments are tokenized similarly to shell, so if you need a space
# in an argument, use quotes.
#   echo "my content" > A/new_file
# Quote char escaping is done like this:
#   echo "my \\" content" > A/new_file
#   echo 'my \\' content' > A/new_file
# If you use r""" echo 'my \' content' > A/new_file """ (triple quotes
# with a leading 'r' character), you don't need to double-escape any
# characters.
#
# You can either supply multiple lines, or separate the lines with ';'.
#   factory.make(sbox, 'echo foo > bar; svn add bar')
#   factory.make(sbox, 'echo foo > bar\n svn add bar')
#   factory.make(sbox, r"""
#                      echo "foo\nbar" > bar
#                      svn add bar
#                      """)
#
#
# WORKING COPY PATHS
# - Factory will automatically build sbox.wc_dir if you didn't do so yet.
#
# - If you supply any path or file name, factory will prepend sbox.wc_dir
#   to it.
#     echo more >> iota
#                      -->   main.file_append(
#                                 os.path.join(sbox.wc_dir, 'iota'),
#                                 "more")
#   You can also do so explicitly.
#     echo more >> wc_dir/iota
#                      -->   main.file_append(
#                                 os.path.join(sbox.wc_dir, 'iota'),
#                                 "more")
#
#   Factory implies the sbox.wc_dir if you fail to supply an explicit
#   working copy dir. If you want to supply one explicitly, you can
#   choose among these wildcards:
#     'wc_dir', 'wcdir', '$WC_DIR', '$WC'  -- all expanded to sbox.wc_dir
#   For example:
#     'svn mkdir wc_dir/A/D/X'
#   But as long as you want to use only the default sbox.wc_dir, you usually
#   don't need to supply any wc_dir-wildcard:
#     'mkdir A/X' creates the directory sbox.wc_dir/A/X
#   (Factory tries to know which arguments of the commands you supplied
#   are eligible to be path arguments. If something goes wrong here, try
#   to fix factory.py to not mistake the arg for something different.
#   You usually just need to tweak some parameters to args2svntest() to
#   achieve correct expansion.)
#
# - If you want to use a second (or Nth) working copy, just supply any
#   working copy wildcard with any made-up suffix, e.g. like this:
#     'svn st wc_dir_2' or 'svn info $WC_2'
#   Factory will detect that you used another wc_dir and will automatically
#   add a corresponding directory to your sbox. The directory will initially
#   be nonexistent, so call 'mkdir', 'svn co' or 'cp' before using:
#     'cp wc_dir wc_dir_other'  -- a copy of the current WC
#     'svn co $URL wc_dir_new'  -- a clean checkout
#     'mkdir wc_dir_empty'      -- an empty directory
#   You can subsequently use any wc-dir wildcard with your suffix added.
#
#     cp wc_dir wc_dir_2
#     echo more >> wc_dir_2/iota
#                      -->   wc_dir_2 = sbox.add_wc_path('2')
#                            shutil.copytrees(wc_dir, wc_dir_2)
#                            main.file_append(
#                                 os.path.join(wc_dir_2, 'iota'),
#                                 "more")
#
#
# URLs
# Factory currently knows only one repository, thus only one repos root.
# The wildcards you can use for it are:
#   'url', '$URL'
# A URL is not inserted automatically like wc_dir, you need to supply a
# URL wildcard.
# Alternatively, you can use '^/' URLs. However, that is in effect a different
# test from an explicit entire URL. The test needs to chdir to the working
# copy in order find which URL '^/' should expand to.
# (currently, factory will chdir to sbox.wc_dir. It will only chdir
#  to another working copy if one of the other arguments involved a WC.
#  ### TODO add a 'cd wc_dir_2' command to select another WC as default.)
# Example:
#   'svn co $URL Y'   -- make a new nested working copy in sbox.wc_dir/Y
#   'svn co $URL wc_dir_2'  -- create a new separate working copy
#   'svn cp ^/A ^/X'        -- do a URL copy, creating $URL/X (branch)
#
#
# SOME EXAMPLES
# These commands should work:
#
# - "svn <subcommand> <options>"
#   Some subcommands are parsed specially, others by a catch-all default
#   parser (cmd_svn()), see switch().
#     'svn commit', 'svn commit --force', 'svn ci wc_dir_2'
#     'svn copy url/A url/X'
#
# - "echo contents > file"     (replace)
#   "echo contents >> file"    (append)
#   Calls main.file_write() / main.file_append().
#     'echo "froogle" >> A/D/G/rho'  -- append to an existing file
#     'echo "bar" > A/quux'          -- create a new file
#     'echo "fool" > wc_dir_2/me'    -- manipulate other working copies
#
# - "mkdir <names> ..."
#   Calls os.makedirs().
#   You probably want 'svn mkdir' instead, or use 'svn add' after this.
#     'mkdir A/D/X'     -- create an unversioned directory
#     'mkdir wc_dir_5'  -- create a new, empty working copy
#
# - "rm <targets>"
#   Calls main.safe_rmtree().
#   You probably want to use 'svn delete' instead.
#     'rm A/D/G'
#     'rm wc_dir_2'
#
# - "mv <source> [<source2> ...] <target>"
#   Calls shutil.move()
#   You probably want to use 'svn move' instead.
#     'mv iota A/D/'    -- move sbox.wc_dir/iota to sbox.wc_dir/A/D/.
#
# - "cp <source> [<source2> ...] <target>"
#   Do a filesystem copy.
#   You probably want to use 'svn copy' instead.
#     'cp wc_dir wc_dir_copy'
#     'cp A/D/G A/X'
#
# IF YOU NEED ANY OTHER COMMANDS:
# - first check if it doesn't work already. If not,
# - add your desired commands to factory.py! :)
# - alternatively, use a number of separate factory calls, doing what
#   you need done in "real" svntest language in-between.
#
# IF YOU REALLY DON'T GROK THIS:
# - ask #svn-dev
# - ask dev@
# - ask neels

import sys, re, os, shutil, bisect, textwrap, shlex

import svntest
from svntest import main, actions, tree
from svntest import Failure

if sys.version_info[0] >= 3:
  # Python >=3.0
  from io import StringIO
else:
  # Python <3.0
  from StringIO import StringIO

def make(wc_dir, commands, prev_status=None, prev_disk=None, verbose=True):
  """The Factory Invocation Function. This is typically the only one
  called from outside this file. See top comment in factory.py.
  Prints the resulting py script to stdout when verbose is True and
  returns the resulting line-list containing items as:
    [ ['pseudo-shell input line #1', '  translation\n  to\n  py #1'], ...]"""
  fac = TestFactory(wc_dir, prev_status, prev_disk)
  fac.make(commands)
  fac.print_script()
  return fac.lines



class TestFactory:
  """This class keeps all state around a factory.make() call."""

  def __init__(self, sbox, prev_status=None, prev_disk=None):
    self.sbox = sbox

    # The input lines and their translations.
    # Each translation usually has multiple output lines ('\n' characters).
    self.lines = []       #  [ ['in1', 'out1'], ['in2', 'out'], ...

    # Any expected_status still there from a previous verification
    self.prev_status = None
    if prev_status:
      self.prev_status = [None, prev_status]  # svntest.wc.State

    # Any expected_disk still there from a previous verification
    self.prev_disk = None
    if prev_disk:
      self.prev_disk = [None, prev_disk]      # svntest.wc.State

    # Those command line options that expect an argument following
    # which is not a path. (don't expand args following these)
    self.keep_args_of = ['--depth', '--encoding', '-r',
                         '--changelist', '-m', '--message']

    # A stack of $PWDs, to be able to chdir back after a chdir.
    self.prevdirs = []

    # The python variables we want to be declared at the beginning.
    # These are path variables like "A_D = os.path.join(wc_dir, 'A', 'D')".
    # The original wc_dir and url vars are not kept here.
    self.vars = {}

    # An optimized list kept up-to-date by variable additions
    self.sorted_vars_by_pathlen = []

    # Wether we ever used the variables 'wc_dir' and 'url' (tiny tweak)
    self.used_wc_dir = False
    self.used_url = False

    # The alternate working copy directories created that need to be
    # registered with sbox (are not inside another working copy).
    self.other_wc_dirs = {}


  def make(self, commands):
    "internal main function, delegates everything except final output."

    # keep a spacer for init
    self.add_line(None, None)

    init = ""
    if not self.sbox.is_built():
      self.sbox.build()
      init += "sbox.build()\n"


    try:
      # split input args
      input_lines = commands.replace(';','\n').splitlines()
      for str in input_lines:
        if len(str.strip()) > 0:
          self.add_line(str)

      for i in range(len(self.lines)):
        if self.lines[i][0] is not None:
          # This is where everything happens:
          self.lines[i][1] = self.switch(self.lines[i][0])

      # We're done. Add a final greeting.
      self.add_line(
        None,
        "Remember, this only saves you typing. Doublecheck everything.")

      # -- Insert variable defs in the first line --
      # main wc_dir and url
      if self.used_wc_dir:
        init += 'wc_dir = sbox.wc_dir\n'
      if self.used_url:
        init += 'url = sbox.repo_url\n'

      # registration of new WC dirs
      sorted_names = self.get_sorted_other_wc_dir_names()
      for name in sorted_names:
        init += name + ' = ' + self.other_wc_dirs[name][0] + '\n'

      if len(init) > 0:
        init += '\n'

      # general variable definitions
      sorted_names = self.get_sorted_var_names()
      for name in sorted_names:
        init += name + ' = ' + self.vars[name][0] + '\n'

      # Insert at the first line, being the spacer from above
      if len(init) > 0:
        self.lines[0][1] = init

      # This usually goes to make() below (outside this class)
      return self.lines
    except:
      for line in self.lines:
        if line[1] is not None:
          print(line[1])
      raise


  def print_script(self, stream=sys.stdout):
    "Output the resulting script of the preceding make() call"
    if self.lines is not None:
      for line in self.lines:
        if line[1] is None:
          # fall back to just that line as it was in the source
          stripped = line[0].strip()
          if not stripped.startswith('#'):
            # for comments, don't say this:
            stream.write("  # don't know how to handle:\n")
          stream.write("  " + line[0].strip() + '\n')
        else:
          if line[0] is not None:
            stream.write( wrap_each_line(line[0].strip(),
                                         "  # ", "  # ", True) + '\n')
          stream.write(wrap_each_line(line[1], "  ", "    ", False) + '\n\n')
    else:
      stream.write("  # empty.\n")
    stream.flush()


  # End of public functions.



  # "Shell" command handlers:

  def switch(self, line):
    "Given one input line, delegates to the appropriate sub-functions."
    args = shlex.split(line)
    if len(args) < 1:
      return ""
    first = args[0]

    # This is just an if-cascade. Feel free to change that.

    if first == 'svn':
      second = args[1]

      if second == 'add':
        return self.cmd_svn(args[1:], False, self.keep_args_of)

      if second in ['changelist', 'cl']:
        keep_count = 2
        if '--remove' in args:
          keep_count = 1
        return self.cmd_svn(args[1:], False, self.keep_args_of, keep_count)

      if second in ['status','stat','st']:
        return self.cmd_svn_status(args[2:])

      if second in ['commit','ci']:
        return self.cmd_svn_commit(args[2:])

      if second in ['update','up']:
        return self.cmd_svn_update(args[2:])

      if second in ['switch','sw']:
        return self.cmd_svn_switch(args[2:])

      if second in ['copy', 'cp',
                    'move', 'mv', 'rename', 'ren']:
        return self.cmd_svn_copy_move(args[1:])

      if second in ['checkout', 'co']:
        return self.cmd_svn_checkout(args[2:])

      if second in ['propset','pset','ps']:
        return self.cmd_svn(args[1:], False,
          self.keep_args_of, 3)

      if second in ['delete','del','remove', 'rm']:
        return self.cmd_svn(args[1:], False,
          self.keep_args_of + ['--with-revprop'])

      # NOTE that not all commands need to be listed here, since
      # some are already adequately handled by self.cmd_svn().
      # If you find yours is not, add another self.cmd_svn_xxx().
      return self.cmd_svn(args[1:], False, self.keep_args_of)

    if first == 'echo':
      return self.cmd_echo(args[1:])

    if first == 'mkdir':
      return self.cmd_mkdir(args[1:])

    if first == 'rm':
      return self.cmd_rm(args[1:])

    if first == 'mv':
      return self.cmd_mv(args[1:])

    if first == 'cp':
      return self.cmd_cp(args[1:])

    # if all fails, take the line verbatim
    return None


  def cmd_svn_standard_run(self, pyargs, runargs, do_chdir, wc):
    "The generic invocation of svn, helper function."
    pychdir = self.chdir(do_chdir, wc)

    code, out, err = main.run_svn("Maybe", *runargs)

    if code == 0 and len(err) < 1:
      # write a test that expects success
      pylist = self.strlist2py(out)
      if len(out) <= 1:
        py = "expected_stdout = " + pylist + "\n\n"
      else:
        py = "expected_stdout = verify.UnorderedOutput(" + pylist + ")\n\n"
      py += pychdir
      py += "actions.run_and_verify_svn2('OUTPUT', expected_stdout, [], 0"
    else:
      # write a test that expects failure
      pylist = self.strlist2py(err)
      if len(err) <= 1:
        py = "expected_stderr = " + pylist + "\n\n"
      else:
        py = "expected_stderr = verify.UnorderedOutput(" + pylist + ")\n\n"
      py += pychdir
      py += ("actions.run_and_verify_svn2('OUTPUT', " +
             "[], expected_stderr, " + str(code))

    if len(pyargs) > 0:
      py += ", " + ", ".join(pyargs)
    py += ")\n"
    py += self.chdir_back(do_chdir)
    return py


  def cmd_svn(self, svnargs, append_wc_dir_if_missing = False,
              keep_args_of = [], keep_first_count = 1,
              drop_with_arg = []):
    "Handles all svn calls not handled by more specific functions."

    pyargs, runargs, do_chdir, targets = self.args2svntest(svnargs,
                        append_wc_dir_if_missing, keep_args_of,
                        keep_first_count, drop_with_arg)

    return self.cmd_svn_standard_run(pyargs, runargs, do_chdir,
                                     self.get_first_wc(targets))


  def cmd_svn_status(self, status_args):
    "Runs svn status, looks what happened and writes the script for it."
    pyargs, runargs, do_chdir, targets = self.args2svntest(
                                  status_args, True, self.keep_args_of, 0)

    py = ""

    for target in targets:
      if not target.wc:
        py += '# SKIPPING NON-WC ' + target.runarg + '\n'
        continue

      if '-q' in status_args:
        pystatus = self.get_current_status(target.wc, True)
        py += (pystatus +
               "actions.run_and_verify_status(" + target.wc.py +
               ", expected_status)\n")
      else:
        pystatus = self.get_current_status(target.wc, False)
        py += (pystatus +
               "actions.run_and_verify_unquiet_status(" + target.wc.py +
               ", expected_status)\n")
    return py


  def cmd_svn_commit(self, commit_args):
    "Runs svn commit, looks what happened and writes the script for it."
    # these are the options that are followed by something that should not
    # be parsed as a filename in the WC.
    commit_arg_opts = [
      "--depth",
      "--with-revprop",
      "--changelist",
      # "-F", "--file", these take a file argument, don't list here.
      # "-m", "--message", treated separately
      ]

    pyargs, runargs, do_chdir, targets = self.args2svntest(
      commit_args, True, commit_arg_opts, 0, ['-m', '--message'])

    wc = self.get_first_wc(targets)
    pychdir = self.chdir(do_chdir, wc)

    code, output, err = main.run_svn("Maybe", 'ci',
                                  '-m', 'log msg',
                                  *runargs)

    if code == 0 and len(err) < 1:
      # write a test that expects success

      output = actions.process_output_for_commit(output)
      actual_out = tree.build_tree_from_commit(output)
      py = ("expected_output = " +
            self.tree2py(actual_out, wc) + "\n\n")

      pystatus = self.get_current_status(wc)
      py += pystatus

      py += pychdir
      py += ("actions.run_and_verify_commit(" + wc.py + ", " +
             "expected_output, expected_status, " +
             "None")
    else:
      # write a test that expects error
      py = "expected_error = " + self.strlist2py(err) + "\n\n"
      py += pychdir
      py += ("actions.run_and_verify_commit(" + wc.py + ", " +
             "None, None, expected_error")

    if len(pyargs) > 0:
      py += ', ' + ', '.join(pyargs)
    py += ")"
    py += self.chdir_back(do_chdir)
    return py


  def cmd_svn_update(self, update_args):
    "Runs svn update, looks what happened and writes the script for it."

    pyargs, runargs, do_chdir, targets = self.args2svntest(
                                  update_args, True, self.keep_args_of, 0)

    wc = self.get_first_wc(targets)
    pychdir = self.chdir(do_chdir, wc)

    code, output, err = main.run_svn('Maybe', 'up', *runargs)

    if code == 0 and len(err) < 1:
      # write a test that expects success

      actual_out = svntest.wc.State.from_checkout(output).old_tree()
      py = ("expected_output = " +
            self.tree2py(actual_out, wc) + "\n\n")

      pydisk = self.get_current_disk(wc)
      py += pydisk

      pystatus = self.get_current_status(wc)
      py += pystatus

      py += pychdir
      py += ("actions.run_and_verify_update(" + wc.py + ", " +
             "expected_output, expected_disk, expected_status, " +
             "None, None, None, None, None, False")
    else:
      # write a test that expects error
      py = "expected_error = " + self.strlist2py(err) + "\n\n"
      py += pychdir
      py += ("actions.run_and_verify_update(" + wc.py + ", None, None, " +
             "None, expected_error, None, None, None, None, False")

    if len(pyargs) > 0:
      py += ', ' + ', '.join(pyargs)
    py += ")"
    py += self.chdir_back(do_chdir)
    return py


  def cmd_svn_switch(self, switch_args):
    "Runs svn switch, looks what happened and writes the script for it."

    pyargs, runargs, do_chdir, targets = self.args2svntest(
                                  switch_args, True, self.keep_args_of, 0)

    # Sort out the targets. We need one URL and one wc node, in that order.
    if len(targets) < 2:
      raise Failure("Sorry, I'm currently enforcing two targets for svn " +
                    "switch. If you want to supply less, remove this " +
                    "check and implement whatever seems appropriate.")

    wc_arg = targets[1]
    del pyargs[wc_arg.argnr]
    del runargs[wc_arg.argnr]
    url_arg = targets[0]
    del pyargs[url_arg.argnr]
    del runargs[url_arg.argnr]

    wc = wc_arg.wc
    if not wc:
      raise Failure("Unexpected argument ordering to factory's 'svn switch'?")

    pychdir = self.chdir(do_chdir, wc)

    #if '--force' in runargs:
    #  self.really_safe_rmtree(wc_arg.runarg)

    code, output, err = main.run_svn('Maybe', 'sw',
                                     url_arg.runarg, wc_arg.runarg,
                                     *runargs)

    py = ""

    if code == 0 and len(err) < 1:
      # write a test that expects success

      actual_out = tree.build_tree_from_checkout(output)
      py = ("expected_output = " +
            self.tree2py(actual_out, wc) + "\n\n")

      pydisk = self.get_current_disk(wc)
      py += pydisk

      pystatus = self.get_current_status(wc)
      py += pystatus

      py += pychdir
      py += ("actions.run_and_verify_switch(" + wc.py + ", " +
             wc_arg.pyarg + ", " + url_arg.pyarg + ", " +
             "expected_output, expected_disk, expected_status, " +
             "None, None, None, None, None, False")
    else:
      # write a test that expects error
      py = "expected_error = " + self.strlist2py(err) + "\n\n"
      py += pychdir
      py += ("actions.run_and_verify_switch(" + wc.py + ", " +
             wc_arg.pyarg + ", " + url_arg.pyarg + ", " +
             "None, None, None, expected_error, None, None, None, None, False")

    if len(pyargs) > 0:
      py += ', ' + ', '.join(pyargs)
    py += ")"
    py += self.chdir_back(do_chdir)

    return py


  def cmd_svn_checkout(self, checkout_args):
    "Runs svn checkout, looks what happened and writes the script for it."

    pyargs, runargs, do_chdir, targets = self.args2svntest(
                                  checkout_args, True, self.keep_args_of, 0)

    # Sort out the targets. We need one URL and one dir, in that order.
    if len(targets) < 2:
      raise Failure("Sorry, I'm currently enforcing two targets for svn " +
                    "checkout. If you want to supply less, remove this " +
                    "check and implement whatever seems appropriate.")
    # We need this separate for the call to run_and_verify_checkout()
    # that's composed in the output script.
    wc_arg = targets[1]
    del pyargs[wc_arg.argnr]
    del runargs[wc_arg.argnr]
    url_arg = targets[0]
    del pyargs[url_arg.argnr]
    del runargs[url_arg.argnr]

    wc = wc_arg.wc

    pychdir = self.chdir(do_chdir, wc)

    #if '--force' in runargs:
    #  self.really_safe_rmtree(wc_arg.runarg)

    code, output, err = main.run_svn('Maybe', 'co',
                                     url_arg.runarg, wc_arg.runarg,
                                     *runargs)

    py = ""

    if code == 0 and len(err) < 1:
      # write a test that expects success

      actual_out = tree.build_tree_from_checkout(output)
      pyout = ("expected_output = " +
               self.tree2py(actual_out, wc) + "\n\n")
      py += pyout

      pydisk = self.get_current_disk(wc)
      py += pydisk

      py += pychdir

      py += ("actions.run_and_verify_checkout(" +
             url_arg.pyarg + ", " + wc_arg.pyarg +
             ", expected_output, expected_disk, None, None, None, None")
    else:
      # write a test that expects failure
      pylist = self.strlist2py(err)
      if len(err) <= 1:
        py += "expected_stderr = " + pylist + "\n\n"
      else:
        py += "expected_stderr = verify.UnorderedOutput(" + pylist + ")\n\n"
      py += pychdir
      py += ("actions.run_and_verify_svn2('OUTPUT', " +
             "[], expected_stderr, " + str(code) +
             ", " + url_arg.pyarg + ", " + wc_arg.pyarg)

    # Append the remaining args
    if len(pyargs) > 0:
      py += ', ' + ', '.join(pyargs)
    py += ")"
    py += self.chdir_back(do_chdir)
    return py


  def cmd_svn_copy_move(self, args):
    "Runs svn copy or move, looks what happened and writes the script for it."

    pyargs, runargs, do_chdir, targets = self.args2svntest(args,
                                                 False, self.keep_args_of, 1)

    if len(targets) == 2 and targets[1].is_url:
      # The second argument is a URL.
      # This needs a log message. Is one supplied?
      has_message = False
      for arg in runargs:
        if arg.startswith('-m') or arg == '--message':
          has_message = True
          break
      if not has_message:
        # add one
        runargs += [ '-m', 'copy log' ]
        pyargs = []
        for arg in runargs:
          pyargs += [ self.str2svntest(arg) ]

    return self.cmd_svn_standard_run(pyargs, runargs, do_chdir,
                                     self.get_first_wc(targets))


  def cmd_echo(self, echo_args):
    "Writes a string to a file and writes the script for it."
    # split off target
    target_arg = None
    replace = True
    contents = None
    for i in range(len(echo_args)):
      arg = echo_args[i]
      if arg.startswith('>'):
        if len(arg) > 1:
          if arg[1] == '>':
            # it's a '>>'
            replace = False
            arg = arg[2:]
          else:
            arg = arg[1:]
          if len(arg) > 0:
            target_arg = arg

        if target_arg is None:
          # we need an index (i+1) to exist, and
          # we need (i+1) to be the only existing index left in the list.
          if i+1 != len(echo_args)-1:
            raise Failure("don't understand: echo " + " ".join(echo_args))
          target_arg = echo_args[i+1]
        else:
          # already got the target. no more indexes should exist.
          if i != len(echo_args)-1:
            raise Failure("don't understand: echo " + " ".join(echo_args))

        contents = " ".join(echo_args[:i]) + '\n'

    if target_arg is None:
      raise Failure("echo needs a '>' pipe to a file name: echo " +
                    " ".join(echo_args))

    target = self.path2svntest(target_arg)

    if replace:
      main.file_write(target.runarg, contents)
      py = "main.file_write("
    else:
      main.file_append(target.runarg, contents)
      py = "main.file_append("
    py += target.pyarg + ", " + self.str2svntest(contents) + ")"

    return py


  def cmd_mkdir(self, mkdir_args):
    "Makes a new directory and writes the script for it."
    # treat all mkdirs as -p, ignore all -options.
    out = ""
    for arg in mkdir_args:
      if not arg.startswith('-'):
        target = self.path2svntest(arg)
        # don't check for not being a url,
        # maybe it's desired by the test or something.
        os.makedirs(target.runarg)
        out += "os.makedirs(" + target.pyarg + ")\n"
    return out


  def cmd_rm(self, rm_args):
    "Removes a directory tree and writes the script for it."
    # treat all removes as -rf, ignore all -options.
    out = ""
    for arg in rm_args:
      if not arg.startswith('-'):
        target = self.path2svntest(arg)
        if os.path.isfile(target.runarg):
          os.remove(target.runarg)
          out += "os.remove(" + target.pyarg + ")\n"
        else:
          self.really_safe_rmtree(target.runarg)
          out += "main.safe_rmtree(" + target.pyarg + ")\n"
    return out


  def cmd_mv(self, mv_args):
    "Moves things in the filesystem and writes the script for it."
    # ignore all -options.
    out = ""
    sources = []
    target = None
    for arg in mv_args:
      if not arg.startswith('-'):
        if target is not None:
          sources += [target]
        target = self.path2svntest(arg)

    out = ""
    for source in sources:
      out += "shutil.move(" + source.pyarg + ", " + target.pyarg + ")\n"
      shutil.move(source.runarg, target.runarg)

    return out


  def cmd_cp(self, mv_args):
    "Copies in the filesystem and writes the script for it."
    # ignore all -options.
    out = ""
    sources = []
    target = None
    for arg in mv_args:
      if not arg.startswith('-'):
        if target is not None:
          sources += [target]
        target = self.path2svntest(arg)

    if not target:
      raise Failure("cp needs a source and a target 'cp wc_dir wc_dir_2'")

    out = ""
    for source in sources:
      if os.path.exists(target.runarg):
        raise Failure("cp target exists, remove first: " + target.pyarg)
      if os.path.isdir(source.runarg):
        shutil.copytree(source.runarg, target.runarg)
        out += "shutil.copytree(" + source.pyarg + ", " + target.pyarg + ")\n"
      elif os.path.isfile(source.runarg):
        shutil.copy2(source.runarg, target.runarg)
        out += "shutil.copy2(" + source.pyarg + ", " + target.pyarg + ")\n"
      else:
        raise Failure("cp copy source does not exist: " + source.pyarg)

    return out


  # End of "shell" command handling functions.



  # Internal helpers:


  class WorkingCopy:
    "Defines the list of info we need around a working copy."
    def __init__(self, py, realpath, suffix):
      self.py = py
      self.realpath = realpath
      self.suffix = suffix


  class Target:
    "Defines the list of info we need around a command line supplied target."
    def __init__(self, pyarg, runarg, argnr, is_url=False, wc=None):
      self.pyarg = pyarg
      self.runarg = runarg
      self.argnr = argnr
      self.is_url = is_url
      self.wc = wc


  def add_line(self, args, translation=None):
    "Definition of how to add a new in/out line pair to LINES."
    self.lines += [ [args, translation] ]


  def really_safe_rmtree(self, dir):
    # Safety catch. We don't want to remove outside the sandbox.
    if dir.find('svn-test-work') < 0:
      raise Failure("Tried to remove path outside working area: " + dir)
    main.safe_rmtree(dir)


  def get_current_disk(self, wc):
    "Probes the given working copy and writes an expected_disk for it."
    actual_disk = svntest.wc.State.from_wc(wc.realpath, False, True)
    actual_disk.wc_dir = wc.realpath

    make_py, prev_disk = self.get_prev_disk(wc)

    # The tests currently compare SVNTreeNode trees, so let's do that too.
    actual_disk_tree = actual_disk.old_tree()
    prev_disk_tree = prev_disk.old_tree()

    # find out the tweaks
    tweaks = self.diff_trees(prev_disk_tree, actual_disk_tree, wc)
    if tweaks == 'Purge':
      make_py = ''
    else:
      tweaks = self.optimize_tweaks(tweaks, actual_disk_tree, wc)

    self.remember_disk(wc, actual_disk)

    pydisk = make_py + self.tweaks2py(tweaks, "expected_disk", wc)
    if len(pydisk) > 0:
      pydisk += '\n'
    return pydisk

  def get_prev_disk(self, wc):
    "Retrieves the last used expected_disk tree if any."
    make_py = ""
    # If a disk was supplied via __init__(), self.prev_disk[0] is set
    # to None, in which case we always use it, not checking WC.
    if self.prev_disk is None or \
       not self.prev_disk[0] in [None, wc.realpath]:
      disk = svntest.main.greek_state.copy()
      disk.wc_dir = wc.realpath
      self.remember_disk(wc, disk)
      make_py = "expected_disk = svntest.main.greek_state.copy()\n"
    else:
      disk = self.prev_disk[1]
    return make_py, disk

  def remember_disk(self, wc, actual):
    "Remembers the current disk tree for future reference."
    self.prev_disk = [wc.realpath, actual]


  def get_current_status(self, wc, quiet=True):
    "Probes the given working copy and writes an expected_status for it."
    if quiet:
      code, output, err = main.run_svn(None, 'status', '-v', '-u', '-q',
                                       wc.realpath)
    else:
      code, output, err = main.run_svn(None, 'status', '-v', '-u',
                                       wc.realpath)
    if code != 0 or len(err) > 0:
      raise Failure("Hmm. `svn status' failed. What now.")

    make_py, prev_status = self.get_prev_status(wc)

    actual_status = svntest.wc.State.from_status(output)

    # The tests currently compare SVNTreeNode trees, so let's do that too.
    prev_status_tree = prev_status.old_tree()
    actual_status_tree = actual_status.old_tree()

    # Get the tweaks
    tweaks = self.diff_trees(prev_status_tree, actual_status_tree, wc)

    if tweaks == 'Purge':
      # The tree is empty (happens with invalid WC dirs)
      make_py = "expected_status = wc.State(" + wc.py + ", {})\n"
      tweaks = []
    else:
      tweaks = self.optimize_tweaks(tweaks, actual_status_tree, wc)

    self.remember_status(wc, actual_status)

    pystatus = make_py + self.tweaks2py(tweaks, "expected_status", wc)
    if len(pystatus) > 0:
      pystatus += '\n'

    return pystatus

  def get_prev_status(self, wc):
    "Retrieves the last used expected_status tree if any."
    make_py = ""
    prev_status = None

    # re-use any previous status if we are still in the same WC dir.
    # If a status was supplied via __init__(), self.prev_status[0] is set
    # to None, in which case we always use it, not checking WC.
    if self.prev_status is None or \
       not self.prev_status[0] in [None, wc.realpath]:
      # There is no or no matching previous status. Make new one.
      try:
        # If it's really a WC, use its base revision
        base_rev = actions.get_wc_base_rev(wc.realpath)
      except:
        # Else, just use zero. Whatever.
        base_rev = 0
      prev_status = actions.get_virginal_state(wc.realpath, base_rev)
      make_py += ("expected_status = actions.get_virginal_state(" +
                  wc.py + ", " + str(base_rev) + ")\n")
    else:
      # We will re-use the previous expected_status.
      prev_status = self.prev_status[1]
      # no need to make_py anything

    return make_py, prev_status

  def remember_status(self, wc, actual_status):
    "Remembers the current status tree for future reference."
    self.prev_status = [wc.realpath, actual_status]


  def chdir(self, do_chdir, wc):
    "Pushes the current dir onto the dir stack, does an os.chdir()."
    if not do_chdir:
      return ""
    self.prevdirs.append(os.getcwd())
    os.chdir(wc.realpath)
    py = ("orig_dir = os.getcwd()  # Need to chdir because of '^/' args\n" +
          "os.chdir(" + wc.py + ")\n")
    return py

  def chdir_back(self, do_chdir):
    "Does os.chdir() back to the directory popped from the dir stack's top."
    if not do_chdir:
      return ""
    # If this fails, there's a missing chdir() call:
    os.chdir(self.prevdirs.pop())
    return "os.chdir(orig_dir)\n"


  def get_sorted_vars_by_pathlen(self):
    """Compose a listing of variable names to be expanded in script output.
    This is intended to be stored in self.sorted_vars_by_pathlen."""
    lst = []

    for dict in [self.vars, self.other_wc_dirs]:
      for name in dict:
        runpath = dict[name][1]
        if not runpath:
          continue
        strlen = len(runpath)
        item = [strlen, name, runpath]
        bisect.insort(lst, item)

    return lst


  def get_sorted_var_names(self):
    """Compose a listing of variable names to be declared.
    This is used by TestFactory.make()."""
    paths = []
    urls = []
    for name in self.vars:
      if name.startswith('url_'):
        bisect.insort(urls, [name.lower(), name])
      else:
        bisect.insort(paths, [name.lower(), name])
    list = []
    for path in paths:
      list += [path[1]]
    for url in urls:
      list += [url[1]]
    return list


  def get_sorted_other_wc_dir_names(self):
    """Compose a listing of working copies to be declared with sbox.
    This is used by TestFactory.make()."""
    list = []
    for name in self.other_wc_dirs:
      bisect.insort(list, [name.lower(), name])
    names = []
    for item in list:
      names += [item[1]]
    return names


  def str2svntest(self, str):
    "Like str2py(), but replaces any known paths with variable names."
    if str is None:
      return "None"

    str = str2py(str)
    quote = str[0]

    def replace(str, path, name, quote):
      return str.replace(path, quote + " + " + name + " + " + quote)

    # We want longer paths first.
    for var in reversed(self.sorted_vars_by_pathlen):
      name = var[1]
      path = var[2]
      str = replace(str, path, name, quote)

    str = replace(str, self.sbox.wc_dir, 'wc_dir', quote)
    str = replace(str, self.sbox.repo_url, 'url', quote)

    # now remove trailing null-str adds:
    #   '' + url_A_C + ''
    str = str.replace("'' + ",'').replace(" + ''",'')
    #   "" + url_A_C + ""
    str = str.replace('"" + ',"").replace(' + ""',"")

    # just a stupid check. tiny tweak. (don't declare wc_dir and url
    # if they never appear)
    if not self.used_wc_dir:
      self.used_wc_dir = (re.search('\bwc_dir\b', str) is not None)
    if not self.used_url:
      self.used_url = str.find('url') >= 0

    return str


  def strlist2py(self, list):
    "Given a list of strings, composes a py script that produces the same."
    if list is None:
      return "None"
    if len(list) < 1:
      return "[]"
    if len(list) == 1:
      return "[" + self.str2svntest(list[0]) + "]"

    py = "[\n"
    for line in list:
      py += "  " + self.str2svntest(line) + ",\n"
    py += "]"
    return py


  def get_node_path(self, node, wc):
    "Tries to return the node path relative to the given working copy."
    path = node.get_printable_path()
    if path.startswith(wc.realpath + os.sep):
      path = path[len(wc.realpath + os.sep):]
    elif path.startswith(wc.realpath):
      path = path[len(wc.realpath):]
    return path


  def node2py(self, node, wc, prepend="", drop_empties=True):
    "Creates a line like 'A/C'    : Item({ ... }) for wc.State composition."
    buf = StringIO()
    node.print_script(buf, wc.realpath, prepend, drop_empties)
    return buf.getvalue()


  def tree2py(self, node, wc):
    "Writes the wc.State definition for the given SVNTreeNode in given WC."
    # svntest.wc.State(wc_dir, {
    #   'A/mu' : Item(verb='Sending'),
    #   'A/D/G/rho' : Item(verb='Sending'),
    #   })
    buf = StringIO()
    tree.dump_tree_script(node, stream=buf, subtree=wc.realpath,
                          wc_varname=wc.py)
    return buf.getvalue()


  def diff_trees(self, left, right, wc):
    """Compares the two trees given by the SVNTreeNode instances LEFT and
    RIGHT in the given working copy and composes an internal list of
    tweaks necessary to make LEFT into RIGHT."""
    if not right.children:
      return 'Purge'
    return self._diff_trees(left, right, wc)

  def _diff_trees(self, left, right, wc):
    "Used by self.diff_trees(). No need to call this. See there."
    # all tweaks collected
    tweaks = []

    # the current tweak in composition
    path = self.get_node_path(left, wc)
    tweak = []

    # node attributes
    if ((left.contents is None) != (right.contents is None)) or \
       (left.contents != right.contents):
      tweak += [ ["contents", right.contents] ]

    for key in left.props:
      if key not in right.props:
        tweak += [ [key, None] ]
      elif left.props[key] != right.props[key]:
        tweak += [ [key, right.props[key]] ]

    for key in right.props:
      if key not in left.props:
        tweak += [ [key, right.props[key]] ]

    for key in left.atts:
      if key not in right.atts:
        tweak += [ [key, None] ]
      elif left.atts[key] != right.atts[key]:
        tweak += [ [key, right.atts[key]] ]

    for key in right.atts:
      if key not in left.atts:
        tweak += [ [key, right.atts[key]] ]

    if len(tweak) > 0:
      changetweak = [ 'Change', [path], tweak]
      tweaks += [changetweak]

    if left.children is not None:
      for leftchild in left.children:
        rightchild = None
        if right.children is not None:
          rightchild = tree.get_child(right, leftchild.name)
        if rightchild is None:
          paths = leftchild.recurse(lambda n: self.get_node_path(n, wc))
          removetweak = [ 'Remove', paths ]
          tweaks += [removetweak]

    if right.children is not None:
      for rightchild in right.children:
        leftchild = None
        if left.children is not None:
          leftchild = tree.get_child(left, rightchild.name)
        if leftchild is None:
          paths_and_nodes = rightchild.recurse(
                                 lambda n: [ self.get_node_path(n, wc), n ] )
          addtweak = [ 'Add', paths_and_nodes ]
          tweaks += [addtweak]
        else:
          tweaks += self._diff_trees(leftchild, rightchild, wc)

    return tweaks


  def optimize_tweaks(self, tweaks, actual_tree, wc):
    "Given an internal list of tweaks, make them optimal by common sense."
    if tweaks == 'Purge':
      return tweaks

    subtree = actual_tree.find_node(wc.realpath)
    if not subtree:
      subtree = actual_tree

    remove_paths = []
    additions = []
    changes = []

    for tweak in tweaks:
      if tweak[0] == 'Remove':
        remove_paths += tweak[1]
      elif tweak[0] == 'Add':
        additions += tweak[1]
      else:
        changes += [tweak]

    # combine removals
    removal = []
    if len(remove_paths) > 0:
      removal = [ [ 'Remove', remove_paths] ]

    # combine additions
    addition = []
    if len(additions) > 0:
      addition = [ [ 'Add', additions ] ]

    # find those changes that should be done on all nodes at once.
    def remove_mod(mod):
      for change in changes:
        if mod in change[2]:
          change[2].remove(mod)

    seen = []
    tweak_all = []
    for change in changes:
      tweak = change[2]
      for mod in tweak:
        if mod in seen:
          continue
        seen += [mod]

        # here we see each single "name=value" tweak in mod.
        # Check if the actual tree had this anyway all the way through.
        name = mod[0]
        val = mod[1]

        if name == 'contents' and val is None:
          continue;

        def check_node(node):
          if (
              (name == 'contents' and node.contents == val)
              or
              (node.props and (name in node.props) and node.props[name] == val)
              or
              (node.atts and (name in node.atts) and node.atts[name] == val)):
            # has this same thing set. count on the left.
            return [node, None]
          return [None, node]
        results = subtree.recurse(check_node)
        have = []
        havent = []
        for result in results:
          if result[0]:
            have += [result[0]]
          else:
            havent += [result[1]]

        if havent == []:
          # ok, then, remove all tweaks that are like this, then
          # add a generic tweak.
          remove_mod(mod)
          tweak_all += [mod]
        elif len(havent) < len(have) * 3: # this is "an empirical factor"
          remove_mod(mod)
          tweak_all += [mod]
          # record the *other* nodes' actual item, overwritten above
          for node in havent:
            name = mod[0]
            if name == 'contents':
              value = node.contents
            elif name in node.props:
              value = node.props[name]
            elif name in node.atts:
              value = node.atts[name]
            else:
              continue
            changes += [ ['Change',
                          [self.get_node_path(node, wc)],
                          [[name, value]]
                         ]
                       ]

    # combine those paths that have exactly the same changes
    i = 0
    j = 0
    while i < len(changes):
      # find other changes that are identical
      j = i + 1
      while j < len(changes):
        if changes[i][2] == changes[j][2]:
          changes[i][1] += changes[j][1]
          del changes[j]
        else:
          j += 1
      i += 1

    # combine those changes that have exactly the same paths
    i = 0
    j = 0
    while i < len(changes):
      # find other paths that are identical
      j = i + 1
      while j < len(changes):
        if changes[i][1] == changes[j][1]:
          changes[i][2] += changes[j][2]
          del changes[j]
        else:
          j += 1
      i += 1

    if tweak_all != []:
      changes = [ ['Change', [], tweak_all ] ] + changes

    return removal + addition + changes


  def tweaks2py(self, tweaks, var_name, wc):
    "Given an internal list of tweaks, write the tweak script for it."
    py = ""
    if tweaks is None:
      return ""

    if tweaks == 'Purge':
      return var_name + " = wc.State(" + wc.py + ", {})\n"

    for tweak in tweaks:
      if tweak[0] == 'Remove':
        py += var_name + ".remove("
        paths = tweak[1]
        py += self.str2svntest(paths[0])
        for path in paths[1:]:
          py += ", " + self.str2svntest(path)
        py += ")\n"

      elif tweak[0] == 'Add':
        # add({'A/D/H/zeta' : Item(status='  ', wc_rev=9), ...})
        py += var_name + ".add({"
        adds = tweak[1]
        for add in adds:
          path = add[0]
          node = add[1]
          py += self.node2py(node, wc, "\n  ", False)
        py += "\n})\n"

      else:
        paths = tweak[1]
        mods = tweak[2]
        if mods != []:
          py += var_name + ".tweak("
          for path in paths:
            py += self.str2svntest(path) + ", "
          def mod2py(mod):
            return mod[0] + "=" + self.str2svntest(mod[1])
          py += mod2py(mods[0])
          for mod in mods[1:]:
            py += ", " + mod2py(mod)
          py += ")\n"
    return py


  def path2svntest(self, path, argnr=None, do_remove_on_new_wc_path=True):
    """Given an input argument, do one hell of a path expansion on it.
    ARGNR is simply inserted into the resulting Target.
    Returns a self.Target instance.
    """
    wc = self.WorkingCopy('wc_dir', self.sbox.wc_dir, None)
    url = self.sbox.repo_url  # do we need multiple URLs too??

    pathsep = '/'
    if path.find('/') < 0 and path.find('\\') >= 0:
      pathsep = '\\'

    is_url = False

    # If you add to these, make sure you add longer ones first, to
    # avoid e.g. '$WC_DIR' matching '$WC' first.
    wc_dir_wildcards = ['wc_dir', 'wcdir', '$WC_DIR', '$WC']
    url_wildcards = ['url', '$URL']

    first = path.split(pathsep, 1)[0]
    if first in wc_dir_wildcards:
      path = path[len(first):]
    elif first in url_wildcards:
      path = path[len(first):]
      is_url = True
    else:
      for url_scheme in ['^/', 'file:/', 'http:/', 'svn:/', 'svn+ssh:/']:
        if path.startswith(url_scheme):
          is_url = True
          # keep it as it is
          pyarg = self.str2svntest(path)
          runarg = path
          return self.Target(pyarg, runarg, argnr, is_url, None)

      for wc_dir_wildcard in wc_dir_wildcards:
        if first.startswith(wc_dir_wildcard):
          # The first path element starts with "wc_dir" (or similar),
          # but it has more attached to it. Like "wc_dir.2" or "wc_dir_other"
          # Record a new wc dir name.

          # try to figure out a nice suffix to pass to sbox.
          # (it will create a new dir called sbox.wc_dir + '.' + suffix)
          suffix = ''
          if first[len(wc_dir_wildcard)] in ['.','-','_']:
            # it's a separator already, don't duplicate the dot. (warm&fuzzy)
            suffix = first[len(wc_dir_wildcard) + 1:]
          if len(suffix) < 1:
            suffix = first[len(wc_dir_wildcard):]

          if len(suffix) < 1:
            raise Failure("no suffix supplied to other-wc_dir arg")

          # Streamline the var name
          suffix = suffix.replace('.','_').replace('-','_')
          other_wc_dir_varname = 'wc_dir_' + suffix

          path = path[len(first):]

          real_path = self.get_other_wc_real_path(other_wc_dir_varname,
                                                  suffix,
                                                  do_remove_on_new_wc_path)

          wc = self.WorkingCopy(other_wc_dir_varname,
                                          real_path, suffix)
          # found a match, no need to loop further, but still process
          # the path further.
          break

    if len(path) < 1 or path == pathsep:
      if is_url:
        self.used_url = True
        pyarg = 'url'
        runarg = url
        wc = None
      else:
        if wc.suffix is None:
          self.used_wc_dir = True
        pyarg = wc.py
        runarg = wc.realpath
    else:
      pathelements = split_remove_empty(path, pathsep)

      # make a new variable, if necessary
      if is_url:
        pyarg, runarg = self.ensure_url_var(pathelements)
        wc = None
      else:
        pyarg, runarg = self.ensure_path_var(wc, pathelements)

    return self.Target(pyarg, runarg, argnr, is_url, wc)


  def get_other_wc_real_path(self, varname, suffix, do_remove):
    "Create or retrieve the path of an alternate working copy."
    if varname in self.other_wc_dirs:
      return self.other_wc_dirs[varname][1]

    # see if there is a wc already in the sbox
    path = self.sbox.wc_dir + '.' + suffix
    if path in self.sbox.test_paths:
      py = "sbox.wc_dir + '." + suffix + "'"
    else:
      # else, we must still create one.
      path = self.sbox.add_wc_path(suffix, do_remove)
      py = "sbox.add_wc_path(" + str2py(suffix)
      if not do_remove:
        py += ", remove=False"
      py += ')'

    value = [py, path]
    self.other_wc_dirs[varname] = [py, path]
    self.sorted_vars_by_pathlen = self.get_sorted_vars_by_pathlen()
    return path


  def define_var(self, name, value):
    "Add a variable definition, don't allow redefinitions."
    # see if we already have this var
    if name in self.vars:
      if self.vars[name] != value:
        raise Failure("Variable name collision. Hm, fix factory.py?")
      # ok, it's recorded correctly. Nothing needs to happen.
      return

    # a new variable needs to be recorded
    self.vars[name] = value
    # update the sorted list of vars for substitution by str2svntest()
    self.sorted_vars_by_pathlen = self.get_sorted_vars_by_pathlen()


  def ensure_path_var(self, wc, pathelements):
    "Given a path in a working copy, make sure we have a variable for it."
    name = "_".join(pathelements)

    if wc.suffix is not None:
      # This is an "other" working copy (not the default).
      # The suffix of the wc_dir variable serves as the prefix:
      # wc_dir_other ==> other_A_D = os.path.join(wc_dir_other, 'A', 'D')
      name = wc.suffix + "_" + name
      if name[0].isdigit():
        name = "_" + name
    else:
      self.used_wc_dir = True

    py = 'os.path.join(' + wc.py
    if len(pathelements) > 0:
      py += ", '" + "', '".join(pathelements) + "'"
    py += ')'

    wc_dir_real_path = wc.realpath
    run = os.path.join(wc_dir_real_path, *pathelements)

    value = [py, run]
    self.define_var(name, value)

    return name, run


  def ensure_url_var(self, pathelements):
    "Given a path in the test repository, ensure we have a url var for it."
    name = "url_" + "_".join(pathelements)

    joined = "/" + "/".join(pathelements)

    py = 'url'
    if len(pathelements) > 0:
      py += " + " + str2py(joined)
    self.used_url = True

    run = self.sbox.repo_url + joined

    value = [py, run]
    self.define_var(name, value)

    return name, run


  def get_first_wc(self, target_list):
    """In a list of Target instances, find the first one that is in a
    working copy and return that WorkingCopy. Default to sbox.wc_dir.
    This is useful if we need a working copy for a '^/' URL."""
    for target in target_list:
      if target.wc:
        return target.wc
    return self.WorkingCopy('wc_dir', self.sbox.wc_dir, None)


  def args2svntest(self, args, append_wc_dir_if_missing = False,
                   keep_args_of = [], keep_first_count = 1,
                   drop_with_arg = []):
    """Tries to be extremely intelligent at parsing command line arguments.
    It needs to know which args are file targets that should be in a
    working copy. File targets are magically expanded.

    args: list of string tokens as passed to factory.make(), e.g.
          ['svn', 'commit', '--force', 'wc_dir2']

    append_wc_dir_if_missing: It's a switch.

    keep_args_of: See TestFactory.keep_args_of (comment in __init__)

    keep_first_count: Don't expand the first N non-option args. This is used
          to preserve e.g. the token 'update' in '[svn] update wc_dir'
          (the 'svn' is usually split off before this function is called).

    drop_with_arg: list of string tokens that are commandline options with
          following argument which we want to drop from the list of args
          (e.g. -m message).
    """

    wc_dir = self.sbox.wc_dir
    url = self.sbox.repo_url

    target_supplied = False
    pyargs = []
    runargs = []
    do_chdir = False
    targets = []
    wc_dirs = []

    i = 0
    while i < len(args):
      arg = args[i]

      if arg in drop_with_arg:
        # skip this and the next arg
        if not arg.startswith('--') and len(arg) > 2:
          # it is a concatenated arg like -r123 instead of -r 123
          # skip only this one. Do nothing.
          i = i
        else:
          # skip this and the next arg
          i += 1

      elif arg.startswith('-'):
        # keep this option arg verbatim.
        pyargs += [ self.str2svntest(arg) ]
        runargs += [ arg ]
        # does this option expect a non-filename argument?
        # take that verbatim as well.
        if arg in keep_args_of:
          i += 1
          if i < len(args):
            arg = args[i]
            pyargs += [ self.str2svntest(arg) ]
            runargs += [ arg ]

      elif keep_first_count > 0:
        # args still to be taken verbatim.
        pyargs += [ self.str2svntest(arg) ]
        runargs += [ arg ]
        keep_first_count -= 1

      elif arg.startswith('^/'):
        # this is a ^/url, keep it verbatim.
        # if we use "^/", we need to chdir(wc_dir).
        do_chdir = True
        pyarg = str2py(arg)
        targets += [ self.Target(pyarg, arg, len(pyargs), True, None) ]
        pyargs += [ pyarg ]
        runargs += [ arg ]

      else:
        # well, then this must be a filename or url, autoexpand it.
        target = self.path2svntest(arg, argnr=len(pyargs))
        pyargs += [ target.pyarg ]
        runargs += [ target.runarg ]
        target_supplied = True
        targets += [ target ]

      i += 1

    if not target_supplied and append_wc_dir_if_missing:
      # add a simple wc_dir target
      self.used_wc_dir = True
      wc = self.WorkingCopy('wc_dir', wc_dir, None)
      targets += [ self.Target('wc_dir', wc_dir, len(pyargs), False, wc) ]
      pyargs += [ 'wc_dir' ]
      runargs += [ wc_dir ]

    return pyargs, runargs, do_chdir, targets

###### END of the TestFactory class ######



# Quotes-preserving text wrapping for output

def find_quote_end(text, i):
  "In string TEXT, find the end of the qoute that starts at TEXT[i]"
  # don't handle """ quotes
  quote = text[i]
  i += 1
  while i < len(text):
    if text[i] == '\\':
      i += 1
    elif text[i] == quote:
      return i
    i += 1
  return len(text) - 1


class MyWrapper(textwrap.TextWrapper):
  "A textwrap.TextWrapper that doesn't break a line within quotes."
  ### TODO regexes would be nice, maybe?
  def _split(self, text):
    parts = []
    i = 0
    start = 0
    # This loop will break before and after each space, but keep
    # quoted strings in one piece. Example, breaks marked '/':
    #  /(one,/ /two(blagger),/ /'three three  three',)/
    while i < len(text):
      if text[i] in ['"', "'"]:
        # handle """ quotes. (why, actually?)
        if text[i:i+3] == '"""':
          end = text[i+3:].find('"""')
          if end >= 0:
            i += end + 2
          else:
            i = len(text) - 1
        else:
          # handle normal quotes
          i = find_quote_end(text, i)
      elif text[i].isspace():
        # split off previous section, if any
        if start < i:
          parts += [text[start:i]]
          start = i
        # split off this space
        parts += [text[i]]
        start = i + 1

      i += 1

    if start < len(text):
      parts += [text[start:]]
    return parts


def wrap_each_line(str, ii, si, blw):
  """Wrap lines to a defined width (<80 chars). Feed the lines single to
  MyWrapper, so that it preserves the current line endings already in there.
  We only want to insert new wraps, not remove existing newlines."""
  wrapper = MyWrapper(77, initial_indent=ii,
                      subsequent_indent=si)

  lines = str.splitlines()
  for i in range(0,len(lines)):
    if lines[i] != '':
      lines[i] = wrapper.fill(lines[i])
  return '\n'.join(lines)



# Other miscellaneous helpers

def sh2str(string):
  "un-escapes away /x sequences"
  if string is None:
    return None
  return string.decode("string-escape")


def get_quote_style(str):
  """find which quote is the outer one, ' or "."""
  quote_char = None
  at = None

  found = str.find("'")
  found2 = str.find('"')

  # If found == found2, both must be -1, so nothing was found.
  if found != found2:
    # If a quote was found
    if found >= 0 and found2 >= 0:
      # If both were found, invalidate the later one
      if found < found2:
        found2 = -1
      else:
        found = -1
    # See which one remains.
    if found >= 0:
      at = found + 1
      quote_char = "'"
    elif found2 >= 0:
      at = found2 + 1
      quote_char = '"'

  return quote_char, at

def split_remove_empty(str, sep):
  "do a split, then remove empty elements."
  list = str.split(sep)
  return filter(lambda item: item and len(item) > 0, list)

def str2py(str):
  "returns the string enclosed in quotes, suitable for py scripts."
  if str is None:
    return "None"

  # try to make a nice choice of quoting character
  if str.find("'") >= 0:
    return '"' + str.encode("string-escape"
                   ).replace("\\'", "'"
                   ).replace('"', '\\"') + '"'
  else:
    return "'" + str.encode("string-escape") + "'"

  return str


### End of file.

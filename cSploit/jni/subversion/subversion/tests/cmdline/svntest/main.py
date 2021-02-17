#
#  main.py: a shared, automated test suite for Subversion
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

import sys     # for argv[]
import os
import shutil  # for rmtree()
import re
import stat    # for ST_MODE
import subprocess
import time    # for time()
import traceback # for print_exc()
import threading
import optparse # for argument parsing
import xml
import urllib

try:
  # Python >=3.0
  import queue
  from urllib.parse import quote as urllib_parse_quote
  from urllib.parse import unquote as urllib_parse_unquote
except ImportError:
  # Python <3.0
  import Queue as queue
  from urllib import quote as urllib_parse_quote
  from urllib import unquote as urllib_parse_unquote

import svntest
from svntest import Failure
from svntest import Skip


######################################################################
#
#  HOW TO USE THIS MODULE:
#
#  Write a new python script that
#
#     1) imports this 'svntest' package
#
#     2) contains a number of related 'test' routines.  (Each test
#        routine should take no arguments, and return None on success
#        or throw a Failure exception on failure.  Each test should
#        also contain a short docstring.)
#
#     3) places all the tests into a list that begins with None.
#
#     4) calls svntest.main.client_test() on the list.
#
#  Also, your tests will probably want to use some of the common
#  routines in the 'Utilities' section below.
#
#####################################################################
# Global stuff

default_num_threads = 5

class SVNProcessTerminatedBySignal(Failure):
  "Exception raised if a spawned process segfaulted, aborted, etc."
  pass

class SVNLineUnequal(Failure):
  "Exception raised if two lines are unequal"
  pass

class SVNUnmatchedError(Failure):
  "Exception raised if an expected error is not found"
  pass

class SVNCommitFailure(Failure):
  "Exception raised if a commit failed"
  pass

class SVNRepositoryCopyFailure(Failure):
  "Exception raised if unable to copy a repository"
  pass

class SVNRepositoryCreateFailure(Failure):
  "Exception raised if unable to create a repository"
  pass

# Windows specifics
if sys.platform == 'win32':
  windows = True
  file_scheme_prefix = 'file:///'
  _exe = '.exe'
  _bat = '.bat'
  os.environ['SVN_DBG_STACKTRACES_TO_STDERR'] = 'y'
else:
  windows = False
  file_scheme_prefix = 'file://'
  _exe = ''
  _bat = ''

# The location of our mock svneditor script.
if windows:
  svneditor_script = os.path.join(sys.path[0], 'svneditor.bat')
else:
  svneditor_script = os.path.join(sys.path[0], 'svneditor.py')

# Username and password used by the working copies
wc_author = 'jrandom'
wc_passwd = 'rayjandom'

# Username and password used by the working copies for "second user"
# scenarios
wc_author2 = 'jconstant' # use the same password as wc_author

# Set C locale for command line programs
os.environ['LC_ALL'] = 'C'

# This function mimics the Python 2.3 urllib function of the same name.
def pathname2url(path):
  """Convert the pathname PATH from the local syntax for a path to the form
  used in the path component of a URL. This does not produce a complete URL.
  The return value will already be quoted using the quote() function."""

  # Don't leave ':' in file://C%3A/ escaped as our canonicalization
  # rules will replace this with a ':' on input.
  return urllib_parse_quote(path.replace('\\', '/')).replace('%3A', ':')

# This function mimics the Python 2.3 urllib function of the same name.
def url2pathname(path):
  """Convert the path component PATH from an encoded URL to the local syntax
  for a path. This does not accept a complete URL. This function uses
  unquote() to decode PATH."""
  return os.path.normpath(urllib_parse_unquote(path))

######################################################################
# The locations of the svn, svnadmin and svnlook binaries, relative to
# the only scripts that import this file right now (they live in ../).
# Use --bin to override these defaults.
svn_binary = os.path.abspath('../../svn/svn' + _exe)
svnadmin_binary = os.path.abspath('../../svnadmin/svnadmin' + _exe)
svnlook_binary = os.path.abspath('../../svnlook/svnlook' + _exe)
svnrdump_binary = os.path.abspath('../../svnrdump/svnrdump' + _exe)
svnsync_binary = os.path.abspath('../../svnsync/svnsync' + _exe)
svnversion_binary = os.path.abspath('../../svnversion/svnversion' + _exe)
svndumpfilter_binary = os.path.abspath('../../svndumpfilter/svndumpfilter' + \
                                       _exe)
entriesdump_binary = os.path.abspath('entries-dump' + _exe)
atomic_ra_revprop_change_binary = os.path.abspath('atomic-ra-revprop-change' + \
                                                  _exe)
wc_lock_tester_binary = os.path.abspath('../libsvn_wc/wc-lock-tester' + _exe)
wc_incomplete_tester_binary = os.path.abspath('../libsvn_wc/wc-incomplete-tester' + _exe)
svnmucc_binary=os.path.abspath('../../../tools/client-side/svnmucc/svnmucc' + \
                               _exe)

# Location to the pristine repository, will be calculated from test_area_url
# when we know what the user specified for --url.
pristine_greek_repos_url = None

# Global variable to track all of our options
options = None

# End of command-line-set global variables.
######################################################################

# All temporary repositories and working copies are created underneath
# this dir, so there's one point at which to mount, e.g., a ramdisk.
work_dir = "svn-test-work"

# Constant for the merge info property.
SVN_PROP_MERGEINFO = "svn:mergeinfo"

# Where we want all the repositories and working copies to live.
# Each test will have its own!
general_repo_dir = os.path.join(work_dir, "repositories")
general_wc_dir = os.path.join(work_dir, "working_copies")

# temp directory in which we will create our 'pristine' local
# repository and other scratch data.  This should be removed when we
# quit and when we startup.
temp_dir = os.path.join(work_dir, 'local_tmp')

# (derivatives of the tmp dir.)
pristine_greek_repos_dir = os.path.join(temp_dir, "repos")
greek_dump_dir = os.path.join(temp_dir, "greekfiles")
default_config_dir = os.path.abspath(os.path.join(temp_dir, "config"))

#
# Our pristine greek-tree state.
#
# If a test wishes to create an "expected" working-copy tree, it should
# call main.greek_state.copy().  That method will return a copy of this
# State object which can then be edited.
#
_item = svntest.wc.StateItem
greek_state = svntest.wc.State('', {
  'iota'        : _item("This is the file 'iota'.\n"),
  'A'           : _item(),
  'A/mu'        : _item("This is the file 'mu'.\n"),
  'A/B'         : _item(),
  'A/B/lambda'  : _item("This is the file 'lambda'.\n"),
  'A/B/E'       : _item(),
  'A/B/E/alpha' : _item("This is the file 'alpha'.\n"),
  'A/B/E/beta'  : _item("This is the file 'beta'.\n"),
  'A/B/F'       : _item(),
  'A/C'         : _item(),
  'A/D'         : _item(),
  'A/D/gamma'   : _item("This is the file 'gamma'.\n"),
  'A/D/G'       : _item(),
  'A/D/G/pi'    : _item("This is the file 'pi'.\n"),
  'A/D/G/rho'   : _item("This is the file 'rho'.\n"),
  'A/D/G/tau'   : _item("This is the file 'tau'.\n"),
  'A/D/H'       : _item(),
  'A/D/H/chi'   : _item("This is the file 'chi'.\n"),
  'A/D/H/psi'   : _item("This is the file 'psi'.\n"),
  'A/D/H/omega' : _item("This is the file 'omega'.\n"),
  })


######################################################################
# Utilities shared by the tests
def wrap_ex(func):
  "Wrap a function, catch, print and ignore exceptions"
  def w(*args, **kwds):
    try:
      return func(*args, **kwds)
    except Failure, ex:
      if ex.__class__ != Failure or ex.args:
        ex_args = str(ex)
        if ex_args:
          print('EXCEPTION: %s: %s' % (ex.__class__.__name__, ex_args))
        else:
          print('EXCEPTION: %s' % ex.__class__.__name__)
  return w

def setup_development_mode():
  "Wraps functions in module actions"
  l = [ 'run_and_verify_svn',
        'run_and_verify_svnversion',
        'run_and_verify_load',
        'run_and_verify_dump',
        'run_and_verify_checkout',
        'run_and_verify_export',
        'run_and_verify_update',
        'run_and_verify_merge',
        'run_and_verify_switch',
        'run_and_verify_commit',
        'run_and_verify_unquiet_status',
        'run_and_verify_status',
        'run_and_verify_diff_summarize',
        'run_and_verify_diff_summarize_xml',
        'run_and_validate_lock']

  for func in l:
    setattr(svntest.actions, func, wrap_ex(getattr(svntest.actions, func)))

def get_admin_name():
  "Return name of SVN administrative subdirectory."

  if (windows or sys.platform == 'cygwin') \
      and 'SVN_ASP_DOT_NET_HACK' in os.environ:
    return '_svn'
  else:
    return '.svn'

def wc_is_singledb(wcpath):
  """Temporary function that checks whether a working copy directory looks
  like it is part of a single-db working copy."""

  pristine = os.path.join(wcpath, get_admin_name(), 'pristine')
  if not os.path.exists(pristine):
    return True

  # Now we must be looking at a multi-db WC dir or the root dir of a
  # single-DB WC.  Sharded 'pristine' dir => single-db, else => multi-db.
  for name in os.listdir(pristine):
    if len(name) == 2:
      return True
    elif len(name) == 40:
      return False

  return False

def get_start_commit_hook_path(repo_dir):
  "Return the path of the start-commit-hook conf file in REPO_DIR."

  return os.path.join(repo_dir, "hooks", "start-commit")


def get_pre_commit_hook_path(repo_dir):
  "Return the path of the pre-commit-hook conf file in REPO_DIR."

  return os.path.join(repo_dir, "hooks", "pre-commit")


def get_post_commit_hook_path(repo_dir):
  "Return the path of the post-commit-hook conf file in REPO_DIR."

  return os.path.join(repo_dir, "hooks", "post-commit")

def get_pre_revprop_change_hook_path(repo_dir):
  "Return the path of the pre-revprop-change hook script in REPO_DIR."

  return os.path.join(repo_dir, "hooks", "pre-revprop-change")

def get_svnserve_conf_file_path(repo_dir):
  "Return the path of the svnserve.conf file in REPO_DIR."

  return os.path.join(repo_dir, "conf", "svnserve.conf")

def get_fsfs_conf_file_path(repo_dir):
  "Return the path of the fsfs.conf file in REPO_DIR."

  return os.path.join(repo_dir, "db", "fsfs.conf")

def get_fsfs_format_file_path(repo_dir):
  "Return the path of the format file in REPO_DIR."

  return os.path.join(repo_dir, "db", "format")

def filter_dbg(lines):
  for line in lines:
    if not line.startswith('DBG:'):
      yield line

# Run any binary, logging the command line and return code
def run_command(command, error_expected, binary_mode=0, *varargs):
  """Run COMMAND with VARARGS. Return exit code as int; stdout, stderr
  as lists of lines (including line terminators).  See run_command_stdin()
  for details.  If ERROR_EXPECTED is None, any stderr output will be
  printed and any stderr output or a non-zero exit code will raise an
  exception."""

  return run_command_stdin(command, error_expected, 0, binary_mode,
                           None, *varargs)

# A regular expression that matches arguments that are trivially safe
# to pass on a command line without quoting on any supported operating
# system:
_safe_arg_re = re.compile(r'^[A-Za-z\d\.\_\/\-\:\@]+$')

def _quote_arg(arg):
  """Quote ARG for a command line.

  Return a quoted version of the string ARG, or just ARG if it contains
  only universally harmless characters.

  WARNING: This function cannot handle arbitrary command-line
  arguments: it is just good enough for what we need here."""

  arg = str(arg)
  if _safe_arg_re.match(arg):
    return arg

  if windows:
    # Note: subprocess.list2cmdline is Windows-specific.
    return subprocess.list2cmdline([arg])
  else:
    # Quoting suitable for most Unix shells.
    return "'" + arg.replace("'", "'\\''") + "'"

def open_pipe(command, bufsize=0, stdin=None, stdout=None, stderr=None):
  """Opens a subprocess.Popen pipe to COMMAND using STDIN,
  STDOUT, and STDERR.  BUFSIZE is passed to subprocess.Popen's
  argument of the same name.

  Returns (infile, outfile, errfile, waiter); waiter
  should be passed to wait_on_pipe."""
  command = [str(x) for x in command]

  # On Windows subprocess.Popen() won't accept a Python script as
  # a valid program to execute, rather it wants the Python executable.
  if (sys.platform == 'win32') and (command[0].endswith('.py')):
    command.insert(0, sys.executable)

  command_string = command[0] + ' ' + ' '.join(map(_quote_arg, command[1:]))

  if not stdin:
    stdin = subprocess.PIPE
  if not stdout:
    stdout = subprocess.PIPE
  if not stderr:
    stderr = subprocess.PIPE

  p = subprocess.Popen(command,
                       bufsize,
                       stdin=stdin,
                       stdout=stdout,
                       stderr=stderr,
                       close_fds=not windows)
  return p.stdin, p.stdout, p.stderr, (p, command_string)

def wait_on_pipe(waiter, binary_mode, stdin=None):
  """WAITER is (KID, COMMAND_STRING).  Wait for KID (opened with open_pipe)
  to finish, dying if it does.  If KID fails, create an error message
  containing any stdout and stderr from the kid.  Show COMMAND_STRING in
  diagnostic messages.  Normalize Windows line endings of stdout and stderr
  if not BINARY_MODE.  Return KID's exit code as int; stdout, stderr as
  lists of lines (including line terminators)."""
  if waiter is None:
    return

  kid, command_string = waiter
  stdout, stderr = kid.communicate(stdin)
  exit_code = kid.returncode

  # Normalize Windows line endings if in text mode.
  if windows and not binary_mode:
    stdout = stdout.replace('\r\n', '\n')
    stderr = stderr.replace('\r\n', '\n')

  # Convert output strings to lists.
  stdout_lines = stdout.splitlines(True)
  stderr_lines = stderr.splitlines(True)

  if exit_code < 0:
    if not windows:
      exit_signal = os.WTERMSIG(-exit_code)
    else:
      exit_signal = exit_code

    if stdout_lines is not None:
      sys.stdout.write("".join(stdout_lines))
      sys.stdout.flush()
    if stderr_lines is not None:
      sys.stderr.write("".join(stderr_lines))
      sys.stderr.flush()
    if options.verbose:
      # show the whole path to make it easier to start a debugger
      sys.stderr.write("CMD: %s terminated by signal %d\n"
                       % (command_string, exit_signal))
      sys.stderr.flush()
    raise SVNProcessTerminatedBySignal
  else:
    if exit_code and options.verbose:
      sys.stderr.write("CMD: %s exited with %d\n"
                       % (command_string, exit_code))
      sys.stderr.flush()
    return stdout_lines, stderr_lines, exit_code

def spawn_process(command, bufsize=0, binary_mode=0, stdin_lines=None,
                  *varargs):
  """Run any binary, supplying input text, logging the command line.
  BUFSIZE dictates the pipe buffer size used in communication with the
  subprocess: 0 means unbuffered, 1 means line buffered, any other
  positive value means use a buffer of (approximately) that size.
  A negative bufsize means to use the system default, which usually
  means fully buffered. The default value for bufsize is 0 (unbuffered).
  Normalize Windows line endings of stdout and stderr if not BINARY_MODE.
  Return exit code as int; stdout, stderr as lists of lines (including
  line terminators)."""
  if stdin_lines and not isinstance(stdin_lines, list):
    raise TypeError("stdin_lines should have list type")

  # Log the command line
  if options.verbose and not command.endswith('.py'):
    sys.stdout.write('CMD: %s %s\n' % (os.path.basename(command),
                                      ' '.join([_quote_arg(x) for x in varargs])))
    sys.stdout.flush()

  infile, outfile, errfile, kid = open_pipe([command] + list(varargs), bufsize)

  if stdin_lines:
    for x in stdin_lines:
      infile.write(x)

  stdout_lines, stderr_lines, exit_code = wait_on_pipe(kid, binary_mode)
  infile.close()

  outfile.close()
  errfile.close()

  return exit_code, stdout_lines, stderr_lines

def run_command_stdin(command, error_expected, bufsize=0, binary_mode=0,
                      stdin_lines=None, *varargs):
  """Run COMMAND with VARARGS; input STDIN_LINES (a list of strings
  which should include newline characters) to program via stdin - this
  should not be very large, as if the program outputs more than the OS
  is willing to buffer, this will deadlock, with both Python and
  COMMAND waiting to write to each other for ever.  For tests where this
  is a problem, setting BUFSIZE to a sufficiently large value will prevent
  the deadlock, see spawn_process().
  Normalize Windows line endings of stdout and stderr if not BINARY_MODE.
  Return exit code as int; stdout, stderr as lists of lines (including
  line terminators).
  If ERROR_EXPECTED is None, any stderr output will be printed and any
  stderr output or a non-zero exit code will raise an exception."""

  if options.verbose:
    start = time.time()

  exit_code, stdout_lines, stderr_lines = spawn_process(command,
                                                        bufsize,
                                                        binary_mode,
                                                        stdin_lines,
                                                        *varargs)

  if options.verbose:
    stop = time.time()
    print('<TIME = %.6f>' % (stop - start))
    for x in stdout_lines:
      sys.stdout.write(x)
    for x in stderr_lines:
      sys.stdout.write(x)

  if (not error_expected) and ((stderr_lines) or (exit_code != 0)):
    if not options.verbose:
      for x in stderr_lines:
        sys.stdout.write(x)
    raise Failure

  return exit_code, \
         [line for line in stdout_lines if not line.startswith("DBG:")], \
         stderr_lines

def create_config_dir(cfgdir, config_contents=None, server_contents=None):
  "Create config directories and files"

  # config file names
  cfgfile_cfg = os.path.join(cfgdir, 'config')
  cfgfile_srv = os.path.join(cfgdir, 'servers')

  # create the directory
  if not os.path.isdir(cfgdir):
    os.makedirs(cfgdir)

  # define default config file contents if none provided
  if config_contents is None:
    config_contents = """
#
[auth]
password-stores =

[miscellany]
interactive-conflicts = false
"""

  # define default server file contents if none provided
  if server_contents is None:
    http_library_str = ""
    if options.http_library:
      http_library_str = "http-library=%s" % (options.http_library)
    server_contents = """
#
[global]
%s
store-plaintext-passwords=yes
store-passwords=yes
""" % (http_library_str)

  file_write(cfgfile_cfg, config_contents)
  file_write(cfgfile_srv, server_contents)

def _with_config_dir(args):
  if '--config-dir' in args:
    return args
  else:
    return args + ('--config-dir', default_config_dir)

def _with_auth(args):
  assert '--password' not in args
  args = args + ('--password', wc_passwd,
                 '--no-auth-cache' )
  if '--username' in args:
    return args
  else:
    return args + ('--username', wc_author )

def _with_log_message(args):

  if '-m' in args or '--message' in args or '-F' in args:
    return args
  else:
    return args + ('--message', 'default log message')

# For running subversion and returning the output
def run_svn(error_expected, *varargs):
  """Run svn with VARARGS; return exit code as int; stdout, stderr as
  lists of lines (including line terminators).  If ERROR_EXPECTED is
  None, any stderr output will be printed and any stderr output or a
  non-zero exit code will raise an exception.  If
  you're just checking that something does/doesn't come out of
  stdout/stderr, you might want to use actions.run_and_verify_svn()."""
  return run_command(svn_binary, error_expected, 0,
                     *(_with_auth(_with_config_dir(varargs))))

# For running svnadmin.  Ignores the output.
def run_svnadmin(*varargs):
  """Run svnadmin with VARARGS, returns exit code as int; stdout, stderr as
  list of lines (including line terminators)."""
  return run_command(svnadmin_binary, 1, 0, *varargs)

# For running svnlook.  Ignores the output.
def run_svnlook(*varargs):
  """Run svnlook with VARARGS, returns exit code as int; stdout, stderr as
  list of lines (including line terminators)."""
  return run_command(svnlook_binary, 1, 0, *varargs)

def run_svnrdump(stdin_input, *varargs):
  """Run svnrdump with VARARGS, returns exit code as int; stdout, stderr as
  list of lines (including line terminators).  Use binary mode for output."""
  if stdin_input:
    return run_command_stdin(svnrdump_binary, 1, 1, 1, stdin_input,
                             *(_with_auth(_with_config_dir(varargs))))
  else:
    return run_command(svnrdump_binary, 1, 1,
                       *(_with_auth(_with_config_dir(varargs))))

def run_svnsync(*varargs):
  """Run svnsync with VARARGS, returns exit code as int; stdout, stderr as
  list of lines (including line terminators)."""
  return run_command(svnsync_binary, 1, 0, *(_with_config_dir(varargs)))

def run_svnversion(*varargs):
  """Run svnversion with VARARGS, returns exit code as int; stdout, stderr
  as list of lines (including line terminators)."""
  return run_command(svnversion_binary, 1, 0, *varargs)

def run_svnmucc(*varargs):
  """Run svnmucc with VARARGS, returns exit code as int; stdout, stderr as
  list of lines (including line terminators).  Use binary mode for output."""
  return run_command(svnmucc_binary, 1, 1,
                     *(_with_auth(_with_config_dir(_with_log_message(varargs)))))

def run_entriesdump(path):
  """Run the entries-dump helper, returning a dict of Entry objects."""
  # use spawn_process rather than run_command to avoid copying all the data
  # to stdout in verbose mode.
  exit_code, stdout_lines, stderr_lines = spawn_process(entriesdump_binary,
                                                        0, 0, None, path)
  if exit_code or stderr_lines:
    ### report on this? or continue to just skip it?
    return None

  class Entry(object):
    pass
  entries = { }
  exec(''.join([line for line in stdout_lines if not line.startswith("DBG:")]))
  return entries

def run_entriesdump_subdirs(path):
  """Run the entries-dump helper, returning a list of directory names."""
  # use spawn_process rather than run_command to avoid copying all the data
  # to stdout in verbose mode.
  exit_code, stdout_lines, stderr_lines = spawn_process(entriesdump_binary,
                                                        0, 0, None, '--subdirs', path)
  return [line.strip() for line in stdout_lines if not line.startswith("DBG:")]

def run_atomic_ra_revprop_change(url, revision, propname, skel, want_error):
  """Run the atomic-ra-revprop-change helper, returning its exit code, stdout,
  and stderr.  For HTTP, default HTTP library is used."""
  # use spawn_process rather than run_command to avoid copying all the data
  # to stdout in verbose mode.
  #exit_code, stdout_lines, stderr_lines = spawn_process(entriesdump_binary,
  #                                                      0, 0, None, path)

  # This passes HTTP_LIBRARY in addition to our params.
  return run_command(atomic_ra_revprop_change_binary, True, False,
                     url, revision, propname, skel,
                     options.http_library, want_error and 1 or 0)

def run_wc_lock_tester(recursive, path):
  "Run the wc-lock obtainer tool, returning its exit code, stdout and stderr"
  if recursive:
    option = "-r"
  else:
    option = "-1"
  return run_command(wc_lock_tester_binary, False, False, option, path)

def run_wc_incomplete_tester(wc_dir, revision):
  "Run the wc-incomplete tool, returning its exit code, stdout and stderr"
  return run_command(wc_incomplete_tester_binary, False, False,
                     wc_dir, revision)

def youngest(repos_path):
  "run 'svnlook youngest' on REPOS_PATH, returns revision as int"
  exit_code, stdout_lines, stderr_lines = run_command(svnlook_binary, None, 0,
                                                      'youngest', repos_path)
  if exit_code or stderr_lines:
    raise Failure("Unexpected failure of 'svnlook youngest':\n%s" % stderr_lines)
  if len(stdout_lines) != 1:
    raise Failure("Wrong output from 'svnlook youngest':\n%s" % stdout_lines)
  return int(stdout_lines[0].rstrip())

# Chmod recursively on a whole subtree
def chmod_tree(path, mode, mask):
  for dirpath, dirs, files in os.walk(path):
    for name in dirs + files:
      fullname = os.path.join(dirpath, name)
      if not os.path.islink(fullname):
        new_mode = (os.stat(fullname)[stat.ST_MODE] & ~mask) | mode
        os.chmod(fullname, new_mode)

# For clearing away working copies
def safe_rmtree(dirname, retry=0):
  """Remove the tree at DIRNAME, making it writable first.
     If DIRNAME is a symlink, only remove the symlink, not its target."""
  def rmtree(dirname):
    chmod_tree(dirname, 0666, 0666)
    shutil.rmtree(dirname)

  if os.path.islink(dirname):
    os.unlink(dirname)
    return

  if not os.path.exists(dirname):
    return

  if retry:
    for delay in (0.5, 1, 2, 4):
      try:
        rmtree(dirname)
        break
      except:
        time.sleep(delay)
    else:
      rmtree(dirname)
  else:
    rmtree(dirname)

# For making local mods to files
def file_append(path, new_text):
  "Append NEW_TEXT to file at PATH"
  open(path, 'a').write(new_text)

# Append in binary mode
def file_append_binary(path, new_text):
  "Append NEW_TEXT to file at PATH in binary mode"
  open(path, 'ab').write(new_text)

# For creating new files, and making local mods to existing files.
def file_write(path, contents, mode='w'):
  """Write the CONTENTS to the file at PATH, opening file using MODE,
  which is (w)rite by default."""
  open(path, mode).write(contents)

# For replacing parts of contents in an existing file, with new content.
def file_substitute(path, contents, new_contents):
  """Replace the CONTENTS in the file at PATH using the NEW_CONTENTS"""
  fcontent = open(path, 'r').read().replace(contents, new_contents)
  open(path, 'w').write(fcontent)

# For creating blank new repositories
def create_repos(path):
  """Create a brand-new SVN repository at PATH.  If PATH does not yet
  exist, create it."""

  if not os.path.exists(path):
    os.makedirs(path) # this creates all the intermediate dirs, if neccessary

  opts = ("--bdb-txn-nosync",)
  if options.server_minor_version < 4:
    opts += ("--pre-1.4-compatible",)
  elif options.server_minor_version < 5:
    opts += ("--pre-1.5-compatible",)
  elif options.server_minor_version < 6:
    opts += ("--pre-1.6-compatible",)
  if options.fs_type is not None:
    opts += ("--fs-type=" + options.fs_type,)
  exit_code, stdout, stderr = run_command(svnadmin_binary, 1, 0, "create",
                                          path, *opts)

  # Skip tests if we can't create the repository.
  if stderr:
    for line in stderr:
      if line.find('Unknown FS type') != -1:
        raise Skip
    # If the FS type is known, assume the repos couldn't be created
    # (e.g. due to a missing 'svnadmin' binary).
    raise SVNRepositoryCreateFailure("".join(stderr).rstrip())

  # Require authentication to write to the repos, for ra_svn testing.
  file_write(get_svnserve_conf_file_path(path),
             "[general]\nauth-access = write\n");
  if options.enable_sasl:
    file_append(get_svnserve_conf_file_path(path),
                "realm = svntest\n[sasl]\nuse-sasl = true\n")
  else:
    file_append(get_svnserve_conf_file_path(path), "password-db = passwd\n")
    file_append(os.path.join(path, "conf", "passwd"),
                "[users]\njrandom = rayjandom\njconstant = rayjandom\n");

  if options.fs_type is None or options.fs_type == 'fsfs':
    # fsfs.conf file
    if options.config_file is not None:
      shutil.copy(options.config_file, get_fsfs_conf_file_path(path))

    # format file
    if options.fsfs_sharding is not None:
      def transform_line(line):
        if line.startswith('layout '):
          if options.fsfs_sharding > 0:
            line = 'layout sharded %d' % options.fsfs_sharding
          else:
            line = 'layout linear'
        return line

      # read it
      format_file_path = get_fsfs_format_file_path(path)
      contents = open(format_file_path, 'rb').read()

      # tweak it
      new_contents = "".join([transform_line(line) + "\n"
                              for line in contents.split("\n")])
      if new_contents[-1] == "\n":
        # we don't currently allow empty lines (\n\n) in the format file.
        new_contents = new_contents[:-1]

      # replace it
      os.chmod(format_file_path, 0666)
      file_write(format_file_path, new_contents, 'wb')

    # post-commit
    # Note that some tests (currently only commit_tests) create their own
    # post-commit hooks, which would override this one. :-(
    if options.fsfs_packing:
      # some tests chdir.
      abs_path = os.path.abspath(path)
      create_python_hook_script(get_post_commit_hook_path(abs_path),
          "import subprocess\n"
          "import sys\n"
          "command = %s\n"
          "sys.exit(subprocess.Popen(command).wait())\n"
          % repr([svnadmin_binary, 'pack', abs_path]))

  # make the repos world-writeable, for mod_dav_svn's sake.
  chmod_tree(path, 0666, 0666)

# For copying a repository
def copy_repos(src_path, dst_path, head_revision, ignore_uuid = 1):
  "Copy the repository SRC_PATH, with head revision HEAD_REVISION, to DST_PATH"

  # Save any previous value of SVN_DBG_QUIET
  saved_quiet = os.environ.get('SVN_DBG_QUIET')
  os.environ['SVN_DBG_QUIET'] = 'y'

  # Do an svnadmin dump|svnadmin load cycle. Print a fake pipe command so that
  # the displayed CMDs can be run by hand
  create_repos(dst_path)
  dump_args = ['dump', src_path]
  load_args = ['load', dst_path]

  if ignore_uuid:
    load_args = load_args + ['--ignore-uuid']
  if options.verbose:
    sys.stdout.write('CMD: %s %s | %s %s\n' %
                     (os.path.basename(svnadmin_binary), ' '.join(dump_args),
                      os.path.basename(svnadmin_binary), ' '.join(load_args)))
    sys.stdout.flush()
  start = time.time()

  dump_in, dump_out, dump_err, dump_kid = open_pipe(
    [svnadmin_binary] + dump_args)
  load_in, load_out, load_err, load_kid = open_pipe(
    [svnadmin_binary] + load_args,
    stdin=dump_out) # Attached to dump_kid

  stop = time.time()
  if options.verbose:
    print('<TIME = %.6f>' % (stop - start))

  load_stdout, load_stderr, load_exit_code = wait_on_pipe(load_kid, True)
  dump_stdout, dump_stderr, dump_exit_code = wait_on_pipe(dump_kid, True)

  dump_in.close()
  dump_out.close()
  dump_err.close()
  #load_in is dump_out so it's already closed.
  load_out.close()
  load_err.close()

  if saved_quiet is None:
    del os.environ['SVN_DBG_QUIET']
  else:
    os.environ['SVN_DBG_QUIET'] = saved_quiet

  dump_re = re.compile(r'^\* Dumped revision (\d+)\.\r?$')
  expect_revision = 0
  for dump_line in dump_stderr:
    match = dump_re.match(dump_line)
    if not match or match.group(1) != str(expect_revision):
      print('ERROR:  dump failed: %s' % dump_line.strip())
      raise SVNRepositoryCopyFailure
    expect_revision += 1
  if expect_revision != head_revision + 1:
    print('ERROR:  dump failed; did not see revision %s' % head_revision)
    raise SVNRepositoryCopyFailure

  load_re = re.compile(r'^------- Committed revision (\d+) >>>\r?$')
  expect_revision = 1
  for load_line in filter_dbg(load_stdout):
    match = load_re.match(load_line)
    if match:
      if match.group(1) != str(expect_revision):
        print('ERROR:  load failed: %s' % load_line.strip())
        raise SVNRepositoryCopyFailure
      expect_revision += 1
  if expect_revision != head_revision + 1:
    print('ERROR:  load failed; did not see revision %s' % head_revision)
    raise SVNRepositoryCopyFailure


def canonicalize_url(input):
  "Canonicalize the url, if the scheme is unknown, returns intact input"

  m = re.match(r"^((file://)|((svn|svn\+ssh|http|https)(://)))", input)
  if m:
    scheme = m.group(1)
    return scheme + re.sub(r'//*', '/', input[len(scheme):])
  else:
    return input


def create_python_hook_script(hook_path, hook_script_code):
  """Create a Python hook script at HOOK_PATH with the specified
     HOOK_SCRIPT_CODE."""

  if windows:
    # Use an absolute path since the working directory is not guaranteed
    hook_path = os.path.abspath(hook_path)
    # Fill the python file.
    file_write("%s.py" % hook_path, hook_script_code)
    # Fill the batch wrapper file.
    file_append("%s.bat" % hook_path,
                "@\"%s\" %s.py %%*\n" % (sys.executable, hook_path))
  else:
    # For all other platforms
    file_write(hook_path, "#!%s\n%s" % (sys.executable, hook_script_code))
    os.chmod(hook_path, 0755)

def write_restrictive_svnserve_conf(repo_dir, anon_access="none"):
  "Create a restrictive authz file ( no anynomous access )."

  fp = open(get_svnserve_conf_file_path(repo_dir), 'w')
  fp.write("[general]\nanon-access = %s\nauth-access = write\n"
           "authz-db = authz\n" % anon_access)
  if options.enable_sasl:
    fp.write("realm = svntest\n[sasl]\nuse-sasl = true\n");
  else:
    fp.write("password-db = passwd\n")
  fp.close()

# Warning: because mod_dav_svn uses one shared authz file for all
# repositories, you *cannot* use write_authz_file in any test that
# might be run in parallel.
#
# write_authz_file can *only* be used in test suites which disable
# parallel execution at the bottom like so
#   if __name__ == '__main__':
#     svntest.main.run_tests(test_list, serial_only = True)
def write_authz_file(sbox, rules, sections=None):
  """Write an authz file to SBOX, appropriate for the RA method used,
with authorizations rules RULES mapping paths to strings containing
the rules. You can add sections SECTIONS (ex. groups, aliases...) with
an appropriate list of mappings.
"""
  fp = open(sbox.authz_file, 'w')

  # When the sandbox repository is read only it's name will be different from
  # the repository name.
  repo_name = sbox.repo_dir
  while repo_name[-1] == '/':
    repo_name = repo_name[:-1]
  repo_name = os.path.basename(repo_name)

  if sbox.repo_url.startswith("http"):
    prefix = repo_name + ":"
  else:
    prefix = ""
  if sections:
    for p, r in sections.items():
      fp.write("[%s]\n%s\n" % (p, r))

  for p, r in rules.items():
    fp.write("[%s%s]\n%s\n" % (prefix, p, r))
  fp.close()

def use_editor(func):
  os.environ['SVN_EDITOR'] = svneditor_script
  os.environ['SVN_MERGE'] = svneditor_script
  os.environ['SVNTEST_EDITOR_FUNC'] = func
  os.environ['SVN_TEST_PYTHON'] = sys.executable

def mergeinfo_notify_line(revstart, revend):
  """Return an expected output line that describes the beginning of a
  mergeinfo recording notification on revisions REVSTART through REVEND."""
  if (revend is None):
    if (revstart < 0):
      revstart = abs(revstart)
      return "--- Recording mergeinfo for reverse merge of r%ld .*:\n" \
             % (revstart)
    else:
      return "--- Recording mergeinfo for merge of r%ld .*:\n" % (revstart)
  elif (revstart < revend):
    return "--- Recording mergeinfo for merge of r%ld through r%ld .*:\n" \
           % (revstart, revend)
  else:
    return "--- Recording mergeinfo for reverse merge of r%ld through " \
           "r%ld .*:\n" % (revstart, revend)

def merge_notify_line(revstart=None, revend=None, same_URL=True,
                      foreign=False):
  """Return an expected output line that describes the beginning of a
  merge operation on revisions REVSTART through REVEND.  Omit both
  REVSTART and REVEND for the case where the left and right sides of
  the merge are from different URLs."""
  from_foreign_phrase = foreign and "\(from foreign repository\) " or ""
  if not same_URL:
    return "--- Merging differences between %srepository URLs into '.+':\n" \
           % (foreign and "foreign " or "")
  if revend is None:
    if revstart is None:
      # The left and right sides of the merge are from different URLs.
      return "--- Merging differences between %srepository URLs into '.+':\n" \
             % (foreign and "foreign " or "")
    elif revstart < 0:
      return "--- Reverse-merging %sr%ld into '.+':\n" \
             % (from_foreign_phrase, abs(revstart))
    else:
      return "--- Merging %sr%ld into '.+':\n" \
             % (from_foreign_phrase, revstart)
  else:
    if revstart > revend:
      return "--- Reverse-merging %sr%ld through r%ld into '.+':\n" \
             % (from_foreign_phrase, revstart, revend)
    else:
      return "--- Merging %sr%ld through r%ld into '.+':\n" \
             % (from_foreign_phrase, revstart, revend)


def make_log_msg():
  "Conjure up a log message based on the calling test."

  for idx in range(1, 100):
    frame = sys._getframe(idx)

    # If this frame isn't from a function in *_tests.py, then skip it.
    filename = frame.f_code.co_filename
    if not filename.endswith('_tests.py'):
      continue

    # There should be a test_list in this module.
    test_list = frame.f_globals.get('test_list')
    if test_list is None:
      continue

    # If the function is not in the test_list, then skip it.
    func_name = frame.f_code.co_name
    func_ob = frame.f_globals.get(func_name)
    if func_ob not in test_list:
      continue

    # Make the log message look like a line from a traceback.
    # Well...close. We use single quotes to avoid interfering with the
    # double-quote quoting performed on Windows
    return "File '%s', line %d, in %s" % (filename, frame.f_lineno, func_name)


######################################################################
# Functions which check the test configuration
# (useful for conditional XFails)

def is_ra_type_dav():
  return options.test_area_url.startswith('http')

def is_ra_type_dav_neon():
  """Return True iff running tests over RA-Neon.
     CAUTION: Result is only valid if svn was built to support both."""
  return options.test_area_url.startswith('http') and \
    (options.http_library == "neon")

def is_ra_type_dav_serf():
  """Return True iff running tests over RA-Serf.
     CAUTION: Result is only valid if svn was built to support both."""
  return options.test_area_url.startswith('http') and \
    (options.http_library == "serf")

def is_ra_type_svn():
  """Return True iff running tests over RA-svn."""
  return options.test_area_url.startswith('svn')

def is_ra_type_file():
  """Return True iff running tests over RA-local."""
  return options.test_area_url.startswith('file')

def is_fs_type_fsfs():
  # This assumes that fsfs is the default fs implementation.
  return options.fs_type == 'fsfs' or options.fs_type is None

def is_fs_type_bdb():
  return options.fs_type == 'bdb'

def is_os_windows():
  return os.name == 'nt'

def is_windows_type_dav():
  return is_os_windows() and is_ra_type_dav()

def is_posix_os():
  return os.name == 'posix'

def is_os_darwin():
  return sys.platform == 'darwin'

def is_fs_case_insensitive():
  return (is_os_darwin() or is_os_windows())

def is_threaded_python():
  return True

def server_has_mergeinfo():
  return options.server_minor_version >= 5

def server_has_revprop_commit():
  return options.server_minor_version >= 5

def server_authz_has_aliases():
  return options.server_minor_version >= 5

def server_gets_client_capabilities():
  return options.server_minor_version >= 5

def server_has_partial_replay():
  return options.server_minor_version >= 5

def server_enforces_UTF8_fspaths_in_verify():
  return options.server_minor_version >= 6

def server_enforces_date_syntax():
  return options.server_minor_version >= 5

def server_has_atomic_revprop():
  return options.server_minor_version >= 7

######################################################################


class TestSpawningThread(threading.Thread):
  """A thread that runs test cases in their own processes.
  Receives test numbers to run from the queue, and saves results into
  the results field."""
  def __init__(self, queue):
    threading.Thread.__init__(self)
    self.queue = queue
    self.results = []

  def run(self):
    while True:
      try:
        next_index = self.queue.get_nowait()
      except queue.Empty:
        return

      self.run_one(next_index)

  def run_one(self, index):
    command = os.path.abspath(sys.argv[0])

    args = []
    args.append(str(index))
    args.append('-c')
    # add some startup arguments from this process
    if options.fs_type:
      args.append('--fs-type=' + options.fs_type)
    if options.test_area_url:
      args.append('--url=' + options.test_area_url)
    if options.verbose:
      args.append('-v')
    if options.cleanup:
      args.append('--cleanup')
    if options.enable_sasl:
      args.append('--enable-sasl')
    if options.http_library:
      args.append('--http-library=' + options.http_library)
    if options.server_minor_version:
      args.append('--server-minor-version=' + str(options.server_minor_version))
    if options.mode_filter:
      args.append('--mode-filter=' + options.mode_filter)
    if options.milestone_filter:
      args.append('--milestone-filter=' + options.milestone_filter)

    result, stdout_lines, stderr_lines = spawn_process(command, 0, 0, None,
                                                       *args)
    self.results.append((index, result, stdout_lines, stderr_lines))

    if result != 1:
      sys.stdout.write('.')
    else:
      sys.stdout.write('F')

    sys.stdout.flush()

class TestRunner:
  """Encapsulate a single test case (predicate), including logic for
  runing the test and test list output."""

  def __init__(self, func, index):
    self.pred = svntest.testcase.create_test_case(func)
    self.index = index

  def list(self, milestones_dict=None):
    """Print test doc strings.  MILESTONES_DICT is an optional mapping
    of issue numbers to target milestones."""
    if options.mode_filter.upper() == 'ALL' \
       or options.mode_filter.upper() == self.pred.list_mode().upper() \
       or (options.mode_filter.upper() == 'PASS' \
           and self.pred.list_mode() == ''):
      issues = []
      tail = ''
      if self.pred.issues:
        if not options.milestone_filter or milestones_dict is None:
          issues = self.pred.issues
        else: # Limit listing by requested target milestone(s).
          filter_issues = []
          matches_filter = False

          # Get the milestones for all the issues associated with this test.
          # If any one of them matches the MILESTONE_FILTER then we'll print
          # them all.
          for issue in self.pred.issues:
            # A safe starting assumption.
            milestone = 'unknown'
            if milestones_dict:
              if milestones_dict.has_key(str(issue)):
                milestone = milestones_dict[str(issue)]

            filter_issues.append(str(issue) + '(' + milestone + ')')
            pattern = re.compile(options.milestone_filter)
            if pattern.match(milestone):
              matches_filter = True

          # Did at least one of the associated issues meet our filter?
          if matches_filter:
            issues = filter_issues

        tail += " [%s]" % ','.join(['#%s' % str(i) for i in issues])

      # If there is no filter or this test made if through
      # the filter then print it!
      if options.milestone_filter is None or len(issues):
        if options.verbose and self.pred.inprogress:
          tail += " [[%s]]" % self.pred.inprogress
        else:
          print(" %3d    %-5s  %s%s" % (self.index,
                                        self.pred.list_mode(),
                                        self.pred.description,
                                        tail))
    sys.stdout.flush()

  def get_mode(self):
    return self.pred.list_mode()

  def get_issues(self):
    return self.pred.issues

  def get_function_name(self):
    return self.pred.get_function_name()

  def _print_name(self, prefix):
    if self.pred.inprogress:
      print("%s %s %s: %s [[WIMP: %s]]" % (prefix,
                                           os.path.basename(sys.argv[0]),
                                           str(self.index),
                                           self.pred.description,
                                           self.pred.inprogress))
    else:
      print("%s %s %s: %s" % (prefix,
                              os.path.basename(sys.argv[0]),
                              str(self.index),
                              self.pred.description))
    sys.stdout.flush()

  def run(self):
    """Run self.pred and return the result.  The return value is
        - 0 if the test was successful
        - 1 if it errored in a way that indicates test failure
        - 2 if the test skipped
        """
    sbox_name = self.pred.get_sandbox_name()
    if sbox_name:
      sandbox = svntest.sandbox.Sandbox(sbox_name, self.index)
    else:
      sandbox = None

    # Explicitly set this so that commands that commit but don't supply a
    # log message will fail rather than invoke an editor.
    # Tests that want to use an editor should invoke svntest.main.use_editor.
    os.environ['SVN_EDITOR'] = ''
    os.environ['SVNTEST_EDITOR_FUNC'] = ''

    if options.use_jsvn:
      # Set this SVNKit specific variable to the current test (test name plus
      # its index) being run so that SVNKit daemon could use this test name
      # for its separate log file
     os.environ['SVN_CURRENT_TEST'] = os.path.basename(sys.argv[0]) + "_" + \
                                      str(self.index)

    svntest.actions.no_sleep_for_timestamps()
    svntest.actions.do_relocate_validation()

    saved_dir = os.getcwd()
    try:
      rc = self.pred.run(sandbox)
      if rc is not None:
        self._print_name('STYLE ERROR in')
        print('Test driver returned a status code.')
        sys.exit(255)
      result = svntest.testcase.RESULT_OK
    except Skip, ex:
      result = svntest.testcase.RESULT_SKIP
    except Failure, ex:
      result = svntest.testcase.RESULT_FAIL
      # We captured Failure and its subclasses. We don't want to print
      # anything for plain old Failure since that just indicates test
      # failure, rather than relevant information. However, if there
      # *is* information in the exception's arguments, then print it.
      if ex.__class__ != Failure or ex.args:
        ex_args = str(ex)
        print('CWD: %s' % os.getcwd())
        if ex_args:
          print('EXCEPTION: %s: %s' % (ex.__class__.__name__, ex_args))
        else:
          print('EXCEPTION: %s' % ex.__class__.__name__)
      traceback.print_exc(file=sys.stdout)
      sys.stdout.flush()
    except KeyboardInterrupt:
      print('Interrupted')
      sys.exit(0)
    except SystemExit, ex:
      print('EXCEPTION: SystemExit(%d), skipping cleanup' % ex.code)
      self._print_name(ex.code and 'FAIL: ' or 'PASS: ')
      raise
    except:
      result = svntest.testcase.RESULT_FAIL
      print('CWD: %s' % os.getcwd())
      print('UNEXPECTED EXCEPTION:')
      traceback.print_exc(file=sys.stdout)
      sys.stdout.flush()

    os.chdir(saved_dir)
    exit_code, result_text, result_benignity = self.pred.results(result)
    if not (options.quiet and result_benignity):
      self._print_name(result_text)
    if sandbox is not None and exit_code != 1 and options.cleanup:
      sandbox.cleanup_test_paths()
    return exit_code

######################################################################
# Main testing functions

# These two functions each take a TEST_LIST as input.  The TEST_LIST
# should be a list of test functions; each test function should take
# no arguments and return a 0 on success, non-zero on failure.
# Ideally, each test should also have a short, one-line docstring (so
# it can be displayed by the 'list' command.)

# Func to run one test in the list.
def run_one_test(n, test_list, finished_tests = None):
  """Run the Nth client test in TEST_LIST, return the result.

  If we're running the tests in parallel spawn the test in a new process.
  """

  # allow N to be negative, so './basic_tests.py -- -1' works
  num_tests = len(test_list) - 1
  if (n == 0) or (abs(n) > num_tests):
    print("There is no test %s.\n" % n)
    return 1
  if n < 0:
    n += 1+num_tests

  test_mode = TestRunner(test_list[n], n).get_mode().upper()
  if options.mode_filter.upper() == 'ALL' \
     or options.mode_filter.upper() == test_mode \
     or (options.mode_filter.upper() == 'PASS' and test_mode == ''):
    # Run the test.
    exit_code = TestRunner(test_list[n], n).run()
    return exit_code
  else:
    return 0

def _internal_run_tests(test_list, testnums, parallel, srcdir, progress_func):
  """Run the tests from TEST_LIST whose indices are listed in TESTNUMS.

  If we're running the tests in parallel spawn as much parallel processes
  as requested and gather the results in a temp. buffer when a child
  process is finished.
  """

  exit_code = 0
  finished_tests = []
  tests_started = 0

  # Some of the tests use sys.argv[0] to locate their test data
  # directory.  Perhaps we should just be passing srcdir to the tests?
  if srcdir:
    sys.argv[0] = os.path.join(srcdir, 'subversion', 'tests', 'cmdline',
                               sys.argv[0])

  if not parallel:
    for i, testnum in enumerate(testnums):

      if run_one_test(testnum, test_list) == 1:
          exit_code = 1
      # signal progress
      if progress_func:
        progress_func(i+1, len(testnums))
  else:
    number_queue = queue.Queue()
    for num in testnums:
      number_queue.put(num)

    threads = [ TestSpawningThread(number_queue) for i in range(parallel) ]
    for t in threads:
      t.start()

    for t in threads:
      t.join()

    # list of (index, result, stdout, stderr)
    results = []
    for t in threads:
      results += t.results
    results.sort()

    # signal some kind of progress
    if progress_func:
      progress_func(len(testnums), len(testnums))

    # terminate the line of dots
    print("")

    # all tests are finished, find out the result and print the logs.
    for (index, result, stdout_lines, stderr_lines) in results:
      if stdout_lines:
        for line in stdout_lines:
          sys.stdout.write(line)
      if stderr_lines:
        for line in stderr_lines:
          sys.stdout.write(line)
      if result == 1:
        exit_code = 1

  svntest.sandbox.cleanup_deferred_test_paths()
  return exit_code


def create_default_options():
  """Set the global options to the defaults, as provided by the argument
     parser."""
  _parse_options([])


def _create_parser():
  """Return a parser for our test suite."""
  # set up the parser
  _default_http_library = 'neon'
  usage = 'usage: %prog [options] [<test> ...]'
  parser = optparse.OptionParser(usage=usage)
  parser.add_option('-l', '--list', action='store_true', dest='list_tests',
                    help='Print test doc strings instead of running them')
  parser.add_option('--milestone-filter', action='store', dest='milestone_filter',
                    help='Limit --list to those with target milestone specified')
  parser.add_option('-v', '--verbose', action='store_true', dest='verbose',
                    help='Print binary command-lines (not with --quiet)')
  parser.add_option('-q', '--quiet', action='store_true',
                    help='Print only unexpected results (not with --verbose)')
  parser.add_option('-p', '--parallel', action='store_const',
                    const=default_num_threads, dest='parallel',
                    help='Run the tests in parallel')
  parser.add_option('-c', action='store_true', dest='is_child_process',
                    help='Flag if we are running this python test as a ' +
                         'child process')
  parser.add_option('--mode-filter', action='store', dest='mode_filter',
                    default='ALL',
                    help='Limit tests to those with type specified (e.g. XFAIL)')
  parser.add_option('--url', action='store',
                    help='Base url to the repos (e.g. svn://localhost)')
  parser.add_option('--fs-type', action='store',
                    help='Subversion file system type (fsfs or bdb)')
  parser.add_option('--cleanup', action='store_true',
                    help='Whether to clean up')
  parser.add_option('--enable-sasl', action='store_true',
                    help='Whether to enable SASL authentication')
  parser.add_option('--bin', action='store', dest='svn_bin',
                    help='Use the svn binaries installed in this path')
  parser.add_option('--use-jsvn', action='store_true',
                    help="Use the jsvn (SVNKit based) binaries. Can be " +
                         "combined with --bin to point to a specific path")
  parser.add_option('--http-library', action='store',
                    help="Make svn use this DAV library (neon or serf) if " +
                         "it supports both, else assume it's using this " +
                         "one; the default is " + _default_http_library)
  parser.add_option('--server-minor-version', type='int', action='store',
                    help="Set the minor version for the server ('4', " +
                         "'5', or '6').")
  parser.add_option('--fsfs-packing', action='store_true',
                    help="Run 'svnadmin pack' automatically")
  parser.add_option('--fsfs-sharding', action='store', type='int',
                    help='Default shard size (for fsfs)')
  parser.add_option('--config-file', action='store',
                    help="Configuration file for tests.")
  parser.add_option('--keep-local-tmp', action='store_true',
                    help="Don't remove svn-test-work/local_tmp after test " +
                         "run is complete.  Useful for debugging failures.")
  parser.add_option('--development', action='store_true',
                    help='Test development mode: provides more detailed ' +
                         'test output and ignores all exceptions in the ' +
                         'run_and_verify* functions. This option is only ' +
                         'useful during test development!')
  parser.add_option('--srcdir', action='store', dest='srcdir',
                    help='Source directory.')

  # most of the defaults are None, but some are other values, set them here
  parser.set_defaults(
        server_minor_version=7,
        url=file_scheme_prefix + pathname2url(os.path.abspath(os.getcwd())),
        http_library=_default_http_library)

  return parser


def _parse_options(arglist=sys.argv[1:]):
  """Parse the arguments in arg_list, and set the global options object with
     the results"""

  global options

  parser = _create_parser()
  (options, args) = parser.parse_args(arglist)

  # some sanity checking
  if options.verbose and options.quiet:
    parser.error("'verbose' and 'quiet' are incompatible")
  if options.fsfs_packing and not options.fsfs_sharding:
    parser.error("--fsfs-packing requires --fsfs-sharding")

  # If you change the below condition then change
  # ../../../../build/run_tests.py too.
  if options.server_minor_version < 3 or options.server_minor_version > 7:
    parser.error("test harness only supports server minor versions 3-7")

  if options.url:
    if options.url[-1:] == '/': # Normalize url to have no trailing slash
      options.test_area_url = options.url[:-1]
    else:
      options.test_area_url = options.url

  return (parser, args)


def run_tests(test_list, serial_only = False):
  """Main routine to run all tests in TEST_LIST.

  NOTE: this function does not return. It does a sys.exit() with the
        appropriate exit code.
  """

  sys.exit(execute_tests(test_list, serial_only))

def get_target_milestones_for_issues(issue_numbers):
  xml_url = "http://subversion.tigris.org/issues/xml.cgi?id="
  issue_dict = {}

  if isinstance(issue_numbers, int):
    issue_numbers = [str(issue_numbers)]
  elif isinstance(issue_numbers, str):
    issue_numbers = [issue_numbers]

  if issue_numbers is None or len(issue_numbers) == 0:
    return issue_dict

  for num in issue_numbers:
    xml_url += str(num) + ','
    issue_dict[str(num)] = 'unknown'

  try:
    # Parse the xml for ISSUE_NO from the issue tracker into a Document.
    issue_xml_f = urllib.urlopen(xml_url)
  except:
    print "WARNING: Unable to contact issue tracker; " \
          "milestones defaulting to 'unknown'."
    return issue_dict

  try:
    xmldoc = xml.dom.minidom.parse(issue_xml_f)
    issue_xml_f.close()

    # Get the target milestone for each issue.
    issue_element = xmldoc.getElementsByTagName('issue')
    for i in issue_element:
      issue_id_element = i.getElementsByTagName('issue_id')
      issue_id = issue_id_element[0].childNodes[0].nodeValue
      milestone_element = i.getElementsByTagName('target_milestone')
      milestone = milestone_element[0].childNodes[0].nodeValue
      issue_dict[issue_id] = milestone
  except:
    print "ERROR: Unable to parse target milestones from issue tracker"
    raise

  return issue_dict

# Main func.  This is the "entry point" that all the test scripts call
# to run their list of tests.
#
# This routine parses sys.argv to decide what to do.
def execute_tests(test_list, serial_only = False, test_name = None,
                  progress_func = None, test_selection = []):
  """Similar to run_tests(), but just returns the exit code, rather than
  exiting the process.  This function can be used when a caller doesn't
  want the process to die."""

  global pristine_url
  global pristine_greek_repos_url
  global svn_binary
  global svnadmin_binary
  global svnlook_binary
  global svnsync_binary
  global svndumpfilter_binary
  global svnversion_binary
  global svnmucc_binary
  global options

  if test_name:
    sys.argv[0] = test_name

  testnums = []

  if not options:
    # Override which tests to run from the commandline
    (parser, args) = _parse_options()
    test_selection = args
  else:
    parser = _create_parser()

  # parse the positional arguments (test nums, names)
  for arg in test_selection:
    appended = False
    try:
      testnums.append(int(arg))
      appended = True
    except ValueError:
      # Do nothing for now.
      appended = False

    if not appended:
      try:
        # Check if the argument is a range
        numberstrings = arg.split(':');
        if len(numberstrings) != 2:
          numberstrings = arg.split('-');
          if len(numberstrings) != 2:
            raise ValueError
        left = int(numberstrings[0])
        right = int(numberstrings[1])
        if left > right:
          raise ValueError

        for nr in range(left,right+1):
          testnums.append(nr)
        else:
          appended = True
      except ValueError:
        appended = False

    if not appended:
      try:
        # Check if the argument is a function name, and translate
        # it to a number if possible
        for testnum in list(range(1, len(test_list))):
          test_case = TestRunner(test_list[testnum], testnum)
          if test_case.get_function_name() == str(arg):
            testnums.append(testnum)
            appended = True
            break
      except ValueError:
        appended = False

    if not appended:
      parser.error("invalid test number, range of numbers, " +
                   "or function '%s'\n" % arg)

  # Calculate pristine_greek_repos_url from test_area_url.
  pristine_greek_repos_url = options.test_area_url + '/' + pathname2url(pristine_greek_repos_dir)

  if options.use_jsvn:
    if options.svn_bin is None:
      options.svn_bin = ''
    svn_binary = os.path.join(options.svn_bin, 'jsvn' + _bat)
    svnadmin_binary = os.path.join(options.svn_bin, 'jsvnadmin' + _bat)
    svnlook_binary = os.path.join(options.svn_bin, 'jsvnlook' + _bat)
    svnsync_binary = os.path.join(options.svn_bin, 'jsvnsync' + _bat)
    svndumpfilter_binary = os.path.join(options.svn_bin,
                                        'jsvndumpfilter' + _bat)
    svnversion_binary = os.path.join(options.svn_bin,
                                     'jsvnversion' + _bat)
    svnmucc_binary = os.path.join(options.svn_bin, 'jsvnmucc' + _bat)
  else:
    if options.svn_bin:
      svn_binary = os.path.join(options.svn_bin, 'svn' + _exe)
      svnadmin_binary = os.path.join(options.svn_bin, 'svnadmin' + _exe)
      svnlook_binary = os.path.join(options.svn_bin, 'svnlook' + _exe)
      svnsync_binary = os.path.join(options.svn_bin, 'svnsync' + _exe)
      svndumpfilter_binary = os.path.join(options.svn_bin,
                                          'svndumpfilter' + _exe)
      svnversion_binary = os.path.join(options.svn_bin, 'svnversion' + _exe)
      svnmucc_binary = os.path.join(options.svn_bin, 'svnmucc' + _exe)

  ######################################################################

  # Cleanup: if a previous run crashed or interrupted the python
  # interpreter, then `temp_dir' was never removed.  This can cause wonkiness.
  if not options.is_child_process:
    safe_rmtree(temp_dir, 1)

  if not testnums:
    # If no test numbers were listed explicitly, include all of them:
    testnums = list(range(1, len(test_list)))

  if options.list_tests:

    # If we want to list the target milestones, then get all the issues
    # associated with all the individual tests.
    milestones_dict = None
    if options.milestone_filter:
      issues_dict = {}
      for testnum in testnums:
        issues = TestRunner(test_list[testnum], testnum).get_issues()
        test_mode = TestRunner(test_list[testnum], testnum).get_mode().upper()
        if issues:
          for issue in issues:
            if (options.mode_filter.upper() == 'ALL' or
                options.mode_filter.upper() == test_mode or
                (options.mode_filter.upper() == 'PASS' and test_mode == '')):
              issues_dict[issue]=issue
      milestones_dict = get_target_milestones_for_issues(issues_dict.keys())

    header = "Test #  Mode   Test Description\n" \
             "------  -----  ----------------"
    printed_header = False
    for testnum in testnums:
      test_mode = TestRunner(test_list[testnum], testnum).get_mode().upper()
      if options.mode_filter.upper() == 'ALL' \
         or options.mode_filter.upper() == test_mode \
         or (options.mode_filter.upper() == 'PASS' and test_mode == ''):
        if not printed_header:
          print header
          printed_header = True
        TestRunner(test_list[testnum], testnum).list(milestones_dict)
    # We are simply listing the tests so always exit with success.
    return 0

  # don't run tests in parallel when the tests don't support it or there
  # are only a few tests to run.
  if serial_only or len(testnums) < 2:
    options.parallel = 0

  if not options.is_child_process:
    # Build out the default configuration directory
    create_config_dir(default_config_dir)

    # Setup the pristine repository
    svntest.actions.setup_pristine_greek_repository()

  # Run the tests.
  exit_code = _internal_run_tests(test_list, testnums, options.parallel,
                                  options.srcdir, progress_func)

  # Remove all scratchwork: the 'pristine' repository, greek tree, etc.
  # This ensures that an 'import' will happen the next time we run.
  if not options.is_child_process and not options.keep_local_tmp:
    safe_rmtree(temp_dir, 1)

  # Cleanup after ourselves.
  svntest.sandbox.cleanup_deferred_test_paths()

  # Return the appropriate exit code from the tests.
  return exit_code

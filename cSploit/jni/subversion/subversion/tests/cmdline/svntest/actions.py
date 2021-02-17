#
#  actions.py:  routines that actually run the svn client.
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

import os, shutil, re, sys, errno
import difflib, pprint
import xml.parsers.expat
from xml.dom.minidom import parseString

import svntest
from svntest import main, verify, tree, wc
from svntest import Failure

def no_sleep_for_timestamps():
  os.environ['SVN_I_LOVE_CORRUPTED_WORKING_COPIES_SO_DISABLE_SLEEP_FOR_TIMESTAMPS'] = 'yes'

def do_sleep_for_timestamps():
  os.environ['SVN_I_LOVE_CORRUPTED_WORKING_COPIES_SO_DISABLE_SLEEP_FOR_TIMESTAMPS'] = 'no'

def no_relocate_validation():
  os.environ['SVN_I_LOVE_CORRUPTED_WORKING_COPIES_SO_DISABLE_RELOCATE_VALIDATION'] = 'yes'

def do_relocate_validation():
  os.environ['SVN_I_LOVE_CORRUPTED_WORKING_COPIES_SO_DISABLE_RELOCATE_VALIDATION'] = 'no'

def setup_pristine_greek_repository():
  """Create the pristine repository and 'svn import' the greek tree"""

  # these directories don't exist out of the box, so we may have to create them
  if not os.path.exists(main.general_wc_dir):
    os.makedirs(main.general_wc_dir)

  if not os.path.exists(main.general_repo_dir):
    os.makedirs(main.general_repo_dir) # this also creates all the intermediate dirs

  # If there's no pristine repos, create one.
  if not os.path.exists(main.pristine_greek_repos_dir):
    main.create_repos(main.pristine_greek_repos_dir)

    # if this is dav, gives us access rights to import the greek tree.
    if main.is_ra_type_dav():
      authz_file = os.path.join(main.work_dir, "authz")
      main.file_write(authz_file, "[/]\n* = rw\n")

    # dump the greek tree to disk.
    main.greek_state.write_to_disk(main.greek_dump_dir)

    # import the greek tree, using l:foo/p:bar
    ### todo: svn should not be prompting for auth info when using
    ### repositories with no auth/auth requirements
    exit_code, output, errput = main.run_svn(None, 'import', '-m',
                                             'Log message for revision 1.',
                                             main.greek_dump_dir,
                                             main.pristine_greek_repos_url)

    # check for any errors from the import
    if len(errput):
      display_lines("Errors during initial 'svn import':",
                    'STDERR', None, errput)
      sys.exit(1)

    # verify the printed output of 'svn import'.
    lastline = output.pop().strip()
    match = re.search("(Committed|Imported) revision [0-9]+.", lastline)
    if not match:
      print("ERROR:  import did not succeed, while creating greek repos.")
      print("The final line from 'svn import' was:")
      print(lastline)
      sys.exit(1)
    output_tree = wc.State.from_commit(output)

    expected_output_tree = main.greek_state.copy(main.greek_dump_dir)
    expected_output_tree.tweak(verb='Adding',
                               contents=None)

    try:
      expected_output_tree.compare_and_display('output', output_tree)
    except tree.SVNTreeUnequal:
      verify.display_trees("ERROR:  output of import command is unexpected.",
                           "OUTPUT TREE",
                           expected_output_tree.old_tree(),
                           output_tree.old_tree())
      sys.exit(1)

    # Finally, disallow any changes to the "pristine" repos.
    error_msg = "Don't modify the pristine repository"
    create_failing_hook(main.pristine_greek_repos_dir, 'start-commit', error_msg)
    create_failing_hook(main.pristine_greek_repos_dir, 'pre-lock', error_msg)
    create_failing_hook(main.pristine_greek_repos_dir, 'pre-revprop-change', error_msg)


######################################################################

def guarantee_empty_repository(path):
  """Guarantee that a local svn repository exists at PATH, containing
  nothing."""

  if path == main.pristine_greek_repos_dir:
    print("ERROR:  attempt to overwrite the pristine repos!  Aborting.")
    sys.exit(1)

  # create an empty repository at PATH.
  main.safe_rmtree(path)
  main.create_repos(path)

# Used by every test, so that they can run independently of  one
# another. Every time this routine is called, it recursively copies
# the `pristine repos' to a new location.
# Note: make sure setup_pristine_greek_repository was called once before
# using this function.
def guarantee_greek_repository(path):
  """Guarantee that a local svn repository exists at PATH, containing
  nothing but the greek-tree at revision 1."""

  if path == main.pristine_greek_repos_dir:
    print("ERROR:  attempt to overwrite the pristine repos!  Aborting.")
    sys.exit(1)

  # copy the pristine repository to PATH.
  main.safe_rmtree(path)
  if main.copy_repos(main.pristine_greek_repos_dir, path, 1):
    print("ERROR:  copying repository failed.")
    sys.exit(1)

  # make the repos world-writeable, for mod_dav_svn's sake.
  main.chmod_tree(path, 0666, 0666)


def run_and_verify_atomic_ra_revprop_change(message,
                                            expected_stdout,
                                            expected_stderr,
                                            expected_exit,
                                            url, revision, propname,
                                            old_propval, propval,
                                            want_error):
  """Run atomic-ra-revprop-change helper and check its output and exit code.
  Transforms OLD_PROPVAL and PROPVAL into a skel.
  For HTTP, the default HTTP library is used."""

  KEY_OLD_PROPVAL = "old_value_p"
  KEY_NEW_PROPVAL = "value"

  def skel_make_atom(word):
    return "%d %s" % (len(word), word)

  def make_proplist_skel_part(nick, val):
    if val is None:
      return ""
    else:
      return "%s %s" % (skel_make_atom(nick), skel_make_atom(val))

  skel = "( %s %s )" % (make_proplist_skel_part(KEY_OLD_PROPVAL, old_propval),
                        make_proplist_skel_part(KEY_NEW_PROPVAL, propval))

  exit_code, out, err = main.run_atomic_ra_revprop_change(url, revision,
                                                          propname, skel,
                                                          want_error)
  verify.verify_outputs("Unexpected output", out, err,
                        expected_stdout, expected_stderr)
  verify.verify_exit_code(message, exit_code, expected_exit)
  return exit_code, out, err


def run_and_verify_svnlook(message, expected_stdout,
                           expected_stderr, *varargs):
  """Like run_and_verify_svnlook2, but the expected exit code is
  assumed to be 0 if no output is expected on stderr, and 1 otherwise."""

  expected_exit = 0
  if expected_stderr is not None and expected_stderr != []:
    expected_exit = 1
  return run_and_verify_svnlook2(message, expected_stdout, expected_stderr,
                                 expected_exit, *varargs)

def run_and_verify_svnlook2(message, expected_stdout, expected_stderr,
                            expected_exit, *varargs):
  """Run svnlook command and check its output and exit code."""

  exit_code, out, err = main.run_svnlook(*varargs)
  verify.verify_outputs("Unexpected output", out, err,
                        expected_stdout, expected_stderr)
  verify.verify_exit_code(message, exit_code, expected_exit)
  return exit_code, out, err


def run_and_verify_svnadmin(message, expected_stdout,
                            expected_stderr, *varargs):
  """Like run_and_verify_svnadmin2, but the expected exit code is
  assumed to be 0 if no output is expected on stderr, and 1 otherwise."""

  expected_exit = 0
  if expected_stderr is not None and expected_stderr != []:
    expected_exit = 1
  return run_and_verify_svnadmin2(message, expected_stdout, expected_stderr,
                                  expected_exit, *varargs)

def run_and_verify_svnadmin2(message, expected_stdout, expected_stderr,
                             expected_exit, *varargs):
  """Run svnadmin command and check its output and exit code."""

  exit_code, out, err = main.run_svnadmin(*varargs)
  verify.verify_outputs("Unexpected output", out, err,
                        expected_stdout, expected_stderr)
  verify.verify_exit_code(message, exit_code, expected_exit)
  return exit_code, out, err


def run_and_verify_svnversion(message, wc_dir, trail_url,
                              expected_stdout, expected_stderr, *varargs):
  """like run_and_verify_svnversion2, but the expected exit code is
  assumed to be 0 if no output is expected on stderr, and 1 otherwise."""

  expected_exit = 0
  if expected_stderr is not None and expected_stderr != []:
    expected_exit = 1
  return run_and_verify_svnversion2(message, wc_dir, trail_url,
                                    expected_stdout, expected_stderr,
                                    expected_exit, *varargs)

def run_and_verify_svnversion2(message, wc_dir, trail_url,
                               expected_stdout, expected_stderr,
                               expected_exit, *varargs):
  """Run svnversion command and check its output and exit code."""

  if trail_url is None:
    exit_code, out, err = main.run_svnversion(wc_dir, *varargs)
  else:
    exit_code, out, err = main.run_svnversion(wc_dir, trail_url, *varargs)

  verify.verify_outputs("Unexpected output", out, err,
                        expected_stdout, expected_stderr)
  verify.verify_exit_code(message, exit_code, expected_exit)
  return exit_code, out, err

def run_and_verify_svn(message, expected_stdout, expected_stderr, *varargs):
  """like run_and_verify_svn2, but the expected exit code is assumed to
  be 0 if no output is expected on stderr, and 1 otherwise."""

  expected_exit = 0
  if expected_stderr is not None:
    if isinstance(expected_stderr, verify.ExpectedOutput):
      if not expected_stderr.matches([]):
        expected_exit = 1
    elif expected_stderr != []:
      expected_exit = 1
  return run_and_verify_svn2(message, expected_stdout, expected_stderr,
                             expected_exit, *varargs)

def run_and_verify_svn2(message, expected_stdout, expected_stderr,
                        expected_exit, *varargs):
  """Invoke main.run_svn() with *VARARGS. Return exit code as int; stdout,
  stderr as lists of lines (including line terminators).  For both
  EXPECTED_STDOUT and EXPECTED_STDERR, create an appropriate instance of
  verify.ExpectedOutput (if necessary):

     - If it is an array of strings, create a vanilla ExpectedOutput.

     - If it is a single string, create a RegexOutput that must match every
       line (for stdout) or any line (for stderr) of the expected output.

     - If it is already an instance of ExpectedOutput
       (e.g. UnorderedOutput), leave it alone.

  ...and invoke compare_and_display_lines() on MESSAGE, a label based
  on the name of the stream being compared (e.g. STDOUT), the
  ExpectedOutput instance, and the actual output.

  If EXPECTED_STDOUT is None, do not check stdout.
  EXPECTED_STDERR may not be None.

  If output checks pass, the expected and actual codes are compared.

  If a comparison fails, a Failure will be raised."""

  if expected_stderr is None:
    raise verify.SVNIncorrectDatatype("expected_stderr must not be None")

  want_err = None
  if isinstance(expected_stderr, verify.ExpectedOutput):
    if not expected_stderr.matches([]):
      want_err = True
  elif expected_stderr != []:
    want_err = True

  exit_code, out, err = main.run_svn(want_err, *varargs)
  verify.verify_outputs(message, out, err, expected_stdout, expected_stderr)
  verify.verify_exit_code(message, exit_code, expected_exit)
  return exit_code, out, err

def run_and_verify_load(repo_dir, dump_file_content,
                        bypass_prop_validation = False):
  "Runs 'svnadmin load' and reports any errors."
  if not isinstance(dump_file_content, list):
    raise TypeError("dump_file_content argument should have list type")
  expected_stderr = []
  if bypass_prop_validation:
    exit_code, output, errput = main.run_command_stdin(
      main.svnadmin_binary, expected_stderr, 0, 1, dump_file_content,
      'load', '--force-uuid', '--quiet', '--bypass-prop-validation', repo_dir)
  else:
    exit_code, output, errput = main.run_command_stdin(
      main.svnadmin_binary, expected_stderr, 0, 1, dump_file_content,
      'load', '--force-uuid', '--quiet', repo_dir)

  verify.verify_outputs("Unexpected stderr output", None, errput,
                        None, expected_stderr)


def run_and_verify_dump(repo_dir, deltas=False):
  "Runs 'svnadmin dump' and reports any errors, returning the dump content."
  if deltas:
    exit_code, output, errput = main.run_svnadmin('dump', '--deltas',
                                                  repo_dir)
  else:
    exit_code, output, errput = main.run_svnadmin('dump', repo_dir)
  verify.verify_outputs("Missing expected output(s)", output, errput,
                        verify.AnyOutput, verify.AnyOutput)
  return output


def run_and_verify_svnrdump(dumpfile_content, expected_stdout,
                            expected_stderr, expected_exit, *varargs):
  """Runs 'svnrdump dump|load' depending on dumpfile_content and
  reports any errors."""
  exit_code, output, err = main.run_svnrdump(dumpfile_content, *varargs)

  # Since main.run_svnrdump() uses binary mode, normalize the stderr
  # line endings on Windows ourselves.
  if sys.platform == 'win32':
    err = map(lambda x : x.replace('\r\n', '\n'), err)

  for index, line in enumerate(err[:]):
    if re.search("warning: W200007", line):
      del err[index]

  verify.verify_outputs("Unexpected output", output, err,
                        expected_stdout, expected_stderr)
  verify.verify_exit_code("Unexpected return code", exit_code, expected_exit)
  return output


def run_and_verify_svnmucc(message, expected_stdout, expected_stderr,
                           *varargs):
  """Run svnmucc command and check its output"""

  expected_exit = 0
  if expected_stderr is not None and expected_stderr != []:
    expected_exit = 1
  return run_and_verify_svnmucc2(message, expected_stdout, expected_stderr,
                                 expected_exit, *varargs)

def run_and_verify_svnmucc2(message, expected_stdout, expected_stderr,
                            expected_exit, *varargs):
  """Run svnmucc command and check its output and exit code."""

  exit_code, out, err = main.run_svnmucc(*varargs)
  verify.verify_outputs("Unexpected output", out, err,
                        expected_stdout, expected_stderr)
  verify.verify_exit_code(message, exit_code, expected_exit)
  return exit_code, out, err


def load_repo(sbox, dumpfile_path = None, dump_str = None,
              bypass_prop_validation = False):
  "Loads the dumpfile into sbox"
  if not dump_str:
    dump_str = open(dumpfile_path, "rb").read()

  # Create a virgin repos and working copy
  main.safe_rmtree(sbox.repo_dir, 1)
  main.safe_rmtree(sbox.wc_dir, 1)
  main.create_repos(sbox.repo_dir)

  # Load the mergetracking dumpfile into the repos, and check it out the repo
  run_and_verify_load(sbox.repo_dir, dump_str.splitlines(True),
                      bypass_prop_validation)
  run_and_verify_svn(None, None, [], "co", sbox.repo_url, sbox.wc_dir)

  return dump_str

def expected_noop_update_output(rev):
  """Return an ExpectedOutput object describing what we'd expect to
  see from an update to revision REV that was effectively a no-op (no
  server changes transmitted)."""
  return verify.createExpectedOutput("Updating '.*':|At revision %d."
                                     % (rev),
                                     "no-op update")

######################################################################
# Subversion Actions
#
# These are all routines that invoke 'svn' in particular ways, and
# then verify the results by comparing expected trees with actual
# trees.
#


def run_and_verify_checkout2(do_remove,
                             URL, wc_dir_name, output_tree, disk_tree,
                             singleton_handler_a = None,
                             a_baton = None,
                             singleton_handler_b = None,
                             b_baton = None,
                             *args):
  """Checkout the URL into a new directory WC_DIR_NAME. *ARGS are any
  extra optional args to the checkout subcommand.

  The subcommand output will be verified against OUTPUT_TREE,
  and the working copy itself will be verified against DISK_TREE.
  For the latter comparison, SINGLETON_HANDLER_A and
  SINGLETON_HANDLER_B will be passed to tree.compare_trees -- see that
  function's doc string for more details.  Return if successful, raise
  on failure.

  WC_DIR_NAME is deleted if DO_REMOVE is True.
  """

  if isinstance(output_tree, wc.State):
    output_tree = output_tree.old_tree()
  if isinstance(disk_tree, wc.State):
    disk_tree = disk_tree.old_tree()

  # Remove dir if it's already there, unless this is a forced checkout.
  # In that case assume we want to test a forced checkout's toleration
  # of obstructing paths.
  if do_remove:
    main.safe_rmtree(wc_dir_name)

  # Checkout and make a tree of the output, using l:foo/p:bar
  ### todo: svn should not be prompting for auth info when using
  ### repositories with no auth/auth requirements
  exit_code, output, errput = main.run_svn(None, 'co',
                                           URL, wc_dir_name, *args)
  actual = tree.build_tree_from_checkout(output)

  # Verify actual output against expected output.
  try:
    tree.compare_trees("output", actual, output_tree)
  except tree.SVNTreeUnequal:
    print("ACTUAL OUTPUT TREE:")
    tree.dump_tree_script(actual, wc_dir_name + os.sep)
    raise

  # Create a tree by scanning the working copy
  actual = tree.build_tree_from_wc(wc_dir_name)

  # Verify expected disk against actual disk.
  try:
    tree.compare_trees("disk", actual, disk_tree,
                       singleton_handler_a, a_baton,
                       singleton_handler_b, b_baton)
  except tree.SVNTreeUnequal:
    print("ACTUAL DISK TREE:")
    tree.dump_tree_script(actual, wc_dir_name + os.sep)
    raise

def run_and_verify_checkout(URL, wc_dir_name, output_tree, disk_tree,
                            singleton_handler_a = None,
                            a_baton = None,
                            singleton_handler_b = None,
                            b_baton = None,
                            *args):
  """Same as run_and_verify_checkout2(), but without the DO_REMOVE arg.
  WC_DIR_NAME is deleted if present unless the '--force' option is passed
  in *ARGS."""


  # Remove dir if it's already there, unless this is a forced checkout.
  # In that case assume we want to test a forced checkout's toleration
  # of obstructing paths.
  return run_and_verify_checkout2(('--force' not in args),
                                  URL, wc_dir_name, output_tree, disk_tree,
                                  singleton_handler_a,
                                  a_baton,
                                  singleton_handler_b,
                                  b_baton,
                                  *args)


def run_and_verify_export(URL, export_dir_name, output_tree, disk_tree,
                          *args):
  """Export the URL into a new directory WC_DIR_NAME.

  The subcommand output will be verified against OUTPUT_TREE,
  and the exported copy itself will be verified against DISK_TREE.
  Return if successful, raise on failure.
  """
  assert isinstance(output_tree, wc.State)
  assert isinstance(disk_tree, wc.State)

  disk_tree = disk_tree.old_tree()
  output_tree = output_tree.old_tree()

  # Export and make a tree of the output, using l:foo/p:bar
  ### todo: svn should not be prompting for auth info when using
  ### repositories with no auth/auth requirements
  exit_code, output, errput = main.run_svn(None, 'export',
                                           URL, export_dir_name, *args)
  actual = tree.build_tree_from_checkout(output)

  # Verify actual output against expected output.
  try:
    tree.compare_trees("output", actual, output_tree)
  except tree.SVNTreeUnequal:
    print("ACTUAL OUTPUT TREE:")
    tree.dump_tree_script(actual, export_dir_name + os.sep)
    raise

  # Create a tree by scanning the working copy.  Don't ignore
  # the .svn directories so that we generate an error if they
  # happen to show up.
  actual = tree.build_tree_from_wc(export_dir_name, ignore_svn=False)

  # Verify expected disk against actual disk.
  try:
    tree.compare_trees("disk", actual, disk_tree)
  except tree.SVNTreeUnequal:
    print("ACTUAL DISK TREE:")
    tree.dump_tree_script(actual, export_dir_name + os.sep)
    raise


# run_and_verify_log_xml

class LogEntry:
  def __init__(self, revision, changed_paths=None, revprops=None):
    self.revision = revision
    if changed_paths == None:
      self.changed_paths = {}
    else:
      self.changed_paths = changed_paths
    if revprops == None:
      self.revprops = {}
    else:
      self.revprops = revprops

  def assert_changed_paths(self, changed_paths):
    """Assert that changed_paths is the same as this entry's changed_paths
    Raises svntest.Failure if not.
    """
    raise Failure('NOT IMPLEMENTED')

  def assert_revprops(self, revprops):
    """Assert that the dict revprops is the same as this entry's revprops.

    Raises svntest.Failure if not.
    """
    if self.revprops != revprops:
      raise Failure('\n' + '\n'.join(difflib.ndiff(
            pprint.pformat(revprops).splitlines(),
            pprint.pformat(self.revprops).splitlines())))

class LogParser:
  def parse(self, data):
    """Return a list of LogEntrys parsed from the sequence of strings data.

    This is the only method of interest to callers.
    """
    try:
      for i in data:
        self.parser.Parse(i)
      self.parser.Parse('', True)
    except xml.parsers.expat.ExpatError, e:
      raise verify.SVNUnexpectedStdout('%s\n%s\n' % (e, ''.join(data),))
    return self.entries

  def __init__(self):
    # for expat
    self.parser = xml.parsers.expat.ParserCreate()
    self.parser.StartElementHandler = self.handle_start_element
    self.parser.EndElementHandler = self.handle_end_element
    self.parser.CharacterDataHandler = self.handle_character_data
    # Ignore some things.
    self.ignore_elements('log', 'paths', 'path', 'revprops')
    self.ignore_tags('logentry_end', 'author_start', 'date_start', 'msg_start')
    # internal state
    self.cdata = []
    self.property = None
    # the result
    self.entries = []

  def ignore(self, *args, **kwargs):
    del self.cdata[:]
  def ignore_tags(self, *args):
    for tag in args:
      setattr(self, tag, self.ignore)
  def ignore_elements(self, *args):
    for element in args:
      self.ignore_tags(element + '_start', element + '_end')

  # expat handlers
  def handle_start_element(self, name, attrs):
    getattr(self, name + '_start')(attrs)
  def handle_end_element(self, name):
    getattr(self, name + '_end')()
  def handle_character_data(self, data):
    self.cdata.append(data)

  # element handler utilities
  def use_cdata(self):
    result = ''.join(self.cdata).strip()
    del self.cdata[:]
    return result
  def svn_prop(self, name):
    self.entries[-1].revprops['svn:' + name] = self.use_cdata()

  # element handlers
  def logentry_start(self, attrs):
    self.entries.append(LogEntry(int(attrs['revision'])))
  def author_end(self):
    self.svn_prop('author')
  def msg_end(self):
    self.svn_prop('log')
  def date_end(self):
    # svn:date could be anything, so just note its presence.
    self.cdata[:] = ['']
    self.svn_prop('date')
  def property_start(self, attrs):
    self.property = attrs['name']
  def property_end(self):
    self.entries[-1].revprops[self.property] = self.use_cdata()

def run_and_verify_log_xml(message=None, expected_paths=None,
                           expected_revprops=None, expected_stdout=None,
                           expected_stderr=None, args=[]):
  """Call run_and_verify_svn with log --xml and args (optional) as command
  arguments, and pass along message, expected_stdout, and expected_stderr.

  If message is None, pass the svn log command as message.

  expected_paths checking is not yet implemented.

  expected_revprops is an optional list of dicts, compared to each
  revision's revprops.  The list must be in the same order the log entries
  come in.  Any svn:date revprops in the dicts must be '' in order to
  match, as the actual dates could be anything.

  expected_paths and expected_revprops are ignored if expected_stdout or
  expected_stderr is specified.
  """
  if message == None:
    message = ' '.join(args)

  # We'll parse the output unless the caller specifies expected_stderr or
  # expected_stdout for run_and_verify_svn.
  parse = True
  if expected_stderr == None:
    expected_stderr = []
  else:
    parse = False
  if expected_stdout != None:
    parse = False

  log_args = list(args)
  if expected_paths != None:
    log_args.append('-v')

  (exit_code, stdout, stderr) = run_and_verify_svn(
    message, expected_stdout, expected_stderr,
    'log', '--xml', *log_args)
  if not parse:
    return

  entries = LogParser().parse(stdout)
  for index in range(len(entries)):
    entry = entries[index]
    if expected_revprops != None:
      entry.assert_revprops(expected_revprops[index])
    if expected_paths != None:
      entry.assert_changed_paths(expected_paths[index])


def verify_update(actual_output,
                  actual_mergeinfo_output,
                  actual_elision_output,
                  wc_dir_name,
                  output_tree,
                  mergeinfo_output_tree,
                  elision_output_tree,
                  disk_tree,
                  status_tree,
                  singleton_handler_a=None,
                  a_baton=None,
                  singleton_handler_b=None,
                  b_baton=None,
                  check_props=False):
  """Verify update of WC_DIR_NAME.

  The subcommand output (found in ACTUAL_OUTPUT, ACTUAL_MERGEINFO_OUTPUT,
  and ACTUAL_ELISION_OUTPUT) will be verified against OUTPUT_TREE,
  MERGEINFO_OUTPUT_TREE, and ELISION_OUTPUT_TREE respectively (if any of
  these is provided, they may be None in which case a comparison is not
  done).  The working copy itself will be verified against DISK_TREE (if
  provided), and the working copy's 'svn status' output will be verified
  against STATUS_TREE (if provided).  (This is a good way to check that
  revision numbers were bumped.)

  Return if successful, raise on failure.

  For the comparison with DISK_TREE, pass SINGLETON_HANDLER_A and
  SINGLETON_HANDLER_B to tree.compare_trees -- see that function's doc
  string for more details.  If CHECK_PROPS is set, then disk
  comparison will examine props."""

  if isinstance(actual_output, wc.State):
    actual_output = actual_output.old_tree()
  if isinstance(actual_mergeinfo_output, wc.State):
    actual_mergeinfo_output = actual_mergeinfo_output.old_tree()
  if isinstance(actual_elision_output, wc.State):
    actual_elision_output = actual_elision_output.old_tree()
  if isinstance(output_tree, wc.State):
    output_tree = output_tree.old_tree()
  if isinstance(mergeinfo_output_tree, wc.State):
    mergeinfo_output_tree = mergeinfo_output_tree.old_tree()
  if isinstance(elision_output_tree, wc.State):
    elision_output_tree = elision_output_tree.old_tree()
  if isinstance(disk_tree, wc.State):
    disk_tree = disk_tree.old_tree()
  if isinstance(status_tree, wc.State):
    status_tree = status_tree.old_tree()

  # Verify actual output against expected output.
  if output_tree:
    try:
      tree.compare_trees("output", actual_output, output_tree)
    except tree.SVNTreeUnequal:
      print("ACTUAL OUTPUT TREE:")
      tree.dump_tree_script(actual_output, wc_dir_name + os.sep)
      raise

  # Verify actual mergeinfo recording output against expected output.
  if mergeinfo_output_tree:
    try:
      tree.compare_trees("mergeinfo_output", actual_mergeinfo_output,
                         mergeinfo_output_tree)
    except tree.SVNTreeUnequal:
      print("ACTUAL MERGEINFO OUTPUT TREE:")
      tree.dump_tree_script(actual_mergeinfo_output,
                            wc_dir_name + os.sep)
      raise

  # Verify actual mergeinfo elision output against expected output.
  if elision_output_tree:
    try:
      tree.compare_trees("elision_output", actual_elision_output,
                         elision_output_tree)
    except tree.SVNTreeUnequal:
      print("ACTUAL ELISION OUTPUT TREE:")
      tree.dump_tree_script(actual_elision_output,
                            wc_dir_name + os.sep)
      raise

  # Create a tree by scanning the working copy, and verify it
  if disk_tree:
    actual_disk = tree.build_tree_from_wc(wc_dir_name, check_props)
    try:
      tree.compare_trees("disk", actual_disk, disk_tree,
                         singleton_handler_a, a_baton,
                         singleton_handler_b, b_baton)
    except tree.SVNTreeUnequal:
      print("EXPECTED DISK TREE:")
      tree.dump_tree_script(disk_tree)
      print("ACTUAL DISK TREE:")
      tree.dump_tree_script(actual_disk)
      raise

  # Verify via 'status' command too, if possible.
  if status_tree:
    run_and_verify_status(wc_dir_name, status_tree)


def verify_disk(wc_dir_name, disk_tree, check_props=False):
  """Verify WC_DIR_NAME against DISK_TREE.  If CHECK_PROPS is set,
  the comparison will examin props.  Returns if successful, raises on
  failure."""
  verify_update(None, None, None, wc_dir_name, None, None, None, disk_tree,
                None, check_props=check_props)



def run_and_verify_update(wc_dir_name,
                          output_tree, disk_tree, status_tree,
                          error_re_string = None,
                          singleton_handler_a = None,
                          a_baton = None,
                          singleton_handler_b = None,
                          b_baton = None,
                          check_props = False,
                          *args):

  """Update WC_DIR_NAME.  *ARGS are any extra optional args to the
  update subcommand.  NOTE: If *ARGS is specified at all, explicit
  target paths must be passed in *ARGS as well (or a default `.' will
  be chosen by the 'svn' binary).  This allows the caller to update
  many items in a single working copy dir, but still verify the entire
  working copy dir.

  If ERROR_RE_STRING, the update must exit with error, and the error
  message must match regular expression ERROR_RE_STRING.

  Else if ERROR_RE_STRING is None, then:

  If OUTPUT_TREE is not None, the subcommand output will be verified
  against OUTPUT_TREE.  If DISK_TREE is not None, the working copy
  itself will be verified against DISK_TREE.  If STATUS_TREE is not
  None, the 'svn status' output will be verified against STATUS_TREE.
  (This is a good way to check that revision numbers were bumped.)

  For the DISK_TREE verification, SINGLETON_HANDLER_A and
  SINGLETON_HANDLER_B will be passed to tree.compare_trees -- see that
  function's doc string for more details.

  If CHECK_PROPS is set, then disk comparison will examine props.

  Return if successful, raise on failure."""

  # Update and make a tree of the output.
  if len(args):
    exit_code, output, errput = main.run_svn(error_re_string, 'up', *args)
  else:
    exit_code, output, errput = main.run_svn(error_re_string,
                                             'up', wc_dir_name,
                                             *args)

  if error_re_string:
    rm = re.compile(error_re_string)
    for line in errput:
      match = rm.search(line)
      if match:
        return
    raise main.SVNUnmatchedError

  actual = wc.State.from_checkout(output)
  verify_update(actual, None, None, wc_dir_name,
                output_tree, None, None, disk_tree, status_tree,
                singleton_handler_a, a_baton,
                singleton_handler_b, b_baton,
                check_props)


def run_and_parse_info(*args):
  """Run 'svn info ARGS' and parse its output into a list of dicts,
  one dict per reported node."""

  # the returned array
  all_infos = []

  # per-target variables
  iter_info = {}
  prev_key = None
  lock_comment_lines = 0
  lock_comments = []

  exit_code, output, errput = main.run_svn(None, 'info', *args)

  for line in output:
    line = line[:-1] # trim '\n'

    if lock_comment_lines > 0:
      # mop up any lock comment lines
      lock_comments.append(line)
      lock_comment_lines = lock_comment_lines - 1
      if lock_comment_lines == 0:
        iter_info[prev_key] = lock_comments
    elif len(line) == 0:
      # separator line between items
      all_infos.append(iter_info)
      iter_info = {}
      prev_key = None
      lock_comment_lines = 0
      lock_comments = []
    elif line[0].isspace():
      # continuation line (for tree conflicts)
      iter_info[prev_key] += line[1:]
    else:
      # normal line
      key, value = line.split(':', 1)

      if re.search(' \(\d+ lines?\)$', key):
        # numbered continuation lines
        match = re.match('^(.*) \((\d+) lines?\)$', key)
        key = match.group(1)
        lock_comment_lines = int(match.group(2))
      elif len(value) > 1:
        # normal normal line
        iter_info[key] = value[1:]
      else:
        ### originally added for "Tree conflict:\n" lines;
        ### tree-conflicts output format has changed since then
        # continuation lines are implicit (prefixed by whitespace)
        iter_info[key] = ''
      prev_key = key

  return all_infos

def run_and_verify_info(expected_infos, *args):
  """Run 'svn info' with the arguments in *ARGS and verify the results
  against expected_infos.  The latter should be a list of dicts, one dict
  per reported node, in the order in which the 'Path' fields of the output
  will appear after sorting them as Python strings.  (The dicts in
  EXPECTED_INFOS, however, need not have a 'Path' key.)

  In the dicts, each key is the before-the-colon part of the 'svn info' output,
  and each value is either None (meaning that the key should *not* appear in
  the 'svn info' output) or a regex matching the output value.  Output lines
  not matching a key in the dict are ignored.

  Return if successful, raise on failure."""

  actual_infos = run_and_parse_info(*args)
  actual_infos.sort(key=lambda info: info['Path'])

  try:
    # zip() won't complain, so check this manually
    if len(actual_infos) != len(expected_infos):
      raise verify.SVNUnexpectedStdout(
          "Expected %d infos, found %d infos"
           % (len(expected_infos), len(actual_infos)))

    for actual, expected in zip(actual_infos, expected_infos):
      # compare dicts
      for key, value in expected.items():
        assert ':' not in key # caller passed impossible expectations?
        if value is None and key in actual:
          raise main.SVNLineUnequal("Found unexpected key '%s' with value '%s'"
                                    % (key, actual[key]))
        if value is not None and key not in actual:
          raise main.SVNLineUnequal("Expected key '%s' (with value '%s') "
                                    "not found" % (key, value))
        if value is not None and not re.match(value, actual[key]):
          raise verify.SVNUnexpectedStdout("Values of key '%s' don't match:\n"
                                           "  Expected: '%s' (regex)\n"
                                           "  Found:    '%s' (string)\n"
                                           % (key, value, actual[key]))

  except:
    sys.stderr.write("Bad 'svn info' output:\n"
                     "  Received: %s\n"
                     "  Expected: %s\n"
                     % (actual_infos, expected_infos))
    raise

def run_and_verify_merge(dir, rev1, rev2, url1, url2,
                         output_tree,
                         mergeinfo_output_tree,
                         elision_output_tree,
                         disk_tree, status_tree, skip_tree,
                         error_re_string = None,
                         singleton_handler_a = None,
                         a_baton = None,
                         singleton_handler_b = None,
                         b_baton = None,
                         check_props = False,
                         dry_run = True,
                         *args):
  """Run 'svn merge URL1@REV1 URL2@REV2 DIR' if URL2 is not None
  (for a three-way merge between URLs and WC).

  If URL2 is None, run 'svn merge -rREV1:REV2 URL1 DIR'.  If both REV1
  and REV2 are None, leave off the '-r' argument.

  If ERROR_RE_STRING, the merge must exit with error, and the error
  message must match regular expression ERROR_RE_STRING.

  Else if ERROR_RE_STRING is None, then:

  The subcommand output will be verified against OUTPUT_TREE.  Output
  related to mergeinfo notifications will be verified against
  MERGEINFO_OUTPUT_TREE if that is not None.  Output related to mergeinfo
  elision will be verified against ELISION_OUTPUT_TREE if that is not None.
  The working copy itself will be verified against DISK_TREE.  If optional
  STATUS_TREE is given, then 'svn status' output will be compared.  The
  'skipped' merge output will be compared to SKIP_TREE.

  For the DISK_TREE verification, SINGLETON_HANDLER_A and
  SINGLETON_HANDLER_B will be passed to tree.compare_trees -- see that
  function's doc string for more details.

  If CHECK_PROPS is set, then disk comparison will examine props.

  If DRY_RUN is set then a --dry-run merge will be carried out first and
  the output compared with that of the full merge.

  Return if successful, raise on failure.

  *ARGS are any extra optional args to the merge subcommand.
  NOTE: If *ARGS is specified at all, an explicit target path must be passed
  in *ARGS as well. This allows the caller to merge into single items inside
  the working copy, but still verify the entire working copy dir. """

  merge_command = [ "merge" ]
  if url2:
    merge_command.extend((url1 + "@" + str(rev1), url2 + "@" + str(rev2)))
  else:
    if not (rev1 is None and rev2 is None):
      merge_command.append("-r" + str(rev1) + ":" + str(rev2))
    merge_command.append(url1)
  if len(args) == 0:
    merge_command.append(dir)
  merge_command = tuple(merge_command)

  if dry_run:
    pre_disk = tree.build_tree_from_wc(dir)
    dry_run_command = merge_command + ('--dry-run',)
    dry_run_command = dry_run_command + args
    exit_code, out_dry, err_dry = main.run_svn(error_re_string,
                                               *dry_run_command)
    post_disk = tree.build_tree_from_wc(dir)
    try:
      tree.compare_trees("disk", post_disk, pre_disk)
    except tree.SVNTreeError:
      print("=============================================================")
      print("Dry-run merge altered working copy")
      print("=============================================================")
      raise


  # Update and make a tree of the output.
  merge_command = merge_command + args
  exit_code, out, err = main.run_svn(error_re_string, *merge_command)

  if error_re_string:
    if not error_re_string.startswith(".*"):
      error_re_string = ".*(" + error_re_string + ")"
    expected_err = verify.RegexOutput(error_re_string, match_all=False)
    verify.verify_outputs(None, None, err, None, expected_err)
    return
  elif err:
    raise verify.SVNUnexpectedStderr(err)

  # Split the output into that related to application of the actual diff
  # and that related to the recording of mergeinfo describing the merge.
  merge_diff_out = []
  mergeinfo_notification_out = []
  mergeinfo_elision_out = []
  mergeinfo_notifications = False
  elision_notifications = False
  for line in out:
    if line.startswith('--- Recording'):
      mergeinfo_notifications = True
      elision_notifications = False
    elif line.startswith('--- Eliding'):
      mergeinfo_notifications = False
      elision_notifications = True
    elif line.startswith('--- Merging')          or \
         line.startswith('--- Reverse-merging')  or \
         line.startswith('Summary of conflicts') or \
         line.startswith('Skipped missing target'):
      mergeinfo_notifications = False
      elision_notifications = False

    if mergeinfo_notifications:
      mergeinfo_notification_out.append(line)
    elif elision_notifications:
      mergeinfo_elision_out.append(line)
    else:
      merge_diff_out.append(line)

  if dry_run and merge_diff_out != out_dry:
    # Due to the way ra_serf works, it's possible that the dry-run and
    # real merge operations did the same thing, but the output came in
    # a different order.  Let's see if maybe that's the case by comparing
    # the outputs as unordered sets rather than as lists.
    #
    # This now happens for other RA layers with modern APR because the
    # hash order now varies.
    #
    # The different orders of the real and dry-run merges may cause
    # the "Merging rX through rY into" lines to be duplicated a
    # different number of times in the two outputs.  The list-set
    # conversion removes duplicates so these differences are ignored.
    # It also removes "U some/path" duplicate lines.  Perhaps we
    # should avoid that?
    out_copy = set(merge_diff_out[:])
    out_dry_copy = set(out_dry[:])

    if out_copy != out_dry_copy:
      print("=============================================================")
      print("Merge outputs differ")
      print("The dry-run merge output:")
      for x in out_dry:
        sys.stdout.write(x)
      print("The full merge output:")
      for x in out:
        sys.stdout.write(x)
      print("=============================================================")
      raise main.SVNUnmatchedError

  def missing_skip(a, b):
    print("=============================================================")
    print("Merge failed to skip: " + a.path)
    print("=============================================================")
    raise Failure
  def extra_skip(a, b):
    print("=============================================================")
    print("Merge unexpectedly skipped: " + a.path)
    print("=============================================================")
    raise Failure

  myskiptree = tree.build_tree_from_skipped(out)
  if isinstance(skip_tree, wc.State):
    skip_tree = skip_tree.old_tree()
  try:
    tree.compare_trees("skip", myskiptree, skip_tree,
                       extra_skip, None, missing_skip, None)
  except tree.SVNTreeUnequal:
    print("ACTUAL SKIP TREE:")
    tree.dump_tree_script(myskiptree, dir + os.sep)
    raise

  actual_diff = svntest.wc.State.from_checkout(merge_diff_out, False)
  actual_mergeinfo = svntest.wc.State.from_checkout(mergeinfo_notification_out,
                                                    False)
  actual_elision = svntest.wc.State.from_checkout(mergeinfo_elision_out,
                                                  False)
  verify_update(actual_diff, actual_mergeinfo, actual_elision, dir,
                output_tree, mergeinfo_output_tree, elision_output_tree,
                disk_tree, status_tree,
                singleton_handler_a, a_baton,
                singleton_handler_b, b_baton,
                check_props)


def run_and_verify_patch(dir, patch_path,
                         output_tree, disk_tree, status_tree, skip_tree,
                         error_re_string=None,
                         check_props=False,
                         dry_run=True,
                         *args):
  """Run 'svn patch patch_path DIR'.

  If ERROR_RE_STRING, 'svn patch' must exit with error, and the error
  message must match regular expression ERROR_RE_STRING.

  Else if ERROR_RE_STRING is None, then:

  The subcommand output will be verified against OUTPUT_TREE, and the
  working copy itself will be verified against DISK_TREE.  If optional
  STATUS_TREE is given, then 'svn status' output will be compared.
  The 'skipped' merge output will be compared to SKIP_TREE.

  If CHECK_PROPS is set, then disk comparison will examine props.

  If DRY_RUN is set then a --dry-run patch will be carried out first and
  the output compared with that of the full patch application.

  Returns if successful, raises on failure."""

  patch_command = [ "patch" ]
  patch_command.append(patch_path)
  patch_command.append(dir)
  patch_command = tuple(patch_command)

  if dry_run:
    pre_disk = tree.build_tree_from_wc(dir)
    dry_run_command = patch_command + ('--dry-run',)
    dry_run_command = dry_run_command + args
    exit_code, out_dry, err_dry = main.run_svn(error_re_string,
                                               *dry_run_command)
    post_disk = tree.build_tree_from_wc(dir)
    try:
      tree.compare_trees("disk", post_disk, pre_disk)
    except tree.SVNTreeError:
      print("=============================================================")
      print("'svn patch --dry-run' altered working copy")
      print("=============================================================")
      raise

  # Update and make a tree of the output.
  patch_command = patch_command + args
  exit_code, out, err = main.run_svn(True, *patch_command)

  if error_re_string:
    rm = re.compile(error_re_string)
    match = None
    for line in err:
      match = rm.search(line)
      if match:
        break
    if not match:
      raise main.SVNUnmatchedError
  elif err:
    print("UNEXPECTED STDERR:")
    for x in err:
      sys.stdout.write(x)
    raise verify.SVNUnexpectedStderr

  if dry_run and out != out_dry:
    # APR hash order means the output order can vary, assume everything is OK
    # if only the order changes.
    out_dry_expected = svntest.verify.UnorderedOutput(out)
    verify.compare_and_display_lines('dry-run patch output not as expected',
                                     '', out_dry_expected, out_dry)

  def missing_skip(a, b):
    print("=============================================================")
    print("'svn patch' failed to skip: " + a.path)
    print("=============================================================")
    raise Failure
  def extra_skip(a, b):
    print("=============================================================")
    print("'svn patch' unexpectedly skipped: " + a.path)
    print("=============================================================")
    raise Failure

  myskiptree = tree.build_tree_from_skipped(out)
  if isinstance(skip_tree, wc.State):
    skip_tree = skip_tree.old_tree()
  tree.compare_trees("skip", myskiptree, skip_tree,
                     extra_skip, None, missing_skip, None)

  mytree = tree.build_tree_from_checkout(out, 0)

  # when the expected output is a list, we want a line-by-line
  # comparison to happen instead of a tree comparison
  if (isinstance(output_tree, list)
      or isinstance(output_tree, verify.UnorderedOutput)):
    verify.verify_outputs(None, out, err, output_tree, error_re_string)
    output_tree = None

  verify_update(mytree, None, None, dir,
                output_tree, None, None, disk_tree, status_tree,
                check_props=check_props)


def run_and_verify_mergeinfo(error_re_string = None,
                             expected_output = [],
                             *args):
  """Run 'svn mergeinfo ARGS', and compare the result against
  EXPECTED_OUTPUT, a list of string representations of revisions
  expected in the output.  Raise an exception if an unexpected
  output is encountered."""

  mergeinfo_command = ["mergeinfo"]
  mergeinfo_command.extend(args)
  exit_code, out, err = main.run_svn(error_re_string, *mergeinfo_command)

  if error_re_string:
    if not error_re_string.startswith(".*"):
      error_re_string = ".*(" + error_re_string + ")"
    expected_err = verify.RegexOutput(error_re_string, match_all=False)
    verify.verify_outputs(None, None, err, None, expected_err)
    return

  out = sorted([_f for _f in [x.rstrip()[1:] for x in out] if _f])
  expected_output.sort()
  extra_out = []
  if out != expected_output:
    exp_hash = dict.fromkeys(expected_output)
    for rev in out:
      if rev in exp_hash:
        del(exp_hash[rev])
      else:
        extra_out.append(rev)
    extra_exp = list(exp_hash.keys())
    raise Exception("Unexpected 'svn mergeinfo' output:\n"
                    "  expected but not found: %s\n"
                    "  found but not expected: %s"
                    % (', '.join([str(x) for x in extra_exp]),
                       ', '.join([str(x) for x in extra_out])))


def run_and_verify_switch(wc_dir_name,
                          wc_target,
                          switch_url,
                          output_tree, disk_tree, status_tree,
                          error_re_string = None,
                          singleton_handler_a = None,
                          a_baton = None,
                          singleton_handler_b = None,
                          b_baton = None,
                          check_props = False,
                          *args):

  """Switch WC_TARGET (in working copy dir WC_DIR_NAME) to SWITCH_URL.

  If ERROR_RE_STRING, the switch must exit with error, and the error
  message must match regular expression ERROR_RE_STRING.

  Else if ERROR_RE_STRING is None, then:

  The subcommand output will be verified against OUTPUT_TREE, and the
  working copy itself will be verified against DISK_TREE.  If optional
  STATUS_TREE is given, then 'svn status' output will be
  compared.  (This is a good way to check that revision numbers were
  bumped.)

  For the DISK_TREE verification, SINGLETON_HANDLER_A and
  SINGLETON_HANDLER_B will be passed to tree.compare_trees -- see that
  function's doc string for more details.

  If CHECK_PROPS is set, then disk comparison will examine props.

  Return if successful, raise on failure."""

  # Update and make a tree of the output.
  exit_code, output, errput = main.run_svn(error_re_string, 'switch',
                                           switch_url, wc_target, *args)

  if error_re_string:
    if not error_re_string.startswith(".*"):
      error_re_string = ".*(" + error_re_string + ")"
    expected_err = verify.RegexOutput(error_re_string, match_all=False)
    verify.verify_outputs(None, None, errput, None, expected_err)
    return
  elif errput:
    raise verify.SVNUnexpectedStderr(err)

  actual = wc.State.from_checkout(output)

  verify_update(actual, None, None, wc_dir_name,
                output_tree, None, None, disk_tree, status_tree,
                singleton_handler_a, a_baton,
                singleton_handler_b, b_baton,
                check_props)

def process_output_for_commit(output):
  """Helper for run_and_verify_commit(), also used in the factory."""
  # Remove the final output line, and verify that the commit succeeded.
  lastline = ""
  rest = []

  def external_removal(line):
    return line.startswith('Removing external') \
           or line.startswith('Removed external')

  if len(output):
    lastline = output.pop().strip()

    while len(output) and external_removal(lastline):
      rest.append(lastline)
      lastline = output.pop().strip()

    cm = re.compile("(Committed|Imported) revision [0-9]+.")
    match = cm.search(lastline)
    if not match:
      print("ERROR:  commit did not succeed.")
      print("The final line from 'svn ci' was:")
      print(lastline)
      raise main.SVNCommitFailure

  # The new 'final' line in the output is either a regular line that
  # mentions {Adding, Deleting, Sending, ...}, or it could be a line
  # that says "Transmitting file data ...".  If the latter case, we
  # want to remove the line from the output; it should be ignored when
  # building a tree.
  if len(output):
    lastline = output.pop()

    tm = re.compile("Transmitting file data.+")
    match = tm.search(lastline)
    if not match:
      # whoops, it was important output, put it back.
      output.append(lastline)

  if len(rest):
    output.extend(rest)

  return output


def run_and_verify_commit(wc_dir_name, output_tree, status_tree,
                          error_re_string = None,
                          *args):
  """Commit and verify results within working copy WC_DIR_NAME,
  sending ARGS to the commit subcommand.

  The subcommand output will be verified against OUTPUT_TREE.  If
  optional STATUS_TREE is given, then 'svn status' output will
  be compared.  (This is a good way to check that revision numbers
  were bumped.)

  If ERROR_RE_STRING is None, the commit must not exit with error.  If
  ERROR_RE_STRING is a string, the commit must exit with error, and
  the error message must match regular expression ERROR_RE_STRING.

  Return if successful, raise on failure."""

  if isinstance(output_tree, wc.State):
    output_tree = output_tree.old_tree()
  if isinstance(status_tree, wc.State):
    status_tree = status_tree.old_tree()

  # Commit.
  if '-m' not in args and '-F' not in args:
    args = list(args) + ['-m', 'log msg']
  exit_code, output, errput = main.run_svn(error_re_string, 'ci',
                                           *args)

  if error_re_string:
    if not error_re_string.startswith(".*"):
      error_re_string = ".*(" + error_re_string + ")"
    expected_err = verify.RegexOutput(error_re_string, match_all=False)
    verify.verify_outputs(None, None, errput, None, expected_err)
    return

  # Else not expecting error:

  # Convert the output into a tree.
  output = process_output_for_commit(output)
  actual = tree.build_tree_from_commit(output)

  # Verify actual output against expected output.
  try:
    tree.compare_trees("output", actual, output_tree)
  except tree.SVNTreeError:
      verify.display_trees("Output of commit is unexpected",
                           "OUTPUT TREE", output_tree, actual)
      print("ACTUAL OUTPUT TREE:")
      tree.dump_tree_script(actual, wc_dir_name + os.sep)
      raise

  # Verify via 'status' command too, if possible.
  if status_tree:
    run_and_verify_status(wc_dir_name, status_tree)


# This function always passes '-q' to the status command, which
# suppresses the printing of any unversioned or nonexistent items.
def run_and_verify_status(wc_dir_name, output_tree,
                          singleton_handler_a = None,
                          a_baton = None,
                          singleton_handler_b = None,
                          b_baton = None):
  """Run 'status' on WC_DIR_NAME and compare it with the
  expected OUTPUT_TREE.  SINGLETON_HANDLER_A and SINGLETON_HANDLER_B will
  be passed to tree.compare_trees - see that function's doc string for
  more details.
  Returns on success, raises on failure."""

  if isinstance(output_tree, wc.State):
    output_state = output_tree
    output_tree = output_tree.old_tree()
  else:
    output_state = None

  exit_code, output, errput = main.run_svn(None, 'status', '-v', '-u', '-q',
                                           wc_dir_name)

  actual = tree.build_tree_from_status(output)

  # Verify actual output against expected output.
  try:
    tree.compare_trees("status", actual, output_tree,
                       singleton_handler_a, a_baton,
                       singleton_handler_b, b_baton)
  except tree.SVNTreeError:
    verify.display_trees(None, 'STATUS OUTPUT TREE', output_tree, actual)
    print("ACTUAL STATUS TREE:")
    tree.dump_tree_script(actual, wc_dir_name + os.sep)
    raise

  # if we have an output State, and we can/are-allowed to create an
  # entries-based State, then compare the two.
  if output_state:
    entries_state = wc.State.from_entries(wc_dir_name)
    if entries_state:
      tweaked = output_state.copy()
      tweaked.tweak_for_entries_compare()
      try:
        tweaked.compare_and_display('entries', entries_state)
      except tree.SVNTreeUnequal:
        ### do something more
        raise


# A variant of previous func, but doesn't pass '-q'.  This allows us
# to verify unversioned or nonexistent items in the list.
def run_and_verify_unquiet_status(wc_dir_name, status_tree):
  """Run 'status' on WC_DIR_NAME and compare it with the
  expected STATUS_TREE.
  Returns on success, raises on failure."""

  if isinstance(status_tree, wc.State):
    status_tree = status_tree.old_tree()

  exit_code, output, errput = main.run_svn(None, 'status', '-v',
                                           '-u', wc_dir_name)

  actual = tree.build_tree_from_status(output)

  # Verify actual output against expected output.
  try:
    tree.compare_trees("UNQUIET STATUS", actual, status_tree)
  except tree.SVNTreeError:
    print("ACTUAL UNQUIET STATUS TREE:")
    tree.dump_tree_script(actual, wc_dir_name + os.sep)
    raise

def run_and_verify_status_xml(expected_entries = [],
                              *args):
  """ Run 'status --xml' with arguments *ARGS.  If successful the output
  is parsed into an XML document and will be verified by comparing against
  EXPECTED_ENTRIES.
  """

  exit_code, output, errput = run_and_verify_svn(None, None, [],
                                                 'status', '--xml', *args)

  if len(errput) > 0:
    raise Failure

  doc = parseString(''.join(output))
  entries = doc.getElementsByTagName('entry')

  def getText(nodelist):
    rc = []
    for node in nodelist:
        if node.nodeType == node.TEXT_NODE:
            rc.append(node.data)
    return ''.join(rc)

  actual_entries = {}
  for entry in entries:
    wcstatus = entry.getElementsByTagName('wc-status')[0]
    commit = entry.getElementsByTagName('commit')
    author = entry.getElementsByTagName('author')
    rstatus = entry.getElementsByTagName('repos-status')

    actual_entry = {'wcprops' : wcstatus.getAttribute('props'),
                    'wcitem' : wcstatus.getAttribute('item'),
                    }
    if wcstatus.hasAttribute('revision'):
      actual_entry['wcrev'] = wcstatus.getAttribute('revision')
    if (commit):
      actual_entry['crev'] = commit[0].getAttribute('revision')
    if (author):
      actual_entry['author'] = getText(author[0].childNodes)
    if (rstatus):
      actual_entry['rprops'] = rstatus[0].getAttribute('props')
      actual_entry['ritem'] = rstatus[0].getAttribute('item')

    actual_entries[entry.getAttribute('path')] = actual_entry

  if expected_entries != actual_entries:
    raise Failure('\n' + '\n'.join(difflib.ndiff(
          pprint.pformat(expected_entries).splitlines(),
          pprint.pformat(actual_entries).splitlines())))

def run_and_verify_diff_summarize_xml(error_re_string = [],
                                      expected_prefix = None,
                                      expected_paths = [],
                                      expected_items = [],
                                      expected_props = [],
                                      expected_kinds = [],
                                      *args):
  """Run 'diff --summarize --xml' with the arguments *ARGS, which should
  contain all arguments beyond for your 'diff --summarize --xml' omitting
  said arguments.  EXPECTED_PREFIX will store a "common" path prefix
  expected to be at the beginning of each summarized path.  If
  EXPECTED_PREFIX is None, then EXPECTED_PATHS will need to be exactly
  as 'svn diff --summarize --xml' will output.  If ERROR_RE_STRING, the
  command must exit with error, and the error message must match regular
  expression ERROR_RE_STRING.

  Else if ERROR_RE_STRING is None, the subcommand output will be parsed
  into an XML document and will then be verified by comparing the parsed
  output to the contents in the EXPECTED_PATHS, EXPECTED_ITEMS,
  EXPECTED_PROPS and EXPECTED_KINDS. Returns on success, raises
  on failure."""

  exit_code, output, errput = run_and_verify_svn(None, None, error_re_string,
                                                 'diff', '--summarize',
                                                 '--xml', *args)


  # Return if errors are present since they were expected
  if len(errput) > 0:
    return

  doc = parseString(''.join(output))
  paths = doc.getElementsByTagName("path")
  items = expected_items
  kinds = expected_kinds

  for path in paths:
    modified_path = path.childNodes[0].data

    if (expected_prefix is not None
        and modified_path.find(expected_prefix) == 0):
      modified_path = modified_path.replace(expected_prefix, '')[1:].strip()

    # Workaround single-object diff
    if len(modified_path) == 0:
      modified_path = path.childNodes[0].data.split(os.sep)[-1]

    # From here on, we use '/' as path separator.
    if os.sep != "/":
      modified_path = modified_path.replace(os.sep, "/")

    if modified_path not in expected_paths:
      print("ERROR: %s not expected in the changed paths." % modified_path)
      raise Failure

    index = expected_paths.index(modified_path)
    expected_item = items[index]
    expected_kind = kinds[index]
    expected_prop = expected_props[index]
    actual_item = path.getAttribute('item')
    actual_kind = path.getAttribute('kind')
    actual_prop = path.getAttribute('props')

    if expected_item != actual_item:
      print("ERROR: expected: %s actual: %s" % (expected_item, actual_item))
      raise Failure

    if expected_kind != actual_kind:
      print("ERROR: expected: %s actual: %s" % (expected_kind, actual_kind))
      raise Failure

    if expected_prop != actual_prop:
      print("ERROR: expected: %s actual: %s" % (expected_prop, actual_prop))
      raise Failure

def run_and_verify_diff_summarize(output_tree, *args):
  """Run 'diff --summarize' with the arguments *ARGS.

  The subcommand output will be verified against OUTPUT_TREE.  Returns
  on success, raises on failure.
  """

  if isinstance(output_tree, wc.State):
    output_tree = output_tree.old_tree()

  exit_code, output, errput = main.run_svn(None, 'diff', '--summarize',
                                           *args)

  actual = tree.build_tree_from_diff_summarize(output)

  # Verify actual output against expected output.
  try:
    tree.compare_trees("output", actual, output_tree)
  except tree.SVNTreeError:
    verify.display_trees(None, 'DIFF OUTPUT TREE', output_tree, actual)
    print("ACTUAL DIFF OUTPUT TREE:")
    tree.dump_tree_script(actual)
    raise

def run_and_validate_lock(path, username):
  """`svn lock' the given path and validate the contents of the lock.
     Use the given username. This is important because locks are
     user specific."""

  comment = "Locking path:%s." % path

  # lock the path
  run_and_verify_svn(None, ".*locked by user", [], 'lock',
                     '--username', username,
                     '-m', comment, path)

  # Run info and check that we get the lock fields.
  exit_code, output, err = run_and_verify_svn(None, None, [],
                                              'info','-R',
                                              path)

  ### TODO: Leverage RegexOuput([...], match_all=True) here.
  # prepare the regexs to compare against
  token_re = re.compile(".*?Lock Token: opaquelocktoken:.*?", re.DOTALL)
  author_re = re.compile(".*?Lock Owner: %s\n.*?" % username, re.DOTALL)
  created_re = re.compile(".*?Lock Created:.*?", re.DOTALL)
  comment_re = re.compile(".*?%s\n.*?" % re.escape(comment), re.DOTALL)
  # join all output lines into one
  output = "".join(output)
  # Fail even if one regex does not match
  if ( not (token_re.match(output) and
            author_re.match(output) and
            created_re.match(output) and
            comment_re.match(output))):
    raise Failure

def _run_and_verify_resolve(cmd, expected_paths, *args):
  """Run "svn CMD" (where CMD is 'resolve' or 'resolved') with arguments
  ARGS, and verify that it resolves the paths in EXPECTED_PATHS and no others.
  If no ARGS are specified, use the elements of EXPECTED_PATHS as the
  arguments."""
  # TODO: verify that the status of PATHS changes accordingly.
  if len(args) == 0:
    args = expected_paths
  expected_output = verify.UnorderedOutput([
    "Resolved conflicted state of '" + path + "'\n" for path in
    expected_paths])
  run_and_verify_svn(None, expected_output, [],
                     cmd, *args)

def run_and_verify_resolve(expected_paths, *args):
  """Run "svn resolve" with arguments ARGS, and verify that it resolves the
  paths in EXPECTED_PATHS and no others. If no ARGS are specified, use the
  elements of EXPECTED_PATHS as the arguments."""
  _run_and_verify_resolve('resolve', expected_paths, *args)

def run_and_verify_resolved(expected_paths, *args):
  """Run "svn resolved" with arguments ARGS, and verify that it resolves the
  paths in EXPECTED_PATHS and no others. If no ARGS are specified, use the
  elements of EXPECTED_PATHS as the arguments."""
  _run_and_verify_resolve('resolved', expected_paths, *args)

def run_and_verify_revert(expected_paths, *args):
  """Run "svn revert" with arguments ARGS, and verify that it reverts
  the paths in EXPECTED_PATHS and no others.  If no ARGS are
  specified, use the elements of EXPECTED_PATHS as the arguments."""
  if len(args) == 0:
    args = expected_paths
  expected_output = verify.UnorderedOutput([
    "Reverted '" + path + "'\n" for path in
    expected_paths])
  run_and_verify_svn(None, expected_output, [],
                     "revert", *args)


######################################################################
# Other general utilities


# This allows a test to *quickly* bootstrap itself.
def make_repo_and_wc(sbox, create_wc = True, read_only = False):
  """Create a fresh 'Greek Tree' repository and check out a WC from it.

  If READ_ONLY is False, a dedicated repository will be created, at the path
  SBOX.repo_dir.  If READ_ONLY is True, the pristine repository will be used.
  In either case, SBOX.repo_url is assumed to point to the repository that
  will be used.

  If create_wc is True, a dedicated working copy will be checked out from
  the repository, at the path SBOX.wc_dir.

  Returns on success, raises on failure."""

  # Create (or copy afresh) a new repos with a greek tree in it.
  if not read_only:
    guarantee_greek_repository(sbox.repo_dir)

  if create_wc:
    # Generate the expected output tree.
    expected_output = main.greek_state.copy()
    expected_output.wc_dir = sbox.wc_dir
    expected_output.tweak(status='A ', contents=None)

    # Generate an expected wc tree.
    expected_wc = main.greek_state

    # Do a checkout, and verify the resulting output and disk contents.
    run_and_verify_checkout(sbox.repo_url,
                            sbox.wc_dir,
                            expected_output,
                            expected_wc)
  else:
    # just make sure the parent folder of our working copy is created
    try:
      os.mkdir(main.general_wc_dir)
    except OSError, err:
      if err.errno != errno.EEXIST:
        raise

# Duplicate a working copy or other dir.
def duplicate_dir(wc_name, wc_copy_name):
  """Copy the working copy WC_NAME to WC_COPY_NAME.  Overwrite any
  existing tree at that location."""

  main.safe_rmtree(wc_copy_name)
  shutil.copytree(wc_name, wc_copy_name)



def get_virginal_state(wc_dir, rev):
  "Return a virginal greek tree state for a WC and repos at revision REV."

  rev = str(rev) ### maybe switch rev to an integer?

  # copy the greek tree, shift it to the new wc_dir, insert a root elem,
  # then tweak all values
  state = main.greek_state.copy()
  state.wc_dir = wc_dir
  state.desc[''] = wc.StateItem()
  state.tweak(contents=None, status='  ', wc_rev=rev)

  return state

# Cheap administrative directory locking
def lock_admin_dir(wc_dir, recursive=False):
  "Lock a SVN administrative directory"
  db, root_path, relpath = wc.open_wc_db(wc_dir)

  svntest.main.run_wc_lock_tester(recursive, wc_dir)

def set_incomplete(wc_dir, revision):
  "Make wc_dir incomplete at revision"

  svntest.main.run_wc_incomplete_tester(wc_dir, revision)

def get_wc_uuid(wc_dir):
  "Return the UUID of the working copy at WC_DIR."
  return run_and_parse_info(wc_dir)[0]['Repository UUID']

def get_wc_base_rev(wc_dir):
  "Return the BASE revision of the working copy at WC_DIR."
  return run_and_parse_info(wc_dir)[0]['Revision']

def hook_failure_message(hook_name):
  """Return the error message that the client prints for failure of the
  specified hook HOOK_NAME. The wording changed with Subversion 1.5."""
  if svntest.main.options.server_minor_version < 5:
    return "'%s' hook failed with error output:\n" % hook_name
  else:
    if hook_name in ["start-commit", "pre-commit"]:
      action = "Commit"
    elif hook_name == "pre-revprop-change":
      action = "Revprop change"
    elif hook_name == "pre-lock":
      action = "Lock"
    elif hook_name == "pre-unlock":
      action = "Unlock"
    else:
      action = None
    if action is None:
      message = "%s hook failed (exit code 1)" % (hook_name,)
    else:
      message = "%s blocked by %s hook (exit code 1)" % (action, hook_name)
    return message + " with output:\n"

def create_failing_hook(repo_dir, hook_name, text):
  """Create a HOOK_NAME hook in the repository at REPO_DIR that prints
  TEXT to stderr and exits with an error."""

  hook_path = os.path.join(repo_dir, 'hooks', hook_name)
  # Embed the text carefully: it might include characters like "%" and "'".
  main.create_python_hook_script(hook_path, 'import sys\n'
    'sys.stderr.write(' + repr(text) + ')\n'
    'sys.exit(1)\n')

def enable_revprop_changes(repo_dir):
  """Enable revprop changes in the repository at REPO_DIR by creating a
  pre-revprop-change hook script and (if appropriate) making it executable."""

  hook_path = main.get_pre_revprop_change_hook_path(repo_dir)
  main.create_python_hook_script(hook_path, 'import sys; sys.exit(0)')

def disable_revprop_changes(repo_dir):
  """Disable revprop changes in the repository at REPO_DIR by creating a
  pre-revprop-change hook script that prints "pre-revprop-change" followed
  by its arguments, and returns an error."""

  hook_path = main.get_pre_revprop_change_hook_path(repo_dir)
  main.create_python_hook_script(hook_path,
                                 'import sys\n'
                                 'sys.stderr.write("pre-revprop-change %s" % " ".join(sys.argv[1:6]))\n'
                                 'sys.exit(1)\n')

def create_failing_post_commit_hook(repo_dir):
  """Create a post-commit hook script in the repository at REPO_DIR that always
  reports an error."""

  hook_path = main.get_post_commit_hook_path(repo_dir)
  main.create_python_hook_script(hook_path, 'import sys\n'
    'sys.stderr.write("Post-commit hook failed")\n'
    'sys.exit(1)')

# set_prop can be used for properties with NULL characters which are not
# handled correctly when passed to subprocess.Popen() and values like "*"
# which are not handled correctly on Windows.
def set_prop(name, value, path, expected_re_string=None):
  """Set a property with specified value"""
  if value and (value[0] == '-' or '\x00' in value or sys.platform == 'win32'):
    from tempfile import mkstemp
    (fd, value_file_path) = mkstemp()
    value_file = open(value_file_path, 'wb')
    value_file.write(value)
    value_file.flush()
    value_file.close()
    exit_code, out, err = main.run_svn(expected_re_string, 'propset',
                                       '-F', value_file_path, name, path)
    os.close(fd)
    os.remove(value_file_path)
  else:
    exit_code, out, err = main.run_svn(expected_re_string, 'propset',
                                       name, value, path)
  if expected_re_string:
    if not expected_re_string.startswith(".*"):
      expected_re_string = ".*(" + expected_re_string + ")"
    expected_err = verify.RegexOutput(expected_re_string, match_all=False)
    verify.verify_outputs(None, None, err, None, expected_err)

def check_prop(name, path, exp_out, revprop=None):
  """Verify that property NAME on PATH has a value of EXP_OUT.
  If REVPROP is not None, then it is a revision number and
  a revision property is sought."""
  if revprop is not None:
    revprop_options = ['--revprop', '-r', revprop]
  else:
    revprop_options = []
  # Not using run_svn because binary_mode must be set
  exit_code, out, err = main.run_command(main.svn_binary, None, 1, 'pg',
                                         '--strict', name, path,
                                         '--config-dir',
                                         main.default_config_dir,
                                         '--username', main.wc_author,
                                         '--password', main.wc_passwd,
                                         *revprop_options)
  if out != exp_out:
    print("svn pg --strict %s output does not match expected." % name)
    print("Expected standard output:  %s\n" % exp_out)
    print("Actual standard output:  %s\n" % out)
    raise Failure

def fill_file_with_lines(wc_path, line_nbr, line_descrip=None,
                         append=True):
  """Change the file at WC_PATH (adding some lines), and return its
  new contents.  LINE_NBR indicates the line number at which the new
  contents should assume that it's being appended.  LINE_DESCRIP is
  something like 'This is line' (the default) or 'Conflicting line'."""

  if line_descrip is None:
    line_descrip = "This is line"

  # Generate the new contents for the file.
  contents = ""
  for n in range(line_nbr, line_nbr + 3):
    contents = contents + line_descrip + " " + repr(n) + " in '" + \
               os.path.basename(wc_path) + "'.\n"

  # Write the new contents to the file.
  if append:
    main.file_append(wc_path, contents)
  else:
    main.file_write(wc_path, contents)

  return contents

def inject_conflict_into_wc(sbox, state_path, file_path,
                            expected_disk, expected_status, merged_rev):
  """Create a conflict at FILE_PATH by replacing its contents,
  committing the change, backdating it to its previous revision,
  changing its contents again, then updating it to merge in the
  previous change."""

  wc_dir = sbox.wc_dir

  # Make a change to the file.
  contents = fill_file_with_lines(file_path, 1, "This is line", append=False)

  # Commit the changed file, first taking note of the current revision.
  prev_rev = expected_status.desc[state_path].wc_rev
  expected_output = wc.State(wc_dir, {
    state_path : wc.StateItem(verb='Sending'),
    })
  if expected_status:
    expected_status.tweak(state_path, wc_rev=merged_rev)
  run_and_verify_commit(wc_dir, expected_output, expected_status,
                        None, file_path)

  # Backdate the file.
  exit_code, output, errput = main.run_svn(None, "up", "-r", str(prev_rev),
                                           file_path)
  if expected_status:
    expected_status.tweak(state_path, wc_rev=prev_rev)

  # Make a conflicting change to the file, and backdate the file.
  conflicting_contents = fill_file_with_lines(file_path, 1, "Conflicting line",
                                              append=False)

  # Merge the previous change into the file to produce a conflict.
  if expected_disk:
    expected_disk.tweak(state_path, contents="")
  expected_output = wc.State(wc_dir, {
    state_path : wc.StateItem(status='C '),
    })
  inject_conflict_into_expected_state(state_path,
                                      expected_disk, expected_status,
                                      conflicting_contents, contents,
                                      merged_rev)
  exit_code, output, errput = main.run_svn(None, "up", "-r", str(merged_rev),
                                           file_path)
  if expected_status:
    expected_status.tweak(state_path, wc_rev=merged_rev)

def inject_conflict_into_expected_state(state_path,
                                        expected_disk, expected_status,
                                        wc_text, merged_text, merged_rev):
  """Update the EXPECTED_DISK and EXPECTED_STATUS trees for the
  conflict at STATE_PATH (ignored if None).  WC_TEXT, MERGED_TEXT, and
  MERGED_REV are used to determine the contents of the conflict (the
  text parameters should be newline-terminated)."""
  if expected_disk:
    conflict_marker = make_conflict_marker_text(wc_text, merged_text,
                                                merged_rev)
    existing_text = expected_disk.desc[state_path].contents or ""
    expected_disk.tweak(state_path, contents=existing_text + conflict_marker)

  if expected_status:
    expected_status.tweak(state_path, status='C ')

def make_conflict_marker_text(wc_text, merged_text, merged_rev):
  """Return the conflict marker text described by WC_TEXT (the current
  text in the working copy, MERGED_TEXT (the conflicting text merged
  in), and MERGED_REV (the revision from whence the conflicting text
  came)."""
  return "<<<<<<< .working\n" + wc_text + "=======\n" + \
         merged_text + ">>>>>>> .merge-right.r" + str(merged_rev) + "\n"


def build_greek_tree_conflicts(sbox):
  """Create a working copy that has tree-conflict markings.
  After this function has been called, sbox.wc_dir is a working
  copy that has specific tree-conflict markings.

  In particular, this does two conflicting sets of edits and performs an
  update so that tree conflicts appear.

  Note that this function calls sbox.build() because it needs a clean sbox.
  So, there is no need to call sbox.build() before this.

  The conflicts are the result of an 'update' on the following changes:

                Incoming    Local

    A/D/G/pi    text-mod    del
    A/D/G/rho   del         text-mod
    A/D/G/tau   del         del

  This function is useful for testing that tree-conflicts are handled
  properly once they have appeared, e.g. that commits are blocked, that the
  info output is correct, etc.

  See also the tree-conflicts tests using deep_trees in various other
  .py files, and tree_conflict_tests.py.
  """

  sbox.build()
  wc_dir = sbox.wc_dir
  j = os.path.join
  G = j(wc_dir, 'A', 'D', 'G')
  pi = j(G, 'pi')
  rho = j(G, 'rho')
  tau = j(G, 'tau')

  # Make incoming changes and "store them away" with a commit.
  main.file_append(pi, "Incoming edit.\n")
  main.run_svn(None, 'del', rho)
  main.run_svn(None, 'del', tau)

  expected_output = wc.State(wc_dir, {
    'A/D/G/pi'          : Item(verb='Sending'),
    'A/D/G/rho'         : Item(verb='Deleting'),
    'A/D/G/tau'         : Item(verb='Deleting'),
    })
  expected_status = get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/G/pi', wc_rev='2')
  expected_status.remove('A/D/G/rho', 'A/D/G/tau')
  run_and_verify_commit(wc_dir, expected_output, expected_status, None,
                        '-m', 'Incoming changes.', wc_dir )

  # Update back to the pristine state ("time-warp").
  expected_output = wc.State(wc_dir, {
    'A/D/G/pi'          : Item(status='U '),
    'A/D/G/rho'         : Item(status='A '),
    'A/D/G/tau'         : Item(status='A '),
    })
  expected_disk = main.greek_state
  expected_status = get_virginal_state(wc_dir, 1)
  run_and_verify_update(wc_dir, expected_output, expected_disk,
                        expected_status, None, None, None, None, None, False,
                        '-r', '1', wc_dir)

  # Make local changes
  main.run_svn(None, 'del', pi)
  main.file_append(rho, "Local edit.\n")
  main.run_svn(None, 'del', tau)

  # Update, receiving the incoming changes on top of the local changes,
  # causing tree conflicts.  Don't check for any particular result: that is
  # the job of other tests.
  run_and_verify_svn(None, verify.AnyOutput, [], 'update', wc_dir)


def make_deep_trees(base):
  """Helper function for deep trees conflicts. Create a set of trees,
  each in its own "container" dir. Any conflicts can be tested separately
  in each container.
  """
  j = os.path.join
  # Create the container dirs.
  F   = j(base, 'F')
  D   = j(base, 'D')
  DF  = j(base, 'DF')
  DD  = j(base, 'DD')
  DDF = j(base, 'DDF')
  DDD = j(base, 'DDD')
  os.makedirs(F)
  os.makedirs(j(D, 'D1'))
  os.makedirs(j(DF, 'D1'))
  os.makedirs(j(DD, 'D1', 'D2'))
  os.makedirs(j(DDF, 'D1', 'D2'))
  os.makedirs(j(DDD, 'D1', 'D2', 'D3'))

  # Create their files.
  alpha = j(F, 'alpha')
  beta  = j(DF, 'D1', 'beta')
  gamma = j(DDF, 'D1', 'D2', 'gamma')
  main.file_append(alpha, "This is the file 'alpha'.\n")
  main.file_append(beta, "This is the file 'beta'.\n")
  main.file_append(gamma, "This is the file 'gamma'.\n")


def add_deep_trees(sbox, base_dir_name):
  """Prepare a "deep_trees" within a given directory.

  The directory <sbox.wc_dir>/<base_dir_name> is created and a deep_tree
  is created within. The items are only added, a commit has to be
  called separately, if needed.

  <base_dir_name> will thus be a container for the set of containers
  mentioned in make_deep_trees().
  """
  j = os.path.join
  base = j(sbox.wc_dir, base_dir_name)
  make_deep_trees(base)
  main.run_svn(None, 'add', base)


Item = wc.StateItem

# initial deep trees state
deep_trees_virginal_state = wc.State('', {
  'F'               : Item(),
  'F/alpha'         : Item("This is the file 'alpha'.\n"),
  'D'               : Item(),
  'D/D1'            : Item(),
  'DF'              : Item(),
  'DF/D1'           : Item(),
  'DF/D1/beta'      : Item("This is the file 'beta'.\n"),
  'DD'              : Item(),
  'DD/D1'           : Item(),
  'DD/D1/D2'        : Item(),
  'DDF'             : Item(),
  'DDF/D1'          : Item(),
  'DDF/D1/D2'       : Item(),
  'DDF/D1/D2/gamma' : Item("This is the file 'gamma'.\n"),
  'DDD'             : Item(),
  'DDD/D1'          : Item(),
  'DDD/D1/D2'       : Item(),
  'DDD/D1/D2/D3'    : Item(),
  })


# Many actions on deep trees and their resulting states...

def deep_trees_leaf_edit(base):
  """Helper function for deep trees test cases. Append text to files,
  create new files in empty directories, and change leaf node properties."""
  j = os.path.join
  F   = j(base, 'F', 'alpha')
  DF  = j(base, 'DF', 'D1', 'beta')
  DDF = j(base, 'DDF', 'D1', 'D2', 'gamma')
  main.file_append(F, "More text for file alpha.\n")
  main.file_append(DF, "More text for file beta.\n")
  main.file_append(DDF, "More text for file gamma.\n")
  run_and_verify_svn(None, verify.AnyOutput, [],
                     'propset', 'prop1', '1', F, DF, DDF)

  D   = j(base, 'D', 'D1')
  DD  = j(base, 'DD', 'D1', 'D2')
  DDD = j(base, 'DDD', 'D1', 'D2', 'D3')
  run_and_verify_svn(None, verify.AnyOutput, [],
                     'propset', 'prop1', '1', D, DD, DDD)
  D   = j(base, 'D', 'D1', 'delta')
  DD  = j(base, 'DD', 'D1', 'D2', 'epsilon')
  DDD = j(base, 'DDD', 'D1', 'D2', 'D3', 'zeta')
  main.file_append(D, "This is the file 'delta'.\n")
  main.file_append(DD, "This is the file 'epsilon'.\n")
  main.file_append(DDD, "This is the file 'zeta'.\n")
  run_and_verify_svn(None, verify.AnyOutput, [],
                     'add', D, DD, DDD)

# deep trees state after a call to deep_trees_leaf_edit
deep_trees_after_leaf_edit = wc.State('', {
  'F'                 : Item(),
  'F/alpha'           : Item("This is the file 'alpha'.\nMore text for file alpha.\n"),
  'D'                 : Item(),
  'D/D1'              : Item(),
  'D/D1/delta'        : Item("This is the file 'delta'.\n"),
  'DF'                : Item(),
  'DF/D1'             : Item(),
  'DF/D1/beta'        : Item("This is the file 'beta'.\nMore text for file beta.\n"),
  'DD'                : Item(),
  'DD/D1'             : Item(),
  'DD/D1/D2'          : Item(),
  'DD/D1/D2/epsilon'  : Item("This is the file 'epsilon'.\n"),
  'DDF'               : Item(),
  'DDF/D1'            : Item(),
  'DDF/D1/D2'         : Item(),
  'DDF/D1/D2/gamma'   : Item("This is the file 'gamma'.\nMore text for file gamma.\n"),
  'DDD'               : Item(),
  'DDD/D1'            : Item(),
  'DDD/D1/D2'         : Item(),
  'DDD/D1/D2/D3'      : Item(),
  'DDD/D1/D2/D3/zeta' : Item("This is the file 'zeta'.\n"),
  })


def deep_trees_leaf_del(base):
  """Helper function for deep trees test cases. Delete files and empty
  dirs."""
  j = os.path.join
  F   = j(base, 'F', 'alpha')
  D   = j(base, 'D', 'D1')
  DF  = j(base, 'DF', 'D1', 'beta')
  DD  = j(base, 'DD', 'D1', 'D2')
  DDF = j(base, 'DDF', 'D1', 'D2', 'gamma')
  DDD = j(base, 'DDD', 'D1', 'D2', 'D3')
  main.run_svn(None, 'rm', F, D, DF, DD, DDF, DDD)

# deep trees state after a call to deep_trees_leaf_del
deep_trees_after_leaf_del = wc.State('', {
  'F'               : Item(),
  'D'               : Item(),
  'DF'              : Item(),
  'DF/D1'           : Item(),
  'DD'              : Item(),
  'DD/D1'           : Item(),
  'DDF'             : Item(),
  'DDF/D1'          : Item(),
  'DDF/D1/D2'       : Item(),
  'DDD'             : Item(),
  'DDD/D1'          : Item(),
  'DDD/D1/D2'       : Item(),
  })

# deep trees state after a call to deep_trees_leaf_del with no commit
def deep_trees_after_leaf_del_no_ci(wc_dir):
  if svntest.main.wc_is_singledb(wc_dir):
    return deep_trees_after_leaf_del
  else:
    return deep_trees_empty_dirs


def deep_trees_tree_del(base):
  """Helper function for deep trees test cases.  Delete top-level dirs."""
  j = os.path.join
  F   = j(base, 'F', 'alpha')
  D   = j(base, 'D', 'D1')
  DF  = j(base, 'DF', 'D1')
  DD  = j(base, 'DD', 'D1')
  DDF = j(base, 'DDF', 'D1')
  DDD = j(base, 'DDD', 'D1')
  main.run_svn(None, 'rm', F, D, DF, DD, DDF, DDD)

def deep_trees_rmtree(base):
  """Helper function for deep trees test cases.  Delete top-level dirs
     with rmtree instead of svn del."""
  j = os.path.join
  F   = j(base, 'F', 'alpha')
  D   = j(base, 'D', 'D1')
  DF  = j(base, 'DF', 'D1')
  DD  = j(base, 'DD', 'D1')
  DDF = j(base, 'DDF', 'D1')
  DDD = j(base, 'DDD', 'D1')
  os.unlink(F)
  main.safe_rmtree(D)
  main.safe_rmtree(DF)
  main.safe_rmtree(DD)
  main.safe_rmtree(DDF)
  main.safe_rmtree(DDD)

# deep trees state after a call to deep_trees_tree_del
deep_trees_after_tree_del = wc.State('', {
  'F'                 : Item(),
  'D'                 : Item(),
  'DF'                : Item(),
  'DD'                : Item(),
  'DDF'               : Item(),
  'DDD'               : Item(),
  })

# deep trees state without any files
deep_trees_empty_dirs = wc.State('', {
  'F'               : Item(),
  'D'               : Item(),
  'D/D1'            : Item(),
  'DF'              : Item(),
  'DF/D1'           : Item(),
  'DD'              : Item(),
  'DD/D1'           : Item(),
  'DD/D1/D2'        : Item(),
  'DDF'             : Item(),
  'DDF/D1'          : Item(),
  'DDF/D1/D2'       : Item(),
  'DDD'             : Item(),
  'DDD/D1'          : Item(),
  'DDD/D1/D2'       : Item(),
  'DDD/D1/D2/D3'    : Item(),
  })

# deep trees state after a call to deep_trees_tree_del with no commit
def deep_trees_after_tree_del_no_ci(wc_dir):
  if svntest.main.wc_is_singledb(wc_dir):
    return deep_trees_after_tree_del
  else:
    return deep_trees_empty_dirs

def deep_trees_tree_del_repos(base):
  """Helper function for deep trees test cases.  Delete top-level dirs,
  directly in the repository."""
  j = '/'.join
  F   = j([base, 'F', 'alpha'])
  D   = j([base, 'D', 'D1'])
  DF  = j([base, 'DF', 'D1'])
  DD  = j([base, 'DD', 'D1'])
  DDF = j([base, 'DDF', 'D1'])
  DDD = j([base, 'DDD', 'D1'])
  main.run_svn(None, 'mkdir', '-m', '', F, D, DF, DD, DDF, DDD)

# Expected merge/update/switch output.

deep_trees_conflict_output = wc.State('', {
  'F/alpha'           : Item(status='  ', treeconflict='C'),
  'D/D1'              : Item(status='  ', treeconflict='C'),
  'DF/D1'             : Item(status='  ', treeconflict='C'),
  'DD/D1'             : Item(status='  ', treeconflict='C'),
  'DDF/D1'            : Item(status='  ', treeconflict='C'),
  'DDD/D1'            : Item(status='  ', treeconflict='C'),
  })

deep_trees_conflict_output_skipped = wc.State('', {
  'D/D1'              : Item(verb='Skipped'),
  'F/alpha'           : Item(verb='Skipped'),
  'DD/D1'             : Item(verb='Skipped'),
  'DF/D1'             : Item(verb='Skipped'),
  'DDD/D1'            : Item(verb='Skipped'),
  'DDF/D1'            : Item(verb='Skipped'),
  })

# Expected status output after merge/update/switch.

deep_trees_status_local_tree_del = wc.State('', {
  ''                  : Item(status='  ', wc_rev=3),
  'D'                 : Item(status='  ', wc_rev=3),
  'D/D1'              : Item(status='D ', wc_rev=2, treeconflict='C'),
  'DD'                : Item(status='  ', wc_rev=3),
  'DD/D1'             : Item(status='D ', wc_rev=2, treeconflict='C'),
  'DD/D1/D2'          : Item(status='D ', wc_rev=2),
  'DDD'               : Item(status='  ', wc_rev=3),
  'DDD/D1'            : Item(status='D ', wc_rev=2, treeconflict='C'),
  'DDD/D1/D2'         : Item(status='D ', wc_rev=2),
  'DDD/D1/D2/D3'      : Item(status='D ', wc_rev=2),
  'DDF'               : Item(status='  ', wc_rev=3),
  'DDF/D1'            : Item(status='D ', wc_rev=2, treeconflict='C'),
  'DDF/D1/D2'         : Item(status='D ', wc_rev=2),
  'DDF/D1/D2/gamma'   : Item(status='D ', wc_rev=2),
  'DF'                : Item(status='  ', wc_rev=3),
  'DF/D1'             : Item(status='D ', wc_rev=2, treeconflict='C'),
  'DF/D1/beta'        : Item(status='D ', wc_rev=2),
  'F'                 : Item(status='  ', wc_rev=3),
  'F/alpha'           : Item(status='D ', wc_rev=2, treeconflict='C'),
  })

deep_trees_status_local_leaf_edit = wc.State('', {
  ''                  : Item(status='  ', wc_rev=3),
  'D'                 : Item(status='  ', wc_rev=3),
  'D/D1'              : Item(status=' M', wc_rev=2, treeconflict='C'),
  'D/D1/delta'        : Item(status='A ', wc_rev=0),
  'DD'                : Item(status='  ', wc_rev=3),
  'DD/D1'             : Item(status='  ', wc_rev=2, treeconflict='C'),
  'DD/D1/D2'          : Item(status=' M', wc_rev=2),
  'DD/D1/D2/epsilon'  : Item(status='A ', wc_rev=0),
  'DDD'               : Item(status='  ', wc_rev=3),
  'DDD/D1'            : Item(status='  ', wc_rev=2, treeconflict='C'),
  'DDD/D1/D2'         : Item(status='  ', wc_rev=2),
  'DDD/D1/D2/D3'      : Item(status=' M', wc_rev=2),
  'DDD/D1/D2/D3/zeta' : Item(status='A ', wc_rev=0),
  'DDF'               : Item(status='  ', wc_rev=3),
  'DDF/D1'            : Item(status='  ', wc_rev=2, treeconflict='C'),
  'DDF/D1/D2'         : Item(status='  ', wc_rev=2),
  'DDF/D1/D2/gamma'   : Item(status='MM', wc_rev=2),
  'DF'                : Item(status='  ', wc_rev=3),
  'DF/D1'             : Item(status='  ', wc_rev=2, treeconflict='C'),
  'DF/D1/beta'        : Item(status='MM', wc_rev=2),
  'F'                 : Item(status='  ', wc_rev=3),
  'F/alpha'           : Item(status='MM', wc_rev=2, treeconflict='C'),
  })


class DeepTreesTestCase:
  """Describes one tree-conflicts test case.
  See deep_trees_run_tests_scheme_for_update(), ..._switch(), ..._merge().

  The name field is the subdirectory name in which the test should be run.

  The local_action and incoming_action are the functions to run
  to construct the local changes and incoming changes, respectively.
  See deep_trees_leaf_edit, deep_trees_tree_del, etc.

  The expected_* and error_re_string arguments are described in functions
  run_and_verify_[update|switch|merge]
  except expected_info, which is a dict that has path keys with values
  that are dicts as passed to run_and_verify_info():
    expected_info = {
      'F/alpha' : {
        'Revision' : '3',
        'Tree conflict' :
          '^local delete, incoming edit upon update'
          + ' Source  left: .file.*/F/alpha@2'
          + ' Source right: .file.*/F/alpha@3$',
      },
      'DF/D1' : {
        'Tree conflict' :
          '^local delete, incoming edit upon update'
          + ' Source  left: .dir.*/DF/D1@2'
          + ' Source right: .dir.*/DF/D1@3$',
      },
      ...
    }

  Note: expected_skip is only used in merge, i.e. using
  deep_trees_run_tests_scheme_for_merge.
  """

  def __init__(self, name, local_action, incoming_action,
                expected_output = None, expected_disk = None,
                expected_status = None, expected_skip = None,
                error_re_string = None,
                commit_block_string = ".*remains in conflict.*",
                expected_info = None):
    self.name = name
    self.local_action = local_action
    self.incoming_action = incoming_action
    self.expected_output = expected_output
    self.expected_disk = expected_disk
    self.expected_status = expected_status
    self.expected_skip = expected_skip
    self.error_re_string = error_re_string
    self.commit_block_string = commit_block_string
    self.expected_info = expected_info



def deep_trees_run_tests_scheme_for_update(sbox, greater_scheme):
  """
  Runs a given list of tests for conflicts occuring at an update operation.

  This function wants to save time and perform a number of different
  test cases using just a single repository and performing just one commit
  for all test cases instead of one for each test case.

   1) Each test case is initialized in a separate subdir. Each subdir
      again contains one set of "deep_trees", being separate container
      dirs for different depths of trees (F, D, DF, DD, DDF, DDD).

   2) A commit is performed across all test cases and depths.
      (our initial state, -r2)

   3) In each test case subdir (e.g. "local_tree_del_incoming_leaf_edit"),
      its *incoming* action is performed (e.g. "deep_trees_leaf_edit"), in
      each of the different depth trees (F, D, DF, ... DDD).

   4) A commit is performed across all test cases and depths:
      our "incoming" state is "stored away in the repository for now",
      -r3.

   5) All test case dirs and contained deep_trees are time-warped
      (updated) back to -r2, the initial state containing deep_trees.

   6) In each test case subdir (e.g. "local_tree_del_incoming_leaf_edit"),
      its *local* action is performed (e.g. "deep_trees_leaf_del"), in
      each of the different depth trees (F, D, DF, ... DDD).

   7) An update to -r3 is performed across all test cases and depths.
      This causes tree-conflicts between the "local" state in the working
      copy and the "incoming" state from the repository, -r3.

   8) A commit is performed in each separate container, to verify
      that each tree-conflict indeed blocks a commit.

  The sbox parameter is just the sbox passed to a test function. No need
  to call sbox.build(), since it is called (once) within this function.

  The "table" greater_scheme models all of the different test cases
  that should be run using a single repository.

  greater_scheme is a list of DeepTreesTestCase items, which define complete
  test setups, so that they can be performed as described above.
  """

  j = os.path.join

  if not sbox.is_built():
    sbox.build()
  wc_dir = sbox.wc_dir


  # 1) create directories

  for test_case in greater_scheme:
    try:
      add_deep_trees(sbox, test_case.name)
    except:
      print("ERROR IN: Tests scheme for update: "
          + "while setting up deep trees in '%s'" % test_case.name)
      raise


  # 2) commit initial state

  main.run_svn(None, 'commit', '-m', 'initial state', wc_dir)


  # 3) apply incoming changes

  for test_case in greater_scheme:
    try:
      test_case.incoming_action(j(sbox.wc_dir, test_case.name))
    except:
      print("ERROR IN: Tests scheme for update: "
          + "while performing incoming action in '%s'" % test_case.name)
      raise


  # 4) commit incoming changes

  main.run_svn(None, 'commit', '-m', 'incoming changes', wc_dir)


  # 5) time-warp back to -r2

  main.run_svn(None, 'update', '-r2', wc_dir)


  # 6) apply local changes

  for test_case in greater_scheme:
    try:
      test_case.local_action(j(wc_dir, test_case.name))
    except:
      print("ERROR IN: Tests scheme for update: "
          + "while performing local action in '%s'" % test_case.name)
      raise


  # 7) update to -r3, conflicting with incoming changes.
  #    A lot of different things are expected.
  #    Do separate update operations for each test case.

  for test_case in greater_scheme:
    try:
      base = j(wc_dir, test_case.name)

      x_out = test_case.expected_output
      if x_out != None:
        x_out = x_out.copy()
        x_out.wc_dir = base

      x_disk = test_case.expected_disk

      x_status = test_case.expected_status
      if x_status != None:
        x_status.copy()
        x_status.wc_dir = base

      run_and_verify_update(base, x_out, x_disk, None,
                            error_re_string = test_case.error_re_string)
      if x_status:
        run_and_verify_unquiet_status(base, x_status)

      x_info = test_case.expected_info or {}
      for path in x_info:
        run_and_verify_info([x_info[path]], j(base, path))

    except:
      print("ERROR IN: Tests scheme for update: "
          + "while verifying in '%s'" % test_case.name)
      raise


  # 8) Verify that commit fails.

  for test_case in greater_scheme:
    try:
      base = j(wc_dir, test_case.name)

      x_status = test_case.expected_status
      if x_status != None:
        x_status.copy()
        x_status.wc_dir = base

      run_and_verify_commit(base, None, x_status,
                            test_case.commit_block_string,
                            base)
    except:
      print("ERROR IN: Tests scheme for update: "
          + "while checking commit-blocking in '%s'" % test_case.name)
      raise



def deep_trees_skipping_on_update(sbox, test_case, skip_paths,
                                  chdir_skip_paths):
  """
  Create tree conflicts, then update again, expecting the existing tree
  conflicts to be skipped.
  SKIP_PATHS is a list of paths, relative to the "base dir", for which
  "update" on the "base dir" should report as skipped.
  CHDIR_SKIP_PATHS is a list of (target-path, skipped-path) pairs for which
  an update of "target-path" (relative to the "base dir") should result in
  "skipped-path" (relative to "target-path") being reported as skipped.
  """

  """FURTHER_ACTION is a function that will make a further modification to
  each target, this being the modification that we expect to be skipped. The
  function takes the "base dir" (the WC path to the test case directory) as
  its only argument."""
  further_action = deep_trees_tree_del_repos

  j = os.path.join
  wc_dir = sbox.wc_dir
  base = j(wc_dir, test_case.name)

  # Initialize: generate conflicts. (We do not check anything here.)
  setup_case = DeepTreesTestCase(test_case.name,
                                 test_case.local_action,
                                 test_case.incoming_action,
                                 None,
                                 None,
                                 None)
  deep_trees_run_tests_scheme_for_update(sbox, [setup_case])

  # Make a further change to each target in the repository so there is a new
  # revision to update to. (This is r4.)
  further_action(sbox.repo_url + '/' + test_case.name)

  # Update whole working copy, expecting the nodes still in conflict to be
  # skipped.

  x_out = test_case.expected_output
  if x_out != None:
    x_out = x_out.copy()
    x_out.wc_dir = base

  x_disk = test_case.expected_disk

  x_status = test_case.expected_status
  if x_status != None:
    x_status = x_status.copy()
    x_status.wc_dir = base
    # Account for nodes that were updated by further_action
    x_status.tweak('', 'D', 'F', 'DD', 'DF', 'DDD', 'DDF', wc_rev=4)

  run_and_verify_update(base, x_out, x_disk, None,
                        error_re_string = test_case.error_re_string)

  run_and_verify_unquiet_status(base, x_status)

  # Try to update each in-conflict subtree. Expect a 'Skipped' output for
  # each, and the WC status to be unchanged.
  for path in skip_paths:
    run_and_verify_update(j(base, path),
                          wc.State(base, {path : Item(verb='Skipped')}),
                          None, None)

  run_and_verify_unquiet_status(base, x_status)

  # Try to update each in-conflict subtree. Expect a 'Skipped' output for
  # each, and the WC status to be unchanged.
  # This time, cd to the subdir before updating it.
  was_cwd = os.getcwd()
  for path, skipped in chdir_skip_paths:
    if isinstance(skipped, list):
      expected_skip = {}
      for p in skipped:
        expected_skip[p] = Item(verb='Skipped')
    else:
      expected_skip = {skipped : Item(verb='Skipped')}
    p = j(base, path)
    run_and_verify_update(p,
                          wc.State(p, expected_skip),
                          None, None)
  os.chdir(was_cwd)

  run_and_verify_unquiet_status(base, x_status)

  # Verify that commit still fails.
  for path, skipped in chdir_skip_paths:

    run_and_verify_commit(j(base, path), None, None,
                          test_case.commit_block_string,
                          base)

  run_and_verify_unquiet_status(base, x_status)


def deep_trees_run_tests_scheme_for_switch(sbox, greater_scheme):
  """
  Runs a given list of tests for conflicts occuring at a switch operation.

  This function wants to save time and perform a number of different
  test cases using just a single repository and performing just one commit
  for all test cases instead of one for each test case.

   1) Each test case is initialized in a separate subdir. Each subdir
      again contains two subdirs: one "local" and one "incoming" for
      the switch operation. These contain a set of deep_trees each.

   2) A commit is performed across all test cases and depths.
      (our initial state, -r2)

   3) In each test case subdir's incoming subdir, the
      incoming actions are performed.

   4) A commit is performed across all test cases and depths. (-r3)

   5) In each test case subdir's local subdir, the local actions are
      performed. They remain uncommitted in the working copy.

   6) In each test case subdir's local dir, a switch is performed to its
      corresponding incoming dir.
      This causes conflicts between the "local" state in the working
      copy and the "incoming" state from the incoming subdir (still -r3).

   7) A commit is performed in each separate container, to verify
      that each tree-conflict indeed blocks a commit.

  The sbox parameter is just the sbox passed to a test function. No need
  to call sbox.build(), since it is called (once) within this function.

  The "table" greater_scheme models all of the different test cases
  that should be run using a single repository.

  greater_scheme is a list of DeepTreesTestCase items, which define complete
  test setups, so that they can be performed as described above.
  """

  j = os.path.join

  if not sbox.is_built():
    sbox.build()
  wc_dir = sbox.wc_dir


  # 1) Create directories.

  for test_case in greater_scheme:
    try:
      base = j(sbox.wc_dir, test_case.name)
      os.makedirs(base)
      make_deep_trees(j(base, "local"))
      make_deep_trees(j(base, "incoming"))
      main.run_svn(None, 'add', base)
    except:
      print("ERROR IN: Tests scheme for switch: "
          + "while setting up deep trees in '%s'" % test_case.name)
      raise


  # 2) Commit initial state (-r2).

  main.run_svn(None, 'commit', '-m', 'initial state', wc_dir)


  # 3) Apply incoming changes

  for test_case in greater_scheme:
    try:
      test_case.incoming_action(j(sbox.wc_dir, test_case.name, "incoming"))
    except:
      print("ERROR IN: Tests scheme for switch: "
          + "while performing incoming action in '%s'" % test_case.name)
      raise


  # 4) Commit all changes (-r3).

  main.run_svn(None, 'commit', '-m', 'incoming changes', wc_dir)


  # 5) Apply local changes in their according subdirs.

  for test_case in greater_scheme:
    try:
      test_case.local_action(j(sbox.wc_dir, test_case.name, "local"))
    except:
      print("ERROR IN: Tests scheme for switch: "
          + "while performing local action in '%s'" % test_case.name)
      raise


  # 6) switch the local dir to the incoming url, conflicting with incoming
  #    changes. A lot of different things are expected.
  #    Do separate switch operations for each test case.

  for test_case in greater_scheme:
    try:
      local = j(wc_dir, test_case.name, "local")
      incoming = sbox.repo_url + "/" + test_case.name + "/incoming"

      x_out = test_case.expected_output
      if x_out != None:
        x_out = x_out.copy()
        x_out.wc_dir = local

      x_disk = test_case.expected_disk

      x_status = test_case.expected_status
      if x_status != None:
        x_status.copy()
        x_status.wc_dir = local

      run_and_verify_switch(local, local, incoming, x_out, x_disk, None,
                            test_case.error_re_string, None, None, None,
                            None, False, '--ignore-ancestry')
      run_and_verify_unquiet_status(local, x_status)

      x_info = test_case.expected_info or {}
      for path in x_info:
        run_and_verify_info([x_info[path]], j(local, path))
    except:
      print("ERROR IN: Tests scheme for switch: "
          + "while verifying in '%s'" % test_case.name)
      raise


  # 7) Verify that commit fails.

  for test_case in greater_scheme:
    try:
      local = j(wc_dir, test_case.name, 'local')

      x_status = test_case.expected_status
      if x_status != None:
        x_status.copy()
        x_status.wc_dir = local

      run_and_verify_commit(local, None, x_status,
                            test_case.commit_block_string,
                            local)
    except:
      print("ERROR IN: Tests scheme for switch: "
          + "while checking commit-blocking in '%s'" % test_case.name)
      raise


def deep_trees_run_tests_scheme_for_merge(sbox, greater_scheme,
                                          do_commit_local_changes,
                                          do_commit_conflicts=True,
                                          ignore_ancestry=False):
  """
  Runs a given list of tests for conflicts occuring at a merge operation.

  This function wants to save time and perform a number of different
  test cases using just a single repository and performing just one commit
  for all test cases instead of one for each test case.

   1) Each test case is initialized in a separate subdir. Each subdir
      initially contains another subdir, called "incoming", which
      contains a set of deep_trees.

   2) A commit is performed across all test cases and depths.
      (a pre-initial state)

   3) In each test case subdir, the "incoming" subdir is copied to "local",
      via the `svn copy' command. Each test case's subdir now has two sub-
      dirs: "local" and "incoming", initial states for the merge operation.

   4) An update is performed across all test cases and depths, so that the
      copies made in 3) are pulled into the wc.

   5) In each test case's "incoming" subdir, the incoming action is
      performed.

   6) A commit is performed across all test cases and depths, to commit
      the incoming changes.
      If do_commit_local_changes is True, this becomes step 7 (swap steps).

   7) In each test case's "local" subdir, the local_action is performed.
      If do_commit_local_changes is True, this becomes step 6 (swap steps).
      Then, in effect, the local changes are committed as well.

   8) In each test case subdir, the "incoming" subdir is merged into the
      "local" subdir.  If ignore_ancestry is True, then the merge is done
      with the --ignore-ancestry option, so mergeinfo is neither considered
      nor recorded.  This causes conflicts between the "local" state in the
      working copy and the "incoming" state from the incoming subdir.

   9) If do_commit_conflicts is True, then a commit is performed in each
      separate container, to verify that each tree-conflict indeed blocks
      a commit.

  The sbox parameter is just the sbox passed to a test function. No need
  to call sbox.build(), since it is called (once) within this function.

  The "table" greater_scheme models all of the different test cases
  that should be run using a single repository.

  greater_scheme is a list of DeepTreesTestCase items, which define complete
  test setups, so that they can be performed as described above.
  """

  j = os.path.join

  if not sbox.is_built():
    sbox.build()
  wc_dir = sbox.wc_dir

  # 1) Create directories.
  for test_case in greater_scheme:
    try:
      base = j(sbox.wc_dir, test_case.name)
      os.makedirs(base)
      make_deep_trees(j(base, "incoming"))
      main.run_svn(None, 'add', base)
    except:
      print("ERROR IN: Tests scheme for merge: "
          + "while setting up deep trees in '%s'" % test_case.name)
      raise


  # 2) Commit pre-initial state (-r2).

  main.run_svn(None, 'commit', '-m', 'pre-initial state', wc_dir)


  # 3) Copy "incoming" to "local".

  for test_case in greater_scheme:
    try:
      base_url = sbox.repo_url + "/" + test_case.name
      incoming_url = base_url + "/incoming"
      local_url = base_url + "/local"
      main.run_svn(None, 'cp', incoming_url, local_url, '-m',
                   'copy incoming to local')
    except:
      print("ERROR IN: Tests scheme for merge: "
          + "while copying deep trees in '%s'" % test_case.name)
      raise

  # 4) Update to load all of the "/local" subdirs into the working copies.

  try:
    main.run_svn(None, 'up', sbox.wc_dir)
  except:
    print("ERROR IN: Tests scheme for merge: "
          + "while updating local subdirs")
    raise


  # 5) Perform incoming actions

  for test_case in greater_scheme:
    try:
      test_case.incoming_action(j(sbox.wc_dir, test_case.name, "incoming"))
    except:
      print("ERROR IN: Tests scheme for merge: "
          + "while performing incoming action in '%s'" % test_case.name)
      raise


  # 6) or 7) Commit all incoming actions

  if not do_commit_local_changes:
    try:
      main.run_svn(None, 'ci', '-m', 'Committing incoming actions',
                   sbox.wc_dir)
    except:
      print("ERROR IN: Tests scheme for merge: "
          + "while committing incoming actions")
      raise


  # 7) or 6) Perform all local actions.

  for test_case in greater_scheme:
    try:
      test_case.local_action(j(sbox.wc_dir, test_case.name, "local"))
    except:
      print("ERROR IN: Tests scheme for merge: "
          + "while performing local action in '%s'" % test_case.name)
      raise


  # 6) or 7) Commit all incoming actions

  if do_commit_local_changes:
    try:
      main.run_svn(None, 'ci', '-m', 'Committing incoming and local actions',
                   sbox.wc_dir)
    except:
      print("ERROR IN: Tests scheme for merge: "
          + "while committing incoming and local actions")
      raise


  # 8) Merge all "incoming" subdirs to their respective "local" subdirs.
  #    This creates conflicts between the local changes in the "local" wc
  #    subdirs and the incoming states committed in the "incoming" subdirs.

  for test_case in greater_scheme:
    try:
      local = j(sbox.wc_dir, test_case.name, "local")
      incoming = sbox.repo_url + "/" + test_case.name + "/incoming"

      x_out = test_case.expected_output
      if x_out != None:
        x_out = x_out.copy()
        x_out.wc_dir = local

      x_disk = test_case.expected_disk

      x_status = test_case.expected_status
      if x_status != None:
        x_status.copy()
        x_status.wc_dir = local

      x_skip = test_case.expected_skip
      if x_skip != None:
        x_skip.copy()
        x_skip.wc_dir = local

      varargs = (local,)
      if ignore_ancestry:
        varargs = varargs + ('--ignore-ancestry',)

      run_and_verify_merge(local, None, None, incoming, None,
                           x_out, None, None, x_disk, None, x_skip,
                           test_case.error_re_string,
                           None, None, None, None,
                           False, False, *varargs)
      run_and_verify_unquiet_status(local, x_status)
    except:
      print("ERROR IN: Tests scheme for merge: "
          + "while verifying in '%s'" % test_case.name)
      raise


  # 9) Verify that commit fails.

  if do_commit_conflicts:
    for test_case in greater_scheme:
      try:
        local = j(wc_dir, test_case.name, 'local')

        x_status = test_case.expected_status
        if x_status != None:
          x_status.copy()
          x_status.wc_dir = local

        run_and_verify_commit(local, None, x_status,
                              test_case.commit_block_string,
                              local)
      except:
        print("ERROR IN: Tests scheme for merge: "
            + "while checking commit-blocking in '%s'" % test_case.name)
        raise



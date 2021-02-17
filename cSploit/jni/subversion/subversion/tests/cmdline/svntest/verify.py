#
#  verify.py:  routines that handle comparison and display of expected
#              vs. actual output
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

import re, sys
from difflib import unified_diff, ndiff
import pprint

import svntest


######################################################################
# Exception types

class SVNUnexpectedOutput(svntest.Failure):
  """Exception raised if an invocation of svn results in unexpected
  output of any kind."""
  pass

class SVNUnexpectedStdout(SVNUnexpectedOutput):
  """Exception raised if an invocation of svn results in unexpected
  output on STDOUT."""
  pass

class SVNUnexpectedStderr(SVNUnexpectedOutput):
  """Exception raised if an invocation of svn results in unexpected
  output on STDERR."""
  pass

class SVNExpectedStdout(SVNUnexpectedOutput):
  """Exception raised if an invocation of svn results in no output on
  STDOUT when output was expected."""
  pass

class SVNExpectedStderr(SVNUnexpectedOutput):
  """Exception raised if an invocation of svn results in no output on
  STDERR when output was expected."""
  pass

class SVNUnexpectedExitCode(SVNUnexpectedOutput):
  """Exception raised if an invocation of svn exits with a value other
  than what was expected."""
  pass

class SVNIncorrectDatatype(SVNUnexpectedOutput):
  """Exception raised if invalid input is passed to the
  run_and_verify_* API"""
  pass

class SVNDumpParseError(svntest.Failure):
  """Exception raised if parsing a dump file fails"""
  pass


######################################################################
# Comparison of expected vs. actual output

def createExpectedOutput(expected, output_type, match_all=True):
  """Return EXPECTED, promoted to an ExpectedOutput instance if not
  None.  Raise SVNIncorrectDatatype if the data type of EXPECTED is
  not handled."""
  if isinstance(expected, list):
    expected = ExpectedOutput(expected)
  elif isinstance(expected, str):
    expected = RegexOutput(expected, match_all)
  elif isinstance(expected, int):
    expected = RegexOutput(".*: E%d:.*" % expected, False)
  elif expected is AnyOutput:
    expected = AnyOutput()
  elif expected is not None and not isinstance(expected, ExpectedOutput):
    raise SVNIncorrectDatatype("Unexpected type for '%s' data" % output_type)
  return expected

class ExpectedOutput:
  """Contains expected output, and performs comparisons."""

  is_regex = False
  is_unordered = False

  def __init__(self, output, match_all=True):
    """Initialize the expected output to OUTPUT which is a string, or a list
    of strings, or None meaning an empty list. If MATCH_ALL is True, the
    expected strings will be matched with the actual strings, one-to-one, in
    the same order. If False, they will be matched with a subset of the
    actual strings, one-to-one, in the same order, ignoring any other actual
    strings among the matching ones."""
    self.output = output
    self.match_all = match_all

  def __str__(self):
    return str(self.output)

  def __cmp__(self, other):
    raise Exception('badness')

  def matches(self, other, except_re=None):
    """Return whether SELF.output matches OTHER (which may be a list
    of newline-terminated lines, or a single string).  Either value
    may be None."""
    if self.output is None:
      expected = []
    else:
      expected = self.output
    if other is None:
      actual = []
    else:
      actual = other

    if not isinstance(actual, list):
      actual = [actual]
    if not isinstance(expected, list):
      expected = [expected]

    if except_re:
      return self.matches_except(expected, actual, except_re)
    else:
      return self.is_equivalent_list(expected, actual)

  def matches_except(self, expected, actual, except_re):
    "Return whether EXPECTED and ACTUAL match except for except_re."
    if not self.is_regex:
      i_expected = 0
      i_actual = 0
      while i_expected < len(expected) and i_actual < len(actual):
        if re.match(except_re, actual[i_actual]):
          i_actual += 1
        elif re.match(except_re, expected[i_expected]):
          i_expected += 1
        elif expected[i_expected] == actual[i_actual]:
          i_expected += 1
          i_actual += 1
        else:
          return False
      if i_expected == len(expected) and i_actual == len(actual):
            return True
      return False
    else:
      raise Exception("is_regex and except_re are mutually exclusive")

  def is_equivalent_list(self, expected, actual):
    "Return whether EXPECTED and ACTUAL are equivalent."
    if not self.is_regex:
      if self.match_all:
        # The EXPECTED lines must match the ACTUAL lines, one-to-one, in
        # the same order.
        return expected == actual

      # The EXPECTED lines must match a subset of the ACTUAL lines,
      # one-to-one, in the same order, with zero or more other ACTUAL
      # lines interspersed among the matching ACTUAL lines.
      i_expected = 0
      for actual_line in actual:
        if expected[i_expected] == actual_line:
          i_expected += 1
          if i_expected == len(expected):
            return True
      return False

    expected_re = expected[0]
    # If we want to check that every line matches the regexp
    # assume they all match and look for any that don't.  If
    # only one line matching the regexp is enough, assume none
    # match and look for even one that does.
    if self.match_all:
      all_lines_match_re = True
    else:
      all_lines_match_re = False

    # If a regex was provided assume that we actually require
    # some output. Fail if we don't have any.
    if len(actual) == 0:
      return False

    for actual_line in actual:
      if self.match_all:
        if not re.match(expected_re, actual_line):
          return False
      else:
        # As soon an actual_line matches something, then we're good.
        if re.match(expected_re, actual_line):
          return True

    return all_lines_match_re

  def display_differences(self, message, label, actual):
    """Delegate to the display_lines() routine with the appropriate
    args.  MESSAGE is ignored if None."""
    display_lines(message, label, self.output, actual,
                  self.is_regex, self.is_unordered)


class AnyOutput(ExpectedOutput):
  def __init__(self):
    ExpectedOutput.__init__(self, None, False)

  def is_equivalent_list(self, ignored, actual):
    if len(actual) == 0:
      # No actual output. No match.
      return False

    for line in actual:
      # If any line has some text, then there is output, so we match.
      if line:
        return True

    # We did not find a line with text. No match.
    return False

  def display_differences(self, message, label, actual):
    if message:
      print(message)


class RegexOutput(ExpectedOutput):
  is_regex = True


class UnorderedOutput(ExpectedOutput):
  """Marks unordered output, and performs comparisons."""

  is_unordered = True

  def __cmp__(self, other):
    raise Exception('badness')

  def matches_except(self, expected, actual, except_re):
    assert type(actual) == type([]) # ### if this trips: fix it!
    return self.is_equivalent_list([l for l in expected if not except_re.match(l)],
                                   [l for l in actual if not except_re.match(l)])

  def is_equivalent_list(self, expected, actual):
    "Disregard the order of ACTUAL lines during comparison."

    e_set = set(expected)
    a_set = set(actual)

    if self.match_all:
      if len(e_set) != len(a_set):
        return False
      if self.is_regex:
        for expect_re in e_set:
          for actual_line in a_set:
            if re.match(expect_re, actual_line):
              a_set.remove(actual_line)
              break
          else:
            # One of the regexes was not found
            return False
        return True

      # All expected lines must be in the output.
      return e_set == a_set

    if self.is_regex:
      # If any of the expected regexes are in the output, then we match.
      for expect_re in e_set:
        for actual_line in a_set:
          if re.match(expect_re, actual_line):
            return True
      return False

    # If any of the expected lines are in the output, then we match.
    return len(e_set.intersection(a_set)) > 0


class UnorderedRegexOutput(UnorderedOutput, RegexOutput):
  is_regex = True
  is_unordered = True


######################################################################
# Displaying expected and actual output

def display_trees(message, label, expected, actual):
  'Print two trees, expected and actual.'
  if message is not None:
    print(message)
  if expected is not None:
    print('EXPECTED %s:' % label)
    svntest.tree.dump_tree(expected)
  if actual is not None:
    print('ACTUAL %s:' % label)
    svntest.tree.dump_tree(actual)


def display_lines(message, label, expected, actual, expected_is_regexp=None,
                  expected_is_unordered=None):
  """Print MESSAGE, unless it is None, then print EXPECTED (labeled
  with LABEL) followed by ACTUAL (also labeled with LABEL).
  Both EXPECTED and ACTUAL may be strings or lists of strings."""
  if message is not None:
    print(message)
  if expected is not None:
    output = 'EXPECTED %s' % label
    if expected_is_regexp:
      output += ' (regexp)'
      expected = [expected + '\n']
    if expected_is_unordered:
      output += ' (unordered)'
    output += ':'
    print(output)
    for x in expected:
      sys.stdout.write(x)
  if actual is not None:
    print('ACTUAL %s:' % label)
    for x in actual:
      sys.stdout.write(x)

  # Additionally print unified diff
  if not expected_is_regexp:
    print('DIFF ' + ' '.join(output.split(' ')[1:]))

    if type(expected) is str:
      expected = [expected]

    if type(actual) is str:
      actual = [actual]

    for x in unified_diff(expected, actual,
                          fromfile="EXPECTED %s" % label,
                          tofile="ACTUAL %s" % label):
      sys.stdout.write(x)

def compare_and_display_lines(message, label, expected, actual,
                              raisable=None, except_re=None):
  """Compare two sets of output lines, and print them if they differ,
  preceded by MESSAGE iff not None.  EXPECTED may be an instance of
  ExpectedOutput (and if not, it is wrapped as such).  RAISABLE is an
  exception class, an instance of which is thrown if ACTUAL doesn't
  match EXPECTED."""
  if raisable is None:
    raisable = svntest.main.SVNLineUnequal
  ### It'd be nicer to use createExpectedOutput() here, but its
  ### semantics don't match all current consumers of this function.
  if not isinstance(expected, ExpectedOutput):
    expected = ExpectedOutput(expected)

  if isinstance(actual, str):
    actual = [actual]
  actual = [line for line in actual if not line.startswith('DBG:')]

  if not expected.matches(actual, except_re):
    expected.display_differences(message, label, actual)
    raise raisable

def verify_outputs(message, actual_stdout, actual_stderr,
                   expected_stdout, expected_stderr, all_stdout=True):
  """Compare and display expected vs. actual stderr and stdout lines:
  if they don't match, print the difference (preceded by MESSAGE iff
  not None) and raise an exception.

  If EXPECTED_STDERR or EXPECTED_STDOUT is a string the string is
  interpreted as a regular expression.  For EXPECTED_STDOUT and
  ACTUAL_STDOUT to match, every line in ACTUAL_STDOUT must match the
  EXPECTED_STDOUT regex, unless ALL_STDOUT is false.  For
  EXPECTED_STDERR regexes only one line in ACTUAL_STDERR need match."""
  expected_stderr = createExpectedOutput(expected_stderr, 'stderr', False)
  expected_stdout = createExpectedOutput(expected_stdout, 'stdout', all_stdout)

  for (actual, expected, label, raisable) in (
      (actual_stderr, expected_stderr, 'STDERR', SVNExpectedStderr),
      (actual_stdout, expected_stdout, 'STDOUT', SVNExpectedStdout)):
    if expected is None:
      continue

    if isinstance(expected, RegexOutput):
      raisable = svntest.main.SVNUnmatchedError
    elif not isinstance(expected, AnyOutput):
      raisable = svntest.main.SVNLineUnequal

    compare_and_display_lines(message, label, expected, actual, raisable)

def verify_exit_code(message, actual, expected,
                     raisable=SVNUnexpectedExitCode):
  """Compare and display expected vs. actual exit codes:
  if they don't match, print the difference (preceded by MESSAGE iff
  not None) and raise an exception."""

  if expected != actual:
    display_lines(message, "Exit Code",
                  str(expected) + '\n', str(actual) + '\n')
    raise raisable

# A simple dump file parser.  While sufficient for the current
# testsuite it doesn't cope with all valid dump files.
class DumpParser:
  def __init__(self, lines):
    self.current = 0
    self.lines = lines
    self.parsed = {}

  def parse_line(self, regex, required=True):
    m = re.match(regex, self.lines[self.current])
    if not m:
      if required:
        raise SVNDumpParseError("expected '%s' at line %d\n%s"
                                % (regex, self.current,
                                   self.lines[self.current]))
      else:
        return None
    self.current += 1
    return m.group(1)

  def parse_blank(self, required=True):
    if self.lines[self.current] != '\n':  # Works on Windows
      if required:
        raise SVNDumpParseError("expected blank at line %d\n%s"
                                % (self.current, self.lines[self.current]))
      else:
        return False
    self.current += 1
    return True

  def parse_format(self):
    return self.parse_line('SVN-fs-dump-format-version: ([0-9]+)$')

  def parse_uuid(self):
    return self.parse_line('UUID: ([0-9a-z-]+)$')

  def parse_revision(self):
    return self.parse_line('Revision-number: ([0-9]+)$')

  def parse_prop_length(self, required=True):
    return self.parse_line('Prop-content-length: ([0-9]+)$', required)

  def parse_content_length(self, required=True):
    return self.parse_line('Content-length: ([0-9]+)$', required)

  def parse_path(self):
    path = self.parse_line('Node-path: (.+)$', required=False)
    if not path and self.lines[self.current] == 'Node-path: \n':
      self.current += 1
      path = ''
    return path

  def parse_kind(self):
    return self.parse_line('Node-kind: (.+)$', required=False)

  def parse_action(self):
    return self.parse_line('Node-action: ([0-9a-z-]+)$')

  def parse_copyfrom_rev(self):
    return self.parse_line('Node-copyfrom-rev: ([0-9]+)$', required=False)

  def parse_copyfrom_path(self):
    path = self.parse_line('Node-copyfrom-path: (.+)$', required=False)
    if not path and self.lines[self.current] == 'Node-copyfrom-path: \n':
      self.current += 1
      path = ''
    return path

  def parse_copy_md5(self):
    return self.parse_line('Text-copy-source-md5: ([0-9a-z]+)$', required=False)

  def parse_copy_sha1(self):
    return self.parse_line('Text-copy-source-sha1: ([0-9a-z]+)$', required=False)

  def parse_text_md5(self):
    return self.parse_line('Text-content-md5: ([0-9a-z]+)$', required=False)

  def parse_text_sha1(self):
    return self.parse_line('Text-content-sha1: ([0-9a-z]+)$', required=False)

  def parse_text_length(self):
    return self.parse_line('Text-content-length: ([0-9]+)$', required=False)

  # One day we may need to parse individual property name/values into a map
  def get_props(self):
    props = []
    while not re.match('PROPS-END$', self.lines[self.current]):
      props.append(self.lines[self.current])
      self.current += 1
    self.current += 1
    return props

  def get_content(self, length):
    content = ''
    while len(content) < length:
      content += self.lines[self.current]
      self.current += 1
    if len(content) == length + 1:
      content = content[:-1]
    elif len(content) != length:
      raise SVNDumpParseError("content length expected %d actual %d at line %d"
                              % (length, len(content), self.current))
    return content

  def parse_one_node(self):
    node = {}
    node['kind'] = self.parse_kind()
    action = self.parse_action()
    node['copyfrom_rev'] = self.parse_copyfrom_rev()
    node['copyfrom_path'] = self.parse_copyfrom_path()
    node['copy_md5'] = self.parse_copy_md5()
    node['copy_sha1'] = self.parse_copy_sha1()
    node['prop_length'] = self.parse_prop_length(required=False)
    node['text_length'] = self.parse_text_length()
    node['text_md5'] = self.parse_text_md5()
    node['text_sha1'] = self.parse_text_sha1()
    node['content_length'] = self.parse_content_length(required=False)
    self.parse_blank()
    if node['prop_length']:
      node['props'] = self.get_props()
    if node['text_length']:
      node['content'] = self.get_content(int(node['text_length']))
    # Hard to determine how may blanks is 'correct' (a delete that is
    # followed by an add that is a replace and a copy has one fewer
    # than expected but that can't be predicted until seeing the add)
    # so allow arbitrary number
    blanks = 0
    while self.current < len(self.lines) and self.parse_blank(required=False):
      blanks += 1
    node['blanks'] = blanks
    return action, node

  def parse_all_nodes(self):
    nodes = {}
    while True:
      if self.current >= len(self.lines):
        break
      path = self.parse_path()
      if not path and not path is '':
        break
      if not nodes.get(path):
        nodes[path] = {}
      action, node = self.parse_one_node()
      if nodes[path].get(action):
        raise SVNDumpParseError("duplicate action '%s' for node '%s' at line %d"
                                % (action, path, self.current))
      nodes[path][action] = node
    return nodes

  def parse_one_revision(self):
    revision = {}
    number = self.parse_revision()
    revision['prop_length'] = self.parse_prop_length()
    revision['content_length'] = self.parse_content_length()
    self.parse_blank()
    revision['props'] = self.get_props()
    self.parse_blank()
    revision['nodes'] = self.parse_all_nodes()
    return number, revision

  def parse_all_revisions(self):
    while self.current < len(self.lines):
      number, revision = self.parse_one_revision()
      if self.parsed.get(number):
        raise SVNDumpParseError("duplicate revision %d at line %d"
                                % (number, self.current))
      self.parsed[number] = revision

  def parse(self):
    self.parsed['format'] = self.parse_format()
    self.parse_blank()
    self.parsed['uuid'] = self.parse_uuid()
    self.parse_blank()
    self.parse_all_revisions()
    return self.parsed

def compare_dump_files(message, label, expected, actual):
  """Parse two dump files EXPECTED and ACTUAL, both of which are lists
  of lines as returned by run_and_verify_dump, and check that the same
  revisions, nodes, properties, etc. are present in both dumps.
  """

  parsed_expected = DumpParser(expected).parse()
  parsed_actual = DumpParser(actual).parse()

  if parsed_expected != parsed_actual:
    raise svntest.Failure('\n' + '\n'.join(ndiff(
          pprint.pformat(parsed_expected).splitlines(),
          pprint.pformat(parsed_actual).splitlines())))

#!/usr/bin/env python
#
#  info_tests.py:  testing the svn info command
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

# See basic-tests.py for more svn info tests.

# General modules
import shutil, stat, re, os

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

######################################################################
# Tests
#
#   Each test must return on success or raise on failure.

#----------------------------------------------------------------------

# Helpers for XML output
def verify_xml_elements(lines, exprs):
  """Verify that each of the given expressions matches exactly one XML
     element in the list of lines. Each expression is a tuple containing
     a name (a string), a set of attribute name-value pairs (a dict of
     string->string), and element content (a string).  The attribute dict
     and the content string are by default empty.

     Expression format: [ ('name', {'att': 'val', ...}, 'text') , ...]

     Limitations:
     We don't verify that the input is a valid XML document.
     We can't verify text mixed with child elements.
     We don't handle XML comments.
     All of these are taken care of by the Relax NG schemas.
  """
  xml_version_re = re.compile(r"<\?xml\s+[^?]+\?>")

  str = ""
  for line in lines:
    str += line
  m = xml_version_re.match(str)
  if m:
    str = str[m.end():] # skip xml version tag
  (unmatched_str, unmatched_exprs) = match_xml_element(str, exprs)
  if unmatched_exprs:
    print("Failed to find the following expressions:")
    for expr in unmatched_exprs:
      print(expr)
    raise svntest.tree.SVNTreeUnequal

def match_xml_element(str, exprs):
  """Read from STR until the start of an element. If no element is found,
     return the arguments.  Get the element name, attributes and text content.
     If not empty, call recursively on the text content.  Compare the current
     element to all expressions in EXPRS.  If no elements were found in the
     current element's text, include the text in the comparison (i.e., we
     don't support mixed content).  Return the unmatched part of the string
     and any unmatched expressions.
  """
  start_tag_re = re.compile(r"[^<]*<(?P<name>[\w-]+)", re.M)
  atttribute_re = re.compile(
                 r"\s+(?P<key>[\w-]+)\s*=\s*(['\"])(?P<val>[^'\"]*)\2", re.M)
  self_closing_re = re.compile(r"\s*/>", re.M)
  content_re_str = "\\s*>(?P<content>.*?)</%s\s*>"

  m = start_tag_re.match(str)
  if not m:
    return (str, exprs)
  name = m.group('name')
  str = str[m.end():]
  atts = {}
  while True:
    m = atttribute_re.match(str)
    if not m:
      break
    else:
      atts[m.group('key')] = m.group('val')
      str = str[m.end():]
  m = self_closing_re.match(str)
  if m:
    content = ''
    str = str[m.end():]
  else:
    content_re = re.compile(content_re_str % name, re.DOTALL)
    m = content_re.match(str)
    if not m:
      print("No XML end-tag for '%s' found in '%s...'" % (name, str[:100]))
      raise(svntest.tree.SVNTreeUnequal)
    content = m.group('content')
    str = str[m.end():]
  if content != '':
    while True:
      (new_content, exprs) = match_xml_element(content, exprs)
      if new_content == content:
        # there are no (more) child elements
        break
      else:
        content = new_content
  if exprs:
    for expr in exprs:
      # compare element names
      e_name = expr[0]
      if (e_name != name):
        continue
      # compare element attributes
      e_atts = {}
      if len(expr) > 1:
        e_atts = expr[1]
      if e_atts != atts:
        continue
      # compare element content (text only)
      e_content = ''
      if len(expr) > 2:
        e_content = expr[2]
      if (not re.search(e_content, content)):
        continue
      # success!
      exprs.remove(expr)
  return (str, exprs)

def info_with_tree_conflicts(sbox):
  "info with tree conflicts"

  # Info messages reflecting tree conflict status.
  # These tests correspond to use cases 1-3 in
  # notes/tree-conflicts/use-cases.txt.

  svntest.actions.build_greek_tree_conflicts(sbox)
  wc_dir = sbox.wc_dir
  G = os.path.join(wc_dir, 'A', 'D', 'G')

  scenarios = [
    # (filename, action, reason)
    ('pi',  'edit',   'delete'),
    ('rho', 'delete', 'edit'),
    ('tau', 'delete', 'delete'),
    ]

  for fname, action, reason in scenarios:
    path = os.path.join(G, fname)

    # check plain info
    expected_str1 = ".*local %s, incoming %s.*" % (reason, action)
    expected_info = { 'Tree conflict' : expected_str1 }
    svntest.actions.run_and_verify_info([expected_info], path)

    # check XML info
    exit_code, output, error = svntest.actions.run_and_verify_svn(None, None,
                                                                  [], 'info',
                                                                  path,
                                                                  '--xml')

    # In the XML, action and reason are past tense: 'edited' not 'edit'.
    verify_xml_elements(output,
                        [('tree-conflict', {'victim'   : fname,
                                            'kind'     : 'file',
                                            'operation': 'update',
                                            'action'   : action,
                                            'reason'   : reason,
                                            },
                          )])

  # Check recursive info.
  expected_infos = [{ 'Path' : re.escape(G) }]
  for fname, action, reason in scenarios:
    path = os.path.join(G, fname)
    tree_conflict_re = ".*local %s, incoming %s.*" % (reason, action)
    expected_infos.append({ 'Path' : re.escape(path),
                            'Tree conflict' : tree_conflict_re })
  expected_infos.sort(key=lambda info: info['Path'])
  svntest.actions.run_and_verify_info(expected_infos, G, '-R')

def info_on_added_file(sbox):
  """info on added file"""

  svntest.actions.make_repo_and_wc(sbox)
  wc_dir = sbox.wc_dir

  # create new file
  new_file = os.path.join(wc_dir, 'new_file')
  svntest.main.file_append(new_file, '')

  svntest.main.run_svn(None, 'add', new_file)

  uuid_regex = '[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}'

  # check that we have a Repository Root and Repository UUID
  expected = {'Path' : re.escape(new_file),
              'Name' : 'new_file',
              'URL' : '.*/new_file',
              'Repository Root' : '.*',
              'Node Kind' : 'file',
              'Schedule' : 'add',
              'Repository UUID' : uuid_regex,
             }

  svntest.actions.run_and_verify_info([expected], new_file)

  # check XML info
  exit_code, output, error = svntest.actions.run_and_verify_svn(None, None,
                                                                [], 'info',
                                                                new_file,
                                                                '--xml')

  verify_xml_elements(output,
                      [('entry',    {'kind'     : 'file',
                                     'path'     : new_file,
                                     'revision' : 'Resource is not under version control.'}),
                       ('url',      {}, '.*/new_file'),
                       ('root',     {}, '.*'),
                       ('uuid',     {}, uuid_regex),
                       ('depth',    {}, 'infinity'),
                       ('schedule', {}, 'add')])

def info_on_mkdir(sbox):
  """info on new dir with mkdir"""
  svntest.actions.make_repo_and_wc(sbox)
  wc_dir = sbox.wc_dir

  # create a new directory using svn mkdir
  new_dir = os.path.join(wc_dir, 'new_dir')
  svntest.main.run_svn(None, 'mkdir', new_dir)

  uuid_regex = '[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}'

  # check that we have a Repository Root and Repository UUID
  expected = {'Path' : re.escape(new_dir),
              'URL' : '.*/new_dir',
              'Repository Root' : '.*',
              'Node Kind' : 'directory',
              'Schedule' : 'add',
              'Repository UUID' : uuid_regex,
             }

  svntest.actions.run_and_verify_info([expected], new_dir)

  # check XML info
  exit_code, output, error = svntest.actions.run_and_verify_svn(None, None,
                                                                [], 'info',
                                                                new_dir,
                                                                '--xml')
  verify_xml_elements(output,
                      [('entry',    {'kind'     : 'dir',
                                     'path'     : new_dir,
                                     'revision' : 'Resource is not under version control.'}),
                       ('url',      {}, '.*/new_dir'),
                       ('root',     {}, '.*'),
                       ('uuid',     {}, uuid_regex),
                       ('depth',    {}, 'infinity'),
                       ('schedule', {}, 'add')])

def info_wcroot_abspaths(sbox):
  """wc root paths in 'svn info' output"""

  def check_wcroot_paths(lines, wcroot_abspath):
    "check that paths found on input lines beginning 'Path: ' are as expected"
    path = None
    target = None
    for line in lines:
      if line.startswith('Path: '):
        target = line[6:].rstrip()
      if line.startswith('Working Copy Root Path: '):
        path = line[24:].rstrip()
      if target is not None and path is not None:
        break

    if target is None:
      target = "(UNKNOWN)"

    if path is None:
      print "No WC root path for '%s'" % (target)
      raise svntest.Failure

    if path != wcroot_abspath:
      print("For target '%s'..." % (target))
      print("   Reported WC root path: %s" % (path))
      print("   Expected WC root path: %s" % (wcroot_abspath))
      raise svntest.Failure

  sbox.build(read_only=True)
  exit_code, output, errput = svntest.main.run_svn(None, 'info', '-R', sbox.wc_dir)
  check_wcroot_paths(output, os.path.abspath(sbox.wc_dir))

def info_url_special_characters(sbox):
  """special characters in svn info URL"""
  sbox.build(create_wc = False)
  wc_dir = sbox.wc_dir

  special_urls = [sbox.repo_url + '/A' + '/%2E',
                  sbox.repo_url + '%2F' + 'A']

  expected = {'Path' : 'A',
              'Repository Root' : re.escape(sbox.repo_url),
              'Revision' : '1',
              'Node Kind' : 'dir',
             }

  for url in special_urls:
    svntest.actions.run_and_verify_info([expected], url)

def info_multiple_targets(sbox):
  "info multiple targets"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  def multiple_wc_targets():
    "multiple wc targets"

    alpha = sbox.ospath('A/B/E/alpha')
    beta = sbox.ospath('A/B/E/beta')
    non_existent_path = os.path.join(wc_dir, 'non-existent')

    # All targets are existing
    svntest.actions.run_and_verify_svn2(None, None, [],
                                        0, 'info', alpha, beta)

    # One non-existing target
    expected_err = ".*W155010.*\n\n.*E200009.*"
    expected_err_re = re.compile(expected_err, re.DOTALL)

    exit_code, output, error = svntest.main.run_svn(1, 'info', alpha,
                                                    non_existent_path, beta)

    # Verify error
    if not expected_err_re.match("".join(error)):
      raise svntest.Failure('info failed: expected error "%s", but received '
                            '"%s"' % (expected_err, "".join(error)))

  def multiple_url_targets():
    "multiple url targets"

    alpha = sbox.repo_url +  '/A/B/E/alpha'
    beta = sbox.repo_url +  '/A/B/E/beta'
    non_existent_url = sbox.repo_url +  '/non-existent'

    # All targets are existing
    svntest.actions.run_and_verify_svn2(None, None, [],
                                        0, 'info', alpha, beta)

    # One non-existing target
    expected_err = ".*W170000.*\n\n.*E200009.*"
    expected_err_re = re.compile(expected_err, re.DOTALL)

    exit_code, output, error = svntest.main.run_svn(1, 'info', alpha,
                                                    non_existent_url, beta)

    # Verify error
    if not expected_err_re.match("".join(error)):
      raise svntest.Failure('info failed: expected error "%s", but received '
                            '"%s"' % (expected_err, "".join(error)))
  # Test one by one
  multiple_wc_targets()
  multiple_url_targets()

def info_repos_root_url(sbox):
  """verify values for repository root"""
  sbox.build(create_wc = False)
  wc_dir = sbox.wc_dir

  expected_info = [
    {
        'Path'              : re.escape(os.path.basename(sbox.repo_dir)),
        'Repository Root'   : re.escape(sbox.repo_url),
        'URL'               : re.escape(sbox.repo_url),
        'Revision'          : '1',
        'Node Kind'         : 'directory',
        'Last Changed Rev'  : '1',
    },
    {
        'Path'              : 'iota',
        'Name'              : 'iota',
        'Repository Root'   : re.escape(sbox.repo_url),
        'URL'               : re.escape(sbox.repo_url + '/iota'),
        'Revision'          : '1',
        'Node Kind'         : 'file',
        'Last Changed Rev'  : '1',
    }
  ]

  svntest.actions.run_and_verify_info(expected_info, sbox.repo_url,
                                      '--depth', 'files')

@Issue(3787)
def info_show_exclude(sbox):
  "tests 'info --depth' variants on excluded node"

  sbox.build()
  wc_dir = sbox.wc_dir

  A_path = os.path.join(wc_dir, 'A')
  iota = os.path.join(wc_dir, 'iota')
  svntest.main.run_svn(None, 'up', '--set-depth', 'exclude', A_path)
  wc_uuid = svntest.actions.get_wc_uuid(wc_dir)

  expected_info = [{
      'Path' : re.escape(wc_dir),
      'Repository Root' : sbox.repo_url,
      'Repository UUID' : wc_uuid,
  }]

  svntest.actions.run_and_verify_info(expected_info, '--depth', 'empty',
                                      wc_dir)

  expected_info = [{
      'Path' : '.*%sA' % re.escape(os.sep),
      'Repository Root' : sbox.repo_url,
      'Repository UUID' : wc_uuid,
      'Depth' : 'exclude',
  }]

  svntest.actions.run_and_verify_info(expected_info, '--depth',
                                      'empty', A_path)
  svntest.actions.run_and_verify_info(expected_info, '--depth',
                                      'infinity', A_path)
  svntest.actions.run_and_verify_info(expected_info, '--depth',
                                      'immediates', A_path)

  expected_info = [{
      'Path' : '.*%siota' % re.escape(os.sep),
     'Repository Root' : sbox.repo_url,
     'Repository UUID' : wc_uuid,
  }]
  svntest.main.run_svn(None, 'up', '--set-depth', 'exclude', iota)
  svntest.actions.run_and_verify_info(expected_info, iota)

  # And now get iota back, to allow testing other states
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='A '),
    })

  expected_status = svntest.wc.State(iota, {
    '' : Item(status='  ', wc_rev='1')
  })

  svntest.actions.run_and_verify_update(iota,
                                        expected_output, None, expected_status)

  sbox.simple_rm('iota')
  sbox.simple_commit()

  # Expect error on iota (status = not-present)
  svntest.actions.run_and_verify_svn(None, [],
       'svn: E200009: Could not display info for all targets.*',
        'info', iota)

  sbox.simple_update()

  # Expect error on iota (unversioned)
  svntest.actions.run_and_verify_svn(None, [],
       'svn: E200009: Could not display info for all targets.*',
        'info', iota)

@Issue(3998)
def binary_tree_conflict(sbox):
  "svn info shouldn't crash on conflict"
  sbox.build()
  wc_dir = sbox.wc_dir

  sbox.simple_propset('svn:mime-type', 'binary/octet-stream', 'iota')
  sbox.simple_commit()

  iota = sbox.ospath('iota')

  svntest.main.file_write(iota, 'something-else')
  sbox.simple_commit()

  svntest.main.file_write(iota, 'third')

  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='C '),
    })
  expected_status = svntest.wc.State(iota, {
    '' : Item(status='C ', wc_rev='2')
  })
  svntest.actions.run_and_verify_update(iota,
                                        expected_output, None, expected_status,
                                        None, None, None, None, None, False,
                                        iota, '-r', '2')

  expected_info = [{
      'Path' : '%s' % re.escape(iota),
      'Conflict Previous Base File' : re.escape(iota + '.r3'),
      'Conflict Current Base File' : re.escape(iota + '.r2'),
  }]
  svntest.actions.run_and_verify_info(expected_info, iota)




########################################################################
# Run the tests

# list all tests here, starting with None:
test_list = [ None,
              info_with_tree_conflicts,
              info_on_added_file,
              info_on_mkdir,
              info_wcroot_abspaths,
              info_url_special_characters,
              info_multiple_targets,
              info_repos_root_url,
              info_show_exclude,
              binary_tree_conflict,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.

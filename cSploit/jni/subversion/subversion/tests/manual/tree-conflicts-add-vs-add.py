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

# Setup your environment so that `which svn` shows the svn you want to test.
# Just run this file, no parameters. Test files are created in /tmp/...
# To adjust which tests are run, look at 'p = Permutations(' way below...
#
# This runs an insane amount of tests of add-vs.-add situations during update
# and switch. The output scrolls by, and a summary for all tests with simple
# greps and wc.db SELECT results is printed in the very end.
#
# There is no automatic validation. You have to read the results.
#
# To run a gdb in any given place, replace a 'svn()' with 'gdbsvn()',
# presumably in either up() or sw().

from subprocess import Popen, PIPE, call
from types import FunctionType, ListType, TupleType
import tempfile, os
from itertools import product


def run_cmd(cmd, verbose=True, shell=False):
  if verbose:
    if shell:
      print '\n---', cmd
    else:
      print '\n---', ' '.join(cmd)
  p = Popen(cmd, stdout=PIPE, stderr=PIPE, shell=shell)
  stdout,stderr = p.communicate()[0:2]
  if verbose and stdout:
    print stdout,
  if verbose and stderr:
    print stderr,
  return stdout,stderr

def qsvn(*args):
  return run_cmd(['svn'] + list(args), False)

def svn(*args):
  return run_cmd(['svn'] + list(args))

def gdbsvn(*args):
  call(['gdb', '--args', 'svn'] + list(args))
  return 'gdb', 'gdb'

def shell(script):
  return run_cmd(script, shell=True)

def rewrite_file(path, contents, auto_newline='\n\n'):
  dirname = os.path.dirname(path)
  if dirname and not os.path.lexists(dirname):
    os.makedirs(dirname)
  f = open(path, "w")
  f.write(contents + (auto_newline or ''))
  f.close()

def read_file(path):
  try:
    f = open(path, "r")
    contents = f.read()
    f.close()
    return contents
  except IOError:
    return None

def append_file(path, contents):
  dirname = os.path.dirname(path)
  if not os.path.lexists(dirname):
    os.makedirs(dirname)
  f = open(path, "a")
  f.write(contents)
  f.close()

def remove_file(path):
  if os.path.isfile(path):
    os.remove(path)

def tempdir():
  return tempfile.mkdtemp(prefix='tc_add-')

j = os.path.join

f = 'f' # file
d = 'd' # dir
l = 'l' # symbolic link

class TestContext:
  def __init__(self):
    self.base = tempdir()
    self.repos = j(self.base, 'repos')
    self.URL = 'file://' + self.repos
    shell('svnadmin create "' + self.repos + '"')
    self.WC = j(self.base, 'wc')
    svn('checkout', self.URL, self.WC)

  def create_wc2(self):
    self.WC2 = j(self.base, 'wc2')
    svn('checkout', self.URL, self.WC2)

  def wc(self, *relpath):
    if not relpath:
      return self.WC
    return j(self.WC, *relpath)

  def wc2(self, *relpath):
    if not relpath:
      return self.WC2
    return j(self.WC2, *relpath)

  def url(self, relpath):
    if not relpath:
      return self.URL
    return self.URL + '/' + relpath

  def head(self):
    out, err = qsvn('info', self.URL)
    revstr = 'Revision: '
    for line in out.split('\n'):
      if line.startswith(revstr):
        return int(line.strip()[len(revstr):])



def unver(ctx, target, kind, content=None):
  if not content:
    content = 'content of ' + os.path.basename(target)

  if kind == f:
    rewrite_file(target, content)
    shell('cat ' + target)
  elif kind == l:
    os.symlink('symlink', target)
  else:
    os.mkdir(target)



def add(ctx, target, kind, content):
  unver(ctx, target, kind, content)
  svn('add', target)
  svn('ps', 'PROP_add_' + content + '_' + kind,
      'content_add_' + content + '_' + kind,
      target)



def cp(ctx, suffix, target, kind, content=None):
  if not content:
    content = 'modified ' + os.path.basename(target)

  if kind == f:
    src = ctx.url('file' + suffix)
  elif kind == l:
    src = ctx.url('symlink' + suffix)
  else:
    src = ctx.url('dir' + suffix)

  src = src + '@1'
  svn('copy', src, target)
  svn('ps', 'PROP_copied_' + content + '_' + kind + suffix,
      'content_' + content + '_' + kind + suffix,
      target)
  svn('status', target)


def cp1(ctx, target, kind, content=None):
  return cp(ctx, '1', target, kind, content)

def cp2(ctx, target, kind, content=None):
  return cp(ctx, '2', target, kind, content)

def prepare_cp(ctx, kind, suffix):
  if kind == f:
    target = ctx.wc('file' + suffix)
    if not os.path.lexists(target):
      rewrite_file(target, 'copy source ' + suffix)
      svn('add', target)
      svn('ps', 'PROP_copy_source_' + kind + suffix,
          'content_copy_source_' + kind + suffix,
          target)
  elif kind == l:
    target = ctx.wc('symlink' + suffix)
    if not os.path.lexists(target):
      os.symlink('copy_source' + suffix, target)
      svn('add', target)
      svn('ps', 'PROP_copy_source_' + kind + suffix,
          'content_copy_source_' + kind + suffix,
          target)
  else:
    target = ctx.wc('dir' + suffix)
    svn('mkdir', target)
    svn('ps', 'PROP_copy_source_' + kind + suffix,
        'content_copy_source_' + kind + suffix,
        target)

def postpare_cp(ctx, kind, suffix):
  if kind == f:
    target = ctx.wc('file' + suffix)
    rewrite_file(target, 'local mod on copy source ' + suffix)
    svn('ps', 'PROP_local_copy_src_mod_' + kind + suffix,
        'content_local_copy_src_mod_' + kind + suffix,
        target)
  elif kind == l:
    target = ctx.wc('symlink' + suffix)
    svn('ps', 'PROP_local_copy_src_mod_' + kind + suffix,
        'content_local_copy_src_mod_' + kind + suffix,
        target)
  else:
    target = ctx.wc('dir' + suffix)
    svn('ps', 'PROP_local_copy_src_mod_' + kind + suffix,
        'content_local_copy_src_mod_' + kind + suffix,
        target)


def prepare(ctx, action, kind):
  if action == cp1:
    prepare_cp(ctx, kind, '1')
  elif action == cp2:
    prepare_cp(ctx, kind, '2')


def postpare(ctx, action, kind):
  if action == cp1:
    postpare_cp(ctx, kind, '1')
  elif action == cp2:
    postpare_cp(ctx, kind, '2')



def co(name, local_action, local_kind, incoming_action, incoming_kind):
  ctx = TestContext()

  prepare(ctx, local_action, local_kind)
  prepare(ctx, incoming_action, incoming_kind)

  svn('commit', '-mm', ctx.WC)
  svn('up', ctx.WC)

  head = ctx.head()
  print head

  ctx.create_wc2()
  target = ctx.wc2(name)
  incoming_action(ctx, target, incoming_kind, 'incoming')
  svn('commit', '-mm', ctx.WC2)

  target = ctx.wc(name)
  local_action(ctx, target, local_kind, 'local')

  postpare(ctx, local_action, local_kind)
  postpare(ctx, incoming_action, incoming_kind)

  # get conflicts
  o1,e1 = shell('yes p | svn checkout "' + ctx.URL + '" ' +
                '"' + ctx.WC + '"')
  o2,e2 = svn('status', ctx.WC)
  o3,e3 = run_cmd(['sqlite3', ctx.wc('.svn', 'wc.db'),
                   'select local_relpath,properties from base_node; '
                   +'select local_relpath,properties from working_node; '
                   +'select local_relpath,properties from actual_node; '
                   ])
  return o1, e1, o2, e2, o3, e3


def up(name, local_action, local_kind, incoming_action, incoming_kind):
  ctx = TestContext()

  prepare(ctx, local_action, local_kind)
  prepare(ctx, incoming_action, incoming_kind)

  svn('commit', '-mm', ctx.WC)
  svn('up', ctx.WC)

  head = ctx.head()
  print head

  target = ctx.wc(name)
  incoming_action(ctx, target, incoming_kind, 'incoming')
  svn('commit', '-mm', ctx.WC)

  # time warp
  svn('update', '-r', str(head), ctx.WC)

  local_action(ctx, target, local_kind, 'local')

  postpare(ctx, local_action, local_kind)
  postpare(ctx, incoming_action, incoming_kind)

  # get conflicts
  o1,e1 = svn('update', '--accept=postpone', ctx.WC)
  o2,e2 = svn('status', ctx.WC)
  o3,e3 = run_cmd(['sqlite3', ctx.wc('.svn', 'wc.db'),
                   'select local_relpath,properties from base_node; '
                   +'select local_relpath,properties from working_node; '
                   +'select local_relpath,properties from actual_node; '
                   ])
  return o1, e1, o2, e2, o3, e3


def sw(name, local_action, local_kind, incoming_action, incoming_kind):
  ctx = TestContext()
  prepare(ctx, local_action, local_kind)
  prepare(ctx, incoming_action, incoming_kind)
  svn('commit', '-mm', ctx.WC)

  svn('mkdir', '-mm', ctx.url('trunk'))
  svn('copy', '-mm', ctx.url('trunk'), ctx.url('branch'))

  svn('up', ctx.WC)

  target = ctx.wc('branch', name)
  incoming_action(ctx, target, incoming_kind, 'incoming')
  svn('commit', '-mm', ctx.WC)
  svn('up', ctx.WC)

  target = ctx.wc('trunk', name)
  local_action(ctx, target, local_kind, 'local')

  postpare(ctx, local_action, local_kind)
  postpare(ctx, incoming_action, incoming_kind)

  # get conflicts
  o1,e1 = svn('switch', '--accept=postpone', ctx.url('branch'), ctx.wc('trunk'))
  o2,e2 = svn('status', ctx.WC)
  o3,e3 = run_cmd(['sqlite3', ctx.wc('trunk', '.svn', 'wc.db'),
                   'select local_relpath,properties from base_node; select'
                   + ' local_relpath,properties from working_node;'])
  # This is a bit stupid. Someone rewire this.
  return o1, e1, o2, e2, o3, e3


# This controls which tests are run. All possible combinations are tested.
# The elements are functions for up,sw and add,cp1,cp2,unver, and they are
# simple strings for f (file), l (symlink), d (directory).
#
#            cmd        local action and kind     incoming action and kind
p = product((co,up,sw), (add,cp1,unver), (f,l,d), (add,cp2,cp1), (f,l,d))

# Incoming cp1 is meant to match up only with local cp1. Also, cp1-cp1 is
# supposed to perform identical copies in both incoming and local, so they
# only make sense with matching kinds. Skip all rows that don't match this:
skip = lambda row: (row[3] == cp1 and (row[4] != row[2] or row[1] != cp1)
                    #Select subsets if desired
                    #or (row[0] != up or row[3] not in [cp1, cp2])
                    #or (row[2] != l and row[4] != l)
                    #
                    #or row not in (
                    ##[up, cp1, l, add, l],
                    ##[up, cp1, f, cp2, l],
                    #[up, cp1, f, cp2, f],
                    ##[up, cp1, f, add, f],
                    ##[up, cp1, f, add, l],
                    ##[up, add, l, add, l],
                    #)
                    )




def nameof(thing):
  if isinstance(thing, FunctionType):
    return thing.__name__
  if isinstance(thing, ListType) or isinstance(thing, TupleType):
    return '_'.join([ nameof(thang) for thang in thing])
  return str(thing)


def analyze(name, outs):
  stats = []
  for o in outs:
    for line in o.split('\n'):
      if (line.startswith('svn: ') or line.startswith('      > ')
          or line.find(name) > -1):
        stats.append(line)
  return stats

#os.environ['SVN_I_LOVE_CORRUPTED_WORKING_COPIES_SO_DISABLE_SLEEP_FOR_TIMESTAMPS'] = 'yes'

results = []
name = None
try:
  # there is probably a better way than this:
  for row in list(p):
    if skip(row):
      continue
    name = nameof(row)
    print name
    test_func = row[0]
    results.append( (name, analyze( name, test_func( name, *row[1:] ) )) )
except:
  if name:
    print 'Error during', name
  raise
finally:
  lines = []
  for result in results:
    name = result[0]
    if result[1]:
      lines.append('----- ' + name)
      for stat in result[1]:
        lines.append(stat)
    else:
      lines.append('----- ' + name + ': nothing.')
  dump = '\n'.join(lines)
  print dump
  rewrite_file('tree-conflicts-add-vs-add.py.results', dump)

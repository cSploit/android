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

"""
usage: benchmark.py run <run_file> <levels> <spread> [N]
       benchmark.py show <run_file>
       benchmark.py compare <run_file1> <run_file2>
       benchmark.py combine <new_file> <run_file1> <run_file2> ...

Test data is written to run_file.
If a run_file exists, data is added to it.
<levels> is the number of directory levels to create
<spread> is the number of child trees spreading off each dir level
If <N> is provided, the run is repeated N times.
"""

import os
import sys
import tempfile
import subprocess
import datetime
import random
import shutil
import cPickle
import optparse
import stat

TOTAL_RUN = 'TOTAL RUN'

timings = None

def run_cmd(cmd, stdin=None, shell=False):

  if shell:
    printable_cmd = 'CMD: ' + cmd
  else:
    printable_cmd = 'CMD: ' + ' '.join(cmd)
  if options.verbose:
    print printable_cmd

  if stdin:
    stdin_arg = subprocess.PIPE
  else:
    stdin_arg = None

  p = subprocess.Popen(cmd,
                       stdin=stdin_arg,
                       stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE,
                       shell=shell)
  stdout,stderr = p.communicate(input=stdin)

  if options.verbose:
    if (stdout):
      print "STDOUT: [[[\n%s]]]" % ''.join(stdout)
  if (stderr):
    print "STDERR: [[[\n%s]]]" % ''.join(stderr)

  return stdout,stderr

def timedelta_to_seconds(td):
  return ( float(td.seconds)
           + float(td.microseconds) / (10**6)
           + td.days * 24 * 60 * 60 )


class Timings:

  def __init__(self, *ignore_svn_cmds):
    self.timings = {}
    self.current_name = None
    self.tic_at = None
    self.ignore = ignore_svn_cmds
    self.name = None

  def tic(self, name):
    if name in self.ignore:
      return
    self.toc()
    self.current_name = name
    self.tic_at = datetime.datetime.now()

  def toc(self):
    if self.current_name and self.tic_at:
      toc_at = datetime.datetime.now()
      self.submit_timing(self.current_name,
                         timedelta_to_seconds(toc_at - self.tic_at))
    self.current_name = None
    self.tic_at = None

  def submit_timing(self, name, seconds):
    times = self.timings.get(name)
    if not times:
      times = []
      self.timings[name] = times
    times.append(seconds)

  def min_max_avg(self, name):
    ttimings = self.timings.get(name)
    return ( min(ttimings),
             max(ttimings),
             reduce(lambda x,y: x + y, ttimings) / len(ttimings) )

  def summary(self):
    s = []
    if self.name:
      s.append('Timings for %s' % self.name)
    s.append('    N   min     max     avg    operation  (unit is seconds)')

    names = sorted(self.timings.keys())

    for name in names:
      timings = self.timings.get(name)
      if not name or not timings: continue

      tmin, tmax, tavg = self.min_max_avg(name)

      s.append('%5d %7.2f %7.2f %7.2f  %s' % (
                 len(timings),
                 tmin,
                 tmax,
                 tavg,
                 name))

    return '\n'.join(s)


  def compare_to(self, other):
    def do_div(a, b):
      if b:
        return float(a) / float(b)
      else:
        return 0.0

    def do_diff(a, b):
      return float(a) - float(b)

    selfname = self.name
    if not selfname:
      selfname = 'unnamed'
    othername = other.name
    if not othername:
      othername = 'the other'

    selftotal = self.min_max_avg(TOTAL_RUN)[2]
    othertotal = other.min_max_avg(TOTAL_RUN)[2]

    s = ['COMPARE %s to %s' % (othername, selfname)]

    if TOTAL_RUN in self.timings and TOTAL_RUN in other.timings:
      s.append('  %s times: %5.1f seconds avg for %s' % (TOTAL_RUN,
                                                         othertotal, othername))
      s.append('  %s        %5.1f seconds avg for %s' % (' ' * len(TOTAL_RUN),
                                                         selftotal, selfname))


    s.append('      min              max              avg         operation')

    names = sorted(self.timings.keys())

    for name in names:
      if not name in other.timings:
        continue


      min_me, max_me, avg_me = self.min_max_avg(name)
      min_other, max_other, avg_other = other.min_max_avg(name)

      s.append('%-16s %-16s %-16s  %s' % (
                 '%7.2f|%+7.3f' % (
                     do_div(min_me, min_other),
                     do_diff(min_me, min_other)
                   ),

                 '%7.2f|%+7.3f' % (
                     do_div(max_me, max_other),
                     do_diff(max_me, max_other)
                   ),

                 '%7.2f|%+7.3f' % (
                     do_div(avg_me, avg_other),
                     do_diff(avg_me, avg_other)
                   ),

                 name))

    s.extend([
         '("1.23|+0.45"  means factor=1.23, difference in seconds = 0.45',
         'factor < 1 or difference < 0 means \'%s\' is faster than \'%s\')'
           % (self.name, othername)])

    return '\n'.join(s)


  def add(self, other):
    for name, other_times in other.timings.items():
      my_times = self.timings.get(name)
      if not my_times:
        my_times = []
        self.timings[name] = my_times
      my_times.extend(other_times)




j = os.path.join

_create_count = 0

def next_name(prefix):
  global _create_count
  _create_count += 1
  return '_'.join((prefix, str(_create_count)))

def create_tree(in_dir, levels, spread=5):
  try:
    os.mkdir(in_dir)
  except:
    pass

  for i in range(spread):
    # files
    fn = j(in_dir, next_name('file'))
    f = open(fn, 'w')
    f.write('This is %s\n' % fn)
    f.close()

    # dirs
    if (levels > 1):
      dn = j(in_dir, next_name('dir'))
      create_tree(dn, levels - 1, spread)


def svn(*args):
  name = args[0]

  ### options comes from the global namespace; it should be passed
  cmd = [options.svn] + list(args)
  if options.verbose:
    print 'svn cmd:', ' '.join(cmd)

  stdin = None
  if stdin:
    stdin_arg = subprocess.PIPE
  else:
    stdin_arg = None

  ### timings comes from the global namespace; it should be passed
  timings.tic(name)
  try:
    p = subprocess.Popen(cmd,
                         stdin=stdin_arg,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE,
                         shell=False)
    stdout,stderr = p.communicate(input=stdin)
  except OSError:
    stdout = stderr = None
  finally:
    timings.toc()

  if options.verbose:
    if (stdout):
      print "STDOUT: [[[\n%s]]]" % ''.join(stdout)
    if (stderr):
      print "STDERR: [[[\n%s]]]" % ''.join(stderr)

  return stdout,stderr


def add(*args):
  return svn('add', *args)

def ci(*args):
  return svn('commit', '-mm', *args)

def up(*args):
  return svn('update', *args)

def st(*args):
  return svn('status', *args)

_chars = [chr(x) for x in range(ord('a'), ord('z') +1)]

def randstr(len=8):
  return ''.join( [random.choice(_chars) for i in range(len)] )

def _copy(path):
  dest = next_name(path + '_copied')
  svn('copy', path, dest)

def _move(path):
  dest = path + '_moved'
  svn('move', path, dest)

def _propmod(path):
  so, se = svn('proplist', path)
  propnames = [line.strip() for line in so.strip().split('\n')[1:]]

  # modify?
  if len(propnames):
    svn('ps', propnames[len(propnames) / 2], randstr(), path)

  # del?
  if len(propnames) > 1:
    svn('propdel', propnames[len(propnames) / 2], path)


def _propadd(path):
  # set a new one.
  svn('propset', randstr(), randstr(), path)


def _mod(path):
  if os.path.isdir(path):
    return _propmod(path)

  f = open(path, 'a')
  f.write('\n%s\n' % randstr())
  f.close()

def _add(path):
  if os.path.isfile(path):
    return _mod(path)

  if random.choice((True, False)):
    # create a dir
    svn('mkdir', j(path, next_name('new_dir')))
  else:
    # create a file
    new_path = j(path, next_name('new_file'))
    f = open(new_path, 'w')
    f.write(randstr())
    f.close()
    svn('add', new_path)

def _del(path):
  svn('delete', path)

_mod_funcs = (_mod, _add, _propmod, _propadd, )#_copy,) # _move, _del)

def modify_tree(in_dir, fraction):
  child_names = os.listdir(in_dir)
  for child_name in child_names:
    if child_name[0] == '.':
      continue
    if random.random() < fraction:
      path = j(in_dir, child_name)
      random.choice(_mod_funcs)(path)

  for child_name in child_names:
    if child_name[0] == '.': continue
    path = j(in_dir, child_name)
    if os.path.isdir(path):
      modify_tree(path, fraction)

def propadd_tree(in_dir, fraction):
  for child_name in os.listdir(in_dir):
    if child_name[0] == '.': continue
    path = j(in_dir, child_name)
    if random.random() < fraction:
      _propadd(path)
    if os.path.isdir(path):
      propadd_tree(path, fraction)


def rmtree_onerror(func, path, exc_info):
  """Error handler for ``shutil.rmtree``.

  If the error is due to an access error (read only file)
  it attempts to add write permission and then retries.

  If the error is for another reason it re-raises the error.

  Usage : ``shutil.rmtree(path, onerror=onerror)``
  """
  if not os.access(path, os.W_OK):
    # Is the error an access error ?
    os.chmod(path, stat.S_IWUSR)
    func(path)
  else:
    raise


def run(levels, spread, N):
  for i in range(N):
    base = tempfile.mkdtemp()

    # ensure identical modifications for every run
    random.seed(0)

    try:
      repos = j(base, 'repos')
      repos = repos.replace('\\', '/')
      wc = j(base, 'wc')
      wc2 = j(base, 'wc2')

      if repos.startswith('/'):
        file_url = 'file://%s' % repos
      else:
        file_url = 'file:///%s' % repos

      so, se = svn('--version')
      if not so:
        print "Can't find svn."
        exit(1)
      version = ', '.join([s.strip() for s in so.split('\n')[:2]])

      print '\nRunning svn benchmark in', base
      print 'dir levels: %s; new files and dirs per leaf: %s; run %d of %d' %(
            levels, spread, i + 1, N)

      print version
      started = datetime.datetime.now()

      try:
        run_cmd(['svnadmin', 'create', repos])
        svn('checkout', file_url, wc)

        trunk = j(wc, 'trunk')
        create_tree(trunk, levels, spread)
        add(trunk)
        st(wc)
        ci(wc)
        up(wc)
        propadd_tree(trunk, 0.5)
        ci(wc)
        up(wc)
        st(wc)

        trunk_url = file_url + '/trunk'
        branch_url = file_url + '/branch'

        svn('copy', '-mm', trunk_url, branch_url)
        st(wc)

        up(wc)
        st(wc)

        svn('checkout', trunk_url, wc2)
        st(wc2)
        modify_tree(wc2, 0.5)
        st(wc2)
        ci(wc2)
        up(wc2)
        up(wc)

        svn('switch', branch_url, wc2)
        modify_tree(wc2, 0.5)
        st(wc2)
        ci(wc2)
        up(wc2)
        up(wc)

        modify_tree(trunk, 0.5)
        st(wc)
        ci(wc)
        up(wc2)
        up(wc)

        svn('merge', '--accept=postpone', trunk_url, wc2)
        st(wc2)
        svn('resolve', '--accept=mine-conflict', wc2)
        st(wc2)
        svn('resolved', '-R', wc2)
        st(wc2)
        ci(wc2)
        up(wc2)
        up(wc)

        svn('merge', '--accept=postpone', '--reintegrate', branch_url, trunk)
        st(wc)
        svn('resolve', '--accept=mine-conflict', wc)
        st(wc)
        svn('resolved', '-R', wc)
        st(wc)
        ci(wc)
        up(wc2)
        up(wc)

        svn('delete', j(wc, 'branch'))
        ci(wc)
        up(wc2)
        up(wc)


      finally:
        stopped = datetime.datetime.now()
        print '\nDone with svn benchmark in', (stopped - started)

        ### timings comes from the global namespace; it should be passed
        timings.submit_timing(TOTAL_RUN,
                              timedelta_to_seconds(stopped - started))

        # rename ps to prop mod
        if timings.timings.get('ps'):
          has = timings.timings.get('prop mod')
          if not has:
            has = []
            timings.timings['prop mod'] = has
          has.extend( timings.timings['ps'] )
          del timings.timings['ps']

        print timings.summary()
    finally:
      shutil.rmtree(base, onerror=rmtree_onerror)


def read_from_file(file_path):
  f = open(file_path, 'rb')
  try:
    instance = cPickle.load(f)
    instance.name = os.path.basename(file_path)
  finally:
    f.close()
  return instance


def write_to_file(file_path, instance):
  f = open(file_path, 'wb')
  cPickle.dump(instance, f)
  f.close()

def cmd_compare(path1, path2):
  t1 = read_from_file(path1)
  t2 = read_from_file(path2)

  print t1.summary()
  print '---'
  print t2.summary()
  print '---'
  print t2.compare_to(t1)

def cmd_combine(dest, *paths):
  total = Timings('--version');

  for path in paths:
    t = read_from_file(path)
    total.add(t)

  print total.summary()
  write_to_file(dest, total)

def cmd_run(timings_path, levels, spread, N=1):
  levels = int(levels)
  spread = int(spread)
  N = int(N)

  print '\n\nHi, going to run a Subversion benchmark series of %d runs...' % N

  ### UGH! should pass to run()
  global timings

  if os.path.isfile(timings_path):
    print 'Going to add results to existing file', timings_path
    timings = read_from_file(timings_path)
  else:
    print 'Going to write results to new file', timings_path
    timings = Timings('--version')

  run(levels, spread, N)

  write_to_file(timings_path, timings)

def cmd_show(*paths):
  for timings_path in paths:
    timings = read_from_file(timings_path)
    print '---\n%s' % timings_path
    print timings.summary()


def usage():
  print __doc__

if __name__ == '__main__':
  parser = optparse.OptionParser()
  # -h is automatically added.
  ### should probably expand the help for that. and see about -?
  parser.add_option('-v', '--verbose', action='store_true', dest='verbose',
                    help='Verbose operation')
  parser.add_option('--svn', action='store', dest='svn', default='svn',
                    help='Specify Subversion executable to use')

  ### should start passing this, but for now: make it global
  global options

  options, args = parser.parse_args()

  # there should be at least one arg left: the sub-command
  if not args:
    usage()
    exit(1)

  cmd = args[0]
  del args[0]

  if cmd == 'compare':
    if len(args) != 2:
      usage()
      exit(1)
    cmd_compare(*args)

  elif cmd == 'combine':
    if len(args) < 3:
      usage()
      exit(1)
    cmd_combine(*args)

  elif cmd == 'run':
    if len(args) < 3 or len(args) > 4:
      usage()
      exit(1)
    cmd_run(*args)

  elif cmd == 'show':
    if not args:
      usage()
      exit(1)
    cmd_show(*args)

  else:
    usage()

#!/usr/bin/env python
#
#
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
#
#
# -*- Python -*-
"""find-fix.py: produce a find/fix report for Subversion's IZ database

For simple text summary:
       find-fix.py query-set-1.tsv YYYY-MM-DD YYYY-MM-DD
Statistics will be printed for bugs found or fixed within the
time frame.

For gnuplot presentation:
       find-fix.py query-set-1.tsv outfile
Gnuplot provides its own way to select date ranges.

Either way, get a query-set-1.tsv from:
  http://subversion.tigris.org/iz-data/query-set-1.tsv  (updated nightly)
See http://subversion.tigris.org/iz-data/README for more info on that file.

For more usage info on this script:
        find-fix.py --help
"""

_version = "$Revision:"

#
# This can be run over the data file found at:
#   http://subversion.tigris.org/iz-data/query-set-1.tsv
#

import getopt
try:
  my_getopt = getopt.gnu_getopt
except AttributeError:
  my_getopt = getopt.getopt
import operator
import os
import os.path
import pydoc
import re
try:
  # Python >=2.6
  from functools import reduce
except ImportError:
  # Python <2.6
  pass
import sys
import time

me = os.path.basename(sys.argv[0])

# Long options and their usage strings; "=" means it takes an argument.
# To get a list suitable for getopt, just do
#
#   [x[0] for x in long_opts]
#
# Make sure to sacrifice a lamb to Guido for each element of the list.
long_opts = [
  ["milestones=",      """Optional, milestones NOT to report on
        (one or more of Beta, 1.0, Post-1.0, cvs2svn-1.0, cvs2svn-opt,
        inapplicable)"""],
  ["update",          """Optional, update the statistics first."""],
  ["doc",             """Optional, print pydocs."""],
  ["help",            """Optional, print usage (this text)."""],
  ["verbose",         """Optional, print more progress messages."""],
  ]

help    = 0
verbose = 0
update  = 0

DATA_FILE = "http://subversion.tigris.org/iz-data/query-set-1.tsv"
ONE_WEEK = 7 * 24 * 60 * 60

_types = []
_milestone_filter = []

noncore_milestone_filter = [
  'Post-1.0',
  '1.1',
  'cvs2svn-1.0',
  'cvs2svn-opt',
  'inapplicable',
  'no milestone',
  ]

one_point_oh_milestone_filter = noncore_milestone_filter + []

beta_milestone_filter = one_point_oh_milestone_filter + ['1.0']


_types = [
  'DEFECT',
  'TASK',
  'FEATURE',
  'ENHANCEMENT',
  'PATCH',
  ]


def main():
  """Report bug find/fix rate statistics for Subversion."""

  global verbose
  global update
  global _types
  global _milestone_filter
  global noncore_milestone_filter

  try:
      opts, args = my_getopt(sys.argv[1:], "", [x[0] for x in long_opts])
  except getopt.GetoptError, e:
      sys.stderr.write("Error: %s\n" % e.msg)
      shortusage()
      sys.stderr.write("%s --help for options.\n" % me)
      sys.exit(1)

  for opt, arg in opts:
    if opt == "--help":
      usage()
      sys.exit(0)
    elif opt == "--verbose":
      verbose = 1
    elif opt == "--milestones":
      for mstone in arg.split(","):
        if mstone == "noncore":
          _milestone_filter = noncore_milestone_filter
        elif mstone == "beta":
          _milestone_filter = beta_milestone_filter
        elif mstone == "one":
          _milestone_filter = one_point_oh_milestone_filter
        elif mstone[0] == '-':
          if mstone[1:] in _milestone_filter:
            spot = _milestone_filter.index(mstone[1:])
            _milestone_filter = _milestone_filter[:spot] \
                                + _milestone_filter[(spot+1):]
        else:
          _milestone_filter += [mstone]

    elif opt == "--update":
      update = 1
    elif opt == "--doc":
      pydoc.doc(pydoc.importfile(sys.argv[0]))
      sys.exit(0)

  if len(_milestone_filter) == 0:
    _milestone_filter = noncore_milestone_filter

  if verbose:
    sys.stderr.write("%s: Filtering out milestones %s.\n"
                     % (me, ", ".join(_milestone_filter)))

  if len(args) == 2:
    if verbose:
      sys.stderr.write("%s: Generating gnuplot data.\n" % me)
    if update:
      if verbose:
        sys.stderr.write("%s: Updating %s from %s.\n" % (me, args[0], DATA_FILE))
      if os.system("curl " + DATA_FILE + "> " + args[0]):
        os.system("wget " + DATA_FILE)
    plot(args[0], args[1])

  elif len(args) == 3:
    if verbose:
      sys.stderr.write("%s: Generating summary from %s to %s.\n"
                       % (me, args[1], args[2]))
    if update:
      if verbose:
        sys.stderr.write("%s: Updating %s from %s.\n" % (me, args[0], DATA_FILE))
      if os.system("curl " + DATA_FILE + "> " + args[0]):
        os.system("wget " + DATA_FILE)

    try:
      t_start = parse_time(args[1] + " 00:00:00")
    except ValueError:
      sys.stderr.write('%s: ERROR: bad time value: %s\n' % (me, args[1]))
      sys.exit(1)

    try:
      t_end = parse_time(args[2] + " 00:00:00")
    except ValueError:
      sys.stderr.write('%s: ERROR: bad time value: %s\n' % (me, args[2]))
      sys.exit(1)

    summary(args[0], t_start, t_end)
  else:
    usage()

  sys.exit(0)


def summary(datafile, d_start, d_end):
  "Prints a summary of activity within a specified date range."

  data = load_data(datafile)

  # activity during the requested period
  found, fixed, inval, dup, other = extract(data, 1, d_start, d_end)

  # activity from the beginning of time to the end of the request
  # used to compute remaining
  # XXX It would be faster to change extract to collect this in one
  # pass.  But we don't presently have enough data, nor use this
  # enough, to justify that rework.
  fromzerofound, fromzerofixed, fromzeroinval, fromzerodup, fromzeroother \
              = extract(data, 1, 0, d_end)

  alltypes_found = alltypes_fixed = alltypes_inval = alltypes_dup \
                   = alltypes_other = alltypes_rem = 0
  for t in _types:
    fromzerorem_t = fromzerofound[t]\
                    - (fromzerofixed[t] + fromzeroinval[t] + fromzerodup[t]
                       + fromzeroother[t])
    print('%12s: found=%3d  fixed=%3d  inval=%3d  dup=%3d  ' \
          'other=%3d  remain=%3d' \
          % (t, found[t], fixed[t], inval[t], dup[t], other[t], fromzerorem_t))
    alltypes_found = alltypes_found + found[t]
    alltypes_fixed = alltypes_fixed + fixed[t]
    alltypes_inval = alltypes_inval + inval[t]
    alltypes_dup   = alltypes_dup   + dup[t]
    alltypes_other = alltypes_other + other[t]
    alltypes_rem   = alltypes_rem + fromzerorem_t

  print('-' * 77)
  print('%12s: found=%3d  fixed=%3d  inval=%3d  dup=%3d  ' \
        'other=%3d  remain=%3d' \
        % ('totals', alltypes_found, alltypes_fixed, alltypes_inval,
           alltypes_dup, alltypes_other, alltypes_rem))
  # print '%12s  find/fix ratio: %g%%' \
  #      % (" "*12, (alltypes_found*100.0/(alltypes_fixed
  #         + alltypes_inval + alltypes_dup + alltypes_other)))


def plot(datafile, outbase):
  "Generates data files intended for use by gnuplot."

  global _types

  data = load_data(datafile)

  t_min = 1L<<32
  for issue in data:
    if issue.created < t_min:
      t_min = issue.created

  # break the time up into a tuple, then back up to Sunday
  t_start = time.localtime(t_min)
  t_start = time.mktime((t_start[0], t_start[1], t_start[2] - t_start[6] - 1,
                         0, 0, 0, 0, 0, 0))

  plots = { }
  for t in _types:
    # for each issue type, we will record per-week stats, compute a moving
    # average of the find/fix delta, and track the number of open issues
    plots[t] = [ [ ], MovingAverage(), 0 ]

  week = 0
  for date in range(t_start, time.time(), ONE_WEEK):
    ### this is quite inefficient, as we could just sort by date, but
    ### I'm being lazy
    found, fixed = extract(data, None, date, date + ONE_WEEK - 1)

    for t in _types:
      per_week, avg, open_issues = plots[t]
      delta = found[t] - fixed[t]
      per_week.append((week, date,
                       found[t], -fixed[t], avg.add(delta), open_issues))
      plots[t][2] = open_issues + delta

    week = week + 1

  for t in _types:
    week_data = plots[t][0]
    write_file(week_data, outbase, t, 'found', 2)
    write_file(week_data, outbase, t, 'fixed', 3)
    write_file(week_data, outbase, t, 'avg', 4)
    write_file(week_data, outbase, t, 'open', 5)

def write_file(week_data, base, type, tag, idx):
  f = open('%s.%s.%s' % (base, tag, type), 'w')
  for info in week_data:
    f.write('%s %s # %s\n' % (info[0], info[idx], time.ctime(info[1])))


class MovingAverage:
  "Helper class to compute moving averages."
  def __init__(self, n=4):
    self.n = n
    self.data = [ 0 ] * n
  def add(self, value):
    self.data.pop(0)
    self.data.append(float(value) / self.n)
    return self.avg()
  def avg(self):
    return reduce(operator.add, self.data)


def extract(data, details, d_start, d_end):
  """Extract found/fixed counts for each issue type within the data range.

  If DETAILS is false, then return two dictionaries:

    found, fixed

  ...each mapping issue types to the number of issues of that type
  found or fixed respectively.

  If DETAILS is true, return five dictionaries:

    found, fixed, invalid, duplicate, other

  The first is still the found issues, but the other four break down
  the resolution into 'FIXED', 'INVALID', 'DUPLICATE', and a grab-bag
  category for 'WORKSFORME', 'LATER', 'REMIND', and 'WONTFIX'."""

  global _types
  global _milestone_filter

  found = { }
  fixed = { }
  invalid = { }
  duplicate = { }
  other = { }  # "WORKSFORME", "LATER", "REMIND", and "WONTFIX"

  for t in _types:
    found[t] = fixed[t] = invalid[t] = duplicate[t] = other[t] = 0

  for issue in data:
    # filter out disrespected milestones
    if issue.milestone in _milestone_filter:
      continue

    # record the found/fixed counts
    if d_start <= issue.created <= d_end:
      found[issue.type] = found[issue.type] + 1
    if d_start <= issue.resolved <= d_end:
      if details:
        if issue.resolution == "FIXED":
          fixed[issue.type] = fixed[issue.type] + 1
        elif issue.resolution == "INVALID":
          invalid[issue.type] = invalid[issue.type] + 1
        elif issue.resolution == "DUPLICATE":
          duplicate[issue.type] = duplicate[issue.type] + 1
        else:
          other[issue.type] = other[issue.type] + 1
      else:
        fixed[issue.type] = fixed[issue.type] + 1

  if details:
    return found, fixed, invalid, duplicate, other
  else:
    return found, fixed


def load_data(datafile):
  "Return a list of Issue objects for the specified data."
  return list(map(Issue, open(datafile).readlines()))


class Issue:
  "Represents a single issue from the exported IssueZilla data."

  def __init__(self, line):
    row = line.strip().split('\t')

    self.id = int(row[0])
    self.type = row[1]
    self.reporter = row[2]
    if row[3] == 'NULL':
      self.assigned = None
    else:
      self.assigned = row[3]
    self.milestone = row[4]
    self.created = parse_time(row[5])
    self.resolution = row[7]
    if not self.resolution:
      # If the resolution is empty, then force the resolved date to None.
      # When an issue is reopened, there will still be activity showing
      # a "RESOLVED", thus we get a resolved date. But we simply want to
      # ignore that date.
      self.resolved = None
    else:
      self.resolved = parse_time(row[6])
    self.summary = row[8]


parse_time_re = re.compile('([0-9]{4})-([0-9]{2})-([0-9]{2}) '
                           '([0-9]{2}):([0-9]{2}):([0-9]{2})')

def parse_time(t):
  "Convert an exported MySQL timestamp into seconds since the epoch."

  global parse_time_re

  if t == 'NULL':
    return None
  try:
    matches = parse_time_re.match(t)
    return time.mktime((int(matches.group(1)),
                        int(matches.group(2)),
                        int(matches.group(3)),
                        int(matches.group(4)),
                        int(matches.group(5)),
                        int(matches.group(6)),
                        0, 0, -1))
  except ValueError:
    sys.stderr.write('ERROR: bad time value: %s\n'% t)
    sys.exit(1)

def shortusage():
  print(pydoc.synopsis(sys.argv[0]))
  print("""
For simple text summary:
       find-fix.py [options] query-set-1.tsv YYYY-MM-DD YYYY-MM-DD

For gnuplot presentation:
       find-fix.py [options] query-set-1.tsv outfile
""")

def usage():
  shortusage()
  for x in long_opts:
      padding_limit = 18
      if x[0][-1:] == '=':
          sys.stdout.write("   --%s " % x[0][:-1])
          padding_limit = 19
      else:
          sys.stdout.write("   --%s " % x[0])
      print("%s %s" % ((' ' * (padding_limit - len(x[0]))), x[1]))
  print('''
Option keywords may be abbreviated to any unique prefix.
Most options require "=xxx" arguments.
Option order is not important.''')

if __name__ == '__main__':
  main()

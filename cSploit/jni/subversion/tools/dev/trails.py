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

## See the usage() function for operating instructions. ##

import re
try:
  # Python >=2.6
  from functools import reduce
except ImportError:
  # Python <2.6
  pass
import sys
import operator

_re_trail = re.compile('\((?P<txn_body>[a-z_]*), (?P<filename>[a-z_\-./]*), (?P<lineno>[0-9]*), (?P<txn>0|1)\): (?P<ops>.*)')
_re_table_op = re.compile('\(([a-z]*), ([a-z]*)\)')

_seperator = '------------------------------------------------------------\n'

def parse_trails_log(infile):
  trails = []
  lineno = 0
  for line in infile.readlines():
    m = _re_trail.match(line)

    lineno = lineno + 1

    if not m:
      sys.stderr.write('Invalid input, line %u:\n%s\n' % (lineno, line))
      sys.exit(1)

    txn = int(m.group('txn'))
    if not txn:
      ### We're not interested in trails that don't use txns at this point.
      continue

    txn_body = (m.group('txn_body'), m.group('filename'),
                int(m.group('lineno')))
    trail = _re_table_op.findall(m.group('ops'))
    trail.reverse()

    if not trail:
      sys.stderr.write('Warning!  Empty trail at line %u:\n%s' % (lineno, line))

    trails.append((txn_body, trail))

  return trails


def output_summary(trails, outfile):
  ops = []
  for (txn_body, trail) in trails:
    ops.append(len(trail))
  ops.sort()

  total_trails = len(ops)
  total_ops = reduce(operator.add, ops)
  max_ops = ops[-1]
  median_ops = ops[total_trails / 2]
  average_ops = float(total_ops) / total_trails

  outfile.write(_seperator)
  outfile.write('Summary\n')
  outfile.write(_seperator)
  outfile.write('Total number of trails: %10i\n' % total_trails)
  outfile.write('Total number of ops:    %10i\n' % total_ops)
  outfile.write('max ops/trail:          %10i\n' % max_ops)
  outfile.write('median ops/trail:       %10i\n' % median_ops)
  outfile.write('average ops/trail:      %10.2f\n' % average_ops)
  outfile.write('\n')


# custom compare function
def _freqtable_cmp(a_b, c_d):
  (a, b) = a_b
  (c, d) = c_d
  c = cmp(d, b)
  if not c:
    c = cmp(a, c)
  return c

def list_frequencies(list):
  """
  Given a list, return a list composed of (item, frequency)
  in sorted order
  """

  counter = {}
  for item in list:
    counter[item] = counter.get(item, 0) + 1

  frequencies = list(counter.items())
  frequencies.sort(_freqtable_cmp)

  return frequencies


def output_trail_length_frequencies(trails, outfile):
  ops = []
  for (txn_body, trail) in trails:
    ops.append(len(trail))

  total_trails = len(ops)
  frequencies = list_frequencies(ops)

  outfile.write(_seperator)
  outfile.write('Trail length frequencies\n')
  outfile.write(_seperator)
  outfile.write('ops/trail   frequency   percentage\n')
  for (r, f) in frequencies:
    p = float(f) * 100 / total_trails
    outfile.write('%4i         %6i       %5.2f\n' % (r, f, p))
  outfile.write('\n')


def output_trail(outfile, trail, column = 0):
  ### Output the trail itself, in its own column

  if len(trail) == 0:
    outfile.write('<empty>\n')
    return

  line = str(trail[0])
  for op in trail[1:]:
    op_str = str(op)
    if len(line) + len(op_str) > 75 - column:
      outfile.write('%s,\n' % line)
      outfile.write(''.join(' ' * column))
      line = op_str
    else:
      line = line + ', ' + op_str
  outfile.write('%s\n' % line)

  outfile.write('\n')


def output_trail_frequencies(trails, outfile):

  total_trails = len(trails)

  ttrails = []
  for (txn_body, trail) in trails:
    ttrails.append((txn_body, tuple(trail)))

  frequencies = list_frequencies(ttrails)

  outfile.write(_seperator)
  outfile.write('Trail frequencies\n')
  outfile.write(_seperator)
  outfile.write('frequency   percentage   ops/trail   trail\n')
  for (((txn_body, file, line), trail), f) in frequencies:
    p = float(f) * 100 / total_trails
    outfile.write('-- %s - %s:%u --\n' % (txn_body, file, line))
    outfile.write('%6i        %5.2f       %4i       ' % (f, p, len(trail)))
    output_trail(outfile, trail, 37)


def output_txn_body_frequencies(trails, outfile):
  bodies = []
  for (txn_body, trail) in trails:
    bodies.append(txn_body)

  total_trails = len(trails)
  frequencies = list_frequencies(bodies)

  outfile.write(_seperator)
  outfile.write('txn_body frequencies\n')
  outfile.write(_seperator)
  outfile.write('frequency   percentage   txn_body\n')
  for ((txn_body, file, line), f) in frequencies:
    p = float(f) * 100 / total_trails
    outfile.write('%6i        %5.2f       %s - %s:%u\n'
                  % (f, p, txn_body, file, line))


def usage(pgm):
  w = sys.stderr.write
  w("%s: a program for analyzing Subversion trail usage statistics.\n" % pgm)
  w("\n")
  w("Usage:\n")
  w("\n")
  w("   Compile Subversion with -DSVN_FS__TRAIL_DEBUG, which will cause it\n")
  w("   it to print trail statistics to stderr.  Save the stats to a file,\n")
  w("   invoke %s on the file, and ponder the output.\n" % pgm)
  w("\n")


if __name__ == '__main__':
  if len(sys.argv) > 2:
    sys.stderr.write("Error: too many arguments\n\n")
    usage(sys.argv[0])
    sys.exit(1)

  if len(sys.argv) == 1:
    infile = sys.stdin
  else:
    try:
      infile = open(sys.argv[1])
    except (IOError):
      sys.stderr.write("Error: unable to open '%s'\n\n" % sys.argv[1])
      usage(sys.argv[0])
      sys.exit(1)

  trails = parse_trails_log(infile)

  output_summary(trails, sys.stdout)
  output_trail_length_frequencies(trails, sys.stdout)
  output_trail_frequencies(trails, sys.stdout)
  output_txn_body_frequencies(trails, sys.stdout)

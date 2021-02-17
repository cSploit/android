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

import matplotlib.mlab as mlab
import matplotlib.pyplot as plt
from matplotlib import pylab
import numpy as np

import csv
import sys

min_rev = 35000

data_reader = csv.reader(open('data.csv'))

data = []
for row in data_reader:
  row = row[:-1]
  if row[0] == 'Revision':
    data.append(row)
    continue

  if int(row[0]) < min_rev:
    continue

  for i, x in enumerate(row):
    if i <= 1:
      row[i] = int(row[i])
    else:
      row[i] = int(row[i-1]) + int(row[i])
  data.append(row)

x = [d[0] for d in data[1:]]
data = [d[1:] for d in data]
y = zip(*data)

l = []
for i, foo in enumerate(y):
  ln = plt.plot(x, foo[1:], linewidth=1)
  l.append(ln)

plt.figlegend(l, data[0], 'lower left')
plt.fill_between(x, 0, y[0][1:], facecolor=l[0].color)
#for i in range(0, len(y)-1):
#  plt.fill_between(x, y[i][1:], y[i+1][1:])
plt.xlabel('Revision')
plt.ylabel('Symbol Count')
plt.show()

png = open('chart2.png', 'w')
plt.savefig(png)

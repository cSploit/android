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
#
# graph-svn-dav.py by Brian W. Fitzpatrick <fitz@red-bean.com>
#
# This was originally a quick hack to make a pretty picture of svn DAV servers.
#
# I've dropped it in Subversion's repository at the request of Karl Fogel.
#
# Be warned this this script has many dependencies that don't ship with Python.

import sys
import os
import fileinput
import datetime
import time
import datetime
from matplotlib import dates
import matplotlib
matplotlib.use('Agg')
from matplotlib import pylab
import Image

OUTPUT_FILE = '../../www/images/svn-dav-securityspace-survey.png'
OUTPUT_IMAGE_WIDTH = 800

STATS = [
  ('1/1/2003', 70),
  ('2/1/2003', 158),
  ('3/1/2003', 222),
  ('4/1/2003', 250),
  ('5/1/2003', 308),
  ('6/1/2003', 369),
  ('7/1/2003', 448),
  ('8/1/2003', 522),
  ('9/1/2003', 665),
  ('10/1/2003', 782),
  ('11/1/2003', 969),
  ('12/1/2003', 1009),
  ('1/1/2004', 1162),
  ('2/1/2004', 1307),
  ('3/1/2004', 1424),
  ('4/1/2004', 1792),
  ('5/1/2004', 2113),
  ('6/1/2004', 2502),
  ('7/1/2004', 2941),
  ('8/1/2004', 3863),
  ('9/1/2004', 4174),
  ('10/1/2004', 4187),
  ('11/1/2004', 4783),
  ('12/1/2004', 4995),
  ('1/1/2005', 5565),
  ('2/1/2005', 6505),
  ('3/1/2005', 7897),
  ('4/1/2005', 8751),
  ('5/1/2005', 9793),
  ('6/1/2005', 11534),
  ('7/1/2005', 12808),
  ('8/1/2005', 13545),
  ('9/1/2005', 15233),
  ('10/1/2005', 17588),
  ('11/1/2005', 18893),
  ('12/1/2005', 20278),
  ('1/1/2006', 21084),
  ('2/1/2006', 23861),
  ('3/1/2006', 26540),
  ('4/1/2006', 29396),
  ('5/1/2006', 33001),
  ('6/1/2006', 35082),
  ('7/1/2006', 38939),
  ('8/1/2006', 40672),
  ('9/1/2006', 46525),
  ('10/1/2006', 54247),
  ('11/1/2006', 63145),
  ('12/1/2006', 68988),
  ('1/1/2007', 77027),
  ('2/1/2007', 84813),
  ('3/1/2007', 95679),
  ('4/1/2007', 103852),
  ('5/1/2007', 117267),
  ('6/1/2007', 133665),
  ('7/1/2007', 137575),
  ('8/1/2007', 155426),
  ('9/1/2007', 159055),
  ('10/1/2007', 169939),
  ('11/1/2007', 180831),
  ('12/1/2007', 187093),
  ('1/1/2008', 199432),
  ('2/1/2008', 221547),
  ('3/1/2008', 240794),
  ('4/1/2008', 255520),
  ('5/1/2008', 269478),
  ('6/1/2008', 286614),
  ('7/1/2008', 294579),
  ('8/1/2008', 307923),
  ('9/1/2008', 254757),
  ('10/1/2008', 268081),
  ('11/1/2008', 299071),
  ('12/1/2008', 330884),
  ('1/1/2009', 369719),
  ('2/1/2009', 378434),
  ('3/1/2009', 390502),
  ('4/1/2009', 408658),
  ('5/1/2009', 407044),
  ('6/1/2009', 406520),
  ('7/1/2009', 334276),
  ]


def get_date(raw_date):
  month, day, year = map(int, raw_date.split('/'))
  return datetime.datetime(year, month, day)


def get_ordinal_date(date):
  # This is the only way I can get matplotlib to do the dates right.
  return int(dates.date2num(get_date(date)))


def load_stats():
  dates = [get_ordinal_date(date) for date, value in STATS]
  counts = [x[1] for x in STATS]

  return dates, counts


def draw_graph(dates, counts):
  ###########################################################
  # Drawing takes place here.
  pylab.figure(1)

  ax = pylab.subplot(111)
  pylab.plot_date(dates, counts,
                  color='r', linestyle='-', marker='o', markersize=3)

  ax.xaxis.set_major_formatter( pylab.DateFormatter('%Y') )
  ax.xaxis.set_major_locator( pylab.YearLocator() )
  ax.xaxis.set_minor_locator( pylab.MonthLocator() )
  ax.set_xlim( (dates[0] - 92, dates[len(dates) - 1] + 92) )

  ax.yaxis.set_major_formatter( pylab.FormatStrFormatter('%d') )

  pylab.ylabel('Total # of Public DAV Servers')

  lastdate = datetime.datetime.fromordinal(dates[len(dates) - 1]).strftime("%B %Y")
  pylab.xlabel("Data as of " + lastdate)
  pylab.title('Security Space Survey of\nPublic Subversion DAV Servers')
  # End drawing
  ###########################################################
  png = open(OUTPUT_FILE, 'w')
  pylab.savefig(png)
  png.close()
  os.rename(OUTPUT_FILE, OUTPUT_FILE + ".tmp.png")
  try:
    im = Image.open(OUTPUT_FILE + ".tmp.png", 'r')
    (width, height) = im.size
    print("Original size: %d x %d pixels" % (width, height))
    scale = float(OUTPUT_IMAGE_WIDTH) / float(width)
    width = OUTPUT_IMAGE_WIDTH
    height = int(float(height) * scale)
    print("Final size: %d x %d pixels" % (width, height))
    im = im.resize((width, height), Image.ANTIALIAS)
    im.save(OUTPUT_FILE, im.format)
    os.unlink(OUTPUT_FILE + ".tmp.png")
  except Exception, e:
    sys.stderr.write("Error attempting to resize the graphic: %s\n" % (str(e)))
    os.rename(OUTPUT_FILE + ".tmp.png", OUTPUT_FILE)
    raise
  pylab.close()


if __name__ == '__main__':
  dates, counts = load_stats()
  draw_graph(dates, counts)
  print("Don't forget to update ../../www/svn-dav-securityspace-survey.html!")

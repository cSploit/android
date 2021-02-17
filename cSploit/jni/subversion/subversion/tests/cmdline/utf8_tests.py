#!/usr/bin/env python
#  -*- coding: utf-8 -*-
#
#  utf8_tests.py:  testing the svn client's utf8 (i18n) handling
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

# General modules
import sys, re, os, locale

# Our testing module
import svntest
from svntest import wc

# (abbreviation)
Item = wc.StateItem
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco

#--------------------------------------------------------------------
# Data

# Here's a filename and a log message which contain some high-ascii
# data.  In theory this data has different interpretations when
# converting from 2 different charsets into UTF-8.

### "bÔçÅ" in ISO-8859-1 encoding:
i18n_filename = 'b\xd4\xe7\xc5'

### "drieÃ«ntwintig keer was Ã©Ã©n keer teveel" in ISO-8859-1 encoding:
i18n_logmsg = 'drie\xc3\xabntwintig keer was \xc3\xa9\xc3\xa9n keer teveel'


######################################################################
# Tests
#
#   Each test must return on success or raise on failure.

@Skip()
def basic_utf8_conversion(sbox):
  "conversion of paths and logs to/from utf8"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Create the new i18n file and schedule it for addition
  svntest.main.file_append(os.path.join(wc_dir, i18n_filename), "hi")
  svntest.actions.run_and_verify_svn(
    "Failed to schedule i18n filename for addition", None, [],
    'add', os.path.join(wc_dir, i18n_filename))

  svntest.actions.run_and_verify_svn(
    "Failed to commit i18n filename", None, [],
    'commit', '-m', i18n_logmsg, wc_dir)

# Here's how the test should really work:

# 1. sh LC_ALL=ISO-8859-1 svn commit <filename> -m "<logmsg>"

# 2. sh LC_ALL=UTF-8 svn log -rHEAD > output

# 3. verify that output is the exact UTF-8 data that we expect.

# 4. repeat the process using some other locale other than ISO8859-1,
#    preferably some locale which will convert the high-ascii data to
#    *different* UTF-8.



#----------------------------------------------------------------------

########################################################################
# Run the tests

try:
  # Generic setlocale so that getlocale returns something sensible
  locale.setlocale(locale.LC_ALL, '')

  # Try to make these test run in an ISO-8859-1 environment, otherwise
  # they would run in whatever random locale the testing platform
  # happens to have, and then we couldn't predict the exact results.
  if svntest.main.windows:
    # In this case, it would probably be "english_usa.1252", but you should
    # be able to set just the encoding by using ".1252" (that's codepage
    # 1252, which is almost but not quite entirely unlike tea; um, I mean
    # it's very similar to ISO-8859-1).
    #                                     -- Branko Čibej <brane@xbc.nu>
    locale.setlocale(locale.LC_ALL, '.1252')
  else:
    locale.setlocale(locale.LC_ALL, 'en_US.ISO8859-1')

    if os.putenv:
      # propagate to the svn* executables, so they do the correct translation
      # the line below works for Linux systems if they have the particular
      # locale installed
      os.environ['LC_ALL'] = "en_US.ISO8859-1"
except:
  pass

# Check to see if the locale uses ISO-8859-1 encoding.  The regex is necessary
# because some systems ommit the first hyphen or use lowercase letters for ISO.
if sys.platform == 'win32':
  localematch = 1
else:
  localeenc = locale.getlocale()[1]
  if localeenc:
    localeregex = re.compile('^ISO-?8859-1$', re.I)
    localematch = localeregex.search(localeenc)
    try:
      svntest.actions.run_and_verify_svn(None, svntest.SVNAnyOutput, [],"help")
    except:
      # We won't be able to run the client; this might be because the
      # system does not support the iso-8859-1 locale. Anyhow, it makes
      # no sense to run the test.
      localematch = None
  else:
    localematch = None

# Also check that the environment contains the expected locale settings
# either by default, or because we set them above.
if localematch:
  localeregex = re.compile('^en_US\.ISO-?8859-1$', re.I)
  for env in [ 'LC_ALL', 'LC_CTYPE', 'LANG' ]:
    env_value = os.getenv(env)
    if env_value:
      if localeregex.search(env_value):
        break
      else:
        localematch = None
        break


########################################################################
# Run the tests

# list all tests here, starting with None:
test_list = [ None,
              basic_utf8_conversion,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.

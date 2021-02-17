#!/bin/env python
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

# $Id: gen_junit_report.py 1141953 2011-07-01 14:42:56Z rhuijben $
"""
gen_junit_report.py -- The script is to generate the junit report for
Subversion tests.  The script uses the log file, tests.log created by
"make check" process. It parses the log file and generate the junit
files for each test separately in the specified output directory. The
script can take --log-file and --output-dir arguments.
"""

import sys
import os
import getopt

def replace_from_map(data, encode):
    """replace substrings in DATA with replacements defined in ENCODING"""
    for pattern, replacement in encode.items():
        data = data.replace(pattern, replacement)
    return data

xml_encode_map = {
      '&': '&amp;',
      '<': '&lt;',
      '>': '&gt;',
      '"': '&quot;',
      "'": '&apos;',
      }

def xml_encode(data):
    """encode the xml characters in the data"""
    return replace_from_map(data, xml_encode_map)

special_encode_map = {
    ']]>': ']]]]><![CDATA[>', # CDATA terminator sequence
    '\000': '&#9216;',        # U+2400 SYMBOL FOR NULL
    '\001': '&#9217;',        # U+2401 SYMBOL FOR START OF HEADING
    '\002': '&#9218;',        # U+2402 SYMBOL FOR START OF TEXT
    '\003': '&#9219;',        # U+2403 SYMBOL FOR END OF TEXT
    '\004': '&#9220;',        # U+2404 SYMBOL FOR END OF TRANSMISSION
    '\005': '&#9221;',        # U+2405 SYMBOL FOR ENQUIRY
    '\006': '&#9222;',        # U+2406 SYMBOL FOR ACKNOWLEDGE
    '\007': '&#9223;',        # U+2407 SYMBOL FOR BELL
    '\010': '&#9224;',        # U+2408 SYMBOL FOR BACKSPACE
    '\011': '&#9225;',        # U+2409 SYMBOL FOR HORIZONTAL TABULATION
   #'\012': '&#9226;',        # U+240A SYMBOL FOR LINE FEED
    '\013': '&#9227;',        # U+240B SYMBOL FOR VERTICAL TABULATION
    '\014': '&#9228;',        # U+240C SYMBOL FOR FORM FEED
   #'\015': '&#9229;',        # U+240D SYMBOL FOR CARRIAGE RETURN
    '\016': '&#9230;',        # U+240E SYMBOL FOR SHIFT OUT
    '\017': '&#9231;',        # U+240F SYMBOL FOR SHIFT IN
    '\020': '&#9232;',        # U+2410 SYMBOL FOR DATA LINK ESCAPE
    '\021': '&#9233;',        # U+2411 SYMBOL FOR DEVICE CONTROL ONE
    '\022': '&#9234;',        # U+2412 SYMBOL FOR DEVICE CONTROL TWO
    '\023': '&#9235;',        # U+2413 SYMBOL FOR DEVICE CONTROL THREE
    '\024': '&#9236;',        # U+2414 SYMBOL FOR DEVICE CONTROL FOUR
    '\025': '&#9237;',        # U+2415 SYMBOL FOR NEGATIVE ACKNOWLEDGE
    '\026': '&#9238;',        # U+2416 SYMBOL FOR SYNCHRONOUS IDLE
    '\027': '&#9239;',        # U+2417 SYMBOL FOR END OF TRAMSNISSION BLOCK
    '\030': '&#9240;',        # U+2418 SYMBOL FOR CANCEL
    '\031': '&#9241;',        # U+2419 SYMBOL FOR END OF MEDIUM
    '\032': '&#9242;',        # U+241A SYMBOL FOR SUBSTITUTE
    '\033': '&#9243;',        # U+241B SYMBOL FOR ESCAPE
    '\034': '&#9244;',        # U+241C SYMBOL FOR FILE SEPARATOR
    '\035': '&#9245;',        # U+241D SYMBOL FOR GROUP SEPARATOR
    '\036': '&#9246;',        # U+241E SYMBOL FOR RECORD SEPARATOR
    '\037': '&#9247;',        # U+241F SYMBOL FOR UNIT SEPARATOR
    }

def escape_special_characters(data):
    """remove special characters in test failure reasons"""
    if data:
        data = replace_from_map(data, special_encode_map)
    return data

def start_junit():
    """define the beginning of xml document"""
    head = """<?xml version="1.0" encoding="UTF-8"?>"""
    return head

def start_testsuite(test_name):
    """start testsuite. The value for the attributes are replaced later
    when the junit file handling is concluded"""
    sub_test_name = test_name.replace('.', '-')
    start = """<testsuite time="ELAPSED_%s" tests="TOTAL_%s" name="%s"
    failures="FAIL_%s" errors="FAIL_%s" skipped="SKIP_%s">""" % \
    (test_name, test_name, sub_test_name, test_name, test_name, test_name)
    return start

def junit_testcase_ok(test_name, casename):
    """mark the test case as PASSED"""
    casename = xml_encode(casename)
    sub_test_name = test_name.replace('.', '-')
    case = """<testcase time="ELAPSED_CASE_%s" name="%s" classname="%s"/>""" % \
    (test_name, casename, sub_test_name)
    return case

def junit_testcase_fail(test_name, casename, reason=None):
    """mark the test case as FAILED"""
    casename = xml_encode(casename)
    sub_test_name = test_name.replace('.', '-')
    reason = escape_special_characters(reason)
    case = """<testcase time="ELAPSED_CASE_%s" name="%s" classname="%s">
      <failure type="Failed"><![CDATA[%s]]></failure>
    </testcase>""" % (test_name, casename, sub_test_name, reason)
    return case

def junit_testcase_xfail(test_name, casename, reason=None):
    """mark the test case as XFAILED"""
    casename = xml_encode(casename)
    sub_test_name = test_name.replace('.', '-')
    reason = escape_special_characters(reason)
    case = """<testcase time="ELAPSED_CASE_%s" name="%s" classname="%s">
      <system-out><![CDATA[%s]]></system-out>
    </testcase>""" % (test_name, casename, sub_test_name, reason)
    return case

def junit_testcase_skip(test_name, casename):
    """mark the test case as SKIPPED"""
    casename = xml_encode(casename)
    sub_test_name = test_name.replace('.', '-')
    case = """<testcase time="ELAPSED_CASE_%s" name="%s" classname="%s">
      <skipped message="Skipped"/>
    </testcase>""" % (test_name, casename, sub_test_name)
    return case

def end_testsuite():
    """mark the end of testsuite"""
    end = """</testsuite>"""
    return end

def update_stat(test_name, junit, count):
    """update the test statistics in the junit string"""
    junit_str = '\n'.join(junit)
    t_count = count[test_name]
    total = float(t_count['pass'] + t_count['fail'] + t_count['skip'])
    elapsed = float(t_count['elapsed'])
    case_time = 0
    if total > 0: # there are tests with no test cases
        case_time = elapsed/total

    total_patt = 'TOTAL_%s' % test_name
    fail_patt = 'FAIL_%s' % test_name
    skip_patt = 'SKIP_%s' % test_name
    elapsed_patt = 'ELAPSED_%s' % test_name
    elapsed_case_patt = 'ELAPSED_CASE_%s' % test_name

    # replace the pattern in junit string with actual statistics
    junit_str = junit_str.replace(total_patt, "%s" % total)
    junit_str = junit_str.replace(fail_patt, "%s" % t_count['fail'])
    junit_str = junit_str.replace(skip_patt, "%s" % t_count['skip'])
    junit_str = junit_str.replace(elapsed_patt, "%.3f" % elapsed)
    junit_str = junit_str.replace(elapsed_case_patt, "%.3f" % case_time)
    return junit_str

def main():
    """main method"""
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'l:d:h',
                                  ['log-file=', 'output-dir=', 'help'])
    except getopt.GetoptError, err:
        usage(err)

    log_file = None
    output_dir = None
    for opt, value in opts:
        if (opt in ('-h', '--help')):
            usage()
        elif (opt in ('-l', '--log-file')):
            log_file = value
        elif (opt in ('-d', '--output-dir')):
            output_dir = value
        else:
            usage('Unable to recognize option')

    if not log_file or not output_dir:
        usage("The options --log-file and --output-dir are mandatory")

    # create junit output directory, if not exists
    if not os.path.exists(output_dir):
        print("Directory '%s' not exists, creating ..." % output_dir)
        try:
            os.makedirs(output_dir)
        except OSError, err:
            sys.stderr.write("ERROR: %s\n" % err)
            sys.exit(1)
    patterns = {
      'start' : 'START:',
      'end' : 'END:',
      'pass' : 'PASS:',
      'skip' : 'SKIP:',
      'fail' : 'FAIL:',
      'xfail' : 'XFAIL:',
      'elapsed' : 'ELAPSED:'
    }

    junit = []
    junit.append(start_junit())
    reason = None
    count = {}
    fp = None
    try:
        fp = open(log_file, 'r')
    except IOError, err:
        sys.stderr.write("ERROR: %s\n" % err)
        sys.exit(1)

    for line in fp.readlines():
        line = line.strip()
        if line.startswith(patterns['start']):
            reason = ""
            test_name = line.split(' ')[1]
            # replace '.' in test name with '_' to avoid confusing class
            # name in test result displayed in the CI user interface
            test_name.replace('.', '_')
            count[test_name] = {
              'pass' : 0,
              'skip' : 0,
              'fail' : 0,
              'xfail' : 0,
              'elapsed' : 0,
              'total' : 0
            }
            junit.append(start_testsuite(test_name))
        elif line.startswith(patterns['end']):
            junit.append(end_testsuite())
        elif line.startswith(patterns['pass']):
            reason = ""
            casename = line.strip(patterns['pass']).strip()
            junit.append(junit_testcase_ok(test_name, casename))
            count[test_name]['pass'] += 1
        elif line.startswith(patterns['skip']):
            reason = ""
            casename = line.strip(patterns['skip']).strip()
            junit.append(junit_testcase_skip(test_name, casename))
            count[test_name]['skip'] += 1
        elif line.startswith(patterns['fail']):
            casename = line.strip(patterns['fail']).strip()
            junit.append(junit_testcase_fail(test_name, casename, reason))
            count[test_name]['fail'] += 1
            reason = ""
        elif line.startswith(patterns['xfail']):
            casename = line.strip(patterns['xfail']).strip()
            junit.append(junit_testcase_xfail(test_name, casename, reason))
            count[test_name]['pass'] += 1
            reason = ""
        elif line.startswith(patterns['elapsed']):
            reason = ""
            elapsed = line.split(' ')[2].strip()
            (hrs, mins, secs) = elapsed.split(':')
            secs_taken = int(hrs)*24 + int(mins)*60 + float(secs)
            count[test_name]['elapsed'] = secs_taken

            junit_str = update_stat(test_name, junit, count)
            test_junit_file = os.path.join(output_dir,
                                           "%s.junit.xml" % test_name)
            w_fp = open (test_junit_file, 'w')
            w_fp.writelines(junit_str)
            w_fp.close()
            junit = []
        elif len(line):
            reason = "%s\n%s" % (reason, line)
    fp.close()

def usage(errorMsg=None):
    script_name = os.path.basename(sys.argv[0])
    sys.stdout.write("""USAGE: %s: [--help|h] --log-file|l --output-dir|d

Options:
  --help|-h       Display help message
  --log-file|l    The log file to parse for generating junit xml files
  --output-dir|d  The directory to create the junit xml file for each
                  test
""" % script_name)
    if errorMsg is not None:
        sys.stderr.write("\nERROR: %s\n" % errorMsg)
        sys.exit(1)
    sys.exit(0)

if __name__ == '__main__':
    main()

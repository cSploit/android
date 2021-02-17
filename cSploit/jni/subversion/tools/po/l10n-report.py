#!/usr/bin/env python
#
# $Id: l10n-report.py 1132657 2011-06-06 14:23:36Z julianfoad $
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

"""Usage: l10n-report.py [OPTION...]

Send the l10n translation status report to an email address. If the
email address is not specified, print in stdout.

Options:

 -h, --help           Show this help message.

 -m, --to-email-id    Send the l10n translation status report to this
                      email address.
"""

import sys
import getopt
import os
import re
import subprocess

FROM_ADDRESS = "Subversion Translation Status <noreply@subversion.apache.org>"
LIST_ADDRESS = "dev@subversion.apache.org"
SUBJECT_TEMPLATE = "[l10n] Translation status report for %s r%s"
MAIL_THREAD_ID = '<translation_status_report_for_%s@subversion.apache.org>'

def _rev():
  dollar = "$Revision: 1132657 $"
  return int(re.findall('[0-9]+', dollar)[0]);

def usage_and_exit(errmsg=None):
    """Print a usage message, plus an ERRMSG (if provided), then exit.
    If ERRMSG is provided, the usage message is printed to stderr and
    the script exits with a non-zero error code.  Otherwise, the usage
    message goes to stdout, and the script exits with a zero
    errorcode."""
    if errmsg is None:
        stream = sys.stdout
    else:
        stream = sys.stderr
    stream.write("%s\n" % __doc__)
    stream.flush()
    if errmsg:
        stream.write("\nError: %s\n" % errmsg)
        stream.flush()
        sys.exit(2)
    sys.exit(0)


class l10nReport:
    def __init__(self, to_email_id=""):
        self.to_email_id = to_email_id
        self.from_email_id = "<%s>" % LIST_ADDRESS

    def safe_command(self, cmd_and_args, cmd_in=""):
        [stdout, stderr] = subprocess.Popen(cmd_and_args, \
                           stdin=subprocess.PIPE, \
                           stdout=subprocess.PIPE, \
                           stderr=subprocess.PIPE).communicate(input=cmd_in)
        return stdout, stderr

    def match(self, pattern, string):
        if isinstance(pattern, basestring):
            pattern = re.compile(pattern)
        match = re.compile(pattern).search(string)
        if match and match.groups():
            return match.group(1)
        else:
            return None

    def get_msgattribs(self, file):
        msgout = self.safe_command(['msgattrib', '--translated', file])[0]
        grepout = self.safe_command(['grep', '-E', '^msgid *"'], msgout)[0]
        sedout = self.safe_command(['sed', '1d'], grepout)[0]
        trans = self.safe_command(['wc', '-l'], sedout)[0]

        msgout = self.safe_command(['msgattrib', '--untranslated', file])[0]
        grepout = self.safe_command(['grep', '-E', '^msgid *"'], msgout)[0]
        sedout = self.safe_command(['sed', '1d'], grepout)[0]
        untrans = self.safe_command(['wc', '-l'], sedout)[0]

        msgout = self.safe_command(['msgattrib', '--only-fuzzy', file])[0]
        grepout = self.safe_command(['grep', '-E', '^msgid *"'], msgout)[0]
        sedout = self.safe_command(['sed', '1d'], grepout)[0]
        fuzzy = self.safe_command(['wc', '-l'], sedout)[0]

        msgout = self.safe_command(['msgattrib', '--only-obsolete', file])[0]
        grepout = self.safe_command(['grep', '-E', '^#~ msgid *"'], msgout)[0]
        obsolete = self.safe_command(['wc', '-l'], grepout)[0]

        return int(trans), int(untrans), int(fuzzy), int(obsolete)

    def pre_l10n_report(self):
        # svn revert --recursive subversion/po
        cmd = ['svn', 'revert', '--recursive', 'subversion/po']
        stderr = self.safe_command(cmd)[1]
        if stderr:
          sys.stderr.write("\nError: %s\n" % stderr)
          sys.stderr.flush()
          sys.exit(0)

        # svn update
        cmd = ['svn', 'update']
        stderr = self.safe_command(cmd)[1]
        if stderr:
          sys.stderr.write("\nError: %s\n" % stderr)
          sys.stderr.flush()
          sys.exit(0)

        # tools/po/po-update.sh
        cmd = ['sh', 'tools/po/po-update.sh']
        self.safe_command(cmd)


def bar_graph(nominal_length, trans, untrans, fuzzy, obsolete):
    """Format the given four counts into a bar graph string in which the
    total length of the bars representing the TRANS, UNTRANS and FUZZY
    counts is NOMINAL_LENGTH characters, and the bar representing the
    OBSOLETE count extends beyond that."""

    total_count = trans + untrans + fuzzy  # don't include 'obsolete'
    accum_bar = 0
    accum_count = 0
    s = ''
    for count, letter in [(trans, '+'), (untrans, 'U'), (fuzzy, '~'),
                          (obsolete, 'o')]:
        accum_count += count
        new_bar_end = nominal_length * accum_count / total_count
        s += letter * (new_bar_end - accum_bar)
        accum_bar = new_bar_end
    return s


def main():
    # Parse the command-line options and arguments.
    try:
        opts, args = getopt.gnu_getopt(sys.argv[1:], "hm:",
                                       ["help",
                                        "to-email-id=",
                                        ])
    except getopt.GetoptError, msg:
        usage_and_exit(msg)

    to_email_id = None
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage_and_exit()
        elif opt in ("-m", "--to-email-id"):
            to_email_id = arg

    l10n = l10nReport()
    os.chdir("%s/../.." % os.path.dirname(os.path.abspath(sys.argv[0])))
    l10n.pre_l10n_report()
    [info_out, info_err] = l10n.safe_command(['svn', 'info'])
    if info_err:
        sys.stderr.write("\nError: %s\n" % info_err)
        sys.stderr.flush()
        sys.exit(0)

    po_dir = 'subversion/po'
    branch_name = l10n.match('URL:.*/asf/subversion/(\S+)', info_out)
    [info_out, info_err] = l10n.safe_command(['svnversion', po_dir])
    if info_err:
        sys.stderr.write("\nError: %s\n" % info_err)
        sys.stderr.flush()
        sys.exit(0)

    wc_version = re.sub('[MS]', '', info_out.strip())
    title = "Translation status report for %s@r%s" % \
               (branch_name, wc_version)

    os.chdir(po_dir)
    files = sorted(os.listdir('.'))
    format_head = "\n%6s %7s %7s %7s %7s" % ("lang", "trans", "untrans",
                   "fuzzy", "obs")
    format_line = "--------------------------------------"
    print("\n%s\n%s\n%s" % (title, format_head, format_line))

    body = ""
    po_pattern = re.compile('(.*).po$')
    for file in files:
        lang = l10n.match(po_pattern, file)
        if not lang:
            continue
        [trans, untrans, fuzzy, obsolete]  = l10n.get_msgattribs(file)
        po_format = "%6s %7d %7d %7d %7d" %\
                    (lang, trans, untrans, fuzzy, obsolete)
        po_format += "  " + bar_graph(30, trans, untrans, fuzzy, obsolete)
        body += "%s\n" % po_format
        print(po_format)

    if to_email_id:
        import smtplib
        # Ensure compatibility of the email module all the way to Python 2.3
        try:
            from email.message import Message
        except ImportError:
            from email.Message import Message

        msg = Message()
        msg["From"] = FROM_ADDRESS
        msg["To"] = to_email_id
        msg["Subject"] = SUBJECT_TEMPLATE % (branch_name, wc_version)
        msg["X-Mailer"] = "l10n-report.py r%s" % _rev()
        msg["Reply-To"] = LIST_ADDRESS
        msg["Mail-Followup-To"] = LIST_ADDRESS
        msg["In-Reply-To"] = MAIL_THREAD_ID % (branch_name.replace('/', '_'))
        msg["References"] = msg["In-Reply-To"]

        # http://www.iana.org/assignments/auto-submitted-keywords/auto-submitted-keywords.xhtml
        msg["Auto-Submitted"] = 'auto-generated'

        msg.set_type("text/plain")
        msg.set_payload("\n".join((title, format_head, format_line, body)))

        server = smtplib.SMTP('localhost')
        server.sendmail("From: " + FROM_ADDRESS,
                        "To: " + to_email_id,
                        msg.as_string())
        print("The report is sent to '%s' email id." % to_email_id)
    else:
        print("\nYou have not passed '-m' option, so email is not sent.")

if __name__ == "__main__":
    main()

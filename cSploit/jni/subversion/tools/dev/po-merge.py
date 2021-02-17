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

import os, re, sys

msgstr_re = re.compile('msgstr\[\d+\] "')

def parse_translation(f):
    """Read a single translation entry from the file F and return a
    tuple with the comments, msgid, msgid_plural and msgstr.  The comments is
    returned as a list of lines which do not end in new-lines.  The msgid is
    string.  The msgid_plural is string or None.  The msgstr is a list of
    strings.  The msgid, msgid_plural and msgstr strings can contain embedded
    newlines"""
    line = f.readline()

    # Parse comments
    comments = []
    while True:
        if line.strip() == '' or line[:2] == '#~':
            return comments, None, None, None
        elif line[0] == '#':
            comments.append(line[:-1])
        else:
            break
        line = f.readline()

    # Parse msgid
    if line[:7] != 'msgid "' or line[-2] != '"':
        raise RuntimeError("parse error")
    msgid = line[6:-1]
    while True:
        line = f.readline()
        if line[0] != '"':
            break
        msgid += '\n' + line[:-1]

    # Parse optional msgid_plural
    msgid_plural = None
    if line[:14] == 'msgid_plural "':
        if line[-2] != '"':
            raise RuntimeError("parse error")
        msgid_plural = line[13:-1]
        while True:
            line = f.readline()
            if line[0] != '"':
                break
            msgid_plural += '\n' + line[:-1]

    # Parse msgstr
    msgstr = []
    if not msgid_plural:
        if line[:8] != 'msgstr "' or line[-2] != '"':
            raise RuntimeError("parse error")
        msgstr.append(line[7:-1])
        while True:
            line = f.readline()
            if len(line) == 0 or line[0] != '"':
                break
            msgstr[0] += '\n' + line[:-1]
    else:
        if line[:7] != 'msgstr[' or line[-2] != '"':
            raise RuntimeError("parse error")
        i = 0
        while True:
            matched_msgstr = msgstr_re.match(line)
            if matched_msgstr:
                matched_msgstr_len = len(matched_msgstr.group(0))
                msgstr.append(line[matched_msgstr_len-1:-1])
            else:
                break
            while True:
                line = f.readline()
                if len(line) == 0 or line[0] != '"':
                    break
                msgstr[i] += '\n' + line[:-1]
            i += 1

    if line.strip() != '':
        raise RuntimeError("parse error")

    return comments, msgid, msgid_plural, msgstr

def split_comments(comments):
    """Split COMMENTS into flag comments and other comments.  Flag
    comments are those that begin with '#,', e.g. '#,fuzzy'."""
    flags = []
    other = []
    for c in comments:
        if len(c) > 1 and c[1] == ',':
            flags.append(c)
        else:
            other.append(c)
    return flags, other

def main(argv):
    if len(argv) != 2:
        argv0 = os.path.basename(argv[0])
        sys.exit('Usage: %s <lang.po>\n'
                 '\n'
                 'This script will replace the translations and flags in lang.po with\n'
                 'the translations and flags in the source po file read from standard\n'
                 'input.  Strings that are not found in the source file are left untouched.\n'
                 'A backup copy of lang.po is saved as lang.po.bak.\n'
                 '\n'
                 'Example:\n'
                 '    svn cat http://svn.apache.org/repos/asf/subversion/trunk/subversion/po/sv.po | \\\n'
                 '        %s sv.po' % (argv0, argv0))

    # Read the source po file into a hash
    source = {}
    while True:
        comments, msgid, msgid_plural, msgstr = parse_translation(sys.stdin)
        if not comments and msgid is None:
            break
        if msgid is not None:
            source[msgid] = msgstr, split_comments(comments)[0]

    # Make a backup of the output file, open the copy for reading
    # and the original for writing.
    os.rename(argv[1], argv[1] + '.bak')
    infile = open(argv[1] + '.bak')
    outfile = open(argv[1], 'w')

    # Loop thought the original and replace stuff as we go
    first = 1
    string_count = 0
    update_count = 0
    untranslated = 0
    while True:
        comments, msgid, msgid_plural, msgstr = parse_translation(infile)
        if not comments and msgid is None:
            break
        if not first:
            outfile.write('\n')
        first = 0
        if msgid is None:
            outfile.write('\n'.join(comments) + '\n')
        else:
            string_count += 1
            # Do not update the header, and only update if the source
            # has a non-empty translation.
            if msgid != '""' and source.get(msgid, ['""', []])[0] != '""':
                other = split_comments(comments)[1]
                new_msgstr, new_flags = source[msgid]
                new_comments = other + new_flags
                if new_msgstr != msgstr or new_comments != comments:
                    update_count += 1
                    msgstr = new_msgstr
                    comments = new_comments
            outfile.write('\n'.join(comments) + '\n')
            outfile.write('msgid ' + msgid + '\n')
            if not msgid_plural:
                outfile.write('msgstr ' + msgstr[0] + '\n')
            else:
                outfile.write('msgid_plural ' + msgid_plural + '\n')
                n = 0
                for i in msgstr:
                    outfile.write('msgstr[%s] %s\n' % (n, msgstr[n]))
                    n += 1
        for m in msgstr:
            if m == '""':
                untranslated += 1

    # We're done.  Tell the user what we did.
    print(('%d strings updated. '
          '%d of %d strings are still untranslated (%.0f%%).' %
          (update_count, untranslated, string_count,
           100.0 * untranslated / string_count)))

if __name__ == '__main__':
    main(sys.argv)

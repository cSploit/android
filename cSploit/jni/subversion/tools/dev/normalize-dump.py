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

import sys
import re

header_re = re.compile(r'^([^:]*): ?(.*)$')

class NodePath:
    def __init__(self, path, headers):
        self.path = path
        self.headers = headers

    def dump(self):
        print((' ' * 3) + self.path)
        headers = sorted(self.headers.keys())
        for header in headers:
            print((' ' * 6) + header + ': ' + self.headers[header])


def dump_revision(rev, nodepaths):
    sys.stderr.write('* Normalizing revision ' + rev + '...')
    print('Revision ' + rev)
    paths = sorted(nodepaths.keys())
    for path in paths:
        nodepath = nodepaths[path]
        nodepath.dump()
    sys.stderr.write('done\n')



def parse_header_block(fp):
    headers = {}
    while True:
        line = fp.readline()
        if line == '':
            return headers, 1
        line = line.strip()
        if line == '':
            return headers, 0
        matches = header_re.match(line)
        if not matches:
            raise Exception('Malformed header block')
        headers[matches.group(1)] = matches.group(2)


def parse_file(fp):
    nodepaths = {}
    current_rev = None

    while True:
        # Parse a block of headers
        headers, eof = parse_header_block(fp)

        # This is a revision header block
        if 'Revision-number' in headers:

            # If there was a previous revision, dump it
            if current_rev:
                dump_revision(current_rev, nodepaths)

            # Reset the data for this revision
            current_rev = headers['Revision-number']
            nodepaths = {}

            # Skip the contents
            prop_len = headers.get('Prop-content-length', 0)
            fp.read(int(prop_len))

        # This is a node header block
        elif 'Node-path' in headers:

            # Make a new NodePath object, and add it to the
            # dictionary thereof
            path = headers['Node-path']
            node = NodePath(path, headers)
            nodepaths[path] = node

            # Skip the content
            text_len = headers.get('Text-content-length', 0)
            prop_len = headers.get('Prop-content-length', 0)
            fp.read(int(text_len) + int(prop_len))

        # Not a revision, not a node -- if we've already seen at least
        # one revision block, we are in an errorful state.
        elif current_rev and len(headers.keys()):
            raise Exception('Header block from outta nowhere')

        if eof:
            if current_rev:
                dump_revision(current_rev, nodepaths)
            break

def usage():
    print('Usage: ' + sys.argv[0] + ' [DUMPFILE]')
    print('')
    print('Reads a Subversion dumpfile from DUMPFILE (or, if not provided,')
    print('from stdin) and normalizes the metadata contained therein,')
    print('printing summarized and sorted information.  This is useful for')
    print('generating data about dumpfiles in a diffable fashion.')
    sys.exit(0)

def main():
    if len(sys.argv) > 1:
        if sys.argv[1] == '--help':
            usage()
        fp = open(sys.argv[1], 'rb')
    else:
        fp = sys.stdin
    parse_file(fp)


if __name__ == '__main__':
    main()





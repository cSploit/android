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

# mlpatch.py: Run with no arguments for usage

import sys, os
import sgmllib
try:
  # Python >=3.0
  from html.entities import entitydefs
  from urllib.request import urlopen as urllib_request_urlopen
except ImportError:
  # Python <3.0
  from htmlentitydefs import entitydefs
  from urllib2 import urlopen as urllib_request_urlopen
import fileinput

CHUNKSIZE = 8 * 1024

class MyParser(sgmllib.SGMLParser):
  def __init__(self):
    self.baseclass = sgmllib.SGMLParser
    self.baseclass.__init__(self)
    self.entitydefs = entitydefs
    self.entitydefs["nbsp"] = " "
    self.inbody = False
    self.complete_line = False
    self.discard_gathered()

  def discard_gathered(self):
    self.gather_data = False
    self.gathered_data = ""

  def noop(self):
    pass

  def out(self, data):
    sys.stdout.write(data)

  def handle_starttag(self, tag, method, attrs):
    if not self.inbody: return
    self.baseclass.handle_starttag(self, tag, method, attrs)

  def handle_endtag(self, tag, method):
    if not self.inbody: return
    self.baseclass.handle_endtag(self, tag, method)

  def handle_data(self, data):
    if not self.inbody: return
    data = data.replace('\n','')
    if len(data) == 0: return
    if self.gather_data:
      self.gathered_data += data
    else:
      if self.complete_line:
        if data[0] in ('+', '-', ' ', '#') \
            or data.startswith("Index:") \
            or data.startswith("@@ ") \
            or data.startswith("======"):
          # Real new line
          self.out('\n')
        else:
          # Presume that we are wrapped
          self.out(' ')
      self.complete_line = False
      self.out(data)

  def handle_charref(self, ref):
    if not self.inbody: return
    self.baseclass.handle_charref(self, ref)

  def handle_entityref(self, ref):
    if not self.inbody: return
    self.baseclass.handle_entityref(self, ref)

  def handle_comment(self, comment):
    if comment == ' body="start" ':
      self.inbody = True
    elif comment == ' body="end" ':
      self.inbody = False

  def handle_decl(self, data):
    if not self.inbody: return
    print("DECL: " + data)

  def unknown_starttag(self, tag, attrs):
    if not self.inbody: return
    print("UNKTAG: %s %s" % (tag, attrs))

  def unknown_endtag(self, tag):
    if not self.inbody: return
    print("UNKTAG: /%s" % (tag))

  def do_br(self, attrs):
    self.complete_line = True

  def do_p(self, attrs):
    if self.complete_line:
      self.out('\n')
    self.out(' ')
    self.complete_line = True

  def start_a(self, attrs):
    self.gather_data = True

  def end_a(self):
    self.out(self.gathered_data.replace('_at_', '@'))
    self.discard_gathered()

  def close(self):
    if self.complete_line:
      self.out('\n')
    self.baseclass.close(self)


def main():
  if len(sys.argv) == 1:
    sys.stderr.write(
    "usage:   mlpatch.py dev|users year month msgno > foobar.patch\n" +
    "example: mlpatch.py dev 2005 01 0001 > issue-XXXX.patch\n" +
    """
    Very annoyingly, the http://svn.haxx.se/ subversion mailing list archives
    mangle inline patches, and provide no raw message download facility
    (other than for an entire month's email as an mbox).

    So, I wrote this script, to demangle them. It's not perfect, as it has to
    guess about whitespace, but it does an acceptable job.\n""")
    sys.exit(0)
  elif len(sys.argv) != 5:
    sys.stderr.write("error: mlpatch.py: Bad parameters - run with no "
    + "parameters for usage\n")
    sys.exit(1)
  else:
    list, year, month, msgno = sys.argv[1:]
    url = "http://svn.haxx.se/" \
        + "%(list)s/archive-%(year)s-%(month)s/%(msgno)s.shtml" % locals()
    print("MsgUrl: " + url)
    msgfile = urllib_request_urlopen(url)
    p = MyParser()
    buffer = msgfile.read(CHUNKSIZE)
    while buffer:
      p.feed(buffer)
      buffer = msgfile.read(CHUNKSIZE)
    p.close()
    msgfile.close()

if __name__ == '__main__':
  main()

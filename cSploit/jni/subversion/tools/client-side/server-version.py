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
# server-version.py: print a Subversion server's version number
#
# USAGE: server-version.py URL
#
# The URL can contain any path on the server, as we are simply looking
# for Apache's response to OPTIONS, and its Server: header.
#
# EXAMPLE:
#
#   $ ./server-version.py http://svn.collab.net/
#                   or
#   $ ./server-version.py https://svn.collab.net/
#

import sys
try:
  # Python >=3.0
  from http.client import HTTPConnection as http_client_HTTPConnection
  from http.client import HTTPSConnection as http_client_HTTPSConnection
  from urllib.parse import urlparse as urllib_parse_urlparse
except ImportError:
  # Python <3.0
  from httplib import HTTPConnection as http_client_HTTPConnection
  from httplib import HTTPSConnection as http_client_HTTPSConnection
  from urlparse import urlparse as urllib_parse_urlparse


def print_version(url):
  scheme, netloc, path, params, query, fragment = urllib_parse_urlparse(url)
  if scheme == 'http':
    conn = http_client_HTTPConnection(netloc)
  elif scheme == 'https':
    conn = http_client_HTTPSConnection(netloc)
  else:
    print('ERROR: this script only supports "http" and "https" URLs')
    sys.exit(1)
  conn.putrequest('OPTIONS', path)
  conn.putheader('Host', netloc)
  conn.endheaders()
  resp = conn.getresponse()
  status, msg, server = (resp.status, resp.msg, resp.getheader('Server'))
  conn.close()

  # 1) Handle "OK" (200)
  # 2) Handle redirect requests (302), if requested resource
  #    resides temporarily under a different URL
  # 3) Handle authorization (401), if server requests for authorization
  #    ignore it, since we are interested in server version only
  if status != 200 and status != 302 and status != 401:
    print('ERROR: bad status response: %s %s' % (status, msg))
    sys.exit(1)
  if not server:
    # a missing Server: header. Bad, bad server! Go sit in the corner!
    print('WARNING: missing header')
  else:
    for part in server.split(' '):
      if part[:4] == 'SVN/':
        print(part[4:])
        break
    else:
      # the server might be configured to hide this information, or it
      # might not have mod_dav_svn loaded into it.
      print('NOTICE: version unknown')


if __name__ == '__main__':
  if len(sys.argv) != 2:
    print('USAGE: %s URL' % sys.argv[0])
    sys.exit(1)
  print_version(sys.argv[1])

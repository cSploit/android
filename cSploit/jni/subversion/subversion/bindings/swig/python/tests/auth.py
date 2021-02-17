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
import unittest, setup_path

from svn import core

class SubversionAuthTestCase(unittest.TestCase):
  """Test cases for the Subversion auth."""

  def test_open(self):
    baton = core.svn_auth_open([])
    self.assert_(baton is not None)

  def test_set_parameter(self):
    baton = core.svn_auth_open([])
    core.svn_auth_set_parameter(baton, "name", "somedata")
    core.svn_auth_set_parameter(baton, "name", None)
    core.svn_auth_set_parameter(baton, "name", 2)
    core.svn_auth_set_parameter(baton, "name",
                                core.svn_auth_ssl_server_cert_info_t())

  def test_invalid_cred_kind(self):
    baton = core.svn_auth_open([])
    self.assertRaises(core.SubversionException,
            lambda: core.svn_auth_first_credentials(
                "unknown", "somerealm", baton))

  def test_credentials_get_username(self):
    def myfunc(realm, maysave, pool):
      self.assertEquals("somerealm", realm)
      username_cred = core.svn_auth_cred_username_t()
      username_cred.username = "bar"
      username_cred.may_save = False
      return username_cred
    baton = core.svn_auth_open([core.svn_auth_get_username_prompt_provider(myfunc, 1)])
    creds = core.svn_auth_first_credentials(
                core.SVN_AUTH_CRED_USERNAME, "somerealm", baton)
    self.assert_(creds is not None)

  def test_credentials_get_simple(self):
    def myfunc(realm, username, may_save, pool):
      self.assertEquals("somerealm", realm)
      simple_cred = core.svn_auth_cred_simple_t()
      simple_cred.username = "mijnnaam"
      simple_cred.password = "geheim"
      simple_cred.may_save = False
      return simple_cred
    baton = core.svn_auth_open([core.svn_auth_get_simple_prompt_provider(myfunc, 1)])
    creds = core.svn_auth_first_credentials(
                core.SVN_AUTH_CRED_SIMPLE, "somerealm", baton)
    self.assert_(creds is not None)

  def test_credentials_get_ssl_client_cert(self):
    def myfunc(realm, may_save, pool):
      self.assertEquals("somerealm", realm)
      ssl_cred = core.svn_auth_cred_ssl_client_cert_t()
      ssl_cred.cert_file = "my-certs-file"
      ssl_cred.may_save = False
      return ssl_cred
    baton = core.svn_auth_open([core.svn_auth_get_ssl_client_cert_prompt_provider(myfunc, 1)])
    creds = core.svn_auth_first_credentials(
                core.SVN_AUTH_CRED_SSL_CLIENT_CERT, "somerealm", baton)
    self.assert_(creds is not None)

  def test_credentials_get_ssl_client_cert_pw(self):
    def myfunc(realm, may_save, pool):
      self.assertEquals("somerealm", realm)
      ssl_cred_pw = core.svn_auth_cred_ssl_client_cert_pw_t()
      ssl_cred_pw.password = "supergeheim"
      ssl_cred_pw.may_save = False
      return ssl_cred_pw
    baton = core.svn_auth_open([core.svn_auth_get_ssl_client_cert_pw_prompt_provider(myfunc, 1)])
    creds = core.svn_auth_first_credentials(
                core.SVN_AUTH_CRED_SSL_CLIENT_CERT_PW, "somerealm", baton)
    self.assert_(creds is not None)

  def test_credentials_get_ssl_server_trust(self):
    def myfunc(realm, failures, cert_info, may_save, pool):
      self.assertEquals("somerealm", realm)
      ssl_trust = core.svn_auth_cred_ssl_server_trust_t()
      ssl_trust.accepted_failures = 0
      ssl_trust.may_save = False
      return ssl_trust
    baton = core.svn_auth_open([core.svn_auth_get_ssl_server_trust_prompt_provider(myfunc)])
    core.svn_auth_set_parameter(baton, core.SVN_AUTH_PARAM_SSL_SERVER_FAILURES,
                                "2")
    cert_info = core.svn_auth_ssl_server_cert_info_t()
    core.svn_auth_set_parameter(baton, core.SVN_AUTH_PARAM_SSL_SERVER_CERT_INFO,
                cert_info)
    creds = core.svn_auth_first_credentials(
                core.SVN_AUTH_CRED_SSL_SERVER_TRUST, "somerealm", baton)
    self.assert_(creds is not None)

def suite():
    return unittest.defaultTestLoader.loadTestsFromTestCase(
      SubversionAuthTestCase)

if __name__ == '__main__':
    runner = unittest.TextTestRunner()
    runner.run(suite())

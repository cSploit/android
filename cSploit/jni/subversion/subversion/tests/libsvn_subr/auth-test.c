/*
 * auth-test.c -- test the auth functions
 *
 * ====================================================================
 *    Licensed to the Apache Software Foundation (ASF) under one
 *    or more contributor license agreements.  See the NOTICE file
 *    distributed with this work for additional information
 *    regarding copyright ownership.  The ASF licenses this file
 *    to you under the Apache License, Version 2.0 (the
 *    "License"); you may not use this file except in compliance
 *    with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing,
 *    software distributed under the License is distributed on an
 *    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *    KIND, either express or implied.  See the License for the
 *    specific language governing permissions and limitations
 *    under the License.
 * ====================================================================
 */

#include "svn_auth.h"
#include "svn_private_config.h"

#include "../svn_test.h"

static svn_error_t *
test_platform_specific_auth_providers(apr_pool_t *pool)
{
  apr_array_header_t *providers;
  svn_auth_provider_object_t *provider;
  int number_of_providers = 0;

  /* Test non-available auth provider */
  SVN_ERR(svn_auth_get_platform_specific_provider(&provider, "fake", "fake",
                                                  pool));

  if (provider)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_auth_get_platform_specific_provider('fake', 'fake') should " \
       "return NULL");

  /* Make sure you get appropriate number of providers when retrieving
     all auth providers */
  SVN_ERR(svn_auth_get_platform_specific_client_providers(&providers, NULL,
                                                          pool));

#ifdef SVN_HAVE_GNOME_KEYRING
  number_of_providers += 2;
#endif
#ifdef SVN_HAVE_KWALLET
  number_of_providers += 2;
#endif
#ifdef SVN_HAVE_KEYCHAIN_SERVICES
  number_of_providers += 2;
#endif
#if defined(WIN32) && !defined(__MINGW32__)
  number_of_providers += 2;
#endif
  if (providers->nelts != number_of_providers)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_auth_get_platform_specific_client_providers should return " \
       "an array of %d providers", number_of_providers);

  /* Test Keychain auth providers */
#ifdef SVN_HAVE_KEYCHAIN_SERVICES
  svn_auth_get_platform_specific_provider(&provider, "keychain",
                                          "simple", pool);

  if (!provider)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_auth_get_platform_specific_provider('keychain', 'simple') "
       "should not return NULL");

  svn_auth_get_platform_specific_provider(&provider, "keychain",
                                          "ssl_client_cert_pw", pool);

  if (!provider)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_auth_get_platform_specific_provider('keychain', " \
       "'ssl_client_cert_pw') should not return NULL");

  /* Make sure you do not get a Windows auth provider */
  svn_auth_get_platform_specific_provider(&provider, "windows",
                                          "simple", pool);

  if (provider)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_auth_get_platform_specific_provider('windows', 'simple') should " \
       "return NULL");
#endif

  /* Test Windows auth providers */
#if defined(WIN32) && !defined(__MINGW32__)
  svn_auth_get_platform_specific_provider(&provider, "windows",
                                          "simple", pool);

  if (!provider)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_auth_get_platform_specific_provider('windows', 'simple') "
       "should not return NULL");


  svn_auth_get_platform_specific_provider(&provider, "windows",
                                          "ssl_client_cert_pw", pool);

  if (!provider)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_auth_get_platform_specific_provider('windows', "
       "'ssl_client_cert_pw') should not return NULL");

  svn_auth_get_platform_specific_provider(&provider, "windows",
                                          "ssl_server_trust", pool);

  if (!provider)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_auth_get_platform_specific_provider('windows', "
       "'ssl_server_trust') should not return NULL");

  /* Make sure you do not get a Keychain auth provider */
  svn_auth_get_platform_specific_provider(&provider, "keychain",
                                          "simple", pool);

  if (provider)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_auth_get_platform_specific_provider('keychain', 'simple') should " \
       "return NULL");
#endif

  /* Test GNOME Keyring auth providers */
#ifdef SVN_HAVE_GNOME_KEYRING
  svn_auth_get_platform_specific_provider(&provider, "gnome_keyring",
                                          "simple", pool);

  if (!provider)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_auth_get_platform_specific_provider('gnome_keyring', 'simple') "
       "should not return NULL");

  svn_auth_get_platform_specific_provider(&provider, "gnome_keyring",
                                          "ssl_client_cert_pw", pool);

  if (!provider)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_auth_get_platform_specific_provider('gnome_keyring', " \
       "'ssl_client_cert_pw') should not return NULL");

  /* Make sure you do not get a Windows auth provider */
  svn_auth_get_platform_specific_provider(&provider, "windows",
                                          "simple", pool);

  if (provider)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_auth_get_platform_specific_provider('windows', 'simple') should " \
       "return NULL");
#endif

  /* Test KWallet auth providers */
#ifdef SVN_HAVE_KWALLET
  svn_auth_get_platform_specific_provider(&provider, "kwallet",
                                          "simple", pool);

  if (!provider)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_auth_get_platform_specific_provider('kwallet', 'simple') "
       "should not return NULL");

  svn_auth_get_platform_specific_provider(&provider, "kwallet",
                                          "ssl_client_cert_pw", pool);

  if (!provider)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_auth_get_platform_specific_provider('kwallet', " \
       "'ssl_client_cert_pw') should not return NULL");

  /* Make sure you do not get a Windows auth provider */
  svn_auth_get_platform_specific_provider(&provider, "windows",
                                          "simple", pool);

  if (provider)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_auth_get_platform_specific_provider('windows', 'simple') should " \
       "return NULL");
#endif

  return SVN_NO_ERROR;
}


/* The test table.  */

struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_PASS2(test_platform_specific_auth_providers,
                   "test retrieving platform-specific auth providers"),
    SVN_TEST_NULL
  };

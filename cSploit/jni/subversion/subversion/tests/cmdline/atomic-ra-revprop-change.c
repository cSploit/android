/*
 * atomic-ra-revprop-change.c :  wrapper around svn_ra_change_rev_prop2()
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

#include <stdlib.h>
#include <stdio.h>

#include <apr_pools.h>
#include <apr_general.h>

#include "svn_types.h"
#include "svn_pools.h"
#include "svn_dirent_uri.h"
#include "svn_ra.h"

#include "private/svn_skel.h"

#include "svn_private_config.h"


#define KEY_OLD_PROPVAL "old_value_p"
#define KEY_NEW_PROPVAL "value"

#define USAGE_MSG \
  "Usage: %s URL REVISION PROPNAME VALUES_SKEL HTTP_LIBRARY WANT_ERROR\n" \
  "\n" \
  "VALUES_SKEL is a proplist skel containing pseudo-properties '%s' \n" \
  "and '%s'.  A pseudo-property missing from the skel is interpreted \n" \
  "as unset.\n" \
  "\n" \
  "WANT_ERROR is 1 if the propchange is expected to fail due to the atomicity,"\
  "and 0 if it is expected to succeed.  If the expectation matches reality," \
  "the exit code shall be zero.\n"



/* implements svn_auth_simple_prompt_func_t */
static svn_error_t *
aborting_simple_prompt_func(svn_auth_cred_simple_t **cred,
                            void *baton,
                            const char *realm,
                            const char *username,
                            svn_boolean_t may_save,
                            apr_pool_t *pool)
{
  /* Oops, the jrandom:rayjandom we passed for SVN_AUTH_PARAM_DEFAULT_* failed,
     and the prompt provider has retried.
   */
  SVN_ERR_MALFUNCTION();
}

/* implements svn_auth_username_prompt_func_t */
static svn_error_t *
aborting_username_prompt_func(svn_auth_cred_username_t **cred,
                              void *baton,
                              const char *realm,
                              svn_boolean_t may_save,
                              apr_pool_t *pool)
{
  /* Oops, the jrandom:rayjandom we passed for SVN_AUTH_PARAM_DEFAULT_* failed,
     and the prompt provider has retried.
   */
  SVN_ERR_MALFUNCTION();
}

static svn_error_t *
construct_auth_baton(svn_auth_baton_t **auth_baton_p,
                     apr_pool_t *pool)
{
  apr_array_header_t *providers;
  svn_auth_provider_object_t *simple_provider;
  svn_auth_baton_t *auth_baton;

  /* A bit of dancing just to pass jrandom:rayjandom. */
  providers = apr_array_make(pool, 2, sizeof(svn_auth_provider_object_t *)),
  svn_auth_get_simple_prompt_provider(&simple_provider,
                                      aborting_simple_prompt_func, NULL,
                                      0, pool);
  APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = simple_provider;
  svn_auth_get_username_prompt_provider(&simple_provider,
                                        aborting_username_prompt_func, NULL,
                                        0, pool);
  APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = simple_provider;
  svn_auth_open(&auth_baton, providers, pool);
  svn_auth_set_parameter(auth_baton,
                         SVN_AUTH_PARAM_DEFAULT_USERNAME, "jrandom");
  svn_auth_set_parameter(auth_baton,
                         SVN_AUTH_PARAM_DEFAULT_PASSWORD, "rayjandom");

  *auth_baton_p = auth_baton;
  return SVN_NO_ERROR;
}

static svn_error_t *
construct_config(apr_hash_t **config_p,
                 const char *http_library,
                 apr_pool_t *pool)
{
  apr_hash_t *config;
  svn_config_t *servers;

  /* Populate SERVERS. */
  SVN_ERR(svn_config_create(&servers, FALSE,  pool));
  svn_config_set(servers, SVN_CONFIG_SECTION_GLOBAL,
                 SVN_CONFIG_OPTION_HTTP_LIBRARY, http_library);

  /* Populate CONFIG. */
  config = apr_hash_make(pool);
  apr_hash_set(config, SVN_CONFIG_CATEGORY_SERVERS,
               APR_HASH_KEY_STRING, servers);

  *config_p = config;
  return SVN_NO_ERROR;
}

static svn_error_t *
change_rev_prop(const char *url,
                svn_revnum_t revision,
                const char *propname,
                const svn_string_t *propval,
                const svn_string_t *old_value,
                const char *http_library,
                svn_boolean_t want_error,
                apr_pool_t *pool)
{
  svn_ra_callbacks2_t *callbacks;
  svn_ra_session_t *sess;
  apr_hash_t *config;
  svn_boolean_t capable;
  svn_error_t *err;

  SVN_ERR(svn_ra_create_callbacks(&callbacks, pool));
  SVN_ERR(construct_auth_baton(&callbacks->auth_baton, pool));
  SVN_ERR(construct_config(&config, http_library, pool));

  SVN_ERR(svn_ra_open4(&sess, NULL, url, NULL, callbacks, NULL /* baton */,
                       config, pool));

  SVN_ERR(svn_ra_has_capability(sess, &capable,
                                SVN_RA_CAPABILITY_ATOMIC_REVPROPS,
                                pool));
  if (capable)
    {
      err = svn_ra_change_rev_prop2(sess, revision, propname,
                                    &old_value, propval, pool);

      if (want_error && err
          && svn_error_find_cause(err, SVN_ERR_FS_PROP_BASEVALUE_MISMATCH))
        {
          /* Expectation was matched.  Get out. */
          svn_error_clear(err);
          return SVN_NO_ERROR;
        }
      else if (! want_error && ! err)
        /* Expectation was matched.  Get out. */
      	return SVN_NO_ERROR;
      else if (want_error && ! err)
        return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                                "An error was expected but not seen");
      else
      	/* A real (non-SVN_ERR_FS_PROP_BASEVALUE_MISMATCH) error. */
      	return svn_error_trace(err);
    }
  else
    /* Running under --server-minor-version? */
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "Server doesn't advertise "
                            "SVN_RA_CAPABILITY_ATOMIC_REVPROPS");
}

/* Parse SKEL_CSTR according to the description in USAGE_MSG. */
static svn_error_t *
extract_values_from_skel(svn_string_t **old_propval_p,
                         svn_string_t **propval_p,
                         const char *skel_cstr,
                         apr_pool_t *pool)
{
  apr_hash_t *proplist;
  svn_skel_t *skel;

  skel = svn_skel__parse(skel_cstr, strlen(skel_cstr), pool);
  SVN_ERR(svn_skel__parse_proplist(&proplist, skel, pool));
  *old_propval_p = apr_hash_get(proplist, KEY_OLD_PROPVAL, APR_HASH_KEY_STRING);
  *propval_p = apr_hash_get(proplist, KEY_NEW_PROPVAL, APR_HASH_KEY_STRING);

  return SVN_NO_ERROR;
}

int
main(int argc, const char *argv[])
{
  apr_pool_t *pool;
  int exit_code = EXIT_SUCCESS;
  svn_error_t *err;
  const char *url;
  svn_revnum_t revision;
  const char *propname;
  svn_string_t *propval;
  svn_string_t *old_propval;
  const char *http_library;
  char *digits_end = NULL;
  svn_boolean_t want_error;

  if (argc != 7)
    {
      fprintf(stderr, USAGE_MSG, argv[0], KEY_OLD_PROPVAL, KEY_NEW_PROPVAL);
      exit(1);
    }

  if (apr_initialize() != APR_SUCCESS)
    {
      fprintf(stderr, "apr_initialize() failed.\n");
      exit(1);
    }

  /* set up the global pool */
  pool = svn_pool_create(NULL);

  /* Parse argv. */
  url = svn_uri_canonicalize(argv[1], pool);
  revision = strtol(argv[2], &digits_end, 10);
  propname = argv[3];
  SVN_INT_ERR(extract_values_from_skel(&old_propval, &propval, argv[4], pool));
  http_library = argv[5];
  want_error = !strcmp(argv[6], "1");

  if ((! SVN_IS_VALID_REVNUM(revision)) || (! digits_end) || *digits_end)
    SVN_INT_ERR(svn_error_create(SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                                 _("Invalid revision number supplied")));

  /* Do something. */
  err = change_rev_prop(url, revision, propname, propval, old_propval,
                        http_library, want_error, pool);
  if (err)
    {
      svn_handle_error2(err, stderr, FALSE, "atomic-ra-revprop-change: ");
      svn_error_clear(err);
      exit_code = EXIT_FAILURE;
    }

  /* Clean up, and get outta here */
  svn_pool_destroy(pool);
  apr_terminate();

  return exit_code;
}

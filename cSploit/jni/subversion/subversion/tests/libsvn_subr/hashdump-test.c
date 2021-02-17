/*
 * hashdump-test.c :  testing the reading/writing of hashes
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



#include <stdio.h>       /* for sprintf() */
#include <stdlib.h>
#include <apr_pools.h>
#include <apr_hash.h>
#include <apr_file_io.h>

#include "../svn_test.h"

#include "svn_types.h"
#include "svn_string.h"
#include "svn_error.h"
#include "svn_hash.h"


/* Our own global variables */
apr_hash_t *proplist, *new_proplist;

const char *review =
"A forthright entrance, yet coquettish on the tongue, its deceptively\n"
"fruity exterior hides the warm mahagony undercurrent that is the\n"
"hallmark of Chateau Fraisant-Pitre.  Connoisseurs of the region will\n"
"be pleased to note the familiar, subtle hints of mulberries and\n"
"carburator fluid.  Its confident finish is marred only by a barely\n"
"detectable suggestion of rancid squid ink.";




static svn_error_t *
test1(apr_pool_t *pool)
{
  svn_error_t *result;
  svn_stringbuf_t *key;
  apr_file_t *f;

  /* Build a hash in memory, and fill it with test data. */
  proplist = apr_hash_make(pool);

  key = svn_stringbuf_create("color", pool);
  apr_hash_set(proplist, key->data, key->len,
               svn_string_create("red", pool));

  key = svn_stringbuf_create("wine review", pool);
  apr_hash_set(proplist, key->data, key->len,
               svn_string_create(review, pool));

  key = svn_stringbuf_create("price", pool);
  apr_hash_set(proplist, key->data, key->len,
               svn_string_create("US $6.50", pool));

  /* Test overwriting: same key both times, but different values. */
  key = svn_stringbuf_create("twice-used property name", pool);
  apr_hash_set(proplist, key->data, key->len,
               svn_string_create("This is the FIRST value.", pool));
  apr_hash_set(proplist, key->data, key->len,
               svn_string_create("This is the SECOND value.", pool));

  /* Dump the hash to a file. */
  apr_file_open(&f, "hashdump.out",
                (APR_WRITE | APR_CREATE),
                APR_OS_DEFAULT, pool);

  result = svn_hash_write2(proplist, svn_stream_from_aprfile(f, pool),
                           SVN_HASH_TERMINATOR, pool);

  apr_file_close(f);

  return result;
}




static svn_error_t *
test2(apr_pool_t *pool)
{
  svn_error_t *result;
  apr_file_t *f;

  new_proplist = apr_hash_make(pool);

  apr_file_open(&f, "hashdump.out", APR_READ, APR_OS_DEFAULT, pool);

  result = svn_hash_read2(new_proplist, svn_stream_from_aprfile(f, pool),
                          SVN_HASH_TERMINATOR, pool);

  apr_file_close(f);

  apr_file_remove("hashdump.out", pool);

  return result;
}



static svn_error_t *
test3(apr_pool_t *pool)
{
  apr_hash_index_t *this;
  int found_discrepancy = 0;

  /* Build a hash in global variable "proplist", then write to a file. */
  SVN_ERR(test1(pool));

  /* Read this file back into global variable "new_proplist" */
  SVN_ERR(test2(pool));

  /* Now let's make sure that proplist and new_proplist contain the
     same data. */

  /* Loop over our original hash */
  for (this = apr_hash_first(pool, proplist);
       this;
       this = apr_hash_next(this))
    {
      const void *key;
      apr_ssize_t keylen;
      void *val;
      svn_string_t *orig_str, *new_str;

      /* Get a key and val. */
      apr_hash_this(this, &key, &keylen, &val);
      orig_str = val;

      /* Look up the key in the new hash */
      new_str = apr_hash_get(new_proplist, key, keylen);

      /* Does the new hash contain the key at all? */
      if (new_str == NULL)
        found_discrepancy = 1;

      /* Do the two strings contain identical data? */
      else if (! svn_string_compare(orig_str, new_str))
        found_discrepancy = 1;
    }


  if (found_discrepancy)
    return svn_error_createf(SVN_ERR_TEST_FAILED, 0,
                             "found discrepancy reading back hash table");

  return SVN_NO_ERROR;
}




/*
   ====================================================================
   If you add a new test to this file, update this array.

*/

/* An array of all test functions */
struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_PASS2(test1,
                   "write a hash to a file"),
    SVN_TEST_PASS2(test2,
                   "read a file into a hash"),
    SVN_TEST_PASS2(test3,
                   "write hash out, read back in, compare"),
    SVN_TEST_NULL
  };

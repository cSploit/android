/*
 * subst_translate-test.c -- test the svn_subst_translate* functions
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

#include <locale.h>
#include <string.h>

#include "../svn_test.h"

#include "svn_types.h"
#include "svn_string.h"
#include "svn_subst.h"

#define ARRAY_LEN(ary) ((sizeof (ary)) / (sizeof ((ary)[0])))

/* Test inputs and expected output for svn_subst_translate_string2(). */
struct translate_string2_data_t
{
  const char *source;
  const char *expected_str;
  svn_boolean_t translated_to_utf8;
  svn_boolean_t translated_line_endings;
};

static svn_error_t *
test_svn_subst_translate_string2(apr_pool_t *pool)
{
  static const struct translate_string2_data_t tests[] =
    {
      /* No reencoding, no translation of line endings */
      { "abcdefz",
        "abcdefz", FALSE, FALSE },
      /* No reencoding, translation of line endings */
      { "     \r\n\r\n      \r\n        \r\n",
        "     \n\n      \n        \n", FALSE, TRUE },
      /* Reencoding, no translation of line endings */
      { "\xc7\xa9\xf4\xdf",
        "\xc3\x87\xc2\xa9\xc3\xb4\xc3\x9f", TRUE, FALSE },
      /* Reencoding, translation of line endings */
      { "\xc7\xa9\xf4\xdf\r\n",
        "\xc3\x87\xc2\xa9\xc3\xb4\xc3\x9f\n", TRUE, TRUE },
      { NULL, NULL, FALSE, FALSE }
    };
  const struct translate_string2_data_t *t;

  for (t = tests; t->source != NULL; t++)
  {
    svn_string_t *source_string = svn_string_create(t->source, pool);
    svn_string_t *new_value = NULL;
    svn_boolean_t translated_line_endings = ! t->translated_line_endings;
    svn_boolean_t translated_to_utf8;

    SVN_ERR(svn_subst_translate_string2(&new_value,
                                        NULL, &translated_line_endings,
                                        source_string, "ISO-8859-1", FALSE,
                                        pool, pool));
    SVN_TEST_STRING_ASSERT(new_value->data, t->expected_str);
    SVN_TEST_ASSERT(translated_line_endings == t->translated_line_endings);

    new_value = NULL;
    translated_to_utf8 = ! t->translated_to_utf8;
    translated_line_endings = ! t->translated_line_endings;
    SVN_ERR(svn_subst_translate_string2(&new_value, &translated_to_utf8,
                                        &translated_line_endings,
                                        source_string, "ISO-8859-1", FALSE,
                                        pool, pool));
    SVN_TEST_STRING_ASSERT(new_value->data, t->expected_str);
    SVN_TEST_ASSERT(translated_to_utf8 == t->translated_to_utf8);
    SVN_TEST_ASSERT(translated_line_endings == t->translated_line_endings);
  }

  /* Test that when REPAIR is FALSE, SVN_ERR_IO_INCONSISTENT_EOL is returned. */
    {
      svn_string_t *source_string = svn_string_create("  \r   \r\n  \n ", pool);
      svn_string_t *new_value = NULL;
      svn_error_t *err = svn_subst_translate_string2(&new_value, NULL, NULL,
                                                     source_string,
                                                     "ISO-8859-1", FALSE, pool,
                                                     pool);
      SVN_TEST_ASSERT(err != SVN_NO_ERROR);
      SVN_TEST_ASSERT(err->apr_err == SVN_ERR_IO_INCONSISTENT_EOL);
      svn_error_clear(err);
    }

  return SVN_NO_ERROR;
}

/* The body of the svn_subst_translate_string2_null_encoding test. It should
   only be called by test_svn_subst_translate_string2_null_encoding(), as this
   code assumes that the process locale has been changed to a locale that uses
   either CP-1252 or ISO-8859-1 for the default narrow string encoding. */
static svn_error_t *
test_svn_subst_translate_string2_null_encoding_helper(apr_pool_t *pool)
{
  {
    svn_string_t *new_value = NULL;
    svn_boolean_t translated_to_utf8 = FALSE;
    svn_boolean_t translated_line_endings = TRUE;
    /* 'Æ', which is 0xc6 in both ISO-8859-1 and Windows-1252 */
    svn_string_t *source_string = svn_string_create("\xc6", pool);

    SVN_ERR(svn_subst_translate_string2(&new_value, &translated_to_utf8,
                                        &translated_line_endings,
                                        source_string, NULL, FALSE,
                                        pool, pool));
    SVN_TEST_STRING_ASSERT(new_value->data, "\xc3\x86");
    SVN_TEST_ASSERT(translated_to_utf8 == TRUE);
    SVN_TEST_ASSERT(translated_line_endings == FALSE);
  }

  return SVN_NO_ERROR;
}

/* Test that when ENCODING is NULL, the system-default language encoding is used.

   This is a wrapper of test_svn_subst_translate_string2_null_encoding_helper()
   that ensures that the system-default character encoding is set to either
   CP-1252 or ISO-8859-1 before test_svn_subst_translate_string2_null_encoding_helper()
   is called, later restoring the original system-default character encoding. */
static svn_error_t *
test_svn_subst_translate_string2_null_encoding(apr_pool_t *pool)
{
  char orig_lc_all[256] = { '\0' };
  svn_error_t *test_result;

  const char *other_locales[] =
    {
      /* For Windows' msvcrt */
      "English.1252", "German.1252", "French.1252",

      /* For glibc */
      "en_US.ISO-8859-1", "en_GB.ISO-8859-1", "de_DE.ISO-8859-1",

      /* For OpenBSD's libc */
      "en_US.ISO8859-1",  "en_GB.ISO8859-1",  "de_DE.ISO8859-1",

      /* Must be last */
      NULL
    };
  const char **other_locale;

  strncpy(orig_lc_all, setlocale(LC_ALL, NULL), sizeof (orig_lc_all));

  for (other_locale = other_locales; *other_locale != NULL; ++other_locale)
  {
    if (setlocale(LC_ALL, *other_locale))
      break;
  }

  /* If the end of the list of other locales to try has been reached, then none
     of the tested locales are installed, so the test must be skipped. */
  if (*other_locale == NULL)
    return svn_error_createf(SVN_ERR_TEST_SKIPPED, NULL,
                             "Tried %d locales, but none are installed.",
                             (int) (ARRAY_LEN(other_locales) - 1));

  test_result = test_svn_subst_translate_string2_null_encoding_helper(pool);

  /* Restore the original locale for category LC_ALL. */
  SVN_TEST_ASSERT(setlocale(LC_ALL, orig_lc_all) != NULL);

  return test_result;
}

static svn_error_t *
test_repairing_svn_subst_translate_string2(apr_pool_t *pool)
{
    {
      svn_string_t *source_string = svn_string_create("  \r   \r\n  \n ", pool);
      svn_string_t *new_value = NULL;
      SVN_ERR(svn_subst_translate_string2(&new_value, NULL, NULL, source_string,
                                          "ISO-8859-1", TRUE, pool, pool));
      SVN_TEST_ASSERT(new_value != NULL);
      SVN_TEST_ASSERT(new_value->data != NULL);
      SVN_TEST_STRING_ASSERT(new_value->data, "  \n   \n  \n ");
    }

  return SVN_NO_ERROR;
}

/*static svn_error_t *
test_svn_subst_copy_and_translate4(apr_pool_t *pool)
{
  return SVN_NO_ERROR;
}

static svn_error_t *
test_svn_subst_stream_translated(apr_pool_t *pool)
{
  return SVN_NO_ERROR;
}*/

/* Test inputs and expected output for svn_subst_translate_cstring2(). */
struct translate_cstring2_data_t
{
  const char *source;
  const char *eol_str;
  svn_boolean_t repair;
  const char *expected_str;
};

static svn_error_t *
test_svn_subst_translate_cstring2(apr_pool_t *pool)
{
  static const struct translate_cstring2_data_t tests[] =
    {
      /* Test the unusual case where EOL_STR is an empty string. */
      { "   \r   \n\r\n     \n\n\n", "", TRUE,
        "           " },
      /* Test the unusual case where EOL_STR is not a standard EOL string. */
      { "   \r   \n\r\n     \n\n\n", "z", TRUE,
        "   z   zz     zzz" },
      { "    \n    \n ", "buzz", FALSE,
        "    buzz    buzz " },
      { "    \r\n    \n", "buzz", TRUE ,
        "    buzz    buzz"},
      { NULL, NULL, FALSE, NULL }
    };
  const struct translate_cstring2_data_t *t;

  for (t = tests; t->source != NULL; t++)
    {
      const char *result = NULL;
      SVN_ERR(svn_subst_translate_cstring2(t->source, &result, t->eol_str,
                                           t->repair, NULL, FALSE, pool));
      SVN_TEST_STRING_ASSERT(result, t->expected_str);
    }

  return SVN_NO_ERROR;
}

struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_PASS2(test_svn_subst_translate_string2,
                   "test svn_subst_translate_string2()"),
    SVN_TEST_PASS2(test_svn_subst_translate_string2_null_encoding,
                   "test svn_subst_translate_string2(encoding = NULL)"),
    SVN_TEST_PASS2(test_repairing_svn_subst_translate_string2,
                   "test repairing svn_subst_translate_string2()"),
    SVN_TEST_PASS2(test_svn_subst_translate_cstring2,
                   "test svn_subst_translate_cstring2()"),
    SVN_TEST_NULL
  };

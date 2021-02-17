/* tst_tld.c --- Self tests for tld_*().
 * Copyright (C) 2004-2013 Simon Josefsson
 *
 * This file is part of GNU Libidn.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <stringprep.h>
#include <idn-free.h>
#include <tld.h>

#include "utils.h"

struct tld
{
  const char *name;
  const char *tld;
  const char *example;
  size_t inlen;
  uint32_t in[100];
  int rc;
  size_t errpos;
};

static const struct tld tld[] = {
  {
   "Simple valid French domain",
   "fr",
   "example.fr",
   3,
   {0x00E0, 0x00E2, 0x00E6},
   TLD_SUCCESS},
  {
   "Simple invalid French domain",
   "fr",
   "ªexample.fr",
   5,
   {0x00E0, 0x00E2, 0x00E6, 0x4711, 0x0042},
   TLD_INVALID,
   3}
};

static const Tld_table _tld_fr_override =
  {
    "fr",
    "2.0",
    0,
    NULL
  };

/* Main array */
const Tld_table * my_tld_tables[] =
  {
    &_tld_fr_override,
    NULL
  };

void
doit (void)
{
  size_t i;
  const Tld_table *tldtable;
  char *out;
  size_t errpos;
  int rc;

  tldtable = tld_get_table (NULL, NULL);
  if (tldtable != NULL)
    fail ("FAIL: tld_get_table (NULL, NULL) != NULL\n");

  tldtable = tld_get_table ("nonexisting", NULL);
  if (tldtable != NULL)
    fail ("FAIL: tld_get_table (\"nonexisting\", NULL) != NULL\n");

  tldtable = tld_default_table (NULL, NULL);
  if (tldtable != NULL)
    fail ("FAIL: tld_default_table (NULL, NULL) != NULL\n");

  tldtable = tld_default_table (NULL, NULL);
  if (tldtable != NULL)
    fail ("FAIL: tld_default_table (NULL, NULL) != NULL\n");

  tldtable = tld_default_table ("fr", NULL);
  if (tldtable == NULL)
    fail ("FAIL: tld_default_table (\"fr\", NULL) == NULL\n");
  else if (tldtable->version == NULL)
    fail ("FAIL: tld_default_table (\"fr\", NULL)->version == NULL\n");
  else if (tldtable->name && strcmp (tldtable->version, "1.0") != 0)
    fail ("FAIL: tld_default_table (\"fr\", NULL)->version = \"%s\""
	  " != \"1.0\"\n", tldtable->version);

  tldtable = tld_default_table ("fr", my_tld_tables);
  if (tldtable == NULL)
    fail ("FAIL: tld_default_table (\"fr\", NULL) == NULL\n");
  else if (tldtable->version == NULL)
    fail ("FAIL: tld_default_table (\"fr\", NULL)->version == NULL\n");
  else if (tldtable->name && strcmp (tldtable->version, "2.0") != 0)
    fail ("FAIL: tld_default_table (\"fr\", NULL)->version = \"%s\""
	  " != \"2.0\"\n", tldtable->version);

  rc = tld_get_4 (NULL, 42, &out);
  if (rc != TLD_NODATA)
    fail ("FAIL: tld_get_4 (NULL, 42, &out) != TLD_NODATA: %d\n", rc);

  rc = tld_get_4 (tld[0].in, 0, &out);
  if (rc != TLD_NODATA)
    fail ("FAIL: tld_get_4 (NULL, 42, &out) != TLD_NODATA: %d\n", rc);

  rc = tld_check_4t (tld[0].in, tld[0].inlen, NULL, NULL);
  if (rc != TLD_SUCCESS)
    fail ("FAIL: tld_check_4t (tld=NULL) != TLD_SUCCESS: %d\n", rc);

  rc = tld_check_4z (NULL, NULL, NULL);
  if (rc != TLD_NODATA)
    fail ("FAIL: tld_check_4z (NULL) != TLD_NODATA: %d\n", rc);

  rc = tld_check_4z (tld[0].in, NULL, NULL);
  if (rc != TLD_SUCCESS)
    fail ("FAIL: tld_check_4z (in) != TLD_SUCCESS: %d\n", rc);

  rc = tld_check_8z (NULL, NULL, NULL);
  if (rc != TLD_NODATA)
    fail ("FAIL: tld_check_8z (NULL) != TLD_NODATA: %d\n", rc);

  rc = tld_check_lz (NULL, NULL, NULL);
  if (rc != TLD_NODATA)
    fail ("FAIL: tld_check_lz (NULL) != TLD_NODATA: %d\n", rc);

  rc = tld_check_lz ("foo", NULL, NULL);
  if (rc != TLD_SUCCESS)
    fail ("FAIL: tld_check_lz (\"foo\") != TLD_SUCCESS: %d\n", rc);

  {
    uint32_t in[] = { 0x73, 0x6a, 0x64, 0x2e, 0x73, 0x65, 0x00 };
    const char *p;

    rc = tld_get_4 (in, 6, &out);
    if (rc != TLD_SUCCESS)
      fail ("FAIL: tld_get_4 (in, 6, &out) != TLD_OK: %d\n", rc);
    if (strcmp ("se", out) != 0)
      fail ("FAIL: tld_get_4 (in, 6, &out): %s\n", out);
    idn_free (out);

    rc = tld_get_4z (in, &out);
    if (rc != TLD_SUCCESS)
      fail ("FAIL: tld_get_4z (in, &out) != TLD_OK: %d\n", rc);
    if (strcmp ("se", out) != 0)
      fail ("FAIL: tld_get_4z (in, &out): %s\n", out);
    idn_free (out);

    p = "sjd.se";
    rc = tld_get_z (p, &out);
    if (rc != TLD_SUCCESS)
      fail ("FAIL: tld_get_z (\"%s\", &out) != TLD_OK: %d\n", p, rc);
    if (strcmp ("se", out) != 0)
      fail ("FAIL: tld_get_z (\"%s\", &out): %s\n", p, out);
    idn_free (out);

    p = "foo.bar.baz.sjd.se";
    rc = tld_get_z (p, &out);
    if (rc != TLD_SUCCESS)
      fail ("FAIL: tld_get_z (\"%s\", &out) != TLD_OK: %d\n", p, rc);
    if (strcmp ("se", out) != 0)
      fail ("FAIL: tld_get_z (\"%s\", &out): %s\n", p, out);
    idn_free (out);

    p = ".sjd.se";
    rc = tld_get_z (p, &out);
    if (rc != TLD_SUCCESS)
      fail ("FAIL: tld_get_z (\"%s\", &out) != TLD_OK: %d\n", p, rc);
    if (strcmp ("se", out) != 0)
      fail ("FAIL: tld_get_z (\"%s\", &out): %s\n", p, out);
    idn_free (out);

    p = ".se";
    rc = tld_get_z (p, &out);
    if (rc != TLD_SUCCESS)
      fail ("FAIL: tld_get_z (\"%s\", &out) != TLD_OK: %d\n", p, rc);
    if (strcmp ("se", out) != 0)
      fail ("FAIL: tld_get_z (\"%s\", &out): %s\n", p, out);
    idn_free (out);
  }

  for (i = 0; i < sizeof (tld) / sizeof (tld[0]); i++)
    {
      if (debug)
	printf ("TLD entry %ld: %s\n", i, tld[i].name);

      if (debug)
	{
	  printf ("in:\n");
	  ucs4print (tld[i].in, tld[i].inlen);
	}

      tldtable = tld_default_table (tld[i].tld, NULL);
      if (tldtable == NULL)
	{
	  fail ("TLD entry %ld tld_get_table (%s)\n", i, tld[i].tld);
	  if (debug)
	    printf ("FATAL\n");
	  continue;
	}

      rc = tld_check_4t (tld[i].in, tld[i].inlen, &errpos, tldtable);
      if (rc != tld[i].rc)
	{
	  fail ("TLD entry %ld failed: %d\n", i, rc);
	  if (debug)
	    printf ("FATAL\n");
	  continue;
	}

      if (debug)
	printf ("returned %d expected %d\n", rc, tld[i].rc);

      if (rc != tld[i].rc)
	{
	  fail ("TLD entry %ld failed\n", i);
	  if (debug)
	    printf ("ERROR\n");
	}
      else if (rc == TLD_INVALID)
	{
	  if (debug)
	    printf ("returned errpos %ld expected errpos %ld\n",
		    errpos, tld[i].errpos);

	  if (tld[i].errpos != errpos)
	    {
	      fail ("TLD entry %ld failed because errpos %ld != %ld\n", i,
		    tld[i].errpos, errpos);
	      if (debug)
		printf ("ERROR\n");
	    }
	}
      else if (debug)
	printf ("OK\n");

      {
	rc = tld_check_8z (tld[i].example, &errpos, NULL);
	if (rc != tld[i].rc)
	  {
	    fail ("TLD entry %ld failed\n", i);
	    if (debug)
	      printf ("ERROR\n");
	  }
	if (debug)
	  printf ("TLD entry %ld tld_check_8z (%s)\n", i, tld[i].example);
      }
    }
}

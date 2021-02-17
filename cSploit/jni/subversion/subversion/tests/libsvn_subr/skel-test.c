/* skel-test.c --- tests for the skeleton functions
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
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include <apr.h>

#include "svn_pools.h"
#include "svn_string.h"
#include "private/svn_skel.h"

#include "../svn_test.h"
#include "../svn_test_fs.h"


/* Some utility functions.  */


/* A quick way to create error messages.  */
static svn_error_t *
fail(apr_pool_t *pool, const char *fmt, ...)
{
  va_list ap;
  char *msg;

  va_start(ap, fmt);
  msg = apr_pvsprintf(pool, fmt, ap);
  va_end(ap);

  return svn_error_create(SVN_ERR_TEST_FAILED, 0, msg);
}


/* Free everything from pool, and return an empty Subversion string.  */
static svn_stringbuf_t *
get_empty_string(apr_pool_t *pool)
{
  svn_pool_clear(pool);

  return svn_stringbuf_ncreate(0, 0, pool);
}

/* Parse a skeleton from a Subversion string.  */
static svn_skel_t *
parse_str(svn_stringbuf_t *str, apr_pool_t *pool)
{
  return svn_skel__parse(str->data, str->len, pool);
}


/* Parse a skeleton from a C string.  */
static svn_skel_t *
parse_cstr(const char *str, apr_pool_t *pool)
{
  return svn_skel__parse(str, strlen(str), pool);
}


enum char_type {
  type_nothing = 0,
  type_space = 1,
  type_digit = 2,
  type_paren = 3,
  type_name = 4
};

static int skel_char_map_initialized;
static enum char_type skel_char_map[256];

static void
init_char_types(void)
{
  int i;
  const char *c;

  if (skel_char_map_initialized)
    return;

  for (i = 0; i < 256; i++)
    skel_char_map[i] = type_nothing;

  for (i = '0'; i <= '9'; i++)
    skel_char_map[i] = type_digit;

  for (c = "\t\n\f\r "; *c; c++)
    skel_char_map[(unsigned char) *c] = type_space;

  for (c = "()[]"; *c; c++)
    skel_char_map[(unsigned char) *c] = type_paren;

  for (i = 'A'; i <= 'Z'; i++)
    skel_char_map[i] = type_name;
  for (i = 'a'; i <= 'z'; i++)
    skel_char_map[i] = type_name;

  skel_char_map_initialized = 1;
}

/* Return true iff BYTE is a whitespace byte.  */
static int
skel_is_space(char byte)
{
  init_char_types();

  return skel_char_map[(unsigned char) byte] == type_space;
}

#if 0
/* Return true iff BYTE is a digit byte.  */
static int
skel_is_digit(char byte)
{
  init_char_types();

  return skel_char_map[(unsigned char) byte] == type_digit;
}
#endif

/* Return true iff BYTE is a paren byte.  */
static int
skel_is_paren(char byte)
{
  init_char_types();

  return skel_char_map[(unsigned char) byte] == type_paren;
}

/* Return true iff BYTE is a name byte.  */
static int
skel_is_name(char byte)
{
  init_char_types();

  return skel_char_map[(unsigned char) byte] == type_name;
}


/* Check that SKEL is an atom, and its contents match LEN bytes of
   DATA. */
static int
check_atom(svn_skel_t *skel, const char *data, apr_size_t len)
{
  return (skel
          && skel->is_atom
          && skel->len == len
          && ! memcmp(skel->data, data, len));
}


/* Functions that generate/check interesting implicit-length atoms.  */


/* Append to STR an implicit-length atom consisting of the byte BYTE,
   terminated by the character TERM.  BYTE must be a name byte,
   and TERM must be a valid skel separator, or NULL.  */
static void
put_implicit_length_byte(svn_stringbuf_t *str, char byte, char term)
{
  if (! skel_is_name(byte))
    abort();
  if (term != '\0'
      && ! skel_is_space(term)
      && ! skel_is_paren(term))
    abort();
  svn_stringbuf_appendbyte(str, byte);
  if (term != '\0')
    svn_stringbuf_appendbyte(str, term);
}


/* Return true iff SKEL is the parsed form of the atom produced by
   calling put_implicit_length with BYTE.  */
static int
check_implicit_length_byte(svn_skel_t *skel, char byte)
{
  if (! skel_is_name(byte))
    abort();

  return check_atom(skel, &byte, 1);
}


/* Subroutine for the *_implicit_length_all_chars functions.  */
static char *
gen_implicit_length_all_chars(apr_size_t *len_p)
{
  apr_size_t pos;
  int i;
  static char name[256];

  /* Gotta start with a valid name character.  */
  pos = 0;
  name[pos++] = 'x';
  for (i = 0; i < 256; i++)
    if (! skel_is_space( (apr_byte_t)i)
        && ! skel_is_paren( (apr_byte_t)i))
      name[pos++] = i;

  *len_p = pos;
  return name;
}


/* Append to STR an implicit-length atom containing every character
   that's legal in such atoms, terminated by the valid atom terminator
   TERM.  */
static void
put_implicit_length_all_chars(svn_stringbuf_t *str, char term)
{
  apr_size_t len;
  char *name = gen_implicit_length_all_chars(&len);

  if (term != '\0'
      && ! skel_is_space(term)
      && ! skel_is_paren(term))
    abort();

  svn_stringbuf_appendbytes(str, name, len);
  if (term != '\0')
    svn_stringbuf_appendbyte(str, term);
}


/* Return true iff SKEL is the parsed form of the atom produced by
   calling put_implicit_length_all_chars.  */
static int
check_implicit_length_all_chars(svn_skel_t *skel)
{
  apr_size_t len;
  char *name = gen_implicit_length_all_chars(&len);

  return check_atom(skel, name, len);
}



/* Test parsing of implicit-length atoms.  */

static svn_error_t *
parse_implicit_length(apr_pool_t *pool)
{
  svn_stringbuf_t *str = get_empty_string(pool);
  svn_skel_t *skel;

  /* Try all valid single-byte atoms.  */
  {
    const char *c;
    int i;

    for (c = "\t\n\f\r ()[]"; *c; c++)
      for (i = 0; i < 256; i++)
        if (skel_is_name((apr_byte_t)i))
          {
            svn_stringbuf_setempty(str);
            put_implicit_length_byte(str, (apr_byte_t)i, *c);
            skel = parse_str(str, pool);
            if (! check_implicit_length_byte(skel,  (apr_byte_t)i))
              return fail(pool, "single-byte implicit-length skel 0x%02x"
                          " with terminator 0x%02x",
                          i, c);
          }
  }

  /* Try an atom that contains every character that's legal in an
     implicit-length atom.  */
  svn_stringbuf_setempty(str);
  put_implicit_length_all_chars(str, '\0');
  skel = parse_str(str, pool);
  if (! check_implicit_length_all_chars(skel))
    return fail(pool, "implicit-length skel containing all legal chars");

  return SVN_NO_ERROR;
}


/* Functions that generate/check interesting explicit-length atoms.  */


/* Append to STR the representation of the atom containing the LEN
   bytes at DATA, in explicit-length form, using SEP as the separator
   between the length and the data.  */
static void
put_explicit_length(svn_stringbuf_t *str,
                    const char *data,
                    apr_size_t len,
                    char sep)
{
  char *buf = malloc(len + 100);
  apr_size_t length_len;

  if (! skel_is_space(sep))
    abort();

  /* Generate the length and separator character.  */
  sprintf(buf, "%"APR_SIZE_T_FMT"%c", len, sep);
  length_len = strlen(buf);

  /* Copy in the real data (which may contain nulls).  */
  memcpy(buf + length_len, data, len);

  svn_stringbuf_appendbytes(str, buf, length_len + len);
  free(buf);
}


/* Return true iff SKEL is the parsed form of an atom generated by
   put_explicit_length.  */
static int
check_explicit_length(svn_skel_t *skel, const char *data, apr_size_t len)
{
  return check_atom(skel, data, len);
}


/* Test parsing of explicit-length atoms.  */

static svn_error_t *
try_explicit_length(const char *data,
                    apr_size_t len,
                    apr_size_t check_len,
                    apr_pool_t *pool)
{
  int i;
  svn_stringbuf_t *str = get_empty_string(pool);
  svn_skel_t *skel;

  /* Try it with every possible separator character.  */
  for (i = 0; i < 256; i++)
    if (skel_is_space( (apr_byte_t)i))
      {
        svn_stringbuf_setempty(str);
        put_explicit_length(str, data, len,  (apr_byte_t)i);
        skel = parse_str(str, pool);
        if (! check_explicit_length(skel, data, check_len))
          return fail(pool, "failed to reparse explicit-length atom");
      }

  return SVN_NO_ERROR;
}


static svn_error_t *
parse_explicit_length(apr_pool_t *pool)
{
  /* Try to parse the empty atom.  */
  SVN_ERR(try_explicit_length("", 0, 0, pool));

  /* Try to parse every one-character atom.  */
  {
    int i;

    for (i = 0; i < 256; i++)
      {
        char buf[1];

        buf[0] = i;
        SVN_ERR(try_explicit_length(buf, 1, 1, pool));
      }
  }

  /* Try to parse an atom containing every character.  */
  {
    int i;
    char data[256];

    for (i = 0; i < 256; i++)
      data[i] = i;

    SVN_ERR(try_explicit_length(data, 256, 256, pool));
  }

  return SVN_NO_ERROR;
}



/* Test parsing of invalid atoms. */

static struct invalid_atoms
{
  int type;
  apr_size_t len;
  const char *data;
} invalid_atoms[] = { { 1,  1, "(" },
                      { 1,  1, ")" },
                      { 1,  1, "[" },
                      { 1,  1, "]" },
                      { 1,  1, " " },
                      { 1, 13, "Hello, World!" },
                      { 1,  8, "1mplicit" },

                      { 2,  2, "1" },
                      { 2,  1, "12" },

                      { 7,  0, NULL } };

static svn_error_t *
parse_invalid_atoms(apr_pool_t *pool)
{
  struct invalid_atoms *ia = invalid_atoms;

  while (ia->type != 7)
    {
      if (ia->type == 1)
        {
          svn_skel_t *skel = parse_cstr(ia->data, pool);
          if (check_atom(skel, ia->data, ia->len))
            return fail(pool,
                        "failed to detect parsing error in '%s'", ia->data);
        }
      else
        {
          svn_error_t *err = try_explicit_length(ia->data, ia->len,
                                                 strlen(ia->data), pool);
          if (err == SVN_NO_ERROR)
            return fail(pool, "got wrong length in explicit-length atom");
          svn_error_clear(err);
        }

      ia++;
    }

  return SVN_NO_ERROR;
}



/* Functions that generate/check interesting lists.  */

/* Append the start of a list to STR, using LEN bytes of the
   whitespace character SPACE.  */
static void
put_list_start(svn_stringbuf_t *str, char space, int len)
{
  int i;

  if (len > 0 && ! skel_is_space(space))
    abort();

  svn_stringbuf_appendcstr(str, "(");
  for (i = 0; i < len; i++)
    svn_stringbuf_appendbyte(str, space);
}


/* Append the end of a list to STR, using LEN bytes of the
   whitespace character SPACE.  */
static void
put_list_end(svn_stringbuf_t *str, char space, int len)
{
  int i;

  if (len > 0 && ! skel_is_space(space))
    abort();

  for (i = 0; i < len; i++)
    svn_stringbuf_appendbyte(str, space);
  svn_stringbuf_appendcstr(str, ")");
}


/* Return true iff SKEL is a list of length DESIRED_LEN.  */
static int
check_list(svn_skel_t *skel, int desired_len)
{
  int len;
  svn_skel_t *child;

  if (! (skel
         && ! skel->is_atom))
    return 0;

  len = 0;
  for (child = skel->children; child; child = child->next)
    len++;

  return len == desired_len;
}



/* Parse lists.  */

static svn_error_t *
parse_list(apr_pool_t *pool)
{
  {
    /* Try lists of varying length.  */
    int list_len;

    for (list_len = 0;
         list_len < 30;
         list_len < 4 ? list_len++ : (list_len *= 3))
      {
        /* Try lists with different separators.  */
        int sep;

        for (sep = 0; sep < 256; sep++)
          if (skel_is_space( (apr_byte_t)sep))
            {
              /* Try lists with different numbers of separator
                 characters between the elements.  */
              int sep_count;

              for (sep_count = 0;
                   sep_count < 30;
                   sep_count < 4 ? sep_count++ : (sep_count *= 3))
                {
                  /* Try various single-byte implicit-length atoms
                     for elements.  */
                  int atom_byte;

                  for (atom_byte = 0; atom_byte < 256; atom_byte++)
                    if (skel_is_name( (apr_byte_t)atom_byte))
                      {
                        int i;
                        svn_stringbuf_t *str = get_empty_string(pool);
                        svn_skel_t *skel;
                        svn_skel_t *child;

                        put_list_start(str,  (apr_byte_t)sep, sep_count);
                        for (i = 0; i < list_len; i++)
                          put_implicit_length_byte(str,
                                                   (apr_byte_t)atom_byte,
                                                   (apr_byte_t)sep);
                        put_list_end(str,  (apr_byte_t)sep, sep_count);

                        skel = parse_str(str, pool);
                        if (! check_list(skel, list_len))
                          return fail(pool, "couldn't parse list");
                        for (child = skel->children;
                             child;
                             child = child->next)
                          if (! check_implicit_length_byte
                                 (child, (apr_byte_t)atom_byte))
                            return fail(pool,
                                        "list was reparsed incorrectly");
                      }

                  /* Try the atom containing every character that's
                     legal in an implicit-length atom as the element.  */
                  {
                    int i;
                    svn_stringbuf_t *str = get_empty_string(pool);
                    svn_skel_t *skel;
                    svn_skel_t *child;

                    put_list_start(str,  (apr_byte_t)sep, sep_count);
                    for (i = 0; i < list_len; i++)
                      put_implicit_length_all_chars(str,  (apr_byte_t)sep);
                    put_list_end(str,  (apr_byte_t)sep, sep_count);

                    skel = parse_str(str, pool);
                    if (! check_list(skel, list_len))
                      return fail(pool, "couldn't parse list");
                    for (child = skel->children;
                         child;
                         child = child->next)
                      if (! check_implicit_length_all_chars(child))
                        return fail(pool, "couldn't parse list");
                  }

                  /* Try using every one-byte explicit-length atom as
                     an element.  */
                  for (atom_byte = 0; atom_byte < 256; atom_byte++)
                    {
                      int i;
                      svn_stringbuf_t *str = get_empty_string(pool);
                      svn_skel_t *skel;
                      svn_skel_t *child;
                      char buf[1];

                      buf[0] = atom_byte;

                      put_list_start(str,  (apr_byte_t)sep, sep_count);
                      for (i = 0; i < list_len; i++)
                        put_explicit_length(str, buf, 1,  (apr_byte_t)sep);
                      put_list_end(str,  (apr_byte_t)sep, sep_count);

                      skel = parse_str(str, pool);
                      if (! check_list(skel, list_len))
                        return fail(pool, "couldn't parse list");
                      for (child = skel->children;
                           child;
                           child = child->next)
                        if (! check_explicit_length(child, buf, 1))
                          return fail(pool, "list was reparsed incorrectly");
                    }

                  /* Try using an atom containing every character as
                     an element.  */
                  {
                    int i;
                    svn_stringbuf_t *str = get_empty_string(pool);
                    svn_skel_t *skel;
                    svn_skel_t *child;
                    char data[256];

                    for (i = 0; i < 256; i++)
                      data[i] = i;

                    put_list_start(str,  (apr_byte_t)sep, sep_count);
                    for (i = 0; i < list_len; i++)
                      put_explicit_length(str, data, 256,  (apr_byte_t)sep);
                    put_list_end(str,  (apr_byte_t)sep, sep_count);

                    skel = parse_str(str, pool);
                    if (! check_list(skel, list_len))
                      return fail(pool, "couldn't parse list");
                    for (child = skel->children;
                         child;
                         child = child->next)
                      if (! check_explicit_length(child, data, 256))
                        return fail(pool, "list was re-parsed incorrectly");
                  }
                }
            }
      }
  }

  /* Try to parse some invalid lists.  */
  {
    int sep;

    /* Try different separators.  */
    for (sep = 0; sep < 256; sep++)
      if (skel_is_space( (apr_byte_t)sep))
        {
          /* Try lists with different numbers of separator
             characters between the elements.  */
          int sep_count;

          for (sep_count = 0;
               sep_count < 100;
               sep_count < 10 ? sep_count++ : (sep_count *= 3))
            {
              svn_stringbuf_t *str;

              /* A list with only a separator.  */
              str = get_empty_string(pool);
              put_list_start(str,  (apr_byte_t)sep, sep_count);
              if (parse_str(str, pool))
                return fail(pool, "failed to detect syntax error");

              /* A list with only a terminator.  */
              str = get_empty_string(pool);
              put_list_end(str,  (apr_byte_t)sep, sep_count);
              if (parse_str(str, pool))
                return fail(pool, "failed to detect syntax error");

              /* A list containing an invalid element.  */
              str = get_empty_string(pool);
              put_list_start(str,  (apr_byte_t)sep, sep_count);
              svn_stringbuf_appendcstr(str, "100 ");
              put_list_end(str,  (apr_byte_t)sep, sep_count);
              if (parse_str(str, pool))
                return fail(pool, "failed to detect invalid element");
            }
        }
  }

  return SVN_NO_ERROR;
}



/* Building interesting skels.  */

/* Build an atom skel containing the LEN bytes at DATA.  */
static svn_skel_t *
build_atom(apr_size_t len, char *data, apr_pool_t *pool)
{
  char *copy = apr_palloc(pool, len);
  svn_skel_t *skel = apr_palloc(pool, sizeof(*skel));

  memcpy(copy, data, len);
  skel->is_atom = 1;
  skel->len = len;
  skel->data = copy;

  return skel;
}

/* Build an empty list skel.  */
static svn_skel_t *
empty(apr_pool_t *pool)
{
  svn_skel_t *skel = apr_palloc(pool, sizeof(*skel));

  skel->is_atom = 0;
  skel->children = 0;

  return skel;
}

/* Stick ELEMENT at the beginning of the list skeleton LIST.  */
static void
add(svn_skel_t *element, svn_skel_t *list)
{
  element->next = list->children;
  list->children = element;
}


/* Return true if the contents of skel A are identical to those of
   skel B.  */
static int
skel_equal(svn_skel_t *a, svn_skel_t *b)
{
  if (a->is_atom != b->is_atom)
    return 0;

  if (a->is_atom)
    return (a->len == b->len
            && ! memcmp(a->data, b->data, a->len));
  else
    {
      svn_skel_t *a_child, *b_child;

      for (a_child = a->children, b_child = b->children;
           a_child && b_child;
           a_child = a_child->next, b_child = b_child->next)
        if (! skel_equal(a_child, b_child))
          return 0;

      if (a_child || b_child)
        return 0;
    }

  return 1;
}


/* Unparsing implicit-length atoms.  */

static svn_error_t *
unparse_implicit_length(apr_pool_t *pool)
{
  /* Unparse and check every single-byte implicit-length atom.  */
  {
    int byte;

    for (byte = 0; byte < 256; byte++)
      if (skel_is_name( (apr_byte_t)byte))
        {
          char buf =  (char)byte;
          svn_skel_t *skel = build_atom(1, &buf, pool);
          svn_stringbuf_t *str = svn_skel__unparse(skel, pool);

          if (! (str
                 && str->len == 1
                 && str->data[0] == (char)byte))
            return fail(pool, "incorrectly unparsed single-byte "
                        "implicit-length atom");
        }
  }

  return SVN_NO_ERROR;
}



/* Unparse some lists.  */

static svn_error_t *
unparse_list(apr_pool_t *pool)
{
  /* Make a list of all the single-byte implicit-length atoms.  */
  {
    svn_stringbuf_t *str;
    int byte;
    svn_skel_t *list = empty(pool);
    svn_skel_t *reparsed, *elt;

    for (byte = 0; byte < 256; byte++)
      if (skel_is_name( (apr_byte_t)byte))
        {
          char buf = byte;
          add(build_atom(1, &buf, pool), list);
        }

    /* Unparse that, parse it again, and see if we got the same thing
       back.  */
    str = svn_skel__unparse(list, pool);
    reparsed = svn_skel__parse(str->data, str->len, pool);

    if (! reparsed || reparsed->is_atom)
      return fail(pool, "result is syntactically misformed, or not a list");

    if (! skel_equal(list, reparsed))
      return fail(pool, "unparsing and parsing didn't preserve contents");

    elt = reparsed->children;
    for (byte = 255; byte >= 0; byte--)
      if (skel_is_name( (apr_byte_t)byte))
        {
          if (! (elt
                 && elt->is_atom
                 && elt->len == 1
                 && elt->data[0] == byte))
            return fail(pool, "bad element");

          /* Verify that each element's data falls within the string.  */
          if (elt->data < str->data
              || elt->data + elt->len > str->data + str->len)
            return fail(pool, "bad element");

          elt = elt->next;
        }

    /* We should have reached the end of the list at this point.  */
    if (elt)
      return fail(pool, "list too long");
  }

  /* Make a list of lists.  */
  {
    svn_stringbuf_t *str;
    svn_skel_t *top = empty(pool);
    svn_skel_t *reparsed;
    int i;

    for (i = 0; i < 10; i++)
      {
        svn_skel_t *middle = empty(pool);
        int j;

        for (j = 0; j < 10; j++)
          {
            char buf[10];
            apr_size_t k;
            int val;

            /* Make some interesting atom, containing lots of binary
               characters.  */
            val = i * 10 + j;
            for (k = 0; k < sizeof(buf); k++)
              {
                buf[k] = val;
                val += j;
              }

            add(build_atom(sizeof(buf), buf, pool), middle);
          }

        add(middle, top);
      }

    str = svn_skel__unparse(top, pool);
    reparsed = svn_skel__parse(str->data, str->len, pool);

    if (! skel_equal(top, reparsed))
      return fail(pool, "failed to reparse list of lists");
  }

  return SVN_NO_ERROR;
}


/* The test table.  */

struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_PASS2(parse_implicit_length,
                   "parse implicit-length atoms"),
    SVN_TEST_PASS2(parse_explicit_length,
                   "parse explicit-length atoms"),
    SVN_TEST_PASS2(parse_invalid_atoms,
                   "parse invalid atoms"),
    SVN_TEST_PASS2(parse_list,
                   "parse lists"),
    SVN_TEST_PASS2(unparse_implicit_length,
                   "unparse implicit-length atoms"),
    SVN_TEST_PASS2(unparse_list,
                   "unparse lists"),
    SVN_TEST_NULL
  };

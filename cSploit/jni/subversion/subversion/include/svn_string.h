/**
 * @copyright
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
 * @endcopyright
 *
 * @file svn_string.h
 * @brief Counted-length strings for Subversion, plus some C string goodies.
 *
 * There are two string datatypes: @c svn_string_t and @c svn_stringbuf_t.
 * The former is a simple pointer/length pair useful for passing around
 * strings (or arbitrary bytes) with a counted length. @c svn_stringbuf_t is
 * buffered to enable efficient appending of strings without an allocation
 * and copy for each append operation.
 *
 * @c svn_string_t contains a <tt>const char *</tt> for its data, so it is
 * most appropriate for constant data and for functions which expect constant,
 * counted data. Functions should generally use <tt>const @c svn_string_t
 * *</tt> as their parameter to indicate they are expecting a constant,
 * counted string.
 *
 * @c svn_stringbuf_t uses a plain <tt>char *</tt> for its data, so it is
 * most appropriate for modifiable data.
 *
 * <h3>Invariants</h3>
 *
 *   1. Null termination:
 *
 *      Both structures maintain a significant invariant:
 *
 *         <tt>s->data[s->len] == '\\0'</tt>
 *
 *      The functions defined within this header file will maintain
 *      the invariant (which does imply that memory is
 *      allocated/defined as @c len+1 bytes).  If code outside of the
 *      @c svn_string.h functions manually builds these structures,
 *      then they must enforce this invariant.
 *
 *      Note that an @c svn_string(buf)_t may contain binary data,
 *      which means that strlen(s->data) does not have to equal @c
 *      s->len. The NULL terminator is provided to make it easier to
 *      pass @c s->data to C string interfaces.
 *
 *
 *   2. Non-NULL input:
 *
 *      All the functions assume their input data is non-NULL,
 *      unless otherwise documented, and may seg fault if passed
 *      NULL.  The input data may *contain* null bytes, of course, just
 *      the data pointer itself must not be NULL.
 *
 * <h3>Memory allocation</h3>
 *
 *   All the functions make a deep copy of all input data, and never store
 *   a pointer to the original input data.
 */


#ifndef SVN_STRING_H
#define SVN_STRING_H

#include <apr.h>          /* for apr_size_t */
#include <apr_pools.h>    /* for apr_pool_t */
#include <apr_tables.h>   /* for apr_array_header_t */

#include "svn_types.h"    /* for svn_boolean_t, svn_error_t */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup svn_string String handling
 * @{
 */



/** A simple counted string. */
typedef struct svn_string_t
{
  const char *data; /**< pointer to the bytestring */
  apr_size_t len;   /**< length of bytestring */
} svn_string_t;

/** A buffered string, capable of appending without an allocation and copy
 * for each append. */
typedef struct svn_stringbuf_t
{
  /** a pool from which this string was originally allocated, and is not
   * necessarily specific to this string.  This is used only for allocating
   * more memory from when the string needs to grow.
   */
  apr_pool_t *pool;

  /** pointer to the bytestring */
  char *data;

  /** length of bytestring */
  apr_size_t len;

  /** total size of buffer allocated */
  apr_size_t blocksize;
} svn_stringbuf_t;


/**
 * @defgroup svn_string_svn_string_t svn_string_t functions
 * @{
 */

/** Create a new bytestring containing a C string (NULL-terminated). */
svn_string_t *
svn_string_create(const char *cstring, apr_pool_t *pool);

/** Create a new bytestring containing a generic string of bytes
 * (NOT NULL-terminated) */
svn_string_t *
svn_string_ncreate(const char *bytes, apr_size_t size, apr_pool_t *pool);

/** Create a new string with the contents of the given stringbuf */
svn_string_t *
svn_string_create_from_buf(const svn_stringbuf_t *strbuf, apr_pool_t *pool);

/** Create a new bytestring by formatting @a cstring (NULL-terminated)
 * from varargs, which are as appropriate for apr_psprintf().
 */
svn_string_t *
svn_string_createf(apr_pool_t *pool, const char *fmt, ...)
  __attribute__((format(printf, 2, 3)));

/** Create a new bytestring by formatting @a cstring (NULL-terminated)
 * from a @c va_list (see svn_stringbuf_createf()).
 */
svn_string_t *
svn_string_createv(apr_pool_t *pool, const char *fmt, va_list ap)
  __attribute__((format(printf, 2, 0)));

/** Return TRUE if a bytestring is empty (has length zero). */
svn_boolean_t
svn_string_isempty(const svn_string_t *str);

/** Return a duplicate of @a original_string. */
svn_string_t *
svn_string_dup(const svn_string_t *original_string, apr_pool_t *pool);

/** Return @c TRUE iff @a str1 and @a str2 have identical length and data. */
svn_boolean_t
svn_string_compare(const svn_string_t *str1, const svn_string_t *str2);

/** Return offset of first non-whitespace character in @a str, or return
 * @a str->len if none.
 */
apr_size_t
svn_string_first_non_whitespace(const svn_string_t *str);

/** Return position of last occurrence of @a ch in @a str, or return
 * @a str->len if no occurrence.
 */
apr_size_t
svn_string_find_char_backward(const svn_string_t *str, char ch);

/** @} */


/**
 * @defgroup svn_string_svn_stringbuf_t svn_stringbuf_t functions
 * @{
 */

/** Create a new bytestring containing a C string (NULL-terminated). */
svn_stringbuf_t *
svn_stringbuf_create(const char *cstring, apr_pool_t *pool);

/** Create a new bytestring containing a generic string of bytes
 * (NON-NULL-terminated)
 */
svn_stringbuf_t *
svn_stringbuf_ncreate(const char *bytes, apr_size_t size, apr_pool_t *pool);

/** Create a new empty bytestring with at least @a minimum_size bytes of
 * space available in the memory block.
 *
 * The allocated string buffer will be one byte larger than @a minimum_size
 * to account for a final '\\0'.
 *
 * @since New in 1.6.
 */
svn_stringbuf_t *
svn_stringbuf_create_ensure(apr_size_t minimum_size, apr_pool_t *pool);

/** Create a new stringbuf with the contents of the given string */
svn_stringbuf_t *
svn_stringbuf_create_from_string(const svn_string_t *str, apr_pool_t *pool);

/** Create a new bytestring by formatting @a cstring (NULL-terminated)
 * from varargs, which are as appropriate for apr_psprintf().
 */
svn_stringbuf_t *
svn_stringbuf_createf(apr_pool_t *pool, const char *fmt, ...)
  __attribute__((format(printf, 2, 3)));

/** Create a new bytestring by formatting @a cstring (NULL-terminated)
 * from a @c va_list (see svn_stringbuf_createf()).
 */
svn_stringbuf_t *
svn_stringbuf_createv(apr_pool_t *pool, const char *fmt, va_list ap)
  __attribute__((format(printf, 2, 0)));

/** Make sure that the string @a str has at least @a minimum_size bytes of
 * space available in the memory block.
 *
 * (@a minimum_size should include space for the terminating NULL character.)
 */
void
svn_stringbuf_ensure(svn_stringbuf_t *str, apr_size_t minimum_size);

/** Set a bytestring @a str to @a value */
void
svn_stringbuf_set(svn_stringbuf_t *str, const char *value);

/** Set a bytestring @a str to empty (0 length). */
void
svn_stringbuf_setempty(svn_stringbuf_t *str);

/** Return @c TRUE if a bytestring is empty (has length zero). */
svn_boolean_t
svn_stringbuf_isempty(const svn_stringbuf_t *str);

/** Chop @a nbytes bytes off end of @a str, but not more than @a str->len. */
void
svn_stringbuf_chop(svn_stringbuf_t *str, apr_size_t nbytes);

/** Fill bytestring @a str with character @a c. */
void
svn_stringbuf_fillchar(svn_stringbuf_t *str, unsigned char c);

/** Append a single character @a byte onto @a targetstr.
 * This is an optimized version of svn_stringbuf_appendbytes()
 * that is much faster to call and execute. Gains vary with the ABI.
 * The advantages extend beyond the actual call because the reduced
 * register pressure allows for more optimization within the caller.
 *
 * reallocs if necessary. @a targetstr is affected, nothing else is.
 * @since New in 1.7.
 */
void
svn_stringbuf_appendbyte(svn_stringbuf_t *targetstr,
                         char byte);

/** Append an array of bytes onto @a targetstr.
 *
 * reallocs if necessary. @a targetstr is affected, nothing else is.
 */
void
svn_stringbuf_appendbytes(svn_stringbuf_t *targetstr,
                          const char *bytes,
                          apr_size_t count);

/** Append an @c svn_stringbuf_t onto @a targetstr.
 *
 * reallocs if necessary. @a targetstr is affected, nothing else is.
 */
void
svn_stringbuf_appendstr(svn_stringbuf_t *targetstr,
                        const svn_stringbuf_t *appendstr);

/** Append a C string onto @a targetstr.
 *
 * reallocs if necessary. @a targetstr is affected, nothing else is.
 */
void
svn_stringbuf_appendcstr(svn_stringbuf_t *targetstr,
                         const char *cstr);

/** Return a duplicate of @a original_string. */
svn_stringbuf_t *
svn_stringbuf_dup(const svn_stringbuf_t *original_string, apr_pool_t *pool);

/** Return @c TRUE iff @a str1 and @a str2 have identical length and data. */
svn_boolean_t
svn_stringbuf_compare(const svn_stringbuf_t *str1,
                      const svn_stringbuf_t *str2);

/** Return offset of first non-whitespace character in @a str, or return
 * @a str->len if none.
 */
apr_size_t
svn_stringbuf_first_non_whitespace(const svn_stringbuf_t *str);

/** Strip whitespace from both sides of @a str (modified in place). */
void
svn_stringbuf_strip_whitespace(svn_stringbuf_t *str);

/** Return position of last occurrence of @a ch in @a str, or return
 * @a str->len if no occurrence.
 */
apr_size_t
svn_stringbuf_find_char_backward(const svn_stringbuf_t *str, char ch);

/** Return @c TRUE iff @a str1 and @a str2 have identical length and data. */
svn_boolean_t
svn_string_compare_stringbuf(const svn_string_t *str1,
                             const svn_stringbuf_t *str2);

/** @} */


/**
 * @defgroup svn_string_cstrings C string functions
 * @{
 */

/** Divide @a input into substrings along @a sep_chars boundaries, return an
 * array of copies of those substrings (plain const char*), allocating both
 * the array and the copies in @a pool.
 *
 * None of the elements added to the array contain any of the
 * characters in @a sep_chars, and none of the new elements are empty
 * (thus, it is possible that the returned array will have length
 * zero).
 *
 * If @a chop_whitespace is TRUE, then remove leading and trailing
 * whitespace from the returned strings.
 */
apr_array_header_t *
svn_cstring_split(const char *input,
                  const char *sep_chars,
                  svn_boolean_t chop_whitespace,
                  apr_pool_t *pool);

/** Like svn_cstring_split(), but append to existing @a array instead of
 * creating a new one.  Allocate the copied substrings in @a pool
 * (i.e., caller decides whether or not to pass @a array->pool as @a pool).
 */
void
svn_cstring_split_append(apr_array_header_t *array,
                         const char *input,
                         const char *sep_chars,
                         svn_boolean_t chop_whitespace,
                         apr_pool_t *pool);


/** Return @c TRUE iff @a str matches any of the elements of @a list, a list
 * of zero or more glob patterns.
 */
svn_boolean_t
svn_cstring_match_glob_list(const char *str, const apr_array_header_t *list);

/** Return @c TRUE iff @a str exactly matches any of the elements of @a list.
 *
 * @since new in 1.7
 */
svn_boolean_t
svn_cstring_match_list(const char *str, const apr_array_header_t *list);

/**
 * Return the number of line breaks in @a msg, allowing any kind of newline
 * termination (CR, LF, CRLF, or LFCR), even inconsistent.
 *
 * @since New in 1.2.
 */
int
svn_cstring_count_newlines(const char *msg);

/**
 * Return a cstring which is the concatenation of @a strings (an array
 * of char *) each followed by @a separator (that is, @a separator
 * will also end the resulting string).  Allocate the result in @a pool.
 * If @a strings is empty, then return the empty string.
 *
 * @since New in 1.2.
 */
char *
svn_cstring_join(const apr_array_header_t *strings,
                 const char *separator,
                 apr_pool_t *pool);

/**
 * Compare two strings @a atr1 and @a atr2, treating case-equivalent
 * unaccented Latin (ASCII subset) letters as equal.
 *
 * Returns in integer greater than, equal to, or less than 0,
 * according to whether @a str1 is considered greater than, equal to,
 * or less than @a str2.
 *
 * @since New in 1.5.
 */
int
svn_cstring_casecmp(const char *str1, const char *str2);

/**
 * Parse the C string @a str into a 64 bit number, and return it in @a *n.
 * Assume that the number is represented in base @a base.
 * Raise an error if conversion fails (e.g. due to overflow), or if the
 * converted number is smaller than @a minval or larger than @a maxval.
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_cstring_strtoi64(apr_int64_t *n, const char *str,
                     apr_int64_t minval, apr_int64_t maxval,
                     int base);

/**
 * Parse the C string @a str into a 64 bit number, and return it in @a *n.
 * Assume that the number is represented in base 10.
 * Raise an error if conversion fails (e.g. due to overflow).
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_cstring_atoi64(apr_int64_t *n, const char *str);

/**
 * Parse the C string @a str into a 32 bit number, and return it in @a *n.
 * Assume that the number is represented in base 10.
 * Raise an error if conversion fails (e.g. due to overflow).
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_cstring_atoi(int *n, const char *str);

/**
 * Parse the C string @a str into an unsigned 64 bit number, and return
 * it in @a *n. Assume that the number is represented in base @a base.
 * Raise an error if conversion fails (e.g. due to overflow), or if the
 * converted number is smaller than @a minval or larger than @a maxval.
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_cstring_strtoui64(apr_uint64_t *n, const char *str,
                      apr_uint64_t minval, apr_uint64_t maxval,
                      int base);

/**
 * Parse the C string @a str into an unsigned 64 bit number, and return
 * it in @a *n. Assume that the number is represented in base 10.
 * Raise an error if conversion fails (e.g. due to overflow).
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_cstring_atoui64(apr_uint64_t *n, const char *str);

/**
 * Parse the C string @a str into an unsigned 32 bit number, and return
 * it in @a *n. Assume that the number is represented in base 10.
 * Raise an error if conversion fails (e.g. due to overflow).
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_cstring_atoui(unsigned int *n, const char *str);

/** @} */

/** @} */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* SVN_STRING_H */

/*
 * svn_string.c:  routines to manipulate counted-length strings
 *                (svn_stringbuf_t and svn_string_t) and C strings.
 *
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



#include <apr.h>

#include <string.h>      /* for memcpy(), memcmp(), strlen() */
#include <apr_lib.h>     /* for apr_isspace() */
#include <apr_fnmatch.h>
#include "svn_string.h"  /* loads "svn_types.h" and <apr_pools.h> */
#include "svn_ctype.h"
#include "private/svn_dep_compat.h"
#include "private/svn_string_private.h"

#include "svn_private_config.h"


/* Our own realloc, since APR doesn't have one.  Note: this is a
   generic realloc for memory pools, *not* for strings. */
static void *
my__realloc(char *data, apr_size_t oldsize, apr_size_t request,
            apr_pool_t *pool)
{
  void *new_area;

  /* kff todo: it's a pity APR doesn't give us this -- sometimes it
     could realloc the block merely by extending in place, sparing us
     a memcpy(), but only the pool would know enough to be able to do
     this.  We should add a realloc() to APR if someone hasn't
     already. */

  /* malloc new area */
  new_area = apr_palloc(pool, request);

  /* copy data to new area */
  memcpy(new_area, data, oldsize);

  /* I'm NOT freeing old area here -- cuz we're using pools, ugh. */

  /* return new area */
  return new_area;
}

static APR_INLINE svn_boolean_t
string_compare(const char *str1,
               const char *str2,
               apr_size_t len1,
               apr_size_t len2)
{
  /* easy way out :)  */
  if (len1 != len2)
    return FALSE;

  /* now the strings must have identical lenghths */

  if ((memcmp(str1, str2, len1)) == 0)
    return TRUE;
  else
    return FALSE;
}

static APR_INLINE apr_size_t
string_first_non_whitespace(const char *str, apr_size_t len)
{
  apr_size_t i;

  for (i = 0; i < len; i++)
    {
      if (! svn_ctype_isspace(str[i]))
        return i;
    }

  /* if we get here, then the string must be entirely whitespace */
  return len;
}

static APR_INLINE apr_size_t
find_char_backward(const char *str, apr_size_t len, char ch)
{
  apr_size_t i = len;

  while (i != 0)
    {
      if (str[--i] == ch)
        return i;
    }

  /* char was not found, return len */
  return len;
}


/* svn_string functions */

/* Return a new svn_string_t object, allocated in POOL, initialized with
 * DATA and SIZE.  Do not copy the contents of DATA, just store the pointer.
 * SIZE is the length in bytes of DATA, excluding the required NUL
 * terminator. */
static svn_string_t *
create_string(const char *data, apr_size_t size,
              apr_pool_t *pool)
{
  svn_string_t *new_string;

  new_string = apr_palloc(pool, sizeof(*new_string));

  new_string->data = data;
  new_string->len = size;

  return new_string;
}

svn_string_t *
svn_string_ncreate(const char *bytes, apr_size_t size, apr_pool_t *pool)
{
  void *mem;
  char *data;
  svn_string_t *new_string;

  /* Allocate memory for svn_string_t and data in one chunk. */
  mem = apr_palloc(pool, sizeof(*new_string) + size + 1);
  data = (char*)mem + sizeof(*new_string);

  new_string = mem;
  new_string->data = data;
  new_string->len = size;

  memcpy(data, bytes, size);

  /* Null termination is the convention -- even if we suspect the data
     to be binary, it's not up to us to decide, it's the caller's
     call.  Heck, that's why they call it the caller! */
  data[size] = '\0';

  return new_string;
}


svn_string_t *
svn_string_create(const char *cstring, apr_pool_t *pool)
{
  return svn_string_ncreate(cstring, strlen(cstring), pool);
}


svn_string_t *
svn_string_create_from_buf(const svn_stringbuf_t *strbuf, apr_pool_t *pool)
{
  return svn_string_ncreate(strbuf->data, strbuf->len, pool);
}


svn_string_t *
svn_string_createv(apr_pool_t *pool, const char *fmt, va_list ap)
{
  char *data = apr_pvsprintf(pool, fmt, ap);

  /* wrap an svn_string_t around the new data */
  return create_string(data, strlen(data), pool);
}


svn_string_t *
svn_string_createf(apr_pool_t *pool, const char *fmt, ...)
{
  svn_string_t *str;

  va_list ap;
  va_start(ap, fmt);
  str = svn_string_createv(pool, fmt, ap);
  va_end(ap);

  return str;
}


svn_boolean_t
svn_string_isempty(const svn_string_t *str)
{
  return (str->len == 0);
}


svn_string_t *
svn_string_dup(const svn_string_t *original_string, apr_pool_t *pool)
{
  return (svn_string_ncreate(original_string->data,
                             original_string->len, pool));
}



svn_boolean_t
svn_string_compare(const svn_string_t *str1, const svn_string_t *str2)
{
  return
    string_compare(str1->data, str2->data, str1->len, str2->len);
}



apr_size_t
svn_string_first_non_whitespace(const svn_string_t *str)
{
  return
    string_first_non_whitespace(str->data, str->len);
}


apr_size_t
svn_string_find_char_backward(const svn_string_t *str, char ch)
{
  return find_char_backward(str->data, str->len, ch);
}

svn_string_t *
svn_stringbuf__morph_into_string(svn_stringbuf_t *strbuf)
{
  /* In debug mode, detect attempts to modify the original STRBUF object.
   */
#ifdef SVN_DEBUG
  strbuf->pool = NULL;
  strbuf->blocksize = strbuf->len;
#endif

  /* Both, svn_string_t and svn_stringbuf_t are public API structures
   * since a couple of releases now. Thus, we can rely on their precise
   * layout not to change.
   *
   * It just so happens that svn_string_t is structurally equivalent
   * to the (data, len) sub-set of svn_stringbuf_t. There is also no
   * difference in alignment and padding. So, we can just re-interpret
   * that part of STRBUF as a svn_string_t.
   *
   * However, since svn_string_t does not know about the blocksize
   * member in svn_stringbuf_t, any attempt to re-size the returned
   * svn_string_t might invalidate the STRBUF struct. Hence, we consider
   * the source STRBUF "consumed".
   *
   * Modifying the string character content is fine, though.
   */
  return (svn_string_t *)&strbuf->data;
}



/* svn_stringbuf functions */

static svn_stringbuf_t *
create_stringbuf(char *data, apr_size_t size, apr_size_t blocksize,
                 apr_pool_t *pool)
{
  svn_stringbuf_t *new_string;

  new_string = apr_palloc(pool, sizeof(*new_string));

  new_string->data = data;
  new_string->len = size;
  new_string->blocksize = blocksize;
  new_string->pool = pool;

  return new_string;
}

svn_stringbuf_t *
svn_stringbuf_create_ensure(apr_size_t blocksize, apr_pool_t *pool)
{
  void *mem;
  svn_stringbuf_t *new_string;

  /* apr_palloc will allocate multiples of 8.
   * Thus, we would waste some of that memory if we stuck to the
   * smaller size. Note that this is safe even if apr_palloc would
   * use some other aligment or none at all. */

  ++blocksize; /* + space for '\0' */
  blocksize = APR_ALIGN_DEFAULT(blocksize);

  /* Allocate memory for svn_string_t and data in one chunk. */
  mem = apr_palloc(pool, sizeof(*new_string) + blocksize);

  /* Initialize header and string */
  new_string = mem;

  new_string->data = (char*)mem + sizeof(*new_string);
  new_string->data[0] = '\0';
  new_string->len = 0;
  new_string->blocksize = blocksize;
  new_string->pool = pool;

  return new_string;
}

svn_stringbuf_t *
svn_stringbuf_ncreate(const char *bytes, apr_size_t size, apr_pool_t *pool)
{
  svn_stringbuf_t *strbuf = svn_stringbuf_create_ensure(size, pool);
  memcpy(strbuf->data, bytes, size);

  /* Null termination is the convention -- even if we suspect the data
     to be binary, it's not up to us to decide, it's the caller's
     call.  Heck, that's why they call it the caller! */
  strbuf->data[size] = '\0';
  strbuf->len = size;

  return strbuf;
}


svn_stringbuf_t *
svn_stringbuf_create(const char *cstring, apr_pool_t *pool)
{
  return svn_stringbuf_ncreate(cstring, strlen(cstring), pool);
}


svn_stringbuf_t *
svn_stringbuf_create_from_string(const svn_string_t *str, apr_pool_t *pool)
{
  return svn_stringbuf_ncreate(str->data, str->len, pool);
}


svn_stringbuf_t *
svn_stringbuf_createv(apr_pool_t *pool, const char *fmt, va_list ap)
{
  char *data = apr_pvsprintf(pool, fmt, ap);
  apr_size_t size = strlen(data);

  /* wrap an svn_stringbuf_t around the new data */
  return create_stringbuf(data, size, size + 1, pool);
}


svn_stringbuf_t *
svn_stringbuf_createf(apr_pool_t *pool, const char *fmt, ...)
{
  svn_stringbuf_t *str;

  va_list ap;
  va_start(ap, fmt);
  str = svn_stringbuf_createv(pool, fmt, ap);
  va_end(ap);

  return str;
}


void
svn_stringbuf_fillchar(svn_stringbuf_t *str, unsigned char c)
{
  memset(str->data, c, str->len);
}


void
svn_stringbuf_set(svn_stringbuf_t *str, const char *value)
{
  apr_size_t amt = strlen(value);

  svn_stringbuf_ensure(str, amt + 1);
  memcpy(str->data, value, amt + 1);
  str->len = amt;
}

void
svn_stringbuf_setempty(svn_stringbuf_t *str)
{
  if (str->len > 0)
    str->data[0] = '\0';

  str->len = 0;
}


void
svn_stringbuf_chop(svn_stringbuf_t *str, apr_size_t nbytes)
{
  if (nbytes > str->len)
    str->len = 0;
  else
    str->len -= nbytes;

  str->data[str->len] = '\0';
}


svn_boolean_t
svn_stringbuf_isempty(const svn_stringbuf_t *str)
{
  return (str->len == 0);
}


void
svn_stringbuf_ensure(svn_stringbuf_t *str, apr_size_t minimum_size)
{
  /* Keep doubling capacity until have enough. */
  if (str->blocksize < minimum_size)
    {
      if (str->blocksize == 0)
        /* APR will increase odd allocation sizes to the next
         * multiple for 8, for instance. Take advantage of that
         * knowledge and allow for the extra size to be used. */
        str->blocksize = APR_ALIGN_DEFAULT(minimum_size);
      else
        while (str->blocksize < minimum_size)
          {
            /* str->blocksize is aligned;
             * doubling it should keep it aligned */
            apr_size_t prev_size = str->blocksize;
            str->blocksize *= 2;

            /* check for apr_size_t overflow */
            if (prev_size > str->blocksize)
              {
                str->blocksize = minimum_size;
                break;
              }
          }

      str->data = (char *) my__realloc(str->data,
                                       str->len + 1,
                                       /* We need to maintain (and thus copy)
                                          the trailing nul */
                                       str->blocksize,
                                       str->pool);
    }
}


/* WARNING - Optimized code ahead!
 * This function has been hand-tuned for performance. Please read
 * the comments below before modifying the code.
 */
void
svn_stringbuf_appendbyte(svn_stringbuf_t *str, char byte)
{
  char *dest;
  apr_size_t old_len = str->len;

  /* In most cases, there will be pre-allocated memory left
   * to just write the new byte at the end of the used section
   * and terminate the string properly.
   */
  if (str->blocksize > old_len + 1)
    {
      /* The following read does not depend this write, so we
       * can issue the write first to minimize register pressure:
       * The value of old_len+1 is no longer needed; on most processors,
       * dest[old_len+1] will be calculated implicitly as part of
       * the addressing scheme.
       */
      str->len = old_len+1;

      /* Since the compiler cannot be sure that *src->data and *src
       * don't overlap, we read src->data *once* before writing
       * to *src->data. Replacing dest with str->data would force
       * the compiler to read it again after the first byte.
       */
      dest = str->data;

      /* If not already available in a register as per ABI, load
       * "byte" into the register (e.g. the one freed from old_len+1),
       * then write it to the string buffer and terminate it properly.
       *
       * Including the "byte" fetch, all operations so far could be
       * issued at once and be scheduled at the CPU's descression.
       * Most likely, no-one will soon depend on the data that will be
       * written in this function. So, no stalls there, either.
       */
      dest[old_len] = byte;
      dest[old_len+1] = '\0';
    }
  else
    {
      /* we need to re-allocate the string buffer
       * -> let the more generic implementation take care of that part
       */

      /* Depending on the ABI, "byte" is a register value. If we were
       * to take its address directly, the compiler might decide to
       * put in on the stack *unconditionally*, even if that would
       * only be necessary for this block.
       */
      char b = byte;
      svn_stringbuf_appendbytes(str, &b, 1);
    }
}


void
svn_stringbuf_appendbytes(svn_stringbuf_t *str, const char *bytes,
                          apr_size_t count)
{
  apr_size_t total_len;
  void *start_address;

  total_len = str->len + count;  /* total size needed */

  /* +1 for null terminator. */
  svn_stringbuf_ensure(str, (total_len + 1));

  /* get address 1 byte beyond end of original bytestring */
  start_address = (str->data + str->len);

  memcpy(start_address, bytes, count);
  str->len = total_len;

  str->data[str->len] = '\0';  /* We don't know if this is binary
                                  data or not, but convention is
                                  to null-terminate. */
}


void
svn_stringbuf_appendstr(svn_stringbuf_t *targetstr,
                        const svn_stringbuf_t *appendstr)
{
  svn_stringbuf_appendbytes(targetstr, appendstr->data, appendstr->len);
}


void
svn_stringbuf_appendcstr(svn_stringbuf_t *targetstr, const char *cstr)
{
  svn_stringbuf_appendbytes(targetstr, cstr, strlen(cstr));
}


svn_stringbuf_t *
svn_stringbuf_dup(const svn_stringbuf_t *original_string, apr_pool_t *pool)
{
  return (svn_stringbuf_ncreate(original_string->data,
                                original_string->len, pool));
}



svn_boolean_t
svn_stringbuf_compare(const svn_stringbuf_t *str1,
                      const svn_stringbuf_t *str2)
{
  return string_compare(str1->data, str2->data, str1->len, str2->len);
}



apr_size_t
svn_stringbuf_first_non_whitespace(const svn_stringbuf_t *str)
{
  return string_first_non_whitespace(str->data, str->len);
}


void
svn_stringbuf_strip_whitespace(svn_stringbuf_t *str)
{
  /* Find first non-whitespace character */
  apr_size_t offset = svn_stringbuf_first_non_whitespace(str);

  /* Go ahead!  Waste some RAM, we've got pools! :)  */
  str->data += offset;
  str->len -= offset;
  str->blocksize -= offset;

  /* Now that we've trimmed the front, trim the end, wasting more RAM. */
  while ((str->len > 0) && svn_ctype_isspace(str->data[str->len - 1]))
    str->len--;
  str->data[str->len] = '\0';
}


apr_size_t
svn_stringbuf_find_char_backward(const svn_stringbuf_t *str, char ch)
{
  return find_char_backward(str->data, str->len, ch);
}


svn_boolean_t
svn_string_compare_stringbuf(const svn_string_t *str1,
                             const svn_stringbuf_t *str2)
{
  return string_compare(str1->data, str2->data, str1->len, str2->len);
}



/*** C string stuff. ***/

void
svn_cstring_split_append(apr_array_header_t *array,
                         const char *input,
                         const char *sep_chars,
                         svn_boolean_t chop_whitespace,
                         apr_pool_t *pool)
{
  char *last;
  char *pats;
  char *p;

  pats = apr_pstrdup(pool, input);  /* strtok wants non-const data */
  p = apr_strtok(pats, sep_chars, &last);

  while (p)
    {
      if (chop_whitespace)
        {
          while (svn_ctype_isspace(*p))
            p++;

          {
            char *e = p + (strlen(p) - 1);
            while ((e >= p) && (svn_ctype_isspace(*e)))
              e--;
            *(++e) = '\0';
          }
        }

      if (p[0] != '\0')
        APR_ARRAY_PUSH(array, const char *) = p;

      p = apr_strtok(NULL, sep_chars, &last);
    }

  return;
}


apr_array_header_t *
svn_cstring_split(const char *input,
                  const char *sep_chars,
                  svn_boolean_t chop_whitespace,
                  apr_pool_t *pool)
{
  apr_array_header_t *a = apr_array_make(pool, 5, sizeof(input));
  svn_cstring_split_append(a, input, sep_chars, chop_whitespace, pool);
  return a;
}


svn_boolean_t svn_cstring_match_glob_list(const char *str,
                                          const apr_array_header_t *list)
{
  int i;

  for (i = 0; i < list->nelts; i++)
    {
      const char *this_pattern = APR_ARRAY_IDX(list, i, char *);

      if (apr_fnmatch(this_pattern, str, 0) == APR_SUCCESS)
        return TRUE;
    }

  return FALSE;
}

svn_boolean_t
svn_cstring_match_list(const char *str, const apr_array_header_t *list)
{
  int i;

  for (i = 0; i < list->nelts; i++)
    {
      const char *this_str = APR_ARRAY_IDX(list, i, char *);

      if (strcmp(this_str, str) == 0)
        return TRUE;
    }

  return FALSE;
}

int svn_cstring_count_newlines(const char *msg)
{
  int count = 0;
  const char *p;

  for (p = msg; *p; p++)
    {
      if (*p == '\n')
        {
          count++;
          if (*(p + 1) == '\r')
            p++;
        }
      else if (*p == '\r')
        {
          count++;
          if (*(p + 1) == '\n')
            p++;
        }
    }

  return count;
}

char *
svn_cstring_join(const apr_array_header_t *strings,
                 const char *separator,
                 apr_pool_t *pool)
{
  svn_stringbuf_t *new_str = svn_stringbuf_create("", pool);
  int sep_len = strlen(separator);
  int i;

  for (i = 0; i < strings->nelts; i++)
    {
      const char *string = APR_ARRAY_IDX(strings, i, const char *);
      svn_stringbuf_appendbytes(new_str, string, strlen(string));
      svn_stringbuf_appendbytes(new_str, separator, sep_len);
    }
  return new_str->data;
}

int
svn_cstring_casecmp(const char *str1, const char *str2)
{
  for (;;)
    {
      const int a = *str1++;
      const int b = *str2++;
      const int cmp = svn_ctype_casecmp(a, b);
      if (cmp || !a || !b)
        return cmp;
    }
}

svn_error_t *
svn_cstring_strtoui64(apr_uint64_t *n, const char *str,
                      apr_uint64_t minval, apr_uint64_t maxval,
                      int base)
{
  apr_int64_t val;
  char *endptr;

  /* We assume errno is thread-safe. */
  errno = 0; /* APR-0.9 doesn't always set errno */

  /* ### We're throwing away half the number range here.
   * ### APR needs a apr_strtoui64() function. */
  val = apr_strtoi64(str, &endptr, base);
  if (errno == EINVAL || endptr == str || str[0] == '\0' || *endptr != '\0')
    return svn_error_createf(SVN_ERR_INCORRECT_PARAMS, NULL,
                             _("Could not convert '%s' into a number"),
                             str);
  if ((errno == ERANGE && (val == APR_INT64_MIN || val == APR_INT64_MAX)) ||
      val < 0 || (apr_uint64_t)val < minval || (apr_uint64_t)val > maxval)
    /* ### Mark this for translation when gettext doesn't choke on macros. */
    return svn_error_createf(SVN_ERR_INCORRECT_PARAMS, NULL,
                             "Number '%s' is out of range "
                             "'[%" APR_UINT64_T_FMT ", %" APR_UINT64_T_FMT "]'",
                             str, minval, maxval);
  *n = val;
  return SVN_NO_ERROR;
}

svn_error_t *
svn_cstring_atoui64(apr_uint64_t *n, const char *str)
{
  return svn_error_trace(svn_cstring_strtoui64(n, str, 0,
                                               APR_UINT64_MAX, 10));
}

svn_error_t *
svn_cstring_atoui(unsigned int *n, const char *str)
{
  apr_uint64_t val;

  SVN_ERR(svn_cstring_strtoui64(&val, str, 0, APR_UINT32_MAX, 10));
  *n = (unsigned int)val;
  return SVN_NO_ERROR;
}

svn_error_t *
svn_cstring_strtoi64(apr_int64_t *n, const char *str,
                     apr_int64_t minval, apr_int64_t maxval,
                     int base)
{
  apr_int64_t val;
  char *endptr;

  /* We assume errno is thread-safe. */
  errno = 0; /* APR-0.9 doesn't always set errno */

  val = apr_strtoi64(str, &endptr, base);
  if (errno == EINVAL || endptr == str || str[0] == '\0' || *endptr != '\0')
    return svn_error_createf(SVN_ERR_INCORRECT_PARAMS, NULL,
                             _("Could not convert '%s' into a number"),
                             str);
  if ((errno == ERANGE && (val == APR_INT64_MIN || val == APR_INT64_MAX)) ||
      val < minval || val > maxval)
    /* ### Mark this for translation when gettext doesn't choke on macros. */
    return svn_error_createf(SVN_ERR_INCORRECT_PARAMS, NULL,
                             "Number '%s' is out of range "
                             "'[%" APR_INT64_T_FMT ", %" APR_INT64_T_FMT "]'",
                             str, minval, maxval);
  *n = val;
  return SVN_NO_ERROR;
}

svn_error_t *
svn_cstring_atoi64(apr_int64_t *n, const char *str)
{
  return svn_error_trace(svn_cstring_strtoi64(n, str, APR_INT64_MIN,
                                              APR_INT64_MAX, 10));
}

svn_error_t *
svn_cstring_atoi(int *n, const char *str)
{
  apr_int64_t val;

  SVN_ERR(svn_cstring_strtoi64(&val, str, APR_INT32_MIN, APR_INT32_MAX, 10));
  *n = (int)val;
  return SVN_NO_ERROR;
}


apr_status_t
svn__strtoff(apr_off_t *offset, const char *buf, char **end, int base)
{
#if !APR_VERSION_AT_LEAST(1,0,0)
  errno = 0;
  *offset = strtol(buf, end, base);
  return APR_FROM_OS_ERROR(errno);
#else
  return apr_strtoff(offset, buf, end, base);
#endif
}

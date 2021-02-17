/*
 * util.c :  routines for doing diffs
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
#include <apr_general.h>

#include "svn_error.h"
#include "svn_diff.h"
#include "svn_types.h"
#include "svn_ctype.h"
#include "svn_version.h"

#include "diff.h"

svn_boolean_t
svn_diff_contains_conflicts(svn_diff_t *diff)
{
  while (diff != NULL)
    {
      if (diff->type == svn_diff__type_conflict)
        {
          return TRUE;
        }

      diff = diff->next;
    }

  return FALSE;
}

svn_boolean_t
svn_diff_contains_diffs(svn_diff_t *diff)
{
  while (diff != NULL)
    {
      if (diff->type != svn_diff__type_common)
        {
          return TRUE;
        }

      diff = diff->next;
    }

  return FALSE;
}

svn_error_t *
svn_diff_output(svn_diff_t *diff,
                void *output_baton,
                const svn_diff_output_fns_t *vtable)
{
  svn_error_t *(*output_fn)(void *,
                            apr_off_t, apr_off_t,
                            apr_off_t, apr_off_t,
                            apr_off_t, apr_off_t);

  while (diff != NULL)
    {
      switch (diff->type)
        {
        case svn_diff__type_common:
          output_fn = vtable->output_common;
          break;

        case svn_diff__type_diff_common:
          output_fn = vtable->output_diff_common;
          break;

        case svn_diff__type_diff_modified:
          output_fn = vtable->output_diff_modified;
          break;

        case svn_diff__type_diff_latest:
          output_fn = vtable->output_diff_latest;
          break;

        case svn_diff__type_conflict:
          output_fn = NULL;
          if (vtable->output_conflict != NULL)
            {
              SVN_ERR(vtable->output_conflict(output_baton,
                               diff->original_start, diff->original_length,
                               diff->modified_start, diff->modified_length,
                               diff->latest_start, diff->latest_length,
                               diff->resolved_diff));
            }
          break;

        default:
          output_fn = NULL;
          break;
        }

      if (output_fn != NULL)
        {
          SVN_ERR(output_fn(output_baton,
                            diff->original_start, diff->original_length,
                            diff->modified_start, diff->modified_length,
                            diff->latest_start, diff->latest_length));
        }

      diff = diff->next;
    }

  return SVN_NO_ERROR;
}


void
svn_diff__normalize_buffer(char **tgt,
                           apr_off_t *lengthp,
                           svn_diff__normalize_state_t *statep,
                           const char *buf,
                           const svn_diff_file_options_t *opts)
{
  /* Variables for looping through BUF */
  const char *curp, *endp;

  /* Variable to record normalizing state */
  svn_diff__normalize_state_t state = *statep;

  /* Variables to track what needs copying into the target buffer */
  const char *start = buf;
  apr_size_t include_len = 0;
  svn_boolean_t last_skipped = FALSE; /* makes sure we set 'start' */

  /* Variable to record the state of the target buffer */
  char *tgt_newend = *tgt;

  /* If this is a noop, then just get out of here. */
  if (! opts->ignore_space && ! opts->ignore_eol_style)
    {
      *tgt = (char *)buf;
      return;
    }


  /* It only took me forever to get this routine right,
     so here my thoughts go:

    Below, we loop through the data, doing 2 things:

     - Normalizing
     - Copying other data

     The routine tries its hardest *not* to copy data, but instead
     returning a pointer into already normalized existing data.

     To this end, a block 'other data' shouldn't be copied when found,
     but only as soon as it can't be returned in-place.

     On a character level, there are 3 possible operations:

     - Skip the character (don't include in the normalized data)
     - Include the character (do include in the normalizad data)
     - Include as another character
       This is essentially the same as skipping the current character
       and inserting a given character in the output data.

    The macros below (SKIP, INCLUDE and INCLUDE_AS) are defined to
    handle the character based operations.  The macros themselves
    collect character level data into blocks.

    At all times designate the START, INCLUDED_LEN and CURP pointers
    an included and and skipped block like this:

      [ start, start + included_len ) [ start + included_len, curp )
             INCLUDED                        EXCLUDED

    When the routine flips from skipping to including, the last
    included block has to be flushed to the output buffer.
  */

  /* Going from including to skipping; only schedules the current
     included section for flushing.
     Also, simply chop off the character if it's the first in the buffer,
     so we can possibly just return the remainder of the buffer */
#define SKIP             \
  do {                   \
    if (start == curp)   \
       ++start;          \
    last_skipped = TRUE; \
  } while (0)

#define INCLUDE                \
  do {                         \
    if (last_skipped)          \
      COPY_INCLUDED_SECTION;   \
    ++include_len;             \
    last_skipped = FALSE;      \
  } while (0)

#define COPY_INCLUDED_SECTION                     \
  do {                                            \
    if (include_len > 0)                          \
      {                                           \
         memmove(tgt_newend, start, include_len); \
         tgt_newend += include_len;               \
         include_len = 0;                         \
      }                                           \
    start = curp;                                 \
  } while (0)

  /* Include the current character as character X.
     If the current character already *is* X, add it to the
     currently included region, increasing chances for consecutive
     fully normalized blocks. */
#define INCLUDE_AS(x)          \
  do {                         \
    if (*curp == (x))          \
      INCLUDE;                 \
    else                       \
      {                        \
        INSERT((x));           \
        SKIP;                  \
      }                        \
  } while (0)

  /* Insert character X in the output buffer */
#define INSERT(x)              \
  do {                         \
    COPY_INCLUDED_SECTION;     \
    *tgt_newend++ = (x);       \
  } while (0)

  for (curp = buf, endp = buf + *lengthp; curp != endp; ++curp)
    {
      switch (*curp)
        {
        case '\r':
          if (opts->ignore_eol_style)
            INCLUDE_AS('\n');
          else
            INCLUDE;
          state = svn_diff__normalize_state_cr;
          break;

        case '\n':
          if (state == svn_diff__normalize_state_cr
              && opts->ignore_eol_style)
            SKIP;
          else
            INCLUDE;
          state = svn_diff__normalize_state_normal;
          break;

        default:
          if (svn_ctype_isspace(*curp)
              && opts->ignore_space != svn_diff_file_ignore_space_none)
            {
              /* Whitespace but not '\r' or '\n' */
              if (state != svn_diff__normalize_state_whitespace
                  && opts->ignore_space
                     == svn_diff_file_ignore_space_change)
                /*### If we can postpone insertion of the space
                  until the next non-whitespace character,
                  we have a potential of reducing the number of copies:
                  If this space is followed by more spaces,
                  this will cause a block-copy.
                  If the next non-space block is considered normalized
                  *and* preceded by a space, we can take advantage of that. */
                /* Note, the above optimization applies to 90% of the source
                   lines in our own code, since it (generally) doesn't use
                   more than one space per blank section, except for the
                   beginning of a line. */
                INCLUDE_AS(' ');
              else
                SKIP;
              state = svn_diff__normalize_state_whitespace;
            }
          else
            {
              /* Non-whitespace character, or whitespace character in
                 svn_diff_file_ignore_space_none mode. */
              INCLUDE;
              state = svn_diff__normalize_state_normal;
            }
        }
    }

  /* If we're not in whitespace, flush the last chunk of data.
   * Note that this will work correctly when this is the last chunk of the
   * file:
   * * If there is an eol, it will either have been output when we entered
   *   the state_cr, or it will be output now.
   * * If there is no eol and we're not in whitespace, then we just output
   *   everything below.
   * * If there's no eol and we are in whitespace, we want to ignore
   *   whitespace unconditionally. */

  if (*tgt == tgt_newend)
    {
      /* we haven't copied any data in to *tgt and our chunk consists
         only of one block of (already normalized) data.
         Just return the block. */
      *tgt = (char *)start;
      *lengthp = include_len;
    }
  else
    {
      COPY_INCLUDED_SECTION;
      *lengthp = tgt_newend - *tgt;
    }

  *statep = state;

#undef SKIP
#undef INCLUDE
#undef INCLUDE_AS
#undef INSERT
#undef COPY_INCLUDED_SECTION
}


/* Return the library version number. */
const svn_version_t *
svn_diff_version(void)
{
  SVN_VERSION_BODY;
}

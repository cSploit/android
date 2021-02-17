/* error.c:  common exception handling for Subversion
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



#include <stdarg.h>

#include <apr_general.h>
#include <apr_pools.h>
#include <apr_strings.h>

#include "svn_cmdline.h"
#include "svn_error.h"
#include "svn_pools.h"
#include "svn_utf.h"

#ifdef SVN_DEBUG
/* XXX FIXME: These should be protected by a thread mutex.
   svn_error__locate and make_error_internal should cooperate
   in locking and unlocking it. */

/* XXX TODO: Define mutex here #if APR_HAS_THREADS */
static const char * volatile error_file = NULL;
static long volatile error_line = -1;

/* file_line for the non-debug case. */
static const char SVN_FILE_LINE_UNDEFINED[] = "svn:<undefined>";
#endif /* SVN_DEBUG */

#include "svn_private_config.h"
#include "private/svn_error_private.h"


/*
 * Undefine the helpers for creating errors.
 *
 * *NOTE*: Any use of these functions in any other function may need
 * to call svn_error__locate() because the macro that would otherwise
 * do this is being undefined and the filename and line number will
 * not be properly set in the static error_file and error_line
 * variables.
 */
#undef svn_error_create
#undef svn_error_createf
#undef svn_error_quick_wrap
#undef svn_error_wrap_apr

/* Note: This function was historically in the public API, so we need
 * to define it even when !SVN_DEBUG. */
void
svn_error__locate(const char *file, long line)
{
#if defined(SVN_DEBUG)
  /* XXX TODO: Lock mutex here */
  error_file = file;
  error_line = line;
#endif
}


/* Cleanup function for errors.  svn_error_clear () removes this so
   errors that are properly handled *don't* hit this code. */
#if defined(SVN_DEBUG)
static apr_status_t err_abort(void *data)
{
  svn_error_t *err = data;  /* For easy viewing in a debugger */
  err = err; /* Fake a use for the variable to avoid compiler warnings */

  if (!getenv("SVN_DBG_NO_ABORT_ON_ERROR_LEAK"))
    abort();
  return APR_SUCCESS;
}
#endif


static svn_error_t *
make_error_internal(apr_status_t apr_err,
                    svn_error_t *child)
{
  apr_pool_t *pool;
  svn_error_t *new_error;

  /* Reuse the child's pool, or create our own. */
  if (child)
    pool = child->pool;
  else
    {
      if (apr_pool_create(&pool, NULL))
        abort();
    }

  /* Create the new error structure */
  new_error = apr_pcalloc(pool, sizeof(*new_error));

  /* Fill 'er up. */
  new_error->apr_err = apr_err;
  new_error->child   = child;
  new_error->pool    = pool;
#if defined(SVN_DEBUG)
  new_error->file    = error_file;
  new_error->line    = error_line;
  /* XXX TODO: Unlock mutex here */

  if (! child)
      apr_pool_cleanup_register(pool, new_error,
                                err_abort,
                                apr_pool_cleanup_null);
#endif

  return new_error;
}



/*** Creating and destroying errors. ***/

svn_error_t *
svn_error_create(apr_status_t apr_err,
                 svn_error_t *child,
                 const char *message)
{
  svn_error_t *err;

  err = make_error_internal(apr_err, child);

  if (message)
    err->message = apr_pstrdup(err->pool, message);

  return err;
}


svn_error_t *
svn_error_createf(apr_status_t apr_err,
                  svn_error_t *child,
                  const char *fmt,
                  ...)
{
  svn_error_t *err;
  va_list ap;

  err = make_error_internal(apr_err, child);

  va_start(ap, fmt);
  err->message = apr_pvsprintf(err->pool, fmt, ap);
  va_end(ap);

  return err;
}


svn_error_t *
svn_error_wrap_apr(apr_status_t status,
                   const char *fmt,
                   ...)
{
  svn_error_t *err, *utf8_err;
  va_list ap;
  char errbuf[255];
  const char *msg_apr, *msg;

  err = make_error_internal(status, NULL);

  if (fmt)
    {
      /* Grab the APR error message. */
      apr_strerror(status, errbuf, sizeof(errbuf));
      utf8_err = svn_utf_cstring_to_utf8(&msg_apr, errbuf, err->pool);
      if (utf8_err)
        msg_apr = NULL;
      svn_error_clear(utf8_err);

      /* Append it to the formatted message. */
      va_start(ap, fmt);
      msg = apr_pvsprintf(err->pool, fmt, ap);
      va_end(ap);
      err->message = apr_psprintf(err->pool, "%s%s%s", msg,
                                  (msg_apr) ? ": " : "",
                                  (msg_apr) ? msg_apr : "");
    }

  return err;
}


svn_error_t *
svn_error_quick_wrap(svn_error_t *child, const char *new_msg)
{
  if (child == SVN_NO_ERROR)
    return SVN_NO_ERROR;

  return svn_error_create(child->apr_err,
                          child,
                          new_msg);
}


svn_error_t *
svn_error_compose_create(svn_error_t *err1,
                         svn_error_t *err2)
{
  if (err1 && err2)
    {
      svn_error_compose(err1, err2);
      return err1;
    }
  return err1 ? err1 : err2;
}


void
svn_error_compose(svn_error_t *chain, svn_error_t *new_err)
{
  apr_pool_t *pool = chain->pool;
  apr_pool_t *oldpool = new_err->pool;

  while (chain->child)
    chain = chain->child;

#if defined(SVN_DEBUG)
  /* Kill existing handler since the end of the chain is going to change */
  apr_pool_cleanup_kill(pool, chain, err_abort);
#endif

  /* Copy the new error chain into the old chain's pool. */
  while (new_err)
    {
      chain->child = apr_palloc(pool, sizeof(*chain->child));
      chain = chain->child;
      *chain = *new_err;
      if (chain->message)
        chain->message = apr_pstrdup(pool, new_err->message);
      chain->pool = pool;
#if defined(SVN_DEBUG)
      if (! new_err->child)
        apr_pool_cleanup_kill(oldpool, new_err, err_abort);
#endif
      new_err = new_err->child;
    }

#if defined(SVN_DEBUG)
  apr_pool_cleanup_register(pool, chain,
                            err_abort,
                            apr_pool_cleanup_null);
#endif

  /* Destroy the new error chain. */
  svn_pool_destroy(oldpool);
}

svn_error_t *
svn_error_root_cause(svn_error_t *err)
{
  while (err)
    {
      if (err->child)
        err = err->child;
      else
        break;
    }

  return err;
}

svn_error_t *
svn_error_find_cause(svn_error_t *err, apr_status_t apr_err)
{
  svn_error_t *child;

  for (child = err; child; child = child->child)
    if (child->apr_err == apr_err)
      return child;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_error_dup(svn_error_t *err)
{
  apr_pool_t *pool;
  svn_error_t *new_err = NULL, *tmp_err = NULL;

  if (apr_pool_create(&pool, NULL))
    abort();

  for (; err; err = err->child)
    {
      if (! new_err)
        {
          new_err = apr_palloc(pool, sizeof(*new_err));
          tmp_err = new_err;
        }
      else
        {
          tmp_err->child = apr_palloc(pool, sizeof(*tmp_err->child));
          tmp_err = tmp_err->child;
        }
      *tmp_err = *err;
      tmp_err->pool = pool;
      if (tmp_err->message)
        tmp_err->message = apr_pstrdup(pool, tmp_err->message);
    }

#if defined(SVN_DEBUG)
  apr_pool_cleanup_register(pool, tmp_err,
                            err_abort,
                            apr_pool_cleanup_null);
#endif

  return new_err;
}

void
svn_error_clear(svn_error_t *err)
{
  if (err)
    {
#if defined(SVN_DEBUG)
      while (err->child)
        err = err->child;
      apr_pool_cleanup_kill(err->pool, err, err_abort);
#endif
      svn_pool_destroy(err->pool);
    }
}

svn_boolean_t
svn_error__is_tracing_link(svn_error_t *err)
{
#ifdef SVN_ERR__TRACING
  /* ### A strcmp()?  Really?  I think it's the best we can do unless
     ### we add a boolean field to svn_error_t that's set only for
     ### these "placeholder error chain" items.  Not such a bad idea,
     ### really...  */
  return (err && err->message && !strcmp(err->message, SVN_ERR__TRACED));
#else
  return FALSE;
#endif
}

svn_error_t *
svn_error_purge_tracing(svn_error_t *err)
{
#ifdef SVN_ERR__TRACING
  svn_error_t *new_err = NULL, *new_err_leaf = NULL;

  if (! err)
    return SVN_NO_ERROR;

  do
    {
      svn_error_t *tmp_err;

      /* Skip over any trace-only links. */
      while (err && svn_error__is_tracing_link(err))
        err = err->child;

      /* The link must be a real link in the error chain, otherwise an
         error chain with trace only links would map into SVN_NO_ERROR. */
      if (! err)
        return svn_error_create(
                 SVN_ERR_ASSERTION_ONLY_TRACING_LINKS,
                 svn_error_compose_create(
                   svn_error__malfunction(TRUE, __FILE__, __LINE__,
                                          NULL /* ### say something? */),
                   err),
                 NULL);

      /* Copy the current error except for its child error pointer
         into the new error.  Share any message and source filename
         strings from the error. */
      tmp_err = apr_palloc(err->pool, sizeof(*tmp_err));
      *tmp_err = *err;
      tmp_err->child = NULL;

      /* Add a new link to the new chain (creating the chain if necessary). */
      if (! new_err)
        {
          new_err = tmp_err;
          new_err_leaf = tmp_err;
        }
      else
        {
          new_err_leaf->child = tmp_err;
          new_err_leaf = tmp_err;
        }

      /* Advance to the next link in the original chain. */
      err = err->child;
    } while (err);

  return new_err;
#else  /* SVN_ERR__TRACING */
  return err;
#endif /* SVN_ERR__TRACING */
}

static void
print_error(svn_error_t *err, FILE *stream, const char *prefix)
{
  char errbuf[256];
  const char *err_string;
  svn_error_t *temp_err = NULL;  /* ensure initialized even if
                                    err->file == NULL */
  /* Pretty-print the error */
  /* Note: we can also log errors here someday. */

#ifdef SVN_DEBUG
  /* Note: err->file is _not_ in UTF-8, because it's expanded from
           the __FILE__ preprocessor macro. */
  const char *file_utf8;

  if (err->file
      && !(temp_err = svn_utf_cstring_to_utf8(&file_utf8, err->file,
                                              err->pool)))
    svn_error_clear(svn_cmdline_fprintf(stream, err->pool,
                                        "%s:%ld", err->file, err->line));
  else
    {
      svn_error_clear(svn_cmdline_fputs(SVN_FILE_LINE_UNDEFINED,
                                        stream, err->pool));
      svn_error_clear(temp_err);
    }

  svn_error_clear(svn_cmdline_fprintf(stream, err->pool,
                                      ": (apr_err=%d)\n", err->apr_err));
#endif /* SVN_DEBUG */

  /* "traced call" */
  if (svn_error__is_tracing_link(err))
    {
      /* Skip it.  We already printed the file-line coordinates. */
    }
  /* Only print the same APR error string once. */
  else if (err->message)
    {
      svn_error_clear(svn_cmdline_fprintf(stream, err->pool,
                                          "%sE%06d: %s\n",
                                          prefix, err->apr_err, err->message));
    }
  else
    {
      /* Is this a Subversion-specific error code? */
      if ((err->apr_err > APR_OS_START_USEERR)
          && (err->apr_err <= APR_OS_START_CANONERR))
        err_string = svn_strerror(err->apr_err, errbuf, sizeof(errbuf));
      /* Otherwise, this must be an APR error code. */
      else if ((temp_err = svn_utf_cstring_to_utf8
                (&err_string, apr_strerror(err->apr_err, errbuf,
                                           sizeof(errbuf)), err->pool)))
        {
          svn_error_clear(temp_err);
          err_string = _("Can't recode error string from APR");
        }

      svn_error_clear(svn_cmdline_fprintf(stream, err->pool,
                                          "%sE%06d: %s\n",
                                          prefix, err->apr_err, err_string));
    }
}

void
svn_handle_error(svn_error_t *err, FILE *stream, svn_boolean_t fatal)
{
  svn_handle_error2(err, stream, fatal, "svn: ");
}

void
svn_handle_error2(svn_error_t *err,
                  FILE *stream,
                  svn_boolean_t fatal,
                  const char *prefix)
{
  /* In a long error chain, there may be multiple errors with the same
     error code and no custom message.  We only want to print the
     default message for that code once; printing it multiple times
     would add no useful information.  The 'empties' array below
     remembers the codes of empty errors already seen in the chain.

     We could allocate it in err->pool, but there's no telling how
     long err will live or how many times it will get handled.  So we
     use a subpool. */
  apr_pool_t *subpool;
  apr_array_header_t *empties;
  svn_error_t *tmp_err;

  /* ### The rest of this file carefully avoids using svn_pool_*(),
     preferring apr_pool_*() instead.  I can't remember why -- it may
     be an artifact of r843793, or it may be for some deeper reason --
     but I'm playing it safe and using apr_pool_*() here too. */
  apr_pool_create(&subpool, err->pool);
  empties = apr_array_make(subpool, 0, sizeof(apr_status_t));

  tmp_err = err;
  while (tmp_err)
    {
      svn_boolean_t printed_already = FALSE;

      if (! tmp_err->message)
        {
          int i;

          for (i = 0; i < empties->nelts; i++)
            {
              if (tmp_err->apr_err == APR_ARRAY_IDX(empties, i, apr_status_t) )
                {
                  printed_already = TRUE;
                  break;
                }
            }
        }

      if (! printed_already)
        {
          print_error(tmp_err, stream, prefix);
          if (! tmp_err->message)
            {
              APR_ARRAY_PUSH(empties, apr_status_t) = tmp_err->apr_err;
            }
        }

      tmp_err = tmp_err->child;
    }

  svn_pool_destroy(subpool);

  fflush(stream);
  if (fatal)
    {
      /* Avoid abort()s in maintainer mode. */
      svn_error_clear(err);

      /* We exit(1) here instead of abort()ing so that atexit handlers
         get called. */
      exit(EXIT_FAILURE);
    }
}


void
svn_handle_warning(FILE *stream, svn_error_t *err)
{
  svn_handle_warning2(stream, err, "svn: ");
}

void
svn_handle_warning2(FILE *stream, svn_error_t *err, const char *prefix)
{
  char buf[256];

  svn_error_clear(svn_cmdline_fprintf
                  (stream, err->pool,
                   _("%swarning: W%06d: %s\n"),
                   prefix, err->apr_err,
                   svn_err_best_message(err, buf, sizeof(buf))));
  fflush(stream);
}

const char *
svn_err_best_message(svn_error_t *err, char *buf, apr_size_t bufsize)
{
  /* Skip over any trace records.  */
  while (svn_error__is_tracing_link(err))
    err = err->child;
  if (err->message)
    return err->message;
  else
    return svn_strerror(err->apr_err, buf, bufsize);
}


/* svn_strerror() and helpers */

typedef struct err_defn {
  svn_errno_t errcode;
  const char *errdesc;
} err_defn;

/* To understand what is going on here, read svn_error_codes.h. */
#define SVN_ERROR_BUILD_ARRAY
#include "svn_error_codes.h"

char *
svn_strerror(apr_status_t statcode, char *buf, apr_size_t bufsize)
{
  const err_defn *defn;

  for (defn = error_table; defn->errdesc != NULL; ++defn)
    if (defn->errcode == (svn_errno_t)statcode)
      {
        apr_cpystrn(buf, _(defn->errdesc), bufsize);
        return buf;
      }

  return apr_strerror(statcode, buf, bufsize);
}

svn_error_t *
svn_error_raise_on_malfunction(svn_boolean_t can_return,
                               const char *file, int line,
                               const char *expr)
{
  if (!can_return)
    abort(); /* Nothing else we can do as a library */

  /* The filename and line number of the error source needs to be set
     here because svn_error_createf() is not the macro defined in
     svn_error.h but the real function. */
  svn_error__locate(file, line);

  if (expr)
    return svn_error_createf(SVN_ERR_ASSERTION_FAIL, NULL,
                             _("In file '%s' line %d: assertion failed (%s)"),
                             file, line, expr);
  else
    return svn_error_createf(SVN_ERR_ASSERTION_FAIL, NULL,
                             _("In file '%s' line %d: internal malfunction"),
                             file, line);
}

svn_error_t *
svn_error_abort_on_malfunction(svn_boolean_t can_return,
                               const char *file, int line,
                               const char *expr)
{
  svn_error_t *err = svn_error_raise_on_malfunction(TRUE, file, line, expr);

  svn_handle_error2(err, stderr, FALSE, "svn: ");
  abort();
  return err;  /* Not reached. */
}

/* The current handler for reporting malfunctions, and its default setting. */
static svn_error_malfunction_handler_t malfunction_handler
  = svn_error_abort_on_malfunction;

svn_error_malfunction_handler_t
svn_error_set_malfunction_handler(svn_error_malfunction_handler_t func)
{
  svn_error_malfunction_handler_t old_malfunction_handler
    = malfunction_handler;

  malfunction_handler = func;
  return old_malfunction_handler;
}

svn_error_t *
svn_error__malfunction(svn_boolean_t can_return,
                       const char *file, int line,
                       const char *expr)
{
  return malfunction_handler(can_return, file, line, expr);
}

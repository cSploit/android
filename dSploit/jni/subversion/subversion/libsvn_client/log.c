/*
 * log.c:  return log messages
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

#define APR_WANT_STRFUNC
#include <apr_want.h>

#include <apr_strings.h>
#include <apr_pools.h>

#include "svn_pools.h"
#include "svn_client.h"
#include "svn_compat.h"
#include "svn_error.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_sorts.h"
#include "svn_props.h"

#include "client.h"

#include "svn_private_config.h"
#include "private/svn_wc_private.h"


/*** Getting misc. information ***/

/* The baton for use with copyfrom_info_receiver(). */
typedef struct copyfrom_info_t
{
  svn_boolean_t is_first;
  const char *path;
  svn_revnum_t rev;
  apr_pool_t *pool;
} copyfrom_info_t;

/* A location segment callback for obtaining the copy source of
   a node at a path and storing it in *BATON (a struct copyfrom_info_t *).
   Implements svn_location_segment_receiver_t. */
static svn_error_t *
copyfrom_info_receiver(svn_location_segment_t *segment,
                       void *baton,
                       apr_pool_t *pool)
{
  copyfrom_info_t *copyfrom_info = baton;

  /* If we've already identified the copy source, there's nothing more
     to do.
     ### FIXME:  We *should* be able to send */
  if (copyfrom_info->path)
    return SVN_NO_ERROR;

  /* If this is the first segment, it's not of interest to us. Otherwise
     (so long as this segment doesn't represent a history gap), it holds
     our path's previous location (from which it was last copied). */
  if (copyfrom_info->is_first)
    {
      copyfrom_info->is_first = FALSE;
    }
  else if (segment->path)
    {
      /* The end of the second non-gap segment is the location copied from.  */
      copyfrom_info->path = apr_pstrdup(copyfrom_info->pool, segment->path);
      copyfrom_info->rev = segment->range_end;

      /* ### FIXME: We *should* be able to return SVN_ERR_CEASE_INVOCATION
         ### here so we don't get called anymore. */
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_client__get_copy_source(const char *path_or_url,
                            const svn_opt_revision_t *revision,
                            const char **copyfrom_path,
                            svn_revnum_t *copyfrom_rev,
                            svn_client_ctx_t *ctx,
                            apr_pool_t *pool)
{
  svn_error_t *err;
  copyfrom_info_t copyfrom_info = { 0 };
  apr_pool_t *sesspool = svn_pool_create(pool);
  svn_ra_session_t *ra_session;
  svn_revnum_t at_rev;
  const char *at_url;

  copyfrom_info.is_first = TRUE;
  copyfrom_info.path = NULL;
  copyfrom_info.rev = SVN_INVALID_REVNUM;
  copyfrom_info.pool = pool;

  SVN_ERR(svn_client__ra_session_from_path(&ra_session, &at_rev, &at_url,
                                           path_or_url, NULL,
                                           revision, revision,
                                           ctx, sesspool));

  /* Find the copy source.  Walk the location segments to find the revision
     at which this node was created (copied or added). */

  err = svn_ra_get_location_segments(ra_session, "", at_rev, at_rev,
                                     SVN_INVALID_REVNUM,
                                     copyfrom_info_receiver, &copyfrom_info,
                                     pool);

  svn_pool_destroy(sesspool);

  if (err)
    {
      if (err->apr_err == SVN_ERR_FS_NOT_FOUND ||
          err->apr_err == SVN_ERR_RA_DAV_REQUEST_FAILED)
        {
          /* A locally-added but uncommitted versioned resource won't
             exist in the repository. */
            svn_error_clear(err);
            err = SVN_NO_ERROR;

            *copyfrom_path = NULL;
            *copyfrom_rev = SVN_INVALID_REVNUM;
        }
      return svn_error_trace(err);
    }

  *copyfrom_path = copyfrom_info.path;
  *copyfrom_rev = copyfrom_info.rev;
  return SVN_NO_ERROR;
}


/* compatibility with pre-1.5 servers, which send only author/date/log
 *revprops in log entries */
typedef struct pre_15_receiver_baton_t
{
  svn_client_ctx_t *ctx;
  /* ra session for retrieving revprops from old servers */
  svn_ra_session_t *ra_session;
  /* caller's list of requested revprops, receiver, and baton */
  const char *ra_session_url;
  apr_pool_t *ra_session_pool;
  const apr_array_header_t *revprops;
  svn_log_entry_receiver_t receiver;
  void *baton;
} pre_15_receiver_baton_t;

static svn_error_t *
pre_15_receiver(void *baton, svn_log_entry_t *log_entry, apr_pool_t *pool)
{
  pre_15_receiver_baton_t *rb = baton;

  if (log_entry->revision == SVN_INVALID_REVNUM)
    return rb->receiver(rb->baton, log_entry, pool);

  /* If only some revprops are requested, get them one at a time on the
     second ra connection.  If all are requested, get them all with
     svn_ra_rev_proplist.  This avoids getting unrequested revprops (which
     may be arbitrarily large), but means one round-trip per requested
     revprop.  epg isn't entirely sure which should be optimized for. */
  if (rb->revprops)
    {
      int i;
      svn_boolean_t want_author, want_date, want_log;
      want_author = want_date = want_log = FALSE;
      for (i = 0; i < rb->revprops->nelts; i++)
        {
          const char *name = APR_ARRAY_IDX(rb->revprops, i, const char *);
          svn_string_t *value;

          /* If a standard revprop is requested, we know it is already in
             log_entry->revprops if available. */
          if (strcmp(name, SVN_PROP_REVISION_AUTHOR) == 0)
            {
              want_author = TRUE;
              continue;
            }
          if (strcmp(name, SVN_PROP_REVISION_DATE) == 0)
            {
              want_date = TRUE;
              continue;
            }
          if (strcmp(name, SVN_PROP_REVISION_LOG) == 0)
            {
              want_log = TRUE;
              continue;
            }

          if (rb->ra_session == NULL)
            SVN_ERR(svn_client_open_ra_session(&rb->ra_session,
                                                rb->ra_session_url,
                                               rb->ctx, rb->ra_session_pool));

          SVN_ERR(svn_ra_rev_prop(rb->ra_session, log_entry->revision,
                                  name, &value, pool));
          if (log_entry->revprops == NULL)
            log_entry->revprops = apr_hash_make(pool);
          apr_hash_set(log_entry->revprops, (const void *)name,
                       APR_HASH_KEY_STRING, (const void *)value);
        }
      if (log_entry->revprops)
        {
          /* Pre-1.5 servers send the standard revprops unconditionally;
             clear those the caller doesn't want. */
          if (!want_author)
            apr_hash_set(log_entry->revprops, SVN_PROP_REVISION_AUTHOR,
                         APR_HASH_KEY_STRING, NULL);
          if (!want_date)
            apr_hash_set(log_entry->revprops, SVN_PROP_REVISION_DATE,
                         APR_HASH_KEY_STRING, NULL);
          if (!want_log)
            apr_hash_set(log_entry->revprops, SVN_PROP_REVISION_LOG,
                         APR_HASH_KEY_STRING, NULL);
        }
    }
  else
    {
      if (rb->ra_session == NULL)
        SVN_ERR(svn_client_open_ra_session(&rb->ra_session,
                                           rb->ra_session_url,
                                           rb->ctx, rb->ra_session_pool));

      SVN_ERR(svn_ra_rev_proplist(rb->ra_session, log_entry->revision,
                                  &log_entry->revprops, pool));
    }

  return rb->receiver(rb->baton, log_entry, pool);
}

/* limit receiver */
typedef struct limit_receiver_baton_t
{
  int limit;
  svn_log_entry_receiver_t receiver;
  void *baton;
} limit_receiver_baton_t;

static svn_error_t *
limit_receiver(void *baton, svn_log_entry_t *log_entry, apr_pool_t *pool)
{
  limit_receiver_baton_t *rb = baton;

  rb->limit--;

  return rb->receiver(rb->baton, log_entry, pool);
}


/*** Public Interface. ***/


svn_error_t *
svn_client_log5(const apr_array_header_t *targets,
                const svn_opt_revision_t *peg_revision,
                const apr_array_header_t *revision_ranges,
                int limit,
                svn_boolean_t discover_changed_paths,
                svn_boolean_t strict_node_history,
                svn_boolean_t include_merged_revisions,
                const apr_array_header_t *revprops,
                svn_log_entry_receiver_t real_receiver,
                void *real_receiver_baton,
                svn_client_ctx_t *ctx,
                apr_pool_t *pool)
{
  svn_ra_session_t *ra_session;
  const char *url_or_path;
  svn_boolean_t has_log_revprops;
  const char *actual_url;
  apr_array_header_t *condensed_targets;
  svn_revnum_t ignored_revnum;
  svn_opt_revision_t session_opt_rev;
  const char *ra_target;
  pre_15_receiver_baton_t rb = {0};
  apr_pool_t *iterpool;
  int i;
  svn_opt_revision_t peg_rev;

  if (revision_ranges->nelts == 0)
    {
      return svn_error_create
        (SVN_ERR_CLIENT_BAD_REVISION, NULL,
         _("Missing required revision specification"));
    }

  /* Make a copy of PEG_REVISION, we may need to change it to a
     default value. */
  peg_rev.kind = peg_revision->kind;
  peg_rev.value = peg_revision->value;

  /* Use the passed URL, if there is one.  */
  url_or_path = APR_ARRAY_IDX(targets, 0, const char *);
  session_opt_rev.kind = svn_opt_revision_unspecified;

  for (i = 0; i < revision_ranges->nelts; i++)
    {
      svn_opt_revision_range_t *range;

      range = APR_ARRAY_IDX(revision_ranges, i, svn_opt_revision_range_t *);

      if ((range->start.kind != svn_opt_revision_unspecified)
          && (range->end.kind == svn_opt_revision_unspecified))
        {
          /* If the user specified exactly one revision, then start rev is
           * set but end is not.  We show the log message for just that
           * revision by making end equal to start.
           *
           * Note that if the user requested a single dated revision, then
           * this will cause the same date to be resolved twice.  The
           * extra code complexity to get around this slight inefficiency
           * doesn't seem worth it, however. */
          range->end = range->start;
        }
      else if (range->start.kind == svn_opt_revision_unspecified)
        {
          /* Default to any specified peg revision.  Otherwise, if the
           * first target is an URL, then we default to HEAD:0.  Lastly,
           * the default is BASE:0 since WC@HEAD may not exist. */
          if (peg_rev.kind == svn_opt_revision_unspecified)
            {
              if (svn_path_is_url(url_or_path))
                range->start.kind = svn_opt_revision_head;
              else
                range->start.kind = svn_opt_revision_base;
            }
          else
            range->start = peg_rev;

          if (range->end.kind == svn_opt_revision_unspecified)
            {
              range->end.kind = svn_opt_revision_number;
              range->end.value.number = 0;
            }
        }

      if ((range->start.kind == svn_opt_revision_unspecified)
          || (range->end.kind == svn_opt_revision_unspecified))
        {
          return svn_error_create
            (SVN_ERR_CLIENT_BAD_REVISION, NULL,
             _("Missing required revision specification"));
        }

      /* Determine the revision to open the RA session to. */
      if (session_opt_rev.kind == svn_opt_revision_unspecified)
        {
          if (range->start.kind == svn_opt_revision_number &&
              range->end.kind == svn_opt_revision_number)
            {
              session_opt_rev =
                  (range->start.value.number > range->end.value.number ?
                   range->start : range->end);
            }
          else if (range->start.kind == svn_opt_revision_head ||
                   range->end.kind == svn_opt_revision_head)
            {
              session_opt_rev.kind = svn_opt_revision_head;
            }
          else if (range->start.kind == svn_opt_revision_date &&
                   range->end.kind == svn_opt_revision_date)
            {
              session_opt_rev =
                  (range->start.value.date > range->end.value.date ?
                   range->start : range->end);
            }
        }
    }

  /* Use the passed URL, if there is one.  */
  if (svn_path_is_url(url_or_path))
    {
      /* Initialize this array, since we'll be building it below */
      condensed_targets = apr_array_make(pool, 1, sizeof(const char *));

      /* The logic here is this: If we get passed one argument, we assume
         it is the full URL to a file/dir we want log info for. If we get
         a URL plus some paths, then we assume that the URL is the base,
         and that the paths passed are relative to it.  */
      if (targets->nelts > 1)
        {
          /* We have some paths, let's use them. Start after the URL.  */
          for (i = 1; i < targets->nelts; i++)
            {
              const char *target;

              target = APR_ARRAY_IDX(targets, i, const char *);

              if (svn_path_is_url(target) || svn_dirent_is_absolute(target))
                return svn_error_createf(SVN_ERR_ILLEGAL_TARGET, NULL,
                                         _("'%s' is not a relative path"),
                                          target);

              APR_ARRAY_PUSH(condensed_targets, const char *) = target;
            }
        }
      else
        {
          /* If we have a single URL, then the session will be rooted at
             it, so just send an empty string for the paths we are
             interested in. */
          APR_ARRAY_PUSH(condensed_targets, const char *) = "";
        }
    }
  else
    {
      apr_array_header_t *target_urls;
      apr_array_header_t *real_targets;

      /* See FIXME about multiple wc targets, below. */
      if (targets->nelts > 1)
        return svn_error_create(SVN_ERR_UNSUPPORTED_FEATURE, NULL,
                                _("When specifying working copy paths, only "
                                  "one target may be given"));

      /* An unspecified PEG_REVISION for a working copy path defautls
         to svn_opt_revision_working. */
      if (peg_rev.kind == svn_opt_revision_unspecified)
          peg_rev.kind = svn_opt_revision_working;

      /* Get URLs for each target */
      target_urls = apr_array_make(pool, 1, sizeof(const char *));
      real_targets = apr_array_make(pool, 1, sizeof(const char *));
      iterpool = svn_pool_create(pool);
      for (i = 0; i < targets->nelts; i++)
        {
          const char *url;
          const char *target = APR_ARRAY_IDX(targets, i, const char *);
          const char *target_abspath;

          svn_pool_clear(iterpool);
          SVN_ERR(svn_dirent_get_absolute(&target_abspath, target, iterpool));
          SVN_ERR(svn_wc__node_get_url(&url, ctx->wc_ctx, target_abspath,
                                       pool, iterpool));

          if (! url)
            return svn_error_createf
              (SVN_ERR_ENTRY_MISSING_URL, NULL,
               _("Entry '%s' has no URL"),
               svn_dirent_local_style(target, pool));

          APR_ARRAY_PUSH(target_urls, const char *) = url;
          APR_ARRAY_PUSH(real_targets, const char *) = target;
        }

      /* if we have no valid target_urls, just exit. */
      if (target_urls->nelts == 0)
        return SVN_NO_ERROR;

      /* Find the base URL and condensed targets relative to it. */
      SVN_ERR(svn_uri_condense_targets(&url_or_path, &condensed_targets,
                                       target_urls, TRUE, pool, iterpool));

      if (condensed_targets->nelts == 0)
        APR_ARRAY_PUSH(condensed_targets, const char *) = "";

      /* 'targets' now becomes 'real_targets', which has bogus,
         unversioned things removed from it. */
      targets = real_targets;
      svn_pool_destroy(iterpool);
    }


  {
    /* If this is a revision type that requires access to the working copy,
     * we use our initial target path to figure out where to root the RA
     * session, otherwise we use our URL. */
    if (SVN_CLIENT__REVKIND_NEEDS_WC(peg_rev.kind))
      SVN_ERR(svn_dirent_condense_targets(&ra_target, NULL, targets,
                                          TRUE, pool, pool));
    else
      ra_target = url_or_path;

    SVN_ERR(svn_client__ra_session_from_path(&ra_session, &ignored_revnum,
                                             &actual_url, ra_target, NULL,
                                             &peg_rev, &session_opt_rev,
                                             ctx, pool));

    SVN_ERR(svn_ra_has_capability(ra_session, &has_log_revprops,
                                  SVN_RA_CAPABILITY_LOG_REVPROPS, pool));

    if (!has_log_revprops) {
      /* See above pre-1.5 notes. */
      rb.ctx = ctx;

      /* Create ra session on first use */
      rb.ra_session_pool = pool;
      rb.ra_session_url = actual_url;
    }
  }

  /* It's a bit complex to correctly handle the special revision words
   * such as "BASE", "COMMITTED", and "PREV".  For example, if the
   * user runs
   *
   *   $ svn log -rCOMMITTED foo.txt bar.c
   *
   * which committed rev should be used?  The younger of the two?  The
   * first one?  Should we just error?
   *
   * None of the above, I think.  Rather, the committed rev of each
   * target in turn should be used.  This is what most users would
   * expect, and is the most useful interpretation.  Of course, this
   * goes for the other dynamic (i.e., local) revision words too.
   *
   * Note that the code to do this is a bit more complex than a simple
   * loop, because the user might run
   *
   *    $ svn log -rCOMMITTED:42 foo.txt bar.c
   *
   * in which case we want to avoid recomputing the static revision on
   * every iteration.
   *
   * ### FIXME: However, we can't yet handle multiple wc targets anyway.
   *
   * We used to iterate over each target in turn, getting the logs for
   * the named range.  This led to revisions being printed in strange
   * order or being printed more than once.  This is issue 1550.
   *
   * In r851673, jpieper blocked multiple wc targets in svn/log-cmd.c,
   * meaning this block not only doesn't work right in that case, but isn't
   * even testable that way (svn has no unit test suite; we can only test
   * via the svn command).  So, that check is now moved into this function
   * (see above).
   *
   * kfogel ponders future enhancements in r844260:
   * I think that's okay behavior, since the sense of the command is
   * that one wants a particular range of logs for *this* file, then
   * another range for *that* file, and so on.  But we should
   * probably put some sort of separator header between the log
   * groups.  Of course, libsvn_client can't just print stuff out --
   * it has to take a callback from the client to do that.  So we
   * need to define that callback interface, then have the command
   * line client pass one down here.
   *
   * epg wonders if the repository could send a unified stream of log
   * entries if the paths and revisions were passed down.
   */
  iterpool = svn_pool_create(pool);
  for (i = 0; i < revision_ranges->nelts; i++)
    {
      svn_revnum_t start_revnum, end_revnum, youngest_rev = SVN_INVALID_REVNUM;
      const char *path = APR_ARRAY_IDX(targets, 0, const char *);
      const char *local_abspath_or_url;
      svn_opt_revision_range_t *range;
      limit_receiver_baton_t lb;
      svn_log_entry_receiver_t passed_receiver;
      void *passed_receiver_baton;
      const apr_array_header_t *passed_receiver_revprops;

      svn_pool_clear(iterpool);

      if (!svn_path_is_url(path))
        SVN_ERR(svn_dirent_get_absolute(&local_abspath_or_url, path, iterpool));
      else
        local_abspath_or_url = path;

      range = APR_ARRAY_IDX(revision_ranges, i, svn_opt_revision_range_t *);

      SVN_ERR(svn_client__get_revision_number(&start_revnum, &youngest_rev,
                                              ctx->wc_ctx, local_abspath_or_url,
                                              ra_session, &range->start,
                                              iterpool));
      SVN_ERR(svn_client__get_revision_number(&end_revnum, &youngest_rev,
                                              ctx->wc_ctx, local_abspath_or_url,
                                              ra_session, &range->end,
                                              iterpool));

      if (has_log_revprops)
        {
          passed_receiver = real_receiver;
          passed_receiver_baton = real_receiver_baton;
          passed_receiver_revprops = revprops;
        }
      else
        {
          rb.revprops = revprops;
          rb.receiver = real_receiver;
          rb.baton = real_receiver_baton;

          passed_receiver = pre_15_receiver;
          passed_receiver_baton = &rb;
          passed_receiver_revprops = svn_compat_log_revprops_in(iterpool);
        }

      if (limit && revision_ranges->nelts > 1)
        {
          lb.limit = limit;
          lb.receiver = passed_receiver;
          lb.baton = passed_receiver_baton;

          passed_receiver = limit_receiver;
          passed_receiver_baton = &lb;
        }

      SVN_ERR(svn_ra_get_log2(ra_session,
                              condensed_targets,
                              start_revnum,
                              end_revnum,
                              limit,
                              discover_changed_paths,
                              strict_node_history,
                              include_merged_revisions,
                              passed_receiver_revprops,
                              passed_receiver,
                              passed_receiver_baton,
                              iterpool));

      if (limit && revision_ranges->nelts > 1)
        {
          limit = lb.limit;
          if (limit == 0)
            {
              return SVN_NO_ERROR;
            }
        }
    }
  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}

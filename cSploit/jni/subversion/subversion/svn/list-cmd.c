/*
 * list-cmd.c -- list a URL
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

#include "svn_cmdline.h"
#include "svn_client.h"
#include "svn_error.h"
#include "svn_pools.h"
#include "svn_time.h"
#include "svn_xml.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_utf.h"
#include "svn_opt.h"

#include "cl.h"

#include "svn_private_config.h"



/* Baton used when printing directory entries. */
struct print_baton {
  svn_boolean_t verbose;
  svn_client_ctx_t *ctx;
};

/* This implements the svn_client_list_func_t API, printing a single
   directory entry in text format. */
static svn_error_t *
print_dirent(void *baton,
             const char *path,
             const svn_dirent_t *dirent,
             const svn_lock_t *lock,
             const char *abs_path,
             apr_pool_t *pool)
{
  struct print_baton *pb = baton;
  const char *entryname;

  if (pb->ctx->cancel_func)
    SVN_ERR(pb->ctx->cancel_func(pb->ctx->cancel_baton));

  if (strcmp(path, "") == 0)
    {
      if (dirent->kind == svn_node_file)
        entryname = svn_dirent_basename(abs_path, pool);
      else if (pb->verbose)
        entryname = ".";
      else
        /* Don't bother to list if no useful information will be shown. */
        return SVN_NO_ERROR;
    }
  else
    entryname = path;

  if (pb->verbose)
    {
      apr_time_t now = apr_time_now();
      apr_time_exp_t exp_time;
      apr_status_t apr_err;
      apr_size_t size;
      char timestr[20];
      const char *sizestr, *utf8_timestr;

      /* svn_time_to_human_cstring gives us something *way* too long
         to use for this, so we have to roll our own.  We include
         the year if the entry's time is not within half a year. */
      apr_time_exp_lt(&exp_time, dirent->time);
      if (apr_time_sec(now - dirent->time) < (365 * 86400 / 2)
          && apr_time_sec(dirent->time - now) < (365 * 86400 / 2))
        {
          apr_err = apr_strftime(timestr, &size, sizeof(timestr),
                                 _("%b %d %H:%M"), &exp_time);
        }
      else
        {
          apr_err = apr_strftime(timestr, &size, sizeof(timestr),
                                 _("%b %d  %Y"), &exp_time);
        }

      /* if that failed, just zero out the string and print nothing */
      if (apr_err)
        timestr[0] = '\0';

      /* we need it in UTF-8. */
      SVN_ERR(svn_utf_cstring_to_utf8(&utf8_timestr, timestr, pool));

      sizestr = apr_psprintf(pool, "%" SVN_FILESIZE_T_FMT, dirent->size);

      return svn_cmdline_printf
              (pool, "%7ld %-8.8s %c %10s %12s %s%s\n",
               dirent->created_rev,
               dirent->last_author ? dirent->last_author : " ? ",
               lock ? 'O' : ' ',
               (dirent->kind == svn_node_file) ? sizestr : "",
               utf8_timestr,
               entryname,
               (dirent->kind == svn_node_dir) ? "/" : "");
    }
  else
    {
      return svn_cmdline_printf(pool, "%s%s\n", entryname,
                                (dirent->kind == svn_node_dir)
                                ? "/" : "");
    }
}


/* This implements the svn_client_list_func_t API, printing a single dirent
   in XML format. */
static svn_error_t *
print_dirent_xml(void *baton,
                 const char *path,
                 const svn_dirent_t *dirent,
                 const svn_lock_t *lock,
                 const char *abs_path,
                 apr_pool_t *pool)
{
  struct print_baton *pb = baton;
  const char *entryname;
  svn_stringbuf_t *sb;

  if (strcmp(path, "") == 0)
    {
      if (dirent->kind == svn_node_file)
        entryname = svn_dirent_basename(abs_path, pool);
      else if (pb->verbose)
        entryname = ".";
      else
        /* Don't bother to list if no useful information will be shown. */
        return SVN_NO_ERROR;
    }
  else
    entryname = path;

  if (pb->ctx->cancel_func)
    SVN_ERR(pb->ctx->cancel_func(pb->ctx->cancel_baton));

  sb = svn_stringbuf_create("", pool);

  svn_xml_make_open_tag(&sb, pool, svn_xml_normal, "entry",
                        "kind", svn_cl__node_kind_str_xml(dirent->kind),
                        NULL);

  svn_cl__xml_tagged_cdata(&sb, pool, "name", entryname);

  if (dirent->kind == svn_node_file)
    {
      svn_cl__xml_tagged_cdata
        (&sb, pool, "size",
         apr_psprintf(pool, "%" SVN_FILESIZE_T_FMT, dirent->size));
    }

  svn_xml_make_open_tag(&sb, pool, svn_xml_normal, "commit",
                        "revision",
                        apr_psprintf(pool, "%ld", dirent->created_rev),
                        NULL);
  svn_cl__xml_tagged_cdata(&sb, pool, "author", dirent->last_author);
  if (dirent->time)
    svn_cl__xml_tagged_cdata(&sb, pool, "date",
                             svn_time_to_cstring(dirent->time, pool));
  svn_xml_make_close_tag(&sb, pool, "commit");

  if (lock)
    {
      svn_xml_make_open_tag(&sb, pool, svn_xml_normal, "lock", NULL);
      svn_cl__xml_tagged_cdata(&sb, pool, "token", lock->token);
      svn_cl__xml_tagged_cdata(&sb, pool, "owner", lock->owner);
      svn_cl__xml_tagged_cdata(&sb, pool, "comment", lock->comment);
      svn_cl__xml_tagged_cdata(&sb, pool, "created",
                               svn_time_to_cstring(lock->creation_date,
                                                   pool));
      if (lock->expiration_date != 0)
        svn_cl__xml_tagged_cdata(&sb, pool, "expires",
                                 svn_time_to_cstring
                                 (lock->expiration_date, pool));
      svn_xml_make_close_tag(&sb, pool, "lock");
    }

  svn_xml_make_close_tag(&sb, pool, "entry");

  return svn_cl__error_checked_fputs(sb->data, stdout);
}


/* This implements the `svn_opt_subcommand_t' interface. */
svn_error_t *
svn_cl__list(apr_getopt_t *os,
             void *baton,
             apr_pool_t *pool)
{
  svn_cl__opt_state_t *opt_state = ((svn_cl__cmd_baton_t *) baton)->opt_state;
  svn_client_ctx_t *ctx = ((svn_cl__cmd_baton_t *) baton)->ctx;
  apr_array_header_t *targets;
  int i;
  apr_pool_t *subpool = svn_pool_create(pool);
  apr_uint32_t dirent_fields;
  struct print_baton pb;
  svn_boolean_t seen_nonexistent_target = FALSE;
  svn_error_t *err;

  SVN_ERR(svn_cl__args_to_target_array_print_reserved(&targets, os,
                                                      opt_state->targets,
                                                      ctx, FALSE, pool));

  /* Add "." if user passed 0 arguments */
  svn_opt_push_implicit_dot_target(targets, pool);

  if (opt_state->xml)
    {
      /* The XML output contains all the information, so "--verbose"
         does not apply. */
      if (opt_state->verbose)
        return svn_error_create(SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                                _("'verbose' option invalid in XML mode"));

      /* If output is not incremental, output the XML header and wrap
         everything in a top-level element. This makes the output in
         its entirety a well-formed XML document. */
      if (! opt_state->incremental)
        SVN_ERR(svn_cl__xml_print_header("lists", pool));
    }
  else
    {
      if (opt_state->incremental)
        return svn_error_create(SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                                _("'incremental' option only valid in XML "
                                  "mode"));
    }

  if (opt_state->verbose || opt_state->xml)
    dirent_fields = SVN_DIRENT_ALL;
  else
    dirent_fields = SVN_DIRENT_KIND; /* the only thing we actually need... */

  pb.ctx = ctx;
  pb.verbose = opt_state->verbose;

  if (opt_state->depth == svn_depth_unknown)
    opt_state->depth = svn_depth_immediates;

  /* For each target, try to list it. */
  for (i = 0; i < targets->nelts; i++)
    {
      const char *target = APR_ARRAY_IDX(targets, i, const char *);
      const char *truepath;
      svn_opt_revision_t peg_revision;

      svn_pool_clear(subpool);

      SVN_ERR(svn_cl__check_cancel(ctx->cancel_baton));

      /* Get peg revisions. */
      SVN_ERR(svn_opt_parse_path(&peg_revision, &truepath, target,
                                 subpool));

      if (opt_state->xml)
        {
          svn_stringbuf_t *sb = svn_stringbuf_create("", pool);
          svn_xml_make_open_tag(&sb, pool, svn_xml_normal, "list",
                                "path", truepath[0] == '\0' ? "." : truepath,
                                NULL);
          SVN_ERR(svn_cl__error_checked_fputs(sb->data, stdout));
        }

      err = svn_client_list2(truepath, &peg_revision,
                             &(opt_state->start_revision),
                             opt_state->depth,
                             dirent_fields,
                             (opt_state->xml || opt_state->verbose),
                             opt_state->xml ? print_dirent_xml : print_dirent,
                             &pb, ctx, subpool);

      if (err)
        {
          /* If one of the targets is a non-existent URL or wc-entry,
             don't bail out.  Just warn and move on to the next target. */
          if (err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND ||
              err->apr_err == SVN_ERR_FS_NOT_FOUND)
              svn_handle_warning2(stderr, err, "svn: ");
          else
              return svn_error_trace(err);

          svn_error_clear(err);
          err = NULL;
          seen_nonexistent_target = TRUE;
        }

      if (opt_state->xml)
        {
          svn_stringbuf_t *sb = svn_stringbuf_create("", pool);
          svn_xml_make_close_tag(&sb, pool, "list");
          SVN_ERR(svn_cl__error_checked_fputs(sb->data, stdout));
        }
    }

  svn_pool_destroy(subpool);

  if (opt_state->xml && ! opt_state->incremental)
    SVN_ERR(svn_cl__xml_print_footer("lists", pool));

  if (seen_nonexistent_target)
    return svn_error_create(
      SVN_ERR_ILLEGAL_TARGET, NULL,
      _("Could not list all targets because some targets don't exist"));
  else
    return SVN_NO_ERROR;
}

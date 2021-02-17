/*
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
#include "svn_dirent_uri.h"
#include "svn_pools.h"
#include "svn_wc.h"
#include "svn_utf.h"
#include "svn_opt.h"
#include "svn_version.h"

#include "private/svn_opt_private.h"

#include "svn_private_config.h"

#define SVNVERSION_OPT_VERSION SVN_OPT_FIRST_LONGOPT_ID


static svn_error_t *
version(svn_boolean_t quiet, apr_pool_t *pool)
{
  return svn_opt_print_help3(NULL, "svnversion", TRUE, quiet, NULL, NULL,
                             NULL, NULL, NULL, NULL, pool);
}

static void
usage(apr_pool_t *pool)
{
  svn_error_clear(svn_cmdline_fprintf
                  (stderr, pool, _("Type 'svnversion --help' for usage.\n")));
  exit(1);
}


static void
help(const apr_getopt_option_t *options, apr_pool_t *pool)
{
  svn_error_clear
    (svn_cmdline_fprintf
     (stdout, pool,
      _("usage: svnversion [OPTIONS] [WC_PATH [TRAIL_URL]]\n\n"
        "  Produce a compact 'version number' for the working copy path\n"
        "  WC_PATH.  TRAIL_URL is the trailing portion of the URL used to\n"
        "  determine if WC_PATH itself is switched (detection of switches\n"
        "  within WC_PATH does not rely on TRAIL_URL).  The version number\n"
        "  is written to standard output.  For example:\n"
        "\n"
        "    $ svnversion . /repos/svn/trunk\n"
        "    4168\n"
        "\n"
        "  The version number will be a single number if the working\n"
        "  copy is single revision, unmodified, not switched and with\n"
        "  an URL that matches the TRAIL_URL argument.  If the working\n"
        "  copy is unusual the version number will be more complex:\n"
        "\n"
        "   4123:4168     mixed revision working copy\n"
        "   4168M         modified working copy\n"
        "   4123S         switched working copy\n"
        "   4123P         partial working copy, from a sparse checkout\n"
        "   4123:4168MS   mixed revision, modified, switched working copy\n"
        "\n"
        "  If WC_PATH is an unversioned path, the program will output\n"
        "  'Unversioned directory' or 'Unversioned file'.  If WC_PATH is\n"
        "  an added or copied or moved path, the program will output\n"
        "  'Uncommitted local addition, copy or move'.\n"
        "\n"
        "  If invoked without arguments WC_PATH will be the current directory.\n"
        "\n"
        "Valid options:\n")));
  while (options->description)
    {
      const char *optstr;
      svn_opt_format_option(&optstr, options, TRUE, pool);
      svn_error_clear(svn_cmdline_fprintf(stdout, pool, "  %s\n", optstr));
      ++options;
    }
  svn_error_clear(svn_cmdline_fprintf(stdout, pool, "\n"));
  exit(0);
}


/* Version compatibility check */
static svn_error_t *
check_lib_versions(void)
{
  static const svn_version_checklist_t checklist[] =
    {
      { "svn_subr",   svn_subr_version },
      { "svn_wc",     svn_wc_version },
      { NULL, NULL }
    };

  SVN_VERSION_DEFINE(my_version);
  return svn_ver_check_list(&my_version, checklist);
}

/*
 * Why is this not an svn subcommand?  I have this vague idea that it could
 * be run as part of the build process, with the output embedded in the svn
 * program.  Obviously we don't want to have to run svn when building svn.
 */
int
main(int argc, const char *argv[])
{
  const char *wc_path, *trail_url;
  const char *local_abspath;
  apr_allocator_t *allocator;
  apr_pool_t *pool;
  svn_wc_revision_status_t *res;
  svn_boolean_t no_newline = FALSE, committed = FALSE;
  svn_error_t *err;
  apr_getopt_t *os;
  svn_wc_context_t *wc_ctx;
  svn_boolean_t quiet = FALSE;
  svn_boolean_t is_version = FALSE;
  const apr_getopt_option_t options[] =
    {
      {"no-newline", 'n', 0, N_("do not output the trailing newline")},
      {"committed",  'c', 0, N_("last changed rather than current revisions")},
      {"help", 'h', 0, N_("display this help")},
      {"version", SVNVERSION_OPT_VERSION, 0,
       N_("show program version information")},
      {"quiet",         'q', 0,
       N_("no progress (only errors) to stderr")},
      {0,             0,  0,  0}
    };

  /* Initialize the app. */
  if (svn_cmdline_init("svnversion", stderr) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  /* Create our top-level pool.  Use a separate mutexless allocator,
   * given this application is single threaded.
   */
  if (apr_allocator_create(&allocator))
    return EXIT_FAILURE;

  apr_allocator_max_free_set(allocator, SVN_ALLOCATOR_RECOMMENDED_MAX_FREE);

  pool = svn_pool_create_ex(NULL, allocator);
  apr_allocator_owner_set(allocator, pool);

  /* Check library versions */
  err = check_lib_versions();
  if (err)
    return svn_cmdline_handle_exit_error(err, pool, "svnversion: ");

#if defined(WIN32) || defined(__CYGWIN__)
  /* Set the working copy administrative directory name. */
  if (getenv("SVN_ASP_DOT_NET_HACK"))
    {
      err = svn_wc_set_adm_dir("_svn", pool);
      if (err)
        return svn_cmdline_handle_exit_error(err, pool, "svnversion: ");
    }
#endif

  err = svn_cmdline__getopt_init(&os, argc, argv, pool);
  if (err)
    return svn_cmdline_handle_exit_error(err, pool, "svnversion: ");

  os->interleave = 1;
  while (1)
    {
      int opt;
      const char *arg;
      apr_status_t status = apr_getopt_long(os, options, &opt, &arg);
      if (APR_STATUS_IS_EOF(status))
        break;
      if (status != APR_SUCCESS)
        {
          usage(pool);
          return EXIT_FAILURE;
        }
      switch (opt)
        {
        case 'n':
          no_newline = TRUE;
          break;
        case 'c':
          committed = TRUE;
          break;
        case 'q':
          quiet = TRUE;
          break;
        case 'h':
          help(options, pool);
          break;
        case SVNVERSION_OPT_VERSION:
          is_version = TRUE;
          break;
        default:
          usage(pool);
          return EXIT_FAILURE;
        }
    }

  if (is_version)
    {
      SVN_INT_ERR(version(quiet, pool));
      exit(0);
    }
  if (os->ind > argc || os->ind < argc - 2)
    {
      usage(pool);
      return EXIT_FAILURE;
    }

  SVN_INT_ERR(svn_utf_cstring_to_utf8(&wc_path,
                                      (os->ind < argc) ? os->argv[os->ind] 
                                                       : ".",
                                      pool));

  SVN_INT_ERR(svn_opt__arg_canonicalize_path(&wc_path, wc_path, pool));
  SVN_INT_ERR(svn_dirent_get_absolute(&local_abspath, wc_path, pool));
  SVN_INT_ERR(svn_wc_context_create(&wc_ctx, NULL, pool, pool));

  if (os->ind+1 < argc)
    SVN_INT_ERR(svn_utf_cstring_to_utf8(&trail_url, os->argv[os->ind+1],
                                        pool));
  else
    trail_url = NULL;

  err = svn_wc_revision_status2(&res, wc_ctx, local_abspath, trail_url,
                                committed, NULL, NULL, pool, pool);

  if (err && (err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND
              || err->apr_err == SVN_ERR_WC_NOT_WORKING_COPY))
    {
      svn_node_kind_t kind;
      svn_boolean_t special;

      svn_error_clear(err);

      SVN_INT_ERR(svn_io_check_special_path(local_abspath, &kind, &special,
                                            pool));

      if (special)
        SVN_INT_ERR(svn_cmdline_printf(pool, _("Unversioned symlink%s"),
                                       no_newline ? "" : "\n"));
      else if (kind == svn_node_dir)
        SVN_INT_ERR(svn_cmdline_printf(pool, _("Unversioned directory%s"),
                                       no_newline ? "" : "\n"));
      else if (kind == svn_node_file)
        SVN_INT_ERR(svn_cmdline_printf(pool, _("Unversioned file%s"),
                                       no_newline ? "" : "\n"));
      else
        {
          SVN_INT_ERR(svn_cmdline_fprintf(stderr, pool,
                                          kind == svn_node_none
                                           ? _("'%s' doesn't exist\n")
                                           : _("'%s' is of unknown type\n"),
                                          svn_dirent_local_style(local_abspath,
                                                                 pool)));
          svn_pool_destroy(pool);
          return EXIT_FAILURE;
        }
      svn_pool_destroy(pool);
      return EXIT_SUCCESS;
    }

  SVN_INT_ERR(err);

  if (! SVN_IS_VALID_REVNUM(res->min_rev))
    {
      /* Local uncommitted modifications, no revision info was found. */
      SVN_INT_ERR(svn_cmdline_printf(pool, _("Uncommitted local addition, "
                                             "copy or move%s"),
                                             no_newline ? "" : "\n"));
      svn_pool_destroy(pool);
      return EXIT_SUCCESS;
    }

  /* Build compact '123[:456]M?S?' string. */
  SVN_INT_ERR(svn_cmdline_printf(pool, "%ld", res->min_rev));
  if (res->min_rev != res->max_rev)
    SVN_INT_ERR(svn_cmdline_printf(pool, ":%ld", res->max_rev));
  if (res->modified)
    SVN_INT_ERR(svn_cmdline_fputs("M", stdout, pool));
  if (res->switched)
    SVN_INT_ERR(svn_cmdline_fputs("S", stdout, pool));
  if (res->sparse_checkout)
    SVN_INT_ERR(svn_cmdline_fputs("P", stdout, pool));

  if (! no_newline)
    SVN_INT_ERR(svn_cmdline_fputs("\n", stdout, pool));

  svn_pool_destroy(pool);

  /* Flush stdout to make sure that the user will see any printing errors. */
  SVN_INT_ERR(svn_cmdline_fflush(stdout));

  return EXIT_SUCCESS;
}

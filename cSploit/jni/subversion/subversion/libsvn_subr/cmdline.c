/*
 * cmdline.c :  Helpers for command-line programs.
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


#include <stdlib.h>             /* for atexit() */
#include <stdio.h>              /* for setvbuf() */
#include <locale.h>             /* for setlocale() */

#ifndef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#else
#include <crtdbg.h>
#endif

#include <apr_errno.h>          /* for apr_strerror */
#include <apr_general.h>        /* for apr_initialize/apr_terminate */
#include <apr_atomic.h>         /* for apr_atomic_init */
#include <apr_strings.h>        /* for apr_snprintf */
#include <apr_pools.h>

#include "svn_cmdline.h"
#include "svn_dso.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_pools.h"
#include "svn_error.h"
#include "svn_nls.h"
#include "svn_utf.h"
#include "svn_auth.h"
#include "svn_xml.h"
#include "svn_base64.h"
#include "svn_config.h"

#include "private/svn_cmdline_private.h"
#include "private/svn_utf_private.h"

#include "svn_private_config.h"

#include "win32_crashrpt.h"

/* The stdin encoding. If null, it's the same as the native encoding. */
static const char *input_encoding = NULL;

/* The stdout encoding. If null, it's the same as the native encoding. */
static const char *output_encoding = NULL;


int
svn_cmdline_init(const char *progname, FILE *error_stream)
{
  apr_status_t status;
  apr_pool_t *pool;
  svn_error_t *err;

#ifndef WIN32
  {
    struct stat st;

    /* The following makes sure that file descriptors 0 (stdin), 1
       (stdout) and 2 (stderr) will not be "reused", because if
       e.g. file descriptor 2 would be reused when opening a file, a
       write to stderr would write to that file and most likely
       corrupt it. */
    if ((fstat(0, &st) == -1 && open("/dev/null", O_RDONLY) == -1) ||
        (fstat(1, &st) == -1 && open("/dev/null", O_WRONLY) == -1) ||
        (fstat(2, &st) == -1 && open("/dev/null", O_WRONLY) == -1))
      {
        if (error_stream)
          fprintf(error_stream, "%s: error: cannot open '/dev/null'\n",
                  progname);
        return EXIT_FAILURE;
      }
  }
#endif

  /* Ignore any errors encountered while attempting to change stream
     buffering, as the streams should retain their default buffering
     modes. */
  if (error_stream)
    setvbuf(error_stream, NULL, _IONBF, 0);
#ifndef WIN32
  setvbuf(stdout, NULL, _IOLBF, 0);
#endif

#ifdef WIN32
#if _MSC_VER < 1400
  /* Initialize the input and output encodings. */
  {
    static char input_encoding_buffer[16];
    static char output_encoding_buffer[16];

    apr_snprintf(input_encoding_buffer, sizeof input_encoding_buffer,
                 "CP%u", (unsigned) GetConsoleCP());
    input_encoding = input_encoding_buffer;

    apr_snprintf(output_encoding_buffer, sizeof output_encoding_buffer,
                 "CP%u", (unsigned) GetConsoleOutputCP());
    output_encoding = output_encoding_buffer;
  }
#endif /* _MSC_VER < 1400 */

#ifdef SVN_USE_WIN32_CRASHHANDLER
  /* Attach (but don't load) the crash handler */
  SetUnhandledExceptionFilter(svn__unhandled_exception_filter);

#if _MSC_VER >= 1400
  /* ### This should work for VC++ 2002 (=1300) and later */
  /* Show the abort message on STDERR instead of a dialog to allow
     scripts (e.g. our testsuite) to continue after an abort without
     user intervention. Allow overriding for easier debugging. */
  if (!getenv("SVN_CMDLINE_USE_DIALOG_FOR_ABORT"))
    {
      /* In release mode: Redirect abort() errors to stderr */
      _set_error_mode(_OUT_TO_STDERR);

      /* In _DEBUG mode: Redirect all debug output (E.g. assert() to stderr.
         (Ignored in releas builds) */
      _CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR);
      _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
      _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
      _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    }
#endif /* _MSC_VER >= 1400 */

#endif /* SVN_USE_WIN32_CRASHHANDLER */

#endif /* WIN32 */

  /* C programs default to the "C" locale. But because svn is supposed
     to be i18n-aware, it should inherit the default locale of its
     environment.  */
  if (!setlocale(LC_ALL, "")
      && !setlocale(LC_CTYPE, ""))
    {
      if (error_stream)
        {
          const char *env_vars[] = { "LC_ALL", "LC_CTYPE", "LANG", NULL };
          const char **env_var = &env_vars[0], *env_val = NULL;
          while (*env_var)
            {
              env_val = getenv(*env_var);
              if (env_val && env_val[0])
                break;
              ++env_var;
            }

          if (!*env_var)
            {
              /* Unlikely. Can setlocale fail if no env vars are set? */
              --env_var;
              env_val = "not set";
            }

          fprintf(error_stream,
                  "%s: warning: cannot set LC_CTYPE locale\n"
                  "%s: warning: environment variable %s is %s\n"
                  "%s: warning: please check that your locale name is correct\n",
                  progname, progname, *env_var, env_val, progname);
        }
    }

  /* Initialize the APR subsystem, and register an atexit() function
     to Uninitialize that subsystem at program exit. */
  status = apr_initialize();
  if (status)
    {
      if (error_stream)
        {
          char buf[1024];
          apr_strerror(status, buf, sizeof(buf) - 1);
          fprintf(error_stream,
                  "%s: error: cannot initialize APR: %s\n",
                  progname, buf);
        }
      return EXIT_FAILURE;
    }

  /* This has to happen before any pools are created. */
  if ((err = svn_dso_initialize2()))
    {
      if (error_stream && err->message)
        fprintf(error_stream, "%s", err->message);

      svn_error_clear(err);
      return EXIT_FAILURE;
    }

  if (0 > atexit(apr_terminate))
    {
      if (error_stream)
        fprintf(error_stream,
                "%s: error: atexit registration failed\n",
                progname);
      return EXIT_FAILURE;
    }

  /* Create a pool for use by the UTF-8 routines.  It will be cleaned
     up by APR at exit time. */
  pool = svn_pool_create(NULL);
  svn_utf_initialize(pool);

  if ((err = svn_nls_init()))
    {
      if (error_stream && err->message)
        fprintf(error_stream, "%s", err->message);

      svn_error_clear(err);
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}


svn_error_t *
svn_cmdline_cstring_from_utf8(const char **dest,
                              const char *src,
                              apr_pool_t *pool)
{
  if (output_encoding == NULL)
    return svn_utf_cstring_from_utf8(dest, src, pool);
  else
    return svn_utf_cstring_from_utf8_ex2(dest, src, output_encoding, pool);
}


const char *
svn_cmdline_cstring_from_utf8_fuzzy(const char *src,
                                    apr_pool_t *pool)
{
  return svn_utf__cstring_from_utf8_fuzzy(src, pool,
                                          svn_cmdline_cstring_from_utf8);
}


svn_error_t *
svn_cmdline_cstring_to_utf8(const char **dest,
                            const char *src,
                            apr_pool_t *pool)
{
  if (input_encoding == NULL)
    return svn_utf_cstring_to_utf8(dest, src, pool);
  else
    return svn_utf_cstring_to_utf8_ex2(dest, src, input_encoding, pool);
}


svn_error_t *
svn_cmdline_path_local_style_from_utf8(const char **dest,
                                       const char *src,
                                       apr_pool_t *pool)
{
  return svn_cmdline_cstring_from_utf8(dest,
                                       svn_dirent_local_style(src, pool),
                                       pool);
}

svn_error_t *
svn_cmdline_printf(apr_pool_t *pool, const char *fmt, ...)
{
  const char *message;
  va_list ap;

  /* A note about encoding issues:
   * APR uses the execution character set, but here we give it UTF-8 strings,
   * both the fmt argument and any other string arguments.  Since apr_pvsprintf
   * only cares about and produces ASCII characters, this works under the
   * assumption that all supported platforms use an execution character set
   * with ASCII as a subset.
   */

  va_start(ap, fmt);
  message = apr_pvsprintf(pool, fmt, ap);
  va_end(ap);

  return svn_cmdline_fputs(message, stdout, pool);
}

svn_error_t *
svn_cmdline_fprintf(FILE *stream, apr_pool_t *pool, const char *fmt, ...)
{
  const char *message;
  va_list ap;

  /* See svn_cmdline_printf () for a note about character encoding issues. */

  va_start(ap, fmt);
  message = apr_pvsprintf(pool, fmt, ap);
  va_end(ap);

  return svn_cmdline_fputs(message, stream, pool);
}

svn_error_t *
svn_cmdline_fputs(const char *string, FILE* stream, apr_pool_t *pool)
{
  svn_error_t *err;
  const char *out;

  err = svn_cmdline_cstring_from_utf8(&out, string, pool);

  if (err)
    {
      svn_error_clear(err);
      out = svn_cmdline_cstring_from_utf8_fuzzy(string, pool);
    }

  /* On POSIX systems, errno will be set on an error in fputs, but this might
     not be the case on other platforms.  We reset errno and only
     use it if it was set by the below fputs call.  Else, we just return
     a generic error. */
  errno = 0;

  if (fputs(out, stream) == EOF)
    {
      if (apr_get_os_error()) /* is errno on POSIX */
        {
          /* ### Issue #3014: Return a specific error for broken pipes,
           * ### with a single element in the error chain. */
          if (APR_STATUS_IS_EPIPE(apr_get_os_error()))
            return svn_error_create(SVN_ERR_IO_PIPE_WRITE_ERROR, NULL, NULL);
          else
            return svn_error_wrap_apr(apr_get_os_error(), _("Write error"));
        }
      else
        return svn_error_create(SVN_ERR_IO_WRITE_ERROR, NULL, NULL);
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_cmdline_fflush(FILE *stream)
{
  /* See comment in svn_cmdline_fputs about use of errno and stdio. */
  errno = 0;
  if (fflush(stream) == EOF)
    {
      if (apr_get_os_error()) /* is errno on POSIX */
        {
          /* ### Issue #3014: Return a specific error for broken pipes,
           * ### with a single element in the error chain. */
          if (APR_STATUS_IS_EPIPE(apr_get_os_error()))
            return svn_error_create(SVN_ERR_IO_PIPE_WRITE_ERROR, NULL, NULL);
          else
            return svn_error_wrap_apr(apr_get_os_error(), _("Write error"));
        }
      else
        return svn_error_create(SVN_ERR_IO_WRITE_ERROR, NULL, NULL);
    }

  return SVN_NO_ERROR;
}

const char *svn_cmdline_output_encoding(apr_pool_t *pool)
{
  if (output_encoding)
    return apr_pstrdup(pool, output_encoding);
  else
    return SVN_APR_LOCALE_CHARSET;
}

int
svn_cmdline_handle_exit_error(svn_error_t *err,
                              apr_pool_t *pool,
                              const char *prefix)
{
  /* Issue #3014:
   * Don't print anything on broken pipes. The pipe was likely
   * closed by the process at the other end. We expect that
   * process to perform error reporting as necessary.
   *
   * ### This assumes that there is only one error in a chain for
   * ### SVN_ERR_IO_PIPE_WRITE_ERROR. See svn_cmdline_fputs(). */
  if (err->apr_err != SVN_ERR_IO_PIPE_WRITE_ERROR)
    svn_handle_error2(err, stderr, FALSE, prefix);
  svn_error_clear(err);
  if (pool)
    svn_pool_destroy(pool);
  return EXIT_FAILURE;
}

/* This implements 'svn_auth_ssl_server_trust_prompt_func_t'.

   Don't actually prompt.  Instead, set *CRED_P to valid credentials
   iff FAILURES is empty or is exactly SVN_AUTH_SSL_UNKNOWNCA.  If
   there are any other failure bits, then set *CRED_P to null (that
   is, reject the cert).

   Ignore MAY_SAVE; we don't save certs we never prompted for.

   Ignore BATON, REALM, and CERT_INFO,

   Ignore any further films by George Lucas. */
static svn_error_t *
ssl_trust_unknown_server_cert
  (svn_auth_cred_ssl_server_trust_t **cred_p,
   void *baton,
   const char *realm,
   apr_uint32_t failures,
   const svn_auth_ssl_server_cert_info_t *cert_info,
   svn_boolean_t may_save,
   apr_pool_t *pool)
{
  *cred_p = NULL;

  if (failures == 0 || failures == SVN_AUTH_SSL_UNKNOWNCA)
    {
      *cred_p = apr_pcalloc(pool, sizeof(**cred_p));
      (*cred_p)->may_save = FALSE;
      (*cred_p)->accepted_failures = failures;
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_cmdline_create_auth_baton(svn_auth_baton_t **ab,
                              svn_boolean_t non_interactive,
                              const char *auth_username,
                              const char *auth_password,
                              const char *config_dir,
                              svn_boolean_t no_auth_cache,
                              svn_boolean_t trust_server_cert,
                              svn_config_t *cfg,
                              svn_cancel_func_t cancel_func,
                              void *cancel_baton,
                              apr_pool_t *pool)
{
  svn_boolean_t store_password_val = TRUE;
  svn_boolean_t store_auth_creds_val = TRUE;
  svn_auth_provider_object_t *provider;
  svn_cmdline_prompt_baton2_t *pb = NULL;

  /* The whole list of registered providers */
  apr_array_header_t *providers;

  /* Populate the registered providers with the platform-specific providers */
  SVN_ERR(svn_auth_get_platform_specific_client_providers
            (&providers, cfg, pool));

  /* If we have a cancellation function, cram it and the stuff it
     needs into the prompt baton. */
  if (cancel_func)
    {
      pb = apr_palloc(pool, sizeof(*pb));
      pb->cancel_func = cancel_func;
      pb->cancel_baton = cancel_baton;
      pb->config_dir = config_dir;
    }

  if (non_interactive == FALSE)
    {
      /* This provider doesn't prompt the user in order to get creds;
         it prompts the user regarding the caching of creds. */
      svn_auth_get_simple_provider2(&provider,
                                    svn_cmdline_auth_plaintext_prompt,
                                    pb, pool);
    }
  else
    {
      svn_auth_get_simple_provider2(&provider, NULL, NULL, pool);
    }

  APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;
  svn_auth_get_username_provider(&provider, pool);
  APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

  /* The server-cert, client-cert, and client-cert-password providers. */
  SVN_ERR(svn_auth_get_platform_specific_provider(&provider,
                                                  "windows",
                                                  "ssl_server_trust",
                                                  pool));

  if (provider)
    APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

  svn_auth_get_ssl_server_trust_file_provider(&provider, pool);
  APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;
  svn_auth_get_ssl_client_cert_file_provider(&provider, pool);
  APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

  if (non_interactive == FALSE)
    {
      /* This provider doesn't prompt the user in order to get creds;
         it prompts the user regarding the caching of creds. */
      svn_auth_get_ssl_client_cert_pw_file_provider2
        (&provider, svn_cmdline_auth_plaintext_passphrase_prompt,
         pb, pool);
    }
  else
    {
      svn_auth_get_ssl_client_cert_pw_file_provider2(&provider, NULL, NULL,
                                                     pool);
    }
  APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

  if (non_interactive == FALSE)
    {
      /* Two basic prompt providers: username/password, and just username. */
      svn_auth_get_simple_prompt_provider(&provider,
                                          svn_cmdline_auth_simple_prompt,
                                          pb,
                                          2, /* retry limit */
                                          pool);
      APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

      svn_auth_get_username_prompt_provider
        (&provider, svn_cmdline_auth_username_prompt, pb,
         2, /* retry limit */ pool);
      APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

      /* Three ssl prompt providers, for server-certs, client-certs,
         and client-cert-passphrases.  */
      svn_auth_get_ssl_server_trust_prompt_provider
        (&provider, svn_cmdline_auth_ssl_server_trust_prompt, pb, pool);
      APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

      svn_auth_get_ssl_client_cert_prompt_provider
        (&provider, svn_cmdline_auth_ssl_client_cert_prompt, pb, 2, pool);
      APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

      svn_auth_get_ssl_client_cert_pw_prompt_provider
        (&provider, svn_cmdline_auth_ssl_client_cert_pw_prompt, pb, 2, pool);
      APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;
    }
  else if (trust_server_cert)
    {
      /* Remember, only register this provider if non_interactive. */
      svn_auth_get_ssl_server_trust_prompt_provider
        (&provider, ssl_trust_unknown_server_cert, NULL, pool);
      APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;
    }

  /* Build an authentication baton to give to libsvn_client. */
  svn_auth_open(ab, providers, pool);

  /* Place any default --username or --password credentials into the
     auth_baton's run-time parameter hash. */
  if (auth_username)
    svn_auth_set_parameter(*ab, SVN_AUTH_PARAM_DEFAULT_USERNAME,
                           auth_username);
  if (auth_password)
    svn_auth_set_parameter(*ab, SVN_AUTH_PARAM_DEFAULT_PASSWORD,
                           auth_password);

  /* Same with the --non-interactive option. */
  if (non_interactive)
    svn_auth_set_parameter(*ab, SVN_AUTH_PARAM_NON_INTERACTIVE, "");

  if (config_dir)
    svn_auth_set_parameter(*ab, SVN_AUTH_PARAM_CONFIG_DIR,
                           config_dir);

  /* Determine whether storing passwords in any form is allowed.
   * This is the deprecated location for this option, the new
   * location is SVN_CONFIG_CATEGORY_SERVERS. The RA layer may
   * override the value we set here. */
  SVN_ERR(svn_config_get_bool(cfg, &store_password_val,
                              SVN_CONFIG_SECTION_AUTH,
                              SVN_CONFIG_OPTION_STORE_PASSWORDS,
                              SVN_CONFIG_DEFAULT_OPTION_STORE_PASSWORDS));

  if (! store_password_val)
    svn_auth_set_parameter(*ab, SVN_AUTH_PARAM_DONT_STORE_PASSWORDS, "");

  /* Determine whether we are allowed to write to the auth/ area.
   * This is the deprecated location for this option, the new
   * location is SVN_CONFIG_CATEGORY_SERVERS. The RA layer may
   * override the value we set here. */
  SVN_ERR(svn_config_get_bool(cfg, &store_auth_creds_val,
                              SVN_CONFIG_SECTION_AUTH,
                              SVN_CONFIG_OPTION_STORE_AUTH_CREDS,
                              SVN_CONFIG_DEFAULT_OPTION_STORE_AUTH_CREDS));

  if (no_auth_cache || ! store_auth_creds_val)
    svn_auth_set_parameter(*ab, SVN_AUTH_PARAM_NO_AUTH_CACHE, "");

#ifdef SVN_HAVE_GNOME_KEYRING
  svn_auth_set_parameter(*ab, SVN_AUTH_PARAM_GNOME_KEYRING_UNLOCK_PROMPT_FUNC,
                         &svn_cmdline__auth_gnome_keyring_unlock_prompt);
#endif /* SVN_HAVE_GNOME_KEYRING */

  return SVN_NO_ERROR;
}

svn_error_t *
svn_cmdline__getopt_init(apr_getopt_t **os,
                         int argc,
                         const char *argv[],
                         apr_pool_t *pool)
{
  apr_status_t apr_err = apr_getopt_init(os, pool, argc, argv);
  if (apr_err)
    return svn_error_wrap_apr(apr_err,
                              _("Error initializing command line arguments"));
  return SVN_NO_ERROR;
}


void
svn_cmdline__print_xml_prop(svn_stringbuf_t **outstr,
                            const char* propname,
                            svn_string_t *propval,
                            apr_pool_t *pool)
{
  const char *xml_safe;
  const char *encoding = NULL;

  if (*outstr == NULL)
    *outstr = svn_stringbuf_create("", pool);

  if (svn_xml_is_xml_safe(propval->data, propval->len))
    {
      svn_stringbuf_t *xml_esc = NULL;
      svn_xml_escape_cdata_string(&xml_esc, propval, pool);
      xml_safe = xml_esc->data;
    }
  else
    {
      const svn_string_t *base64ed = svn_base64_encode_string2(propval, TRUE,
                                                               pool);
      encoding = "base64";
      xml_safe = base64ed->data;
    }

  if (encoding)
    svn_xml_make_open_tag(outstr, pool, svn_xml_protect_pcdata,
                          "property", "name", propname,
                          "encoding", encoding, NULL);
  else
    svn_xml_make_open_tag(outstr, pool, svn_xml_protect_pcdata,
                          "property", "name", propname, NULL);

  svn_stringbuf_appendcstr(*outstr, xml_safe);

  svn_xml_make_close_tag(outstr, pool, "property");

  return;
}

svn_error_t *
svn_cmdline__parse_config_option(apr_array_header_t *config_options,
                                 const char *opt_arg,
                                 apr_pool_t *pool)
{
  svn_cmdline__config_argument_t *config_option;
  const char *first_colon, *second_colon, *equals_sign;
  apr_size_t len = strlen(opt_arg);
  if ((first_colon = strchr(opt_arg, ':')) && (first_colon != opt_arg))
    {
      if ((second_colon = strchr(first_colon + 1, ':')) &&
          (second_colon != first_colon + 1))
        {
          if ((equals_sign = strchr(second_colon + 1, '=')) &&
              (equals_sign != second_colon + 1))
            {
              config_option = apr_pcalloc(pool, sizeof(*config_option));
              config_option->file = apr_pstrndup(pool, opt_arg,
                                                 first_colon - opt_arg);
              config_option->section = apr_pstrndup(pool, first_colon + 1,
                                                    second_colon - first_colon - 1);
              config_option->option = apr_pstrndup(pool, second_colon + 1,
                                                   equals_sign - second_colon - 1);

              if (! (strchr(config_option->option, ':')))
                {
                  config_option->value = apr_pstrndup(pool, equals_sign + 1,
                                                      opt_arg + len - equals_sign - 1);
                  APR_ARRAY_PUSH(config_options, svn_cmdline__config_argument_t *)
                                       = config_option;
                  return SVN_NO_ERROR;
                }
            }
        }
    }
  return svn_error_create(SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                          _("Invalid syntax of argument of --config-option"));
}

svn_error_t *
svn_cmdline__apply_config_options(apr_hash_t *config,
                                  const apr_array_header_t *config_options,
                                  const char *prefix,
                                  const char *argument_name)
{
  int i;

  for (i = 0; i < config_options->nelts; i++)
   {
     svn_config_t *cfg;
     svn_cmdline__config_argument_t *arg =
                          APR_ARRAY_IDX(config_options, i,
                                        svn_cmdline__config_argument_t *);

     cfg = apr_hash_get(config, arg->file, APR_HASH_KEY_STRING);

     if (cfg)
       {
         svn_config_set(cfg, arg->section, arg->option, arg->value);
       }
     else
       {
         svn_error_t *err = svn_error_createf(SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
             _("Unrecognized file in argument of %s"), argument_name);

         svn_handle_warning2(stderr, err, prefix);
         svn_error_clear(err);
       }
    }

  return SVN_NO_ERROR;
}

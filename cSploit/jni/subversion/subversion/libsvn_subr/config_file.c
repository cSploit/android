/*
 * config_file.c :  parsing configuration files
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



#include <apr_lib.h>
#include <apr_env.h>
#include "config_impl.h"
#include "svn_io.h"
#include "svn_types.h"
#include "svn_dirent_uri.h"
#include "svn_auth.h"
#include "svn_subst.h"
#include "svn_utf.h"
#include "svn_pools.h"
#include "svn_user.h"
#include "svn_ctype.h"

#include "svn_private_config.h"

#ifdef __HAIKU__
#  include <FindDirectory.h>
#  include <StorageDefs.h>
#endif

/* Used to terminate lines in large multi-line string literals. */
#define NL APR_EOL_STR


/* File parsing context */
typedef struct parse_context_t
{
  /* This config struct and file */
  svn_config_t *cfg;
  const char *file;

  /* The file descriptor */
  svn_stream_t *stream;

  /* The current line in the file */
  int line;

  /* Cached ungotten character  - streams don't support ungetc()
     [emulate it] */
  int ungotten_char;
  svn_boolean_t have_ungotten_char;

  /* Temporary strings, allocated from the temp pool */
  svn_stringbuf_t *section;
  svn_stringbuf_t *option;
  svn_stringbuf_t *value;
} parse_context_t;



/* Emulate getc() because streams don't support it.
 *
 * In order to be able to ungetc(), use the CXT instead of the stream
 * to be able to store the 'ungotton' character.
 *
 */
static APR_INLINE svn_error_t *
parser_getc(parse_context_t *ctx, int *c)
{
  if (ctx->have_ungotten_char)
    {
      *c = ctx->ungotten_char;
      ctx->have_ungotten_char = FALSE;
    }
  else
    {
      char char_buf;
      apr_size_t readlen = 1;

      SVN_ERR(svn_stream_read(ctx->stream, &char_buf, &readlen));

      if (readlen == 1)
        *c = char_buf;
      else
        *c = EOF;
    }

  return SVN_NO_ERROR;
}

/* Emulate ungetc() because streams don't support it.
 *
 * Use CTX to store the ungotten character C.
 */
static APR_INLINE svn_error_t *
parser_ungetc(parse_context_t *ctx, int c)
{
  ctx->ungotten_char = c;
  ctx->have_ungotten_char = TRUE;

  return SVN_NO_ERROR;
}

/* Eat chars from STREAM until encounter non-whitespace, newline, or EOF.
   Set *PCOUNT to the number of characters eaten, not counting the
   last one, and return the last char read (the one that caused the
   break).  */
static APR_INLINE svn_error_t *
skip_whitespace(parse_context_t *ctx, int *c, int *pcount)
{
  int ch;
  int count = 0;

  SVN_ERR(parser_getc(ctx, &ch));
  while (ch != EOF && ch != '\n' && svn_ctype_isspace(ch))
    {
      ++count;
      SVN_ERR(parser_getc(ctx, &ch));
    }
  *pcount = count;
  *c = ch;
  return SVN_NO_ERROR;
}


/* Skip to the end of the line (or file).  Returns the char that ended
   the line; the char is either EOF or newline. */
static APR_INLINE svn_error_t *
skip_to_eoln(parse_context_t *ctx, int *c)
{
  int ch;

  SVN_ERR(parser_getc(ctx, &ch));
  while (ch != EOF && ch != '\n')
    SVN_ERR(parser_getc(ctx, &ch));

  *c = ch;
  return SVN_NO_ERROR;
}


/* Parse a single option value */
static svn_error_t *
parse_value(int *pch, parse_context_t *ctx)
{
  svn_boolean_t end_of_val = FALSE;
  int ch;

  /* Read the first line of the value */
  svn_stringbuf_setempty(ctx->value);
  SVN_ERR(parser_getc(ctx, &ch));
  while (ch != EOF && ch != '\n')
    /* last ch seen was ':' or '=' in parse_option. */
    {
      const char char_from_int = (char)ch;
      svn_stringbuf_appendbyte(ctx->value, char_from_int);
      SVN_ERR(parser_getc(ctx, &ch));
    }
  /* Leading and trailing whitespace is ignored. */
  svn_stringbuf_strip_whitespace(ctx->value);

  /* Look for any continuation lines. */
  for (;;)
    {

      if (ch == EOF || end_of_val)
        {
          /* At end of file. The value is complete, there can't be
             any continuation lines. */
          svn_config_set(ctx->cfg, ctx->section->data,
                         ctx->option->data, ctx->value->data);
          break;
        }
      else
        {
          int count;
          ++ctx->line;
          SVN_ERR(skip_whitespace(ctx, &ch, &count));

          switch (ch)
            {
            case '\n':
              /* The next line was empty. Ergo, it can't be a
                 continuation line. */
              ++ctx->line;
              end_of_val = TRUE;
              continue;

            case EOF:
              /* This is also an empty line. */
              end_of_val = TRUE;
              continue;

            default:
              if (count == 0)
                {
                  /* This line starts in the first column.  That means
                     it's either a section, option or comment.  Put
                     the char back into the stream, because it doesn't
                     belong to us. */
                  SVN_ERR(parser_ungetc(ctx, ch));
                  end_of_val = TRUE;
                }
              else
                {
                  /* This is a continuation line. Read it. */
                  svn_stringbuf_appendbyte(ctx->value, ' ');

                  while (ch != EOF && ch != '\n')
                    {
                      const char char_from_int = (char)ch;
                      svn_stringbuf_appendbyte(ctx->value, char_from_int);
                      SVN_ERR(parser_getc(ctx, &ch));
                    }
                  /* Trailing whitespace is ignored. */
                  svn_stringbuf_strip_whitespace(ctx->value);
                }
            }
        }
    }

  *pch = ch;
  return SVN_NO_ERROR;
}


/* Parse a single option */
static svn_error_t *
parse_option(int *pch, parse_context_t *ctx, apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  int ch;

  svn_stringbuf_setempty(ctx->option);
  ch = *pch;   /* Yes, the first char is relevant. */
  while (ch != EOF && ch != ':' && ch != '=' && ch != '\n')
    {
      const char char_from_int = (char)ch;
      svn_stringbuf_appendbyte(ctx->option, char_from_int);
      SVN_ERR(parser_getc(ctx, &ch));
    }

  if (ch != ':' && ch != '=')
    {
      ch = EOF;
      err = svn_error_createf(SVN_ERR_MALFORMED_FILE, NULL,
                              "%s:%d: Option must end with ':' or '='",
                              svn_dirent_local_style(ctx->file, pool),
                              ctx->line);
    }
  else
    {
      /* Whitespace around the name separator is ignored. */
      svn_stringbuf_strip_whitespace(ctx->option);
      err = parse_value(&ch, ctx);
    }

  *pch = ch;
  return err;
}


/* Read chars until enounter ']', then skip everything to the end of
 * the line.  Set *PCH to the character that ended the line (either
 * newline or EOF), and set CTX->section to the string of characters
 * seen before ']'.
 *
 * This is meant to be called immediately after reading the '[' that
 * starts a section name.
 */
static svn_error_t *
parse_section_name(int *pch, parse_context_t *ctx, apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  int ch;

  svn_stringbuf_setempty(ctx->section);
  SVN_ERR(parser_getc(ctx, &ch));
  while (ch != EOF && ch != ']' && ch != '\n')
    {
      const char char_from_int = (char)ch;
      svn_stringbuf_appendbyte(ctx->section, char_from_int);
      SVN_ERR(parser_getc(ctx, &ch));
    }

  if (ch != ']')
    {
      ch = EOF;
      err = svn_error_createf(SVN_ERR_MALFORMED_FILE, NULL,
                              "%s:%d: Section header must end with ']'",
                              svn_dirent_local_style(ctx->file, pool),
                              ctx->line);
    }
  else
    {
      /* Everything from the ']' to the end of the line is ignored. */
      SVN_ERR(skip_to_eoln(ctx, &ch));
      if (ch != EOF)
        ++ctx->line;
    }

  *pch = ch;
  return err;
}


svn_error_t *
svn_config__sys_config_path(const char **path_p,
                            const char *fname,
                            apr_pool_t *pool)
{
  *path_p = NULL;

  /* Note that even if fname is null, svn_dirent_join_many will DTRT. */

#ifdef WIN32
  {
    const char *folder;
    SVN_ERR(svn_config__win_config_path(&folder, TRUE, pool));
    *path_p = svn_dirent_join_many(pool, folder,
                                   SVN_CONFIG__SUBDIRECTORY, fname, NULL);
  }

#elif defined(__HAIKU__)
  {
    char folder[B_PATH_NAME_LENGTH];

    status_t error = find_directory(B_COMMON_SETTINGS_DIRECTORY, -1, false,
                                    folder, sizeof(folder));
    if (error)
      return SVN_NO_ERROR;

    *path_p = svn_dirent_join_many(pool, folder,
                                   SVN_CONFIG__SYS_DIRECTORY, fname, NULL);
  }
#else  /* ! WIN32 && !__HAIKU__ */

  *path_p = svn_dirent_join_many(pool, SVN_CONFIG__SYS_DIRECTORY, fname, NULL);

#endif /* WIN32 */

  return SVN_NO_ERROR;
}


/*** Exported interfaces. ***/


svn_error_t *
svn_config__parse_file(svn_config_t *cfg, const char *file,
                       svn_boolean_t must_exist, apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  parse_context_t ctx;
  int ch, count;
  svn_stream_t *stream;

  err = svn_stream_open_readonly(&stream, file, pool, pool);

  if (! must_exist && err && APR_STATUS_IS_ENOENT(err->apr_err))
    {
      svn_error_clear(err);
      return SVN_NO_ERROR;
    }
  else
    SVN_ERR(err);

  ctx.cfg = cfg;
  ctx.file = file;
  ctx.stream = svn_subst_stream_translated(stream, "\n", TRUE, NULL, FALSE,
                                           pool);
  ctx.line = 1;
  ctx.have_ungotten_char = FALSE;
  ctx.section = svn_stringbuf_create("", pool);
  ctx.option = svn_stringbuf_create("", pool);
  ctx.value = svn_stringbuf_create("", pool);

  do
    {
      SVN_ERR(skip_whitespace(&ctx, &ch, &count));

      switch (ch)
        {
        case '[':               /* Start of section header */
          if (count == 0)
            SVN_ERR(parse_section_name(&ch, &ctx, pool));
          else
            return svn_error_createf(SVN_ERR_MALFORMED_FILE, NULL,
                                     "%s:%d: Section header"
                                     " must start in the first column",
                                     svn_dirent_local_style(file, pool),
                                     ctx.line);
          break;

        case '#':               /* Comment */
          if (count == 0)
            {
              SVN_ERR(skip_to_eoln(&ctx, &ch));
              ++ctx.line;
            }
          else
            return svn_error_createf(SVN_ERR_MALFORMED_FILE, NULL,
                                     "%s:%d: Comment"
                                     " must start in the first column",
                                     svn_dirent_local_style(file, pool),
                                     ctx.line);
          break;

        case '\n':              /* Empty line */
          ++ctx.line;
          break;

        case EOF:               /* End of file or read error */
          break;

        default:
          if (svn_stringbuf_isempty(ctx.section))
            return svn_error_createf(SVN_ERR_MALFORMED_FILE, NULL,
                                     "%s:%d: Section header expected",
                                     svn_dirent_local_style(file, pool),
                                     ctx.line);
          else if (count != 0)
            return svn_error_createf(SVN_ERR_MALFORMED_FILE, NULL,
                                     "%s:%d: Option expected",
                                     svn_dirent_local_style(file, pool),
                                     ctx.line);
          else
            SVN_ERR(parse_option(&ch, &ctx, pool));
          break;
        }
    }
  while (ch != EOF);

  /* Close the streams (and other cleanup): */
  return svn_stream_close(ctx.stream);
}


/* Helper for ensure_auth_dirs: create SUBDIR under AUTH_DIR, iff
   SUBDIR does not already exist, but ignore any errors.  Use POOL for
   temporary allocation. */
static void
ensure_auth_subdir(const char *auth_dir,
                   const char *subdir,
                   apr_pool_t *pool)
{
  svn_error_t *err;
  const char *subdir_full_path;
  svn_node_kind_t kind;

  subdir_full_path = svn_dirent_join(auth_dir, subdir, pool);
  err = svn_io_check_path(subdir_full_path, &kind, pool);
  if (err || kind == svn_node_none)
    {
      svn_error_clear(err);
      svn_error_clear(svn_io_dir_make(subdir_full_path, APR_OS_DEFAULT, pool));
    }
}

/* Helper for svn_config_ensure:  see if ~/.subversion/auth/ and its
   subdirs exist, try to create them, but don't throw errors on
   failure.  PATH is assumed to be a path to the user's private config
   directory. */
static void
ensure_auth_dirs(const char *path,
                 apr_pool_t *pool)
{
  svn_node_kind_t kind;
  const char *auth_dir;
  svn_error_t *err;

  /* Ensure ~/.subversion/auth/ */
  auth_dir = svn_dirent_join(path, SVN_CONFIG__AUTH_SUBDIR, pool);
  err = svn_io_check_path(auth_dir, &kind, pool);
  if (err || kind == svn_node_none)
    {
      svn_error_clear(err);
      /* 'chmod 700' permissions: */
      err = svn_io_dir_make(auth_dir,
                            (APR_UREAD | APR_UWRITE | APR_UEXECUTE),
                            pool);
      if (err)
        {
          /* Don't try making subdirs if we can't make the top-level dir. */
          svn_error_clear(err);
          return;
        }
    }

  /* If a provider exists that wants to store credentials in
     ~/.subversion, a subdirectory for the cred_kind must exist. */
  ensure_auth_subdir(auth_dir, SVN_AUTH_CRED_SIMPLE, pool);
  ensure_auth_subdir(auth_dir, SVN_AUTH_CRED_USERNAME, pool);
  ensure_auth_subdir(auth_dir, SVN_AUTH_CRED_SSL_SERVER_TRUST, pool);
  ensure_auth_subdir(auth_dir, SVN_AUTH_CRED_SSL_CLIENT_CERT_PW, pool);
}


svn_error_t *
svn_config_ensure(const char *config_dir, apr_pool_t *pool)
{
  const char *path;
  svn_node_kind_t kind;
  svn_error_t *err;

  /* Ensure that the user-specific config directory exists.  */
  SVN_ERR(svn_config_get_user_config_path(&path, config_dir, NULL, pool));

  if (! path)
    return SVN_NO_ERROR;

  err = svn_io_check_resolved_path(path, &kind, pool);
  if (err)
    {
      /* Don't throw an error, but don't continue. */
      svn_error_clear(err);
      return SVN_NO_ERROR;
    }

  if (kind == svn_node_none)
    {
      err = svn_io_dir_make(path, APR_OS_DEFAULT, pool);
      if (err)
        {
          /* Don't throw an error, but don't continue. */
          svn_error_clear(err);
          return SVN_NO_ERROR;
        }
    }
  else if (kind == svn_node_file)
    {
      /* Somebody put a file where the config directory should be.
         Wacky.  Let's bail. */
      return SVN_NO_ERROR;
    }

  /* Else, there's a configuration directory. */

  /* If we get errors trying to do things below, just stop and return
     success.  There's no _need_ to init a config directory if
     something's preventing it. */

  /** If non-existent, try to create a number of auth/ subdirectories. */
  ensure_auth_dirs(path, pool);

  /** Ensure that the `README.txt' file exists. **/
  SVN_ERR(svn_config_get_user_config_path
          (&path, config_dir, SVN_CONFIG__USR_README_FILE, pool));

  if (! path)  /* highly unlikely, since a previous call succeeded */
    return SVN_NO_ERROR;

  err = svn_io_check_path(path, &kind, pool);
  if (err)
    {
      svn_error_clear(err);
      return SVN_NO_ERROR;
    }

  if (kind == svn_node_none)
    {
      apr_file_t *f;
      const char *contents =
   "This directory holds run-time configuration information for Subversion"  NL
   "clients.  The configuration files all share the same syntax, but you"    NL
   "should examine a particular file to learn what configuration"            NL
   "directives are valid for that file."                                     NL
   ""                                                                        NL
   "The syntax is standard INI format:"                                      NL
   ""                                                                        NL
   "   - Empty lines, and lines starting with '#', are ignored."             NL
   "     The first significant line in a file must be a section header."     NL
   ""                                                                        NL
   "   - A section starts with a section header, which must start in"        NL
   "     the first column:"                                                  NL
   ""                                                                        NL
   "       [section-name]"                                                   NL
   ""                                                                        NL
   "   - An option, which must always appear within a section, is a pair"    NL
   "     (name, value).  There are two valid forms for defining an"          NL
   "     option, both of which must start in the first column:"              NL
   ""                                                                        NL
   "       name: value"                                                      NL
   "       name = value"                                                     NL
   ""                                                                        NL
   "     Whitespace around the separator (:, =) is optional."                NL
   ""                                                                        NL
   "   - Section and option names are case-insensitive, but case is"         NL
   "     preserved."                                                         NL
   ""                                                                        NL
   "   - An option's value may be broken into several lines.  The value"     NL
   "     continuation lines must start with at least one whitespace."        NL
   "     Trailing whitespace in the previous line, the newline character"    NL
   "     and the leading whitespace in the continuation line is compressed"  NL
   "     into a single space character."                                     NL
   ""                                                                        NL
   "   - All leading and trailing whitespace around a value is trimmed,"     NL
   "     but the whitespace within a value is preserved, with the"           NL
   "     exception of whitespace around line continuations, as"              NL
   "     described above."                                                   NL
   ""                                                                        NL
   "   - When a value is a boolean, any of the following strings are"        NL
   "     recognised as truth values (case does not matter):"                 NL
   ""                                                                        NL
   "       true      false"                                                  NL
   "       yes       no"                                                     NL
   "       on        off"                                                    NL
   "       1         0"                                                      NL
   ""                                                                        NL
   "   - When a value is a list, it is comma-separated.  Again, the"         NL
   "     whitespace around each element of the list is trimmed."             NL
   ""                                                                        NL
   "   - Option values may be expanded within a value by enclosing the"      NL
   "     option name in parentheses, preceded by a percent sign and"         NL
   "     followed by an 's':"                                                NL
   ""                                                                        NL
   "       %(name)s"                                                         NL
   ""                                                                        NL
   "     The expansion is performed recursively and on demand, during"       NL
   "     svn_option_get.  The name is first searched for in the same"        NL
   "     section, then in the special [DEFAULT] section. If the name"        NL
   "     is not found, the whole '%(name)s' placeholder is left"             NL
   "     unchanged."                                                         NL
   ""                                                                        NL
   "     Any modifications to the configuration data invalidate all"         NL
   "     previously expanded values, so that the next svn_option_get"        NL
   "     will take the modifications into account."                          NL
   ""                                                                        NL
   "The syntax of the configuration files is a subset of the one used by"    NL
   "Python's ConfigParser module; see"                                       NL
   ""                                                                        NL
   "   http://www.python.org/doc/current/lib/module-ConfigParser.html"       NL
   ""                                                                        NL
   "Configuration data in the Windows registry"                              NL
   "=========================================="                              NL
   ""                                                                        NL
   "On Windows, configuration data may also be stored in the registry. The"  NL
   "functions svn_config_read and svn_config_merge will read from the"       NL
   "registry when passed file names of the form:"                            NL
   ""                                                                        NL
   "   REGISTRY:<hive>/path/to/config-key"                                   NL
   ""                                                                        NL
   "The REGISTRY: prefix must be in upper case. The <hive> part must be"     NL
   "one of:"                                                                 NL
   ""                                                                        NL
   "   HKLM for HKEY_LOCAL_MACHINE"                                          NL
   "   HKCU for HKEY_CURRENT_USER"                                           NL
   ""                                                                        NL
   "The values in config-key represent the options in the [DEFAULT] section."NL
   "The keys below config-key represent other sections, and their values"    NL
   "represent the options. Only values of type REG_SZ whose name doesn't"    NL
   "start with a '#' will be used; other values, as well as the keys'"       NL
   "default values, will be ignored."                                        NL
   ""                                                                        NL
   ""                                                                        NL
   "File locations"                                                          NL
   "=============="                                                          NL
   ""                                                                        NL
   "Typically, Subversion uses two config directories, one for site-wide"    NL
   "configuration,"                                                          NL
   ""                                                                        NL
   "  Unix:"                                                                 NL
   "    /etc/subversion/servers"                                             NL
   "    /etc/subversion/config"                                              NL
   "    /etc/subversion/hairstyles"                                          NL
   "  Windows:"                                                              NL
   "    %ALLUSERSPROFILE%\\Application Data\\Subversion\\servers"            NL
   "    %ALLUSERSPROFILE%\\Application Data\\Subversion\\config"             NL
   "    %ALLUSERSPROFILE%\\Application Data\\Subversion\\hairstyles"         NL
   "    REGISTRY:HKLM\\Software\\Tigris.org\\Subversion\\Servers"            NL
   "    REGISTRY:HKLM\\Software\\Tigris.org\\Subversion\\Config"             NL
   "    REGISTRY:HKLM\\Software\\Tigris.org\\Subversion\\Hairstyles"         NL
   ""                                                                        NL
   "and one for per-user configuration:"                                     NL
   ""                                                                        NL
   "  Unix:"                                                                 NL
   "    ~/.subversion/servers"                                               NL
   "    ~/.subversion/config"                                                NL
   "    ~/.subversion/hairstyles"                                            NL
   "  Windows:"                                                              NL
   "    %APPDATA%\\Subversion\\servers"                                      NL
   "    %APPDATA%\\Subversion\\config"                                       NL
   "    %APPDATA%\\Subversion\\hairstyles"                                   NL
   "    REGISTRY:HKCU\\Software\\Tigris.org\\Subversion\\Servers"            NL
   "    REGISTRY:HKCU\\Software\\Tigris.org\\Subversion\\Config"             NL
   "    REGISTRY:HKCU\\Software\\Tigris.org\\Subversion\\Hairstyles"         NL
   ""                                                                        NL;

      err = svn_io_file_open(&f, path,
                             (APR_WRITE | APR_CREATE | APR_EXCL),
                             APR_OS_DEFAULT,
                             pool);

      if (! err)
        {
          SVN_ERR(svn_io_file_write_full(f, contents,
                                         strlen(contents), NULL, pool));
          SVN_ERR(svn_io_file_close(f, pool));
        }

      svn_error_clear(err);
    }

  /** Ensure that the `servers' file exists. **/
  SVN_ERR(svn_config_get_user_config_path
          (&path, config_dir, SVN_CONFIG_CATEGORY_SERVERS, pool));

  if (! path)  /* highly unlikely, since a previous call succeeded */
    return SVN_NO_ERROR;

  err = svn_io_check_path(path, &kind, pool);
  if (err)
    {
      svn_error_clear(err);
      return SVN_NO_ERROR;
    }

  if (kind == svn_node_none)
    {
      apr_file_t *f;
      const char *contents =
        "### This file specifies server-specific parameters,"                NL
        "### including HTTP proxy information, HTTP timeout settings,"       NL
        "### and authentication settings."                                   NL
        "###"                                                                NL
        "### The currently defined server options are:"                      NL
        "###   http-proxy-host            Proxy host for HTTP connection"    NL
        "###   http-proxy-port            Port number of proxy host service" NL
        "###   http-proxy-username        Username for auth to proxy service"NL
        "###   http-proxy-password        Password for auth to proxy service"NL
        "###   http-proxy-exceptions      List of sites that do not use proxy"
                                                                             NL
        "###   http-timeout               Timeout for HTTP requests in seconds"
                                                                             NL
        "###   http-compression           Whether to compress HTTP requests" NL
        "###   neon-debug-mask            Debug mask for Neon HTTP library"  NL
#ifdef SVN_NEON_0_26
        "###   http-auth-types            Auth types to use for HTTP library"NL
#endif
        "###   ssl-authority-files        List of files, each of a trusted CA"
                                                                             NL
        "###   ssl-trust-default-ca       Trust the system 'default' CAs"    NL
        "###   ssl-client-cert-file       PKCS#12 format client certificate file"
                                                                             NL
        "###   ssl-client-cert-password   Client Key password, if needed."   NL
        "###   ssl-pkcs11-provider        Name of PKCS#11 provider to use."  NL
        "###   http-library               Which library to use for http/https"
                                                                             NL
        "###                              connections (neon or serf)"        NL
        "###   store-passwords            Specifies whether passwords used"  NL
        "###                              to authenticate against a"         NL
        "###                              Subversion server may be cached"   NL
        "###                              to disk in any way."               NL
        "###   store-plaintext-passwords  Specifies whether passwords may"   NL
        "###                              be cached on disk unencrypted."    NL
        "###   store-ssl-client-cert-pp   Specifies whether passphrase used" NL
        "###                              to authenticate against a client"  NL
        "###                              certificate may be cached to disk" NL
        "###                              in any way"                        NL
        "###   store-ssl-client-cert-pp-plaintext"                           NL
        "###                              Specifies whether client cert"     NL
        "###                              passphrases may be cached on disk" NL
        "###                              unencrypted (i.e., as plaintext)." NL
        "###   store-auth-creds           Specifies whether any auth info"   NL
        "###                              (passwords as well as server certs)"
                                                                             NL
        "###                              may be cached to disk."            NL
        "###   username                   Specifies the default username."   NL
        "###"                                                                NL
        "### Set store-passwords to 'no' to avoid storing passwords on disk" NL
        "### in any way, including in password stores. It defaults to 'yes',"
                                                                             NL
        "### but Subversion will never save your password to disk in plaintext"
                                                                             NL
        "### unless you tell it to."                                         NL
        "### Note that this option only prevents saving of *new* passwords;" NL
        "### it doesn't invalidate existing passwords.  (To do that, remove" NL
        "### the cache files by hand as described in the Subversion book.)"  NL
        "###"                                                                NL
        "### Set store-plaintext-passwords to 'no' to avoid storing"         NL
        "### passwords in unencrypted form in the auth/ area of your config" NL
        "### directory. Set it to 'yes' to allow Subversion to store"        NL
        "### unencrypted passwords in the auth/ area.  The default is"       NL
        "### 'ask', which means that Subversion will ask you before"         NL
        "### saving a password to disk in unencrypted form.  Note that"      NL
        "### this option has no effect if either 'store-passwords' or "      NL
        "### 'store-auth-creds' is set to 'no'."                             NL
        "###"                                                                NL
        "### Set store-ssl-client-cert-pp to 'no' to avoid storing ssl"      NL
        "### client certificate passphrases in the auth/ area of your"       NL
        "### config directory.  It defaults to 'yes', but Subversion will"   NL
        "### never save your passphrase to disk in plaintext unless you tell"NL
        "### it to via 'store-ssl-client-cert-pp-plaintext' (see below)."    NL
        "###"                                                                NL
        "### Note store-ssl-client-cert-pp only prevents the saving of *new*"NL
        "### passphrases; it doesn't invalidate existing passphrases.  To do"NL
        "### that, remove the cache files by hand as described in the"       NL
        "### Subversion book at http://svnbook.red-bean.com/nightly/en/\\"   NL
        "###                    svn.serverconfig.netmodel.html\\"            NL
        "###                    #svn.serverconfig.netmodel.credcache"        NL
        "###"                                                                NL
        "### Set store-ssl-client-cert-pp-plaintext to 'no' to avoid storing"NL
        "### passphrases in unencrypted form in the auth/ area of your"      NL
        "### config directory.  Set it to 'yes' to allow Subversion to"      NL
        "### store unencrypted passphrases in the auth/ area.  The default"  NL
        "### is 'ask', which means that Subversion will prompt before"       NL
        "### saving a passphrase to disk in unencrypted form.  Note that"    NL
        "### this option has no effect if either 'store-auth-creds' or "     NL
        "### 'store-ssl-client-cert-pp' is set to 'no'."                     NL
        "###"                                                                NL
        "### Set store-auth-creds to 'no' to avoid storing any Subversion"   NL
        "### credentials in the auth/ area of your config directory."        NL
        "### Note that this includes SSL server certificates."               NL
        "### It defaults to 'yes'.  Note that this option only prevents"     NL
        "### saving of *new* credentials;  it doesn't invalidate existing"   NL
        "### caches.  (To do that, remove the cache files by hand.)"         NL
        "###"                                                                NL
        "### HTTP timeouts, if given, are specified in seconds.  A timeout"  NL
        "### of 0, i.e. zero, causes a builtin default to be used."          NL
        "###"                                                                NL
        "### The commented-out examples below are intended only to"          NL
        "### demonstrate how to use this file; any resemblance to actual"    NL
        "### servers, living or dead, is entirely coincidental."             NL
        ""                                                                   NL
        "### In the 'groups' section, the URL of the repository you're"      NL
        "### trying to access is matched against the patterns on the right." NL
        "### If a match is found, the server options are taken from the"     NL
        "### section with the corresponding name on the left."               NL
        ""                                                                   NL
        "[groups]"                                                           NL
        "# group1 = *.collab.net"                                            NL
        "# othergroup = repository.blarggitywhoomph.com"                     NL
        "# thirdgroup = *.example.com"                                       NL
        ""                                                                   NL
        "### Information for the first group:"                               NL
        "# [group1]"                                                         NL
        "# http-proxy-host = proxy1.some-domain-name.com"                    NL
        "# http-proxy-port = 80"                                             NL
        "# http-proxy-username = blah"                                       NL
        "# http-proxy-password = doubleblah"                                 NL
        "# http-timeout = 60"                                                NL
#ifdef SVN_NEON_0_26
        "# http-auth-types = basic;digest;negotiate"                         NL
#endif
        "# neon-debug-mask = 130"                                            NL
        "# store-plaintext-passwords = no"                                   NL
        "# username = harry"                                                 NL
        ""                                                                   NL
        "### Information for the second group:"                              NL
        "# [othergroup]"                                                     NL
        "# http-proxy-host = proxy2.some-domain-name.com"                    NL
        "# http-proxy-port = 9000"                                           NL
        "# No username and password for the proxy, so use the defaults below."
                                                                             NL
        ""                                                                   NL
        "### You can set default parameters in the 'global' section."        NL
        "### These parameters apply if no corresponding parameter is set in" NL
        "### a specifically matched group as shown above.  Thus, if you go"  NL
        "### through the same proxy server to reach every site on the"       NL
        "### Internet, you probably just want to put that server's"          NL
        "### information in the 'global' section and not bother with"        NL
        "### 'groups' or any other sections."                                NL
        "###"                                                                NL
        "### Most people might want to configure password caching"           NL
        "### parameters here, but you can also configure them per server"    NL
        "### group (per-group settings override global settings)."           NL
        "###"                                                                NL
        "### If you go through a proxy for all but a few sites, you can"     NL
        "### list those exceptions under 'http-proxy-exceptions'.  This only"NL
        "### overrides defaults, not explicitly matched server names."       NL
        "###"                                                                NL
        "### 'ssl-authority-files' is a semicolon-delimited list of files,"  NL
        "### each pointing to a PEM-encoded Certificate Authority (CA) "     NL
        "### SSL certificate.  See details above for overriding security "   NL
        "### due to SSL."                                                    NL
        "[global]"                                                           NL
        "# http-proxy-exceptions = *.exception.com, www.internal-site.org"   NL
        "# http-proxy-host = defaultproxy.whatever.com"                      NL
        "# http-proxy-port = 7000"                                           NL
        "# http-proxy-username = defaultusername"                            NL
        "# http-proxy-password = defaultpassword"                            NL
        "# http-compression = no"                                            NL
#ifdef SVN_NEON_0_26
        "# http-auth-types = basic;digest;negotiate"                         NL
#endif
        "# No http-timeout, so just use the builtin default."                NL
        "# No neon-debug-mask, so neon debugging is disabled."               NL
        "# ssl-authority-files = /path/to/CAcert.pem;/path/to/CAcert2.pem"   NL
        "#"                                                                  NL
        "# Password / passphrase caching parameters:"                        NL
        "# store-passwords = no"                                             NL
        "# store-plaintext-passwords = no"                                   NL
        "# store-ssl-client-cert-pp = no"                                    NL
        "# store-ssl-client-cert-pp-plaintext = no"                          NL;

      err = svn_io_file_open(&f, path,
                             (APR_WRITE | APR_CREATE | APR_EXCL),
                             APR_OS_DEFAULT,
                             pool);

      if (! err)
        {
          SVN_ERR(svn_io_file_write_full(f, contents,
                                         strlen(contents), NULL, pool));
          SVN_ERR(svn_io_file_close(f, pool));
        }

      svn_error_clear(err);
    }

  /** Ensure that the `config' file exists. **/
  SVN_ERR(svn_config_get_user_config_path
          (&path, config_dir, SVN_CONFIG_CATEGORY_CONFIG, pool));

  if (! path)  /* highly unlikely, since a previous call succeeded */
    return SVN_NO_ERROR;

  err = svn_io_check_path(path, &kind, pool);
  if (err)
    {
      svn_error_clear(err);
      return SVN_NO_ERROR;
    }

  if (kind == svn_node_none)
    {
      apr_file_t *f;
      const char *contents =
        "### This file configures various client-side behaviors."            NL
        "###"                                                                NL
        "### The commented-out examples below are intended to demonstrate"   NL
        "### how to use this file."                                          NL
        ""                                                                   NL
        "### Section for authentication and authorization customizations."   NL
        "[auth]"                                                             NL
        "### Set password stores used by Subversion. They should be"         NL
        "### delimited by spaces or commas. The order of values determines"  NL
        "### the order in which password stores are used."                   NL
        "### Valid password stores:"                                         NL
        "###   gnome-keyring        (Unix-like systems)"                     NL
        "###   kwallet              (Unix-like systems)"                     NL
        "###   keychain             (Mac OS X)"                              NL
        "###   windows-cryptoapi    (Windows)"                               NL
#ifdef SVN_HAVE_KEYCHAIN_SERVICES
        "# password-stores = keychain"                                       NL
#elif defined(WIN32) && !defined(__MINGW32__)
        "# password-stores = windows-cryptoapi"                              NL
#else
        "# password-stores = gnome-keyring,kwallet"                          NL
#endif
        "### To disable all password stores, use an empty list:"             NL
        "# password-stores ="                                                NL
#ifdef SVN_HAVE_KWALLET
        "###"                                                                NL
        "### Set KWallet wallet used by Subversion. If empty or unset,"      NL
        "### then the default network wallet will be used."                  NL
        "# kwallet-wallet ="                                                 NL
        "###"                                                                NL
        "### Include PID (Process ID) in Subversion application name when"   NL
        "### using KWallet. It defaults to 'no'."                            NL
        "# kwallet-svn-application-name-with-pid = yes"                      NL
#endif
        "###"                                                                NL
        "### The rest of the [auth] section in this file has been deprecated."
                                                                             NL
        "### Both 'store-passwords' and 'store-auth-creds' can now be"       NL
        "### specified in the 'servers' file in your config directory"       NL
        "### and are documented there. Anything specified in this section "  NL
        "### is overridden by settings specified in the 'servers' file."     NL
        "# store-passwords = no"                                             NL
        "# store-auth-creds = no"                                            NL
        ""                                                                   NL
        "### Section for configuring external helper applications."          NL
        "[helpers]"                                                          NL
        "### Set editor-cmd to the command used to invoke your text editor." NL
        "###   This will override the environment variables that Subversion" NL
        "###   examines by default to find this information ($EDITOR, "      NL
        "###   et al)."                                                      NL
        "# editor-cmd = editor (vi, emacs, notepad, etc.)"                   NL
        "### Set diff-cmd to the absolute path of your 'diff' program."      NL
        "###   This will override the compile-time default, which is to use" NL
        "###   Subversion's internal diff implementation."                   NL
        "# diff-cmd = diff_program (diff, gdiff, etc.)"                      NL
        "### Diff-extensions are arguments passed to an external diff"       NL
        "### program or to Subversion's internal diff implementation."       NL
        "### Set diff-extensions to override the default arguments ('-u')."  NL
        "# diff-extensions = -u -p"                                          NL
        "### Set diff3-cmd to the absolute path of your 'diff3' program."    NL
        "###   This will override the compile-time default, which is to use" NL
        "###   Subversion's internal diff3 implementation."                  NL
        "# diff3-cmd = diff3_program (diff3, gdiff3, etc.)"                  NL
        "### Set diff3-has-program-arg to 'yes' if your 'diff3' program"     NL
        "###   accepts the '--diff-program' option."                         NL
        "# diff3-has-program-arg = [yes | no]"                               NL
        "### Set merge-tool-cmd to the command used to invoke your external" NL
        "### merging tool of choice. Subversion will pass 5 arguments to"    NL
        "### the specified command: base theirs mine merged wcfile"          NL
        "# merge-tool-cmd = merge_command"                                   NL
        ""                                                                   NL
        "### Section for configuring tunnel agents."                         NL
        "[tunnels]"                                                          NL
        "### Configure svn protocol tunnel schemes here.  By default, only"  NL
        "### the 'ssh' scheme is defined.  You can define other schemes to"  NL
        "### be used with 'svn+scheme://hostname/path' URLs.  A scheme"      NL
        "### definition is simply a command, optionally prefixed by an"      NL
        "### environment variable name which can override the command if it" NL
        "### is defined.  The command (or environment variable) may contain" NL
        "### arguments, using standard shell quoting for arguments with"     NL
        "### spaces.  The command will be invoked as:"                       NL
        "###   <command> <hostname> svnserve -t"                             NL
        "### (If the URL includes a username, then the hostname will be"     NL
        "### passed to the tunnel agent as <user>@<hostname>.)  If the"      NL
        "### built-in ssh scheme were not predefined, it could be defined"   NL
        "### as:"                                                            NL
        "# ssh = $SVN_SSH ssh -q"                                            NL
        "### If you wanted to define a new 'rsh' scheme, to be used with"    NL
        "### 'svn+rsh:' URLs, you could do so as follows:"                   NL
        "# rsh = rsh"                                                        NL
        "### Or, if you wanted to specify a full path and arguments:"        NL
        "# rsh = /path/to/rsh -l myusername"                                 NL
        "### On Windows, if you are specifying a full path to a command,"    NL
        "### use a forward slash (/) or a paired backslash (\\\\) as the"    NL
        "### path separator.  A single backslash will be treated as an"      NL
        "### escape for the following character."                            NL
        ""                                                                   NL
        "### Section for configuring miscelleneous Subversion options."      NL
        "[miscellany]"                                                       NL
        "### Set global-ignores to a set of whitespace-delimited globs"      NL
        "### which Subversion will ignore in its 'status' output, and"       NL
        "### while importing or adding files and directories."               NL
        "### '*' matches leading dots, e.g. '*.rej' matches '.foo.rej'."     NL
        "# global-ignores = " SVN_CONFIG__DEFAULT_GLOBAL_IGNORES_LINE_1      NL
        "#   " SVN_CONFIG__DEFAULT_GLOBAL_IGNORES_LINE_2                     NL
        "### Set log-encoding to the default encoding for log messages"      NL
        "# log-encoding = latin1"                                            NL
        "### Set use-commit-times to make checkout/update/switch/revert"     NL
        "### put last-committed timestamps on every file touched."           NL
        "# use-commit-times = yes"                                           NL
        "### Set no-unlock to prevent 'svn commit' from automatically"       NL
        "### releasing locks on files."                                      NL
        "# no-unlock = yes"                                                  NL
        "### Set mime-types-file to a MIME type registry file, used to"      NL
        "### provide hints to Subversion's MIME type auto-detection"         NL
        "### algorithm."                                                     NL
        "# mime-types-file = /path/to/mime.types"                            NL
        "### Set preserved-conflict-file-exts to a whitespace-delimited"     NL
        "### list of patterns matching file extensions which should be"      NL
        "### preserved in generated conflict file names.  By default,"       NL
        "### conflict files use custom extensions."                          NL
        "# preserved-conflict-file-exts = doc ppt xls od?"                   NL
        "### Set enable-auto-props to 'yes' to enable automatic properties"  NL
        "### for 'svn add' and 'svn import', it defaults to 'no'."           NL
        "### Automatic properties are defined in the section 'auto-props'."  NL
        "# enable-auto-props = yes"                                          NL
        "### Set interactive-conflicts to 'no' to disable interactive"       NL
        "### conflict resolution prompting.  It defaults to 'yes'."          NL
        "# interactive-conflicts = no"                                       NL
        "### Set memory-cache-size to define the size of the memory cache"   NL
        "### used by the client when accessing a FSFS repository via"        NL
        "### ra_local (the file:// scheme). The value represents the number" NL
        "### of MB used by the cache."                                       NL
        "# memory-cache-size = 16"                                           NL
        ""                                                                   NL
        "### Section for configuring automatic properties."                  NL
        "[auto-props]"                                                       NL
        "### The format of the entries is:"                                  NL
        "###   file-name-pattern = propname[=value][;propname[=value]...]"   NL
        "### The file-name-pattern can contain wildcards (such as '*' and"   NL
        "### '?').  All entries which match (case-insensitively) will be"    NL
        "### applied to the file.  Note that auto-props functionality"       NL
        "### must be enabled, which is typically done by setting the"        NL
        "### 'enable-auto-props' option."                                    NL
        "# *.c = svn:eol-style=native"                                       NL
        "# *.cpp = svn:eol-style=native"                                     NL
        "# *.h = svn:keywords=Author Date Id Rev URL;svn:eol-style=native"   NL
        "# *.dsp = svn:eol-style=CRLF"                                       NL
        "# *.dsw = svn:eol-style=CRLF"                                       NL
        "# *.sh = svn:eol-style=native;svn:executable"                       NL
        "# *.txt = svn:eol-style=native;svn:keywords=Author Date Id Rev URL;"NL
        "# *.png = svn:mime-type=image/png"                                  NL
        "# *.jpg = svn:mime-type=image/jpeg"                                 NL
        "# Makefile = svn:eol-style=native"                                  NL
        ""                                                                   NL;

      err = svn_io_file_open(&f, path,
                             (APR_WRITE | APR_CREATE | APR_EXCL),
                             APR_OS_DEFAULT,
                             pool);

      if (! err)
        {
          SVN_ERR(svn_io_file_write_full(f, contents,
                                         strlen(contents), NULL, pool));
          SVN_ERR(svn_io_file_close(f, pool));
        }

      svn_error_clear(err);
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_config_get_user_config_path(const char **path,
                                const char *config_dir,
                                const char *fname,
                                apr_pool_t *pool)
{
  *path= NULL;

  /* Note that even if fname is null, svn_dirent_join_many will DTRT. */

  if (config_dir)
    {
      *path = svn_dirent_join_many(pool, config_dir, fname, NULL);
      return SVN_NO_ERROR;
    }

#ifdef WIN32
  {
    const char *folder;
    SVN_ERR(svn_config__win_config_path(&folder, FALSE, pool));
    *path = svn_dirent_join_many(pool, folder,
                                 SVN_CONFIG__SUBDIRECTORY, fname, NULL);
  }

#elif defined(__HAIKU__)
  {
    char folder[B_PATH_NAME_LENGTH];

    status_t error = find_directory(B_USER_SETTINGS_DIRECTORY, -1, false,
                                    folder, sizeof(folder));
    if (error)
      return SVN_NO_ERROR;

    *path = svn_dirent_join_many(pool, folder,
                                 SVN_CONFIG__USR_DIRECTORY, fname, NULL);
  }
#else  /* ! WIN32 && !__HAIKU__ */

  {
    const char *homedir = svn_user_get_homedir(pool);
    if (! homedir)
      return SVN_NO_ERROR;
    *path = svn_dirent_join_many(pool,
                               svn_dirent_canonicalize(homedir, pool),
                               SVN_CONFIG__USR_DIRECTORY, fname, NULL);
  }
#endif /* WIN32 */

  return SVN_NO_ERROR;
}


/*
 * minimal_client.c  - a minimal Subversion client application ("hello world")
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
 *
 *  This app demonstrates how to use the svn_client.h API.
 *
 *  It reads a directory URL from the commandline, runs
 *  svn_client_list() and prints the list of directory-entries.  It
 *  also knows how to deal with basic username/password authentication
 *  challenges.
 *
 *  For a much more complex example, the svn cmdline client might be
 *  considered the 'reference implementation'.
 *
 *  From a Linux system, a typical commandline compile might look like:
 *
 *  cc minimal_client.c -o minimal_client \
 *  -I/usr/local/include/subversion-1 -I/usr/local/apache2/include \
 *  -L/usr/local/apache2/lib -L/usr/local/lib \
 *  -lsvn_client-1 -lapr-0 -laprutil-0
 *
 */


#include "svn_client.h"
#include "svn_cmdline.h"
#include "svn_pools.h"
#include "svn_config.h"
#include "svn_fs.h"


/* Display a prompt and read a one-line response into the provided buffer,
   removing a trailing newline if present. */
static svn_error_t *
prompt_and_read_line(const char *prompt,
                     char *buffer,
                     size_t max)
{
  int len;
  printf("%s: ", prompt);
  if (fgets(buffer, max, stdin) == NULL)
    return svn_error_create(0, NULL, "error reading stdin");
  len = strlen(buffer);
  if (len > 0 && buffer[len-1] == '\n')
    buffer[len-1] = 0;
  return SVN_NO_ERROR;
}

/* A tiny callback function of type 'svn_auth_simple_prompt_func_t'. For
   a much better example, see svn_cl__auth_simple_prompt in the official
   svn cmdline client. */
static svn_error_t *
my_simple_prompt_callback (svn_auth_cred_simple_t **cred,
                           void *baton,
                           const char *realm,
                           const char *username,
                           svn_boolean_t may_save,
                           apr_pool_t *pool)
{
  svn_auth_cred_simple_t *ret = apr_pcalloc (pool, sizeof (*ret));
  char answerbuf[100];

  if (realm)
    {
      printf ("Authentication realm: %s\n", realm);
    }

  if (username)
    ret->username = apr_pstrdup (pool, username);
  else
    {
      SVN_ERR (prompt_and_read_line("Username", answerbuf, sizeof(answerbuf)));
      ret->username = apr_pstrdup (pool, answerbuf);
    }

  SVN_ERR (prompt_and_read_line("Password", answerbuf, sizeof(answerbuf)));
  ret->password = apr_pstrdup (pool, answerbuf);

  *cred = ret;
  return SVN_NO_ERROR;
}


/* A tiny callback function of type 'svn_auth_username_prompt_func_t'. For
   a much better example, see svn_cl__auth_username_prompt in the official
   svn cmdline client. */
static svn_error_t *
my_username_prompt_callback (svn_auth_cred_username_t **cred,
                             void *baton,
                             const char *realm,
                             svn_boolean_t may_save,
                             apr_pool_t *pool)
{
  svn_auth_cred_username_t *ret = apr_pcalloc (pool, sizeof (*ret));
  char answerbuf[100];

  if (realm)
    {
      printf ("Authentication realm: %s\n", realm);
    }

  SVN_ERR (prompt_and_read_line("Username", answerbuf, sizeof(answerbuf)));
  ret->username = apr_pstrdup (pool, answerbuf);

  *cred = ret;
  return SVN_NO_ERROR;
}



int
main (int argc, const char **argv)
{
  apr_pool_t *pool;
  svn_error_t *err;
  svn_opt_revision_t revision;
  apr_hash_t *dirents;
  apr_hash_index_t *hi;
  svn_client_ctx_t *ctx;
  const char *URL;

  if (argc <= 1)
    {
      printf ("Usage:  %s URL\n", argv[0]);
      return EXIT_FAILURE;
    }
  else
    URL = argv[1];

  /* Initialize the app.  Send all error messages to 'stderr'.  */
  if (svn_cmdline_init ("minimal_client", stderr) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  /* Create top-level memory pool. Be sure to read the HACKING file to
     understand how to properly use/free subpools. */
  pool = svn_pool_create (NULL);

  /* Initialize the FS library. */
  err = svn_fs_initialize (pool);
  if (err)
    {
      /* For functions deeper in the stack, we usually use the
         SVN_ERR() exception-throwing macro (see svn_error.h).  At the
         top level, we catch & print the error with svn_handle_error2(). */
      svn_handle_error2 (err, stderr, FALSE, "minimal_client: ");
      return EXIT_FAILURE;
    }

  /* Make sure the ~/.subversion run-time config files exist */
  err = svn_config_ensure (NULL, pool);
  if (err)
    {
      svn_handle_error2 (err, stderr, FALSE, "minimal_client: ");
      return EXIT_FAILURE;
    }

  /* All clients need to fill out a client_ctx object. */
  {
    /* Initialize and allocate the client_ctx object. */
    if ((err = svn_client_create_context (&ctx, pool)))
      {
        svn_handle_error2 (err, stderr, FALSE, "minimal_client: ");
        return EXIT_FAILURE;
      }

    /* Load the run-time config file into a hash */
    if ((err = svn_config_get_config (&(ctx->config), NULL, pool)))
      {
        svn_handle_error2 (err, stderr, FALSE, "minimal_client: ");
        return EXIT_FAILURE;
      }

#ifdef WIN32
    /* Set the working copy administrative directory name. */
    if (getenv ("SVN_ASP_DOT_NET_HACK"))
      {
        err = svn_wc_set_adm_dir ("_svn", pool);
        if (err)
          {
            svn_handle_error2 (err, stderr, FALSE, "minimal_client: ");
            return EXIT_FAILURE;
          }
      }
#endif

    /* Depending on what your client does, you'll want to read about
       (and implement) the various callback function types below.  */

    /* A func (& context) which receives event signals during
       checkouts, updates, commits, etc.  */
    /* ctx->notify_func = my_notification_func;
       ctx->notify_baton = NULL; */

    /* A func (& context) which can receive log messages */
    /* ctx->log_msg_func = my_log_msg_receiver_func;
       ctx->log_msg_baton = NULL; */

    /* A func (& context) which checks whether the user cancelled */
    /* ctx->cancel_func = my_cancel_checking_func;
       ctx->cancel_baton = NULL; */

    /* Make the client_ctx capable of authenticating users */
    {
      /* There are many different kinds of authentication back-end
         "providers".  See svn_auth.h for a full overview.

         If you want to get the auth behavior of the 'svn' program,
         you can use svn_cmdline_setup_auth_baton, which will give
         you the exact set of auth providers it uses.  This program
         doesn't use it because it's only appropriate for a command
         line program, and this is supposed to be a general purpose
         example. */

      svn_auth_provider_object_t *provider;
      apr_array_header_t *providers
        = apr_array_make (pool, 4, sizeof (svn_auth_provider_object_t *));

      svn_auth_get_simple_prompt_provider (&provider,
                                           my_simple_prompt_callback,
                                           NULL, /* baton */
                                           2, /* retry limit */ pool);
      APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

      svn_auth_get_username_prompt_provider (&provider,
                                             my_username_prompt_callback,
                                             NULL, /* baton */
                                             2, /* retry limit */ pool);
      APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

      /* Register the auth-providers into the context's auth_baton. */
      svn_auth_open (&ctx->auth_baton, providers, pool);
    }
  } /* end of client_ctx setup */


  /* Now do the real work. */

  /* Set revision to always be the HEAD revision.  It could, however,
     be set to a specific revision number, date, or other values. */
  revision.kind = svn_opt_revision_head;

  /* Main call into libsvn_client does all the work. */
  err = svn_client_ls (&dirents,
                       URL, &revision,
                       FALSE, /* no recursion */
                       ctx, pool);
  if (err)
    {
      svn_handle_error2 (err, stderr, FALSE, "minimal_client: ");
      return EXIT_FAILURE;
    }

  /* Print the dir entries in the hash. */
  for (hi = apr_hash_first (pool, dirents); hi; hi = apr_hash_next (hi))
    {
      const char *entryname;
      svn_dirent_t *val;

      apr_hash_this (hi, (void *) &entryname, NULL, (void *) &val);
      printf ("   %s\n", entryname);

      /* 'val' is actually an svn_dirent_t structure; a more complex
          program would mine it for extra printable information. */
    }

  return EXIT_SUCCESS;
}

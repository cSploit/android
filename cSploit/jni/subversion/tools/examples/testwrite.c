/*
 * testwrite.c : test whether a user has commit access.
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
 *  To compile on unix against Subversion and APR libraries, try
 *  something like:
 *
 *  cc testwrite.c -o testwrite \
 *  -I/usr/local/include/subversion-1 -I/usr/local/apache2/include \
 *  -L/usr/local/apache2/lib -L/usr/local/lib \
 *  -lsvn_client-1 -lsvn_ra-1 -lsvn_subr-1 -lsvn-fs-1 -lapr-0 -laprutil-0
 *
 */

#include "svn_client.h"
#include "svn_pools.h"
#include "svn_config.h"
#include "svn_fs.h"
#include "svn_cmdline.h"
#include "svn_path.h"
#include "svn_time.h"


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

/* A callback function used when the RA layer needs a handle to a
   temporary file.  This is a reduced version of the callback used in
   the official svn cmdline client. */
static svn_error_t *
open_tmp_file (apr_file_t **fp,
               void *callback_baton,
               apr_pool_t *pool)
{
  const char *path;
  const char *ignored_filename;

  SVN_ERR (svn_io_temp_dir (&path, pool));
  path = svn_path_join (path, "tempfile", pool);

  /* Open a unique file, with delete-on-close set. */
  SVN_ERR (svn_io_open_unique_file2 (fp, &ignored_filename,
                                     path, ".tmp",
                                     svn_io_file_del_on_close, pool));

  return SVN_NO_ERROR;
}


/* Called when a commit is successful. */
static svn_error_t *
my_commit_callback (svn_revnum_t new_revision,
                      const char *date,
                      const char *author,
                      void *baton)
{
  printf ("Upload complete.  Committed revision %ld.\n", new_revision);
  return SVN_NO_ERROR;
}



int
main (int argc, const char **argv)
{
  apr_pool_t *pool;
  svn_error_t *err;
  apr_hash_t *dirents;
  const char *upload_file, *URL;
  const char *parent_URL, *basename;
  svn_ra_plugin_t *ra_lib;
  void *session, *ra_baton;
  svn_revnum_t rev;
  const svn_delta_editor_t *editor;
  void *edit_baton;
  svn_dirent_t *dirent;
  svn_ra_callbacks_t *cbtable;
  apr_hash_t *cfg_hash;
  svn_auth_baton_t *auth_baton;

  if (argc <= 1)
    {
      printf ("Usage:  %s URL\n", argv[0]);
      printf ("    Tries to create an svn commit-transaction at URL.\n");
      return EXIT_FAILURE;
    }
  URL = argv[1];

  /* Initialize the app.  Send all error messages to 'stderr'.  */
  if (svn_cmdline_init ("minimal_client", stderr) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  /* Create top-level memory pool. Be sure to read the HACKING file to
     understand how to properly use/free subpools. */
  pool = svn_pool_create (NULL);

  /* Initialize the FS library. */
  err = svn_fs_initialize (pool);
  if (err) goto hit_error;

  /* Make sure the ~/.subversion run-time config files exist, and load. */
  err = svn_config_ensure (NULL, pool);
  if (err) goto hit_error;

  err = svn_config_get_config (&cfg_hash, NULL, pool);
  if (err) goto hit_error;

  /* Build an authentication baton. */
  {
    /* There are many different kinds of authentication back-end
       "providers".  See svn_auth.h for a full overview. */
    svn_auth_provider_object_t *provider;
    apr_array_header_t *providers
      = apr_array_make (pool, 4, sizeof (svn_auth_provider_object_t *));

    svn_client_get_simple_prompt_provider (&provider,
                                           my_simple_prompt_callback,
                                           NULL, /* baton */
                                           2, /* retry limit */ pool);
    APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

    svn_client_get_username_prompt_provider (&provider,
                                             my_username_prompt_callback,
                                             NULL, /* baton */
                                             2, /* retry limit */ pool);
    APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

    /* Register the auth-providers into the context's auth_baton. */
    svn_auth_open (&auth_baton, providers, pool);
  }

  /* Create a table of callbacks for the RA session, mostly nonexistent. */
  cbtable = apr_pcalloc (pool, sizeof(*cbtable));
  cbtable->auth_baton = auth_baton;
  cbtable->open_tmp_file = open_tmp_file;

  /* Now do the real work. */

  /* Open an RA session to the parent URL, fetch current HEAD rev and
     "lock" onto that revnum for the remainder of the session. */
  svn_path_split (URL, &parent_URL, &basename, pool);

  err = svn_ra_init_ra_libs (&ra_baton, pool);
  if (err) goto hit_error;

  err = svn_ra_get_ra_library (&ra_lib, ra_baton, parent_URL, pool);
  if (err) goto hit_error;

  err = ra_lib->open (&session, parent_URL, cbtable, NULL, cfg_hash, pool);
  if (err) goto hit_error;

  /* Fetch a commit editor (it's anchored on the parent URL, because
     the session is too.) */
  /* ### someday add an option for a user-written commit message?  */
  err = ra_lib->get_commit_editor (session, &editor, &edit_baton,
                                   "File upload from 'svnput' program.",
                                   my_commit_callback, NULL, pool);
  if (err) goto hit_error;

  /* Drive the editor */
  {
    void *root_baton, *file_baton, *handler_baton;
    svn_txdelta_window_handler_t handler;
    svn_stream_t *contents;
    apr_file_t *f = NULL;

    err = editor->open_root (edit_baton, rev, pool, &root_baton);
    if (err) goto hit_error;

    err = editor->abort_edit (edit_baton, pool);
    if (err) goto hit_error;
  }

  printf ("No problems creating commit transaction.\n");
  return EXIT_SUCCESS;

 hit_error:
  {
    printf("Could not open a commit transaction.\n");
    svn_handle_error2 (err, stderr, FALSE, "testwrite: ");
    return EXIT_FAILURE;
  }

}

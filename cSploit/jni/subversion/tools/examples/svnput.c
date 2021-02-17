/*
 * svnput.c : upload a single file to a repository, overwriting
 *            any existing file by the same name.
 *
 *   ***************************************************************

 *    WARNING!!  Despite the warnings it gives, this program allows
 *    you to potentially overwrite a file you've never seen.
 *    USE AT YOUR OWN RISK!
 *
 *      (While the repository won't 'lose' overwritten data, the
 *      overwriting may happen without your knowledge, and has the
 *      potential to cause much grief with your collaborators!)
 *
 *   ***************************************************************
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
 *  cc svnput.c -o svnput \
 *  -I/usr/local/include/subversion-1 -I/usr/local/apache2/include \
 *  -L/usr/local/apache2/lib -L/usr/local/lib \
 *  -lsvn_client-1 -lapr-0 -laprutil-0
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

  if (argc <= 2)
    {
      printf ("Usage:  %s PATH URL\n", argv[0]);
      printf ("    Uploads file at PATH to Subversion repository URL.\n");
      return EXIT_FAILURE;
    }
  upload_file = argv[1];
  URL = argv[2];

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

  err = ra_lib->get_latest_revnum (session, &rev, pool);
  if (err) goto hit_error;

  /* Examine contents of parent dir in the rev. */
  err = ra_lib->get_dir (session, "", rev, &dirents, NULL, NULL, pool);
  if (err) goto hit_error;

  /* Sanity checks.  Don't let the user shoot himself *too* much. */
  dirent = apr_hash_get (dirents, basename, APR_HASH_KEY_STRING);
  if (dirent && dirent->kind == svn_node_dir)
    {
      printf ("Sorry, a directory already exists at that URL.\n");
      return EXIT_FAILURE;
    }
  if (dirent && dirent->kind == svn_node_file)
    {
      char answer[5];

      printf ("\n*** WARNING ***\n\n");
      printf ("You're about to overwrite r%ld of this file.\n", rev);
      printf ("It was last changed by user '%s',\n",
              dirent->last_author ? dirent->last_author : "?");
      printf ("on %s.\n", svn_time_to_human_cstring (dirent->time, pool));
      printf ("\nSomebody *might* have just changed the file seconds ago,\n"
              "and your upload would be overwriting their changes!\n\n");

      err = prompt_and_read_line("Are you SURE you want to upload? [y/n]",
                                 answer, sizeof(answer));
      if (err) goto hit_error;

      if (apr_strnatcasecmp (answer, "y"))
        {
          printf ("Operation aborted.\n");
          return EXIT_SUCCESS;
        }
    }

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

    if (! dirent)
      {
        err = editor->add_file (basename, root_baton, NULL, SVN_INVALID_REVNUM,
                                pool, &file_baton);
      }
    else
      {
        err = editor->open_file (basename, root_baton, rev, pool,
                                 &file_baton);
      }
    if (err) goto hit_error;

    err = editor->apply_textdelta (file_baton, NULL, pool,
                                   &handler, &handler_baton);
    if (err) goto hit_error;

    err = svn_io_file_open (&f, upload_file, APR_READ, APR_OS_DEFAULT, pool);
    if (err) goto hit_error;

    contents = svn_stream_from_aprfile (f, pool);
    err = svn_txdelta_send_stream (contents, handler, handler_baton,
                                   NULL, pool);
    if (err) goto hit_error;

    err = svn_io_file_close (f, pool);
    if (err) goto hit_error;

    err = editor->close_file (file_baton, NULL, pool);
    if (err) goto hit_error;

    err = editor->close_edit (edit_baton, pool);
    if (err) goto hit_error;
  }

  return EXIT_SUCCESS;

 hit_error:
  svn_handle_error2 (err, stderr, FALSE, "svnput: ");
  return EXIT_FAILURE;
}

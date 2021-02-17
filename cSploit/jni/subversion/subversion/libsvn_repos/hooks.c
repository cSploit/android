/* hooks.c : running repository hooks
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <apr_pools.h>
#include <apr_file_io.h>

#include "svn_error.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_repos.h"
#include "svn_utf.h"
#include "repos.h"
#include "svn_private_config.h"
#include "private/svn_fs_private.h"
#include "private/svn_repos_private.h"
#include "private/svn_string_private.h"



/*** Hook drivers. ***/

/* Helper function for run_hook_cmd().  Wait for a hook to finish
   executing and return either SVN_NO_ERROR if the hook script completed
   without error, or an error describing the reason for failure.

   NAME and CMD are the name and path of the hook program, CMD_PROC
   is a pointer to the structure representing the running process,
   and READ_ERRHANDLE is an open handle to the hook's stderr.

   Hooks are considered to have failed if we are unable to wait for the
   process, if we are unable to read from the hook's stderr, if the
   process has failed to exit cleanly (due to a coredump, for example),
   or if the process returned a non-zero return code.

   Any error output returned by the hook's stderr will be included in an
   error message, though the presence of output on stderr is not itself
   a reason to fail a hook. */
static svn_error_t *
check_hook_result(const char *name, const char *cmd, apr_proc_t *cmd_proc,
                  apr_file_t *read_errhandle, apr_pool_t *pool)
{
  svn_error_t *err, *err2;
  svn_stringbuf_t *native_stderr, *failure_message;
  const char *utf8_stderr;
  int exitcode;
  apr_exit_why_e exitwhy;

  err2 = svn_stringbuf_from_aprfile(&native_stderr, read_errhandle, pool);

  err = svn_io_wait_for_cmd(cmd_proc, cmd, &exitcode, &exitwhy, pool);
  if (err)
    {
      svn_error_clear(err2);
      return svn_error_trace(err);
    }

  if (APR_PROC_CHECK_EXIT(exitwhy) && exitcode == 0)
    {
      /* The hook exited cleanly.  However, if we got an error reading
         the hook's stderr, fail the hook anyway, because this might be
         symptomatic of a more important problem. */
      if (err2)
        {
          return svn_error_createf
            (SVN_ERR_REPOS_HOOK_FAILURE, err2,
             _("'%s' hook succeeded, but error output could not be read"),
             name);
        }

      return SVN_NO_ERROR;
    }

  /* The hook script failed. */

  /* If we got the stderr output okay, try to translate it into UTF-8.
     Ensure there is something sensible in the UTF-8 string regardless. */
  if (!err2)
    {
      err2 = svn_utf_cstring_to_utf8(&utf8_stderr, native_stderr->data, pool);
      if (err2)
        utf8_stderr = _("[Error output could not be translated from the "
                        "native locale to UTF-8.]");
    }
  else
    {
      utf8_stderr = _("[Error output could not be read.]");
    }
  /*### It would be nice to include the text of any translation or read
        error in the messages above before we clear it here. */
  svn_error_clear(err2);

  if (!APR_PROC_CHECK_EXIT(exitwhy))
    {
      failure_message = svn_stringbuf_createf(pool,
        _("'%s' hook failed (did not exit cleanly: "
          "apr_exit_why_e was %d, exitcode was %d).  "),
        name, exitwhy, exitcode);
    }
  else
    {
      const char *action;
      if (strcmp(name, "start-commit") == 0
          || strcmp(name, "pre-commit") == 0)
        action = _("Commit");
      else if (strcmp(name, "pre-revprop-change") == 0)
        action = _("Revprop change");
      else if (strcmp(name, "pre-lock") == 0)
        action = _("Lock");
      else if (strcmp(name, "pre-unlock") == 0)
        action = _("Unlock");
      else
        action = NULL;
      if (action == NULL)
        failure_message = svn_stringbuf_createf(
            pool, _("%s hook failed (exit code %d)"),
            name, exitcode);
      else
        failure_message = svn_stringbuf_createf(
            pool, _("%s blocked by %s hook (exit code %d)"),
            action, name, exitcode);
    }

  if (utf8_stderr[0])
    {
      svn_stringbuf_appendcstr(failure_message,
                               _(" with output:\n"));
      svn_stringbuf_appendcstr(failure_message, utf8_stderr);
    }
  else
    {
      svn_stringbuf_appendcstr(failure_message,
                               _(" with no output."));
    }

  return svn_error_create(SVN_ERR_REPOS_HOOK_FAILURE, err,
                          failure_message->data);
}

/* NAME, CMD and ARGS are the name, path to and arguments for the hook
   program that is to be run.  The hook's exit status will be checked,
   and if an error occurred the hook's stderr output will be added to
   the returned error.

   If STDIN_HANDLE is non-null, pass it as the hook's stdin, else pass
   no stdin to the hook.

   If RESULT is non-null, set *RESULT to the stdout of the hook or to
   a zero-length string if the hook generates no output on stdout. */
static svn_error_t *
run_hook_cmd(svn_string_t **result,
             const char *name,
             const char *cmd,
             const char **args,
             apr_file_t *stdin_handle,
             apr_pool_t *pool)
{
  apr_file_t *null_handle;
  apr_status_t apr_err;
  svn_error_t *err;
  apr_proc_t cmd_proc;

  if (result)
    {
      null_handle = NULL;
    }
  else
    {
      /* Redirect stdout to the null device */
        apr_err = apr_file_open(&null_handle, SVN_NULL_DEVICE_NAME, APR_WRITE,
                                APR_OS_DEFAULT, pool);
        if (apr_err)
          return svn_error_wrap_apr
            (apr_err, _("Can't create null stdout for hook '%s'"), cmd);
    }

  err = svn_io_start_cmd2(&cmd_proc, ".", cmd, args, FALSE,
                          FALSE, stdin_handle, result != NULL, null_handle,
                          TRUE, NULL, pool);

  if (err)
    {
      /* CMD_PROC is not safe to use. Bail. */
      return svn_error_createf
        (SVN_ERR_REPOS_HOOK_FAILURE, err, _("Failed to start '%s' hook"), cmd);
    }
  else
    {
      err = check_hook_result(name, cmd, &cmd_proc, cmd_proc.err, pool);
    }

  /* Hooks are fallible, and so hook failure is "expected" to occur at
     times.  When such a failure happens we still want to close the pipe
     and null file */
  apr_err = apr_file_close(cmd_proc.err);
  if (!err && apr_err)
    return svn_error_wrap_apr
      (apr_err, _("Error closing read end of stderr pipe"));

  if (result)
    {
      svn_stringbuf_t *native_stdout;
      SVN_ERR(svn_stringbuf_from_aprfile(&native_stdout, cmd_proc.out, pool));
      apr_err = apr_file_close(cmd_proc.out);
      if (!err && apr_err)
        return svn_error_wrap_apr
          (apr_err, _("Error closing read end of stderr pipe"));

      *result = svn_stringbuf__morph_into_string(native_stdout);
    }
  else
    {
      apr_err = apr_file_close(null_handle);
      if (!err && apr_err)
        return svn_error_wrap_apr(apr_err, _("Error closing null file"));
    }

  return svn_error_trace(err);
}


/* Create a temporary file F that will automatically be deleted when the
   pool is cleaned up.  Fill it with VALUE, and leave it open and rewound,
   ready to be read from. */
static svn_error_t *
create_temp_file(apr_file_t **f, const svn_string_t *value, apr_pool_t *pool)
{
  apr_off_t offset = 0;

  SVN_ERR(svn_io_open_unique_file3(f, NULL, NULL,
                                   svn_io_file_del_on_pool_cleanup,
                                   pool, pool));
  SVN_ERR(svn_io_file_write_full(*f, value->data, value->len, NULL, pool));
  return svn_io_file_seek(*f, APR_SET, &offset, pool);
}


/* Check if the HOOK program exists and is a file or a symbolic link, using
   POOL for temporary allocations.

   If the hook exists but is a broken symbolic link, set *BROKEN_LINK
   to TRUE, else if the hook program exists set *BROKEN_LINK to FALSE.

   Return the hook program if found, else return NULL and don't touch
   *BROKEN_LINK.
*/
static const char*
check_hook_cmd(const char *hook, svn_boolean_t *broken_link, apr_pool_t *pool)
{
  static const char* const check_extns[] = {
#ifdef WIN32
  /* For WIN32, we need to check with file name extension(s) added.

     As Windows Scripting Host (.wsf) files can accomodate (at least)
     JavaScript (.js) and VB Script (.vbs) code, extensions for the
     corresponding file types need not be enumerated explicitly. */
    ".exe", ".cmd", ".bat", ".wsf", /* ### Any other extensions? */
#else
    "",
#endif
    NULL
  };

  const char *const *extn;
  svn_error_t *err = NULL;
  svn_boolean_t is_special;
  for (extn = check_extns; *extn; ++extn)
    {
      const char *const hook_path =
        (**extn ? apr_pstrcat(pool, hook, *extn, (char *)NULL) : hook);

      svn_node_kind_t kind;
      if (!(err = svn_io_check_resolved_path(hook_path, &kind, pool))
          && kind == svn_node_file)
        {
          *broken_link = FALSE;
          return hook_path;
        }
      svn_error_clear(err);
      if (!(err = svn_io_check_special_path(hook_path, &kind, &is_special,
                                            pool))
          && is_special)
        {
          *broken_link = TRUE;
          return hook_path;
        }
      svn_error_clear(err);
    }
  return NULL;
}


/* Return an error for the failure of HOOK due to a broken symlink. */
static svn_error_t *
hook_symlink_error(const char *hook)
{
  return svn_error_createf
    (SVN_ERR_REPOS_HOOK_FAILURE, NULL,
     _("Failed to run '%s' hook; broken symlink"), hook);
}

svn_error_t *
svn_repos__hooks_start_commit(svn_repos_t *repos,
                              const char *user,
                              const apr_array_header_t *capabilities,
                              apr_pool_t *pool)
{
  const char *hook = svn_repos_start_commit_hook(repos, pool);
  svn_boolean_t broken_link;

  if ((hook = check_hook_cmd(hook, &broken_link, pool)) && broken_link)
    {
      return hook_symlink_error(hook);
    }
  else if (hook)
    {
      const char *args[5];
      char *capabilities_string;

      if (capabilities)
        {
          capabilities_string = svn_cstring_join(capabilities, ":", pool);

          /* Get rid of that annoying final colon. */
          if (capabilities_string[0])
            capabilities_string[strlen(capabilities_string) - 1] = '\0';
        }
      else
        {
          capabilities_string = apr_pstrdup(pool, "");
        }

      args[0] = hook;
      args[1] = svn_dirent_local_style(svn_repos_path(repos, pool), pool);
      args[2] = user ? user : "";
      args[3] = capabilities_string;
      args[4] = NULL;

      SVN_ERR(run_hook_cmd(NULL, SVN_REPOS__HOOK_START_COMMIT, hook, args, NULL,
                           pool));
    }

  return SVN_NO_ERROR;
}

/* Set *HANDLE to an open filehandle for a temporary file (i.e.,
   automatically deleted when closed), into which the LOCK_TOKENS have
   been written out in the format described in the pre-commit hook
   template.

   LOCK_TOKENS is as returned by svn_fs__access_get_lock_tokens().

   Allocate *HANDLE in POOL, and use POOL for temporary allocations. */
static svn_error_t *
lock_token_content(apr_file_t **handle, apr_hash_t *lock_tokens,
                   apr_pool_t *pool)
{
  svn_stringbuf_t *lock_str = svn_stringbuf_create("LOCK-TOKENS:\n", pool);
  apr_hash_index_t *hi;

  for (hi = apr_hash_first(pool, lock_tokens); hi;
       hi = apr_hash_next(hi))
    {
      void *val;
      const char *path, *token;

      apr_hash_this(hi, (void *)&token, NULL, &val);
      path = val;
      svn_stringbuf_appendstr(lock_str,
        svn_stringbuf_createf(pool, "%s|%s\n",
                              svn_path_uri_autoescape(path, pool),
                              token));
    }

  svn_stringbuf_appendcstr(lock_str, "\n");
  return create_temp_file(handle,
                          svn_stringbuf__morph_into_string(lock_str), pool);
}



svn_error_t  *
svn_repos__hooks_pre_commit(svn_repos_t *repos,
                            const char *txn_name,
                            apr_pool_t *pool)
{
  const char *hook = svn_repos_pre_commit_hook(repos, pool);
  svn_boolean_t broken_link;

  if ((hook = check_hook_cmd(hook, &broken_link, pool)) && broken_link)
    {
      return hook_symlink_error(hook);
    }
  else if (hook)
    {
      const char *args[4];
      svn_fs_access_t *access_ctx;
      apr_file_t *stdin_handle = NULL;

      args[0] = hook;
      args[1] = svn_dirent_local_style(svn_repos_path(repos, pool), pool);
      args[2] = txn_name;
      args[3] = NULL;

      SVN_ERR(svn_fs_get_access(&access_ctx, repos->fs));
      if (access_ctx)
        {
          apr_hash_t *lock_tokens = svn_fs__access_get_lock_tokens(access_ctx);
          if (apr_hash_count(lock_tokens))  {
            SVN_ERR(lock_token_content(&stdin_handle, lock_tokens, pool));
          }
        }

      if (!stdin_handle)
        SVN_ERR(svn_io_file_open(&stdin_handle, SVN_NULL_DEVICE_NAME,
                                 APR_READ, APR_OS_DEFAULT, pool));

      SVN_ERR(run_hook_cmd(NULL, SVN_REPOS__HOOK_PRE_COMMIT, hook, args,
                           stdin_handle, pool));
    }

  return SVN_NO_ERROR;
}


svn_error_t  *
svn_repos__hooks_post_commit(svn_repos_t *repos,
                             svn_revnum_t rev,
                             apr_pool_t *pool)
{
  const char *hook = svn_repos_post_commit_hook(repos, pool);
  svn_boolean_t broken_link;

  if ((hook = check_hook_cmd(hook, &broken_link, pool)) && broken_link)
    {
      return hook_symlink_error(hook);
    }
  else if (hook)
    {
      const char *args[4];

      args[0] = hook;
      args[1] = svn_dirent_local_style(svn_repos_path(repos, pool), pool);
      args[2] = apr_psprintf(pool, "%ld", rev);
      args[3] = NULL;

      SVN_ERR(run_hook_cmd(NULL, SVN_REPOS__HOOK_POST_COMMIT, hook, args, NULL,
                           pool));
    }

  return SVN_NO_ERROR;
}


svn_error_t  *
svn_repos__hooks_pre_revprop_change(svn_repos_t *repos,
                                    svn_revnum_t rev,
                                    const char *author,
                                    const char *name,
                                    const svn_string_t *new_value,
                                    char action,
                                    apr_pool_t *pool)
{
  const char *hook = svn_repos_pre_revprop_change_hook(repos, pool);
  svn_boolean_t broken_link;

  if ((hook = check_hook_cmd(hook, &broken_link, pool)) && broken_link)
    {
      return hook_symlink_error(hook);
    }
  else if (hook)
    {
      const char *args[7];
      apr_file_t *stdin_handle = NULL;
      char action_string[2];

      /* Pass the new value as stdin to hook */
      if (new_value)
        SVN_ERR(create_temp_file(&stdin_handle, new_value, pool));
      else
        SVN_ERR(svn_io_file_open(&stdin_handle, SVN_NULL_DEVICE_NAME,
                                 APR_READ, APR_OS_DEFAULT, pool));

      action_string[0] = action;
      action_string[1] = '\0';

      args[0] = hook;
      args[1] = svn_dirent_local_style(svn_repos_path(repos, pool), pool);
      args[2] = apr_psprintf(pool, "%ld", rev);
      args[3] = author ? author : "";
      args[4] = name;
      args[5] = action_string;
      args[6] = NULL;

      SVN_ERR(run_hook_cmd(NULL, SVN_REPOS__HOOK_PRE_REVPROP_CHANGE, hook, args,
                           stdin_handle, pool));

      SVN_ERR(svn_io_file_close(stdin_handle, pool));
    }
  else
    {
      /* If the pre- hook doesn't exist at all, then default to
         MASSIVE PARANOIA.  Changing revision properties is a lossy
         operation; so unless the repository admininstrator has
         *deliberately* created the pre-hook, disallow all changes. */
      return
        svn_error_create
        (SVN_ERR_REPOS_DISABLED_FEATURE, NULL,
         _("Repository has not been enabled to accept revision propchanges;\n"
           "ask the administrator to create a pre-revprop-change hook"));
    }

  return SVN_NO_ERROR;
}


svn_error_t  *
svn_repos__hooks_post_revprop_change(svn_repos_t *repos,
                                     svn_revnum_t rev,
                                     const char *author,
                                     const char *name,
                                     const svn_string_t *old_value,
                                     char action,
                                     apr_pool_t *pool)
{
  const char *hook = svn_repos_post_revprop_change_hook(repos, pool);
  svn_boolean_t broken_link;

  if ((hook = check_hook_cmd(hook, &broken_link, pool)) && broken_link)
    {
      return hook_symlink_error(hook);
    }
  else if (hook)
    {
      const char *args[7];
      apr_file_t *stdin_handle = NULL;
      char action_string[2];

      /* Pass the old value as stdin to hook */
      if (old_value)
        SVN_ERR(create_temp_file(&stdin_handle, old_value, pool));
      else
        SVN_ERR(svn_io_file_open(&stdin_handle, SVN_NULL_DEVICE_NAME,
                                 APR_READ, APR_OS_DEFAULT, pool));

      action_string[0] = action;
      action_string[1] = '\0';

      args[0] = hook;
      args[1] = svn_dirent_local_style(svn_repos_path(repos, pool), pool);
      args[2] = apr_psprintf(pool, "%ld", rev);
      args[3] = author ? author : "";
      args[4] = name;
      args[5] = action_string;
      args[6] = NULL;

      SVN_ERR(run_hook_cmd(NULL, SVN_REPOS__HOOK_POST_REVPROP_CHANGE, hook,
                           args, stdin_handle, pool));

      SVN_ERR(svn_io_file_close(stdin_handle, pool));
    }

  return SVN_NO_ERROR;
}


svn_error_t  *
svn_repos__hooks_pre_lock(svn_repos_t *repos,
                          const char **token,
                          const char *path,
                          const char *username,
                          const char *comment,
                          svn_boolean_t steal_lock,
                          apr_pool_t *pool)
{
  const char *hook = svn_repos_pre_lock_hook(repos, pool);
  svn_boolean_t broken_link;

  if ((hook = check_hook_cmd(hook, &broken_link, pool)) && broken_link)
    {
      return hook_symlink_error(hook);
    }
  else if (hook)
    {
      const char *args[7];
      svn_string_t *buf;

      args[0] = hook;
      args[1] = svn_dirent_local_style(svn_repos_path(repos, pool), pool);
      args[2] = path;
      args[3] = username;
      args[4] = comment ? comment : "";
      args[5] = steal_lock ? "1" : "0";
      args[6] = NULL;

      SVN_ERR(run_hook_cmd(&buf, SVN_REPOS__HOOK_PRE_LOCK, hook, args, NULL,
                           pool));

      if (token)
        /* No validation here; the FS will take care of that. */
        *token = buf->data;

    }
  else if (token)
    *token = "";

  return SVN_NO_ERROR;
}


svn_error_t  *
svn_repos__hooks_post_lock(svn_repos_t *repos,
                           const apr_array_header_t *paths,
                           const char *username,
                           apr_pool_t *pool)
{
  const char *hook = svn_repos_post_lock_hook(repos, pool);
  svn_boolean_t broken_link;

  if ((hook = check_hook_cmd(hook, &broken_link, pool)) && broken_link)
    {
      return hook_symlink_error(hook);
    }
  else if (hook)
    {
      const char *args[5];
      apr_file_t *stdin_handle = NULL;
      svn_string_t *paths_str = svn_string_create(svn_cstring_join
                                                  (paths, "\n", pool),
                                                  pool);

      SVN_ERR(create_temp_file(&stdin_handle, paths_str, pool));

      args[0] = hook;
      args[1] = svn_dirent_local_style(svn_repos_path(repos, pool), pool);
      args[2] = username;
      args[3] = NULL;
      args[4] = NULL;

      SVN_ERR(run_hook_cmd(NULL, SVN_REPOS__HOOK_POST_LOCK, hook, args,
                           stdin_handle, pool));

      SVN_ERR(svn_io_file_close(stdin_handle, pool));
    }

  return SVN_NO_ERROR;
}


svn_error_t  *
svn_repos__hooks_pre_unlock(svn_repos_t *repos,
                            const char *path,
                            const char *username,
                            const char *token,
                            svn_boolean_t break_lock,
                            apr_pool_t *pool)
{
  const char *hook = svn_repos_pre_unlock_hook(repos, pool);
  svn_boolean_t broken_link;

  if ((hook = check_hook_cmd(hook, &broken_link, pool)) && broken_link)
    {
      return hook_symlink_error(hook);
    }
  else if (hook)
    {
      const char *args[7];

      args[0] = hook;
      args[1] = svn_dirent_local_style(svn_repos_path(repos, pool), pool);
      args[2] = path;
      args[3] = username ? username : "";
      args[4] = token ? token : "";
      args[5] = break_lock ? "1" : "0";
      args[6] = NULL;

      SVN_ERR(run_hook_cmd(NULL, SVN_REPOS__HOOK_PRE_UNLOCK, hook, args, NULL,
                           pool));
    }

  return SVN_NO_ERROR;
}


svn_error_t  *
svn_repos__hooks_post_unlock(svn_repos_t *repos,
                             const apr_array_header_t *paths,
                             const char *username,
                             apr_pool_t *pool)
{
  const char *hook = svn_repos_post_unlock_hook(repos, pool);
  svn_boolean_t broken_link;

  if ((hook = check_hook_cmd(hook, &broken_link, pool)) && broken_link)
    {
      return hook_symlink_error(hook);
    }
  else if (hook)
    {
      const char *args[5];
      apr_file_t *stdin_handle = NULL;
      svn_string_t *paths_str = svn_string_create(svn_cstring_join
                                                  (paths, "\n", pool),
                                                  pool);

      SVN_ERR(create_temp_file(&stdin_handle, paths_str, pool));

      args[0] = hook;
      args[1] = svn_dirent_local_style(svn_repos_path(repos, pool), pool);
      args[2] = username ? username : "";
      args[3] = NULL;
      args[4] = NULL;

      SVN_ERR(run_hook_cmd(NULL, SVN_REPOS__HOOK_POST_UNLOCK, hook, args,
                           stdin_handle, pool));

      SVN_ERR(svn_io_file_close(stdin_handle, pool));
    }

  return SVN_NO_ERROR;
}



/*
 * vim:ts=4:sw=4:expandtab:tw=80:fo=tcroq
 * vim:isk=a-z,A-Z,48-57,_,.,-,>
 * vim:cino=>1s,e0,n0,f0,{.5s,}0,^-.5s,=.5s,t0,+1s,c3,(0,u0,\:0
 */

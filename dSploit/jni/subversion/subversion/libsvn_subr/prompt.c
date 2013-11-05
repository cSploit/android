/*
 * prompt.c -- ask the user for authentication information.
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

/* ==================================================================== */



/*** Includes. ***/

#include <apr_lib.h>
#include <apr_poll.h>

#include "svn_cmdline.h"
#include "svn_string.h"
#include "svn_auth.h"
#include "svn_error.h"
#include "svn_path.h"

#include "private/svn_cmdline_private.h"
#include "svn_private_config.h"



/* Wait for input on @a *f.  Doing all allocations
 * in @a pool.  This functions is based on apr_wait_for_io_or_timeout().
 * Note that this will return an EINTR on a signal.
 *
 * ### FIX: When APR gives us a better way of doing this use it. */
static apr_status_t wait_for_input(apr_file_t *f,
                                   apr_pool_t *pool)
{
#ifndef WIN32
  apr_pollfd_t pollset;
  int srv, n;

  pollset.desc_type = APR_POLL_FILE;
  pollset.desc.f = f;
  pollset.p = pool;
  pollset.reqevents = APR_POLLIN;

  srv = apr_poll(&pollset, 1, &n, -1);

  if (n == 1 && pollset.rtnevents & APR_POLLIN)
    return APR_SUCCESS;

  return srv;
#else
  /* APR specs say things that are unimplemented are supposed to return
   * APR_ENOTIMPL.  But when trying to use APR_POLL_FILE with apr_poll
   * on Windows it returns APR_EBADF instead.  So just return APR_ENOTIMPL
   * ourselves here.
   */
  return APR_ENOTIMPL;
#endif
}

/* Set @a *result to the result of prompting the user with @a
 * prompt_msg.  Use @ *pb to get the cancel_func and cancel_baton.
 * Do not call the cancel_func if @a *pb is NULL.
 * Allocate @a *result in @a pool.
 *
 * If @a hide is true, then try to avoid displaying the user's input.
 */
static svn_error_t *
prompt(const char **result,
       const char *prompt_msg,
       svn_boolean_t hide,
       svn_cmdline_prompt_baton2_t *pb,
       apr_pool_t *pool)
{
  /* XXX: If this functions ever starts using members of *pb
   * which were not included in svn_cmdline_prompt_baton_t,
   * we need to update svn_cmdline_prompt_user2 and its callers. */
  apr_status_t status;
  apr_file_t *fp;
  char c;

  svn_stringbuf_t *strbuf = svn_stringbuf_create("", pool);

  status = apr_file_open_stdin(&fp, pool);
  if (status)
    return svn_error_wrap_apr(status, _("Can't open stdin"));

  if (! hide)
    {
      svn_boolean_t saw_first_half_of_eol = FALSE;
      SVN_ERR(svn_cmdline_fputs(prompt_msg, stderr, pool));
      fflush(stderr);

      while (1)
        {
          /* Hack to allow us to not block for io on the prompt, so
           * we can cancel. */
          if (pb)
            SVN_ERR(pb->cancel_func(pb->cancel_baton));
          status = wait_for_input(fp, pool);
          if (APR_STATUS_IS_EINTR(status))
            continue;
          else if (status && status != APR_ENOTIMPL)
            return svn_error_wrap_apr(status, _("Can't read stdin"));

          status = apr_file_getc(&c, fp);
          if (status)
            return svn_error_wrap_apr(status, _("Can't read stdin"));

          if (saw_first_half_of_eol)
            {
              if (c == APR_EOL_STR[1])
                break;
              else
                saw_first_half_of_eol = FALSE;
            }
          else if (c == APR_EOL_STR[0])
            {
              /* GCC might complain here: "warning: will never be executed"
               * That's fine. This is a compile-time check for "\r\n\0" */
              if (sizeof(APR_EOL_STR) == 3)
                {
                  saw_first_half_of_eol = TRUE;
                  continue;
                }
              else if (sizeof(APR_EOL_STR) == 2)
                break;
              else
                /* ### APR_EOL_STR holds more than two chars?  Who
                   ever heard of such a thing? */
                SVN_ERR_MALFUNCTION();
            }

          svn_stringbuf_appendbyte(strbuf, c);
        }
    }
  else
    {
      const char *prompt_stdout;
      size_t bufsize = 300;
      SVN_ERR(svn_cmdline_cstring_from_utf8(&prompt_stdout, prompt_msg,
                                            pool));
      svn_stringbuf_ensure(strbuf, bufsize);

      status = apr_password_get(prompt_stdout, strbuf->data, &bufsize);
      if (status)
        return svn_error_wrap_apr(status, _("Can't get password"));
    }

  return svn_cmdline_cstring_to_utf8(result, strbuf->data, pool);
}



/** Prompt functions for auth providers. **/

/* Helper function for auth provider prompters: mention the
 * authentication @a realm on stderr, in a manner appropriate for
 * preceding a prompt; or if @a realm is null, then do nothing.
 */
static svn_error_t *
maybe_print_realm(const char *realm, apr_pool_t *pool)
{
  if (realm)
    {
      SVN_ERR(svn_cmdline_fprintf(stderr, pool,
                                  _("Authentication realm: %s\n"), realm));
      fflush(stderr);
    }

  return SVN_NO_ERROR;
}


/* This implements 'svn_auth_simple_prompt_func_t'. */
svn_error_t *
svn_cmdline_auth_simple_prompt(svn_auth_cred_simple_t **cred_p,
                               void *baton,
                               const char *realm,
                               const char *username,
                               svn_boolean_t may_save,
                               apr_pool_t *pool)
{
  svn_auth_cred_simple_t *ret = apr_pcalloc(pool, sizeof(*ret));
  const char *pass_prompt;
  svn_cmdline_prompt_baton2_t *pb = baton;

  SVN_ERR(maybe_print_realm(realm, pool));

  if (username)
    ret->username = apr_pstrdup(pool, username);
  else
    SVN_ERR(prompt(&(ret->username), _("Username: "), FALSE, pb, pool));

  pass_prompt = apr_psprintf(pool, _("Password for '%s': "), ret->username);
  SVN_ERR(prompt(&(ret->password), pass_prompt, TRUE, pb, pool));
  ret->may_save = may_save;
  *cred_p = ret;
  return SVN_NO_ERROR;
}


/* This implements 'svn_auth_username_prompt_func_t'. */
svn_error_t *
svn_cmdline_auth_username_prompt(svn_auth_cred_username_t **cred_p,
                                 void *baton,
                                 const char *realm,
                                 svn_boolean_t may_save,
                                 apr_pool_t *pool)
{
  svn_auth_cred_username_t *ret = apr_pcalloc(pool, sizeof(*ret));
  svn_cmdline_prompt_baton2_t *pb = baton;

  SVN_ERR(maybe_print_realm(realm, pool));

  SVN_ERR(prompt(&(ret->username), _("Username: "), FALSE, pb, pool));
  ret->may_save = may_save;
  *cred_p = ret;
  return SVN_NO_ERROR;
}


/* This implements 'svn_auth_ssl_server_trust_prompt_func_t'. */
svn_error_t *
svn_cmdline_auth_ssl_server_trust_prompt
  (svn_auth_cred_ssl_server_trust_t **cred_p,
   void *baton,
   const char *realm,
   apr_uint32_t failures,
   const svn_auth_ssl_server_cert_info_t *cert_info,
   svn_boolean_t may_save,
   apr_pool_t *pool)
{
  const char *choice;
  svn_stringbuf_t *msg;
  svn_cmdline_prompt_baton2_t *pb = baton;
  svn_stringbuf_t *buf = svn_stringbuf_createf
    (pool, _("Error validating server certificate for '%s':\n"), realm);

  if (failures & SVN_AUTH_SSL_UNKNOWNCA)
    {
      svn_stringbuf_appendcstr
        (buf,
         _(" - The certificate is not issued by a trusted authority. Use the\n"
           "   fingerprint to validate the certificate manually!\n"));
    }

  if (failures & SVN_AUTH_SSL_CNMISMATCH)
    {
      svn_stringbuf_appendcstr
        (buf, _(" - The certificate hostname does not match.\n"));
    }

  if (failures & SVN_AUTH_SSL_NOTYETVALID)
    {
      svn_stringbuf_appendcstr
        (buf, _(" - The certificate is not yet valid.\n"));
    }

  if (failures & SVN_AUTH_SSL_EXPIRED)
    {
      svn_stringbuf_appendcstr
        (buf, _(" - The certificate has expired.\n"));
    }

  if (failures & SVN_AUTH_SSL_OTHER)
    {
      svn_stringbuf_appendcstr
        (buf, _(" - The certificate has an unknown error.\n"));
    }

  msg = svn_stringbuf_createf
    (pool,
     _("Certificate information:\n"
       " - Hostname: %s\n"
       " - Valid: from %s until %s\n"
       " - Issuer: %s\n"
       " - Fingerprint: %s\n"),
     cert_info->hostname,
     cert_info->valid_from,
     cert_info->valid_until,
     cert_info->issuer_dname,
     cert_info->fingerprint);
  svn_stringbuf_appendstr(buf, msg);

  if (may_save)
    {
      svn_stringbuf_appendcstr
        (buf, _("(R)eject, accept (t)emporarily or accept (p)ermanently? "));
    }
  else
    {
      svn_stringbuf_appendcstr(buf, _("(R)eject or accept (t)emporarily? "));
    }
  SVN_ERR(prompt(&choice, buf->data, FALSE, pb, pool));

  if (choice[0] == 't' || choice[0] == 'T')
    {
      *cred_p = apr_pcalloc(pool, sizeof(**cred_p));
      (*cred_p)->may_save = FALSE;
      (*cred_p)->accepted_failures = failures;
    }
  else if (may_save && (choice[0] == 'p' || choice[0] == 'P'))
    {
      *cred_p = apr_pcalloc(pool, sizeof(**cred_p));
      (*cred_p)->may_save = TRUE;
      (*cred_p)->accepted_failures = failures;
    }
  else
    {
      *cred_p = NULL;
    }

  return SVN_NO_ERROR;
}


/* This implements 'svn_auth_ssl_client_cert_prompt_func_t'. */
svn_error_t *
svn_cmdline_auth_ssl_client_cert_prompt
  (svn_auth_cred_ssl_client_cert_t **cred_p,
   void *baton,
   const char *realm,
   svn_boolean_t may_save,
   apr_pool_t *pool)
{
  svn_auth_cred_ssl_client_cert_t *cred = NULL;
  const char *cert_file = NULL;
  const char *abs_cert_file = NULL;
  svn_cmdline_prompt_baton2_t *pb = baton;

  SVN_ERR(maybe_print_realm(realm, pool));
  SVN_ERR(prompt(&cert_file, _("Client certificate filename: "),
                 FALSE, pb, pool));
  SVN_ERR(svn_dirent_get_absolute(&abs_cert_file, cert_file, pool));

  cred = apr_palloc(pool, sizeof(*cred));
  cred->cert_file = abs_cert_file;
  cred->may_save = may_save;
  *cred_p = cred;

  return SVN_NO_ERROR;
}


/* This implements 'svn_auth_ssl_client_cert_pw_prompt_func_t'. */
svn_error_t *
svn_cmdline_auth_ssl_client_cert_pw_prompt
  (svn_auth_cred_ssl_client_cert_pw_t **cred_p,
   void *baton,
   const char *realm,
   svn_boolean_t may_save,
   apr_pool_t *pool)
{
  svn_auth_cred_ssl_client_cert_pw_t *cred = NULL;
  const char *result;
  const char *text = apr_psprintf(pool, _("Passphrase for '%s': "), realm);
  svn_cmdline_prompt_baton2_t *pb = baton;

  SVN_ERR(prompt(&result, text, TRUE, pb, pool));

  cred = apr_pcalloc(pool, sizeof(*cred));
  cred->password = result;
  cred->may_save = may_save;
  *cred_p = cred;

  return SVN_NO_ERROR;
}

/* This is a helper for plaintext prompt functions. */
static svn_error_t *
plaintext_prompt_helper(svn_boolean_t *may_save_plaintext,
                        const char *realmstring,
                        const char *prompt_string,
                        const char *prompt_text,
                        void *baton,
                        apr_pool_t *pool)
{
  const char *answer = NULL;
  svn_boolean_t answered = FALSE;
  svn_cmdline_prompt_baton2_t *pb = baton;
  const char *config_path = NULL;

  if (pb)
    SVN_ERR(svn_config_get_user_config_path(&config_path, pb->config_dir,
                                            SVN_CONFIG_CATEGORY_SERVERS, pool));

  SVN_ERR(svn_cmdline_fprintf(stderr, pool, prompt_text, realmstring,
                              config_path));

  do
    {
      svn_error_t *err = prompt(&answer, prompt_string, FALSE, pb, pool);
      if (err)
        {
          if (err->apr_err == SVN_ERR_CANCELLED)
            {
              svn_error_clear(err);
              *may_save_plaintext = FALSE;
              return SVN_NO_ERROR;
            }
          else
            return err;
        }
      if (apr_strnatcasecmp(answer, _("yes")) == 0 ||
          apr_strnatcasecmp(answer, _("y")) == 0)
        {
          *may_save_plaintext = TRUE;
          answered = TRUE;
        }
      else if (apr_strnatcasecmp(answer, _("no")) == 0 ||
               apr_strnatcasecmp(answer, _("n")) == 0)
        {
          *may_save_plaintext = FALSE;
          answered = TRUE;
        }
      else
          prompt_string = _("Please type 'yes' or 'no': ");
    }
  while (! answered);

  return SVN_NO_ERROR;
}

/* This implements 'svn_auth_plaintext_prompt_func_t'. */
svn_error_t *
svn_cmdline_auth_plaintext_prompt(svn_boolean_t *may_save_plaintext,
                                  const char *realmstring,
                                  void *baton,
                                  apr_pool_t *pool)
{
  const char *prompt_string = _("Store password unencrypted (yes/no)? ");
  const char *prompt_text =
  _("\n-----------------------------------------------------------------------"
    "\nATTENTION!  Your password for authentication realm:\n"
    "\n"
    "   %s\n"
    "\n"
    "can only be stored to disk unencrypted!  You are advised to configure\n"
    "your system so that Subversion can store passwords encrypted, if\n"
    "possible.  See the documentation for details.\n"
    "\n"
    "You can avoid future appearances of this warning by setting the value\n"
    "of the 'store-plaintext-passwords' option to either 'yes' or 'no' in\n"
    "'%s'.\n"
    "-----------------------------------------------------------------------\n"
    );

  return plaintext_prompt_helper(may_save_plaintext, realmstring,
                                 prompt_string, prompt_text, baton,
                                 pool);
}

/* This implements 'svn_auth_plaintext_passphrase_prompt_func_t'. */
svn_error_t *
svn_cmdline_auth_plaintext_passphrase_prompt(svn_boolean_t *may_save_plaintext,
                                             const char *realmstring,
                                             void *baton,
                                             apr_pool_t *pool)
{
  const char *prompt_string = _("Store passphrase unencrypted (yes/no)? ");
  const char *prompt_text =
  _("\n-----------------------------------------------------------------------\n"
    "ATTENTION!  Your passphrase for client certificate:\n"
    "\n"
    "   %s\n"
    "\n"
    "can only be stored to disk unencrypted!  You are advised to configure\n"
    "your system so that Subversion can store passphrase encrypted, if\n"
    "possible.  See the documentation for details.\n"
    "\n"
    "You can avoid future appearances of this warning by setting the value\n"
    "of the 'store-ssl-client-cert-pp-plaintext' option to either 'yes' or\n"
    "'no' in '%s'.\n"
    "-----------------------------------------------------------------------\n"
    );

  return plaintext_prompt_helper(may_save_plaintext, realmstring,
                                 prompt_string, prompt_text, baton,
                                 pool);
}


/** Generic prompting. **/

svn_error_t *
svn_cmdline_prompt_user2(const char **result,
                         const char *prompt_str,
                         svn_cmdline_prompt_baton_t *baton,
                         apr_pool_t *pool)
{
  /* XXX: We know prompt doesn't use the new members
   * of svn_cmdline_prompt_baton2_t. */
  return prompt(result, prompt_str, FALSE /* don't hide input */,
                (svn_cmdline_prompt_baton2_t *)baton, pool);
}

/* This implements 'svn_auth_gnome_keyring_unlock_prompt_func_t'. */
svn_error_t *
svn_cmdline__auth_gnome_keyring_unlock_prompt(char **keyring_password,
                                              const char *keyring_name,
                                              void *baton,
                                              apr_pool_t *pool)
{
  const char *password;
  const char *pass_prompt;
  svn_cmdline_prompt_baton2_t *pb = baton;

  pass_prompt = apr_psprintf(pool, _("Password for '%s' GNOME keyring: "),
                             keyring_name);
  SVN_ERR(prompt(&password, pass_prompt, TRUE, pb, pool));
  *keyring_password = apr_pstrdup(pool, password);
  return SVN_NO_ERROR;
}

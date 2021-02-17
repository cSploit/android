/*
 * deprecated.c:  holding file for all deprecated APIs.
 *                "we can't lose 'em, but we can shun 'em!"
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

/* We define this here to remove any further warnings about the usage of
   deprecated functions in this file. */
#define SVN_DEPRECATED

#include "svn_repos.h"
#include "svn_compat.h"
#include "svn_props.h"

#include "svn_private_config.h"




/*** From commit.c ***/

svn_error_t *
svn_repos_get_commit_editor4(const svn_delta_editor_t **editor,
                             void **edit_baton,
                             svn_repos_t *repos,
                             svn_fs_txn_t *txn,
                             const char *repos_url,
                             const char *base_path,
                             const char *user,
                             const char *log_msg,
                             svn_commit_callback2_t callback,
                             void *callback_baton,
                             svn_repos_authz_callback_t authz_callback,
                             void *authz_baton,
                             apr_pool_t *pool)
{
  apr_hash_t *revprop_table = apr_hash_make(pool);
  if (user)
    apr_hash_set(revprop_table, SVN_PROP_REVISION_AUTHOR,
                 APR_HASH_KEY_STRING,
                 svn_string_create(user, pool));
  if (log_msg)
    apr_hash_set(revprop_table, SVN_PROP_REVISION_LOG,
                 APR_HASH_KEY_STRING,
                 svn_string_create(log_msg, pool));
  return svn_repos_get_commit_editor5(editor, edit_baton, repos, txn,
                                      repos_url, base_path, revprop_table,
                                      callback, callback_baton,
                                      authz_callback, authz_baton, pool);
}


svn_error_t *
svn_repos_get_commit_editor3(const svn_delta_editor_t **editor,
                             void **edit_baton,
                             svn_repos_t *repos,
                             svn_fs_txn_t *txn,
                             const char *repos_url,
                             const char *base_path,
                             const char *user,
                             const char *log_msg,
                             svn_commit_callback_t callback,
                             void *callback_baton,
                             svn_repos_authz_callback_t authz_callback,
                             void *authz_baton,
                             apr_pool_t *pool)
{
  svn_commit_callback2_t callback2;
  void *callback2_baton;

  svn_compat_wrap_commit_callback(&callback2, &callback2_baton,
                                  callback, callback_baton,
                                  pool);

  return svn_repos_get_commit_editor4(editor, edit_baton, repos, txn,
                                      repos_url, base_path, user,
                                      log_msg, callback2,
                                      callback2_baton, authz_callback,
                                      authz_baton, pool);
}


svn_error_t *
svn_repos_get_commit_editor2(const svn_delta_editor_t **editor,
                             void **edit_baton,
                             svn_repos_t *repos,
                             svn_fs_txn_t *txn,
                             const char *repos_url,
                             const char *base_path,
                             const char *user,
                             const char *log_msg,
                             svn_commit_callback_t callback,
                             void *callback_baton,
                             apr_pool_t *pool)
{
  return svn_repos_get_commit_editor3(editor, edit_baton, repos, txn,
                                      repos_url, base_path, user,
                                      log_msg, callback, callback_baton,
                                      NULL, NULL, pool);
}


svn_error_t *
svn_repos_get_commit_editor(const svn_delta_editor_t **editor,
                            void **edit_baton,
                            svn_repos_t *repos,
                            const char *repos_url,
                            const char *base_path,
                            const char *user,
                            const char *log_msg,
                            svn_commit_callback_t callback,
                            void *callback_baton,
                            apr_pool_t *pool)
{
  return svn_repos_get_commit_editor2(editor, edit_baton, repos, NULL,
                                      repos_url, base_path, user,
                                      log_msg, callback,
                                      callback_baton, pool);
}

svn_error_t *
svn_repos_open(svn_repos_t **repos_p,
               const char *path,
               apr_pool_t *pool)
{
  return svn_repos_open2(repos_p, path, NULL, pool);
}


/*** From repos.c ***/
struct recover_baton
{
  svn_error_t *(*start_callback)(void *baton);
  void *start_callback_baton;
};

static void
recovery_started(void *baton,
                 const svn_repos_notify_t *notify,
                 apr_pool_t *scratch_pool)
{
  struct recover_baton *rb = baton;

  if (notify->action == svn_repos_notify_mutex_acquired
      && rb->start_callback != NULL)
    svn_error_clear(rb->start_callback(rb->start_callback_baton));
}

svn_error_t *
svn_repos_recover3(const char *path,
                   svn_boolean_t nonblocking,
                   svn_error_t *(*start_callback)(void *baton),
                   void *start_callback_baton,
                   svn_cancel_func_t cancel_func, void *cancel_baton,
                   apr_pool_t *pool)
{
  struct recover_baton rb;

  rb.start_callback = start_callback;
  rb.start_callback_baton = start_callback_baton;

  return svn_repos_recover4(path, nonblocking, recovery_started, &rb,
                            cancel_func, cancel_baton, pool);
}

svn_error_t *
svn_repos_recover2(const char *path,
                   svn_boolean_t nonblocking,
                   svn_error_t *(*start_callback)(void *baton),
                   void *start_callback_baton,
                   apr_pool_t *pool)
{
  return svn_repos_recover3(path, nonblocking,
                            start_callback, start_callback_baton,
                            NULL, NULL,
                            pool);
}

svn_error_t *
svn_repos_recover(const char *path,
                  apr_pool_t *pool)
{
  return svn_repos_recover2(path, FALSE, NULL, NULL, pool);
}

svn_error_t *
svn_repos_upgrade(const char *path,
                  svn_boolean_t nonblocking,
                  svn_error_t *(*start_callback)(void *baton),
                  void *start_callback_baton,
                  apr_pool_t *pool)
{
  struct recover_baton rb;

  rb.start_callback = start_callback;
  rb.start_callback_baton = start_callback_baton;

  return svn_repos_upgrade2(path, nonblocking, recovery_started, &rb, pool);
}

/*** From reporter.c ***/
svn_error_t *
svn_repos_begin_report(void **report_baton,
                       svn_revnum_t revnum,
                       const char *username,
                       svn_repos_t *repos,
                       const char *fs_base,
                       const char *s_operand,
                       const char *switch_path,
                       svn_boolean_t text_deltas,
                       svn_boolean_t recurse,
                       svn_boolean_t ignore_ancestry,
                       const svn_delta_editor_t *editor,
                       void *edit_baton,
                       svn_repos_authz_func_t authz_read_func,
                       void *authz_read_baton,
                       apr_pool_t *pool)
{
  return svn_repos_begin_report2(report_baton,
                                 revnum,
                                 repos,
                                 fs_base,
                                 s_operand,
                                 switch_path,
                                 text_deltas,
                                 SVN_DEPTH_INFINITY_OR_FILES(recurse),
                                 ignore_ancestry,
                                 FALSE, /* don't send copyfrom args */
                                 editor,
                                 edit_baton,
                                 authz_read_func,
                                 authz_read_baton,
                                 pool);
}

svn_error_t *
svn_repos_set_path2(void *baton, const char *path, svn_revnum_t rev,
                    svn_boolean_t start_empty, const char *lock_token,
                    apr_pool_t *pool)
{
  return svn_repos_set_path3(baton, path, rev, svn_depth_infinity,
                             start_empty, lock_token, pool);
}

svn_error_t *
svn_repos_set_path(void *baton, const char *path, svn_revnum_t rev,
                   svn_boolean_t start_empty, apr_pool_t *pool)
{
  return svn_repos_set_path2(baton, path, rev, start_empty, NULL, pool);
}

svn_error_t *
svn_repos_link_path2(void *baton, const char *path, const char *link_path,
                     svn_revnum_t rev, svn_boolean_t start_empty,
                     const char *lock_token, apr_pool_t *pool)
{
  return svn_repos_link_path3(baton, path, link_path, rev, svn_depth_infinity,
                              start_empty, lock_token, pool);
}

svn_error_t *
svn_repos_link_path(void *baton, const char *path, const char *link_path,
                    svn_revnum_t rev, svn_boolean_t start_empty,
                    apr_pool_t *pool)
{
  return svn_repos_link_path2(baton, path, link_path, rev, start_empty,
                              NULL, pool);
}

/*** From dir-delta.c ***/
svn_error_t *
svn_repos_dir_delta(svn_fs_root_t *src_root,
                    const char *src_parent_dir,
                    const char *src_entry,
                    svn_fs_root_t *tgt_root,
                    const char *tgt_fullpath,
                    const svn_delta_editor_t *editor,
                    void *edit_baton,
                    svn_repos_authz_func_t authz_read_func,
                    void *authz_read_baton,
                    svn_boolean_t text_deltas,
                    svn_boolean_t recurse,
                    svn_boolean_t entry_props,
                    svn_boolean_t ignore_ancestry,
                    apr_pool_t *pool)
{
  return svn_repos_dir_delta2(src_root,
                              src_parent_dir,
                              src_entry,
                              tgt_root,
                              tgt_fullpath,
                              editor,
                              edit_baton,
                              authz_read_func,
                              authz_read_baton,
                              text_deltas,
                              SVN_DEPTH_INFINITY_OR_FILES(recurse),
                              entry_props,
                              ignore_ancestry,
                              pool);
}

/*** From replay.c ***/
svn_error_t *
svn_repos_replay(svn_fs_root_t *root,
                 const svn_delta_editor_t *editor,
                 void *edit_baton,
                 apr_pool_t *pool)
{
  return svn_repos_replay2(root,
                           "" /* the whole tree */,
                           SVN_INVALID_REVNUM, /* no low water mark */
                           FALSE /* no text deltas */,
                           editor, edit_baton,
                           NULL /* no authz func */,
                           NULL /* no authz baton */,
                           pool);
}

/*** From fs-wrap.c ***/
svn_error_t *
svn_repos_fs_change_rev_prop3(svn_repos_t *repos,
                              svn_revnum_t rev,
                              const char *author,
                              const char *name,
                              const svn_string_t *new_value,
                              svn_boolean_t use_pre_revprop_change_hook,
                              svn_boolean_t use_post_revprop_change_hook,
                              svn_repos_authz_func_t authz_read_func,
                              void *authz_read_baton,
                              apr_pool_t *pool)
{
  return svn_repos_fs_change_rev_prop4(repos, rev, author, name, NULL,
                                       new_value,
                                       use_pre_revprop_change_hook,
                                       use_post_revprop_change_hook,
                                       authz_read_func,
                                       authz_read_baton, pool);
}

svn_error_t *
svn_repos_fs_change_rev_prop2(svn_repos_t *repos,
                              svn_revnum_t rev,
                              const char *author,
                              const char *name,
                              const svn_string_t *new_value,
                              svn_repos_authz_func_t authz_read_func,
                              void *authz_read_baton,
                              apr_pool_t *pool)
{
  return svn_repos_fs_change_rev_prop3(repos, rev, author, name, new_value,
                                       TRUE, TRUE, authz_read_func,
                                       authz_read_baton, pool);
}



svn_error_t *
svn_repos_fs_change_rev_prop(svn_repos_t *repos,
                             svn_revnum_t rev,
                             const char *author,
                             const char *name,
                             const svn_string_t *new_value,
                             apr_pool_t *pool)
{
  return svn_repos_fs_change_rev_prop2(repos, rev, author, name, new_value,
                                       NULL, NULL, pool);
}

struct pack_notify_wrapper_baton
{
  svn_fs_pack_notify_t notify_func;
  void *notify_baton;
};

static void
pack_notify_wrapper_func(void *baton,
                         const svn_repos_notify_t *notify,
                         apr_pool_t *scratch_pool)
{
  struct pack_notify_wrapper_baton *pnwb = baton;

  svn_error_clear(pnwb->notify_func(pnwb->notify_baton, notify->shard,
                                    notify->action - 3, scratch_pool));
}

svn_error_t *
svn_repos_fs_pack(svn_repos_t *repos,
                  svn_fs_pack_notify_t notify_func,
                  void *notify_baton,
                  svn_cancel_func_t cancel_func,
                  void *cancel_baton,
                  apr_pool_t *pool)
{
  struct pack_notify_wrapper_baton pnwb;

  pnwb.notify_func = notify_func;
  pnwb.notify_baton = notify_baton;

  return svn_repos_fs_pack2(repos, pack_notify_wrapper_func, &pnwb,
                            cancel_func, cancel_baton, pool);
}


svn_error_t *
svn_repos_fs_get_locks(apr_hash_t **locks,
                       svn_repos_t *repos,
                       const char *path,
                       svn_repos_authz_func_t authz_read_func,
                       void *authz_read_baton,
                       apr_pool_t *pool)
{
  return svn_error_trace(svn_repos_fs_get_locks2(locks, repos, path,
                                                 svn_depth_infinity,
                                                 authz_read_func,
                                                 authz_read_baton, pool));
}


/*** From logs.c ***/
svn_error_t *
svn_repos_get_logs3(svn_repos_t *repos,
                    const apr_array_header_t *paths,
                    svn_revnum_t start,
                    svn_revnum_t end,
                    int limit,
                    svn_boolean_t discover_changed_paths,
                    svn_boolean_t strict_node_history,
                    svn_repos_authz_func_t authz_read_func,
                    void *authz_read_baton,
                    svn_log_message_receiver_t receiver,
                    void *receiver_baton,
                    apr_pool_t *pool)
{
  svn_log_entry_receiver_t receiver2;
  void *receiver2_baton;

  svn_compat_wrap_log_receiver(&receiver2, &receiver2_baton,
                               receiver, receiver_baton,
                               pool);

  return svn_repos_get_logs4(repos, paths, start, end, limit,
                             discover_changed_paths, strict_node_history,
                             FALSE, svn_compat_log_revprops_in(pool),
                             authz_read_func, authz_read_baton,
                             receiver2, receiver2_baton,
                             pool);
}

svn_error_t *
svn_repos_get_logs2(svn_repos_t *repos,
                    const apr_array_header_t *paths,
                    svn_revnum_t start,
                    svn_revnum_t end,
                    svn_boolean_t discover_changed_paths,
                    svn_boolean_t strict_node_history,
                    svn_repos_authz_func_t authz_read_func,
                    void *authz_read_baton,
                    svn_log_message_receiver_t receiver,
                    void *receiver_baton,
                    apr_pool_t *pool)
{
  return svn_repos_get_logs3(repos, paths, start, end, 0,
                             discover_changed_paths, strict_node_history,
                             authz_read_func, authz_read_baton, receiver,
                             receiver_baton, pool);
}


svn_error_t *
svn_repos_get_logs(svn_repos_t *repos,
                   const apr_array_header_t *paths,
                   svn_revnum_t start,
                   svn_revnum_t end,
                   svn_boolean_t discover_changed_paths,
                   svn_boolean_t strict_node_history,
                   svn_log_message_receiver_t receiver,
                   void *receiver_baton,
                   apr_pool_t *pool)
{
  return svn_repos_get_logs3(repos, paths, start, end, 0,
                             discover_changed_paths, strict_node_history,
                             NULL, NULL, /* no authz stuff */
                             receiver, receiver_baton, pool);
}

/*** From rev_hunt.c ***/
svn_error_t *
svn_repos_history(svn_fs_t *fs,
                  const char *path,
                  svn_repos_history_func_t history_func,
                  void *history_baton,
                  svn_revnum_t start,
                  svn_revnum_t end,
                  svn_boolean_t cross_copies,
                  apr_pool_t *pool)
{
  return svn_repos_history2(fs, path, history_func, history_baton,
                            NULL, NULL,
                            start, end, cross_copies, pool);
}

svn_error_t *
svn_repos_get_file_revs(svn_repos_t *repos,
                        const char *path,
                        svn_revnum_t start,
                        svn_revnum_t end,
                        svn_repos_authz_func_t authz_read_func,
                        void *authz_read_baton,
                        svn_repos_file_rev_handler_t handler,
                        void *handler_baton,
                        apr_pool_t *pool)
{
  svn_file_rev_handler_t handler2;
  void *handler2_baton;

  svn_compat_wrap_file_rev_handler(&handler2, &handler2_baton, handler,
                                   handler_baton, pool);

  return svn_repos_get_file_revs2(repos, path, start, end, FALSE,
                                  authz_read_func, authz_read_baton,
                                  handler2, handler2_baton, pool);
}

/*** From dump.c ***/
svn_error_t *
svn_repos_dump_fs(svn_repos_t *repos,
                  svn_stream_t *stream,
                  svn_stream_t *feedback_stream,
                  svn_revnum_t start_rev,
                  svn_revnum_t end_rev,
                  svn_boolean_t incremental,
                  svn_cancel_func_t cancel_func,
                  void *cancel_baton,
                  apr_pool_t *pool)
{
  return svn_repos_dump_fs2(repos, stream, feedback_stream, start_rev,
                            end_rev, incremental, FALSE, cancel_func,
                            cancel_baton, pool);
}

/* Implementation of svn_repos_notify_func_t to wrap the output to a
   response stream for svn_repos_dump_fs2() and svn_repos_verify_fs() */
static void
repos_notify_handler(void *baton,
                     const svn_repos_notify_t *notify,
                     apr_pool_t *scratch_pool)
{
  svn_stream_t *feedback_stream = baton;
  apr_size_t len;

  switch (notify->action)
  {
    case svn_repos_notify_warning:
      len = strlen(notify->warning_str);
      svn_error_clear(svn_stream_write(feedback_stream, notify->warning_str, &len));
      return;

    case svn_repos_notify_dump_rev_end:
      svn_error_clear(svn_stream_printf(feedback_stream, scratch_pool,
                                        _("* Dumped revision %ld.\n"),
                                        notify->revision));
      return;

    case svn_repos_notify_verify_rev_end:
      svn_error_clear(svn_stream_printf(feedback_stream, scratch_pool,
                                        _("* Verified revision %ld.\n"),
                                        notify->revision));
      return;

    case svn_repos_notify_load_txn_committed:
      if (notify->old_revision == SVN_INVALID_REVNUM)
        {
          svn_error_clear(svn_stream_printf(feedback_stream, scratch_pool,
                            _("\n------- Committed revision %ld >>>\n\n"),
                            notify->new_revision));
        }
      else
        {
          svn_error_clear(svn_stream_printf(feedback_stream, scratch_pool,
                            _("\n------- Committed new rev %ld"
                              " (loaded from original rev %ld"
                              ") >>>\n\n"), notify->new_revision,
                              notify->old_revision));
        }
      return;

    case svn_repos_notify_load_node_start:
      {
        switch (notify->node_action)
        {
          case svn_node_action_change:
            svn_error_clear(svn_stream_printf(feedback_stream, scratch_pool,
                                  _("     * editing path : %s ..."),
                                  notify->path));
            break;

          case svn_node_action_delete:
            svn_error_clear(svn_stream_printf(feedback_stream, scratch_pool,
                                  _("     * deleting path : %s ..."),
                                  notify->path));
            break;

          case svn_node_action_add:
            svn_error_clear(svn_stream_printf(feedback_stream, scratch_pool,
                                  _("     * adding path : %s ..."),
                                  notify->path));
            break;

          case svn_node_action_replace:
            svn_error_clear(svn_stream_printf(feedback_stream, scratch_pool,
                                  _("     * replacing path : %s ..."),
                                  notify->path));
            break;

        }
      }
      return;

    case svn_repos_notify_load_node_done:
      len = 7;
      svn_error_clear(svn_stream_write(feedback_stream, _(" done.\n"), &len));
      return;

    case svn_repos_notify_load_copied_node:
      len = 9;
      svn_error_clear(svn_stream_write(feedback_stream, "COPIED...", &len));
      return;

    case svn_repos_notify_load_txn_start:
      svn_error_clear(svn_stream_printf(feedback_stream, scratch_pool,
                                _("<<< Started new transaction, based on "
                                  "original revision %ld\n"),
                                notify->old_revision));
      return;

    case svn_repos_notify_load_normalized_mergeinfo:
      svn_error_clear(svn_stream_printf(feedback_stream, scratch_pool,
                                _(" removing '\\r' from %s ..."),
                                SVN_PROP_MERGEINFO));
      return;

    default:
      return;
  }
}


svn_error_t *
svn_repos_dump_fs2(svn_repos_t *repos,
                   svn_stream_t *stream,
                   svn_stream_t *feedback_stream,
                   svn_revnum_t start_rev,
                   svn_revnum_t end_rev,
                   svn_boolean_t incremental,
                   svn_boolean_t use_deltas,
                   svn_cancel_func_t cancel_func,
                   void *cancel_baton,
                   apr_pool_t *pool)
{
  return svn_error_trace(svn_repos_dump_fs3(repos,
                                            stream,
                                            start_rev,
                                            end_rev,
                                            incremental,
                                            use_deltas,
                                            feedback_stream
                                              ? repos_notify_handler
                                              : NULL,
                                            feedback_stream,
                                            cancel_func,
                                            cancel_baton,
                                            pool));
}

svn_error_t *
svn_repos_verify_fs(svn_repos_t *repos,
                    svn_stream_t *feedback_stream,
                    svn_revnum_t start_rev,
                    svn_revnum_t end_rev,
                    svn_cancel_func_t cancel_func,
                    void *cancel_baton,
                    apr_pool_t *pool)
{
  return svn_error_trace(svn_repos_verify_fs2(repos,
                                              start_rev,
                                              end_rev,
                                              feedback_stream
                                                ? repos_notify_handler
                                                : NULL,
                                              feedback_stream,
                                              cancel_func,
                                              cancel_baton,
                                              pool));
}

/*** From load.c ***/

svn_error_t *
svn_repos_load_fs2(svn_repos_t *repos,
                   svn_stream_t *dumpstream,
                   svn_stream_t *feedback_stream,
                   enum svn_repos_load_uuid uuid_action,
                   const char *parent_dir,
                   svn_boolean_t use_pre_commit_hook,
                   svn_boolean_t use_post_commit_hook,
                   svn_cancel_func_t cancel_func,
                   void *cancel_baton,
                   apr_pool_t *pool)
{
  return svn_repos_load_fs3(repos, dumpstream, uuid_action, parent_dir,
                            use_pre_commit_hook, use_post_commit_hook, FALSE,
                            feedback_stream ? repos_notify_handler : NULL,
                            feedback_stream, cancel_func, cancel_baton, pool);
}


static svn_repos_parser_fns_t *
fns_from_fns2(const svn_repos_parse_fns2_t *fns2,
              apr_pool_t *pool)
{
  svn_repos_parser_fns_t *fns;

  fns = apr_palloc(pool, sizeof(*fns));
  fns->new_revision_record = fns2->new_revision_record;
  fns->uuid_record = fns2->uuid_record;
  fns->new_node_record = fns2->new_node_record;
  fns->set_revision_property = fns2->set_revision_property;
  fns->set_node_property = fns2->set_node_property;
  fns->remove_node_props = fns2->remove_node_props;
  fns->set_fulltext = fns2->set_fulltext;
  fns->close_node = fns2->close_node;
  fns->close_revision = fns2->close_revision;
  return fns;
}

static svn_repos_parse_fns2_t *
fns2_from_fns(const svn_repos_parser_fns_t *fns,
              apr_pool_t *pool)
{
  svn_repos_parse_fns2_t *fns2;

  fns2 = apr_palloc(pool, sizeof(*fns2));
  fns2->new_revision_record = fns->new_revision_record;
  fns2->uuid_record = fns->uuid_record;
  fns2->new_node_record = fns->new_node_record;
  fns2->set_revision_property = fns->set_revision_property;
  fns2->set_node_property = fns->set_node_property;
  fns2->remove_node_props = fns->remove_node_props;
  fns2->set_fulltext = fns->set_fulltext;
  fns2->close_node = fns->close_node;
  fns2->close_revision = fns->close_revision;
  fns2->delete_node_property = NULL;
  fns2->apply_textdelta = NULL;
  return fns2;
}

svn_error_t *
svn_repos_parse_dumpstream(svn_stream_t *stream,
                           const svn_repos_parser_fns_t *parse_fns,
                           void *parse_baton,
                           svn_cancel_func_t cancel_func,
                           void *cancel_baton,
                           apr_pool_t *pool)
{
  svn_repos_parse_fns2_t *fns2 = fns2_from_fns(parse_fns, pool);

  return svn_repos_parse_dumpstream2(stream, fns2, parse_baton,
                                     cancel_func, cancel_baton, pool);
}

svn_error_t *
svn_repos_load_fs(svn_repos_t *repos,
                  svn_stream_t *dumpstream,
                  svn_stream_t *feedback_stream,
                  enum svn_repos_load_uuid uuid_action,
                  const char *parent_dir,
                  svn_cancel_func_t cancel_func,
                  void *cancel_baton,
                  apr_pool_t *pool)
{
  return svn_repos_load_fs2(repos, dumpstream, feedback_stream,
                            uuid_action, parent_dir, FALSE, FALSE,
                            cancel_func, cancel_baton, pool);
}

svn_error_t *
svn_repos_get_fs_build_parser2(const svn_repos_parse_fns2_t **parser,
                               void **parse_baton,
                               svn_repos_t *repos,
                               svn_boolean_t use_history,
                               enum svn_repos_load_uuid uuid_action,
                               svn_stream_t *outstream,
                               const char *parent_dir,
                               apr_pool_t *pool)
{
  return svn_repos_get_fs_build_parser3(parser, parse_baton, repos, use_history,
                                        FALSE, uuid_action, parent_dir,
                                        outstream ? repos_notify_handler : NULL,
                                        outstream, pool);
}

svn_error_t *
svn_repos_get_fs_build_parser(const svn_repos_parser_fns_t **parser_callbacks,
                              void **parse_baton,
                              svn_repos_t *repos,
                              svn_boolean_t use_history,
                              enum svn_repos_load_uuid uuid_action,
                              svn_stream_t *outstream,
                              const char *parent_dir,
                              apr_pool_t *pool)
{
  const svn_repos_parse_fns2_t *fns2;

  SVN_ERR(svn_repos_get_fs_build_parser2(&fns2, parse_baton, repos,
                                         use_history, uuid_action, outstream,
                                         parent_dir, pool));

  *parser_callbacks = fns_from_fns2(fns2, pool);
  return SVN_NO_ERROR;
}

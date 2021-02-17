/* commit.c --- editor for committing changes to a filesystem.
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


#include <string.h>

#include <apr_pools.h>
#include <apr_file_io.h>

#include "svn_compat.h"
#include "svn_pools.h"
#include "svn_error.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_delta.h"
#include "svn_fs.h"
#include "svn_repos.h"
#include "svn_checksum.h"
#include "svn_props.h"
#include "svn_mergeinfo.h"
#include "repos.h"
#include "svn_private_config.h"
#include "private/svn_fspath.h"
#include "private/svn_repos_private.h"



/*** Editor batons. ***/

struct edit_baton
{
  apr_pool_t *pool;

  /** Supplied when the editor is created: **/

  /* Revision properties to set for this commit. */
  apr_hash_t *revprop_table;

  /* Callback to run when the commit is done. */
  svn_commit_callback2_t commit_callback;
  void *commit_callback_baton;

  /* Callback to check authorizations on paths. */
  svn_repos_authz_callback_t authz_callback;
  void *authz_baton;

  /* The already-open svn repository to commit to. */
  svn_repos_t *repos;

  /* URL to the root of the open repository. */
  const char *repos_url;

  /* The name of the repository (here for convenience). */
  const char *repos_name;

  /* The filesystem associated with the REPOS above (here for
     convenience). */
  svn_fs_t *fs;

  /* Location in fs where the edit will begin. */
  const char *base_path;

  /* Does this set of interfaces 'own' the commit transaction? */
  svn_boolean_t txn_owner;

  /* svn transaction associated with this edit (created in
     open_root, or supplied by the public API caller). */
  svn_fs_txn_t *txn;

  /** Filled in during open_root: **/

  /* The name of the transaction. */
  const char *txn_name;

  /* The object representing the root directory of the svn txn. */
  svn_fs_root_t *txn_root;

  /* Avoid aborting an fs transaction more than once */
  svn_boolean_t txn_aborted;

  /** Filled in when the edit is closed: **/

  /* The new revision created by this commit. */
  svn_revnum_t *new_rev;

  /* The date (according to the repository) of this commit. */
  const char **committed_date;

  /* The author (also according to the repository) of this commit. */
  const char **committed_author;
};


struct dir_baton
{
  struct edit_baton *edit_baton;
  struct dir_baton *parent;
  const char *path; /* the -absolute- path to this dir in the fs */
  svn_revnum_t base_rev;        /* the revision I'm based on  */
  svn_boolean_t was_copied; /* was this directory added with history? */
  apr_pool_t *pool; /* my personal pool, in which I am allocated. */
};


struct file_baton
{
  struct edit_baton *edit_baton;
  const char *path; /* the -absolute- path to this file in the fs */
};



/* Create and return a generic out-of-dateness error. */
static svn_error_t *
out_of_date(const char *path, svn_node_kind_t kind)
{
  return svn_error_createf(SVN_ERR_FS_TXN_OUT_OF_DATE, NULL,
                           (kind == svn_node_dir
                            ? _("Directory '%s' is out of date")
                            : kind == svn_node_file
                            ? _("File '%s' is out of date")
                            : _("'%s' is out of date")),
                           path);
}



/* If EDITOR_BATON contains a valid authz callback, verify that the
   REQUIRED access to PATH in ROOT is authorized.  Return an error
   appropriate for throwing out of the commit editor with SVN_ERR.  If
   no authz callback is present in EDITOR_BATON, then authorize all
   paths.  Use POOL for temporary allocation only. */
static svn_error_t *
check_authz(struct edit_baton *editor_baton, const char *path,
            svn_fs_root_t *root, svn_repos_authz_access_t required,
            apr_pool_t *pool)
{
  if (editor_baton->authz_callback)
    {
      svn_boolean_t allowed;

      SVN_ERR(editor_baton->authz_callback(required, &allowed, root, path,
                                           editor_baton->authz_baton, pool));
      if (!allowed)
        return svn_error_create(required & svn_authz_write ?
                                SVN_ERR_AUTHZ_UNWRITABLE :
                                SVN_ERR_AUTHZ_UNREADABLE,
                                NULL, "Access denied");
    }

  return SVN_NO_ERROR;
}


/* Return a directory baton allocated in POOL which represents
   FULL_PATH, which is the immediate directory child of the directory
   represented by PARENT_BATON.  EDIT_BATON is the commit editor
   baton.  WAS_COPIED reveals whether or not this directory is the
   result of a copy operation.  BASE_REVISION is the base revision of
   the directory. */
static struct dir_baton *
make_dir_baton(struct edit_baton *edit_baton,
               struct dir_baton *parent_baton,
               const char *full_path,
               svn_boolean_t was_copied,
               svn_revnum_t base_revision,
               apr_pool_t *pool)
{
  struct dir_baton *db;
  db = apr_pcalloc(pool, sizeof(*db));
  db->edit_baton = edit_baton;
  db->parent = parent_baton;
  db->pool = pool;
  db->path = full_path;
  db->was_copied = was_copied;
  db->base_rev = base_revision;
  return db;
}


/* This function is the shared guts of add_file() and add_directory(),
   which see for the meanings of the parameters.  The only extra
   parameter here is IS_DIR, which is TRUE when adding a directory,
   and FALSE when adding a file.  */
static svn_error_t *
add_file_or_directory(const char *path,
                      void *parent_baton,
                      const char *copy_path,
                      svn_revnum_t copy_revision,
                      svn_boolean_t is_dir,
                      apr_pool_t *pool,
                      void **return_baton)
{
  struct dir_baton *pb = parent_baton;
  struct edit_baton *eb = pb->edit_baton;
  apr_pool_t *subpool = svn_pool_create(pool);
  svn_boolean_t was_copied = FALSE;
  const char *full_path;

  full_path = svn_fspath__join(eb->base_path,
                               svn_relpath_canonicalize(path, pool), pool);

  /* Sanity check. */
  if (copy_path && (! SVN_IS_VALID_REVNUM(copy_revision)))
    return svn_error_createf
      (SVN_ERR_FS_GENERAL, NULL,
       _("Got source path but no source revision for '%s'"), full_path);

  if (copy_path)
    {
      const char *fs_path;
      svn_fs_root_t *copy_root;
      svn_node_kind_t kind;
      size_t repos_url_len;
      svn_repos_authz_access_t required;

      /* Copy requires recursive write access to the destination path
         and write access to the parent path. */
      required = svn_authz_write | (is_dir ? svn_authz_recursive : 0);
      SVN_ERR(check_authz(eb, full_path, eb->txn_root,
                          required, subpool));
      SVN_ERR(check_authz(eb, pb->path, eb->txn_root,
                          svn_authz_write, subpool));

      /* Check PATH in our transaction.  Make sure it does not exist
         unless its parent directory was copied (in which case, the
         thing might have been copied in as well), else return an
         out-of-dateness error. */
      SVN_ERR(svn_fs_check_path(&kind, eb->txn_root, full_path, subpool));
      if ((kind != svn_node_none) && (! pb->was_copied))
        return out_of_date(full_path, kind);

      /* For now, require that the url come from the same repository
         that this commit is operating on. */
      copy_path = svn_path_uri_decode(copy_path, subpool);
      repos_url_len = strlen(eb->repos_url);
      if (strncmp(copy_path, eb->repos_url, repos_url_len) != 0)
        return svn_error_createf
          (SVN_ERR_FS_GENERAL, NULL,
           _("Source url '%s' is from different repository"), copy_path);

      fs_path = apr_pstrdup(subpool, copy_path + repos_url_len);

      /* Now use the "fs_path" as an absolute path within the
         repository to make the copy from. */
      SVN_ERR(svn_fs_revision_root(&copy_root, eb->fs,
                                   copy_revision, subpool));

      /* Copy also requires (recursive) read access to the source */
      required = svn_authz_read | (is_dir ? svn_authz_recursive : 0);
      SVN_ERR(check_authz(eb, fs_path, copy_root, required, subpool));

      SVN_ERR(svn_fs_copy(copy_root, fs_path,
                          eb->txn_root, full_path, subpool));
      was_copied = TRUE;
    }
  else
    {
      /* No ancestry given, just make a new directory or empty file.
         Note that we don't perform an existence check here like the
         copy-from case does -- that's because svn_fs_make_*()
         already errors out if the file already exists.  Verify write
         access to the full path and to the parent. */
      SVN_ERR(check_authz(eb, full_path, eb->txn_root,
                          svn_authz_write, subpool));
      SVN_ERR(check_authz(eb, pb->path, eb->txn_root,
                          svn_authz_write, subpool));
      if (is_dir)
        SVN_ERR(svn_fs_make_dir(eb->txn_root, full_path, subpool));
      else
        SVN_ERR(svn_fs_make_file(eb->txn_root, full_path, subpool));
    }

  /* Cleanup our temporary subpool. */
  svn_pool_destroy(subpool);

  /* Build a new child baton. */
  if (is_dir)
    {
      *return_baton = make_dir_baton(eb, pb, full_path, was_copied,
                                     SVN_INVALID_REVNUM, pool);
    }
  else
    {
      struct file_baton *new_fb = apr_pcalloc(pool, sizeof(*new_fb));
      new_fb->edit_baton = eb;
      new_fb->path = full_path;
      *return_baton = new_fb;
    }

  return SVN_NO_ERROR;
}



/*** Editor functions ***/

static svn_error_t *
open_root(void *edit_baton,
          svn_revnum_t base_revision,
          apr_pool_t *pool,
          void **root_baton)
{
  struct dir_baton *dirb;
  struct edit_baton *eb = edit_baton;
  svn_revnum_t youngest;

  /* Ignore BASE_REVISION.  We always build our transaction against
     HEAD.  However, we will keep it in our dir baton for out of
     dateness checks.  */
  SVN_ERR(svn_fs_youngest_rev(&youngest, eb->fs, eb->pool));

  /* Unless we've been instructed to use a specific transaction, we'll
     make our own. */
  if (eb->txn_owner)
    {
      SVN_ERR(svn_repos_fs_begin_txn_for_commit2(&(eb->txn),
                                                 eb->repos,
                                                 youngest,
                                                 eb->revprop_table,
                                                 eb->pool));
    }
  else /* Even if we aren't the owner of the transaction, we might
          have been instructed to set some properties. */
    {
      apr_array_header_t *props = svn_prop_hash_to_array(eb->revprop_table,
                                                         pool);
      SVN_ERR(svn_repos_fs_change_txn_props(eb->txn, props, pool));
    }
  SVN_ERR(svn_fs_txn_name(&(eb->txn_name), eb->txn, eb->pool));
  SVN_ERR(svn_fs_txn_root(&(eb->txn_root), eb->txn, eb->pool));

  /* Create a root dir baton.  The `base_path' field is an -absolute-
     path in the filesystem, upon which all further editor paths are
     based. */
  dirb = apr_pcalloc(pool, sizeof(*dirb));
  dirb->edit_baton = edit_baton;
  dirb->parent = NULL;
  dirb->pool = pool;
  dirb->was_copied = FALSE;
  dirb->path = apr_pstrdup(pool, eb->base_path);
  dirb->base_rev = base_revision;

  *root_baton = dirb;
  return SVN_NO_ERROR;
}



static svn_error_t *
delete_entry(const char *path,
             svn_revnum_t revision,
             void *parent_baton,
             apr_pool_t *pool)
{
  struct dir_baton *parent = parent_baton;
  struct edit_baton *eb = parent->edit_baton;
  svn_node_kind_t kind;
  svn_revnum_t cr_rev;
  svn_repos_authz_access_t required = svn_authz_write;
  const char *full_path;

  full_path = svn_fspath__join(eb->base_path,
                               svn_relpath_canonicalize(path, pool), pool);

  /* Check PATH in our transaction.  */
  SVN_ERR(svn_fs_check_path(&kind, eb->txn_root, full_path, pool));

  /* Deletion requires a recursive write access, as well as write
     access to the parent directory. */
  if (kind == svn_node_dir)
    required |= svn_authz_recursive;
  SVN_ERR(check_authz(eb, full_path, eb->txn_root,
                      required, pool));
  SVN_ERR(check_authz(eb, parent->path, eb->txn_root,
                      svn_authz_write, pool));

  /* If PATH doesn't exist in the txn, the working copy is out of date. */
  if (kind == svn_node_none)
    return out_of_date(full_path, kind);

  /* Now, make sure we're deleting the node we *think* we're
     deleting, else return an out-of-dateness error. */
  SVN_ERR(svn_fs_node_created_rev(&cr_rev, eb->txn_root, full_path, pool));
  if (SVN_IS_VALID_REVNUM(revision) && (revision < cr_rev))
    return out_of_date(full_path, kind);

  /* This routine is a mindless wrapper.  We call svn_fs_delete()
     because that will delete files and recursively delete
     directories.  */
  return svn_fs_delete(eb->txn_root, full_path, pool);
}


static svn_error_t *
add_directory(const char *path,
              void *parent_baton,
              const char *copy_path,
              svn_revnum_t copy_revision,
              apr_pool_t *pool,
              void **child_baton)
{
  return add_file_or_directory(path, parent_baton, copy_path, copy_revision,
                               TRUE /* is_dir */, pool, child_baton);
}


static svn_error_t *
open_directory(const char *path,
               void *parent_baton,
               svn_revnum_t base_revision,
               apr_pool_t *pool,
               void **child_baton)
{
  struct dir_baton *pb = parent_baton;
  struct edit_baton *eb = pb->edit_baton;
  svn_node_kind_t kind;
  const char *full_path;

  full_path = svn_fspath__join(eb->base_path,
                               svn_relpath_canonicalize(path, pool), pool);

  /* Check PATH in our transaction.  If it does not exist,
     return a 'Path not present' error. */
  SVN_ERR(svn_fs_check_path(&kind, eb->txn_root, full_path, pool));
  if (kind == svn_node_none)
    return svn_error_createf(SVN_ERR_FS_NOT_DIRECTORY, NULL,
                             _("Path '%s' not present"),
                             path);

  /* Build a new dir baton for this directory. */
  *child_baton = make_dir_baton(eb, pb, full_path, pb->was_copied,
                                base_revision, pool);
  return SVN_NO_ERROR;
}


static svn_error_t *
apply_textdelta(void *file_baton,
                const char *base_checksum,
                apr_pool_t *pool,
                svn_txdelta_window_handler_t *handler,
                void **handler_baton)
{
  struct file_baton *fb = file_baton;

  /* Check for write authorization. */
  SVN_ERR(check_authz(fb->edit_baton, fb->path,
                      fb->edit_baton->txn_root,
                      svn_authz_write, pool));

  return svn_fs_apply_textdelta(handler, handler_baton,
                                fb->edit_baton->txn_root,
                                fb->path,
                                base_checksum,
                                NULL,
                                pool);
}


static svn_error_t *
add_file(const char *path,
         void *parent_baton,
         const char *copy_path,
         svn_revnum_t copy_revision,
         apr_pool_t *pool,
         void **file_baton)
{
  return add_file_or_directory(path, parent_baton, copy_path, copy_revision,
                               FALSE /* is_dir */, pool, file_baton);
}


static svn_error_t *
open_file(const char *path,
          void *parent_baton,
          svn_revnum_t base_revision,
          apr_pool_t *pool,
          void **file_baton)
{
  struct file_baton *new_fb;
  struct dir_baton *pb = parent_baton;
  struct edit_baton *eb = pb->edit_baton;
  svn_revnum_t cr_rev;
  apr_pool_t *subpool = svn_pool_create(pool);
  const char *full_path;

  full_path = svn_fspath__join(eb->base_path,
                               svn_relpath_canonicalize(path, pool), pool);

  /* Check for read authorization. */
  SVN_ERR(check_authz(eb, full_path, eb->txn_root,
                      svn_authz_read, subpool));

  /* Get this node's creation revision (doubles as an existence check). */
  SVN_ERR(svn_fs_node_created_rev(&cr_rev, eb->txn_root, full_path,
                                  subpool));

  /* If the node our caller has is an older revision number than the
     one in our transaction, return an out-of-dateness error. */
  if (SVN_IS_VALID_REVNUM(base_revision) && (base_revision < cr_rev))
    return out_of_date(full_path, svn_node_file);

  /* Build a new file baton */
  new_fb = apr_pcalloc(pool, sizeof(*new_fb));
  new_fb->edit_baton = eb;
  new_fb->path = full_path;

  *file_baton = new_fb;

  /* Destory the work subpool. */
  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


static svn_error_t *
change_file_prop(void *file_baton,
                 const char *name,
                 const svn_string_t *value,
                 apr_pool_t *pool)
{
  struct file_baton *fb = file_baton;
  struct edit_baton *eb = fb->edit_baton;

  /* Check for write authorization. */
  SVN_ERR(check_authz(eb, fb->path, eb->txn_root,
                      svn_authz_write, pool));

  return svn_repos_fs_change_node_prop(eb->txn_root, fb->path,
                                       name, value, pool);
}


static svn_error_t *
close_file(void *file_baton,
           const char *text_digest,
           apr_pool_t *pool)
{
  struct file_baton *fb = file_baton;

  if (text_digest)
    {
      svn_checksum_t *checksum;
      svn_checksum_t *text_checksum;

      SVN_ERR(svn_fs_file_checksum(&checksum, svn_checksum_md5,
                                   fb->edit_baton->txn_root, fb->path,
                                   TRUE, pool));
      SVN_ERR(svn_checksum_parse_hex(&text_checksum, svn_checksum_md5,
                                     text_digest, pool));

      if (!svn_checksum_match(text_checksum, checksum))
        return svn_checksum_mismatch_err(text_checksum, checksum, pool,
                            _("Checksum mismatch for resulting fulltext\n(%s)"),
                            fb->path);
    }

  return SVN_NO_ERROR;
}


static svn_error_t *
change_dir_prop(void *dir_baton,
                const char *name,
                const svn_string_t *value,
                apr_pool_t *pool)
{
  struct dir_baton *db = dir_baton;
  struct edit_baton *eb = db->edit_baton;

  /* Check for write authorization. */
  SVN_ERR(check_authz(eb, db->path, eb->txn_root,
                      svn_authz_write, pool));

  if (SVN_IS_VALID_REVNUM(db->base_rev))
    {
      /* Subversion rule:  propchanges can only happen on a directory
         which is up-to-date. */
      svn_revnum_t created_rev;
      SVN_ERR(svn_fs_node_created_rev(&created_rev,
                                      eb->txn_root, db->path, pool));

      if (db->base_rev < created_rev)
        return out_of_date(db->path, svn_node_dir);
    }

  return svn_repos_fs_change_node_prop(eb->txn_root, db->path,
                                       name, value, pool);
}

const char *
svn_repos__post_commit_error_str(svn_error_t *err,
                                 apr_pool_t *pool)
{
  svn_error_t *hook_err1, *hook_err2;
  const char *msg;

  if (! err)
    return _("(no error)");

  err = svn_error_purge_tracing(err);

  /* hook_err1 is the SVN_ERR_REPOS_POST_COMMIT_HOOK_FAILED wrapped
     error from the post-commit script, if any, and hook_err2 should
     be the original error, but be defensive and handle a case where
     SVN_ERR_REPOS_POST_COMMIT_HOOK_FAILED doesn't wrap an error. */
  hook_err1 = svn_error_find_cause(err, SVN_ERR_REPOS_POST_COMMIT_HOOK_FAILED);
  if (hook_err1 && hook_err1->child)
    hook_err2 = hook_err1->child;
  else
    hook_err2 = hook_err1;

  /* This implementation counts on svn_repos_fs_commit_txn() returning
     svn_fs_commit_txn() as the parent error with a child
     SVN_ERR_REPOS_POST_COMMIT_HOOK_FAILED error.  If the parent error
     is SVN_ERR_REPOS_POST_COMMIT_HOOK_FAILED then there was no error
     in svn_fs_commit_txn().

     The post-commit hook error message is already self describing, so
     it can be dropped into an error message without any additional
     text. */
  if (hook_err1)
    {
      if (err == hook_err1)
        {
          if (hook_err2->message)
            msg = apr_pstrdup(pool, hook_err2->message);
          else
            msg = _("post-commit hook failed with no error message.");
        }
      else
        {
          msg = hook_err2->message
                  ? apr_pstrdup(pool, hook_err2->message)
                  : _("post-commit hook failed with no error message.");
          msg = apr_psprintf(
                  pool,
                  _("post commit FS processing had error:\n%s\n%s"),
                  err->message ? err->message : _("(no error message)"),
                  msg);
        }
    }
  else
    {
      msg = apr_psprintf(pool,
                         _("post commit FS processing had error:\n%s"),
                         err->message ? err->message
                                      : _("(no error message)"));
    }

  return msg;
}

static svn_error_t *
close_edit(void *edit_baton,
           apr_pool_t *pool)
{
  struct edit_baton *eb = edit_baton;
  svn_revnum_t new_revision = SVN_INVALID_REVNUM;
  svn_error_t *err;
  const char *conflict;
  const char *post_commit_err = NULL;

  /* If no transaction has been created (ie. if open_root wasn't
     called before close_edit), abort the operation here with an
     error. */
  if (! eb->txn)
    return svn_error_create(SVN_ERR_REPOS_BAD_ARGS, NULL,
                            "No valid transaction supplied to close_edit");

  /* Commit. */
  err = svn_repos_fs_commit_txn(&conflict, eb->repos,
                                &new_revision, eb->txn, pool);

  if (SVN_IS_VALID_REVNUM(new_revision))
    {
      if (err)
        {
          /* If the error was in post-commit, then the commit itself
             succeeded.  In which case, save the post-commit warning
             (to be reported back to the client, who will probably
             display it as a warning) and clear the error. */
          post_commit_err = svn_repos__post_commit_error_str(err, pool);
          svn_error_clear(err);
          err = SVN_NO_ERROR;
        }
    }
  else
    {
      /* ### todo: we should check whether it really was a conflict,
         and return the conflict info if so? */

      /* If the commit failed, it's *probably* due to a conflict --
         that is, the txn being out-of-date.  The filesystem gives us
         the ability to continue diddling the transaction and try
         again; but let's face it: that's not how the cvs or svn works
         from a user interface standpoint.  Thus we don't make use of
         this fs feature (for now, at least.)

         So, in a nutshell: svn commits are an all-or-nothing deal.
         Each commit creates a new fs txn which either succeeds or is
         aborted completely.  No second chances;  the user simply
         needs to update and commit again  :) */

      eb->txn_aborted = TRUE;

      return svn_error_trace(
                svn_error_compose_create(err,
                                         svn_fs_abort_txn(eb->txn, pool)));
    }

  /* Pass new revision information to the caller's callback. */
  {
    svn_string_t *date, *author;
    svn_commit_info_t *commit_info;

    /* Even if there was a post-commit hook failure, it's more serious
       if one of the calls here fails, so we explicitly check for errors
       here, while saving the possible post-commit error for later. */

    err = svn_fs_revision_prop(&date, svn_repos_fs(eb->repos),
                                new_revision, SVN_PROP_REVISION_DATE,
                                pool);
    if (! err)
      {
        err = svn_fs_revision_prop(&author, svn_repos_fs(eb->repos),
                                   new_revision, SVN_PROP_REVISION_AUTHOR,
                                   pool);
      }

    if ((! err) && eb->commit_callback)
      {
        commit_info = svn_create_commit_info(pool);

        /* fill up the svn_commit_info structure */
        commit_info->revision = new_revision;
        commit_info->date = date ? date->data : NULL;
        commit_info->author = author ? author->data : NULL;
        commit_info->post_commit_err = post_commit_err;
        err = (*eb->commit_callback)(commit_info,
                                     eb->commit_callback_baton,
                                     pool);
      }
  }

  return svn_error_trace(err);
}


static svn_error_t *
abort_edit(void *edit_baton,
           apr_pool_t *pool)
{
  struct edit_baton *eb = edit_baton;
  if ((! eb->txn) || (! eb->txn_owner) || eb->txn_aborted)
    return SVN_NO_ERROR;

  eb->txn_aborted = TRUE;

  return svn_error_trace(svn_fs_abort_txn(eb->txn, pool));
}



/*** Public interfaces. ***/

svn_error_t *
svn_repos_get_commit_editor5(const svn_delta_editor_t **editor,
                             void **edit_baton,
                             svn_repos_t *repos,
                             svn_fs_txn_t *txn,
                             const char *repos_url,
                             const char *base_path,
                             apr_hash_t *revprop_table,
                             svn_commit_callback2_t callback,
                             void *callback_baton,
                             svn_repos_authz_callback_t authz_callback,
                             void *authz_baton,
                             apr_pool_t *pool)
{
  svn_delta_editor_t *e;
  apr_pool_t *subpool = svn_pool_create(pool);
  struct edit_baton *eb;

  /* Do a global authz access lookup.  Users with no write access
     whatsoever to the repository don't get a commit editor. */
  if (authz_callback)
    {
      svn_boolean_t allowed;

      SVN_ERR(authz_callback(svn_authz_write, &allowed, NULL, NULL,
                             authz_baton, pool));
      if (!allowed)
        return svn_error_create(SVN_ERR_AUTHZ_UNWRITABLE, NULL,
                                "Not authorized to open a commit editor.");
    }

  /* Allocate the structures. */
  e = svn_delta_default_editor(pool);
  eb = apr_pcalloc(subpool, sizeof(*eb));

  /* Set up the editor. */
  e->open_root         = open_root;
  e->delete_entry      = delete_entry;
  e->add_directory     = add_directory;
  e->open_directory    = open_directory;
  e->change_dir_prop   = change_dir_prop;
  e->add_file          = add_file;
  e->open_file         = open_file;
  e->close_file        = close_file;
  e->apply_textdelta   = apply_textdelta;
  e->change_file_prop  = change_file_prop;
  e->close_edit        = close_edit;
  e->abort_edit        = abort_edit;

  /* Set up the edit baton. */
  eb->pool = subpool;
  eb->revprop_table = svn_prop_hash_dup(revprop_table, subpool);
  eb->commit_callback = callback;
  eb->commit_callback_baton = callback_baton;
  eb->authz_callback = authz_callback;
  eb->authz_baton = authz_baton;
  eb->base_path = svn_fspath__canonicalize(base_path, subpool);
  eb->repos = repos;
  eb->repos_url = repos_url;
  eb->repos_name = svn_dirent_basename(svn_repos_path(repos, subpool),
                                       subpool);
  eb->fs = svn_repos_fs(repos);
  eb->txn = txn;
  eb->txn_owner = txn == NULL;

  *edit_baton = eb;
  *editor = e;

  return SVN_NO_ERROR;
}

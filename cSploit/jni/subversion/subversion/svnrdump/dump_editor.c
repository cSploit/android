/*
 *  dump_editor.c: The svn_delta_editor_t editor used by svnrdump to
 *  dump revisions.
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

#include "svn_hash.h"
#include "svn_pools.h"
#include "svn_repos.h"
#include "svn_path.h"
#include "svn_props.h"
#include "svn_subst.h"
#include "svn_dirent_uri.h"

#include "private/svn_fspath.h"

#include "svnrdump.h"

#define ARE_VALID_COPY_ARGS(p,r) ((p) && SVN_IS_VALID_REVNUM(r))

#if 0
#define LDR_DBG(x) SVN_DBG(x)
#else
#define LDR_DBG(x) while(0)
#endif

/* A directory baton used by all directory-related callback functions
 * in the dump editor.  */
struct dir_baton
{
  struct dump_edit_baton *eb;
  struct dir_baton *parent_dir_baton;

  /* is this directory a new addition to this revision? */
  svn_boolean_t added;

  /* has this directory been written to the output stream? */
  svn_boolean_t written_out;

  /* the absolute path to this directory */
  const char *abspath; /* an fspath */

  /* Copyfrom info for the node, if any. */
  const char *copyfrom_path; /* a relpath */
  svn_revnum_t copyfrom_rev;

  /* Hash of paths that need to be deleted, though some -might- be
     replaced.  Maps const char * paths to this dir_baton. Note that
     they're full paths, because that's what the editor driver gives
     us, although they're all really within this directory. */
  apr_hash_t *deleted_entries;
};

/* A handler baton to be used in window_handler().  */
struct handler_baton
{
  svn_txdelta_window_handler_t apply_handler;
  void *apply_baton;
};

/* The baton used by the dump editor. */
struct dump_edit_baton {
  /* The output stream we write the dumpfile to */
  svn_stream_t *stream;

  /* Pool for per-revision allocations */
  apr_pool_t *pool;

  /* Properties which were modified during change_file_prop
   * or change_dir_prop. */
  apr_hash_t *props;

  /* Properties which were deleted during change_file_prop
   * or change_dir_prop. */
  apr_hash_t *deleted_props;

  /* Temporary buffer to write property hashes to in human-readable
   * form. ### Is this really needed? */
  svn_stringbuf_t *propstring;

  /* Temporary file used for textdelta application along with its
     absolute path; these two variables should be allocated in the
     per-edit-session pool */
  const char *delta_abspath;
  apr_file_t *delta_file;

  /* The checksum of the file the delta is being applied to */
  const char *base_checksum;

  /* Flags to trigger dumping props and text */
  svn_boolean_t dump_text;
  svn_boolean_t dump_props;
  svn_boolean_t dump_newlines;
};

/* Make a directory baton to represent the directory at PATH (relative
 * to the EDIT_BATON).
 *
 * COPYFROM_PATH/COPYFROM_REV are the path/revision against which this
 * directory should be compared for changes. If the copyfrom
 * information is valid, the directory will be compared against its
 * copy source.
 *
 * PARENT_DIR_BATON is the directory baton of this directory's parent,
 * or NULL if this is the top-level directory of the edit.  ADDED
 * indicates if this directory is newly added in this revision.
 * Perform all allocations in POOL.  */
static struct dir_baton *
make_dir_baton(const char *path,
               const char *copyfrom_path,
               svn_revnum_t copyfrom_rev,
               void *edit_baton,
               void *parent_dir_baton,
               svn_boolean_t added,
               apr_pool_t *pool)
{
  struct dump_edit_baton *eb = edit_baton;
  struct dir_baton *pb = parent_dir_baton;
  struct dir_baton *new_db = apr_pcalloc(pool, sizeof(*new_db));
  const char *abspath;

  /* Construct the full path of this node. */
  /* ### FIXME: Not sure why we use an abspath here.  If I understand
     ### correctly, the only place we used this path is in dump_node(),
     ### which immediately converts it into a relpath.  -- cmpilato.  */
  if (pb)
    abspath = svn_fspath__canonicalize(path, pool);
  else
    abspath = "/";

  /* Strip leading slash from copyfrom_path so that the path is
     canonical and svn_relpath_join can be used */
  if (copyfrom_path)
    copyfrom_path = svn_relpath_canonicalize(copyfrom_path, pool);

  new_db->eb = eb;
  new_db->parent_dir_baton = pb;
  new_db->abspath = abspath;
  new_db->copyfrom_path = copyfrom_path;
  new_db->copyfrom_rev = copyfrom_rev;
  new_db->added = added;
  new_db->written_out = FALSE;
  new_db->deleted_entries = apr_hash_make(pool);

  return new_db;
}

/* Extract and dump properties stored in edit baton EB, using POOL for
 * any temporary allocations. If TRIGGER_VAR is not NULL, it is set to FALSE.
 * Unless DUMP_DATA_TOO is set, only property headers are dumped.
 */
static svn_error_t *
do_dump_props(struct dump_edit_baton *eb,
              svn_boolean_t *trigger_var,
              svn_boolean_t dump_data_too,
              apr_pool_t *pool)
{
  svn_stream_t *propstream;
  apr_hash_t *normal_props;

  if (trigger_var && !*trigger_var)
    return SVN_NO_ERROR;

  SVN_ERR(svn_rdump__normalize_props(&normal_props, eb->props, eb->pool));
  svn_stringbuf_setempty(eb->propstring);
  propstream = svn_stream_from_stringbuf(eb->propstring, eb->pool);
  SVN_ERR(svn_hash_write_incremental(normal_props, eb->deleted_props,
                                     propstream, "PROPS-END", pool));
  SVN_ERR(svn_stream_close(propstream));

  /* Prop-delta: true */
  SVN_ERR(svn_stream_printf(eb->stream, pool,
                            SVN_REPOS_DUMPFILE_PROP_DELTA
                            ": true\n"));

  /* Prop-content-length: 193 */
  SVN_ERR(svn_stream_printf(eb->stream, pool,
                            SVN_REPOS_DUMPFILE_PROP_CONTENT_LENGTH
                            ": %" APR_SIZE_T_FMT "\n", eb->propstring->len));

  if (dump_data_too)
    {
      /* Content-length: 14 */
      SVN_ERR(svn_stream_printf(eb->stream, pool,
                                SVN_REPOS_DUMPFILE_CONTENT_LENGTH
                                ": %" APR_SIZE_T_FMT "\n\n",
                                eb->propstring->len));

      /* The properties. */
      SVN_ERR(svn_stream_write(eb->stream, eb->propstring->data,
                               &(eb->propstring->len)));

      /* No text is going to be dumped. Write a couple of newlines and
         wait for the next node/ revision. */
      SVN_ERR(svn_stream_printf(eb->stream, pool, "\n\n"));

      /* Cleanup so that data is never dumped twice. */
      SVN_ERR(svn_hash__clear(eb->props, eb->pool));
      SVN_ERR(svn_hash__clear(eb->deleted_props, eb->pool));
      if (trigger_var)
        *trigger_var = FALSE;
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
do_dump_newlines(struct dump_edit_baton *eb,
                 svn_boolean_t *trigger_var,
                 apr_pool_t *pool)
{
  if (trigger_var && *trigger_var)
    {
      SVN_ERR(svn_stream_printf(eb->stream, pool, "\n\n"));
      *trigger_var = FALSE;
    }
  return SVN_NO_ERROR;
}

/*
 * Write out a node record for PATH of type KIND under EB->FS_ROOT.
 * ACTION describes what is happening to the node (see enum
 * svn_node_action). Write record to writable EB->STREAM, using
 * EB->BUFFER to write in chunks.
 *
 * If the node was itself copied, IS_COPY is TRUE and the
 * path/revision of the copy source are in COPYFROM_PATH/COPYFROM_REV.
 * If IS_COPY is FALSE, yet COPYFROM_PATH/COPYFROM_REV are valid, this
 * node is part of a copied subtree.
 */
static svn_error_t *
dump_node(struct dump_edit_baton *eb,
          const char *path,    /* an absolute path. */
          svn_node_kind_t kind,
          enum svn_node_action action,
          svn_boolean_t is_copy,
          const char *copyfrom_path,
          svn_revnum_t copyfrom_rev,
          apr_pool_t *pool)
{
  /* Remove leading slashes from path and copyfrom_path */
  if (path)
    path = svn_relpath_canonicalize(path, pool);

  if (copyfrom_path)
    copyfrom_path = svn_relpath_canonicalize(copyfrom_path, pool);

  /* Node-path: commons/STATUS */
  SVN_ERR(svn_stream_printf(eb->stream, pool,
                            SVN_REPOS_DUMPFILE_NODE_PATH ": %s\n", path));

  /* Node-kind: file */
  if (kind == svn_node_file)
    SVN_ERR(svn_stream_printf(eb->stream, pool,
                              SVN_REPOS_DUMPFILE_NODE_KIND ": file\n"));
  else if (kind == svn_node_dir)
    SVN_ERR(svn_stream_printf(eb->stream, pool,
                              SVN_REPOS_DUMPFILE_NODE_KIND ": dir\n"));


  /* Write the appropriate Node-action header */
  switch (action)
    {
    case svn_node_action_change:
      /* We are here after a change_file_prop or change_dir_prop. They
         set up whatever dump_props they needed to- nothing to
         do here but print node action information */
      SVN_ERR(svn_stream_printf(eb->stream, pool,
                                SVN_REPOS_DUMPFILE_NODE_ACTION
                                ": change\n"));
      break;

    case svn_node_action_replace:
      if (!is_copy)
        {
          /* Node-action: replace */
          SVN_ERR(svn_stream_printf(eb->stream, pool,
                                    SVN_REPOS_DUMPFILE_NODE_ACTION
                                    ": replace\n"));

          /* Wait for a change_*_prop to be called before dumping
             anything */
          eb->dump_props = TRUE;
          break;
        }
      /* More complex case: is_copy is true, and copyfrom_path/
         copyfrom_rev are present: delete the original, and then re-add
         it */

      SVN_ERR(svn_stream_printf(eb->stream, pool,
                                SVN_REPOS_DUMPFILE_NODE_ACTION
                                ": delete\n\n"));

      /* Recurse: Print an additional add-with-history record. */
      SVN_ERR(dump_node(eb, path, kind, svn_node_action_add,
                        is_copy, copyfrom_path, copyfrom_rev, pool));

      /* We can leave this routine quietly now, don't need to dump any
         content; that was already done in the second record. */
      break;

    case svn_node_action_delete:
      SVN_ERR(svn_stream_printf(eb->stream, pool,
                                SVN_REPOS_DUMPFILE_NODE_ACTION
                                ": delete\n"));

      /* We can leave this routine quietly now. Nothing more to do-
         print a couple of newlines because we're not dumping props or
         text. */
      SVN_ERR(svn_stream_printf(eb->stream, pool, "\n\n"));
      break;

    case svn_node_action_add:
      SVN_ERR(svn_stream_printf(eb->stream, pool,
                                SVN_REPOS_DUMPFILE_NODE_ACTION ": add\n"));

      if (!is_copy)
        {
          /* eb->dump_props for files is handled in close_file
             which is called immediately.  However, directories are not
             closed until all the work inside them has been done;
             eb->dump_props for directories is handled in all the
             functions that can possibly be called after add_directory:
             add_directory, open_directory, delete_entry, close_directory,
             add_file, open_file. change_dir_prop is a special case. */

          /* Wait for a change_*_prop to be called before dumping
             anything */
          eb->dump_props = TRUE;
          break;
        }

      SVN_ERR(svn_stream_printf(eb->stream, pool,
                                SVN_REPOS_DUMPFILE_NODE_COPYFROM_REV
                                ": %ld\n"
                                SVN_REPOS_DUMPFILE_NODE_COPYFROM_PATH
                                ": %s\n",
                                copyfrom_rev, copyfrom_path));

      /* Ugly hack: If a directory was copied from a previous
         revision, nothing like close_file() will be called to write two
         blank lines. If change_dir_prop() is called, props are dumped
         (along with the necessary PROPS-END\n\n and we're good. So
         set DUMP_NEWLINES here to print the newlines unless
         change_dir_prop() is called next otherwise the `svnadmin load`
         parser will fail.  */
      if (kind == svn_node_dir)
        eb->dump_newlines = TRUE;

      break;
    }
  return SVN_NO_ERROR;
}

static svn_error_t *
open_root(void *edit_baton,
          svn_revnum_t base_revision,
          apr_pool_t *pool,
          void **root_baton)
{
  struct dump_edit_baton *eb = edit_baton;

  /* Clear the per-revision pool after each revision */
  svn_pool_clear(eb->pool);

  eb->props = apr_hash_make(eb->pool);
  eb->deleted_props = apr_hash_make(eb->pool);
  eb->propstring = svn_stringbuf_create("", eb->pool);

  *root_baton = make_dir_baton(NULL, NULL, SVN_INVALID_REVNUM,
                               edit_baton, NULL, FALSE, eb->pool);
  LDR_DBG(("open_root %p\n", *root_baton));

  return SVN_NO_ERROR;
}

static svn_error_t *
delete_entry(const char *path,
             svn_revnum_t revision,
             void *parent_baton,
             apr_pool_t *pool)
{
  struct dir_baton *pb = parent_baton;

  LDR_DBG(("delete_entry %s\n", path));

  /* Some pending properties to dump? */
  SVN_ERR(do_dump_props(pb->eb, &(pb->eb->dump_props), TRUE, pool));

  /* Some pending newlines to dump? */
  SVN_ERR(do_dump_newlines(pb->eb, &(pb->eb->dump_newlines), pool));

  /* Add this path to the deleted_entries of the parent directory
     baton. */
  apr_hash_set(pb->deleted_entries, apr_pstrdup(pb->eb->pool, path),
               APR_HASH_KEY_STRING, pb);

  return SVN_NO_ERROR;
}

static svn_error_t *
add_directory(const char *path,
              void *parent_baton,
              const char *copyfrom_path,
              svn_revnum_t copyfrom_rev,
              apr_pool_t *pool,
              void **child_baton)
{
  struct dir_baton *pb = parent_baton;
  void *val;
  struct dir_baton *new_db;
  svn_boolean_t is_copy;

  LDR_DBG(("add_directory %s\n", path));

  new_db = make_dir_baton(path, copyfrom_path, copyfrom_rev, pb->eb,
                          pb, TRUE, pb->eb->pool);

  /* Some pending properties to dump? */
  SVN_ERR(do_dump_props(pb->eb, &(pb->eb->dump_props), TRUE, pool));

  /* Some pending newlines to dump? */
  SVN_ERR(do_dump_newlines(pb->eb, &(pb->eb->dump_newlines), pool));

  /* This might be a replacement -- is the path already deleted? */
  val = apr_hash_get(pb->deleted_entries, path, APR_HASH_KEY_STRING);

  /* Detect an add-with-history */
  is_copy = ARE_VALID_COPY_ARGS(copyfrom_path, copyfrom_rev);

  /* Dump the node */
  SVN_ERR(dump_node(pb->eb, path,
                    svn_node_dir,
                    val ? svn_node_action_replace : svn_node_action_add,
                    is_copy,
                    is_copy ? copyfrom_path : NULL,
                    is_copy ? copyfrom_rev : SVN_INVALID_REVNUM,
                    pool));

  if (val)
    /* Delete the path, it's now been dumped */
    apr_hash_set(pb->deleted_entries, path, APR_HASH_KEY_STRING, NULL);

  new_db->written_out = TRUE;

  *child_baton = new_db;
  return SVN_NO_ERROR;
}

static svn_error_t *
open_directory(const char *path,
               void *parent_baton,
               svn_revnum_t base_revision,
               apr_pool_t *pool,
               void **child_baton)
{
  struct dir_baton *pb = parent_baton;
  struct dir_baton *new_db;
  const char *copyfrom_path = NULL;
  svn_revnum_t copyfrom_rev = SVN_INVALID_REVNUM;

  LDR_DBG(("open_directory %s\n", path));

  /* Some pending properties to dump? */
  SVN_ERR(do_dump_props(pb->eb, &(pb->eb->dump_props), TRUE, pool));

  /* Some pending newlines to dump? */
  SVN_ERR(do_dump_newlines(pb->eb, &(pb->eb->dump_newlines), pool));

  /* If the parent directory has explicit comparison path and rev,
     record the same for this one. */
  if (ARE_VALID_COPY_ARGS(pb->copyfrom_path, pb->copyfrom_rev))
    {
      copyfrom_path = svn_relpath_join(pb->copyfrom_path,
                                       svn_relpath_basename(path, NULL),
                                       pb->eb->pool);
      copyfrom_rev = pb->copyfrom_rev;
    }

  new_db = make_dir_baton(path, copyfrom_path, copyfrom_rev, pb->eb, pb,
                          FALSE, pb->eb->pool);
  *child_baton = new_db;
  return SVN_NO_ERROR;
}

static svn_error_t *
close_directory(void *dir_baton,
                apr_pool_t *pool)
{
  struct dir_baton *db = dir_baton;
  struct dump_edit_baton *eb = db->eb;
  apr_hash_index_t *hi;

  LDR_DBG(("close_directory %p\n", dir_baton));

  /* Some pending properties to dump? */
  SVN_ERR(do_dump_props(eb, &(eb->dump_props), TRUE, pool));

  /* Some pending newlines to dump? */
  SVN_ERR(do_dump_newlines(eb, &(eb->dump_newlines), pool));

  /* Dump the deleted directory entries */
  for (hi = apr_hash_first(pool, db->deleted_entries); hi;
       hi = apr_hash_next(hi))
    {
      const char *path = svn__apr_hash_index_key(hi);

      SVN_ERR(dump_node(db->eb, path, svn_node_unknown, svn_node_action_delete,
                        FALSE, NULL, SVN_INVALID_REVNUM, pool));
    }

  SVN_ERR(svn_hash__clear(db->deleted_entries, pool));
  return SVN_NO_ERROR;
}

static svn_error_t *
add_file(const char *path,
         void *parent_baton,
         const char *copyfrom_path,
         svn_revnum_t copyfrom_rev,
         apr_pool_t *pool,
         void **file_baton)
{
  struct dir_baton *pb = parent_baton;
  void *val;
  svn_boolean_t is_copy;

  LDR_DBG(("add_file %s\n", path));

  /* Some pending properties to dump? */
  SVN_ERR(do_dump_props(pb->eb, &(pb->eb->dump_props), TRUE, pool));

  /* Some pending newlines to dump? */
  SVN_ERR(do_dump_newlines(pb->eb, &(pb->eb->dump_newlines), pool));

  /* This might be a replacement -- is the path already deleted? */
  val = apr_hash_get(pb->deleted_entries, path, APR_HASH_KEY_STRING);

  /* Detect add-with-history. */
  is_copy = ARE_VALID_COPY_ARGS(copyfrom_path, copyfrom_rev);

  /* Dump the node. */
  SVN_ERR(dump_node(pb->eb, path,
                    svn_node_file,
                    val ? svn_node_action_replace : svn_node_action_add,
                    is_copy,
                    is_copy ? copyfrom_path : NULL,
                    is_copy ? copyfrom_rev : SVN_INVALID_REVNUM,
                    pool));

  if (val)
    /* delete the path, it's now been dumped. */
    apr_hash_set(pb->deleted_entries, path, APR_HASH_KEY_STRING, NULL);

  /* Build a nice file baton to pass to change_file_prop and
     apply_textdelta */
  *file_baton = pb->eb;

  return SVN_NO_ERROR;
}

static svn_error_t *
open_file(const char *path,
          void *parent_baton,
          svn_revnum_t ancestor_revision,
          apr_pool_t *pool,
          void **file_baton)
{
  struct dir_baton *pb = parent_baton;
  const char *copyfrom_path = NULL;
  svn_revnum_t copyfrom_rev = SVN_INVALID_REVNUM;

  LDR_DBG(("open_file %s\n", path));

  /* Some pending properties to dump? */
  SVN_ERR(do_dump_props(pb->eb, &(pb->eb->dump_props), TRUE, pool));

  /* Some pending newlines to dump? */
  SVN_ERR(do_dump_newlines(pb->eb, &(pb->eb->dump_newlines), pool));

  /* If the parent directory has explicit copyfrom path and rev,
     record the same for this one. */
  if (ARE_VALID_COPY_ARGS(pb->copyfrom_path, pb->copyfrom_rev))
    {
      copyfrom_path = svn_relpath_join(pb->copyfrom_path,
                                       svn_relpath_basename(path, NULL),
                                       pb->eb->pool);
      copyfrom_rev = pb->copyfrom_rev;
    }

  SVN_ERR(dump_node(pb->eb, path, svn_node_file, svn_node_action_change,
                    FALSE, copyfrom_path, copyfrom_rev, pool));

  /* Build a nice file baton to pass to change_file_prop and
     apply_textdelta */
  *file_baton = pb->eb;

  return SVN_NO_ERROR;
}

static svn_error_t *
change_dir_prop(void *parent_baton,
                const char *name,
                const svn_string_t *value,
                apr_pool_t *pool)
{
  struct dir_baton *db = parent_baton;

  LDR_DBG(("change_dir_prop %p\n", parent_baton));

  if (svn_property_kind(NULL, name) != svn_prop_regular_kind)
    return SVN_NO_ERROR;

  if (value)
    apr_hash_set(db->eb->props, apr_pstrdup(db->eb->pool, name),
                 APR_HASH_KEY_STRING, svn_string_dup(value, db->eb->pool));
  else
    apr_hash_set(db->eb->deleted_props, apr_pstrdup(db->eb->pool, name),
                 APR_HASH_KEY_STRING, "");

  if (! db->written_out)
    {
      /* If db->written_out is set, it means that the node information
         corresponding to this directory has already been written: don't
         do anything; do_dump_props() will take care of dumping the
         props. If it not, dump the node itself before dumping the
         props. */

      SVN_ERR(dump_node(db->eb, db->abspath, svn_node_dir,
                        svn_node_action_change, FALSE, db->copyfrom_path,
                        db->copyfrom_rev, pool));
      db->written_out = TRUE;
    }

  /* Make sure we eventually output the props, and disable printing
     a couple of extra newlines */
  db->eb->dump_newlines = FALSE;
  db->eb->dump_props = TRUE;

  return SVN_NO_ERROR;
}

static svn_error_t *
change_file_prop(void *file_baton,
                 const char *name,
                 const svn_string_t *value,
                 apr_pool_t *pool)
{
  struct dump_edit_baton *eb = file_baton;

  LDR_DBG(("change_file_prop %p\n", file_baton));

  if (svn_property_kind(NULL, name) != svn_prop_regular_kind)
    return SVN_NO_ERROR;

  if (value)
    apr_hash_set(eb->props, apr_pstrdup(eb->pool, name),
                 APR_HASH_KEY_STRING, svn_string_dup(value, eb->pool));
  else
    apr_hash_set(eb->deleted_props, apr_pstrdup(eb->pool, name),
                 APR_HASH_KEY_STRING, "");

  /* Dump the property headers and wait; close_file might need
     to write text headers too depending on whether
     apply_textdelta is called */
  eb->dump_props = TRUE;

  return SVN_NO_ERROR;
}

static svn_error_t *
window_handler(svn_txdelta_window_t *window, void *baton)
{
  struct handler_baton *hb = baton;
  static svn_error_t *err;

  err = hb->apply_handler(window, hb->apply_baton);
  if (window != NULL && !err)
    return SVN_NO_ERROR;

  if (err)
    SVN_ERR(err);

  return SVN_NO_ERROR;
}

static svn_error_t *
apply_textdelta(void *file_baton, const char *base_checksum,
                apr_pool_t *pool,
                svn_txdelta_window_handler_t *handler,
                void **handler_baton)
{
  struct dump_edit_baton *eb = file_baton;

  /* Custom handler_baton allocated in a separate pool */
  struct handler_baton *hb;
  svn_stream_t *delta_filestream;

  hb = apr_pcalloc(eb->pool, sizeof(*hb));

  LDR_DBG(("apply_textdelta %p\n", file_baton));

  /* Use a temporary file to measure the text-content-length */
  delta_filestream = svn_stream_from_aprfile2(eb->delta_file, TRUE, pool);

  /* Prepare to write the delta to the delta_filestream */
  svn_txdelta_to_svndiff3(&(hb->apply_handler), &(hb->apply_baton),
                          delta_filestream, 0,
                          SVN_DELTA_COMPRESSION_LEVEL_DEFAULT, pool);

  eb->dump_text = TRUE;
  eb->base_checksum = apr_pstrdup(eb->pool, base_checksum);
  SVN_ERR(svn_stream_close(delta_filestream));

  /* The actual writing takes place when this function has
     finished. Set handler and handler_baton now so for
     window_handler() */
  *handler = window_handler;
  *handler_baton = hb;

  return SVN_NO_ERROR;
}

static svn_error_t *
close_file(void *file_baton,
           const char *text_checksum,
           apr_pool_t *pool)
{
  struct dump_edit_baton *eb = file_baton;
  apr_finfo_t *info = apr_pcalloc(pool, sizeof(apr_finfo_t));

  LDR_DBG(("close_file %p\n", file_baton));

  /* Some pending properties to dump? Dump just the headers- dump the
     props only after dumping the text headers too (if present) */
  SVN_ERR(do_dump_props(eb, &(eb->dump_props), FALSE, pool));

  /* Dump the text headers */
  if (eb->dump_text)
    {
      apr_status_t err;

      /* Text-delta: true */
      SVN_ERR(svn_stream_printf(eb->stream, pool,
                                SVN_REPOS_DUMPFILE_TEXT_DELTA
                                ": true\n"));

      err = apr_file_info_get(info, APR_FINFO_SIZE, eb->delta_file);
      if (err)
        SVN_ERR(svn_error_wrap_apr(err, NULL));

      if (eb->base_checksum)
        /* Text-delta-base-md5: */
        SVN_ERR(svn_stream_printf(eb->stream, pool,
                                  SVN_REPOS_DUMPFILE_TEXT_DELTA_BASE_MD5
                                  ": %s\n",
                                  eb->base_checksum));

      /* Text-content-length: 39 */
      SVN_ERR(svn_stream_printf(eb->stream, pool,
                                SVN_REPOS_DUMPFILE_TEXT_CONTENT_LENGTH
                                ": %lu\n",
                                (unsigned long)info->size));

      /* Text-content-md5: 82705804337e04dcd0e586bfa2389a7f */
      SVN_ERR(svn_stream_printf(eb->stream, pool,
                                SVN_REPOS_DUMPFILE_TEXT_CONTENT_MD5
                                ": %s\n",
                                text_checksum));
    }

  /* Content-length: 1549 */
  /* If both text and props are absent, skip this header */
  if (eb->dump_props)
    SVN_ERR(svn_stream_printf(eb->stream, pool,
                              SVN_REPOS_DUMPFILE_CONTENT_LENGTH
                              ": %ld\n\n",
                              (unsigned long)info->size + eb->propstring->len));
  else if (eb->dump_text)
    SVN_ERR(svn_stream_printf(eb->stream, pool,
                              SVN_REPOS_DUMPFILE_CONTENT_LENGTH
                              ": %ld\n\n",
                              (unsigned long)info->size));

  /* Dump the props now */
  if (eb->dump_props)
    {
      SVN_ERR(svn_stream_write(eb->stream, eb->propstring->data,
                               &(eb->propstring->len)));

      /* Cleanup */
      eb->dump_props = FALSE;
      SVN_ERR(svn_hash__clear(eb->props, eb->pool));
      SVN_ERR(svn_hash__clear(eb->deleted_props, eb->pool));
    }

  /* Dump the text */
  if (eb->dump_text)
    {
      /* Seek to the beginning of the delta file, map it to a stream,
         and copy the stream to eb->stream. Then close the stream and
         truncate the file so we can reuse it for the next textdelta
         application. Note that the file isn't created, opened or
         closed here */
      svn_stream_t *delta_filestream;
      apr_off_t offset = 0;

      SVN_ERR(svn_io_file_seek(eb->delta_file, APR_SET, &offset, pool));
      delta_filestream = svn_stream_from_aprfile2(eb->delta_file, TRUE, pool);
      SVN_ERR(svn_stream_copy3(delta_filestream, eb->stream, NULL, NULL, pool));

      /* Cleanup */
      SVN_ERR(svn_stream_close(delta_filestream));
      SVN_ERR(svn_io_file_trunc(eb->delta_file, 0, pool));
      eb->dump_text = FALSE;
    }

  /* Write a couple of blank lines for matching output with `svnadmin
     dump` */
  SVN_ERR(svn_stream_printf(eb->stream, pool, "\n\n"));

  return SVN_NO_ERROR;
}

static svn_error_t *
close_edit(void *edit_baton, apr_pool_t *pool)
{
  return SVN_NO_ERROR;
}

svn_error_t *
svn_rdump__get_dump_editor(const svn_delta_editor_t **editor,
                           void **edit_baton,
                           svn_stream_t *stream,
                           svn_cancel_func_t cancel_func,
                           void *cancel_baton,
                           apr_pool_t *pool)
{
  struct dump_edit_baton *eb;
  svn_delta_editor_t *de;

  eb = apr_pcalloc(pool, sizeof(struct dump_edit_baton));
  eb->stream = stream;

  /* Create a special per-revision pool */
  eb->pool = svn_pool_create(pool);

  /* Open a unique temporary file for all textdelta applications in
     this edit session. The file is automatically closed and cleaned
     up when the edit session is done. */
  SVN_ERR(svn_io_open_unique_file3(&(eb->delta_file), &(eb->delta_abspath),
                                   NULL, svn_io_file_del_on_close, pool, pool));

  de = svn_delta_default_editor(pool);
  de->open_root = open_root;
  de->delete_entry = delete_entry;
  de->add_directory = add_directory;
  de->open_directory = open_directory;
  de->close_directory = close_directory;
  de->change_dir_prop = change_dir_prop;
  de->change_file_prop = change_file_prop;
  de->apply_textdelta = apply_textdelta;
  de->add_file = add_file;
  de->open_file = open_file;
  de->close_file = close_file;
  de->close_edit = close_edit;

  /* Set the edit_baton and editor. */
  *edit_baton = eb;
  *editor = de;

  /* Wrap this editor in a cancellation editor. */
  return svn_delta_get_cancellation_editor(cancel_func, cancel_baton,
                                           de, eb, editor, edit_baton, pool);
}

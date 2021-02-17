/* load-fs-vtable.c --- dumpstream loader vtable for committing into a
 *                      Subversion filesystem.
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


#include "svn_private_config.h"
#include "svn_pools.h"
#include "svn_error.h"
#include "svn_fs.h"
#include "svn_repos.h"
#include "svn_string.h"
#include "svn_props.h"
#include "repos.h"
#include "svn_private_config.h"
#include "svn_mergeinfo.h"
#include "svn_checksum.h"
#include "svn_subst.h"
#include "svn_ctype.h"
#include "svn_dirent_uri.h"

#include <apr_lib.h>

#include "private/svn_fspath.h"
#include "private/svn_dep_compat.h"
#include "private/svn_mergeinfo_private.h"

/*----------------------------------------------------------------------*/

/** Batons used herein **/

struct parse_baton
{
  svn_repos_t *repos;
  svn_fs_t *fs;

  svn_boolean_t use_history;
  svn_boolean_t validate_props;
  svn_boolean_t use_pre_commit_hook;
  svn_boolean_t use_post_commit_hook;
  enum svn_repos_load_uuid uuid_action;
  const char *parent_dir; /* repository relpath, or NULL */
  svn_repos_notify_func_t notify_func;
  void *notify_baton;
  svn_repos_notify_t *notify;
  apr_pool_t *pool;

  /* A hash mapping copy-from revisions and mergeinfo range revisions
     (svn_revnum_t *) in the dump stream to their corresponding revisions
     (svn_revnum_t *) in the loaded repository.  The hash and its
     contents are allocated in POOL. */
  /* ### See http://subversion.tigris.org/issues/show_bug.cgi?id=3903
     ### for discussion about improving the memory costs of this mapping. */
  apr_hash_t *rev_map;

  /* The most recent (youngest) revision from the dump stream mapped in
     REV_MAP.  If no revisions have been mapped yet, this is set to
     SVN_INVALID_REVNUM. */
  svn_revnum_t last_rev_mapped;

  /* The oldest old revision loaded from the dump stream.  If no revisions
     have been loaded yet, this is set to SVN_INVALID_REVNUM. */
  svn_revnum_t oldest_old_rev;
};

struct revision_baton
{
  svn_revnum_t rev;

  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;

  const svn_string_t *datestamp;

  apr_int32_t rev_offset;

  struct parse_baton *pb;
  apr_pool_t *pool;
};

struct node_baton
{
  const char *path;
  svn_node_kind_t kind;
  enum svn_node_action action;
  svn_checksum_t *base_checksum;        /* null, if not available */
  svn_checksum_t *result_checksum;      /* null, if not available */
  svn_checksum_t *copy_source_checksum; /* null, if not available */

  svn_revnum_t copyfrom_rev;
  const char *copyfrom_path;

  struct revision_baton *rb;
  apr_pool_t *pool;
};


/*----------------------------------------------------------------------*/

/* Record the mapping of FROM_REV to TO_REV in REV_MAP, ensuring that
   anything added to the hash is allocated in the hash's pool. */
static void
set_revision_mapping(apr_hash_t *rev_map,
                     svn_revnum_t from_rev,
                     svn_revnum_t to_rev)
{
  svn_revnum_t *mapped_revs = apr_palloc(apr_hash_pool_get(rev_map),
                                         sizeof(svn_revnum_t) * 2);
  mapped_revs[0] = from_rev;
  mapped_revs[1] = to_rev;
  apr_hash_set(rev_map, mapped_revs,
               sizeof(svn_revnum_t), mapped_revs + 1);
}

/* Return the revision to which FROM_REV maps in REV_MAP, or
   SVN_INVALID_REVNUM if no such mapping exists. */
static svn_revnum_t
get_revision_mapping(apr_hash_t *rev_map,
                     svn_revnum_t from_rev)
{
  svn_revnum_t *to_rev = apr_hash_get(rev_map, &from_rev,
                                      sizeof(from_rev));
  return to_rev ? *to_rev : SVN_INVALID_REVNUM;
}


/* Change revision property NAME to VALUE for REVISION in REPOS.  If
   VALIDATE_PROPS is set, use functions which perform validation of
   the property value.  Otherwise, bypass those checks. */
static svn_error_t *
change_rev_prop(svn_repos_t *repos,
                svn_revnum_t revision,
                const char *name,
                const svn_string_t *value,
                svn_boolean_t validate_props,
                apr_pool_t *pool)
{
  if (validate_props)
    return svn_repos_fs_change_rev_prop4(repos, revision, NULL, name,
                                         NULL, value, FALSE, FALSE,
                                         NULL, NULL, pool);
  else
    return svn_fs_change_rev_prop2(svn_repos_fs(repos), revision, name,
                                   NULL, value, pool);
}

/* Change property NAME to VALUE for PATH in TXN_ROOT.  If
   VALIDATE_PROPS is set, use functions which perform validation of
   the property value.  Otherwise, bypass those checks. */
static svn_error_t *
change_node_prop(svn_fs_root_t *txn_root,
                 const char *path,
                 const char *name,
                 const svn_string_t *value,
                 svn_boolean_t validate_props,
                 apr_pool_t *pool)
{
  if (validate_props)
    return svn_repos_fs_change_node_prop(txn_root, path, name, value, pool);
  else
    return svn_fs_change_node_prop(txn_root, path, name, value, pool);
}

/* Prepend the mergeinfo source paths in MERGEINFO_ORIG with PARENT_DIR, and
   return it in *MERGEINFO_VAL. */
/* ### FIXME:  Consider somehow sharing code with
   ### svnrdump/load_editor.c:prefix_mergeinfo_paths() */
static svn_error_t *
prefix_mergeinfo_paths(svn_string_t **mergeinfo_val,
                       const svn_string_t *mergeinfo_orig,
                       const char *parent_dir,
                       apr_pool_t *pool)
{
  apr_hash_t *prefixed_mergeinfo, *mergeinfo;
  apr_hash_index_t *hi;
  void *rangelist;

  SVN_ERR(svn_mergeinfo_parse(&mergeinfo, mergeinfo_orig->data, pool));
  prefixed_mergeinfo = apr_hash_make(pool);
  for (hi = apr_hash_first(pool, mergeinfo); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      const char *path, *merge_source;

      apr_hash_this(hi, &key, NULL, &rangelist);
      merge_source = svn_relpath_canonicalize(key, pool);

      /* The svn:mergeinfo property syntax demands a repos abspath */
      path = svn_fspath__canonicalize(svn_relpath_join(parent_dir,
                                                       merge_source, pool),
                                      pool);
      apr_hash_set(prefixed_mergeinfo, path, APR_HASH_KEY_STRING, rangelist);
    }
  return svn_mergeinfo_to_string(mergeinfo_val, prefixed_mergeinfo, pool);
}


/* Examine the mergeinfo in INITIAL_VAL, renumber revisions in rangelists
   as appropriate, and return the (possibly new) mergeinfo in *FINAL_VAL
   (allocated from POOL). */
/* ### FIXME:  Consider somehow sharing code with
   ### svnrdump/load_editor.c:renumber_mergeinfo_revs() */
static svn_error_t *
renumber_mergeinfo_revs(svn_string_t **final_val,
                        const svn_string_t *initial_val,
                        struct revision_baton *rb,
                        apr_pool_t *pool)
{
  apr_pool_t *subpool = svn_pool_create(pool);
  svn_mergeinfo_t mergeinfo, predates_stream_mergeinfo;
  svn_mergeinfo_t final_mergeinfo = apr_hash_make(subpool);
  apr_hash_index_t *hi;

  SVN_ERR(svn_mergeinfo_parse(&mergeinfo, initial_val->data, subpool));

  /* Issue #3020
     http://subversion.tigris.org/issues/show_bug.cgi?id=3020#desc16
     Remove mergeinfo older than the oldest revision in the dump stream
     and adjust its revisions by the difference between the head rev of
     the target repository and the current dump stream rev. */
  if (rb->pb->oldest_old_rev > 1)
    {
      SVN_ERR(svn_mergeinfo__filter_mergeinfo_by_ranges(
        &predates_stream_mergeinfo, mergeinfo,
        rb->pb->oldest_old_rev - 1, 0,
        TRUE, subpool, subpool));
      SVN_ERR(svn_mergeinfo__filter_mergeinfo_by_ranges(
        &mergeinfo, mergeinfo,
        rb->pb->oldest_old_rev - 1, 0,
        FALSE, subpool, subpool));
      SVN_ERR(svn_mergeinfo__adjust_mergeinfo_rangelists(
        &predates_stream_mergeinfo, predates_stream_mergeinfo,
        -rb->rev_offset, subpool, subpool));
    }
  else
    {
      predates_stream_mergeinfo = NULL;
    }

  for (hi = apr_hash_first(subpool, mergeinfo); hi; hi = apr_hash_next(hi))
    {
      const char *merge_source;
      apr_array_header_t *rangelist;
      struct parse_baton *pb = rb->pb;
      int i;
      const void *key;
      void *val;

      apr_hash_this(hi, &key, NULL, &val);
      merge_source = key;
      rangelist = val;

      /* Possibly renumber revisions in merge source's rangelist. */
      for (i = 0; i < rangelist->nelts; i++)
        {
          svn_revnum_t rev_from_map;
          svn_merge_range_t *range = APR_ARRAY_IDX(rangelist, i,
                                                   svn_merge_range_t *);
          rev_from_map = get_revision_mapping(pb->rev_map, range->start);
          if (SVN_IS_VALID_REVNUM(rev_from_map))
            {
              range->start = rev_from_map;
            }
          else if (range->start == pb->oldest_old_rev - 1)
            {
              /* Since the start revision of svn_merge_range_t are not
                 inclusive there is one possible valid start revision that
                 won't be found in the PB->REV_MAP mapping of load stream
                 revsions to loaded revisions: The revision immediately
                 preceeding the oldest revision from the load stream.
                 This is a valid revision for mergeinfo, but not a valid
                 copy from revision (which PB->REV_MAP also maps for) so it
                 will never be in the mapping.

                 If that is what we have here, then find the mapping for the
                 oldest rev from the load stream and subtract 1 to get the
                 renumbered, non-inclusive, start revision. */
              rev_from_map = get_revision_mapping(pb->rev_map,
                                                  pb->oldest_old_rev);
              if (SVN_IS_VALID_REVNUM(rev_from_map))
                range->start = rev_from_map - 1;
            }
          else
            {
              /* If we can't remap the start revision then don't even bother
                 trying to remap the end revision.  It's possible we might
                 actually succeed at the latter, which can result in invalid
                 mergeinfo with a start rev > end rev.  If that gets into the
                 repository then a world of bustage breaks loose anytime that
                 bogus mergeinfo is parsed.  See
                 http://subversion.tigris.org/issues/show_bug.cgi?id=3020#desc16.
                 */
              continue;
            }

          rev_from_map = get_revision_mapping(pb->rev_map, range->end);
          if (SVN_IS_VALID_REVNUM(rev_from_map))
            range->end = rev_from_map;
        }
      apr_hash_set(final_mergeinfo, merge_source,
                   APR_HASH_KEY_STRING, rangelist);
    }

  if (predates_stream_mergeinfo)
      SVN_ERR(svn_mergeinfo_merge(final_mergeinfo, predates_stream_mergeinfo,
                                  subpool));

  SVN_ERR(svn_mergeinfo_sort(final_mergeinfo, subpool));

  /* Mergeinfo revision sources for r0 and r1 are invalid; you can't merge r0
     or r1.  However, svndumpfilter can be abused to produce r1 merge source
     revs.  So if we encounter any, then strip them out, no need to put them
     into the load target. */
  SVN_ERR(svn_mergeinfo__filter_mergeinfo_by_ranges(&final_mergeinfo,
                                                    final_mergeinfo,
                                                    1, 0, FALSE,
                                                    subpool, subpool));

  SVN_ERR(svn_mergeinfo_to_string(final_val, final_mergeinfo, pool));
  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}

/*----------------------------------------------------------------------*/

/** vtable for doing commits to a fs **/


static svn_error_t *
make_node_baton(struct node_baton **node_baton_p,
                apr_hash_t *headers,
                struct revision_baton *rb,
                apr_pool_t *pool)
{
  struct node_baton *nb = apr_pcalloc(pool, sizeof(*nb));
  const char *val;

  /* Start with sensible defaults. */
  nb->rb = rb;
  nb->pool = pool;
  nb->kind = svn_node_unknown;

  /* Then add info from the headers.  */
  if ((val = apr_hash_get(headers, SVN_REPOS_DUMPFILE_NODE_PATH,
                          APR_HASH_KEY_STRING)))
  {
    val = svn_relpath_canonicalize(val, pool);
    if (rb->pb->parent_dir)
      nb->path = svn_relpath_join(rb->pb->parent_dir, val, pool);
    else
      nb->path = val;
  }

  if ((val = apr_hash_get(headers, SVN_REPOS_DUMPFILE_NODE_KIND,
                          APR_HASH_KEY_STRING)))
    {
      if (! strcmp(val, "file"))
        nb->kind = svn_node_file;
      else if (! strcmp(val, "dir"))
        nb->kind = svn_node_dir;
    }

  nb->action = (enum svn_node_action)(-1);  /* an invalid action code */
  if ((val = apr_hash_get(headers, SVN_REPOS_DUMPFILE_NODE_ACTION,
                          APR_HASH_KEY_STRING)))
    {
      if (! strcmp(val, "change"))
        nb->action = svn_node_action_change;
      else if (! strcmp(val, "add"))
        nb->action = svn_node_action_add;
      else if (! strcmp(val, "delete"))
        nb->action = svn_node_action_delete;
      else if (! strcmp(val, "replace"))
        nb->action = svn_node_action_replace;
    }

  nb->copyfrom_rev = SVN_INVALID_REVNUM;
  if ((val = apr_hash_get(headers, SVN_REPOS_DUMPFILE_NODE_COPYFROM_REV,
                          APR_HASH_KEY_STRING)))
    {
      nb->copyfrom_rev = SVN_STR_TO_REV(val);
    }
  if ((val = apr_hash_get(headers, SVN_REPOS_DUMPFILE_NODE_COPYFROM_PATH,
                          APR_HASH_KEY_STRING)))
    {
      val = svn_relpath_canonicalize(val, pool);
      if (rb->pb->parent_dir)
        nb->copyfrom_path = svn_relpath_join(rb->pb->parent_dir, val, pool);
      else
        nb->copyfrom_path = val;
    }

  if ((val = apr_hash_get(headers, SVN_REPOS_DUMPFILE_TEXT_CONTENT_CHECKSUM,
                          APR_HASH_KEY_STRING)))
    {
      SVN_ERR(svn_checksum_parse_hex(&nb->result_checksum, svn_checksum_md5,
                                     val, pool));
    }

  if ((val = apr_hash_get(headers, SVN_REPOS_DUMPFILE_TEXT_DELTA_BASE_CHECKSUM,
                          APR_HASH_KEY_STRING)))
    {
      SVN_ERR(svn_checksum_parse_hex(&nb->base_checksum, svn_checksum_md5, val,
                                     pool));
    }

  if ((val = apr_hash_get(headers, SVN_REPOS_DUMPFILE_TEXT_COPY_SOURCE_CHECKSUM,
                          APR_HASH_KEY_STRING)))
    {
      SVN_ERR(svn_checksum_parse_hex(&nb->copy_source_checksum,
                                     svn_checksum_md5, val, pool));
    }

  /* What's cool about this dump format is that the parser just
     ignores any unrecognized headers.  :-)  */

  *node_baton_p = nb;
  return SVN_NO_ERROR;
}

static struct revision_baton *
make_revision_baton(apr_hash_t *headers,
                    struct parse_baton *pb,
                    apr_pool_t *pool)
{
  struct revision_baton *rb = apr_pcalloc(pool, sizeof(*rb));
  const char *val;

  rb->pb = pb;
  rb->pool = pool;
  rb->rev = SVN_INVALID_REVNUM;

  if ((val = apr_hash_get(headers, SVN_REPOS_DUMPFILE_REVISION_NUMBER,
                          APR_HASH_KEY_STRING)))
    rb->rev = SVN_STR_TO_REV(val);

  return rb;
}


static svn_error_t *
new_revision_record(void **revision_baton,
                    apr_hash_t *headers,
                    void *parse_baton,
                    apr_pool_t *pool)
{
  struct parse_baton *pb = parse_baton;
  struct revision_baton *rb;
  svn_revnum_t head_rev;

  rb = make_revision_baton(headers, pb, pool);
  SVN_ERR(svn_fs_youngest_rev(&head_rev, pb->fs, pool));

  /* FIXME: This is a lame fallback loading multiple segments of dump in
     several separate operations. It is highly susceptible to race conditions.
     Calculate the revision 'offset' for finding copyfrom sources.
     It might be positive or negative. */
  rb->rev_offset = (apr_int32_t) (rb->rev) - (head_rev + 1);

  if (rb->rev > 0)
    {
      /* Create a new fs txn. */
      SVN_ERR(svn_fs_begin_txn2(&(rb->txn), pb->fs, head_rev, 0, pool));
      SVN_ERR(svn_fs_txn_root(&(rb->txn_root), rb->txn, pool));

      if (pb->notify_func)
        {
          pb->notify->action = svn_repos_notify_load_txn_start;
          pb->notify->old_revision = rb->rev;
          pb->notify_func(pb->notify_baton, pb->notify, rb->pool);
        }

      /* Stash the oldest "old" revision committed from the load stream. */
      if (!SVN_IS_VALID_REVNUM(pb->oldest_old_rev))
        pb->oldest_old_rev = rb->rev;
    }

  /* If we're parsing revision 0, only the revision are (possibly)
     interesting to us: when loading the stream into an empty
     filesystem, then we want new filesystem's revision 0 to have the
     same props.  Otherwise, we just ignore revision 0 in the stream. */

  *revision_baton = rb;
  return SVN_NO_ERROR;
}



/* Factorized helper func for new_node_record() */
static svn_error_t *
maybe_add_with_history(struct node_baton *nb,
                       struct revision_baton *rb,
                       apr_pool_t *pool)
{
  struct parse_baton *pb = rb->pb;

  if ((nb->copyfrom_path == NULL) || (! pb->use_history))
    {
      /* Add empty file or dir, without history. */
      if (nb->kind == svn_node_file)
        SVN_ERR(svn_fs_make_file(rb->txn_root, nb->path, pool));

      else if (nb->kind == svn_node_dir)
        SVN_ERR(svn_fs_make_dir(rb->txn_root, nb->path, pool));
    }
  else
    {
      /* Hunt down the source revision in this fs. */
      svn_fs_root_t *copy_root;
      svn_revnum_t copyfrom_rev;

      /* Try to find the copyfrom revision in the revision map;
         failing that, fall back to the revision offset approach. */
      copyfrom_rev = get_revision_mapping(rb->pb->rev_map, nb->copyfrom_rev);
      if (! SVN_IS_VALID_REVNUM(copyfrom_rev))
        copyfrom_rev = nb->copyfrom_rev - rb->rev_offset;

      if (! SVN_IS_VALID_REVNUM(copyfrom_rev))
        return svn_error_createf(SVN_ERR_FS_NO_SUCH_REVISION, NULL,
                                 _("Relative source revision %ld is not"
                                   " available in current repository"),
                                 copyfrom_rev);

      SVN_ERR(svn_fs_revision_root(&copy_root, pb->fs, copyfrom_rev, pool));

      if (nb->copy_source_checksum)
        {
          svn_checksum_t *checksum;
          SVN_ERR(svn_fs_file_checksum(&checksum, svn_checksum_md5, copy_root,
                                       nb->copyfrom_path, TRUE, pool));
          if (!svn_checksum_match(nb->copy_source_checksum, checksum))
            return svn_checksum_mismatch_err(nb->copy_source_checksum,
                      checksum, pool,
                      _("Copy source checksum mismatch on copy from '%s'@%ld\n"
                        "to '%s' in rev based on r%ld"),
                      nb->copyfrom_path, copyfrom_rev, nb->path, rb->rev);
        }

      SVN_ERR(svn_fs_copy(copy_root, nb->copyfrom_path,
                          rb->txn_root, nb->path, pool));

      if (pb->notify_func)
        {
          pb->notify->action = svn_repos_notify_load_copied_node;
          pb->notify_func(pb->notify_baton, pb->notify, rb->pool);
        }
    }

  return SVN_NO_ERROR;
}


static svn_error_t *
uuid_record(const char *uuid,
            void *parse_baton,
            apr_pool_t *pool)
{
  struct parse_baton *pb = parse_baton;
  svn_revnum_t youngest_rev;

  if (pb->uuid_action == svn_repos_load_uuid_ignore)
    return SVN_NO_ERROR;

  if (pb->uuid_action != svn_repos_load_uuid_force)
    {
      SVN_ERR(svn_fs_youngest_rev(&youngest_rev, pb->fs, pool));
      if (youngest_rev != 0)
        return SVN_NO_ERROR;
    }

  return svn_fs_set_uuid(pb->fs, uuid, pool);
}

static svn_error_t *
new_node_record(void **node_baton,
                apr_hash_t *headers,
                void *revision_baton,
                apr_pool_t *pool)
{
  struct revision_baton *rb = revision_baton;
  struct parse_baton *pb = rb->pb;
  struct node_baton *nb;

  if (rb->rev == 0)
    return svn_error_create(SVN_ERR_STREAM_MALFORMED_DATA, NULL,
                            _("Malformed dumpstream: "
                              "Revision 0 must not contain node records"));

  SVN_ERR(make_node_baton(&nb, headers, rb, pool));

  /* Make sure we have an action we recognize. */
  if (nb->action < svn_node_action_change
        || nb->action > svn_node_action_replace)
      return svn_error_createf(SVN_ERR_STREAM_UNRECOGNIZED_DATA, NULL,
                               _("Unrecognized node-action on node '%s'"),
                               nb->path);

  if (pb->notify_func)
    {
      pb->notify->action = svn_repos_notify_load_node_start;
      pb->notify->node_action = nb->action;
      pb->notify->path = nb->path;
      pb->notify_func(pb->notify_baton, pb->notify, rb->pool);
    }

  switch (nb->action)
    {
    case svn_node_action_change:
      break;

    case svn_node_action_delete:
      SVN_ERR(svn_fs_delete(rb->txn_root, nb->path, pool));
      break;

    case svn_node_action_add:
      SVN_ERR(maybe_add_with_history(nb, rb, pool));
      break;

    case svn_node_action_replace:
      SVN_ERR(svn_fs_delete(rb->txn_root, nb->path, pool));
      SVN_ERR(maybe_add_with_history(nb, rb, pool));
      break;
    }

  *node_baton = nb;
  return SVN_NO_ERROR;
}

static svn_error_t *
set_revision_property(void *baton,
                      const char *name,
                      const svn_string_t *value)
{
  struct revision_baton *rb = baton;

  if (rb->rev > 0)
    {
      if (rb->pb->validate_props)
        SVN_ERR(svn_repos_fs_change_txn_prop(rb->txn, name, value, rb->pool));
      else
        SVN_ERR(svn_fs_change_txn_prop(rb->txn, name, value, rb->pool));

      /* Remember any datestamp that passes through!  (See comment in
         close_revision() below.) */
      if (! strcmp(name, SVN_PROP_REVISION_DATE))
        rb->datestamp = svn_string_dup(value, rb->pool);
    }
  else if (rb->rev == 0)
    {
      /* Special case: set revision 0 properties when loading into an
         'empty' filesystem. */
      struct parse_baton *pb = rb->pb;
      svn_revnum_t youngest_rev;

      SVN_ERR(svn_fs_youngest_rev(&youngest_rev, pb->fs, rb->pool));

      if (youngest_rev == 0)
        SVN_ERR(change_rev_prop(pb->repos, 0, name, value,
                                pb->validate_props, rb->pool));
    }

  return SVN_NO_ERROR;
}


static svn_error_t *
set_node_property(void *baton,
                  const char *name,
                  const svn_string_t *value)
{
  struct node_baton *nb = baton;
  struct revision_baton *rb = nb->rb;
  struct parse_baton *pb = rb->pb;

  if (strcmp(name, SVN_PROP_MERGEINFO) == 0)
    {
      svn_string_t *renumbered_mergeinfo;
      /* ### Need to cast away const. We cannot change the declaration of
       * ### this function since it is part of svn_repos_parse_fns2_t. */
      svn_string_t *prop_val = (svn_string_t *)value;

      /* Tolerate mergeinfo with "\r\n" line endings because some
         dumpstream sources might contain as much.  If so normalize
         the line endings to '\n' and make a notification to
         PARSE_BATON->FEEDBACK_STREAM that we have made this
         correction. */
      if (strstr(prop_val->data, "\r"))
        {
          const char *prop_eol_normalized;

          SVN_ERR(svn_subst_translate_cstring2(prop_val->data,
                                               &prop_eol_normalized,
                                               "\n",  /* translate to LF */
                                               FALSE, /* no repair */
                                               NULL,  /* no keywords */
                                               FALSE, /* no expansion */
                                               nb->pool));
          prop_val->data = prop_eol_normalized;
          prop_val->len = strlen(prop_eol_normalized);

          if (pb->notify_func)
            {
              pb->notify->action = svn_repos_notify_load_normalized_mergeinfo;
              pb->notify_func(pb->notify_baton, pb->notify, nb->pool);
            }
        }

      /* Renumber mergeinfo as appropriate. */
      SVN_ERR(renumber_mergeinfo_revs(&renumbered_mergeinfo, prop_val, rb,
                                      nb->pool));
      value = renumbered_mergeinfo;
      if (pb->parent_dir)
        {
          /* Prefix the merge source paths with PB->parent_dir. */
          /* ASSUMPTION: All source paths are included in the dump stream. */
          svn_string_t *mergeinfo_val;
          SVN_ERR(prefix_mergeinfo_paths(&mergeinfo_val, value,
                                         pb->parent_dir, nb->pool));
          value = mergeinfo_val;
        }
    }

  return change_node_prop(rb->txn_root, nb->path, name, value,
                          pb->validate_props, nb->pool);
}


static svn_error_t *
delete_node_property(void *baton,
                     const char *name)
{
  struct node_baton *nb = baton;
  struct revision_baton *rb = nb->rb;

  return change_node_prop(rb->txn_root, nb->path, name, NULL,
                          rb->pb->validate_props, nb->pool);
}


static svn_error_t *
remove_node_props(void *baton)
{
  struct node_baton *nb = baton;
  struct revision_baton *rb = nb->rb;
  apr_hash_t *proplist;
  apr_hash_index_t *hi;

  SVN_ERR(svn_fs_node_proplist(&proplist,
                               rb->txn_root, nb->path, nb->pool));

  for (hi = apr_hash_first(nb->pool, proplist); hi; hi = apr_hash_next(hi))
    {
      const void *key;

      apr_hash_this(hi, &key, NULL, NULL);
      SVN_ERR(change_node_prop(rb->txn_root, nb->path, key, NULL,
                               rb->pb->validate_props, nb->pool));
    }

  return SVN_NO_ERROR;
}


static svn_error_t *
apply_textdelta(svn_txdelta_window_handler_t *handler,
                void **handler_baton,
                void *node_baton)
{
  struct node_baton *nb = node_baton;
  struct revision_baton *rb = nb->rb;

  return svn_fs_apply_textdelta(handler, handler_baton,
                                rb->txn_root, nb->path,
                                svn_checksum_to_cstring(nb->base_checksum,
                                                        nb->pool),
                                svn_checksum_to_cstring(nb->result_checksum,
                                                        nb->pool),
                                nb->pool);
}


static svn_error_t *
set_fulltext(svn_stream_t **stream,
             void *node_baton)
{
  struct node_baton *nb = node_baton;
  struct revision_baton *rb = nb->rb;

  return svn_fs_apply_text(stream,
                           rb->txn_root, nb->path,
                           svn_checksum_to_cstring(nb->result_checksum,
                                                   nb->pool),
                           nb->pool);
}


static svn_error_t *
close_node(void *baton)
{
  struct node_baton *nb = baton;
  struct revision_baton *rb = nb->rb;
  struct parse_baton *pb = rb->pb;

  if (pb->notify_func)
    {
      pb->notify->action = svn_repos_notify_load_node_done;
      pb->notify_func(pb->notify_baton, pb->notify, rb->pool);
    }

  return SVN_NO_ERROR;
}


static svn_error_t *
close_revision(void *baton)
{
  struct revision_baton *rb = baton;
  struct parse_baton *pb = rb->pb;
  const char *conflict_msg = NULL;
  svn_revnum_t committed_rev;
  svn_error_t *err;

  if (rb->rev <= 0)
    return SVN_NO_ERROR;

  /* Run the pre-commit hook, if so commanded. */
  if (pb->use_pre_commit_hook)
    {
      const char *txn_name;
      err = svn_fs_txn_name(&txn_name, rb->txn, rb->pool);
      if (! err)
        err = svn_repos__hooks_pre_commit(pb->repos, txn_name, rb->pool);
      if (err)
        {
          svn_error_clear(svn_fs_abort_txn(rb->txn, rb->pool));
          return svn_error_trace(err);
        }
    }

  /* Commit. */
  err = svn_fs_commit_txn(&conflict_msg, &committed_rev, rb->txn, rb->pool);
  if (SVN_IS_VALID_REVNUM(committed_rev))
    {
      if (err)
        {
          /* ### Log any error, but better yet is to rev
             ### close_revision()'s API to allow both committed_rev and err
             ### to be returned, see #3768. */
          svn_error_clear(err);
        }
    }
  else
    {
      svn_error_clear(svn_fs_abort_txn(rb->txn, rb->pool));
      if (conflict_msg)
        return svn_error_quick_wrap(err, conflict_msg);
      else
        return svn_error_trace(err);
    }

  /* Run post-commit hook, if so commanded.  */
  if (pb->use_post_commit_hook)
    {
      if ((err = svn_repos__hooks_post_commit(pb->repos, committed_rev,
                                              rb->pool)))
        return svn_error_create
          (SVN_ERR_REPOS_POST_COMMIT_HOOK_FAILED, err,
           _("Commit succeeded, but post-commit hook failed"));
    }

  /* After a successful commit, must record the dump-rev -> in-repos-rev
     mapping, so that copyfrom instructions in the dump file can look up the
     correct repository revision to copy from. */
  set_revision_mapping(pb->rev_map, rb->rev, committed_rev);

  /* If the incoming dump stream has non-contiguous revisions (e.g. from
     using svndumpfilter --drop-empty-revs without --renumber-revs) then
     we must account for the missing gaps in PB->REV_MAP.  Otherwise we
     might not be able to map all mergeinfo source revisions to the correct
     revisions in the target repos. */
  if ((pb->last_rev_mapped != SVN_INVALID_REVNUM)
      && (rb->rev != pb->last_rev_mapped + 1))
    {
      svn_revnum_t i;

      for (i = pb->last_rev_mapped + 1; i < rb->rev; i++)
        {
          set_revision_mapping(pb->rev_map, i, pb->last_rev_mapped);
        }
    }

  /* Update our "last revision mapped". */
  pb->last_rev_mapped = rb->rev;

  /* Deltify the predecessors of paths changed in this revision. */
  SVN_ERR(svn_fs_deltify_revision(pb->fs, committed_rev, rb->pool));

  /* Grrr, svn_fs_commit_txn rewrites the datestamp property to the
     current clock-time.  We don't want that, we want to preserve
     history exactly.  Good thing revision props aren't versioned!
     Note that if rb->datestamp is NULL, that's fine -- if the dump
     data doesn't carry a datestamp, we want to preserve that fact in
     the load. */
  SVN_ERR(change_rev_prop(pb->repos, committed_rev, SVN_PROP_REVISION_DATE,
                          rb->datestamp, pb->validate_props, rb->pool));

  if (pb->notify_func)
    {
      pb->notify->action = svn_repos_notify_load_txn_committed;
      pb->notify->new_revision = committed_rev;
      pb->notify->old_revision = ((committed_rev == rb->rev)
                                    ? SVN_INVALID_REVNUM
                                    : rb->rev);
      pb->notify_func(pb->notify_baton, pb->notify, rb->pool);
    }

  return SVN_NO_ERROR;
}


/*----------------------------------------------------------------------*/

/** The public routines **/


svn_error_t *
svn_repos_get_fs_build_parser3(const svn_repos_parse_fns2_t **callbacks,
                               void **parse_baton,
                               svn_repos_t *repos,
                               svn_boolean_t use_history,
                               svn_boolean_t validate_props,
                               enum svn_repos_load_uuid uuid_action,
                               const char *parent_dir,
                               svn_repos_notify_func_t notify_func,
                               void *notify_baton,
                               apr_pool_t *pool)
{
  svn_repos_parse_fns2_t *parser = apr_pcalloc(pool, sizeof(*parser));
  struct parse_baton *pb = apr_pcalloc(pool, sizeof(*pb));

  if (parent_dir)
    parent_dir = svn_relpath_canonicalize(parent_dir, pool);

  parser->new_revision_record = new_revision_record;
  parser->new_node_record = new_node_record;
  parser->uuid_record = uuid_record;
  parser->set_revision_property = set_revision_property;
  parser->set_node_property = set_node_property;
  parser->remove_node_props = remove_node_props;
  parser->set_fulltext = set_fulltext;
  parser->close_node = close_node;
  parser->close_revision = close_revision;
  parser->delete_node_property = delete_node_property;
  parser->apply_textdelta = apply_textdelta;

  pb->repos = repos;
  pb->fs = svn_repos_fs(repos);
  pb->use_history = use_history;
  pb->validate_props = validate_props;
  pb->notify_func = notify_func;
  pb->notify_baton = notify_baton;
  pb->notify = svn_repos_notify_create(svn_repos_notify_load_txn_start, pool);
  pb->uuid_action = uuid_action;
  pb->parent_dir = parent_dir;
  pb->pool = pool;
  pb->rev_map = apr_hash_make(pool);
  pb->oldest_old_rev = SVN_INVALID_REVNUM;
  pb->last_rev_mapped = SVN_INVALID_REVNUM;

  *callbacks = parser;
  *parse_baton = pb;
  return SVN_NO_ERROR;
}



svn_error_t *
svn_repos_load_fs3(svn_repos_t *repos,
                   svn_stream_t *dumpstream,
                   enum svn_repos_load_uuid uuid_action,
                   const char *parent_dir,
                   svn_boolean_t use_pre_commit_hook,
                   svn_boolean_t use_post_commit_hook,
                   svn_boolean_t validate_props,
                   svn_repos_notify_func_t notify_func,
                   void *notify_baton,
                   svn_cancel_func_t cancel_func,
                   void *cancel_baton,
                   apr_pool_t *pool)
{
  const svn_repos_parse_fns2_t *parser;
  void *parse_baton;
  struct parse_baton *pb;

  /* This is really simple. */

  SVN_ERR(svn_repos_get_fs_build_parser3(&parser, &parse_baton,
                                         repos,
                                         TRUE, /* look for copyfrom revs */
                                         validate_props,
                                         uuid_action,
                                         parent_dir,
                                         notify_func,
                                         notify_baton,
                                         pool));

  /* Heh.  We know this is a parse_baton.  This file made it.  So
     cast away, and set our hook booleans.  */
  pb = parse_baton;
  pb->use_pre_commit_hook = use_pre_commit_hook;
  pb->use_post_commit_hook = use_post_commit_hook;

  return svn_repos_parse_dumpstream2(dumpstream, parser, parse_baton,
                                     cancel_func, cancel_baton, pool);
}

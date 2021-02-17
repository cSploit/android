/* rev_hunt.c --- routines to hunt down particular fs revisions and
 *                their properties.
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
#include "svn_compat.h"
#include "svn_private_config.h"
#include "svn_pools.h"
#include "svn_error.h"
#include "svn_error_codes.h"
#include "svn_fs.h"
#include "svn_repos.h"
#include "svn_string.h"
#include "svn_time.h"
#include "svn_sorts.h"
#include "svn_props.h"
#include "svn_mergeinfo.h"
#include "repos.h"
#include "private/svn_fspath.h"


/* Note:  this binary search assumes that the datestamp properties on
   each revision are in chronological order.  That is if revision A >
   revision B, then A's datestamp is younger then B's datestamp.

   If someone comes along and sets a bogus datestamp, this routine
   might not work right.

   ### todo:  you know, we *could* have svn_fs_change_rev_prop() do
   some semantic checking when it's asked to change special reserved
   svn: properties.  It could prevent such a problem. */


/* helper for svn_repos_dated_revision().

   Set *TM to the apr_time_t datestamp on revision REV in FS. */
static svn_error_t *
get_time(apr_time_t *tm,
         svn_fs_t *fs,
         svn_revnum_t rev,
         apr_pool_t *pool)
{
  svn_string_t *date_str;

  SVN_ERR(svn_fs_revision_prop(&date_str, fs, rev, SVN_PROP_REVISION_DATE,
                               pool));
  if (! date_str)
    return svn_error_createf
      (SVN_ERR_FS_GENERAL, NULL,
       _("Failed to find time on revision %ld"), rev);

  return svn_time_from_cstring(tm, date_str->data, pool);
}


svn_error_t *
svn_repos_dated_revision(svn_revnum_t *revision,
                         svn_repos_t *repos,
                         apr_time_t tm,
                         apr_pool_t *pool)
{
  svn_revnum_t rev_mid, rev_top, rev_bot, rev_latest;
  apr_time_t this_time;
  svn_fs_t *fs = repos->fs;

  /* Initialize top and bottom values of binary search. */
  SVN_ERR(svn_fs_youngest_rev(&rev_latest, fs, pool));
  rev_bot = 0;
  rev_top = rev_latest;

  while (rev_bot <= rev_top)
    {
      rev_mid = (rev_top + rev_bot) / 2;
      SVN_ERR(get_time(&this_time, fs, rev_mid, pool));

      if (this_time > tm)/* we've overshot */
        {
          apr_time_t previous_time;

          if ((rev_mid - 1) < 0)
            {
              *revision = 0;
              break;
            }

          /* see if time falls between rev_mid and rev_mid-1: */
          SVN_ERR(get_time(&previous_time, fs, rev_mid - 1, pool));
          if (previous_time <= tm)
            {
              *revision = rev_mid - 1;
              break;
            }

          rev_top = rev_mid - 1;
        }

      else if (this_time < tm) /* we've undershot */
        {
          apr_time_t next_time;

          if ((rev_mid + 1) > rev_latest)
            {
              *revision = rev_latest;
              break;
            }

          /* see if time falls between rev_mid and rev_mid+1: */
          SVN_ERR(get_time(&next_time, fs, rev_mid + 1, pool));
          if (next_time > tm)
            {
              *revision = rev_mid;
              break;
            }

          rev_bot = rev_mid + 1;
        }

      else
        {
          *revision = rev_mid;  /* exact match! */
          break;
        }
    }

  return SVN_NO_ERROR;
}


svn_error_t *
svn_repos_get_committed_info(svn_revnum_t *committed_rev,
                             const char **committed_date,
                             const char **last_author,
                             svn_fs_root_t *root,
                             const char *path,
                             apr_pool_t *pool)
{
  svn_fs_t *fs = svn_fs_root_fs(root);

  /* ### It might be simpler just to declare that revision
     properties have char * (i.e., UTF-8) values, not arbitrary
     binary values, hmmm. */
  svn_string_t *committed_date_s, *last_author_s;

  /* Get the CR field out of the node's skel. */
  SVN_ERR(svn_fs_node_created_rev(committed_rev, root, path, pool));

  /* Get the date property of this revision. */
  SVN_ERR(svn_fs_revision_prop(&committed_date_s, fs, *committed_rev,
                               SVN_PROP_REVISION_DATE, pool));

  /* Get the author property of this revision. */
  SVN_ERR(svn_fs_revision_prop(&last_author_s, fs, *committed_rev,
                               SVN_PROP_REVISION_AUTHOR, pool));

  *committed_date = committed_date_s ? committed_date_s->data : NULL;
  *last_author = last_author_s ? last_author_s->data : NULL;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_repos_history2(svn_fs_t *fs,
                   const char *path,
                   svn_repos_history_func_t history_func,
                   void *history_baton,
                   svn_repos_authz_func_t authz_read_func,
                   void *authz_read_baton,
                   svn_revnum_t start,
                   svn_revnum_t end,
                   svn_boolean_t cross_copies,
                   apr_pool_t *pool)
{
  svn_fs_history_t *history;
  apr_pool_t *oldpool = svn_pool_create(pool);
  apr_pool_t *newpool = svn_pool_create(pool);
  const char *history_path;
  svn_revnum_t history_rev;
  svn_fs_root_t *root;

  /* Validate the revisions. */
  if (! SVN_IS_VALID_REVNUM(start))
    return svn_error_createf
      (SVN_ERR_FS_NO_SUCH_REVISION, 0,
       _("Invalid start revision %ld"), start);
  if (! SVN_IS_VALID_REVNUM(end))
    return svn_error_createf
      (SVN_ERR_FS_NO_SUCH_REVISION, 0,
       _("Invalid end revision %ld"), end);

  /* Ensure that the input is ordered. */
  if (start > end)
    {
      svn_revnum_t tmprev = start;
      start = end;
      end = tmprev;
    }

  /* Get a revision root for END, and an initial HISTORY baton.  */
  SVN_ERR(svn_fs_revision_root(&root, fs, end, pool));

  if (authz_read_func)
    {
      svn_boolean_t readable;
      SVN_ERR(authz_read_func(&readable, root, path,
                              authz_read_baton, pool));
      if (! readable)
        return svn_error_create(SVN_ERR_AUTHZ_UNREADABLE, NULL, NULL);
    }

  SVN_ERR(svn_fs_node_history(&history, root, path, oldpool));

  /* Now, we loop over the history items, calling svn_fs_history_prev(). */
  do
    {
      /* Note that we have to do some crazy pool work here.  We can't
         get rid of the old history until we use it to get the new, so
         we alternate back and forth between our subpools.  */
      apr_pool_t *tmppool;
      svn_error_t *err;

      SVN_ERR(svn_fs_history_prev(&history, history, cross_copies, newpool));

      /* Only continue if there is further history to deal with. */
      if (! history)
        break;

      /* Fetch the location information for this history step. */
      SVN_ERR(svn_fs_history_location(&history_path, &history_rev,
                                      history, newpool));

      /* If this history item predates our START revision, quit
         here. */
      if (history_rev < start)
        break;

      /* Is the history item readable?  If not, quit. */
      if (authz_read_func)
        {
          svn_boolean_t readable;
          svn_fs_root_t *history_root;
          SVN_ERR(svn_fs_revision_root(&history_root, fs,
                                       history_rev, newpool));
          SVN_ERR(authz_read_func(&readable, history_root, history_path,
                                  authz_read_baton, newpool));
          if (! readable)
            break;
        }

      /* Call the user-provided callback function. */
      err = history_func(history_baton, history_path, history_rev, newpool);
      if (err)
        {
          if (err->apr_err == SVN_ERR_CEASE_INVOCATION)
            {
              svn_error_clear(err);
              goto cleanup;
            }
          else
            {
              return svn_error_trace(err);
            }
        }

      /* We're done with the old history item, so we can clear its
         pool, and then toggle our notion of "the old pool". */
      svn_pool_clear(oldpool);
      tmppool = oldpool;
      oldpool = newpool;
      newpool = tmppool;
    }
  while (history); /* shouldn't hit this */

 cleanup:
  svn_pool_destroy(oldpool);
  svn_pool_destroy(newpool);
  return SVN_NO_ERROR;
}


svn_error_t *
svn_repos_deleted_rev(svn_fs_t *fs,
                      const char *path,
                      svn_revnum_t start,
                      svn_revnum_t end,
                      svn_revnum_t *deleted,
                      apr_pool_t *pool)
{
  apr_pool_t *subpool;
  svn_fs_root_t *root, *copy_root;
  const char *copy_path;
  svn_revnum_t mid_rev;
  const svn_fs_id_t *start_node_id, *curr_node_id;
  svn_error_t *err;

  /* Validate the revision range. */
  if (! SVN_IS_VALID_REVNUM(start))
    return svn_error_createf
      (SVN_ERR_FS_NO_SUCH_REVISION, 0,
       _("Invalid start revision %ld"), start);
  if (! SVN_IS_VALID_REVNUM(end))
    return svn_error_createf
      (SVN_ERR_FS_NO_SUCH_REVISION, 0,
       _("Invalid end revision %ld"), end);

  /* Ensure that the input is ordered. */
  if (start > end)
    {
      svn_revnum_t tmprev = start;
      start = end;
      end = tmprev;
    }

  /* Ensure path exists in fs at start revision. */
  SVN_ERR(svn_fs_revision_root(&root, fs, start, pool));
  err = svn_fs_node_id(&start_node_id, root, path, pool);
  if (err)
    {
      if (err->apr_err == SVN_ERR_FS_NOT_FOUND)
        {
          /* Path must exist in fs at start rev. */
          *deleted = SVN_INVALID_REVNUM;
          svn_error_clear(err);
          return SVN_NO_ERROR;
        }
      return svn_error_trace(err);
    }

  /* Ensure path was deleted at or before end revision. */
  SVN_ERR(svn_fs_revision_root(&root, fs, end, pool));
  err = svn_fs_node_id(&curr_node_id, root, path, pool);
  if (err && err->apr_err == SVN_ERR_FS_NOT_FOUND)
    {
      svn_error_clear(err);
    }
  else if (err)
    {
      return svn_error_trace(err);
    }
  else
    {
      /* path exists in the end node and the end node is equivalent
         or otherwise equivalent to the start node.  This can mean
         a few things:

           1) The end node *is* simply the start node, uncopied
              and unmodified in the start to end range.

           2) The start node was modified, but never copied.

           3) The start node was copied, but this copy occurred at
              start or some rev *previous* to start, this is
              effectively the same situation as 1 if the node was
              never modified or 2 if it was.

         In the first three cases the path was not deleted in
         the specified range and we are done, but in the following
         cases the start node must have been deleted at least once:

           4) The start node was deleted and replaced by a copy of
              itself at some rev between start and end.  This copy
              may itself have been replaced with copies of itself.

           5) The start node was deleted and replaced by a node which
              it does not share any history with.
      */
      SVN_ERR(svn_fs_node_id(&curr_node_id, root, path, pool));
      if (svn_fs_compare_ids(start_node_id, curr_node_id) != -1)
        {
          SVN_ERR(svn_fs_closest_copy(&copy_root, &copy_path, root,
                                      path, pool));
          if (!copy_root ||
              (svn_fs_revision_root_revision(copy_root) <= start))
            {
              /* Case 1,2 or 3, nothing more to do. */
              *deleted = SVN_INVALID_REVNUM;
              return SVN_NO_ERROR;
            }
        }
    }

  /* If we get here we know that path exists in rev start and was deleted
     at least once before rev end.  To find the revision path was first
     deleted we use a binary search.  The rules for the determining if
     the deletion comes before or after a given median revision are
     described by this matrix:

                   |             Most recent copy event that
                   |               caused mid node to exist.
                   |-----------------------------------------------------
     Compare path  |                   |                |               |
     at start and  |   Copied at       |  Copied at     | Never copied  |
     mid nodes.    |   rev > start     |  rev <= start  |               |
                   |                   |                |               |
     -------------------------------------------------------------------|
     Mid node is   |  A) Start node    |                                |
     equivalent to |     replaced with |  E) Mid node == start node,    |
     start node    |     an unmodified |     look HIGHER.               |
                   |     copy of       |                                |
                   |     itself,       |                                |
                   |     look LOWER.   |                                |
     -------------------------------------------------------------------|
     Mid node is   |  B) Start node    |                                |
     otherwise     |     replaced with |  F) Mid node is a modified     |
     related to    |     a modified    |     version of start node,     |
     start node    |     copy of       |     look HIGHER.               |
                   |     itself,       |                                |
                   |     look LOWER.   |                                |
     -------------------------------------------------------------------|
     Mid node is   |                                                    |
     unrelated to  |  C) Start node replaced with unrelated mid node,   |
     start node    |     look LOWER.                                    |
                   |                                                    |
     -------------------------------------------------------------------|
     Path doesn't  |                                                    |
     exist at mid  |  D) Start node deleted before mid node,            |
     node          |     look LOWER                                     |
                   |                                                    |
     --------------------------------------------------------------------
  */

  mid_rev = (start + end) / 2;
  subpool = svn_pool_create(pool);

  while (1)
    {
      svn_pool_clear(subpool);

      /* Get revision root and node id for mid_rev at that revision. */
      SVN_ERR(svn_fs_revision_root(&root, fs, mid_rev, subpool));
      err = svn_fs_node_id(&curr_node_id, root, path, subpool);

      if (err)
        {
          if (err->apr_err == SVN_ERR_FS_NOT_FOUND)
            {
              /* Case D: Look lower in the range. */
              svn_error_clear(err);
              end = mid_rev;
              mid_rev = (start + mid_rev) / 2;
            }
          else
            return svn_error_trace(err);
        }
      else
        {
          /* Determine the relationship between the start node
             and the current node. */
          int cmp = svn_fs_compare_ids(start_node_id, curr_node_id);
          SVN_ERR(svn_fs_closest_copy(&copy_root, &copy_path, root,
                                      path, subpool));
          if (cmp == -1 ||
              (copy_root &&
               (svn_fs_revision_root_revision(copy_root) > start)))
            {
              /* Cases A, B, C: Look at lower revs. */
              end = mid_rev;
              mid_rev = (start + mid_rev) / 2;
            }
          else if (end - mid_rev == 1)
            {
              /* Found the node path was deleted. */
              *deleted = end;
              break;
            }
          else
            {
              /* Cases E, F: Look at higher revs. */
              start = mid_rev;
              mid_rev = (start + end) / 2;
            }
        }
    }

  svn_pool_destroy(subpool);
  return SVN_NO_ERROR;
}


/* Helper func:  return SVN_ERR_AUTHZ_UNREADABLE if ROOT/PATH is
   unreadable. */
static svn_error_t *
check_readability(svn_fs_root_t *root,
                  const char *path,
                  svn_repos_authz_func_t authz_read_func,
                  void *authz_read_baton,
                  apr_pool_t *pool)
{
  svn_boolean_t readable;
  SVN_ERR(authz_read_func(&readable, root, path, authz_read_baton, pool));
  if (! readable)
    return svn_error_create(SVN_ERR_AUTHZ_UNREADABLE, NULL,
                            _("Unreadable path encountered; access denied"));
  return SVN_NO_ERROR;
}


/* The purpose of this function is to discover if fs_path@future_rev
 * is derived from fs_path@peg_rev.  The return is placed in *is_ancestor. */

static svn_error_t *
check_ancestry_of_peg_path(svn_boolean_t *is_ancestor,
                           svn_fs_t *fs,
                           const char *fs_path,
                           svn_revnum_t peg_revision,
                           svn_revnum_t future_revision,
                           apr_pool_t *pool)
{
  svn_fs_root_t *root;
  svn_fs_history_t *history;
  const char *path;
  svn_revnum_t revision;
  apr_pool_t *lastpool, *currpool;

  lastpool = svn_pool_create(pool);
  currpool = svn_pool_create(pool);

  SVN_ERR(svn_fs_revision_root(&root, fs, future_revision, pool));

  SVN_ERR(svn_fs_node_history(&history, root, fs_path, lastpool));

  /* Since paths that are different according to strcmp may still be
     equivalent (due to number of consecutive slashes and the fact that
     "" is the same as "/"), we get the "canonical" path in the first
     iteration below so that the comparison after the loop will work
     correctly. */
  fs_path = NULL;

  while (1)
    {
      apr_pool_t *tmppool;

      SVN_ERR(svn_fs_history_prev(&history, history, TRUE, currpool));

      if (!history)
        break;

      SVN_ERR(svn_fs_history_location(&path, &revision, history, currpool));

      if (!fs_path)
        fs_path = apr_pstrdup(pool, path);

      if (revision <= peg_revision)
        break;

      /* Clear old pool and flip. */
      svn_pool_clear(lastpool);
      tmppool = lastpool;
      lastpool = currpool;
      currpool = tmppool;
    }

  /* We must have had at least one iteration above where we
     reassigned fs_path. Else, the path wouldn't have existed at
     future_revision and svn_fs_history would have thrown. */
  SVN_ERR_ASSERT(fs_path != NULL);

  *is_ancestor = (history && strcmp(path, fs_path) == 0);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_repos__prev_location(svn_revnum_t *appeared_rev,
                         const char **prev_path,
                         svn_revnum_t *prev_rev,
                         svn_fs_t *fs,
                         svn_revnum_t revision,
                         const char *path,
                         apr_pool_t *pool)
{
  svn_fs_root_t *root, *copy_root;
  const char *copy_path, *copy_src_path, *remainder = "";
  svn_revnum_t copy_src_rev;

  /* Initialize return variables. */
  if (appeared_rev)
    *appeared_rev = SVN_INVALID_REVNUM;
  if (prev_rev)
    *prev_rev = SVN_INVALID_REVNUM;
  if (prev_path)
    *prev_path = NULL;

  /* Ask about the most recent copy which affected PATH@REVISION.  If
     there was no such copy, we're done.  */
  SVN_ERR(svn_fs_revision_root(&root, fs, revision, pool));
  SVN_ERR(svn_fs_closest_copy(&copy_root, &copy_path, root, path, pool));
  if (! copy_root)
    return SVN_NO_ERROR;

  /* Ultimately, it's not the path of the closest copy's source that
     we care about -- it's our own path's location in the copy source
     revision.  So we'll tack the relative path that expresses the
     difference between the copy destination and our path in the copy
     revision onto the copy source path to determine this information.

     In other words, if our path is "/branches/my-branch/foo/bar", and
     we know that the closest relevant copy was a copy of "/trunk" to
     "/branches/my-branch", then that relative path under the copy
     destination is "/foo/bar".  Tacking that onto the copy source
     path tells us that our path was located at "/trunk/foo/bar"
     before the copy.
  */
  SVN_ERR(svn_fs_copied_from(&copy_src_rev, &copy_src_path,
                             copy_root, copy_path, pool));
  if (! strcmp(copy_path, path) == 0)
    remainder = svn_fspath__is_child(copy_path, path, pool);
  if (prev_path)
    *prev_path = svn_fspath__join(copy_src_path, remainder, pool);
  if (appeared_rev)
    *appeared_rev = svn_fs_revision_root_revision(copy_root);
  if (prev_rev)
    *prev_rev = copy_src_rev;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_repos_trace_node_locations(svn_fs_t *fs,
                               apr_hash_t **locations,
                               const char *fs_path,
                               svn_revnum_t peg_revision,
                               const apr_array_header_t *location_revisions_orig,
                               svn_repos_authz_func_t authz_read_func,
                               void *authz_read_baton,
                               apr_pool_t *pool)
{
  apr_array_header_t *location_revisions;
  svn_revnum_t *revision_ptr, *revision_ptr_end;
  svn_fs_root_t *root;
  const char *path;
  svn_revnum_t revision;
  svn_boolean_t is_ancestor;
  apr_pool_t *lastpool, *currpool;
  const svn_fs_id_t *id;

  SVN_ERR_ASSERT(location_revisions_orig->elt_size == sizeof(svn_revnum_t));

  /* Ensure that FS_PATH is absolute, because our path-math below will
     depend on that being the case.  */
  if (*fs_path != '/')
    fs_path = apr_pstrcat(pool, "/", fs_path, (char *)NULL);

  /* Another sanity check. */
  if (authz_read_func)
    {
      svn_fs_root_t *peg_root;
      SVN_ERR(svn_fs_revision_root(&peg_root, fs, peg_revision, pool));
      SVN_ERR(check_readability(peg_root, fs_path,
                                authz_read_func, authz_read_baton, pool));
    }

  *locations = apr_hash_make(pool);

  /* We flip between two pools in the second loop below. */
  lastpool = svn_pool_create(pool);
  currpool = svn_pool_create(pool);

  /* First - let's sort the array of the revisions from the greatest revision
   * downward, so it will be easier to search on. */
  location_revisions = apr_array_copy(pool, location_revisions_orig);
  qsort(location_revisions->elts, location_revisions->nelts,
        sizeof(*revision_ptr), svn_sort_compare_revisions);

  revision_ptr = (svn_revnum_t *)location_revisions->elts;
  revision_ptr_end = revision_ptr + location_revisions->nelts;

  /* Ignore revisions R that are younger than the peg_revisions where
     path@peg_revision is not an ancestor of path@R. */
  is_ancestor = FALSE;
  while (revision_ptr < revision_ptr_end && *revision_ptr > peg_revision)
    {
      svn_pool_clear(currpool);
      SVN_ERR(check_ancestry_of_peg_path(&is_ancestor, fs, fs_path,
                                         peg_revision, *revision_ptr,
                                         currpool));
      if (is_ancestor)
        break;
      ++revision_ptr;
    }

  revision = is_ancestor ? *revision_ptr : peg_revision;
  path = fs_path;
  if (authz_read_func)
    {
      SVN_ERR(svn_fs_revision_root(&root, fs, revision, pool));
      SVN_ERR(check_readability(root, fs_path, authz_read_func,
                                authz_read_baton, pool));
    }

  while (revision_ptr < revision_ptr_end)
    {
      apr_pool_t *tmppool;
      svn_revnum_t appeared_rev, prev_rev;
      const char *prev_path;

      /* Find the target of the innermost copy relevant to path@revision.
         The copy may be of path itself, or of a parent directory. */
      SVN_ERR(svn_repos__prev_location(&appeared_rev, &prev_path, &prev_rev,
                                       fs, revision, path, currpool));
      if (! prev_path)
        break;

      if (authz_read_func)
        {
          svn_boolean_t readable;
          svn_fs_root_t *tmp_root;

          SVN_ERR(svn_fs_revision_root(&tmp_root, fs, revision, currpool));
          SVN_ERR(authz_read_func(&readable, tmp_root, path,
                                  authz_read_baton, currpool));
          if (! readable)
            {
              svn_pool_destroy(lastpool);
              svn_pool_destroy(currpool);

              return SVN_NO_ERROR;
            }
        }

      /* Assign the current path to all younger revisions until we reach
         the copy target rev. */
      while ((revision_ptr < revision_ptr_end)
             && (*revision_ptr >= appeared_rev))
        {
          /* *revision_ptr is allocated out of pool, so we can point
             to in the hash table. */
          apr_hash_set(*locations, revision_ptr, sizeof(*revision_ptr),
                       apr_pstrdup(pool, path));
          revision_ptr++;
        }

      /* Ignore all revs between the copy target rev and the copy
         source rev (non-inclusive). */
      while ((revision_ptr < revision_ptr_end)
             && (*revision_ptr > prev_rev))
        revision_ptr++;

      /* State update. */
      path = prev_path;
      revision = prev_rev;

      /* Clear last pool and switch. */
      svn_pool_clear(lastpool);
      tmppool = lastpool;
      lastpool = currpool;
      currpool = tmppool;
    }

  /* There are no copies relevant to path@revision.  So any remaining
     revisions either predate the creation of path@revision or have
     the node existing at the same path.  We will look up path@lrev
     for each remaining location-revision and make sure it is related
     to path@revision. */
  SVN_ERR(svn_fs_revision_root(&root, fs, revision, currpool));
  SVN_ERR(svn_fs_node_id(&id, root, path, pool));
  while (revision_ptr < revision_ptr_end)
    {
      svn_node_kind_t kind;
      const svn_fs_id_t *lrev_id;

      svn_pool_clear(currpool);
      SVN_ERR(svn_fs_revision_root(&root, fs, *revision_ptr, currpool));
      SVN_ERR(svn_fs_check_path(&kind, root, path, currpool));
      if (kind == svn_node_none)
        break;
      SVN_ERR(svn_fs_node_id(&lrev_id, root, path, currpool));
      if (! svn_fs_check_related(id, lrev_id))
        break;

      /* The node exists at the same path; record that and advance. */
      apr_hash_set(*locations, revision_ptr, sizeof(*revision_ptr),
                   apr_pstrdup(pool, path));
      revision_ptr++;
    }

  /* Ignore any remaining location-revisions; they predate the
     creation of path@revision. */

  svn_pool_destroy(lastpool);
  svn_pool_destroy(currpool);

  return SVN_NO_ERROR;
}


/* Transmit SEGMENT through RECEIVER/RECEIVER_BATON iff a portion of
   its revision range fits between END_REV and START_REV, possibly
   cropping the range so that it fits *entirely* in that range. */
static svn_error_t *
maybe_crop_and_send_segment(svn_location_segment_t *segment,
                            svn_revnum_t start_rev,
                            svn_revnum_t end_rev,
                            svn_location_segment_receiver_t receiver,
                            void *receiver_baton,
                            apr_pool_t *pool)
{
  /* We only want to transmit this segment if some portion of it
     is between our END_REV and START_REV. */
  if (! ((segment->range_start > start_rev)
         || (segment->range_end < end_rev)))
    {
      /* Correct our segment range when the range straddles one of
         our requested revision boundaries. */
      if (segment->range_start < end_rev)
        segment->range_start = end_rev;
      if (segment->range_end > start_rev)
        segment->range_end = start_rev;
      SVN_ERR(receiver(segment, receiver_baton, pool));
    }
  return SVN_NO_ERROR;
}


svn_error_t *
svn_repos_node_location_segments(svn_repos_t *repos,
                                 const char *path,
                                 svn_revnum_t peg_revision,
                                 svn_revnum_t start_rev,
                                 svn_revnum_t end_rev,
                                 svn_location_segment_receiver_t receiver,
                                 void *receiver_baton,
                                 svn_repos_authz_func_t authz_read_func,
                                 void *authz_read_baton,
                                 apr_pool_t *pool)
{
  svn_fs_t *fs = svn_repos_fs(repos);
  svn_stringbuf_t *current_path;
  svn_revnum_t youngest_rev = SVN_INVALID_REVNUM, current_rev;
  apr_pool_t *subpool;

  /* No PEG_REVISION?  We'll use HEAD. */
  if (! SVN_IS_VALID_REVNUM(peg_revision))
    {
      SVN_ERR(svn_fs_youngest_rev(&youngest_rev, fs, pool));
      peg_revision = youngest_rev;
    }

  /* No START_REV?  We'll use HEAD (which we may have already fetched). */
  if (! SVN_IS_VALID_REVNUM(start_rev))
    {
      if (SVN_IS_VALID_REVNUM(youngest_rev))
        start_rev = youngest_rev;
      else
        SVN_ERR(svn_fs_youngest_rev(&start_rev, fs, pool));
    }

  /* No END_REV?  We'll use 0. */
  end_rev = SVN_IS_VALID_REVNUM(end_rev) ? end_rev : 0;

  /* Are the revision properly ordered?  They better be -- the API
     demands it. */
  SVN_ERR_ASSERT(end_rev <= start_rev);
  SVN_ERR_ASSERT(start_rev <= peg_revision);

  /* Ensure that PATH is absolute, because our path-math will depend
     on that being the case.  */
  if (*path != '/')
    path = apr_pstrcat(pool, "/", path, (char *)NULL);

  /* Auth check. */
  if (authz_read_func)
    {
      svn_fs_root_t *peg_root;
      SVN_ERR(svn_fs_revision_root(&peg_root, fs, peg_revision, pool));
      SVN_ERR(check_readability(peg_root, path,
                                authz_read_func, authz_read_baton, pool));
    }

  /* Okay, let's get searching! */
  subpool = svn_pool_create(pool);
  current_rev = peg_revision;
  current_path = svn_stringbuf_create(path, pool);
  while (current_rev >= end_rev)
    {
      svn_revnum_t appeared_rev, prev_rev;
      const char *cur_path, *prev_path;
      svn_location_segment_t *segment;

      svn_pool_clear(subpool);

      cur_path = apr_pstrmemdup(subpool, current_path->data,
                                current_path->len);
      segment = apr_pcalloc(subpool, sizeof(*segment));
      segment->range_end = current_rev;
      segment->range_start = end_rev;
      /* segment path should be absolute without leading '/'. */
      segment->path = cur_path + 1;

      SVN_ERR(svn_repos__prev_location(&appeared_rev, &prev_path, &prev_rev,
                                       fs, current_rev, cur_path, subpool));

      /* If there are no previous locations for this thing (meaning,
         it originated at the current path), then we simply need to
         find its revision of origin to populate our final segment.
         Otherwise, the APPEARED_REV is the start of current segment's
         range. */
      if (! prev_path)
        {
          svn_fs_root_t *revroot;
          SVN_ERR(svn_fs_revision_root(&revroot, fs, current_rev, subpool));
          SVN_ERR(svn_fs_node_origin_rev(&(segment->range_start), revroot,
                                         cur_path, subpool));
          if (segment->range_start < end_rev)
            segment->range_start = end_rev;
          current_rev = SVN_INVALID_REVNUM;
        }
      else
        {
          segment->range_start = appeared_rev;
          svn_stringbuf_set(current_path, prev_path);
          current_rev = prev_rev;
        }

      /* Report our segment, providing it passes authz muster. */
      if (authz_read_func)
        {
          svn_boolean_t readable;
          svn_fs_root_t *cur_rev_root;

          /* authz_read_func requires path to have a leading slash. */
          const char *abs_path = apr_pstrcat(subpool, "/", segment->path,
                                             (char *)NULL);

          SVN_ERR(svn_fs_revision_root(&cur_rev_root, fs,
                                       segment->range_end, subpool));
          SVN_ERR(authz_read_func(&readable, cur_rev_root, abs_path,
                                  authz_read_baton, subpool));
          if (! readable)
            return SVN_NO_ERROR;
        }

      /* Transmit the segment (if it's within the scope of our concern). */
      SVN_ERR(maybe_crop_and_send_segment(segment, start_rev, end_rev,
                                          receiver, receiver_baton, subpool));

      /* If we've set CURRENT_REV to SVN_INVALID_REVNUM, we're done
         (and didn't ever reach END_REV).  */
      if (! SVN_IS_VALID_REVNUM(current_rev))
        break;

      /* If there's a gap in the history, we need to report as much
         (if the gap is within the scope of our concern). */
      if (segment->range_start - current_rev > 1)
        {
          svn_location_segment_t *gap_segment;
          gap_segment = apr_pcalloc(subpool, sizeof(*gap_segment));
          gap_segment->range_end = segment->range_start - 1;
          gap_segment->range_start = current_rev + 1;
          gap_segment->path = NULL;
          SVN_ERR(maybe_crop_and_send_segment(gap_segment, start_rev, end_rev,
                                              receiver, receiver_baton,
                                              subpool));
        }
    }
  svn_pool_destroy(subpool);
  return SVN_NO_ERROR;
}

/* Get the mergeinfo for PATH in REPOS at REVNUM and store it in MERGEINFO. */
static svn_error_t *
get_path_mergeinfo(apr_hash_t **mergeinfo,
                   svn_fs_t *fs,
                   const char *path,
                   svn_revnum_t revnum,
                   apr_pool_t *pool)
{
  svn_mergeinfo_catalog_t tmp_catalog;
  svn_fs_root_t *root;
  apr_pool_t *subpool = svn_pool_create(pool);
  apr_array_header_t *paths = apr_array_make(subpool, 1,
                                             sizeof(const char *));

  APR_ARRAY_PUSH(paths, const char *) = path;

  SVN_ERR(svn_fs_revision_root(&root, fs, revnum, subpool));
  /* We do not need to call svn_repos_fs_get_mergeinfo() (which performs authz)
     because we will filter out unreadable revisions in
     find_interesting_revision(), above */
  SVN_ERR(svn_fs_get_mergeinfo(&tmp_catalog, root, paths,
                               svn_mergeinfo_inherited, FALSE, subpool));

  *mergeinfo = apr_hash_get(tmp_catalog, path, APR_HASH_KEY_STRING);
  if (*mergeinfo)
    *mergeinfo = svn_mergeinfo_dup(*mergeinfo, pool);
  else
    *mergeinfo = apr_hash_make(pool);

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}

static APR_INLINE svn_boolean_t
is_path_in_hash(apr_hash_t *duplicate_path_revs,
                const char *path,
                svn_revnum_t revision,
                apr_pool_t *pool)
{
  const char *key = apr_psprintf(pool, "%s:%ld", path, revision);
  void *ptr;

  ptr = apr_hash_get(duplicate_path_revs, key, APR_HASH_KEY_STRING);
  return ptr != NULL;
}

struct path_revision
{
  svn_revnum_t revnum;
  const char *path;

  /* Does this path_rev have merges to also be included?  */
  apr_hash_t *merged_mergeinfo;

  /* Is this a merged revision? */
  svn_boolean_t merged;
};

/* Check for merges in OLD_PATH_REV->PATH at OLD_PATH_REV->REVNUM.  Store
   the mergeinfo difference in *MERGED_MERGEINFO, allocated in POOL.  The
   returned *MERGED_MERGEINFO will be NULL if there are no changes. */
static svn_error_t *
get_merged_mergeinfo(apr_hash_t **merged_mergeinfo,
                     svn_repos_t *repos,
                     struct path_revision *old_path_rev,
                     apr_pool_t *pool)
{
  apr_pool_t *subpool = svn_pool_create(pool);
  apr_hash_t *curr_mergeinfo, *prev_mergeinfo, *deleted, *changed;
  svn_error_t *err;
  svn_fs_root_t *root;
  apr_hash_t *changed_paths;
  const char *path = old_path_rev->path;

  /* Getting/parsing/diffing svn:mergeinfo is expensive, so only do it
     if there is a property change. */
  SVN_ERR(svn_fs_revision_root(&root, repos->fs, old_path_rev->revnum,
                               subpool));
  SVN_ERR(svn_fs_paths_changed2(&changed_paths, root, subpool));
  while (1)
    {
      svn_fs_path_change2_t *changed_path = apr_hash_get(changed_paths,
                                                         path,
                                                         APR_HASH_KEY_STRING);
      if (changed_path && changed_path->prop_mod)
        break;
      if (svn_fspath__is_root(path, strlen(path)))
        {
          svn_pool_destroy(subpool);
          *merged_mergeinfo = NULL;
          return SVN_NO_ERROR;
        }
      path = svn_fspath__dirname(path, subpool);
    }

  /* First, find the mergeinfo difference for old_path_rev->revnum, and
     old_path_rev->revnum - 1. */
  err = get_path_mergeinfo(&curr_mergeinfo, repos->fs, old_path_rev->path,
                           old_path_rev->revnum, subpool);
  if (err)
    {
      if (err->apr_err == SVN_ERR_MERGEINFO_PARSE_ERROR)
        {
          /* Issue #3896: If invalid mergeinfo is encountered the
             best we can do is ignore it and act is if there are
             no mergeinfo differences. */
          svn_error_clear(err);
          svn_pool_destroy(subpool);
          *merged_mergeinfo = NULL;
          return SVN_NO_ERROR;
        }
      else
        {
          return svn_error_trace(err);
        }
    }

  err = get_path_mergeinfo(&prev_mergeinfo, repos->fs, old_path_rev->path,
                           old_path_rev->revnum - 1, subpool);
  if (err && (err->apr_err == SVN_ERR_FS_NOT_FOUND
              || err->apr_err == SVN_ERR_MERGEINFO_PARSE_ERROR))
    {
      /* If the path doesn't exist in the previous revision or it does exist
         but has invalid mergeinfo (Issue #3896), assume no merges. */
      svn_error_clear(err);
      svn_pool_destroy(subpool);
      *merged_mergeinfo = NULL;
      return SVN_NO_ERROR;
    }
  else
    SVN_ERR(err);

  /* Then calculate and merge the differences. */
  SVN_ERR(svn_mergeinfo_diff(&deleted, &changed, prev_mergeinfo, curr_mergeinfo,
                             FALSE, subpool));
  SVN_ERR(svn_mergeinfo_merge(changed, deleted, subpool));

  /* Store the result. */
  if (apr_hash_count(changed))
    *merged_mergeinfo = svn_mergeinfo_dup(changed, pool);
  else
    *merged_mergeinfo = NULL;

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}

static svn_error_t *
find_interesting_revisions(apr_array_header_t *path_revisions,
                           svn_repos_t *repos,
                           const char *path,
                           svn_revnum_t start,
                           svn_revnum_t end,
                           svn_boolean_t include_merged_revisions,
                           svn_boolean_t mark_as_merged,
                           apr_hash_t *duplicate_path_revs,
                           svn_repos_authz_func_t authz_read_func,
                           void *authz_read_baton,
                           apr_pool_t *result_pool,
                           apr_pool_t *scratch_pool)
{
  apr_pool_t *iterpool, *last_pool;
  svn_fs_history_t *history;
  svn_fs_root_t *root;
  svn_node_kind_t kind;

  /* We switch between two pools while looping, since we need information from
     the last iteration to be available. */
  iterpool = svn_pool_create(scratch_pool);
  last_pool = svn_pool_create(scratch_pool);

  /* The path had better be a file in this revision. */
  SVN_ERR(svn_fs_revision_root(&root, repos->fs, end, scratch_pool));
  SVN_ERR(svn_fs_check_path(&kind, root, path, scratch_pool));
  if (kind != svn_node_file)
    return svn_error_createf
      (SVN_ERR_FS_NOT_FILE, NULL, _("'%s' is not a file in revision %ld"),
       path, end);

  /* Open a history object. */
  SVN_ERR(svn_fs_node_history(&history, root, path, scratch_pool));
  while (1)
    {
      struct path_revision *path_rev;
      svn_revnum_t tmp_revnum;
      const char *tmp_path;
      apr_pool_t *tmp_pool;

      svn_pool_clear(iterpool);

      /* Fetch the history object to walk through. */
      SVN_ERR(svn_fs_history_prev(&history, history, TRUE, iterpool));
      if (!history)
        break;
      SVN_ERR(svn_fs_history_location(&tmp_path, &tmp_revnum,
                                      history, iterpool));

      /* Check to see if we already saw this path (and it's ancestors) */
      if (include_merged_revisions
          && is_path_in_hash(duplicate_path_revs, tmp_path,
                             tmp_revnum, iterpool))
         break;

      /* Check authorization. */
      if (authz_read_func)
        {
          svn_boolean_t readable;
          svn_fs_root_t *tmp_root;

          SVN_ERR(svn_fs_revision_root(&tmp_root, repos->fs, tmp_revnum,
                                       iterpool));
          SVN_ERR(authz_read_func(&readable, tmp_root, tmp_path,
                                  authz_read_baton, iterpool));
          if (! readable)
            break;
        }

      /* We didn't break, so we must really want this path-rev. */
      path_rev = apr_palloc(result_pool, sizeof(*path_rev));
      path_rev->path = apr_pstrdup(result_pool, tmp_path);
      path_rev->revnum = tmp_revnum;
      path_rev->merged = mark_as_merged;
      APR_ARRAY_PUSH(path_revisions, struct path_revision *) = path_rev;

      if (include_merged_revisions)
        SVN_ERR(get_merged_mergeinfo(&path_rev->merged_mergeinfo, repos,
                                     path_rev, result_pool));
      else
        path_rev->merged_mergeinfo = NULL;

      /* Add the path/rev pair to the hash, so we can filter out future
         occurrences of it.  We only care about this if including merged
         revisions, 'cause that's the only time we can have duplicates. */
      apr_hash_set(duplicate_path_revs,
                   apr_psprintf(result_pool, "%s:%ld", path_rev->path,
                                path_rev->revnum),
                   APR_HASH_KEY_STRING, (void *)0xdeadbeef);

      if (path_rev->revnum <= start)
        break;

      /* Swap pools. */
      tmp_pool = iterpool;
      iterpool = last_pool;
      last_pool = tmp_pool;
    }

  svn_pool_destroy(iterpool);
  svn_pool_destroy(last_pool);

  return SVN_NO_ERROR;
}

/* Comparison function to sort path/revisions in increasing order */
static int
compare_path_revisions(const void *a, const void *b)
{
  struct path_revision *a_pr = *(struct path_revision *const *)a;
  struct path_revision *b_pr = *(struct path_revision *const *)b;

  if (a_pr->revnum == b_pr->revnum)
    return 0;

  return a_pr->revnum < b_pr->revnum ? 1 : -1;
}

static svn_error_t *
find_merged_revisions(apr_array_header_t **merged_path_revisions_out,
                      svn_revnum_t start,
                      const apr_array_header_t *mainline_path_revisions,
                      svn_repos_t *repos,
                      apr_hash_t *duplicate_path_revs,
                      svn_repos_authz_func_t authz_read_func,
                      void *authz_read_baton,
                      apr_pool_t *pool)
{
  const apr_array_header_t *old;
  apr_array_header_t *new_merged_path_revs;
  apr_pool_t *iterpool, *last_pool;
  apr_array_header_t *merged_path_revisions =
    apr_array_make(pool, 0, sizeof(struct path_revision *));

  old = mainline_path_revisions;
  iterpool = svn_pool_create(pool);
  last_pool = svn_pool_create(pool);

  do
    {
      int i;
      apr_pool_t *temp_pool;

      svn_pool_clear(iterpool);
      new_merged_path_revs = apr_array_make(iterpool, 0,
                                            sizeof(struct path_revision *));

      /* Iterate over OLD, checking for non-empty mergeinfo.  If found, gather
         path_revisions for any merged revisions, and store those in NEW. */
      for (i = 0; i < old->nelts; i++)
        {
          apr_pool_t *iterpool2;
          apr_hash_index_t *hi;
          struct path_revision *old_pr = APR_ARRAY_IDX(old, i,
                                                       struct path_revision *);
          if (!old_pr->merged_mergeinfo)
            continue;

          iterpool2 = svn_pool_create(iterpool);

          /* Determine and trace the merge sources. */
          for (hi = apr_hash_first(iterpool, old_pr->merged_mergeinfo); hi;
               hi = apr_hash_next(hi))
            {
              apr_pool_t *iterpool3;
              apr_array_header_t *rangelist;
              const char *path;
              int j;

              svn_pool_clear(iterpool2);
              iterpool3 = svn_pool_create(iterpool2);

              apr_hash_this(hi, (void *) &path, NULL, (void *) &rangelist);

              for (j = 0; j < rangelist->nelts; j++)
                {
                  svn_merge_range_t *range = APR_ARRAY_IDX(rangelist, j,
                                                           svn_merge_range_t *);
                  svn_node_kind_t kind;
                  svn_fs_root_t *root;

                  if (range->end < start)
                    continue;

                  svn_pool_clear(iterpool3);

                  SVN_ERR(svn_fs_revision_root(&root, repos->fs, range->end,
                                               iterpool3));
                  SVN_ERR(svn_fs_check_path(&kind, root, path, iterpool3));
                  if (kind != svn_node_file)
                    continue;

                  /* Search and find revisions to add to the NEW list. */
                  SVN_ERR(find_interesting_revisions(new_merged_path_revs,
                                                     repos, path,
                                                     range->start, range->end,
                                                     TRUE, TRUE,
                                                     duplicate_path_revs,
                                                     authz_read_func,
                                                     authz_read_baton, pool,
                                                     iterpool3));
                }
              svn_pool_destroy(iterpool3);
            }
          svn_pool_destroy(iterpool2);
        }

      /* Append the newly found path revisions with the old ones. */
      merged_path_revisions = apr_array_append(iterpool, merged_path_revisions,
                                               new_merged_path_revs);

      /* Swap data structures */
      old = new_merged_path_revs;
      temp_pool = last_pool;
      last_pool = iterpool;
      iterpool = temp_pool;
    }
  while (new_merged_path_revs->nelts > 0);

  /* Sort MERGED_PATH_REVISIONS in increasing order by REVNUM. */
  qsort(merged_path_revisions->elts, merged_path_revisions->nelts,
        sizeof(struct path_revision *), compare_path_revisions);

  /* Copy to the output array. */
  *merged_path_revisions_out = apr_array_copy(pool, merged_path_revisions);

  svn_pool_destroy(iterpool);
  svn_pool_destroy(last_pool);

  return SVN_NO_ERROR;
}

struct send_baton
{
  apr_pool_t *iterpool;
  apr_pool_t *last_pool;
  apr_hash_t *last_props;
  const char *last_path;
  svn_fs_root_t *last_root;
};

/* Send PATH_REV to HANDLER and HANDLER_BATON, using information provided by
   SB. */
static svn_error_t *
send_path_revision(struct path_revision *path_rev,
                   svn_repos_t *repos,
                   struct send_baton *sb,
                   svn_file_rev_handler_t handler,
                   void *handler_baton)
{
  apr_hash_t *rev_props;
  apr_hash_t *props;
  apr_array_header_t *prop_diffs;
  svn_fs_root_t *root;
  svn_txdelta_stream_t *delta_stream;
  svn_txdelta_window_handler_t delta_handler = NULL;
  void *delta_baton = NULL;
  apr_pool_t *tmp_pool;  /* For swapping */
  svn_boolean_t contents_changed;

  svn_pool_clear(sb->iterpool);

  /* Get the revision properties. */
  SVN_ERR(svn_fs_revision_proplist(&rev_props, repos->fs,
                                   path_rev->revnum, sb->iterpool));

  /* Open the revision root. */
  SVN_ERR(svn_fs_revision_root(&root, repos->fs, path_rev->revnum,
                               sb->iterpool));

  /* Get the file's properties for this revision and compute the diffs. */
  SVN_ERR(svn_fs_node_proplist(&props, root, path_rev->path,
                                   sb->iterpool));
  SVN_ERR(svn_prop_diffs(&prop_diffs, props, sb->last_props,
                         sb->iterpool));

  /* Check if the contents changed. */
  /* Special case: In the first revision, we always provide a delta. */
  if (sb->last_root)
    SVN_ERR(svn_fs_contents_changed(&contents_changed, sb->last_root,
                                    sb->last_path, root, path_rev->path,
                                    sb->iterpool));
  else
    contents_changed = TRUE;

  /* We have all we need, give to the handler. */
  SVN_ERR(handler(handler_baton, path_rev->path, path_rev->revnum,
                  rev_props, path_rev->merged,
                  contents_changed ? &delta_handler : NULL,
                  contents_changed ? &delta_baton : NULL,
                  prop_diffs, sb->iterpool));

  /* Compute and send delta if client asked for it.
     Note that this was initialized to NULL, so if !contents_changed,
     no deltas will be computed. */
  if (delta_handler)
    {
      /* Get the content delta. */
      SVN_ERR(svn_fs_get_file_delta_stream(&delta_stream,
                                           sb->last_root, sb->last_path,
                                           root, path_rev->path,
                                           sb->iterpool));
      /* And send. */
      SVN_ERR(svn_txdelta_send_txstream(delta_stream,
                                        delta_handler, delta_baton,
                                        sb->iterpool));
    }

  /* Remember root, path and props for next iteration. */
  sb->last_root = root;
  sb->last_path = path_rev->path;
  sb->last_props = props;

  /* Swap the pools. */
  tmp_pool = sb->iterpool;
  sb->iterpool = sb->last_pool;
  sb->last_pool = tmp_pool;

  return SVN_NO_ERROR;
}

/* We don't yet support sending revisions in reverse order; the caller wait
 * until we've traced back through the entire history, and then accept
 * them from oldest to youngest.  Someday this may change, but in the meantime,
 * the general algorithm is thus:
 *
 *  1) Trace back through the history of an object, adding each revision
 *     found to the MAINLINE_PATH_REVISIONS array, marking any which were
 *     merges.
 *  2) If INCLUDE_MERGED_REVISIONS is TRUE, we repeat Step 1 on each of the
 *     merged revisions, including them in the MERGED_PATH_REVISIONS, and using
 *     DUPLICATE_PATH_REVS to avoid tracing the same paths of history multiple
 *     times.
 *  3) Send both MAINLINE_PATH_REVISIONS and MERGED_PATH_REVISIONS from
 *     oldest to youngest, interleaving as appropriate.  This is implemented
 *     similar to an insertion sort, but instead of inserting into another
 *     array, we just call the appropriate handler.
 */
svn_error_t *
svn_repos_get_file_revs2(svn_repos_t *repos,
                         const char *path,
                         svn_revnum_t start,
                         svn_revnum_t end,
                         svn_boolean_t include_merged_revisions,
                         svn_repos_authz_func_t authz_read_func,
                         void *authz_read_baton,
                         svn_file_rev_handler_t handler,
                         void *handler_baton,
                         apr_pool_t *pool)
{
  apr_array_header_t *mainline_path_revisions, *merged_path_revisions;
  apr_hash_t *duplicate_path_revs;
  struct send_baton sb;
  int mainline_pos, merged_pos;

  /* Get the revisions we are interested in. */
  duplicate_path_revs = apr_hash_make(pool);
  mainline_path_revisions = apr_array_make(pool, 0,
                                           sizeof(struct path_revision *));
  SVN_ERR(find_interesting_revisions(mainline_path_revisions, repos, path,
                                     start, end, include_merged_revisions,
                                     FALSE, duplicate_path_revs,
                                     authz_read_func, authz_read_baton, pool,
                                     pool));

  /* If we are including merged revisions, go get those, too. */
  if (include_merged_revisions)
    SVN_ERR(find_merged_revisions(&merged_path_revisions, start,
                                  mainline_path_revisions, repos,
                                  duplicate_path_revs, authz_read_func,
                                  authz_read_baton, pool));
  else
    merged_path_revisions = apr_array_make(pool, 0,
                                           sizeof(struct path_revision *));

  /* We must have at least one revision to get. */
  SVN_ERR_ASSERT(mainline_path_revisions->nelts > 0);

  /* We switch betwwen two pools while looping, since we need information from
     the last iteration to be available. */
  sb.iterpool = svn_pool_create(pool);
  sb.last_pool = svn_pool_create(pool);

  /* We want the first txdelta to be against the empty file. */
  sb.last_root = NULL;
  sb.last_path = NULL;

  /* Create an empty hash table for the first property diff. */
  sb.last_props = apr_hash_make(sb.last_pool);

  /* Walk through both mainline and merged revisions, and send them in
     reverse chronological order, interleaving as appropriate. */
  mainline_pos = mainline_path_revisions->nelts - 1;
  merged_pos = merged_path_revisions->nelts - 1;
  while (mainline_pos >= 0 && merged_pos >= 0)
    {
      struct path_revision *main_pr = APR_ARRAY_IDX(mainline_path_revisions,
                                                    mainline_pos,
                                                    struct path_revision *);
      struct path_revision *merged_pr = APR_ARRAY_IDX(merged_path_revisions,
                                                      merged_pos,
                                                      struct path_revision *);

      if (main_pr->revnum <= merged_pr->revnum)
        {
          SVN_ERR(send_path_revision(main_pr, repos, &sb, handler,
                                     handler_baton));
          mainline_pos -= 1;
        }
      else
        {
          SVN_ERR(send_path_revision(merged_pr, repos, &sb, handler,
                                     handler_baton));
          merged_pos -= 1;
        }
    }

  /* Send any remaining revisions from the mainline list. */
  for (; mainline_pos >= 0; mainline_pos -= 1)
    {
      struct path_revision *main_pr = APR_ARRAY_IDX(mainline_path_revisions,
                                                    mainline_pos,
                                                    struct path_revision *);
      SVN_ERR(send_path_revision(main_pr, repos, &sb, handler, handler_baton));
    }

  /* Ditto for the merged list. */
  for (; merged_pos >= 0; merged_pos -= 1)
    {
      struct path_revision *merged_pr = APR_ARRAY_IDX(merged_path_revisions,
                                                      merged_pos,
                                                      struct path_revision *);
      SVN_ERR(send_path_revision(merged_pr, repos, &sb, handler,
                                 handler_baton));
    }

  svn_pool_destroy(sb.last_pool);
  svn_pool_destroy(sb.iterpool);

  return SVN_NO_ERROR;
}

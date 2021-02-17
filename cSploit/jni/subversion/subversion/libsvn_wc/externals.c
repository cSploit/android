/*
 * externals.c :  routines dealing with (file) externals in the working copy
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



#include <stdlib.h>
#include <string.h>

#include <apr_pools.h>
#include <apr_hash.h>
#include <apr_tables.h>
#include <apr_general.h>

#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_error.h"
#include "svn_hash.h"
#include "svn_io.h"
#include "svn_pools.h"
#include "svn_props.h"
#include "svn_string.h"
#include "svn_time.h"
#include "svn_types.h"
#include "svn_wc.h"

#include "private/svn_skel.h"

#include "wc.h"
#include "adm_files.h"
#include "props.h"
#include "translate.h"
#include "workqueue.h"
#include "conflicts.h"

#include "svn_private_config.h"

/** Externals **/

/*
 * Look for either
 *
 *   -r N
 *   -rN
 *
 * in the LINE_PARTS array and update the revision field in ITEM with
 * the revision if the revision is found.  Set REV_IDX to the index in
 * LINE_PARTS where the revision specification starts.  Remove from
 * LINE_PARTS the element(s) that specify the revision.
 * PARENT_DIRECTORY_DISPLAY and LINE are given to return a nice error
 * string.
 *
 * If this function returns successfully, then LINE_PARTS will have
 * only two elements in it.
 */
static svn_error_t *
find_and_remove_externals_revision(int *rev_idx,
                                   const char **line_parts,
                                   int num_line_parts,
                                   svn_wc_external_item2_t *item,
                                   const char *parent_directory_display,
                                   const char *line,
                                   apr_pool_t *pool)
{
  int i;

  for (i = 0; i < 2; ++i)
    {
      const char *token = line_parts[i];

      if (token[0] == '-' && token[1] == 'r')
        {
          svn_opt_revision_t end_revision = { svn_opt_revision_unspecified };
          const char *digits_ptr;
          int shift_count;
          int j;

          *rev_idx = i;

          if (token[2] == '\0')
            {
              /* There must be a total of four elements in the line if
                 -r N is used. */
              if (num_line_parts != 4)
                goto parse_error;

              shift_count = 2;
              digits_ptr = line_parts[i+1];
            }
          else
            {
              /* There must be a total of three elements in the line
                 if -rN is used. */
              if (num_line_parts != 3)
                goto parse_error;

              shift_count = 1;
              digits_ptr = token+2;
            }

          if (svn_opt_parse_revision(&item->revision,
                                     &end_revision,
                                     digits_ptr, pool) != 0)
            goto parse_error;
          /* We want a single revision, not a range. */
          if (end_revision.kind != svn_opt_revision_unspecified)
            goto parse_error;
          /* Allow only numbers and dates, not keywords. */
          if (item->revision.kind != svn_opt_revision_number
              && item->revision.kind != svn_opt_revision_date)
            goto parse_error;

          /* Shift any line elements past the revision specification
             down over the revision specification. */
          for (j = i; j < num_line_parts-shift_count; ++j)
            line_parts[j] = line_parts[j+shift_count];
          line_parts[num_line_parts-shift_count] = NULL;

          /* Found the revision, so leave the function immediately, do
           * not continue looking for additional revisions. */
          return SVN_NO_ERROR;
        }
    }

  /* No revision was found, so there must be exactly two items in the
     line array. */
  if (num_line_parts == 2)
    return SVN_NO_ERROR;

 parse_error:
  return svn_error_createf
    (SVN_ERR_CLIENT_INVALID_EXTERNALS_DESCRIPTION, NULL,
     _("Error parsing %s property on '%s': '%s'"),
     SVN_PROP_EXTERNALS,
     parent_directory_display,
     line);
}

svn_error_t *
svn_wc_parse_externals_description3(apr_array_header_t **externals_p,
                                    const char *parent_directory,
                                    const char *desc,
                                    svn_boolean_t canonicalize_url,
                                    apr_pool_t *pool)
{
  apr_array_header_t *lines = svn_cstring_split(desc, "\n\r", TRUE, pool);
  int i;
  const char *parent_directory_display = svn_path_is_url(parent_directory) ?
    parent_directory : svn_dirent_local_style(parent_directory, pool);

  if (externals_p)
    *externals_p = apr_array_make(pool, 1, sizeof(svn_wc_external_item2_t *));

  for (i = 0; i < lines->nelts; i++)
    {
      const char *line = APR_ARRAY_IDX(lines, i, const char *);
      apr_status_t status;
      char **line_parts;
      int num_line_parts;
      svn_wc_external_item2_t *item;
      const char *token0;
      const char *token1;
      svn_boolean_t token0_is_url;
      svn_boolean_t token1_is_url;

      /* Index into line_parts where the revision specification
         started. */
      int rev_idx = -1;

      if ((! line) || (line[0] == '#'))
        continue;

      /* else proceed */

      status = apr_tokenize_to_argv(line, &line_parts, pool);
      if (status)
        return svn_error_wrap_apr(status,
                                  _("Can't split line into components: '%s'"),
                                  line);
      /* Count the number of tokens. */
      for (num_line_parts = 0; line_parts[num_line_parts]; num_line_parts++)
        ;

      SVN_ERR(svn_wc_external_item_create
              ((const svn_wc_external_item2_t **) &item, pool));
      item->revision.kind = svn_opt_revision_unspecified;
      item->peg_revision.kind = svn_opt_revision_unspecified;

      /*
       * There are six different formats of externals:
       *
       * 1) DIR URL
       * 2) DIR -r N URL
       * 3) DIR -rN  URL
       * 4) URL DIR
       * 5) -r N URL DIR
       * 6) -rN URL DIR
       *
       * The last three allow peg revisions in the URL.
       *
       * With relative URLs and no '-rN' or '-r N', there is no way to
       * distinguish between 'DIR URL' and 'URL DIR' when URL is a
       * relative URL like /svn/repos/trunk, so this case is taken as
       * case 4).
       */
      if (num_line_parts < 2 || num_line_parts > 4)
        return svn_error_createf
          (SVN_ERR_CLIENT_INVALID_EXTERNALS_DESCRIPTION, NULL,
           _("Error parsing %s property on '%s': '%s'"),
           SVN_PROP_EXTERNALS,
           parent_directory_display,
           line);

      /* To make it easy to check for the forms, find and remove -r N
         or -rN from the line item array.  If it is found, rev_idx
         contains the index into line_parts where '-r' was found and
         set item->revision to the parsed revision. */
      /* ### ugh. stupid cast. */
      SVN_ERR(find_and_remove_externals_revision(&rev_idx,
                                                 (const char **)line_parts,
                                                 num_line_parts, item,
                                                 parent_directory_display,
                                                 line, pool));

      token0 = line_parts[0];
      token1 = line_parts[1];

      token0_is_url = svn_path_is_url(token0);
      token1_is_url = svn_path_is_url(token1);

      if (token0_is_url && token1_is_url)
        return svn_error_createf
          (SVN_ERR_CLIENT_INVALID_EXTERNALS_DESCRIPTION, NULL,
           _("Invalid %s property on '%s': "
             "cannot use two absolute URLs ('%s' and '%s') in an external; "
             "one must be a path where an absolute or relative URL is "
             "checked out to"),
           SVN_PROP_EXTERNALS, parent_directory_display, token0, token1);

      if (0 == rev_idx && token1_is_url)
        return svn_error_createf
          (SVN_ERR_CLIENT_INVALID_EXTERNALS_DESCRIPTION, NULL,
           _("Invalid %s property on '%s': "
             "cannot use a URL '%s' as the target directory for an external "
             "definition"),
           SVN_PROP_EXTERNALS, parent_directory_display, token1);

      if (1 == rev_idx && token0_is_url)
        return svn_error_createf
          (SVN_ERR_CLIENT_INVALID_EXTERNALS_DESCRIPTION, NULL,
           _("Invalid %s property on '%s': "
             "cannot use a URL '%s' as the target directory for an external "
             "definition"),
           SVN_PROP_EXTERNALS, parent_directory_display, token0);

      /* The appearence of -r N or -rN forces the type of external.
         If -r is at the beginning of the line or the first token is
         an absolute URL or if the second token is not an absolute
         URL, then the URL supports peg revisions. */
      if (0 == rev_idx ||
          (-1 == rev_idx && (token0_is_url || ! token1_is_url)))
        {
          /* The URL is passed to svn_opt_parse_path in
             uncanonicalized form so that the scheme relative URL
             //hostname/foo is not collapsed to a server root relative
             URL /hostname/foo. */
          SVN_ERR(svn_opt_parse_path(&item->peg_revision, &item->url,
                                     token0, pool));
          item->target_dir = token1;
        }
      else
        {
          item->target_dir = token0;
          item->url = token1;
          item->peg_revision = item->revision;
        }

      SVN_ERR(svn_opt_resolve_revisions(&item->peg_revision,
                                        &item->revision, TRUE, FALSE,
                                        pool));

      item->target_dir = svn_dirent_internal_style(item->target_dir, pool);

      if (item->target_dir[0] == '\0'
          || svn_dirent_is_absolute(item->target_dir)
          || svn_path_is_backpath_present(item->target_dir)
          || !svn_dirent_skip_ancestor("dummy",
                                       svn_dirent_join("dummy",
                                                       item->target_dir,
                                                       pool)))
        return svn_error_createf
          (SVN_ERR_CLIENT_INVALID_EXTERNALS_DESCRIPTION, NULL,
           _("Invalid %s property on '%s': "
             "target '%s' is an absolute path or involves '..'"),
           SVN_PROP_EXTERNALS,
           parent_directory_display,
           item->target_dir);

      if (canonicalize_url)
        {
          /* Uh... this is stupid.  But it's consistent with what our
             code did before we split up the relpath/dirent/uri APIs.
             Still, given this, it's no wonder that our own libraries
             don't ask this function to canonicalize the results.  */
          if (svn_path_is_url(item->url))
            item->url = svn_uri_canonicalize(item->url, pool);
          else
            item->url = svn_dirent_canonicalize(item->url, pool);
        }

      if (externals_p)
        APR_ARRAY_PUSH(*externals_p, svn_wc_external_item2_t *) = item;
    }

  return SVN_NO_ERROR;
}


struct edit_baton
{
  apr_pool_t *pool;
  svn_wc__db_t *db;

  /* We explicitly use wri_abspath and local_abspath here, because we
     might want to install file externals in an obstructing working copy */
  const char *wri_abspath;     /* The working defining the file external */
  const char *local_abspath;   /* The file external itself */
  const char *name;            /* The basename of the file external itself */

  /* Information from the caller */
  svn_boolean_t use_commit_times;
  const apr_array_header_t *ext_patterns;
  const char *diff3cmd;

  const char *url;
  const char *repos_root_url;
  const char *repos_uuid;

  const char *record_ancestor_abspath;
  const char *recorded_repos_relpath;
  svn_revnum_t recorded_peg_revision;
  svn_revnum_t recorded_revision;

  svn_wc_conflict_resolver_func2_t conflict_func;
  void *conflict_baton;
  svn_cancel_func_t cancel_func;
  void *cancel_baton;
  svn_wc_notify_func2_t notify_func;
  void *notify_baton;

  svn_revnum_t *target_revision;

  /* What was there before the update */
  svn_revnum_t original_revision;
  const svn_checksum_t *original_checksum;

  /* What we are installing now */
  const char *new_pristine_abspath;
  svn_checksum_t *new_sha1_checksum;
  svn_checksum_t *new_md5_checksum;

  /* List of incoming propchanges */
  apr_array_header_t *propchanges;

  /* The last change information */
  svn_revnum_t changed_rev;
  apr_time_t changed_date;
  const char *changed_author;

  svn_boolean_t had_props;

  svn_boolean_t file_closed;
};

/* svn_delta_editor_t function for svn_wc__get_file_external_editor */
static svn_error_t *
set_target_revision(void *edit_baton,
                     svn_revnum_t target_revision,
                     apr_pool_t *pool)
{
  struct edit_baton *eb = edit_baton;

  *eb->target_revision = target_revision;

  return SVN_NO_ERROR;
}

/* svn_delta_editor_t function for svn_wc__get_file_external_editor */
static svn_error_t *
open_root(void *edit_baton,
          svn_revnum_t base_revision,
          apr_pool_t *dir_pool,
          void **root_baton)
{
  *root_baton = edit_baton;
  return SVN_NO_ERROR;
}

/* svn_delta_editor_t function for svn_wc__get_file_external_editor */
static svn_error_t *
add_file(const char *path,
         void *parent_baton,
         const char *copyfrom_path,
         svn_revnum_t copyfrom_revision,
         apr_pool_t *file_pool,
         void **file_baton)
{
  struct edit_baton *eb = parent_baton;
  if (strcmp(path, eb->name))
      return svn_error_createf(SVN_ERR_WC_PATH_NOT_FOUND, NULL,
                               _("This editor can only update '%s'"),
                               svn_dirent_local_style(eb->local_abspath,
                                                      file_pool));

  *file_baton = eb;
  eb->original_revision = SVN_INVALID_REVNUM;

  return SVN_NO_ERROR;
}

/* svn_delta_editor_t function for svn_wc__get_file_external_editor */
static svn_error_t *
open_file(const char *path,
          void *parent_baton,
          svn_revnum_t base_revision,
          apr_pool_t *file_pool,
          void **file_baton)
{
  struct edit_baton *eb = parent_baton;
  svn_wc__db_kind_t kind;
  if (strcmp(path, eb->name))
      return svn_error_createf(SVN_ERR_WC_PATH_NOT_FOUND, NULL,
                               _("This editor can only update '%s'"),
                               svn_dirent_local_style(eb->local_abspath,
                                                      file_pool));

  *file_baton = eb;
  SVN_ERR(svn_wc__db_base_get_info(NULL, &kind, &eb->original_revision,
                                   NULL, NULL, NULL, &eb->changed_rev,
                                   &eb->changed_date, &eb->changed_author,
                                   NULL, &eb->original_checksum, NULL, NULL,
                                   &eb->had_props, NULL,
                                   eb->db, eb->local_abspath,
                                   eb->pool, file_pool));

  if (kind != svn_wc__db_kind_file)
    return svn_error_createf(SVN_ERR_WC_PATH_UNEXPECTED_STATUS, NULL,
                               _("Node '%s' is no existing file external"),
                               svn_dirent_local_style(eb->local_abspath,
                                                      file_pool));
  return SVN_NO_ERROR;
}

/* svn_delta_editor_t function for svn_wc__get_file_external_editor */
static svn_error_t *
apply_textdelta(void *file_baton,
                const char *base_checksum_digest,
                apr_pool_t *pool,
                svn_txdelta_window_handler_t *handler,
                void **handler_baton)
{
  struct edit_baton *eb = file_baton;
  svn_stream_t *src_stream;
  svn_stream_t *dest_stream;

  if (eb->original_checksum)
    {
      if (base_checksum_digest)
        {
          svn_checksum_t *expected_checksum;
          const svn_checksum_t *original_md5;

          SVN_ERR(svn_checksum_parse_hex(&expected_checksum, svn_checksum_md5,
                                         base_checksum_digest, pool));

          if (eb->original_checksum->kind != svn_checksum_md5)
            SVN_ERR(svn_wc__db_pristine_get_md5(&original_md5,
                                                eb->db, eb->wri_abspath,
                                                eb->original_checksum,
                                                pool, pool));
          else
            original_md5 = eb->original_checksum;

          if (!svn_checksum_match(expected_checksum, original_md5))
            return svn_error_trace(svn_checksum_mismatch_err(
                                    expected_checksum,
                                    original_md5,
                                    pool,
                                    _("Base checksum mismatch for '%s'"),
                                    svn_dirent_local_style(eb->local_abspath,
                                                           pool)));
        }

      SVN_ERR(svn_wc__db_pristine_read(&src_stream, NULL, eb->db,
                                       eb->wri_abspath, eb->original_checksum,
                                       pool, pool));
    }
  else
    src_stream = svn_stream_empty(pool);

  SVN_ERR(svn_wc__open_writable_base(&dest_stream, &eb->new_pristine_abspath,
                                     &eb->new_md5_checksum,
                                     &eb->new_sha1_checksum,
                                     eb->db, eb->wri_abspath,
                                     eb->pool, pool));

  svn_txdelta_apply(src_stream, dest_stream, NULL, eb->local_abspath, pool,
                    handler, handler_baton);

  return SVN_NO_ERROR;
}

/* svn_delta_editor_t function for svn_wc__get_file_external_editor */
static svn_error_t *
change_file_prop(void *file_baton,
                 const char *name,
                 const svn_string_t *value,
                 apr_pool_t *pool)
{
  struct edit_baton *eb = file_baton;
  svn_prop_t *propchange;

  propchange = apr_array_push(eb->propchanges);
  propchange->name = apr_pstrdup(eb->pool, name);
  propchange->value = value ? svn_string_dup(value, eb->pool) : NULL;

  return SVN_NO_ERROR;
}

/* svn_delta_editor_t function for svn_wc__get_file_external_editor */
static svn_error_t *
close_file(void *file_baton,
           const char *expected_md5_digest,
           apr_pool_t *pool)
{
  struct edit_baton *eb = file_baton;
  svn_wc_notify_state_t prop_state = svn_wc_notify_state_unknown;
  svn_wc_notify_state_t content_state = svn_wc_notify_state_unknown;
  svn_boolean_t obstructed = FALSE;

  eb->file_closed = TRUE; /* We bump the revision here */

  if (expected_md5_digest)
    {
      svn_checksum_t *expected_md5_checksum;
      const svn_checksum_t *actual_md5_checksum = eb->new_md5_checksum;

      SVN_ERR(svn_checksum_parse_hex(&expected_md5_checksum, svn_checksum_md5,
                                     expected_md5_digest, pool));

      if (actual_md5_checksum == NULL)
        {
          actual_md5_checksum = eb->original_checksum;

          if (actual_md5_checksum != NULL
              && actual_md5_checksum->kind != svn_checksum_md5)
            {
              SVN_ERR(svn_wc__db_pristine_get_md5(&actual_md5_checksum,
                                                  eb->db, eb->wri_abspath,
                                                  actual_md5_checksum,
                                                  pool, pool));
            }
        }

      if (! svn_checksum_match(expected_md5_checksum, actual_md5_checksum))
        return svn_checksum_mismatch_err(
                        expected_md5_checksum,
                        actual_md5_checksum, pool,
                        _("Checksum mismatch for '%s'"),
                        svn_dirent_local_style(eb->local_abspath, pool));
    }

  /* First move the file in the pristine store; this hands over the cleanup
     behavior to the pristine store. */
  if (eb->new_sha1_checksum)
    {
      SVN_ERR(svn_wc__db_pristine_install(eb->db, eb->new_pristine_abspath,
                                          eb->new_sha1_checksum,
                                          eb->new_md5_checksum, pool));

      eb->new_pristine_abspath = NULL;
    }

  /* ### TODO: Merge the changes */

  {
    svn_skel_t *all_work_items = NULL;
    svn_skel_t *work_item;
    apr_hash_t *base_props = NULL;
    apr_hash_t *actual_props = NULL;
    apr_hash_t *new_pristine_props = NULL;
    apr_hash_t *new_actual_props = NULL;
    apr_hash_t *new_dav_props = NULL;
    const svn_checksum_t *new_checksum = NULL;
    const svn_checksum_t *original_checksum = NULL;

    svn_boolean_t added = !SVN_IS_VALID_REVNUM(eb->original_revision);
    const char *repos_relpath = svn_uri__is_child(eb->repos_root_url,
                                                  eb->url, pool);

    if (! added)
      {
        new_checksum = eb->original_checksum;

        if (eb->had_props)
          SVN_ERR(svn_wc__db_base_get_props(&base_props, eb->db,
                                            eb->local_abspath,
                                            pool, pool));

        SVN_ERR(svn_wc__db_read_props(&actual_props, eb->db,
                                      eb->local_abspath, pool, pool));
      }

    if (!base_props)
      base_props = apr_hash_make(pool);

    if (!actual_props)
      actual_props = apr_hash_make(pool);

    if (eb->new_sha1_checksum)
      new_checksum = eb->new_sha1_checksum;

    {
      apr_array_header_t *entry_prop_changes;
      apr_array_header_t *dav_prop_changes;
      apr_array_header_t *regular_prop_changes;
      int i;

      SVN_ERR(svn_categorize_props(eb->propchanges, &entry_prop_changes,
                                   &dav_prop_changes, &regular_prop_changes,
                                   pool));

      for (i = 0; i < entry_prop_changes->nelts; i++)
        {
          const svn_prop_t *prop = &APR_ARRAY_IDX(entry_prop_changes, i,
                                                  svn_prop_t);

          if (! prop->value)
            continue; /* authz or something */

          if (! strcmp(prop->name, SVN_PROP_ENTRY_LAST_AUTHOR))
            eb->changed_author = apr_pstrdup(pool, prop->value->data);
          else if (! strcmp(prop->name, SVN_PROP_ENTRY_COMMITTED_REV))
            {
              apr_int64_t rev;
              SVN_ERR(svn_cstring_atoi64(&rev, prop->value->data));
              eb->changed_rev = (svn_revnum_t)rev;
            }
          else if (! strcmp(prop->name, SVN_PROP_ENTRY_COMMITTED_DATE))
            SVN_ERR(svn_time_from_cstring(&eb->changed_date, prop->value->data,
                                          pool));
        }

      if (dav_prop_changes->nelts > 0)
        new_dav_props = svn_prop_array_to_hash(dav_prop_changes, pool);

      if (regular_prop_changes->nelts > 0)
        {
          SVN_ERR(svn_wc__merge_props(&work_item, &prop_state,
                                      &new_pristine_props,
                                      &new_actual_props,
                                      eb->db, eb->local_abspath,
                                      svn_wc__db_kind_file,
                                      NULL, NULL,
                                      NULL /* server_baseprops*/,
                                      base_props,
                                      actual_props,
                                      regular_prop_changes,
                                      TRUE /* base_merge */,
                                      FALSE /* dry_run */,
                                      eb->conflict_func,
                                      eb->conflict_baton,
                                      eb->cancel_func, eb->cancel_baton,
                                      pool, pool));

          if (work_item)
            all_work_items = svn_wc__wq_merge(all_work_items, work_item,
                                              pool);
        }
      else
        {
          new_pristine_props = base_props;
          new_actual_props = actual_props;
        }
    }

    if (eb->new_sha1_checksum)
      {
        svn_node_kind_t disk_kind;
        svn_boolean_t install_pristine = FALSE;
        const char *install_from = NULL;

        SVN_ERR(svn_io_check_path(eb->local_abspath, &disk_kind, pool));

        if (disk_kind == svn_node_none)
          {
            /* Just install the file */
            install_pristine = TRUE;
            content_state = svn_wc_notify_state_changed;
          }
        else if (disk_kind != svn_node_file)
          {
            /* The node is obstructed; we just change the DB */
            obstructed = TRUE;
            content_state = svn_wc_notify_state_unchanged;
          }
        else
          {
            svn_boolean_t is_mod;
            SVN_ERR(svn_wc__internal_file_modified_p(&is_mod,
                                                     eb->db, eb->local_abspath,
                                                     FALSE, pool));

            if (!is_mod)
              {
                install_pristine = TRUE;
                content_state = svn_wc_notify_state_changed;
              }
            else
              {
                enum svn_wc_merge_outcome_t merge_outcome;
                /* Ok, we have to do some work to merge a local change */
                SVN_ERR(svn_wc__perform_file_merge(&work_item,
                                                   &merge_outcome,
                                                   eb->db,
                                                   eb->local_abspath,
                                                   eb->wri_abspath,
                                                   new_checksum,
                                                   original_checksum,
                                                   actual_props,
                                                   eb->ext_patterns,
                                                   eb->original_revision,
                                                   *eb->target_revision,
                                                   eb->propchanges,
                                                   eb->diff3cmd,
                                                   eb->conflict_func,
                                                   eb->conflict_baton,
                                                   eb->cancel_func,
                                                   eb->cancel_baton,
                                                   pool, pool));

                all_work_items = svn_wc__wq_merge(all_work_items, work_item,
                                                  pool);

                if (merge_outcome == svn_wc_merge_conflict)
                  content_state = svn_wc_notify_state_conflicted;
                else
                  content_state = svn_wc_notify_state_merged;
              }
          }
        if (install_pristine)
          {
            SVN_ERR(svn_wc__wq_build_file_install(&work_item, eb->db,
                                            eb->local_abspath,
                                            install_from,
                                            eb->use_commit_times, TRUE,
                                            pool, pool));

            all_work_items = svn_wc__wq_merge(all_work_items, work_item, pool);
          }
      }
    else
      {
        content_state = svn_wc_notify_state_unchanged;
        /* ### Retranslate on magic property changes, etc. */
      }

    SVN_ERR(svn_wc__db_external_add_file(
                        eb->db,
                        eb->local_abspath,
                        eb->wri_abspath,
                        repos_relpath,
                        eb->repos_root_url,
                        eb->repos_uuid,
                        *eb->target_revision,
                        new_pristine_props,
                        eb->changed_rev,
                        eb->changed_date,
                        eb->changed_author,
                        new_checksum,
                        new_dav_props,
                        eb->record_ancestor_abspath,
                        eb->recorded_repos_relpath,
                        eb->recorded_peg_revision,
                        eb->recorded_revision,
                        TRUE, new_actual_props,
                        FALSE /* keep_recorded_info */,
                        all_work_items,
                        pool));

    SVN_ERR(svn_wc__wq_run(eb->db, eb->wri_abspath,
                           eb->cancel_func, eb->cancel_baton, pool));
  }

  if (eb->notify_func)
    {
      svn_wc_notify_action_t action;
      svn_wc_notify_t *notify;

      if (SVN_IS_VALID_REVNUM(eb->original_revision))
        action = obstructed ? svn_wc_notify_update_shadowed_update
                            : svn_wc_notify_update_update;
      else
        action = obstructed ? svn_wc_notify_update_shadowed_add
                            : svn_wc_notify_update_add;

      notify = svn_wc_create_notify(eb->local_abspath, action, pool);
      notify->kind = svn_node_file;

      notify->revision = *eb->target_revision;
      notify->prop_state = prop_state;
      notify->content_state = content_state;

      notify->old_revision = eb->original_revision;

      eb->notify_func(eb->notify_baton, notify, pool);
    }


  return SVN_NO_ERROR;
}

/* svn_delta_editor_t function for svn_wc__get_file_external_editor */
static svn_error_t *
close_edit(void *edit_baton,
           apr_pool_t *pool)
{
  struct edit_baton *eb = edit_baton;

  if (!eb->file_closed)
    {
      /* The node wasn't updated, so we just have to bump its revision */
      SVN_ERR(svn_wc__db_op_bump_revisions_post_update(eb->db,
                                                       eb->local_abspath,
                                                       svn_depth_infinity,
                                                       NULL, NULL, NULL,
                                                       *eb->target_revision,
                                                       apr_hash_make(pool),
                                                       pool));
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__get_file_external_editor(const svn_delta_editor_t **editor,
                                 void **edit_baton,
                                 svn_revnum_t *target_revision,
                                 svn_wc_context_t *wc_ctx,
                                 const char *local_abspath,
                                 const char *wri_abspath,
                                 const char *url,
                                 const char *repos_root_url,
                                 const char *repos_uuid,
                                 svn_boolean_t use_commit_times,
                                 const char *diff3_cmd,
                                 const apr_array_header_t *preserved_exts,
                                 const char *record_ancestor_abspath,
                                 const char *recorded_url,
                                 const svn_opt_revision_t *recorded_peg_rev,
                                 const svn_opt_revision_t *recorded_rev,
                                 svn_wc_conflict_resolver_func2_t conflict_func,
                                 void *conflict_baton,
                                 svn_cancel_func_t cancel_func,
                                 void *cancel_baton,
                                 svn_wc_notify_func2_t notify_func,
                                 void *notify_baton,
                                 apr_pool_t *result_pool,
                                 apr_pool_t *scratch_pool)
{
  svn_wc__db_t *db = wc_ctx->db;
  apr_pool_t *edit_pool = result_pool;
  struct edit_baton *eb = apr_pcalloc(edit_pool, sizeof(*eb));
  svn_delta_editor_t *tree_editor = svn_delta_default_editor(edit_pool);

  eb->pool = edit_pool;
  eb->db = db;
  eb->local_abspath = apr_pstrdup(edit_pool, local_abspath);
  if (wri_abspath)
    eb->wri_abspath = apr_pstrdup(edit_pool, wri_abspath);
  else
    eb->wri_abspath = svn_dirent_dirname(local_abspath, edit_pool);
  eb->name = svn_dirent_basename(eb->local_abspath, NULL);
  eb->target_revision = target_revision;

  eb->url = apr_pstrdup(edit_pool, url);
  eb->repos_root_url = apr_pstrdup(edit_pool, repos_root_url);
  eb->repos_uuid = apr_pstrdup(edit_pool, repos_uuid);

  eb->use_commit_times = use_commit_times;
  eb->ext_patterns = preserved_exts;
  eb->diff3cmd = diff3_cmd;

  eb->record_ancestor_abspath = apr_pstrdup(edit_pool,record_ancestor_abspath);
  eb->recorded_repos_relpath = svn_uri__is_child(repos_root_url, recorded_url,
                                                 edit_pool);

  eb->changed_rev = SVN_INVALID_REVNUM;

  if (recorded_peg_rev->kind == svn_opt_revision_number)
    eb->recorded_peg_revision = recorded_peg_rev->value.number;
  else
    eb->recorded_peg_revision = SVN_INVALID_REVNUM; /* Not fixed/HEAD */

  if (recorded_rev->kind == svn_opt_revision_number)
    eb->recorded_revision = recorded_rev->value.number;
  else
    eb->recorded_revision = SVN_INVALID_REVNUM; /* Not fixed/HEAD */

  eb->conflict_func = conflict_func;
  eb->conflict_baton = conflict_baton;
  eb->cancel_func = cancel_func;
  eb->cancel_baton = cancel_baton;
  eb->notify_func = notify_func;
  eb->notify_baton = notify_baton;

  eb->propchanges  = apr_array_make(edit_pool, 1, sizeof(svn_prop_t));

  tree_editor->open_root = open_root;
  tree_editor->set_target_revision = set_target_revision;
  tree_editor->add_file = add_file;
  tree_editor->open_file = open_file;
  tree_editor->apply_textdelta = apply_textdelta;
  tree_editor->change_file_prop = change_file_prop;
  tree_editor->close_file = close_file;
  tree_editor->close_edit = close_edit;

  return svn_delta_get_cancellation_editor(cancel_func, cancel_baton,
                                           tree_editor, eb,
                                           editor, edit_baton,
                                           result_pool);
}

svn_error_t *
svn_wc__crawl_file_external(svn_wc_context_t *wc_ctx,
                            const char *local_abspath,
                            const svn_ra_reporter3_t *reporter,
                            void *report_baton,
                            svn_boolean_t restore_files,
                            svn_boolean_t use_commit_times,
                            svn_cancel_func_t cancel_func,
                            void *cancel_baton,
                            svn_wc_notify_func2_t notify_func,
                            void *notify_baton,
                            apr_pool_t *scratch_pool)
{
  svn_wc__db_t *db = wc_ctx->db;
  svn_error_t *err;
  svn_wc__db_kind_t kind;
  svn_wc__db_lock_t *lock;
  svn_revnum_t revision;
  const char *repos_root_url;
  const char *repos_relpath;
  svn_boolean_t update_root;

  err = svn_wc__db_base_get_info(NULL, &kind, &revision,
                                 &repos_relpath, &repos_root_url, NULL, NULL,
                                 NULL, NULL, NULL, NULL, NULL, &lock,
                                 NULL, &update_root,
                                 db, local_abspath,
                                 scratch_pool, scratch_pool);

  if (err
      || kind == svn_wc__db_kind_dir
      || !update_root)
    {
      if (err && err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND)
        return svn_error_trace(err);

      svn_error_clear(err);

      /* We don't know about this node, so all we have to do is tell
         the reporter that we don't know this node.

         But first we have to start the report by sending some basic
         information for the root. */

      SVN_ERR(reporter->set_path(report_baton, "", 0, svn_depth_infinity,
                                 FALSE, NULL, scratch_pool));
      SVN_ERR(reporter->delete_path(report_baton, "", scratch_pool));

      /* Finish the report, which causes the update editor to be
         driven. */
      SVN_ERR(reporter->finish_report(report_baton, scratch_pool));

      return SVN_NO_ERROR;
    }
  else
    {
      if (restore_files)
        {
          svn_node_kind_t disk_kind;
          SVN_ERR(svn_io_check_path(local_abspath, &disk_kind, scratch_pool));

          if (disk_kind == svn_node_none)
            {
              err = svn_wc_restore(wc_ctx, local_abspath, use_commit_times,
                                   scratch_pool);

              if (err)
                {
                  if (err->apr_err != SVN_ERR_WC_PATH_UNEXPECTED_STATUS)
                    return svn_error_trace(err);

                  svn_error_clear(err);
                }
            }
        }

      /* Report that we know the path */
      SVN_ERR(reporter->set_path(report_baton, "", revision,
                                 svn_depth_infinity, FALSE, NULL,
                                 scratch_pool));

      /* For compatibility with the normal update editor report we report
         the target as switched.

         ### We can probably report a parent url and unswitched later */
      SVN_ERR(reporter->link_path(report_baton, "",
                                  svn_path_url_add_component2(repos_root_url,
                                                              repos_relpath,
                                                              scratch_pool),
                                  revision,
                                  svn_depth_infinity,
                                  FALSE /* start_empty*/,
                                  lock ? lock->token : NULL,
                                  scratch_pool));
    }

  return svn_error_trace(reporter->finish_report(report_baton, scratch_pool));
}

svn_error_t *
svn_wc__read_external_info(svn_node_kind_t *external_kind,
                           const char **defining_abspath,
                           const char **defining_url,
                           svn_revnum_t *defining_operational_revision,
                           svn_revnum_t *defining_revision,
                           svn_wc_context_t *wc_ctx,
                           const char *wri_abspath,
                           const char *local_abspath,
                           svn_boolean_t ignore_enoent,
                           apr_pool_t *result_pool,
                           apr_pool_t *scratch_pool)
{
  const char *repos_root_url;
  svn_wc__db_status_t status;
  svn_wc__db_kind_t kind;
  svn_error_t *err;

  err = svn_wc__db_external_read(&status, &kind, defining_abspath,
                                 defining_url ? &repos_root_url : NULL, NULL,
                                 defining_url, defining_operational_revision,
                                 defining_revision,
                                 wc_ctx->db, local_abspath, wri_abspath,
                                 result_pool, scratch_pool);

  if (err)
    {
      if (err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND || !ignore_enoent)
        return svn_error_trace(err);

      svn_error_clear(err);

      if (external_kind)
        *external_kind = svn_node_none;

      if (defining_abspath)
        *defining_abspath = NULL;

      if (defining_url)
        *defining_url = NULL;

      if (defining_operational_revision)
        *defining_operational_revision = SVN_INVALID_REVNUM;

      if (defining_revision)
        *defining_revision = SVN_INVALID_REVNUM;

      return SVN_NO_ERROR;
    }

  if (external_kind)
    {
      if (status != svn_wc__db_status_normal)
        *external_kind = svn_node_unknown;
      else
        switch(kind)
          {
            case svn_wc__db_kind_file:
            case svn_wc__db_kind_symlink:
              *external_kind = svn_node_file;
              break;
            case svn_wc__db_kind_dir:
              *external_kind = svn_node_dir;
              break;
            default:
              *external_kind = svn_node_none;
          }
    }

  if (defining_url && *defining_url)
    *defining_url = svn_path_url_add_component2(repos_root_url, *defining_url,
                                                result_pool);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__externals_defined_below(apr_hash_t **externals,
                                svn_wc_context_t *wc_ctx,
                                const char *local_abspath,
                                apr_pool_t *result_pool,
                                apr_pool_t *scratch_pool)
{
  return svn_error_trace(
            svn_wc__db_externals_defined_below(externals,
                                               wc_ctx->db, local_abspath,
                                               result_pool, scratch_pool));
}

svn_error_t *
svn_wc__external_register(svn_wc_context_t *wc_ctx,
                          const char *defining_abspath,
                          const char *local_abspath,
                          svn_node_kind_t kind,
                          const char *repos_root_url,
                          const char *repos_uuid,
                          const char *repos_relpath,
                          svn_revnum_t operational_revision,
                          svn_revnum_t revision,
                          apr_pool_t *scratch_pool)
{
  SVN_ERR_ASSERT(kind == svn_node_dir);
  return svn_error_trace(
            svn_wc__db_external_add_dir(wc_ctx->db, local_abspath,
                                        defining_abspath,
                                        repos_root_url,
                                        repos_uuid,
                                        defining_abspath,
                                        repos_relpath,
                                        operational_revision,
                                        revision,
                                        NULL,
                                        scratch_pool));
}

svn_error_t *
svn_wc__external_remove(svn_wc_context_t *wc_ctx,
                        const char *wri_abspath,
                        const char *local_abspath,
                        svn_cancel_func_t cancel_func,
                        void *cancel_baton,
                        apr_pool_t *scratch_pool)
{
  svn_wc__db_status_t status;
  svn_wc__db_kind_t kind;

  SVN_ERR(svn_wc__db_external_read(&status, &kind, NULL, NULL, NULL, NULL,
                                   NULL, NULL,
                                   wc_ctx->db, local_abspath, wri_abspath,
                                   scratch_pool, scratch_pool));

  SVN_ERR(svn_wc__db_external_remove(wc_ctx->db, local_abspath, wri_abspath,
                                     NULL, scratch_pool));

  if (kind == svn_wc__db_kind_dir)
    SVN_ERR(svn_wc_remove_from_revision_control2(wc_ctx, local_abspath,
                                                 TRUE, FALSE,
                                                 cancel_func, cancel_baton,
                                                 scratch_pool));
  else
    {
      SVN_ERR(svn_wc__db_base_remove(wc_ctx->db, local_abspath, scratch_pool));
      SVN_ERR(svn_io_remove_file2(local_abspath, TRUE, scratch_pool));
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__externals_gather_definitions(apr_hash_t **externals,
                                     apr_hash_t **depths,
                                     svn_wc_context_t *wc_ctx,
                                     const char *local_abspath,
                                     svn_depth_t depth,
                                     apr_pool_t *result_pool,
                                     apr_pool_t *scratch_pool)
{
  if (depth == svn_depth_infinity
      || depth == svn_depth_unknown)
    {
      return svn_error_trace(
        svn_wc__db_externals_gather_definitions(externals, depths,
                                                wc_ctx->db, local_abspath,
                                                result_pool, scratch_pool));
    }
  else
    {
      const svn_string_t *value;
      svn_error_t *err;
      *externals = apr_hash_make(result_pool);

      local_abspath = apr_pstrdup(result_pool, local_abspath);

      err = svn_wc_prop_get2(&value, wc_ctx, local_abspath,
                             SVN_PROP_EXTERNALS, result_pool, scratch_pool);

      if (err)
        {
          if (err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND)
            return svn_error_trace(err);

          svn_error_clear(err);
          value = NULL;
        }

      if (value)
        apr_hash_set(*externals, local_abspath, APR_HASH_KEY_STRING,
                     value->data);

      if (value && depths)
        {
          svn_depth_t node_depth;
          *depths = apr_hash_make(result_pool);

          SVN_ERR(svn_wc__db_read_info(NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL, NULL, &node_depth, NULL,
                                       NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL, NULL, NULL,
                                       wc_ctx->db, local_abspath,
                                       scratch_pool, scratch_pool));

          apr_hash_set(*depths, local_abspath, APR_HASH_KEY_STRING,
                       svn_depth_to_word(node_depth));
        }

      return SVN_NO_ERROR;
    }
}

svn_error_t *
svn_wc__close_db(const char *external_abspath,
                 svn_wc_context_t *wc_ctx,
                 apr_pool_t *scratch_pool)
{
  SVN_ERR(svn_wc__db_drop_root(wc_ctx->db, external_abspath,
                               scratch_pool));
  return SVN_NO_ERROR;
}

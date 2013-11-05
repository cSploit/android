/*
 * update.c: mod_dav_svn REPORT handler for transmitting tree deltas
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

#include <apr_pools.h>
#include <apr_strings.h>
#include <apr_xml.h>

#include <http_request.h>
#include <http_log.h>
#include <mod_dav.h>

#include "svn_pools.h"
#include "svn_repos.h"
#include "svn_fs.h"
#include "svn_base64.h"
#include "svn_xml.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_dav.h"
#include "svn_props.h"

#include "private/svn_log.h"
#include "private/svn_fspath.h"

#include "../dav_svn.h"


typedef struct update_ctx_t {
  const dav_resource *resource;

  /* the revision we are updating to. used to generated IDs. */
  svn_fs_root_t *rev_root;

  const char *anchor;
  const char *target;

  /* if doing a regular update, then dst_path == anchor.  if this is a
     'switch' operation, then this field is the fs path that is being
     switched to.  This path needs to telescope in the update-editor
     just like 'anchor' above; it's used for retrieving CR's and
     vsn-url's during the edit. */
  const char *dst_path;

  /* this buffers the output for a bit and is automatically flushed,
     at appropriate times, by the Apache filter system. */
  apr_bucket_brigade *bb;

  /* where to deliver the output */
  ap_filter_t *output;

  /* where do these editor paths *really* point to? */
  apr_hash_t *pathmap;

  /* are we doing a resource walk? */
  svn_boolean_t resource_walk;

  /* True iff we've already sent the open tag for the update. */
  svn_boolean_t started_update;

  /* True iff client requested all data inline in the report. */
  svn_boolean_t send_all;

  /* SVNDIFF version to send to client.  */
  int svndiff_version;
} update_ctx_t;

typedef struct item_baton_t {
  apr_pool_t *pool;
  update_ctx_t *uc;
  struct item_baton_t *parent; /* the parent of this item. */
  const char *name;    /* the single-component name of this item */
  const char *path;    /* a telescoping extension of uc->anchor */
  const char *path2;   /* a telescoping extension of uc->dst_path */
  const char *path3;   /* a telescoping extension of uc->dst_path
                            without dst_path as prefix. */

  const char *base_checksum;   /* base_checksum (from apply_textdelta) */

  svn_boolean_t text_changed;        /* Did the file's contents change? */
  svn_boolean_t added;               /* File added? (Implies text_changed.) */
  svn_boolean_t copyfrom;            /* File copied? */
  apr_array_header_t *removed_props; /* array of const char * prop names */

} item_baton_t;


#define DIR_OR_FILE(is_dir) ((is_dir) ? "directory" : "file")


/* add PATH to the pathmap HASH with a repository path of LINKPATH.
   if LINKPATH is NULL, PATH will map to itself. */
static void
add_to_path_map(apr_hash_t *hash, const char *path, const char *linkpath)
{
  /* normalize 'root paths' to have a slash */
  const char *norm_path = strcmp(path, "") ? path : "/";

  /* if there is an actual linkpath given, it is the repos path, else
     our path maps to itself. */
  const char *repos_path = linkpath ? linkpath : norm_path;

  /* now, geez, put the path in the map already! */
  apr_hash_set(hash, path, APR_HASH_KEY_STRING, repos_path);
}


/* return the actual repository path referred to by the editor's PATH,
   allocated in POOL, determined by examining the pathmap HASH. */
static const char *
get_from_path_map(apr_hash_t *hash, const char *path, apr_pool_t *pool)
{
  const char *repos_path;
  svn_stringbuf_t *my_path;

  /* no hash means no map.  that's easy enough. */
  if (! hash)
    return apr_pstrdup(pool, path);

  if ((repos_path = apr_hash_get(hash, path, APR_HASH_KEY_STRING)))
    {
      /* what luck!  this path is a hash key!  if there is a linkpath,
         use that, else return the path itself. */
      return apr_pstrdup(pool, repos_path);
    }

  /* bummer.  PATH wasn't a key in path map, so we get to start
     hacking off components and looking for a parent from which to
     derive a repos_path.  use a stringbuf for convenience. */
  my_path = svn_stringbuf_create(path, pool);
  do
    {
      svn_path_remove_component(my_path);
      if ((repos_path = apr_hash_get(hash, my_path->data, my_path->len)))
        {
          /* we found a mapping ... but of one of PATH's parents.
             soooo, we get to re-append the chunks of PATH that we
             broke off to the REPOS_PATH we found. */
          return svn_fspath__join(repos_path, path + my_path->len + 1, pool);
        }
    }
  while (! svn_path_is_empty(my_path->data)
         && strcmp(my_path->data, "/") != 0);

  /* well, we simply never found anything worth mentioning the map.
     PATH is its own default finding, then. */
  return apr_pstrdup(pool, path);
}


static item_baton_t *
make_child_baton(item_baton_t *parent, const char *path, apr_pool_t *pool)
{
  item_baton_t *baton;

  baton = apr_pcalloc(pool, sizeof(*baton));
  baton->pool = pool;
  baton->uc = parent->uc;
  baton->name = svn_relpath_basename(path, pool);
  baton->parent = parent;

  /* Telescope the path based on uc->anchor.  */
  baton->path = svn_fspath__join(parent->path, baton->name, pool);

  /* Telescope the path based on uc->dst_path in the exact same way. */
  baton->path2 = svn_fspath__join(parent->path2, baton->name, pool);

  /* Telescope the third path:  it's relative, not absolute, to
     dst_path.  Now, we gotta be careful here, because if this
     operation had a target, and we're it, then we have to use the
     basename of our source reflection instead of our own.  */
  if ((*baton->uc->target) && (! parent->parent))
    baton->path3 = svn_relpath_join(parent->path3, baton->uc->target, pool);
  else
    baton->path3 = svn_relpath_join(parent->path3, baton->name, pool);

  return baton;
}


/* Get the real filesystem PATH for BATON, and return the value
   allocated from POOL.  This function juggles the craziness of
   updates, switches, and updates of switched things. */
static const char *
get_real_fs_path(item_baton_t *baton, apr_pool_t *pool)
{
  const char *path = get_from_path_map(baton->uc->pathmap, baton->path, pool);
  return strcmp(path, baton->path) ? path : baton->path2;
}


static svn_error_t *
send_vsn_url(item_baton_t *baton, apr_pool_t *pool)
{
  const char *href;
  const char *path;
  svn_revnum_t revision;

  /* Try to use the CR, assuming the path exists in CR. */
  path = get_real_fs_path(baton, pool);
  revision = dav_svn__get_safe_cr(baton->uc->rev_root, path, pool);

  href = dav_svn__build_uri(baton->uc->resource->info->repos,
                            DAV_SVN__BUILD_URI_VERSION,
                            revision, path, 0 /* add_href */, pool);

  return dav_svn__brigade_printf(baton->uc->bb, baton->uc->output,
                                 "<D:checked-in><D:href>%s</D:href>"
                                 "</D:checked-in>" DEBUG_CR,
                                 apr_xml_quote_string(pool, href, 1));
}


static svn_error_t *
absent_helper(svn_boolean_t is_dir,
              const char *path,
              item_baton_t *parent,
              apr_pool_t *pool)
{
  update_ctx_t *uc = parent->uc;

  if (! uc->resource_walk)
    {
      SVN_ERR(dav_svn__brigade_printf
              (uc->bb, uc->output,
               "<S:absent-%s name=\"%s\"/>" DEBUG_CR,
               DIR_OR_FILE(is_dir),
               apr_xml_quote_string(pool,
                                    svn_relpath_basename(path, NULL),
                                    1)));
    }

  return SVN_NO_ERROR;
}


static svn_error_t *
upd_absent_directory(const char *path, void *parent_baton, apr_pool_t *pool)
{
  return absent_helper(TRUE, path, parent_baton, pool);
}


static svn_error_t *
upd_absent_file(const char *path, void *parent_baton, apr_pool_t *pool)
{
  return absent_helper(FALSE, path, parent_baton, pool);
}


static svn_error_t *
add_helper(svn_boolean_t is_dir,
           const char *path,
           item_baton_t *parent,
           const char *copyfrom_path,
           svn_revnum_t copyfrom_revision,
           apr_pool_t *pool,
           void **child_baton)
{
  item_baton_t *child;
  update_ctx_t *uc = parent->uc;
  const char *bc_url = NULL;

  child = make_child_baton(parent, path, pool);
  child->added = TRUE;

  if (uc->resource_walk)
    {
      SVN_ERR(dav_svn__brigade_printf(child->uc->bb, child->uc->output,
                                      "<S:resource path=\"%s\">" DEBUG_CR,
                                      apr_xml_quote_string(pool, child->path3,
                                                           1)));
    }
  else
    {
      const char *qname = apr_xml_quote_string(pool, child->name, 1);
      const char *elt;
      const char *real_path = get_real_fs_path(child, pool);
      const char *bc_url_str = "";
      const char *sha1_checksum_str = "";

      if (is_dir)
        {
          /* we send baseline-collection urls when we add a directory */
          svn_revnum_t revision;
          revision = dav_svn__get_safe_cr(child->uc->rev_root, real_path,
                                          pool);
          bc_url = dav_svn__build_uri(child->uc->resource->info->repos,
                                      DAV_SVN__BUILD_URI_BC,
                                      revision, real_path,
                                      0 /* add_href */, pool);
          bc_url = svn_urlpath__canonicalize(bc_url, pool);

          /* ugh, build_uri ignores the path and just builds the root
             of the baseline collection.  we have to tack the
             real_path on manually, ignoring its leading slash. */
          if (real_path && (! svn_path_is_empty(real_path)))
            bc_url = svn_urlpath__join(bc_url,
                                       svn_path_uri_encode(real_path + 1,
                                                           pool),
                                       pool);

          /* make sure that the BC_URL is xml attribute safe. */
          bc_url = apr_xml_quote_string(pool, bc_url, 1);
        }
      else
        {
          svn_checksum_t *sha1_checksum;

          SVN_ERR(svn_fs_file_checksum(&sha1_checksum, svn_checksum_sha1,
                                       uc->rev_root, real_path, FALSE, pool));
          if (sha1_checksum)
            sha1_checksum_str =
              apr_psprintf(pool, " sha1-checksum=\"%s\"",
                           svn_checksum_to_cstring(sha1_checksum, pool));
        }

      if (bc_url)
        bc_url_str = apr_psprintf(pool, " bc-url=\"%s\"", bc_url);

      if (copyfrom_path == NULL)
        {
          elt = apr_psprintf(pool,
                             "<S:add-%s name=\"%s\"%s%s>" DEBUG_CR,
                             DIR_OR_FILE(is_dir), qname, bc_url_str,
                             sha1_checksum_str);
        }
      else
        {
          const char *qcopy = apr_xml_quote_string(pool, copyfrom_path, 1);

          elt = apr_psprintf(pool,
                             "<S:add-%s name=\"%s\"%s%s "
                             "copyfrom-path=\"%s\" copyfrom-rev=\"%ld\">"
                             DEBUG_CR,
                             DIR_OR_FILE(is_dir),
                             qname, bc_url_str, sha1_checksum_str,
                             qcopy, copyfrom_revision);
          child->copyfrom = TRUE;
        }

      /* Resist the temptation to use 'elt' as a format string (to the
         likes of dav_svn__brigade_printf).  Because it contains URIs,
         it might have sequences that look like format string insert
         placeholders.  For example, "this%20dir" is a valid printf()
         format string that means "this[insert an integer of width 20
         here]ir". */
      SVN_ERR(dav_svn__brigade_puts(child->uc->bb, child->uc->output, elt));
    }

  SVN_ERR(send_vsn_url(child, pool));

  if (uc->resource_walk)
    SVN_ERR(dav_svn__brigade_puts(child->uc->bb, child->uc->output,
                                  "</S:resource>" DEBUG_CR));

  *child_baton = child;

  return SVN_NO_ERROR;
}


static svn_error_t *
open_helper(svn_boolean_t is_dir,
            const char *path,
            item_baton_t *parent,
            svn_revnum_t base_revision,
            apr_pool_t *pool,
            void **child_baton)
{
  item_baton_t *child = make_child_baton(parent, path, pool);
  const char *qname = apr_xml_quote_string(pool, child->name, 1);

  SVN_ERR(dav_svn__brigade_printf(child->uc->bb, child->uc->output,
                                  "<S:open-%s name=\"%s\""
                                  " rev=\"%ld\">" DEBUG_CR,
                                  DIR_OR_FILE(is_dir), qname, base_revision));
  SVN_ERR(send_vsn_url(child, pool));
  *child_baton = child;
  return SVN_NO_ERROR;
}


static svn_error_t *
close_helper(svn_boolean_t is_dir, item_baton_t *baton)
{
  if (baton->uc->resource_walk)
    return SVN_NO_ERROR;

  /* ### ack!  binary names won't float here! */
  /* If this is a copied file/dir, we can have removed props. */
  if (baton->removed_props && baton->copyfrom)
    {
      const char *qname;
      int i;

      for (i = 0; i < baton->removed_props->nelts; i++)
        {
          /* We already XML-escaped the property name in change_xxx_prop. */
          qname = APR_ARRAY_IDX(baton->removed_props, i, const char *);
          SVN_ERR(dav_svn__brigade_printf(baton->uc->bb, baton->uc->output,
                                          "<S:remove-prop name=\"%s\"/>"
                                          DEBUG_CR, qname));
        }
    }

  if (baton->added)
    SVN_ERR(dav_svn__brigade_printf(baton->uc->bb, baton->uc->output,
                                    "</S:add-%s>" DEBUG_CR,
                                    DIR_OR_FILE(is_dir)));
  else
    SVN_ERR(dav_svn__brigade_printf(baton->uc->bb, baton->uc->output,
                                    "</S:open-%s>" DEBUG_CR,
                                    DIR_OR_FILE(is_dir)));
  return SVN_NO_ERROR;
}


/* Send the opening tag of the update-report if it hasn't been sent
   already. */
static svn_error_t *
maybe_start_update_report(update_ctx_t *uc)
{
  if ((! uc->resource_walk) && (! uc->started_update))
    {
      SVN_ERR(dav_svn__brigade_printf(uc->bb, uc->output,
                                      DAV_XML_HEADER DEBUG_CR
                                      "<S:update-report xmlns:S=\""
                                      SVN_XML_NAMESPACE "\" "
                                      "xmlns:V=\"" SVN_DAV_PROP_NS_DAV "\" "
                                      "xmlns:D=\"DAV:\" %s>" DEBUG_CR,
                                      uc->send_all ? "send-all=\"true\"" : ""));

      uc->started_update = TRUE;
    }

  return SVN_NO_ERROR;
}


static svn_error_t *
upd_set_target_revision(void *edit_baton,
                        svn_revnum_t target_revision,
                        apr_pool_t *pool)
{
  update_ctx_t *uc = edit_baton;

  SVN_ERR(maybe_start_update_report(uc));

  if (! uc->resource_walk)
    SVN_ERR(dav_svn__brigade_printf(uc->bb, uc->output,
                                    "<S:target-revision rev=\"%ld\"/>"
                                    DEBUG_CR, target_revision));

  return SVN_NO_ERROR;
}


static svn_error_t *
upd_open_root(void *edit_baton,
              svn_revnum_t base_revision,
              apr_pool_t *pool,
              void **root_baton)
{
  update_ctx_t *uc = edit_baton;
  item_baton_t *b = apr_pcalloc(pool, sizeof(*b));

  /* note that we create a subpool; the root_baton is passed to the
     close_directory callback, where we will destroy the pool. */

  b->uc = uc;
  b->pool = pool;
  b->path = uc->anchor;
  b->path2 = uc->dst_path;
  b->path3 = "";

  *root_baton = b;

  SVN_ERR(maybe_start_update_report(uc));

  if (uc->resource_walk)
    SVN_ERR(dav_svn__brigade_printf(uc->bb, uc->output,
                                    "<S:resource path=\"%s\">" DEBUG_CR,
                                    apr_xml_quote_string(pool, b->path3, 1)));
  else
    SVN_ERR(dav_svn__brigade_printf(uc->bb, uc->output,
                                    "<S:open-directory rev=\"%ld\">" DEBUG_CR,
                                    base_revision));

  /* Only transmit the root directory's Version Resource URL if
     there's no target. */
  if (! *uc->target)
    SVN_ERR(send_vsn_url(b, pool));

  if (uc->resource_walk)
    SVN_ERR(dav_svn__brigade_puts(uc->bb, uc->output,
                                  "</S:resource>" DEBUG_CR));

  return SVN_NO_ERROR;
}


static svn_error_t *
upd_delete_entry(const char *path,
                 svn_revnum_t revision,
                 void *parent_baton,
                 apr_pool_t *pool)
{
  item_baton_t *parent = parent_baton;
  const char *qname = apr_xml_quote_string(pool,
                                           svn_relpath_basename(path, NULL),
                                           1);
  return dav_svn__brigade_printf(parent->uc->bb, parent->uc->output,
                                 "<S:delete-entry name=\"%s\" rev=\"%ld\"/>"
                                   DEBUG_CR, qname, revision);
}


static svn_error_t *
upd_add_directory(const char *path,
                  void *parent_baton,
                  const char *copyfrom_path,
                  svn_revnum_t copyfrom_revision,
                  apr_pool_t *pool,
                  void **child_baton)
{
  return add_helper(TRUE /* is_dir */,
                    path, parent_baton, copyfrom_path, copyfrom_revision, pool,
                    child_baton);
}


static svn_error_t *
upd_open_directory(const char *path,
                                        void *parent_baton,
                                        svn_revnum_t base_revision,
                                        apr_pool_t *pool,
                                        void **child_baton)
{
  return open_helper(TRUE /* is_dir */,
                     path, parent_baton, base_revision, pool, child_baton);
}


static svn_error_t *
upd_change_xxx_prop(void *baton,
                    const char *name,
                    const svn_string_t *value,
                    apr_pool_t *pool)
{
  item_baton_t *b = baton;
  const char *qname;

  /* Resource walks say nothing about props. */
  if (b->uc->resource_walk)
    return SVN_NO_ERROR;

  /* Else this not a resource walk, so either send props or cache them
     to send later, depending on whether this is a modern report
     response or not. */

  qname = apr_xml_quote_string(b->pool, name, 1);

  /* apr_xml_quote_string doesn't realloc if there is nothing to
     quote, so dup the name, but only if necessary. */
  if (qname == name)
    qname = apr_pstrdup(b->pool, name);

  /* Even if we are not in send-all mode we have the prop changes already,
     so send them to the client now instead of telling the client to fetch
     them later. */
  if (b->uc->send_all || !b->added)
    {
      if (value)
        {
          const char *qval;

          if (svn_xml_is_xml_safe(value->data, value->len))
            {
              svn_stringbuf_t *tmp = NULL;
              svn_xml_escape_cdata_string(&tmp, value, pool);
              qval = tmp->data;
              SVN_ERR(dav_svn__brigade_printf(b->uc->bb, b->uc->output,
                                              "<S:set-prop name=\"%s\">",
                                              qname));
            }
          else
            {
              qval = svn_base64_encode_string2(value, TRUE, pool)->data;
              SVN_ERR(dav_svn__brigade_printf(b->uc->bb, b->uc->output,
                                              "<S:set-prop name=\"%s\" "
                                              "encoding=\"base64\">" DEBUG_CR,
                                              qname));
            }

          SVN_ERR(dav_svn__brigade_puts(b->uc->bb, b->uc->output, qval));
          SVN_ERR(dav_svn__brigade_puts(b->uc->bb, b->uc->output,
                                        "</S:set-prop>" DEBUG_CR));
        }
      else  /* value is null, so this is a prop removal */
        {
          SVN_ERR(dav_svn__brigade_printf(b->uc->bb, b->uc->output,
                                          "<S:remove-prop name=\"%s\"/>"
                                          DEBUG_CR,
                                          qname));
        }
    }
  else if (!value) /* This is an addition in 'skelta' mode so there is no
                      need for an inline response since property fetching
                      is implied in addition.  We still need to cache
                      property removals because a copied path might
                      have removed properties. */
    {
      if (! b->removed_props)
        b->removed_props = apr_array_make(b->pool, 1, sizeof(name));

      APR_ARRAY_PUSH(b->removed_props, const char *) = qname;
    }

  return SVN_NO_ERROR;
}


static svn_error_t *
upd_close_directory(void *dir_baton, apr_pool_t *pool)
{
  return close_helper(TRUE /* is_dir */, dir_baton);
}


static svn_error_t *
upd_add_file(const char *path,
             void *parent_baton,
             const char *copyfrom_path,
             svn_revnum_t copyfrom_revision,
             apr_pool_t *pool,
             void **file_baton)
{
  return add_helper(FALSE /* is_dir */,
                    path, parent_baton, copyfrom_path, copyfrom_revision, pool,
                    file_baton);
}


static svn_error_t *
upd_open_file(const char *path,
              void *parent_baton,
              svn_revnum_t base_revision,
              apr_pool_t *pool,
              void **file_baton)
{
  return open_helper(FALSE /* is_dir */,
                     path, parent_baton, base_revision, pool, file_baton);
}


/* We have our own window handler and baton as a simple wrapper around
   the real handler (which converts txdelta windows to base64-encoded
   svndiff data).  The wrapper is responsible for sending the opening
   and closing XML tags around the svndiff data. */
struct window_handler_baton
{
  svn_boolean_t seen_first_window;  /* False until first window seen. */
  update_ctx_t *uc;

  const char *base_checksum; /* For transfer as part of the S:txdelta element */

  /* The _real_ window handler and baton. */
  svn_txdelta_window_handler_t handler;
  void *handler_baton;
};


/* This implements 'svn_txdelta_window_handler_t'. */
static svn_error_t *
window_handler(svn_txdelta_window_t *window, void *baton)
{
  struct window_handler_baton *wb = baton;

  if (! wb->seen_first_window)
    {
      wb->seen_first_window = TRUE;

      if (!wb->base_checksum)
        SVN_ERR(dav_svn__brigade_puts(wb->uc->bb, wb->uc->output,
                                      "<S:txdelta>"));
      else
        SVN_ERR(dav_svn__brigade_printf(wb->uc->bb, wb->uc->output,
                                        "<S:txdelta base-checksum=\"%s\">",
                                        wb->base_checksum));
    }

  SVN_ERR(wb->handler(window, wb->handler_baton));

  if (window == NULL)
    {
      SVN_ERR(dav_svn__brigade_puts(wb->uc->bb, wb->uc->output,
                                    "</S:txdelta>"));
    }

  return SVN_NO_ERROR;
}


/* This implements 'svn_txdelta_window_handler_t'.
   During a resource walk, the driver sends an empty window as a
   boolean indicating that a change happened to this file, but we
   don't want to send anything over the wire as a result. */
static svn_error_t *
dummy_window_handler(svn_txdelta_window_t *window, void *baton)
{
  return SVN_NO_ERROR;
}


static svn_error_t *
upd_apply_textdelta(void *file_baton,
                    const char *base_checksum,
                    apr_pool_t *pool,
                    svn_txdelta_window_handler_t *handler,
                    void **handler_baton)
{
  item_baton_t *file = file_baton;
  struct window_handler_baton *wb;
  svn_stream_t *base64_stream;

  /* Store the base checksum and the fact the file's text changed. */
  file->base_checksum = apr_pstrdup(file->pool, base_checksum);
  file->text_changed = TRUE;

  /* If this is a resource walk, or if we're not in "send-all" mode,
     we don't actually want to transmit text-deltas. */
  if (file->uc->resource_walk || (! file->uc->send_all))
    {
      *handler = dummy_window_handler;
      *handler_baton = NULL;
      return SVN_NO_ERROR;
    }

  wb = apr_palloc(file->pool, sizeof(*wb));
  wb->seen_first_window = FALSE;
  wb->uc = file->uc;
  wb->base_checksum = file->base_checksum;
  base64_stream = dav_svn__make_base64_output_stream(wb->uc->bb,
                                                     wb->uc->output,
                                                     file->pool);

  svn_txdelta_to_svndiff3(&(wb->handler), &(wb->handler_baton),
                          base64_stream, file->uc->svndiff_version,
                          dav_svn__get_compression_level(), file->pool);

  *handler = window_handler;
  *handler_baton = wb;

  return SVN_NO_ERROR;
}


static svn_error_t *
upd_close_file(void *file_baton, const char *text_checksum, apr_pool_t *pool)
{
  item_baton_t *file = file_baton;

  /* If we are not in "send all" mode, and this file is not a new
     addition or didn't otherwise have changed text, tell the client
     to fetch it. */
  if ((! file->uc->send_all) && (! file->added) && file->text_changed)
    {
      svn_checksum_t *sha1_checksum;
      const char *real_path = get_real_fs_path(file, pool);
      const char *sha1_digest = NULL;

      /* Try to grab the SHA1 checksum, if it's readily available, and
         pump it down to the client, too. */
      SVN_ERR(svn_fs_file_checksum(&sha1_checksum, svn_checksum_sha1,
                                   file->uc->rev_root, real_path, FALSE, pool));
      if (sha1_checksum)
        sha1_digest = svn_checksum_to_cstring(sha1_checksum, pool);

      SVN_ERR(dav_svn__brigade_printf
              (file->uc->bb, file->uc->output,
               "<S:fetch-file%s%s%s%s%s%s/>" DEBUG_CR,
               file->base_checksum ? " base-checksum=\"" : "",
               file->base_checksum ? file->base_checksum : "",
               file->base_checksum ? "\"" : "",
               sha1_digest ? " sha1-checksum=\"" : "",
               sha1_digest ? sha1_digest : "",
               sha1_digest ? "\"" : ""));
    }

  if (text_checksum)
    {
      SVN_ERR(dav_svn__brigade_printf(file->uc->bb, file->uc->output,
                                      "<S:prop>"
                                      "<V:md5-checksum>%s</V:md5-checksum>"
                                      "</S:prop>",
                                      text_checksum));
    }

  return close_helper(FALSE /* is_dir */, file);
}


static svn_error_t *
upd_close_edit(void *edit_baton, apr_pool_t *pool)
{
  update_ctx_t *uc = edit_baton;

  /* Our driver will unconditionally close the update report... So if
     the report hasn't even been started yet, start it now. */
  return maybe_start_update_report(uc);
}


/* Return a specific error associated with the contents of TAGNAME
   being malformed.  Use pool for allocations.  */
static dav_error *
malformed_element_error(const char *tagname, apr_pool_t *pool)
{
  const char *errstr = apr_pstrcat(pool, "The request's '", tagname,
                                   "' element is malformed; there "
                                   "is a problem with the client.",
                                   (char *)NULL);
  return dav_svn__new_error_tag(pool, HTTP_BAD_REQUEST, 0, errstr,
                                SVN_DAV_ERROR_NAMESPACE, SVN_DAV_ERROR_TAG);
}


/* Validate that REVISION is a valid revision number for repository in
   which YOUNGEST is the latest revision.  Use RESOURCE as a
   convenient way to access the request record and a pool for error
   messaging.   (It's okay if REVISION is SVN_INVALID_REVNUM, as in
   the related contexts that just means "the youngest revision".)

   REVTYPE is just a string describing the type/purpose of REVISION,
   used in the generated error string.  */
static dav_error *
validate_input_revision(svn_revnum_t revision,
                        svn_revnum_t youngest,
                        const char *revtype,
                        const dav_resource *resource)
{
  if (! SVN_IS_VALID_REVNUM(revision))
    return SVN_NO_ERROR;
    
  if (revision > youngest)
    {
      svn_error_t *serr;

      if (dav_svn__get_master_uri(resource->info->r))
        {
          serr = svn_error_createf(SVN_ERR_FS_NO_SUCH_REVISION, 0,
                                   "No such %s '%ld' found in the repository.  "
                                   "Perhaps the repository is out of date with "
                                   "respect to the master repository?",
                                   revtype, revision);
        }
      else
        {
          serr = svn_error_createf(SVN_ERR_FS_NO_SUCH_REVISION, 0,
                                   "No such %s '%ld' found in the repository.",
                                   revtype, revision);
        }
      return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                  "Invalid revision found in update report "
                                  "request.", resource->pool);
    }
  return SVN_NO_ERROR;
}


dav_error *
dav_svn__update_report(const dav_resource *resource,
                       const apr_xml_doc *doc,
                       ap_filter_t *output)
{
  svn_delta_editor_t *editor;
  apr_xml_elem *child;
  void *rbaton = NULL;
  update_ctx_t uc = { 0 };
  svn_revnum_t youngest, revnum = SVN_INVALID_REVNUM;
  svn_revnum_t from_revnum = SVN_INVALID_REVNUM;
  int ns;
  /* entry_counter and entry_is_empty are for operational logging. */
  int entry_counter = 0;
  svn_boolean_t entry_is_empty = FALSE;
  svn_error_t *serr;
  dav_error *derr = NULL;
  const char *src_path = NULL;
  const char *dst_path = NULL;
  const dav_svn_repos *repos = resource->info->repos;
  const char *target = "";
  svn_boolean_t text_deltas = TRUE;
  svn_depth_t requested_depth = svn_depth_unknown;
  svn_boolean_t saw_depth = FALSE;
  svn_boolean_t saw_recursive = FALSE;
  svn_boolean_t resource_walk = FALSE;
  svn_boolean_t ignore_ancestry = FALSE;
  svn_boolean_t send_copyfrom_args = FALSE;
  dav_svn__authz_read_baton arb;
  apr_pool_t *subpool = svn_pool_create(resource->pool);

  /* Construct the authz read check baton. */
  arb.r = resource->info->r;
  arb.repos = repos;

  if ((resource->info->restype != DAV_SVN_RESTYPE_VCC)
      && (resource->info->restype != DAV_SVN_RESTYPE_ME))
    return dav_svn__new_error_tag(resource->pool, HTTP_CONFLICT, 0,
                                  "This report can only be run against "
                                  "a VCC or root-stub URI.",
                                  SVN_DAV_ERROR_NAMESPACE,
                                  SVN_DAV_ERROR_TAG);

  ns = dav_svn__find_ns(doc->namespaces, SVN_XML_NAMESPACE);
  if (ns == -1)
    {
      return dav_svn__new_error_tag(resource->pool, HTTP_BAD_REQUEST, 0,
                                    "The request does not contain the 'svn:' "
                                    "namespace, so it is not going to have an "
                                    "svn:target-revision element. That element "
                                    "is required.",
                                    SVN_DAV_ERROR_NAMESPACE,
                                    SVN_DAV_ERROR_TAG);
    }

  /* If server configuration permits bulk updates (a report with props
     and textdeltas inline, rather than placeholder tags that tell the
     client to do further fetches), look to see if client requested as
     much.  */
  if (repos->bulk_updates)
    {
      apr_xml_attr *this_attr;

      for (this_attr = doc->root->attr; this_attr; this_attr = this_attr->next)
        {
          if ((strcmp(this_attr->name, "send-all") == 0)
              && (strcmp(this_attr->value, "true") == 0))
            {
              uc.send_all = TRUE;
              break;
            }
        }
    }

  /* Ask the repository about its youngest revision (which we'll need
     for some input validation later). */
  if ((serr = svn_fs_youngest_rev(&youngest, repos->fs, resource->pool)))
    return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                "Could not determine the youngest "
                                "revision for the update process.",
                                resource->pool);

  for (child = doc->root->first_child; child != NULL; child = child->next)
    {
      /* Note that child->name might not match any of the cases below.
         Thus, the check for non-empty cdata in each of these cases
         cannot be moved to the top of the loop, because then it would
         wrongly catch other elements that do allow empty cdata. */
      const char *cdata;

      if (child->ns == ns && strcmp(child->name, "target-revision") == 0)
        {
          cdata = dav_xml_get_cdata(child, resource->pool, 1);
          if (! *cdata)
            return malformed_element_error(child->name, resource->pool);
          revnum = SVN_STR_TO_REV(cdata);
        }
      if (child->ns == ns && strcmp(child->name, "src-path") == 0)
        {
          dav_svn__uri_info this_info;
          cdata = dav_xml_get_cdata(child, resource->pool, 0);
          if (! *cdata)
            return malformed_element_error(child->name, resource->pool);
          if (svn_path_is_url(cdata))
            cdata = svn_uri_canonicalize(cdata, resource->pool);
          if ((serr = dav_svn__simple_parse_uri(&this_info, resource,
                                                cdata, resource->pool)))
            return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                        "Could not parse 'src-path' URL.",
                                        resource->pool);
          src_path = this_info.repos_path;
        }
      if (child->ns == ns && strcmp(child->name, "dst-path") == 0)
        {
          dav_svn__uri_info this_info;
          cdata = dav_xml_get_cdata(child, resource->pool, 0);
          if (! *cdata)
            return malformed_element_error(child->name, resource->pool);
          if (svn_path_is_url(cdata))
            cdata = svn_uri_canonicalize(cdata, resource->pool);
          if ((serr = dav_svn__simple_parse_uri(&this_info, resource,
                                                cdata, resource->pool)))
            return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                        "Could not parse 'dst-path' URL.",
                                        resource->pool);
          dst_path = this_info.repos_path;
        }
      if (child->ns == ns && strcmp(child->name, "update-target") == 0)
        {
          cdata = dav_xml_get_cdata(child, resource->pool, 0);
          if ((derr = dav_svn__test_canonical(cdata, resource->pool)))
            return derr;
          target = cdata;
        }
      if (child->ns == ns && strcmp(child->name, "depth") == 0)
        {
          cdata = dav_xml_get_cdata(child, resource->pool, 1);
          if (! *cdata)
            return malformed_element_error(child->name, resource->pool);
          requested_depth = svn_depth_from_word(cdata);
          saw_depth = TRUE;
        }
      if ((child->ns == ns && strcmp(child->name, "recursive") == 0)
          && (! saw_depth))
        {
          cdata = dav_xml_get_cdata(child, resource->pool, 1);
          if (! *cdata)
            return malformed_element_error(child->name, resource->pool);
          if (strcmp(cdata, "no") == 0)
            requested_depth = svn_depth_files;
          else
            requested_depth = svn_depth_infinity;
          /* Note that even modern, depth-aware clients still transmit
             "no" for "recursive" (along with "files" for "depth") in
             the svn_depth_files case, and transmit "no" in the
             svn_depth_empty case.  This is because they don't know if
             they're talking to a depth-aware server or not, and they
             don't need to know -- all they have to do is transmit
             both, and the server will DTRT either way (although in
             the svn_depth_empty case, the client will still have some
             work to do in ignoring the files that come down).

             When both "depth" and "recursive" are sent, we don't
             bother to check if they're mutually consistent, we just
             let depth dominate. */
          saw_recursive = TRUE;
        }
      if (child->ns == ns && strcmp(child->name, "ignore-ancestry") == 0)
        {
          cdata = dav_xml_get_cdata(child, resource->pool, 1);
          if (! *cdata)
            return malformed_element_error(child->name, resource->pool);
          if (strcmp(cdata, "no") != 0)
            ignore_ancestry = TRUE;
        }
      if (child->ns == ns && strcmp(child->name, "send-copyfrom-args") == 0)
        {
          cdata = dav_xml_get_cdata(child, resource->pool, 1);
          if (! *cdata)
            return malformed_element_error(child->name, resource->pool);
          if (strcmp(cdata, "no") != 0)
            send_copyfrom_args = TRUE;
        }
      if (child->ns == ns && strcmp(child->name, "resource-walk") == 0)
        {
          cdata = dav_xml_get_cdata(child, resource->pool, 1);
          if (! *cdata)
            return malformed_element_error(child->name, resource->pool);
          if (strcmp(cdata, "no") != 0)
            resource_walk = TRUE;
        }
      if (child->ns == ns && strcmp(child->name, "text-deltas") == 0)
        {
          cdata = dav_xml_get_cdata(child, resource->pool, 1);
          if (! *cdata)
            return malformed_element_error(child->name, resource->pool);
          if (strcmp(cdata, "no") == 0)
            text_deltas = FALSE;
        }
    }

  /* If a target revision wasn't requested, or the requested target
     revision was invalid, just update to HEAD as of the moment we
     queried the youngest revision.  Otherwise, at least make sure the
     request makes sense in light of that youngest revision
     number.  */
  if (! SVN_IS_VALID_REVNUM(revnum))
    {
      revnum = youngest;
    }
  else
    {
      derr = validate_input_revision(revnum, youngest, "target revision",
                                     resource);
      if (derr)
        return derr;
    }

  if (!saw_depth && !saw_recursive && (requested_depth == svn_depth_unknown))
    requested_depth = svn_depth_infinity;

  /* If the client never sent a <src-path> element, it's old and
     sending a style of report that we no longer allow. */
  if (! src_path)
    {
      return dav_svn__new_error_tag
        (resource->pool, HTTP_BAD_REQUEST, 0,
         "The request did not contain the '<src-path>' element.\n"
         "This may indicate that your client is too old.",
         SVN_DAV_ERROR_NAMESPACE,
         SVN_DAV_ERROR_TAG);
    }

  uc.svndiff_version = resource->info->svndiff_version;
  uc.resource = resource;
  uc.output = output;
  uc.anchor = src_path;
  uc.target = target;
  uc.bb = apr_brigade_create(resource->pool, output->c->bucket_alloc);
  uc.pathmap = NULL;
  if (dst_path) /* we're doing a 'switch' */
    {
      if (*target)
        {
          /* if the src is split into anchor/target, so must the
             telescoping dst_path be. */
          uc.dst_path = svn_fspath__dirname(dst_path, resource->pool);

          /* Also, the svn_repos_dir_delta2() is going to preserve our
             target's name, so we need a pathmap entry for that. */
          if (! uc.pathmap)
            uc.pathmap = apr_hash_make(resource->pool);
          add_to_path_map(uc.pathmap,
                          svn_fspath__join(src_path, target, resource->pool),
                          dst_path);
        }
      else
        {
          uc.dst_path = dst_path;
        }
    }
  else  /* we're doing an update, so src and dst are the same. */
    uc.dst_path = uc.anchor;

  /* Get the root of the revision we want to update to. This will be used
     to generated stable id values. */
  if ((serr = svn_fs_revision_root(&uc.rev_root, repos->fs,
                                   revnum, resource->pool)))
    {
      return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                  "The revision root could not be created.",
                                  resource->pool);
    }

  /* If the client did *not* request 'send-all' mode, then we will be
     sending only a "skelta" of the difference, which will not need to
     contain actual text deltas. */
  if (! uc.send_all)
    text_deltas = FALSE;

  /* When we call svn_repos_finish_report, it will ultimately run
     dir_delta() between REPOS_PATH/TARGET and TARGET_PATH.  In the
     case of an update or status, these paths should be identical.  In
     the case of a switch, they should be different. */
  editor = svn_delta_default_editor(resource->pool);
  editor->set_target_revision = upd_set_target_revision;
  editor->open_root = upd_open_root;
  editor->delete_entry = upd_delete_entry;
  editor->add_directory = upd_add_directory;
  editor->open_directory = upd_open_directory;
  editor->change_dir_prop = upd_change_xxx_prop;
  editor->close_directory = upd_close_directory;
  editor->absent_directory = upd_absent_directory;
  editor->add_file = upd_add_file;
  editor->open_file = upd_open_file;
  editor->apply_textdelta = upd_apply_textdelta;
  editor->change_file_prop = upd_change_xxx_prop;
  editor->close_file = upd_close_file;
  editor->absent_file = upd_absent_file;
  editor->close_edit = upd_close_edit;
  if ((serr = svn_repos_begin_report2(&rbaton, revnum,
                                      repos->repos,
                                      src_path, target,
                                      dst_path,
                                      text_deltas,
                                      requested_depth,
                                      ignore_ancestry,
                                      send_copyfrom_args,
                                      editor, &uc,
                                      dav_svn__authz_read_func(&arb),
                                      &arb,
                                      resource->pool)))
    {
      return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                  "The state report gatherer could not be "
                                  "created.",
                                  resource->pool);
    }

  /* scan the XML doc for state information */
  for (child = doc->root->first_child; child != NULL; child = child->next)
    if (child->ns == ns)
      {
        /* Clear our subpool. */
        svn_pool_clear(subpool);

        if (strcmp(child->name, "entry") == 0)
          {
            const char *path;
            svn_revnum_t rev = SVN_INVALID_REVNUM;
            svn_boolean_t saw_rev = FALSE;
            const char *linkpath = NULL;
            const char *locktoken = NULL;
            svn_boolean_t start_empty = FALSE;
            apr_xml_attr *this_attr = child->attr;
            /* Default to infinity, for old clients that don't send depth. */
            svn_depth_t depth = svn_depth_infinity;

            entry_counter++;

            while (this_attr)
              {
                if (strcmp(this_attr->name, "rev") == 0)
                  {
                    rev = SVN_STR_TO_REV(this_attr->value);
                    saw_rev = TRUE;
                    if ((derr = validate_input_revision(rev, youngest,
                                                        "reported revision",
                                                        resource)))
                      return derr;
                  }
                else if (strcmp(this_attr->name, "depth") == 0)
                  depth = svn_depth_from_word(this_attr->value);
                else if (strcmp(this_attr->name, "linkpath") == 0)
                  linkpath = this_attr->value;
                else if (strcmp(this_attr->name, "start-empty") == 0)
                  start_empty = entry_is_empty = TRUE;
                else if (strcmp(this_attr->name, "lock-token") == 0)
                  locktoken = this_attr->value;

                this_attr = this_attr->next;
              }

            /* we require the `rev' attribute for this to make sense */
            if (! saw_rev)
              {
                serr = svn_error_create(SVN_ERR_XML_ATTRIB_NOT_FOUND,
                                        NULL, "Missing XML attribute: rev");
                derr = dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                            "A failure occurred while "
                                            "recording one of the items of "
                                            "working copy state.",
                                            resource->pool);
                goto cleanup;
              }

            /* get cdata, stripping whitespace */
            path = dav_xml_get_cdata(child, subpool, 0);

            /* determine the "from rev" for revision range ops */
            if (strcmp(path, "") == 0)
              from_revnum = rev;

            if (! linkpath)
              serr = svn_repos_set_path3(rbaton, path, rev, depth,
                                         start_empty, locktoken, subpool);
            else
              serr = svn_repos_link_path3(rbaton, path, linkpath, rev, depth,
                                          start_empty, locktoken, subpool);
            if (serr != NULL)
              {
                derr = dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                            "A failure occurred while "
                                            "recording one of the items of "
                                            "working copy state.",
                                            resource->pool);
                goto cleanup;
              }

            /* now, add this path to our path map, but only if we are
               doing a regular update (not a `switch') */
            if (linkpath && (! dst_path))
              {
                const char *this_path;
                if (! uc.pathmap)
                  uc.pathmap = apr_hash_make(resource->pool);
                this_path = svn_fspath__join(src_path, target, resource->pool);
                this_path = svn_fspath__join(this_path, path, resource->pool);
                add_to_path_map(uc.pathmap, this_path, linkpath);
              }
          }
        else if (strcmp(child->name, "missing") == 0)
          {
            /* get cdata, stripping whitespace */
            const char *path = dav_xml_get_cdata(child, subpool, 0);
            serr = svn_repos_delete_path(rbaton, path, subpool);
            if (serr != NULL)
              {
                derr = dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                            "A failure occurred while "
                                            "recording one of the (missing) "
                                            "items of working copy state.",
                                            resource->pool);
                goto cleanup;
              }
          }
      }

  /* Try to deduce what sort of client command is being run, then
     make this guess available to apache's logging subsystem. */
  {
    const char *action, *spath;

    if (target)
      spath = svn_fspath__join(src_path, target, resource->pool);
    else
      spath = src_path;

    /* If a second path was passed to svn_repos_dir_delta2(), then it
       must have been switch, diff, or merge.  */
    if (dst_path)
      {
        /* diff/merge don't ask for inline text-deltas. */
        if (uc.send_all)
          action = svn_log__switch(spath, dst_path, revnum,
                                   requested_depth, resource->pool);
        else
          action = svn_log__diff(spath, from_revnum, dst_path, revnum,
                                 requested_depth, ignore_ancestry,
                                 resource->pool);
      }

    /* Otherwise, it must be checkout, export, update, or status -u. */
    else
      {
        /* svn_client_checkout() creates a single root directory, then
           reports it (and it alone) to the server as being empty. */
        if (entry_counter == 1 && entry_is_empty)
          action = svn_log__checkout(spath, revnum, requested_depth,
                                     resource->pool);
        else
          {
            if (text_deltas)
              action = svn_log__update(spath, revnum, requested_depth,
                                       send_copyfrom_args,
                                       resource->pool);
            else
              action = svn_log__status(spath, revnum, requested_depth,
                                       resource->pool);
          }
      }

    dav_svn__operational_log(resource->info, action);
  }

  /* this will complete the report, and then drive our editor to generate
     the response to the client. */
  serr = svn_repos_finish_report(rbaton, resource->pool);

  /* Whether svn_repos_finish_report returns an error or not we can no
     longer abort this report as the file has been closed. */
  rbaton = NULL;

  if (serr)
    {
      derr = dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                  "A failure occurred while "
                                  "driving the update report editor",
                                  resource->pool);
      goto cleanup;
    }

  /* ### Temporarily disable resource_walks for single-file switch
     operations.  It isn't strictly necessary. */
  if (dst_path && resource_walk)
    {
      /* Sanity check: if we switched a file, we can't do a resource
         walk.  dir_delta would choke if we pass a filepath as the
         'target'.  Also, there's no need to do the walk, since the
         new vsn-rsc-url was already in the earlier part of the report. */
      svn_node_kind_t dst_kind;
      if ((serr = svn_fs_check_path(&dst_kind, uc.rev_root, dst_path,
                                    resource->pool)))
        {
          derr = dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                      "Failed checking destination path kind",
                                      resource->pool);
          goto cleanup;
        }
      if (dst_kind != svn_node_dir)
        resource_walk = FALSE;
    }

  /* The potential "resource walk" part of the update-report. */
  if (dst_path && resource_walk)  /* this was a 'switch' operation */
    {
      /* send a second embedded <S:resource-walk> tree that contains
         the new vsn-rsc-urls for the switched dir.  this walk
         contains essentially nothing but <add> tags. */
      svn_fs_root_t *zero_root;
      serr = svn_fs_revision_root(&zero_root, repos->fs, 0,
                                  resource->pool);
      if (serr)
        {
          derr = dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                      "Failed to find the revision root",
                                      resource->pool);
          goto cleanup;
        }

      serr = dav_svn__brigade_puts(uc.bb, uc.output,
                                   "<S:resource-walk>" DEBUG_CR);
      if (serr)
        {
          derr = dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                      "Unable to begin resource walk",
                                      resource->pool);
          goto cleanup;
        }

      uc.resource_walk = TRUE;

      /* Compare subtree DST_PATH within a pristine revision to
         revision 0.  This should result in nothing but 'add' calls
         to the editor. */
      serr = svn_repos_dir_delta2(zero_root, "", target,
                                  uc.rev_root, dst_path,
                                  /* re-use the editor */
                                  editor, &uc,
                                  dav_svn__authz_read_func(&arb),
                                  &arb, FALSE /* text-deltas */,
                                  requested_depth,
                                  TRUE /* entryprops */,
                                  FALSE /* ignore-ancestry */,
                                  resource->pool);

      if (serr)
        {
          derr = dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                      "Resource walk failed.",
                                      resource->pool);
          goto cleanup;
        }

      serr = dav_svn__brigade_puts(uc.bb, uc.output,
                                   "</S:resource-walk>" DEBUG_CR);
      if (serr)
        {
          derr = dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                      "Unable to complete resource walk.",
                                      resource->pool);
          goto cleanup;
        }
    }

  /* Close the report body, unless some error prevented it from being
     started in the first place. */
  if (uc.started_update)
    {
      if ((serr = dav_svn__brigade_puts(uc.bb, uc.output,
                                        "</S:update-report>" DEBUG_CR)))
        {
          derr = dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                      "Unable to complete update report.",
                                      resource->pool);
          goto cleanup;
        }
    }

 cleanup:

  /* If an error was produced EITHER by the dir_delta drive or the
     resource-walker, abort the report. */
  if (derr && rbaton)
    svn_error_clear(svn_repos_abort_report(rbaton, resource->pool));

  /* Destroy our subpool. */
  svn_pool_destroy(subpool);

  return dav_svn__final_flush_or_error(resource->info->r, uc.bb, output,
                                       derr, resource->pool);
}

/*
 * replay.c: mod_dav_svn REPORT handler for replaying revisions
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
#include "svn_path.h"
#include "svn_dav.h"
#include "svn_props.h"
#include "private/svn_log.h"

#include "../dav_svn.h"


typedef struct edit_baton_t {
  apr_bucket_brigade *bb;
  ap_filter_t *output;
  svn_boolean_t started;
  svn_boolean_t sending_textdelta;
} edit_baton_t;



/*** Helper Functions ***/

static svn_error_t *
maybe_start_report(edit_baton_t *eb)
{
  if (! eb->started)
    {
      SVN_ERR(dav_svn__brigade_puts(eb->bb, eb->output,
                                    DAV_XML_HEADER DEBUG_CR
                                    "<S:editor-report xmlns:S=\""
                                    SVN_XML_NAMESPACE "\">" DEBUG_CR));
      eb->started = TRUE;
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
end_report(edit_baton_t *eb)
{
  SVN_ERR(dav_svn__brigade_puts(eb->bb, eb->output,
                                "</S:editor-report>" DEBUG_CR));

  return SVN_NO_ERROR;
}


static svn_error_t *
maybe_close_textdelta(edit_baton_t *eb)
{
  if (eb->sending_textdelta)
    {
      SVN_ERR(dav_svn__brigade_puts(eb->bb, eb->output,
                                    "</S:apply-textdelta>" DEBUG_CR));
      eb->sending_textdelta = FALSE;
    }

  return SVN_NO_ERROR;
}


static svn_error_t *
add_file_or_directory(const char *file_or_directory,
                      const char *path,
                      edit_baton_t *eb,
                      const char *copyfrom_path,
                      svn_revnum_t copyfrom_rev,
                      apr_pool_t *pool,
                      void **added_baton)
{
  const char *qname = apr_xml_quote_string(pool, path, 1);
  const char *qcopy =
    copyfrom_path ? apr_xml_quote_string(pool, copyfrom_path, 1) : NULL;

  SVN_ERR(maybe_close_textdelta(eb));

  *added_baton = (void *)eb;

  if (! copyfrom_path)
    SVN_ERR(dav_svn__brigade_printf(eb->bb, eb->output,
                                    "<S:add-%s name=\"%s\"/>" DEBUG_CR,
                                    file_or_directory, qname));
  else
    SVN_ERR(dav_svn__brigade_printf(eb->bb, eb->output,
                                    "<S:add-%s name=\"%s\" "
                                    "copyfrom-path=\"%s\" "
                                    "copyfrom-rev=\"%ld\"/>" DEBUG_CR,
                                    file_or_directory, qname,
                                    qcopy, copyfrom_rev));

  return SVN_NO_ERROR;
}

static svn_error_t *
open_file_or_directory(const char *file_or_directory,
                       const char *path,
                       edit_baton_t *eb,
                       svn_revnum_t base_revision,
                       apr_pool_t *pool,
                       void **opened_baton)
{
  const char *qname = apr_xml_quote_string(pool, path, 1);
  SVN_ERR(maybe_close_textdelta(eb));
  *opened_baton = (void *)eb;
  return dav_svn__brigade_printf(eb->bb, eb->output,
                                 "<S:open-%s name=\"%s\" rev=\"%ld\"/>"
                                 DEBUG_CR,
                                 file_or_directory, qname, base_revision);
}


static svn_error_t *
change_file_or_dir_prop(const char *file_or_dir,
                        edit_baton_t *eb,
                        const char *name,
                        const svn_string_t *value,
                        apr_pool_t *pool)
{
  const char *qname = apr_xml_quote_string(pool, name, 1);

  SVN_ERR(maybe_close_textdelta(eb));

  if (value)
    {
      const svn_string_t *enc_value =
        svn_base64_encode_string2(value, TRUE, pool);

      /* Some versions of apr_brigade_vprintf() have a buffer overflow
         bug that can be triggered by just the wrong size of a large
         property value.  The bug has been fixed (see
         http://svn.apache.org/viewvc?view=rev&revision=768417), but
         we need a workaround for the buggy APR versions, so we write
         our potentially large block of property data using a
         different underlying function. */
      SVN_ERR(dav_svn__brigade_printf(eb->bb, eb->output,
                                      "<S:change-%s-prop name=\"%s\">",
                                      file_or_dir, qname));
      SVN_ERR(dav_svn__brigade_write(eb->bb, eb->output,
                                     enc_value->data, enc_value->len));
      SVN_ERR(dav_svn__brigade_printf(eb->bb, eb->output,
                                      "</S:change-%s-prop>" DEBUG_CR,
                                      file_or_dir));
    }
  else
    {
      SVN_ERR(dav_svn__brigade_printf
                (eb->bb, eb->output,
                 "<S:change-%s-prop name=\"%s\" del=\"true\"/>" DEBUG_CR,
                 file_or_dir, qname));
    }
  return SVN_NO_ERROR;
}



/*** Editor Implementation ***/


static svn_error_t *
set_target_revision(void *edit_baton,
                    svn_revnum_t target_revision,
                    apr_pool_t *pool)
{
  edit_baton_t *eb = edit_baton;
  SVN_ERR(maybe_start_report(eb));
  return dav_svn__brigade_printf(eb->bb, eb->output,
                                 "<S:target-revision rev=\"%ld\"/>" DEBUG_CR,
                                 target_revision);
}


static svn_error_t *
open_root(void *edit_baton,
          svn_revnum_t base_revision,
          apr_pool_t *pool,
          void **root_baton)
{
  edit_baton_t *eb = edit_baton;
  *root_baton = edit_baton;
  SVN_ERR(maybe_start_report(eb));
  return dav_svn__brigade_printf(eb->bb, eb->output,
                                 "<S:open-root rev=\"%ld\"/>" DEBUG_CR,
                                 base_revision);
}


static svn_error_t *
delete_entry(const char *path,
             svn_revnum_t revision,
             void *parent_baton,
             apr_pool_t *pool)
{
  edit_baton_t *eb = parent_baton;
  const char *qname = apr_xml_quote_string(pool, path, 1);
  SVN_ERR(maybe_close_textdelta(eb));
  return dav_svn__brigade_printf(eb->bb, eb->output,
                                 "<S:delete-entry name=\"%s\" rev=\"%ld\"/>"
                                 DEBUG_CR,
                                 qname, revision);
}


static svn_error_t *
add_directory(const char *path,
              void *parent_baton,
              const char *copyfrom_path,
              svn_revnum_t copyfrom_rev,
              apr_pool_t *pool,
              void **child_baton)
{
  return add_file_or_directory("directory", path, parent_baton,
                               copyfrom_path, copyfrom_rev, pool, child_baton);
}


static svn_error_t *
open_directory(const char *path,
               void *parent_baton,
               svn_revnum_t base_revision,
               apr_pool_t *pool,
               void **child_baton)
{
  return open_file_or_directory("directory", path, parent_baton,
                                base_revision, pool, child_baton);
}


static svn_error_t *
change_dir_prop(void *baton,
                const char *name,
                const svn_string_t *value,
                apr_pool_t *pool)
{
  return change_file_or_dir_prop("dir", baton, name, value, pool);
}


static svn_error_t *
add_file(const char *path,
         void *parent_baton,
         const char *copyfrom_path,
         svn_revnum_t copyfrom_rev,
         apr_pool_t *pool,
         void **file_baton)
{
  return add_file_or_directory("file", path, parent_baton,
                               copyfrom_path, copyfrom_rev, pool, file_baton);
}


static svn_error_t *
open_file(const char *path,
          void *parent_baton,
          svn_revnum_t base_revision,
          apr_pool_t *pool,
          void **file_baton)
{
  return open_file_or_directory("file", path, parent_baton,
                                base_revision, pool, file_baton);
}


static svn_error_t *
change_file_prop(void *baton,
                 const char *name,
                 const svn_string_t *value,
                 apr_pool_t *pool)
{
  return change_file_or_dir_prop("file", baton, name, value, pool);
}


static svn_error_t *
apply_textdelta(void *file_baton,
                const char *base_checksum,
                apr_pool_t *pool,
                svn_txdelta_window_handler_t *handler,
                void **handler_baton)
{
  edit_baton_t *eb = file_baton;

  SVN_ERR(dav_svn__brigade_puts(eb->bb, eb->output, "<S:apply-textdelta"));

  if (base_checksum)
    SVN_ERR(dav_svn__brigade_printf(eb->bb, eb->output, " checksum=\"%s\">",
                                    base_checksum));
  else
    SVN_ERR(dav_svn__brigade_puts(eb->bb, eb->output, ">"));

  svn_txdelta_to_svndiff3(handler,
                          handler_baton,
                          dav_svn__make_base64_output_stream(eb->bb,
                                                             eb->output,
                                                             pool),
                          0,
                          dav_svn__get_compression_level(),
                          pool);

  eb->sending_textdelta = TRUE;

  return SVN_NO_ERROR;
}


static svn_error_t *
close_file(void *file_baton, const char *text_checksum, apr_pool_t *pool)
{
  edit_baton_t *eb = file_baton;
  SVN_ERR(maybe_close_textdelta(eb));
  SVN_ERR(dav_svn__brigade_puts(eb->bb, eb->output, "<S:close-file"));

  if (text_checksum)
    SVN_ERR(dav_svn__brigade_printf(eb->bb, eb->output,
                                    " checksum=\"%s\"/>" DEBUG_CR,
                                    text_checksum));
  else
    SVN_ERR(dav_svn__brigade_puts(eb->bb, eb->output, "/>" DEBUG_CR));

  return SVN_NO_ERROR;
}


static svn_error_t *
close_directory(void *dir_baton, apr_pool_t *pool)
{
  edit_baton_t *eb = dir_baton;
  return dav_svn__brigade_puts(eb->bb, eb->output,
                               "<S:close-directory/>" DEBUG_CR);
}


static void
make_editor(const svn_delta_editor_t **editor,
            void **edit_baton,
            apr_bucket_brigade *bb,
            ap_filter_t *output,
            apr_pool_t *pool)
{
  edit_baton_t *eb = apr_pcalloc(pool, sizeof(*eb));
  svn_delta_editor_t *e = svn_delta_default_editor(pool);

  eb->bb = bb;
  eb->output = output;
  eb->started = FALSE;
  eb->sending_textdelta = FALSE;

  e->set_target_revision = set_target_revision;
  e->open_root = open_root;
  e->delete_entry = delete_entry;
  e->add_directory = add_directory;
  e->open_directory = open_directory;
  e->change_dir_prop = change_dir_prop;
  e->add_file = add_file;
  e->open_file = open_file;
  e->apply_textdelta = apply_textdelta;
  e->change_file_prop = change_file_prop;
  e->close_file = close_file;
  e->close_directory = close_directory;

  *editor = e;
  *edit_baton = eb;
}


static dav_error *
malformed_element_error(const char *tagname, apr_pool_t *pool)
{
  return dav_svn__new_error_tag(pool, HTTP_BAD_REQUEST, 0,
                                apr_pstrcat(pool,
                                            "The request's '", tagname,
                                            "' element is malformed; there "
                                            "is a problem with the client.",
                                            (char *)NULL),
                                SVN_DAV_ERROR_NAMESPACE, SVN_DAV_ERROR_TAG);
}


dav_error *
dav_svn__replay_report(const dav_resource *resource,
                       const apr_xml_doc *doc,
                       ap_filter_t *output)
{
  dav_error *derr = NULL;
  svn_revnum_t low_water_mark = SVN_INVALID_REVNUM;
  svn_revnum_t rev = SVN_INVALID_REVNUM;
  const svn_delta_editor_t *editor;
  svn_boolean_t send_deltas = TRUE;
  dav_svn__authz_read_baton arb;
  const char *base_dir = resource->info->repos_path;
  apr_bucket_brigade *bb;
  apr_xml_elem *child;
  svn_fs_root_t *root;
  svn_error_t *err;
  void *edit_baton;
  int ns;

  /* The request won't have a repos_path if it's for the root. */
  if (! base_dir)
    base_dir = "";

  arb.r = resource->info->r;
  arb.repos = resource->info->repos;

  ns = dav_svn__find_ns(doc->namespaces, SVN_XML_NAMESPACE);
  if (ns == -1)
    return dav_svn__new_error_tag(resource->pool, HTTP_BAD_REQUEST, 0,
                                  "The request does not contain the 'svn:' "
                                  "namespace, so it is not going to have an "
                                  "svn:revision element. That element is "
                                  "required.",
                                  SVN_DAV_ERROR_NAMESPACE,
                                  SVN_DAV_ERROR_TAG);

  for (child = doc->root->first_child; child != NULL; child = child->next)
    {
      if (child->ns == ns)
        {
          const char *cdata;

          if (strcmp(child->name, "revision") == 0)
            {
              cdata = dav_xml_get_cdata(child, resource->pool, 1);
              if (! cdata)
                return malformed_element_error("revision", resource->pool);
              rev = SVN_STR_TO_REV(cdata);
            }
          else if (strcmp(child->name, "low-water-mark") == 0)
            {
              cdata = dav_xml_get_cdata(child, resource->pool, 1);
              if (! cdata)
                return malformed_element_error("low-water-mark",
                                               resource->pool);
              low_water_mark = SVN_STR_TO_REV(cdata);
            }
          else if (strcmp(child->name, "send-deltas") == 0)
            {
              apr_int64_t parsed_val;

              cdata = dav_xml_get_cdata(child, resource->pool, 1);
              if (! cdata)
                return malformed_element_error("send-deltas", resource->pool);
              err = svn_cstring_strtoi64(&parsed_val, cdata, 0, 1, 10);
              if (err)
                {
                  svn_error_clear(err);
                  return malformed_element_error("send-deltas", resource->pool);
                }
              send_deltas = parsed_val ? TRUE : FALSE;
            }
        }
    }

  if (! SVN_IS_VALID_REVNUM(rev))
    return dav_svn__new_error_tag
             (resource->pool, HTTP_BAD_REQUEST, 0,
              "Request was missing the revision argument.",
              SVN_DAV_ERROR_NAMESPACE, SVN_DAV_ERROR_TAG);

  if (! SVN_IS_VALID_REVNUM(low_water_mark))
    return dav_svn__new_error_tag
             (resource->pool, HTTP_BAD_REQUEST, 0,
              "Request was missing the low-water-mark argument.",
              SVN_DAV_ERROR_NAMESPACE, SVN_DAV_ERROR_TAG);

  bb = apr_brigade_create(resource->pool, output->c->bucket_alloc);

  if ((err = svn_fs_revision_root(&root, resource->info->repos->fs, rev,
                                  resource->pool)))
    {
      derr = dav_svn__convert_err(err, HTTP_INTERNAL_SERVER_ERROR,
                                  "Couldn't retrieve revision root",
                                  resource->pool);
      goto cleanup;
    }

  make_editor(&editor, &edit_baton, bb, output, resource->pool);

  if ((err = svn_repos_replay2(root, base_dir, low_water_mark,
                               send_deltas, editor, edit_baton,
                               dav_svn__authz_read_func(&arb), &arb,
                               resource->pool)))
    {
      derr = dav_svn__convert_err(err, HTTP_INTERNAL_SERVER_ERROR,
                                  "Problem replaying revision",
                                  resource->pool);
      goto cleanup;
    }

  if ((err = end_report(edit_baton)))
    {
      derr = dav_svn__convert_err(err, HTTP_INTERNAL_SERVER_ERROR,
                                  "Problem closing editor drive",
                                  resource->pool);
      goto cleanup;
    }

 cleanup:
  dav_svn__operational_log(resource->info,
                           svn_log__replay(base_dir, rev,
                                           resource->info->r->pool));

  /* Flush the brigade. */
  return dav_svn__final_flush_or_error(resource->info->r, bb, output,
                                       derr, resource->pool);
}

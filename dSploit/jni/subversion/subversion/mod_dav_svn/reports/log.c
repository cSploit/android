/*
 * log.c: mod_dav_svn REPORT handler for querying revision log info
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

#include <mod_dav.h>

#include "svn_repos.h"
#include "svn_string.h"
#include "svn_types.h"
#include "svn_base64.h"
#include "svn_xml.h"
#include "svn_path.h"
#include "svn_dav.h"
#include "svn_pools.h"
#include "svn_props.h"

#include "private/svn_log.h"
#include "private/svn_fspath.h"

#include "../dav_svn.h"


struct log_receiver_baton
{
  /* this buffers the output for a bit and is automatically flushed,
     at appropriate times, by the Apache filter system. */
  apr_bucket_brigade *bb;

  /* where to deliver the output */
  ap_filter_t *output;

  /* Whether we've written the <S:log-report> header.  Allows for lazy
     writes to support mod_dav-based error handling. */
  svn_boolean_t needs_header;

  /* How deep we are in the log message tree.  We only need to surpress the
     SVN_INVALID_REVNUM message if the stack_depth is 0. */
  int stack_depth;

  /* whether the client requested any custom revprops */
  svn_boolean_t requested_custom_revprops;

  /* whether the client can handle encoded binary property values */
  svn_boolean_t encode_binary_props;
};


/* If LRB->needs_header is true, send the "<S:log-report>" start
   element and set LRB->needs_header to zero.  Else do nothing.
   This is basically duplicated in file_revs.c.  Consider factoring if
   duplicating again. */
static svn_error_t *
maybe_send_header(struct log_receiver_baton *lrb)
{
  if (lrb->needs_header)
    {
      SVN_ERR(dav_svn__brigade_puts(lrb->bb, lrb->output,
                                    DAV_XML_HEADER DEBUG_CR
                                    "<S:log-report xmlns:S=\""
                                    SVN_XML_NAMESPACE "\" "
                                    "xmlns:D=\"DAV:\">" DEBUG_CR));
      lrb->needs_header = FALSE;
    }
  return SVN_NO_ERROR;
}


/* This implements `svn_log_entry_receiver_t'.
   BATON is a `struct log_receiver_baton *'.  */
static svn_error_t *
log_receiver(void *baton,
             svn_log_entry_t *log_entry,
             apr_pool_t *pool)
{
  struct log_receiver_baton *lrb = baton;
  apr_pool_t *iterpool = svn_pool_create(pool);

  SVN_ERR(maybe_send_header(lrb));

  if (log_entry->revision == SVN_INVALID_REVNUM)
    {
      /* If the stack depth is zero, we've seen the last revision, so don't
         send it, just return.  The footer will be sent later. */
      if (lrb->stack_depth == 0)
        return SVN_NO_ERROR;
      else
        lrb->stack_depth--;
    }

  SVN_ERR(dav_svn__brigade_printf(lrb->bb, lrb->output,
                                  "<S:log-item>" DEBUG_CR "<D:version-name>%ld"
                                  "</D:version-name>" DEBUG_CR,
                                  log_entry->revision));

  if (log_entry->revprops)
    {
      apr_hash_index_t *hi;
      for (hi = apr_hash_first(pool, log_entry->revprops);
           hi != NULL;
           hi = apr_hash_next(hi))
        {
          char *name;
          void *val;
          const svn_string_t *value;
          const char *encoding_str = "";

          svn_pool_clear(iterpool);
          apr_hash_this(hi, (void *)&name, NULL, &val);
          value = val;

          /* If the client is okay with us encoding binary (or really,
             any non-XML-safe) property values, do so as necessary. */
          if (lrb->encode_binary_props)
            {
              if (! svn_xml_is_xml_safe(value->data, value->len))
                {
                  value = svn_base64_encode_string2(value, TRUE, iterpool);
                  encoding_str = " encoding=\"base64\"";
                }
            }

          if (strcmp(name, SVN_PROP_REVISION_AUTHOR) == 0)
            SVN_ERR(dav_svn__brigade_printf
                    (lrb->bb, lrb->output,
                     "<D:creator-displayname%s>%s</D:creator-displayname>"
                     DEBUG_CR, encoding_str,
                     apr_xml_quote_string(iterpool, value->data, 0)));
          else if (strcmp(name, SVN_PROP_REVISION_DATE) == 0)
            /* ### this should be DAV:creation-date, but we need to format
               ### that date a bit differently */
            SVN_ERR(dav_svn__brigade_printf
                    (lrb->bb, lrb->output,
                     "<S:date%s>%s</S:date>" DEBUG_CR, encoding_str,
                     apr_xml_quote_string(iterpool, value->data, 0)));
          else if (strcmp(name, SVN_PROP_REVISION_LOG) == 0)
            SVN_ERR(dav_svn__brigade_printf
                    (lrb->bb, lrb->output,
                     "<D:comment%s>%s</D:comment>" DEBUG_CR, encoding_str,
                     apr_xml_quote_string(pool,
                                          svn_xml_fuzzy_escape(value->data,
                                                               iterpool), 0)));
          else
            SVN_ERR(dav_svn__brigade_printf
                    (lrb->bb, lrb->output,
                     "<S:revprop name=\"%s\"%s>%s</S:revprop>" DEBUG_CR,
                     apr_xml_quote_string(iterpool, name, 0), encoding_str,
                     apr_xml_quote_string(iterpool, value->data, 0)));
        }
    }

  if (log_entry->has_children)
    {
      SVN_ERR(dav_svn__brigade_puts(lrb->bb, lrb->output, "<S:has-children/>"));
      lrb->stack_depth++;
    }

  if (log_entry->subtractive_merge)
    SVN_ERR(dav_svn__brigade_puts(lrb->bb, lrb->output,
                                  "<S:subtractive-merge/>"));

  if (log_entry->changed_paths2)
    {
      apr_hash_index_t *hi;
      char *path;

      for (hi = apr_hash_first(pool, log_entry->changed_paths2);
           hi != NULL;
           hi = apr_hash_next(hi))
        {
          void *val;
          svn_log_changed_path2_t *log_item;
          const char *close_element = NULL;

          svn_pool_clear(iterpool);
          apr_hash_this(hi, (void *) &path, NULL, &val);
          log_item = val;

          /* ### todo: is there a D: namespace equivalent for
             `changed-path'?  Should use it if so. */
          switch (log_item->action)
            {
            case 'A':
              if (log_item->copyfrom_path
                  && SVN_IS_VALID_REVNUM(log_item->copyfrom_rev))
                SVN_ERR(dav_svn__brigade_printf
                        (lrb->bb, lrb->output,
                         "<S:added-path copyfrom-path=\"%s\""
                         " copyfrom-rev=\"%ld\"",
                         apr_xml_quote_string(iterpool,
                                              log_item->copyfrom_path,
                                              1), /* escape quotes */
                         log_item->copyfrom_rev));
              else
                SVN_ERR(dav_svn__brigade_puts(lrb->bb, lrb->output,
                                              "<S:added-path"));

              close_element = "S:added-path";
              break;

            case 'R':
              if (log_item->copyfrom_path
                  && SVN_IS_VALID_REVNUM(log_item->copyfrom_rev))
                SVN_ERR(dav_svn__brigade_printf
                        (lrb->bb, lrb->output,
                         "<S:replaced-path copyfrom-path=\"%s\""
                         " copyfrom-rev=\"%ld\"",
                         apr_xml_quote_string(iterpool,
                                              log_item->copyfrom_path,
                                              1), /* escape quotes */
                         log_item->copyfrom_rev));
              else
                SVN_ERR(dav_svn__brigade_puts(lrb->bb, lrb->output,
                                              "<S:replaced-path"));

              close_element = "S:replaced-path";
              break;

            case 'D':
              SVN_ERR(dav_svn__brigade_puts(lrb->bb, lrb->output,
                                            "<S:deleted-path"));
              close_element = "S:deleted-path";
              break;

            case 'M':
              SVN_ERR(dav_svn__brigade_puts(lrb->bb, lrb->output,
                                            "<S:modified-path"));
              close_element = "S:modified-path";
              break;

            default:
              break;
            }

          /* If we need to close the element, then send the attributes
             that apply to all changed items and then close the element. */
          if (close_element)
            SVN_ERR(dav_svn__brigade_printf
                    (lrb->bb, lrb->output,
                     " node-kind=\"%s\""
                     " text-mods=\"%s\""
                     " prop-mods=\"%s\">%s</%s>" DEBUG_CR,
                     svn_node_kind_to_word(log_item->node_kind),
                     svn_tristate__to_word(log_item->text_modified),
                     svn_tristate__to_word(log_item->props_modified),
                     apr_xml_quote_string(iterpool, path, 0),
                     close_element));
        }
    }

  svn_pool_destroy(iterpool);

  SVN_ERR(dav_svn__brigade_puts(lrb->bb, lrb->output,
                                "</S:log-item>" DEBUG_CR));

  return SVN_NO_ERROR;
}


dav_error *
dav_svn__log_report(const dav_resource *resource,
                    const apr_xml_doc *doc,
                    ap_filter_t *output)
{
  svn_error_t *serr;
  dav_error *derr = NULL;
  apr_xml_elem *child;
  struct log_receiver_baton lrb;
  dav_svn__authz_read_baton arb;
  const dav_svn_repos *repos = resource->info->repos;
  const char *target = NULL;
  int limit = 0;
  int ns;
  svn_boolean_t seen_revprop_element;

  /* These get determined from the request document. */
  svn_revnum_t start = SVN_INVALID_REVNUM;   /* defaults to HEAD */
  svn_revnum_t end = SVN_INVALID_REVNUM;     /* defaults to HEAD */
  svn_boolean_t discover_changed_paths = FALSE;      /* off by default */
  svn_boolean_t strict_node_history = FALSE;         /* off by default */
  svn_boolean_t include_merged_revisions = FALSE;    /* off by default */
  apr_array_header_t *revprops = apr_array_make(resource->pool, 3,
                                                sizeof(const char *));
  apr_array_header_t *paths
    = apr_array_make(resource->pool, 1, sizeof(const char *));

  /* Sanity check. */
  ns = dav_svn__find_ns(doc->namespaces, SVN_XML_NAMESPACE);
  if (ns == -1)
    {
      return dav_svn__new_error_tag(resource->pool, HTTP_BAD_REQUEST, 0,
                                    "The request does not contain the 'svn:' "
                                    "namespace, so it is not going to have "
                                    "certain required elements.",
                                    SVN_DAV_ERROR_NAMESPACE,
                                    SVN_DAV_ERROR_TAG);
    }

  /* If this is still FALSE after the loop, we haven't seen either of
     the revprop elements, meaning a pre-1.5 client; we'll return the
     standard author/date/log revprops. */
  seen_revprop_element = FALSE;

  lrb.requested_custom_revprops = FALSE;
  lrb.encode_binary_props = FALSE;
  for (child = doc->root->first_child; child != NULL; child = child->next)
    {
      /* if this element isn't one of ours, then skip it */
      if (child->ns != ns)
        continue;

      if (strcmp(child->name, "start-revision") == 0)
        start = SVN_STR_TO_REV(dav_xml_get_cdata(child, resource->pool, 1));
      else if (strcmp(child->name, "end-revision") == 0)
        end = SVN_STR_TO_REV(dav_xml_get_cdata(child, resource->pool, 1));
      else if (strcmp(child->name, "limit") == 0)
        {
          serr = svn_cstring_atoi(&limit,
                                  dav_xml_get_cdata(child, resource->pool, 1));
          if (serr)
            {
              return dav_svn__convert_err(serr, HTTP_BAD_REQUEST,
                                          "Malformed CDATA in element "
                                          "\"limit\"", resource->pool);
            }
        }
      else if (strcmp(child->name, "discover-changed-paths") == 0)
        discover_changed_paths = TRUE; /* presence indicates positivity */
      else if (strcmp(child->name, "strict-node-history") == 0)
        strict_node_history = TRUE; /* presence indicates positivity */
      else if (strcmp(child->name, "include-merged-revisions") == 0)
        include_merged_revisions = TRUE; /* presence indicates positivity */
      else if (strcmp(child->name, "encode-binary-props") == 0)
        lrb.encode_binary_props = TRUE; /* presence indicates positivity */
      else if (strcmp(child->name, "all-revprops") == 0)
        {
          revprops = NULL; /* presence indicates fetch all revprops */
          seen_revprop_element = lrb.requested_custom_revprops = TRUE;
        }
      else if (strcmp(child->name, "no-revprops") == 0)
        {
          /* presence indicates fetch no revprops */

          seen_revprop_element = lrb.requested_custom_revprops = TRUE;
        }
      else if (strcmp(child->name, "revprop") == 0)
        {
          if (revprops)
            {
              /* We're not fetching all revprops, append to fetch list. */
              const char *name = dav_xml_get_cdata(child, resource->pool, 0);
              APR_ARRAY_PUSH(revprops, const char *) = name;
              if (!lrb.requested_custom_revprops
                  && strcmp(name, SVN_PROP_REVISION_AUTHOR) != 0
                  && strcmp(name, SVN_PROP_REVISION_DATE) != 0
                  && strcmp(name, SVN_PROP_REVISION_LOG) != 0)
                lrb.requested_custom_revprops = TRUE;
            }
          seen_revprop_element = TRUE;
        }
      else if (strcmp(child->name, "path") == 0)
        {
          const char *rel_path = dav_xml_get_cdata(child, resource->pool, 0);
          if ((derr = dav_svn__test_canonical(rel_path, resource->pool)))
            return derr;

          /* Force REL_PATH to be a relative path, not an fspath. */
          rel_path = svn_relpath_canonicalize(rel_path, resource->pool);

          /* Append the REL_PATH to the base FS path to get an
             absolute repository path. */
          target = svn_fspath__join(resource->info->repos_path, rel_path,
                                    resource->pool);
          APR_ARRAY_PUSH(paths, const char *) = target;
        }
      /* else unknown element; skip it */
    }

  if (!seen_revprop_element)
    {
      /* pre-1.5 client */
      APR_ARRAY_PUSH(revprops, const char *) = SVN_PROP_REVISION_AUTHOR;
      APR_ARRAY_PUSH(revprops, const char *) = SVN_PROP_REVISION_DATE;
      APR_ARRAY_PUSH(revprops, const char *) = SVN_PROP_REVISION_LOG;
    }

  /* Build authz read baton */
  arb.r = resource->info->r;
  arb.repos = resource->info->repos;

  /* Build log receiver baton */
  lrb.bb = apr_brigade_create(resource->pool,  /* not the subpool! */
                              output->c->bucket_alloc);
  lrb.output = output;
  lrb.needs_header = TRUE;
  lrb.stack_depth = 0;
  /* lrb.requested_custom_revprops set above */

  /* Our svn_log_entry_receiver_t sends the <S:log-report> header in
     a lazy fashion.  Before writing the first log message, it assures
     that the header has already been sent (checking the needs_header
     flag in our log_receiver_baton structure). */

  /* Send zero or more log items. */
  serr = svn_repos_get_logs4(repos->repos,
                             paths,
                             start,
                             end,
                             limit,
                             discover_changed_paths,
                             strict_node_history,
                             include_merged_revisions,
                             revprops,
                             dav_svn__authz_read_func(&arb),
                             &arb,
                             log_receiver,
                             &lrb,
                             resource->pool);
  if (serr)
    {
      derr = dav_svn__convert_err(serr, HTTP_BAD_REQUEST, serr->message,
                                  resource->pool);
      goto cleanup;
    }

  if ((serr = maybe_send_header(&lrb)))
    {
      derr = dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                  "Error beginning REPORT response.",
                                  resource->pool);
      goto cleanup;
    }

  if ((serr = dav_svn__brigade_puts(lrb.bb, lrb.output,
                                    "</S:log-report>" DEBUG_CR)))
    {
      derr = dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                  "Error ending REPORT response.",
                                  resource->pool);
      goto cleanup;
    }

 cleanup:

  dav_svn__operational_log(resource->info,
                           svn_log__log(paths, start, end, limit,
                                        discover_changed_paths,
                                        strict_node_history,
                                        include_merged_revisions, revprops,
                                        resource->pool));

  return dav_svn__final_flush_or_error(resource->info->r, lrb.bb, output,
                                       derr, resource->pool);
}

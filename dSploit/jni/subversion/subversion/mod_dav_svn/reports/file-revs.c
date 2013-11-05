/*
 * file-revs.c: mod_dav_svn REPORT handler for transmitting a chain of
 *              file revisions
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

#define APR_WANT_STRFUNC
#include <apr_want.h> /* for strcmp() */

#include "svn_types.h"
#include "svn_xml.h"
#include "svn_pools.h"
#include "svn_base64.h"
#include "svn_props.h"
#include "svn_dav.h"

#include "private/svn_log.h"
#include "private/svn_fspath.h"

#include "../dav_svn.h"

struct file_rev_baton {
  /* this buffers the output for a bit and is automatically flushed,
     at appropriate times, by the Apache filter system. */
  apr_bucket_brigade *bb;

  /* where to deliver the output */
  ap_filter_t *output;

  /* Whether we've written the <S:file-revs-report> header.  Allows for lazy
     writes to support mod_dav-based error handling. */
  svn_boolean_t needs_header;

  /* SVNDIFF version to use when sending to client.  */
  int svndiff_version;

  /* Used by the delta iwndow handler. */
  svn_txdelta_window_handler_t window_handler;
  void *window_baton;
};


/* If FRB->needs_header is true, send the "<S:file-revs-report>" start
   tag and set FRB->needs_header to zero.  Else do nothing.
   This is basically duplicated in log.c.  Consider factoring if
   duplicating again. */
static svn_error_t *
maybe_send_header(struct file_rev_baton *frb)
{
  if (frb->needs_header)
    {
      SVN_ERR(dav_svn__brigade_puts(frb->bb, frb->output,
                                    DAV_XML_HEADER DEBUG_CR
                                    "<S:file-revs-report xmlns:S=\""
                                    SVN_XML_NAMESPACE "\" "
                                    "xmlns:D=\"DAV:\">" DEBUG_CR));
      frb->needs_header = FALSE;
    }
  return SVN_NO_ERROR;
}


/* Send a property named NAME with value VAL in an element named ELEM_NAME.
   Quote NAME and base64-encode VAL if necessary. */
static svn_error_t *
send_prop(struct file_rev_baton *frb,
          const char *elem_name,
          const char *name,
          const svn_string_t *val,
          apr_pool_t *pool)
{
  name = apr_xml_quote_string(pool, name, 1);

  if (svn_xml_is_xml_safe(val->data, val->len))
    {
      svn_stringbuf_t *tmp = NULL;
      svn_xml_escape_cdata_string(&tmp, val, pool);
      val = svn_string_create(tmp->data, pool);
      SVN_ERR(dav_svn__brigade_printf(frb->bb, frb->output,
                                      "<S:%s name=\"%s\">%s</S:%s>" DEBUG_CR,
                                      elem_name, name, val->data, elem_name));
    }
  else
    {
      val = svn_base64_encode_string2(val, TRUE, pool);
      SVN_ERR(dav_svn__brigade_printf(frb->bb, frb->output,
                                      "<S:%s name=\"%s\" encoding=\"base64\">"
                                      "%s</S:%s>" DEBUG_CR,
                                      elem_name, name, val->data, elem_name));
    }

  return SVN_NO_ERROR;
}


/* This implements the svn_txdelta_window_handler interface.
   Forward to a more interesting window handler and if we're done, terminate
   the txdelta and file-rev elements. */
static svn_error_t *
delta_window_handler(svn_txdelta_window_t *window, void *baton)
{
  struct file_rev_baton *frb = baton;

  SVN_ERR(frb->window_handler(window, frb->window_baton));

  /* Terminate elements if we're done. */
  if (!window)
    {
      frb->window_handler = NULL;
      frb->window_baton = NULL;
      SVN_ERR(dav_svn__brigade_puts(frb->bb, frb->output,
                                    "</S:txdelta></S:file-rev>" DEBUG_CR));
    }
  return SVN_NO_ERROR;
}


/* This implements the svn_repos_file_rev_handler2_t interface. */
static svn_error_t *
file_rev_handler(void *baton,
                 const char *path,
                 svn_revnum_t revnum,
                 apr_hash_t *rev_props,
                 svn_boolean_t merged_revision,
                 svn_txdelta_window_handler_t *window_handler,
                 void **window_baton,
                 apr_array_header_t *props,
                 apr_pool_t *pool)
{
  struct file_rev_baton *frb = baton;
  apr_pool_t *subpool = svn_pool_create(pool);
  apr_hash_index_t *hi;
  int i;

  SVN_ERR(maybe_send_header(frb));

  SVN_ERR(dav_svn__brigade_printf(frb->bb, frb->output,
                                  "<S:file-rev path=\"%s\" rev=\"%ld\">"
                                  DEBUG_CR,
                                  apr_xml_quote_string(pool, path, 1),
                                  revnum));

  /* Send rev props. */
  for (hi = apr_hash_first(pool, rev_props); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      void *val;
      const char *pname;
      const svn_string_t *pval;

      svn_pool_clear(subpool);
      apr_hash_this(hi, &key, NULL, &val);
      pname = key;
      pval = val;
      SVN_ERR(send_prop(frb, "rev-prop", pname, pval, subpool));
    }

  /* Send file prop changes. */
  for (i = 0; i < props->nelts; ++i)
    {
      const svn_prop_t *prop = &APR_ARRAY_IDX(props, i, svn_prop_t);

      svn_pool_clear(subpool);
      if (prop->value)
        SVN_ERR(send_prop(frb, "set-prop", prop->name, prop->value,
                          subpool));
      else
        {
          /* Property was removed. */
          SVN_ERR(dav_svn__brigade_printf(frb->bb, frb->output,
                                          "<S:remove-prop name=\"%s\"/>"
                                          DEBUG_CR,
                                          apr_xml_quote_string(subpool,
                                                               prop->name,
                                                               1)));
        }
    }

  /* Send whether this was the result of a merge or not. */
  if (merged_revision)
    SVN_ERR(dav_svn__brigade_puts(frb->bb, frb->output,
                                  "<S:merged-revision/>"));

  /* Maybe send text delta. */
  if (window_handler)
    {
      svn_stream_t *base64_stream;

      base64_stream = dav_svn__make_base64_output_stream(frb->bb, frb->output,
                                                         pool);
      svn_txdelta_to_svndiff3(&frb->window_handler, &frb->window_baton,
                              base64_stream, frb->svndiff_version,
                              dav_svn__get_compression_level(), pool);
      *window_handler = delta_window_handler;
      *window_baton = frb;
      /* Start the txdelta element wich will be terminated by the window
         handler together with the file-rev element. */
      SVN_ERR(dav_svn__brigade_puts(frb->bb, frb->output, "<S:txdelta>"));
    }
  else
    /* No txdelta, so terminate the element here. */
    SVN_ERR(dav_svn__brigade_puts(frb->bb, frb->output,
                                  "</S:file-rev>" DEBUG_CR));

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


/* Respond to a client request for a REPORT of type file-revs-report for the
   RESOURCE.  Get request body from DOC and send result to OUTPUT. */
dav_error *
dav_svn__file_revs_report(const dav_resource *resource,
                          const apr_xml_doc *doc,
                          ap_filter_t *output)
{
  svn_error_t *serr;
  dav_error *derr = NULL;
  apr_xml_elem *child;
  int ns;
  struct file_rev_baton frb;
  dav_svn__authz_read_baton arb;
  const char *abs_path = NULL;

  /* These get determined from the request document. */
  svn_revnum_t start = SVN_INVALID_REVNUM;
  svn_revnum_t end = SVN_INVALID_REVNUM;
  svn_boolean_t include_merged_revisions = FALSE;    /* off by default */

  /* Construct the authz read check baton. */
  arb.r = resource->info->r;
  arb.repos = resource->info->repos;

  /* Sanity check. */
  ns = dav_svn__find_ns(doc->namespaces, SVN_XML_NAMESPACE);
  /* ### This is done on other places, but the document element is
     in this namespace, so is this necessary at all? */
  if (ns == -1)
    {
      return dav_svn__new_error_tag(resource->pool, HTTP_BAD_REQUEST, 0,
                                    "The request does not contain the 'svn:' "
                                    "namespace, so it is not going to have "
                                    "certain required elements.",
                                    SVN_DAV_ERROR_NAMESPACE,
                                    SVN_DAV_ERROR_TAG);
    }

  /* Get request information. */
  for (child = doc->root->first_child; child != NULL; child = child->next)
    {
      /* if this element isn't one of ours, then skip it */
      if (child->ns != ns)
        continue;

      if (strcmp(child->name, "start-revision") == 0)
        start = SVN_STR_TO_REV(dav_xml_get_cdata(child, resource->pool, 1));
      else if (strcmp(child->name, "end-revision") == 0)
        end = SVN_STR_TO_REV(dav_xml_get_cdata(child, resource->pool, 1));
      else if (strcmp(child->name, "include-merged-revisions") == 0)
        include_merged_revisions = TRUE; /* presence indicates positivity */
      else if (strcmp(child->name, "path") == 0)
        {
          const char *rel_path = dav_xml_get_cdata(child, resource->pool, 0);
          if ((derr = dav_svn__test_canonical(rel_path, resource->pool)))
            return derr;

          /* Force REL_PATH to be a relative path, not an fspath. */
          rel_path = svn_relpath_canonicalize(rel_path, resource->pool);

          /* Append the REL_PATH to the base FS path to get an
             absolute repository path. */
          abs_path = svn_fspath__join(resource->info->repos_path, rel_path,
                                      resource->pool);
        }
      /* else unknown element; skip it */
    }

  /* Check that all parameters are present and valid. */
  if (! abs_path)
    return dav_svn__new_error_tag(resource->pool, HTTP_BAD_REQUEST, 0,
                                  "Not all parameters passed.",
                                  SVN_DAV_ERROR_NAMESPACE,
                                  SVN_DAV_ERROR_TAG);

  frb.bb = apr_brigade_create(resource->pool,
                              output->c->bucket_alloc);
  frb.output = output;
  frb.needs_header = TRUE;
  frb.svndiff_version = resource->info->svndiff_version;

  /* file_rev_handler will send header first time it is called. */

  /* Get the revisions and send them. */
  serr = svn_repos_get_file_revs2(resource->info->repos->repos,
                                  abs_path, start, end, include_merged_revisions,
                                  dav_svn__authz_read_func(&arb), &arb,
                                  file_rev_handler, &frb, resource->pool);

  if (serr)
    {
      /* We don't 'goto cleanup' because ap_fflush() tells httpd
         to write the HTTP headers out, and that includes whatever
         r->status is at that particular time.  When we call
         dav_svn__convert_err(), we don't immediately set r->status
         right then, so r->status remains 0, hence HTTP status 200
         would be misleadingly returned. */
      return (dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                   serr->message, resource->pool));
    }

  if ((serr = maybe_send_header(&frb)))
    {
      derr = dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                  "Error beginning REPORT reponse",
                                  resource->pool);
      goto cleanup;
    }

  if ((serr = dav_svn__brigade_puts(frb.bb, frb.output,
                                    "</S:file-revs-report>" DEBUG_CR)))
    {
      derr = dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                  "Error ending REPORT reponse",
                                  resource->pool);
      goto cleanup;
    }

 cleanup:

  /* We've detected a 'high level' svn action to log. */
  dav_svn__operational_log(resource->info,
                           svn_log__get_file_revs(abs_path, start, end,
                                                  include_merged_revisions,
                                                  resource->pool));

  return dav_svn__final_flush_or_error(resource->info->r, frb.bb, output,
                                       derr, resource->pool);
}

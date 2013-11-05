/*
 * dated-rev.c: mod_dav_svn REPORT handler for mapping a date to a revision
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

#include <apr_tables.h>
#include <apr_uuid.h>

#include <httpd.h>
#include <http_log.h>
#include <mod_dav.h>

#include "svn_fs.h"
#include "svn_xml.h"
#include "svn_repos.h"
#include "svn_dav.h"
#include "svn_time.h"
#include "svn_pools.h"
#include "svn_props.h"
#include "svn_dav.h"
#include "svn_base64.h"

#include "private/svn_dav_protocol.h"

#include "../dav_svn.h"


/* Respond to a S:dated-rev-report request.  The request contains a
 * DAV:creationdate element giving the requested date; the response
 * contains a DAV:version-name element giving the most recent revision
 * as of that date. */
dav_error *
dav_svn__dated_rev_report(const dav_resource *resource,
                          const apr_xml_doc *doc,
                          ap_filter_t *output)
{
  apr_xml_elem *child;
  int ns;
  apr_time_t tm = (apr_time_t) -1;
  svn_revnum_t rev;
  apr_bucket_brigade *bb;
  svn_error_t *err;
  apr_status_t apr_err;
  dav_error *derr = NULL;

  /* Find the DAV:creationdate element and get the requested time from it. */
  ns = dav_svn__find_ns(doc->namespaces, "DAV:");
  if (ns != -1)
    {
      for (child = doc->root->first_child; child != NULL; child = child->next)
        {
          if (child->ns != ns ||
              strcmp(child->name, SVN_DAV__CREATIONDATE) != 0)
            continue;
          /* If this fails, we'll notice below, so ignore any error for now. */
          svn_error_clear
            (svn_time_from_cstring(&tm, dav_xml_get_cdata(child,
                                                          resource->pool, 1),
                                   resource->pool));
        }
    }

  if (tm == (apr_time_t) -1)
    {
      return dav_svn__new_error(resource->pool, HTTP_BAD_REQUEST, 0,
                                "The request does not contain a valid "
                                "'DAV:" SVN_DAV__CREATIONDATE "' element.");
    }

  /* Do the actual work of finding the revision by date. */
  if ((err = svn_repos_dated_revision(&rev, resource->info->repos->repos, tm,
                                      resource->pool)) != SVN_NO_ERROR)
    {
      svn_error_clear(err);
      return dav_svn__new_error(resource->pool, HTTP_INTERNAL_SERVER_ERROR, 0,
                                "Could not access revision times.");
    }

  bb = apr_brigade_create(resource->pool, output->c->bucket_alloc);
  apr_err = ap_fprintf(output, bb,
                       DAV_XML_HEADER DEBUG_CR
                       "<S:dated-rev-report xmlns:S=\"" SVN_XML_NAMESPACE "\" "
                       "xmlns:D=\"DAV:\">" DEBUG_CR
                       "<D:" SVN_DAV__VERSION_NAME ">%ld</D:"
                       SVN_DAV__VERSION_NAME ">""</S:dated-rev-report>", rev);
  if (apr_err)
    derr = dav_svn__convert_err(svn_error_create(apr_err, 0, NULL),
                                HTTP_INTERNAL_SERVER_ERROR,
                                "Error writing REPORT response.",
                                resource->pool);

  return dav_svn__final_flush_or_error(resource->info->r, bb, output,
                                       derr, resource->pool);
}

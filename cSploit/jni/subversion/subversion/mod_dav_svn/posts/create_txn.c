/*
 * create_txn.c: mod_dav_svn POST handler for creating a commit transaction
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

#include <httpd.h>
#include <mod_dav.h>

#include "svn_dav.h"
#include "../dav_svn.h"

/* Respond to a "create-txn" POST request.
 *
 * Syntax:  ( create-txn )
 */
dav_error *
dav_svn__post_create_txn(const dav_resource *resource,
                         svn_skel_t *request_skel,
                         ap_filter_t *output)
{
  const char *txn_name;
  const char *vtxn_name;
  dav_error *derr;
  request_rec *r = resource->info->r;

  /* Create a Subversion repository transaction based on HEAD. */
  if ((derr = dav_svn__create_txn(resource->info->repos, &txn_name,
                                  resource->pool)))
    return derr;

  /* Build a "201 Created" response with header that tells the
     client our new transaction's name. */
  vtxn_name =  apr_table_get(r->headers_in, SVN_DAV_VTXN_NAME_HEADER);
  if (vtxn_name && vtxn_name[0])
    {
      /* If the client supplied a vtxn name then store a mapping from
         the client name to the FS transaction name in the activity
         database. */
      if ((derr  = dav_svn__store_activity(resource->info->repos,
                                           vtxn_name, txn_name)))
        return derr;
      apr_table_set(r->headers_out, SVN_DAV_VTXN_NAME_HEADER, vtxn_name);
    }
  else
    apr_table_set(r->headers_out, SVN_DAV_TXN_NAME_HEADER, txn_name);

  r->status = HTTP_CREATED;

  return NULL;
}

/*
 * get-locks.c: mod_dav_svn REPORT handler for querying filesystem locks
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

#include "../dav_svn.h"

/* Respond to a get-locks-report request.  See description of this
   report in libsvn_ra_neon/get_locks.c.  */


#define SVN_APR_ERR(expr)                       \
  do {                                          \
    apr_status_t apr_status__temp = (expr);     \
    if (apr_status__temp)                       \
      return apr_status__temp;                  \
  } while (0)


/* Transmit LOCKS (a hash of Subversion filesystem locks keyed by
   path) across OUTPUT using BB.  Use POOL for necessary allocations.

   NOTE:  As written, this function currently returns one of only two
   status values -- "success", and "we had trouble writing out to the
   output stream".  If you need to return something more interesting,
   you'll probably want to generate dav_error's here instead of
   passing back only apr_status_t's.  */
static apr_status_t
send_get_lock_response(apr_hash_t *locks,
                       ap_filter_t *output,
                       apr_bucket_brigade *bb,
                       apr_pool_t *pool)
{
  apr_pool_t *subpool;
  apr_hash_index_t *hi;

  /* start sending report */
  SVN_APR_ERR(ap_fprintf(output, bb,
                         DAV_XML_HEADER DEBUG_CR
                         "<S:get-locks-report xmlns:S=\"" SVN_XML_NAMESPACE
                         "\" xmlns:D=\"DAV:\">" DEBUG_CR));

  /* stream the locks */
  subpool = svn_pool_create(pool);
  for (hi = apr_hash_first(pool, locks); hi; hi = apr_hash_next(hi))
    {
      void *val;
      const svn_lock_t *lock;

      svn_pool_clear(subpool);
      apr_hash_this(hi, NULL, NULL, &val);
      lock = val;

      /* Begin the <S:lock> tag, transmitting the path, token, and
         creation date. */
      SVN_APR_ERR(ap_fprintf(output, bb,
                             "<S:lock>" DEBUG_CR
                             "<S:path>%s</S:path>" DEBUG_CR
                             "<S:token>%s</S:token>" DEBUG_CR
                             "<S:creationdate>%s</S:creationdate>" DEBUG_CR,
                             apr_xml_quote_string(subpool, lock->path, 1),
                             apr_xml_quote_string(subpool, lock->token, 1),
                             svn_time_to_cstring(lock->creation_date,
                                                 subpool)));

      /* Got expiration date?  Tell the client. */
      if (lock->expiration_date)
        SVN_APR_ERR(ap_fprintf(output, bb,
                               "<S:expirationdate>%s</S:expirationdate>"
                               DEBUG_CR,
                               svn_time_to_cstring(lock->expiration_date,
                                                   subpool)));

      /* Transmit the lock ownership information. */
      if (lock->owner)
        {
          const char *owner;
          svn_boolean_t owner_base64 = FALSE;

          if (svn_xml_is_xml_safe(lock->owner, strlen(lock->owner)))
            {
              owner = apr_xml_quote_string(subpool, lock->owner, 1);
            }
          else
            {
              svn_string_t owner_string;
              const svn_string_t *encoded_owner;

              owner_string.data = lock->owner;
              owner_string.len = strlen(lock->owner);
              encoded_owner = svn_base64_encode_string2(&owner_string, TRUE,
                                                        subpool);
              owner = encoded_owner->data;
              owner_base64 = TRUE;
            }
          SVN_APR_ERR(ap_fprintf(output, bb,
                                 "<S:owner %s>%s</S:owner>" DEBUG_CR,
                                 owner_base64 ? "encoding=\"base64\"" : "",
                                 owner));
        }

      /* If the lock carries a comment, transmit that, too. */
      if (lock->comment)
        {
          const char *comment;
          svn_boolean_t comment_base64 = FALSE;

          if (svn_xml_is_xml_safe(lock->comment, strlen(lock->comment)))
            {
              comment = apr_xml_quote_string(subpool, lock->comment, 1);
            }
          else
            {
              svn_string_t comment_string;
              const svn_string_t *encoded_comment;

              comment_string.data = lock->comment;
              comment_string.len = strlen(lock->comment);
              encoded_comment = svn_base64_encode_string2(&comment_string,
                                                          TRUE, subpool);
              comment = encoded_comment->data;
              comment_base64 = TRUE;
            }
          SVN_APR_ERR(ap_fprintf(output, bb,
                                 "<S:comment %s>%s</S:comment>" DEBUG_CR,
                                 comment_base64 ? "encoding=\"base64\"" : "",
                                 comment));
        }

      /* Okay, finish up this lock by closing the <S:lock> tag. */
      SVN_APR_ERR(ap_fprintf(output, bb, "</S:lock>" DEBUG_CR));
    }
  svn_pool_destroy(subpool);

  /* Finish the report */
  SVN_APR_ERR(ap_fprintf(output, bb, "</S:get-locks-report>" DEBUG_CR));

  return APR_SUCCESS;
}

#undef SVN_APR_ERR

dav_error *
dav_svn__get_locks_report(const dav_resource *resource,
                          const apr_xml_doc *doc,
                          ap_filter_t *output)
{
  apr_bucket_brigade *bb;
  svn_error_t *err;
  dav_error *derr = NULL;
  apr_status_t apr_err;
  apr_hash_t *locks;
  dav_svn__authz_read_baton arb;
  svn_depth_t depth = svn_depth_unknown;
  apr_xml_attr *this_attr;

  /* The request URI should be a public one representing an fs path. */
  if ((! resource->info->repos_path)
      || (! resource->info->repos->repos))
    return dav_svn__new_error(resource->pool, HTTP_BAD_REQUEST, 0,
                              "get-locks-report run on resource which doesn't "
                              "represent a path within a repository.");

  arb.r = resource->info->r;
  arb.repos = resource->info->repos;

  /* See if the client provided additional information for this request. */
  for (this_attr = doc->root->attr; this_attr; this_attr = this_attr->next)
    {
      if (strcmp(this_attr->name, "depth") == 0)
        {
          depth = svn_depth_from_word(this_attr->value);
          if ((depth != svn_depth_empty) &&
              (depth != svn_depth_files) &&
              (depth != svn_depth_immediates) &&
              (depth != svn_depth_infinity))
            return dav_svn__new_error(resource->pool, HTTP_BAD_REQUEST, 0,
                                      "Invalid 'depth' specified in "
                                      "get-locks-report request.");
          continue;
        }
    }

  /* For compatibility, our default depth is infinity. */
  if (depth == svn_depth_unknown)
    depth = svn_depth_infinity;

  /* Fetch the locks, but allow authz_read checks to happen on each. */
  if ((err = svn_repos_fs_get_locks2(&locks,
                                     resource->info->repos->repos,
                                     resource->info->repos_path, depth,
                                     dav_svn__authz_read_func(&arb), &arb,
                                     resource->pool)) != SVN_NO_ERROR)
    return dav_svn__convert_err(err, HTTP_INTERNAL_SERVER_ERROR,
                                err->message, resource->pool);

  bb = apr_brigade_create(resource->pool, output->c->bucket_alloc);

  if ((apr_err = send_get_lock_response(locks, output, bb, resource->pool)))
    derr = dav_svn__convert_err(svn_error_create(apr_err, 0, NULL),
                                HTTP_INTERNAL_SERVER_ERROR,
                                "Error writing REPORT response.",
                                resource->pool);

  return dav_svn__final_flush_or_error(resource->info->r, bb, output,
                                       derr, resource->pool);
}


/*
 * date_rev.c :  RA get-dated-revision API implementation
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

#include <apr_pools.h>
#include <apr_tables.h>
#include <apr_strings.h>
#include <apr_xml.h>

#include <ne_basic.h>

#include "svn_error.h"
#include "svn_pools.h"
#include "svn_ra.h"
#include "../libsvn_ra/ra_loader.h"
#include "svn_path.h"
#include "svn_xml.h"
#include "svn_dav.h"
#include "svn_time.h"

#include "private/svn_dav_protocol.h"
#include "svn_private_config.h"

#include "ra_neon.h"


/* -------------------------------------------------------------------------
**
** DATED REV REPORT HANDLING
**
** DeltaV provides no mechanism for mapping a date to a revision, so
** we use a custom report, S:dated-rev-report.  The request contains a
** DAV:creationdate element giving the requested date; the response
** contains a DAV:version-name element giving the most recent revision
** as of that date.
**
** Since this report is so simple, we don't bother with validation or
** baton structures or anything; we just set the revision number in
** the end-element handler for DAV:version-name.
*/

/* Elements used in a dated-rev-report response */
static const svn_ra_neon__xml_elm_t drev_report_elements[] =
{
  { SVN_XML_NAMESPACE, "dated-rev-report", ELEM_dated_rev_report, 0 },
  { "DAV:", "version-name", ELEM_version_name, SVN_RA_NEON__XML_CDATA },
  { NULL }
};

typedef struct drev_baton_t
{
  svn_stringbuf_t *cdata;
  apr_pool_t *pool;
  svn_revnum_t revision;
} drev_baton_t;


/* This implements the `svn_ra_neon__xml_startelm_cb' prototype. */
static svn_error_t *
drev_start_element(int *elem, void *baton, int parent,
                   const char *nspace, const char *name, const char **atts)
{
  const svn_ra_neon__xml_elm_t *elm =
    svn_ra_neon__lookup_xml_elem(drev_report_elements, nspace, name);
  drev_baton_t *b = baton;

  *elem = elm ? elm->id : SVN_RA_NEON__XML_DECLINE;
  if (!elm)
    return SVN_NO_ERROR;

  if (elm->id == ELEM_version_name)
    b->cdata = svn_stringbuf_create("", b->pool);

  return SVN_NO_ERROR;
}

/* This implements the `svn_ra_neon__xml_endelm_cb' prototype. */
static svn_error_t *
drev_end_element(void *baton, int state,
                 const char *nspace, const char *name)
{
  drev_baton_t *b = baton;

  if (state == ELEM_version_name && b->cdata)
    {
      b->revision = SVN_STR_TO_REV(b->cdata->data);
      b->cdata = NULL;
    }

  return SVN_NO_ERROR;
}

svn_error_t *svn_ra_neon__get_dated_revision(svn_ra_session_t *session,
                                             svn_revnum_t *revision,
                                             apr_time_t timestamp,
                                             apr_pool_t *pool)
{
  svn_ra_neon__session_t *ras = session->priv;
  const char *body;
  const char *report_target;
  svn_error_t *err;
  drev_baton_t *b = apr_palloc(pool, sizeof(*b));

  b->pool = pool;
  b->cdata = NULL;
  b->revision = SVN_INVALID_REVNUM;

  /* Got HTTP v2 support?  We'll report against the "me resource".
     Otherwise, we'll use the VCC.  */
  if (SVN_RA_NEON__HAVE_HTTPV2_SUPPORT(ras))
    {
      report_target = ras->me_resource;
    }
  else
    {
      SVN_ERR(svn_ra_neon__get_vcc(&report_target, ras, ras->root.path, pool));
    }

  body = apr_psprintf(pool,
                      "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                      "<S:dated-rev-report xmlns:S=\"" SVN_XML_NAMESPACE "\" "
                      "xmlns:D=\"DAV:\">"
                      "<D:" SVN_DAV__CREATIONDATE ">%s</D:"
                      SVN_DAV__CREATIONDATE "></S:dated-rev-report>",
                      svn_time_to_cstring(timestamp, pool));

  err = svn_ra_neon__parsed_request(ras, "REPORT",
                                    report_target, body, NULL, NULL,
                                    drev_start_element,
                                    svn_ra_neon__xml_collect_cdata,
                                    drev_end_element,
                                    b, NULL, NULL, FALSE, pool);
  if (err && err->apr_err == SVN_ERR_UNSUPPORTED_FEATURE)
    return svn_error_quick_wrap(err, _("Server does not support date-based "
                                       "operations"));
  else if (err)
    return err;

  if (b->revision == SVN_INVALID_REVNUM)
    return svn_error_create(SVN_ERR_INCOMPLETE_DATA, NULL,
                            _("Invalid server response to dated-rev request"));

  *revision = b->revision;
  return SVN_NO_ERROR;
}

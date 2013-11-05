/*
 * get_locations.c :  RA get-locations API implementation
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

#include "private/svn_dav_protocol.h"
#include "svn_private_config.h"

#include "ra_neon.h"


/*
 * Plan for processing the XML. The XML will be of the form:
 *
 * <S:get-locations-report xmlns...>
 *     <S:location rev="..." path="..."/>
 *     ...
 * </S:get-locations-report>
 *
 * We extract what we want at the start of <S:location>.
*/

/* Elements used in a get-locations response */
static const svn_ra_neon__xml_elm_t gloc_report_elements[] =
{
  { SVN_XML_NAMESPACE, "get-locations-report", ELEM_get_locations_report, 0 },
  { SVN_XML_NAMESPACE, "location", ELEM_location, 0 },
  { NULL }
};

typedef struct get_locations_baton_t {
  svn_ra_neon__session_t *ras;
  apr_hash_t *hash;
  apr_pool_t *pool;
} get_locations_baton_t;


/* This implements the `svn_ra_neon__startelem_cb_t' prototype. */
static svn_error_t *
gloc_start_element(int *elem, void *userdata, int parent_state,
                   const char *ns, const char *ln, const char **atts)
{
  get_locations_baton_t *baton = userdata;
  const svn_ra_neon__xml_elm_t *elm;

  elm = svn_ra_neon__lookup_xml_elem(gloc_report_elements, ns, ln);

  /* Just skip unknown elements. */
  if (!elm)
    {
      *elem = NE_XML_DECLINE;
      return SVN_NO_ERROR;
    }

  if (parent_state == ELEM_get_locations_report
      && elm->id == ELEM_location)
    {
      svn_revnum_t rev = SVN_INVALID_REVNUM;
      const char *path;
      const char *r;

      r = svn_xml_get_attr_value("rev", atts);
      if (r)
        rev = SVN_STR_TO_REV(r);

      path = svn_xml_get_attr_value("path", atts);

      if (SVN_IS_VALID_REVNUM(rev) && path)
        apr_hash_set(baton->hash,
                     apr_pmemdup(baton->pool, &rev, sizeof(rev)),
                     sizeof(rev), apr_pstrdup(baton->pool, path));
      else
        return svn_error_create(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                _("Expected a valid revnum and path"));
    }

  *elem = elm->id;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_ra_neon__get_locations(svn_ra_session_t *session,
                           apr_hash_t **locations,
                           const char *relative_path,
                           svn_revnum_t peg_revision,
                           const apr_array_header_t *location_revisions,
                           apr_pool_t *pool)
{
  svn_ra_neon__session_t *ras = session->priv;
  svn_stringbuf_t *request_body;
  svn_error_t *err;
  get_locations_baton_t request_baton;
  const char *relative_path_quoted;
  const char *bc_url;
  const char *bc_relative;
  const char *final_bc_url;
  int i;
  int status_code = 0;

  *locations = apr_hash_make(pool);

  request_body = svn_stringbuf_create("", pool);
  svn_stringbuf_appendcstr(request_body,
                           "<?xml version=\"1.0\" encoding=\"utf-8\"?>" DEBUG_CR
                           "<S:get-locations xmlns:S=\"" SVN_XML_NAMESPACE
                           "\" xmlns:D=\"DAV:\">" DEBUG_CR);

  svn_stringbuf_appendcstr(request_body, "<S:path>");
  /* We need to escape the path XML-wise. */
  relative_path_quoted = apr_xml_quote_string(pool, relative_path, 0);
  svn_stringbuf_appendcstr(request_body, relative_path_quoted);
  svn_stringbuf_appendcstr(request_body, "</S:path>" DEBUG_CR);
  svn_stringbuf_appendcstr(request_body,
                           apr_psprintf(pool,
                                        "<S:peg-revision>%ld"
                                        "</S:peg-revision>" DEBUG_CR,
                                        peg_revision));

  for (i = 0; i < location_revisions->nelts; ++i)
    {
      svn_revnum_t rev = APR_ARRAY_IDX(location_revisions, i, svn_revnum_t);
      svn_stringbuf_appendcstr(request_body,
                               apr_psprintf(pool,
                                            "<S:location-revision>%ld"
                                            "</S:location-revision>" DEBUG_CR,
                                            rev));
    }

  svn_stringbuf_appendcstr(request_body, "</S:get-locations>");

  request_baton.ras = ras;
  request_baton.hash = *locations;
  request_baton.pool = pool;

  /* ras's URL may not exist in HEAD, and thus it's not safe to send
     it as the main argument to the REPORT request; it might cause
     dav_get_resource() to choke on the server.  So instead, we pass a
     baseline-collection URL, which we get from the peg revision.  */
  SVN_ERR(svn_ra_neon__get_baseline_info(&bc_url, &bc_relative, NULL, ras,
                                         ras->url->data, peg_revision, pool));
  final_bc_url = svn_path_url_add_component2(bc_url, bc_relative, pool);

  err = svn_ra_neon__parsed_request(ras, "REPORT", final_bc_url,
                                    request_body->data, NULL, NULL,
                                    gloc_start_element, NULL, NULL,
                                    &request_baton, NULL, &status_code,
                                    FALSE, pool);

  /* Map status 501: Method Not Implemented to our not implemented error.
     1.0.x servers and older don't support this report. */
  if (status_code == 501)
    return svn_error_createf(SVN_ERR_RA_NOT_IMPLEMENTED, err,
                             _("'%s' REPORT not implemented"), "get-locations");

  return err;
}

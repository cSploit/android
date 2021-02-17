/*
 * props.c :  routines for fetching DAV properties
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
#include <apr_tables.h>
#include <apr_strings.h>
#define APR_WANT_STRFUNC
#include <apr_want.h>

#include "svn_error.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_dav.h"
#include "svn_base64.h"
#include "svn_xml.h"
#include "svn_time.h"
#include "svn_pools.h"
#include "svn_props.h"
#include "../libsvn_ra/ra_loader.h"

#include "private/svn_dav_protocol.h"
#include "private/svn_fspath.h"
#include "svn_private_config.h"

#include "ra_neon.h"


/* some definitions of various properties that may be fetched */
const ne_propname svn_ra_neon__vcc_prop = {
  "DAV:", "version-controlled-configuration"
};
const ne_propname svn_ra_neon__checked_in_prop = {
  "DAV:", "checked-in"
};

/* when we begin a checkout, we fetch these from the "public" resources to
   steer us towards a Baseline Collection. we fetch the resourcetype to
   verify that we're accessing a collection. */
static const ne_propname starting_props[] =
{
  { "DAV:", "version-controlled-configuration" },
  { "DAV:", "resourcetype" },
  { SVN_DAV_PROP_NS_DAV, "baseline-relative-path" },
  { SVN_DAV_PROP_NS_DAV, "repository-uuid"},
  { NULL }
};

/* when speaking to a Baseline to reach the Baseline Collection, fetch these
   properties. */
static const ne_propname baseline_props[] =
{
  { "DAV:", "baseline-collection" },
  { "DAV:", SVN_DAV__VERSION_NAME },
  { NULL }
};



/*** Propfind Implementation ***/

typedef struct elem_defn {
  svn_ra_neon__xml_elmid id;
  const char *name;
  int is_property;      /* is it a property, or part of some structure? */
} elem_defn;


static const elem_defn elem_definitions[] =
{
  /*** NOTE: Make sure that every item in here is also represented in
       propfind_elements[] ***/

  /* DAV elements */
  { ELEM_multistatus, "DAV:multistatus", 0 },
  { ELEM_response, "DAV:response", 0 },
  { ELEM_href, "DAV:href", SVN_RA_NEON__XML_CDATA },
  { ELEM_propstat, "DAV:propstat", 0 },
  { ELEM_prop, "DAV:prop", 0 },
  { ELEM_status, "DAV:status", SVN_RA_NEON__XML_CDATA },
  { ELEM_baseline, "DAV:baseline", SVN_RA_NEON__XML_CDATA },
  { ELEM_collection, "DAV:collection", SVN_RA_NEON__XML_CDATA },
  { ELEM_resourcetype, "DAV:resourcetype", 0 },
  { ELEM_baseline_coll, SVN_RA_NEON__PROP_BASELINE_COLLECTION, 0 },
  { ELEM_checked_in, SVN_RA_NEON__PROP_CHECKED_IN, 0 },
  { ELEM_vcc, SVN_RA_NEON__PROP_VCC, 0 },
  { ELEM_version_name, SVN_RA_NEON__PROP_VERSION_NAME, 1 },
  { ELEM_get_content_length, SVN_RA_NEON__PROP_GETCONTENTLENGTH, 1 },
  { ELEM_creationdate, SVN_RA_NEON__PROP_CREATIONDATE, 1 },
  { ELEM_creator_displayname, SVN_RA_NEON__PROP_CREATOR_DISPLAYNAME, 1 },

  /* SVN elements */
  { ELEM_baseline_relpath, SVN_RA_NEON__PROP_BASELINE_RELPATH, 1 },
  { ELEM_md5_checksum, SVN_RA_NEON__PROP_MD5_CHECKSUM, 1 },
  { ELEM_repository_uuid, SVN_RA_NEON__PROP_REPOSITORY_UUID, 1 },
  { ELEM_deadprop_count, SVN_RA_NEON__PROP_DEADPROP_COUNT, 1 },
  { 0 }
};


static const svn_ra_neon__xml_elm_t propfind_elements[] =
{
  /*** NOTE: Make sure that every item in here is also represented in
       elem_definitions[] ***/

  /* DAV elements */
  { "DAV:", "multistatus", ELEM_multistatus, 0 },
  { "DAV:", "response", ELEM_response, 0 },
  { "DAV:", "href", ELEM_href, SVN_RA_NEON__XML_CDATA },
  { "DAV:", "propstat", ELEM_propstat, 0 },
  { "DAV:", "prop", ELEM_prop, 0 },
  { "DAV:", "status", ELEM_status, SVN_RA_NEON__XML_CDATA },
  { "DAV:", "baseline", ELEM_baseline, SVN_RA_NEON__XML_CDATA },
  { "DAV:", "baseline-collection", ELEM_baseline_coll, SVN_RA_NEON__XML_CDATA },
  { "DAV:", "checked-in", ELEM_checked_in, 0 },
  { "DAV:", "collection", ELEM_collection, SVN_RA_NEON__XML_CDATA },
  { "DAV:", "resourcetype", ELEM_resourcetype, 0 },
  { "DAV:", "version-controlled-configuration", ELEM_vcc, 0 },
  { "DAV:", SVN_DAV__VERSION_NAME, ELEM_version_name, SVN_RA_NEON__XML_CDATA },
  { "DAV:", "getcontentlength", ELEM_get_content_length,
    SVN_RA_NEON__XML_CDATA },
  { "DAV:", SVN_DAV__CREATIONDATE, ELEM_creationdate, SVN_RA_NEON__XML_CDATA },
  { "DAV:", "creator-displayname", ELEM_creator_displayname,
    SVN_RA_NEON__XML_CDATA },

  /* SVN elements */
  { SVN_DAV_PROP_NS_DAV, "baseline-relative-path", ELEM_baseline_relpath,
    SVN_RA_NEON__XML_CDATA },
  { SVN_DAV_PROP_NS_DAV, "md5-checksum", ELEM_md5_checksum,
    SVN_RA_NEON__XML_CDATA },
  { SVN_DAV_PROP_NS_DAV, "repository-uuid", ELEM_repository_uuid,
    SVN_RA_NEON__XML_CDATA },
  { SVN_DAV_PROP_NS_DAV, "deadprop-count", ELEM_deadprop_count,
    SVN_RA_NEON__XML_CDATA },

  /* Unknowns */
  { "", "", ELEM_unknown, SVN_RA_NEON__XML_COLLECT },

  { NULL }
};


typedef struct propfind_ctx_t
{
  /*WARNING: WANT_CDATA should stay the first element in the baton:
    svn_ra_neon__xml_collect_cdata() assumes the baton starts with a stringbuf.
  */
  svn_stringbuf_t *cdata;
  apr_hash_t *props; /* const char *URL-PATH -> svn_ra_neon__resource_t */

  svn_ra_neon__resource_t *rsrc; /* the current resource. */
  const char *encoding; /* property encoding (or NULL) */
  int status; /* status for the current <propstat> (or 0 if unknown). */
  apr_hash_t *propbuffer; /* holds properties until their status is known. */
  svn_ra_neon__xml_elmid last_open_id; /* the id of the last opened tag. */
  ne_xml_parser *parser; /* xml parser handling the PROPSET request. */

  apr_pool_t *pool;

} propfind_ctx_t;


/* Look up an element definition ID.  May return NULL if the elem is
   not recognized. */
static const elem_defn *defn_from_id(svn_ra_neon__xml_elmid id)
{
  const elem_defn *defn;

  for (defn = elem_definitions; defn->name != NULL; ++defn)
    {
      if (id == defn->id)
        return defn;
    }

  return NULL;
}


/* Assign URL to RSRC.  Use POOL for any allocations. */
static svn_error_t *
assign_rsrc_url(svn_ra_neon__resource_t *rsrc,
                const char *url, apr_pool_t *pool)
{
  char *url_path;
  apr_size_t len;
  ne_uri parsed_url;

  /* Parse the PATH element out of the URL.
     NOTE: mod_dav does not (currently) use an absolute URL, but simply a
     server-relative path (i.e. this uri_parse is effectively a no-op).
  */
  if (ne_uri_parse(url, &parsed_url) != 0)
    {
      ne_uri_free(&parsed_url);
      return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                               _("Unable to parse URL '%s'"), url);
    }

  url_path = apr_pstrdup(pool, parsed_url.path);
  ne_uri_free(&parsed_url);

  /* Clean up trailing slashes from the URL. */
  len = strlen(url_path);
  if (len > 1 && url_path[len - 1] == '/')
    url_path[len - 1] = '\0';
  rsrc->url = url_path;

  return SVN_NO_ERROR;
}

/* Determine whether we're receiving the expected XML response.
   Return CHILD when interested in receiving the child's contents
   or one of SVN_RA_NEON__XML_INVALID and SVN_RA_NEON__XML_DECLINE
   when respectively this is the incorrect response or
   the element (and its children) are uninteresting */
static int validate_element(svn_ra_neon__xml_elmid parent,
                            svn_ra_neon__xml_elmid child)
{
  switch (parent)
    {
    case ELEM_root:
      if (child == ELEM_multistatus)
        return child;
      else
        return SVN_RA_NEON__XML_INVALID;

    case ELEM_multistatus:
      if (child == ELEM_response)
        return child;
      else
        return SVN_RA_NEON__XML_DECLINE;

    case ELEM_response:
      if ((child == ELEM_href) || (child == ELEM_propstat))
        return child;
      else
        return SVN_RA_NEON__XML_DECLINE;

    case ELEM_propstat:
      if ((child == ELEM_prop) || (child == ELEM_status))
        return child;
      else
        return SVN_RA_NEON__XML_DECLINE;

    case ELEM_prop:
      return child; /* handle all children of <prop> */

    case ELEM_baseline_coll:
    case ELEM_checked_in:
    case ELEM_vcc:
      if (child == ELEM_href)
        return child;
      else
        return SVN_RA_NEON__XML_DECLINE; /* not concerned with other types */

    case ELEM_resourcetype:
      if ((child == ELEM_collection) || (child == ELEM_baseline))
        return child;
      else
        return SVN_RA_NEON__XML_DECLINE; /* not concerned with other types
                                           (### now) */

    default:
      return SVN_RA_NEON__XML_DECLINE;
    }

  /* NOTREACHED */
}


static svn_error_t *
start_element(int *elem, void *baton, int parent,
              const char *nspace, const char *name, const char **atts)
{
  propfind_ctx_t *pc = baton;
  const svn_ra_neon__xml_elm_t *elm
    = svn_ra_neon__lookup_xml_elem(propfind_elements, nspace, name);


  *elem = elm ? validate_element(parent, elm->id) : SVN_RA_NEON__XML_DECLINE;
  if (*elem < 1) /* not a valid element */
    return SVN_NO_ERROR;

  svn_stringbuf_setempty(pc->cdata);
  *elem = elm ? elm->id : ELEM_unknown;
  switch (*elem)
    {
    case ELEM_response:
      if (pc->rsrc)
        return svn_error_create(SVN_ERR_XML_MALFORMED, NULL, NULL);
      /* Create a new resource. */
      pc->rsrc = apr_pcalloc(pc->pool, sizeof(*(pc->rsrc)));
      pc->rsrc->pool = pc->pool;
      pc->rsrc->propset = apr_hash_make(pc->pool);
      pc->status = 0;
      break;

    case ELEM_propstat:
      pc->status = 0;
      break;

    case ELEM_href:
      /* Remember this <href>'s parent so that when we close this tag,
         we know to whom the URL assignment belongs.  Could be the
         resource itself, or one of the properties:
         ELEM_baseline_coll, ELEM_checked_in, ELEM_vcc: */
      pc->rsrc->href_parent = pc->last_open_id;
      break;

    case ELEM_collection:
      pc->rsrc->is_collection = 1;
      break;

    case ELEM_unknown:
      /* these are our user-visible properties, presumably. */
      pc->encoding = ne_xml_get_attr(pc->parser, atts, SVN_DAV_PROP_NS_DAV,
                                     "encoding");
      if (pc->encoding)
        pc->encoding = apr_pstrdup(pc->pool, pc->encoding);
      break;

    default:
      /* nothing to do for these */
      break;
    }

  /* Remember the last tag we opened. */
  pc->last_open_id = *elem;
  return SVN_NO_ERROR;
}


static svn_error_t * end_element(void *baton, int state,
                                 const char *nspace, const char *name)
{
  propfind_ctx_t *pc = baton;
  svn_ra_neon__resource_t *rsrc = pc->rsrc;
  const svn_string_t *value = NULL;
  const elem_defn *parent_defn;
  const elem_defn *defn;
  ne_status status;
  const char *cdata = pc->cdata->data;

  switch (state)
    {
    case ELEM_response:
      /* Verify that we've received a URL for this resource. */
      if (!pc->rsrc->url)
        return svn_error_create(SVN_ERR_XML_MALFORMED, NULL, NULL);

      /* Store the resource in the top-level hash table. */
      apr_hash_set(pc->props, pc->rsrc->url, APR_HASH_KEY_STRING, pc->rsrc);
      pc->rsrc = NULL;
      return SVN_NO_ERROR;

    case ELEM_propstat:
      /* We're at the end of a set of properties.  Do the right thing
         status-wise. */
      if (pc->status)
        {
          /* We have a status.  Loop over the buffered properties, and
             if the status is a good one (200), copy them into the
             resources's property hash.  Regardless of the status,
             we'll be removing these from the temporary buffer as we
             go along. */
          apr_hash_index_t *hi = apr_hash_first(pc->pool, pc->propbuffer);
          for (; hi; hi = apr_hash_next(hi))
            {
              const void *key;
              apr_ssize_t klen;
              void *val;
              apr_hash_this(hi, &key, &klen, &val);
              if (pc->status == 200)
                apr_hash_set(rsrc->propset, key, klen, val);
              apr_hash_set(pc->propbuffer, key, klen, NULL);
            }
        }
      else if (! pc->status)
        {
          /* No status at all?  Bogosity. */
          return svn_error_create(SVN_ERR_XML_MALFORMED, NULL, NULL);
        }
      return SVN_NO_ERROR;

    case ELEM_status:
      /* Parse the <status> tag's CDATA for a status code. */
      if (ne_parse_statusline(cdata, &status))
        return svn_error_create(SVN_ERR_XML_MALFORMED, NULL, NULL);
      free(status.reason_phrase);
      pc->status = status.code;
      return SVN_NO_ERROR;

    case ELEM_href:
      /* Special handling for <href> that belongs to the <response> tag. */
      if (rsrc->href_parent == ELEM_response)
        return assign_rsrc_url(pc->rsrc,
                               svn_urlpath__canonicalize(cdata, pc->pool),
                               pc->pool);

      /* Use the parent element's name, not the href. */
      parent_defn = defn_from_id(rsrc->href_parent);

      /* No known parent?  Get outta here. */
      if (!parent_defn)
        return SVN_NO_ERROR;

      /* All other href's we'll treat as property values. */
      name = parent_defn->name;
      value = svn_string_create(svn_urlpath__canonicalize(cdata, pc->pool),
                                pc->pool);
      break;

    default:
      /*** This case is, as usual, for everything not covered by other
           cases.  ELM->id should be either ELEM_unknown, or one of
           the ids in the elem_definitions[] structure.  In this case,
           we seek to handle properties.  Since ELEM_unknown should
           only occur for properties, we will handle that id.  All
           other ids will be searched for in the elem_definitions[]
           structure to determine if they are properties.  Properties,
           we handle; all else hits the road.  ***/

      if (state == ELEM_unknown)
        {
          name = apr_pstrcat(pc->pool, nspace, name, (char *)NULL);
        }
      else
        {
          defn = defn_from_id(state);
          if (! (defn && defn->is_property))
            return SVN_NO_ERROR;
          name = defn->name;
        }

      /* Check for encoding attribute. */
      if (pc->encoding == NULL) {
        /* Handle the property value by converting it to string. */
        value = svn_string_create(cdata, pc->pool);
        break;
      }

      /* Check for known encoding type */
      if (strcmp(pc->encoding, "base64") != 0)
        return svn_error_create(SVN_ERR_XML_MALFORMED, NULL, NULL);

      /* There is an encoding on this property, handle it.
       * the braces are needed to allocate "in" on the stack. */
      {
        svn_string_t in;
        in.data = cdata;
        in.len = strlen(cdata);
        value = svn_base64_decode_string(&in, pc->pool);
      }

      pc->encoding = NULL; /* Reset encoding for future attribute(s). */
    }

  /*** Handling resource properties from here out. ***/

  /* Add properties to the temporary propbuffer.  At the end of the
     <propstat>, we'll either dump the props as invalid or move them
     into the resource's property hash. */
  apr_hash_set(pc->propbuffer, name, APR_HASH_KEY_STRING, value);
  return SVN_NO_ERROR;
}


static void set_parser(ne_xml_parser *parser,
                       void *baton)
{
  propfind_ctx_t *pc = baton;
  pc->parser = parser;
}


svn_error_t * svn_ra_neon__get_props(apr_hash_t **results,
                                     svn_ra_neon__session_t *sess,
                                     const char *url,
                                     int depth,
                                     const char *label,
                                     const ne_propname *which_props,
                                     apr_pool_t *pool)
{
  propfind_ctx_t pc;
  svn_stringbuf_t *body;
  apr_hash_t *extra_headers = apr_hash_make(pool);

  svn_ra_neon__add_depth_header(extra_headers, depth);

  /* If we have a label, use it. */
  if (label != NULL)
    apr_hash_set(extra_headers, "Label", 5, label);

  /* It's easier to roll our own PROPFIND here than use neon's current
     interfaces. */
  /* The start of the request body is fixed: */
  body = svn_stringbuf_create
    ("<?xml version=\"1.0\" encoding=\"utf-8\"?>" DEBUG_CR
     "<propfind xmlns=\"DAV:\">" DEBUG_CR, pool);

  /* Are we asking for specific propert(y/ies), or just all of them? */
  if (which_props)
    {
      int n;
      apr_pool_t *iterpool = svn_pool_create(pool);

      svn_stringbuf_appendcstr(body, "<prop>" DEBUG_CR);
      for (n = 0; which_props[n].name != NULL; n++)
        {
          svn_pool_clear(iterpool);
          svn_stringbuf_appendcstr
            (body, apr_pstrcat(iterpool, "<", which_props[n].name, " xmlns=\"",
                               which_props[n].nspace, "\"/>" DEBUG_CR,
                               (char *)NULL));
        }
      svn_stringbuf_appendcstr(body, "</prop></propfind>" DEBUG_CR);
      svn_pool_destroy(iterpool);
    }
  else
    {
      svn_stringbuf_appendcstr(body, "<allprop/></propfind>" DEBUG_CR);
    }

  /* Initialize our baton. */
  memset(&pc, 0, sizeof(pc));
  pc.pool = pool;
  pc.propbuffer = apr_hash_make(pool);
  pc.props = apr_hash_make(pool);
  pc.cdata = svn_stringbuf_create("", pool);

  /* Create and dispatch the request! */
  SVN_ERR(svn_ra_neon__parsed_request(sess, "PROPFIND", url,
                                      body->data, 0,
                                      set_parser,
                                      start_element,
                                      svn_ra_neon__xml_collect_cdata,
                                      end_element,
                                      &pc, extra_headers, NULL, FALSE, pool));

  *results = pc.props;
  return SVN_NO_ERROR;
}

svn_error_t * svn_ra_neon__get_props_resource(svn_ra_neon__resource_t **rsrc,
                                              svn_ra_neon__session_t *sess,
                                              const char *url,
                                              const char *label,
                                              const ne_propname *which_props,
                                              apr_pool_t *pool)
{
  apr_hash_t *props;
  char * url_path = apr_pstrdup(pool, url);
  int len = strlen(url);
  /* Clean up any trailing slashes. */
  if (len > 1 && url[len - 1] == '/')
      url_path[len - 1] = '\0';

  SVN_ERR(svn_ra_neon__get_props(&props, sess, url_path, SVN_RA_NEON__DEPTH_ZERO,
                                 label, which_props, pool));

  /* ### HACK.  We need to have the client canonicalize paths, get rid
     of double slashes and such.  This check is just a check against
     non-SVN servers;  in the long run we want to re-enable this. */
  if (1 || label != NULL)
    {
      /* pick out the first response: the URL requested will not match
       * the response href. */
      apr_hash_index_t *hi = apr_hash_first(pool, props);

      if (hi)
        {
          void *ent;
          apr_hash_this(hi, NULL, NULL, &ent);
          *rsrc = ent;
        }
      else
        *rsrc = NULL;
    }
  else
    {
      *rsrc = apr_hash_get(props, url_path, APR_HASH_KEY_STRING);
    }

  if (*rsrc == NULL)
    {
      /* ### hmmm, should have been in there... */
      return svn_error_createf(APR_EGENERAL, NULL,
                               _("Failed to find label '%s' for URL '%s'"),
                               label ? label : "NULL", url_path);
    }

  return SVN_NO_ERROR;
}

svn_error_t * svn_ra_neon__get_one_prop(const svn_string_t **propval,
                                        svn_ra_neon__session_t *sess,
                                        const char *url,
                                        const char *label,
                                        const ne_propname *propname,
                                        apr_pool_t *pool)
{
  svn_ra_neon__resource_t *rsrc;
  ne_propname props[2] = { { 0 } };
  const char *name;
  const svn_string_t *value;

  props[0] = *propname;
  SVN_ERR(svn_ra_neon__get_props_resource(&rsrc, sess, url, label, props,
                                          pool));

  name = apr_pstrcat(pool, propname->nspace, propname->name, (char *)NULL);
  value = apr_hash_get(rsrc->propset, name, APR_HASH_KEY_STRING);
  if (value == NULL)
    {
      /* ### need an SVN_ERR here */
      return svn_error_createf(SVN_ERR_FS_NOT_FOUND, NULL,
                               _("'%s' was not present on the resource '%s'"),
                               name, url);
    }

  *propval = value;
  return SVN_NO_ERROR;
}

svn_error_t * svn_ra_neon__get_starting_props(svn_ra_neon__resource_t **rsrc,
                                              svn_ra_neon__session_t *sess,
                                              const char *url,
                                              apr_pool_t *pool)
{
  svn_string_t *propval;

  SVN_ERR(svn_ra_neon__get_props_resource(rsrc, sess, url, NULL,
                                          starting_props, pool));

  /* Cache some of the resource information. */

  if (! sess->vcc)
    {
      propval = apr_hash_get((*rsrc)->propset,
                             SVN_RA_NEON__PROP_VCC,
                             APR_HASH_KEY_STRING);
      if (propval)
        sess->vcc = apr_pstrdup(sess->pool, propval->data);
    }

  if (! sess->uuid)
    {
      propval = apr_hash_get((*rsrc)->propset,
                             SVN_RA_NEON__PROP_REPOSITORY_UUID,
                             APR_HASH_KEY_STRING);
      if (propval)
        sess->uuid = apr_pstrdup(sess->pool, propval->data);
    }

  if (! sess->repos_root)
    {
      propval = apr_hash_get((*rsrc)->propset,
                             SVN_RA_NEON__PROP_BASELINE_RELPATH,
                             APR_HASH_KEY_STRING);

      if (propval)
      {
        ne_uri uri;
        svn_stringbuf_t *urlbuf = svn_stringbuf_create(url, pool);

        svn_path_remove_components(urlbuf,
                                   svn_path_component_count(propval->data));

        uri = sess->root;
        uri.path = urlbuf->data;

        sess->repos_root = svn_ra_neon__uri_unparse(&uri, sess->pool);
      }
    }

  return SVN_NO_ERROR;
}



svn_error_t *
svn_ra_neon__search_for_starting_props(svn_ra_neon__resource_t **rsrc,
                                       const char **missing_path,
                                       svn_ra_neon__session_t *sess,
                                       const char *url,
                                       apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  apr_size_t len;
  svn_stringbuf_t *path_s;
  ne_uri parsed_url;
  svn_stringbuf_t *lopped_path =
    svn_stringbuf_create(url, pool); /* initialize to make sure it'll fit
                                        without reallocating */
  apr_pool_t *iterpool = svn_pool_create(pool);

  /* Split the url into its component pieces (scheme, host, path,
     etc).  We want the path part. */
  ne_uri_parse(url, &parsed_url);
  if (parsed_url.path == NULL)
    {
      ne_uri_free(&parsed_url);
      return svn_error_createf(SVN_ERR_RA_ILLEGAL_URL, NULL,
                               _("Neon was unable to parse URL '%s'"), url);
    }

  svn_stringbuf_setempty(lopped_path);
  path_s = svn_stringbuf_create(parsed_url.path, pool);
  ne_uri_free(&parsed_url);

  /* Try to get the starting_props from the public url.  If the
     resource no longer exists in HEAD or is forbidden, we'll get a
     failure.  That's fine: just keep removing components and trying
     to get the starting_props from parent directories. */
  while (! svn_path_is_empty(path_s->data))
    {
      svn_pool_clear(iterpool);
      err = svn_ra_neon__get_starting_props(rsrc, sess, path_s->data,
                                            iterpool);
      if (! err)
        break;   /* found an existing, readable parent! */

      if ((err->apr_err != SVN_ERR_FS_NOT_FOUND) &&
          (err->apr_err != SVN_ERR_RA_DAV_FORBIDDEN))
        return err;  /* found a _real_ error */

      /* else... lop off the basename and try again. */
      /* ### TODO: path_s is an absolute, schema-less URI, but
         ### technically not an FS_PATH. */
      svn_stringbuf_set(lopped_path,
                        svn_relpath_join(svn_urlpath__basename(path_s->data,
                                                               iterpool),
                                         lopped_path->data, iterpool));

      len = path_s->len;
      svn_path_remove_component(path_s);

      /* if we detect an infinite loop, get out. */
      if (path_s->len == len)
        return svn_error_quick_wrap
          (err, _("The path was not part of a repository"));

      svn_error_clear(err);
    }

  /* error out if entire URL was bogus (not a single part of it exists
     in the repository!)  */
  if (svn_path_is_empty(path_s->data))
    return svn_error_createf(SVN_ERR_RA_ILLEGAL_URL, NULL,
                             _("No part of path '%s' was found in "
                               "repository HEAD"), parsed_url.path);

  /* Duplicate rsrc out of iterpool into pool */
  {
    apr_hash_index_t *hi;
    svn_ra_neon__resource_t *tmp = apr_pcalloc(pool, sizeof(*tmp));
    tmp->url = apr_pstrdup(pool, (*rsrc)->url);
    tmp->is_collection = (*rsrc)->is_collection;
    tmp->pool = pool;
    tmp->propset = apr_hash_make(pool);

    for (hi = apr_hash_first(iterpool, (*rsrc)->propset);
         hi; hi = apr_hash_next(hi))
      {
        const void *key;
        void *val;

        apr_hash_this(hi, &key, NULL, &val);
        apr_hash_set(tmp->propset, apr_pstrdup(pool, key), APR_HASH_KEY_STRING,
                     svn_string_dup(val, pool));
      }

    *rsrc = tmp;
  }
  *missing_path = lopped_path->data;
  svn_pool_destroy(iterpool);
  return SVN_NO_ERROR;
}


svn_error_t *svn_ra_neon__get_vcc(const char **vcc,
                                  svn_ra_neon__session_t *sess,
                                  const char *url,
                                  apr_pool_t *pool)
{
  svn_ra_neon__resource_t *rsrc;
  const char *lopped_path;

  /* Look for memory-cached VCC in the RA session. */
  if (sess->vcc)
    {
      *vcc = sess->vcc;
      return SVN_NO_ERROR;
    }

  /* ### Someday, possibly look for disk-cached VCC via get_wcprop callback. */

  /* Finally, resort to a set of PROPFINDs up parent directories. */
  SVN_ERR(svn_ra_neon__search_for_starting_props(&rsrc, &lopped_path,
                                                 sess, url, pool));

  if (! sess->vcc)
    {
      /* ### better error reporting... */
      return svn_error_create(APR_EGENERAL, NULL,
                              _("The VCC property was not found on the "
                                "resource"));
    }

  *vcc = sess->vcc;
  return SVN_NO_ERROR;
}


svn_error_t *svn_ra_neon__get_baseline_props(svn_string_t *bc_relative,
                                             svn_ra_neon__resource_t **bln_rsrc,
                                             svn_ra_neon__session_t *sess,
                                             const char *url,
                                             svn_revnum_t revision,
                                             const ne_propname *which_props,
                                             apr_pool_t *pool)
{
  svn_ra_neon__resource_t *rsrc;
  const char *vcc;
  const svn_string_t *relative_path;
  const char *my_bc_relative;
  const char *lopped_path;

  /* ### we may be able to replace some/all of this code with an
     ### expand-property REPORT when that is available on the server. */

  /* -------------------------------------------------------------------
     STEP 1

     Fetch the following properties from the given URL (or, if URL no
     longer exists in HEAD, get the properties from the nearest
     still-existing parent resource):

     *) DAV:version-controlled-configuration so that we can reach the
        baseline information.

     *) svn:baseline-relative-path so that we can find this resource
        within a Baseline Collection.  If we need to search up parent
        directories, then the relative path is this property value
        *plus* any trailing components we had to chop off.

     *) DAV:resourcetype so that we can identify whether this resource
        is a collection or not -- assuming we never had to search up
        parent directories.
  */

  SVN_ERR(svn_ra_neon__search_for_starting_props(&rsrc, &lopped_path,
                                                 sess, url, pool));

  SVN_ERR(svn_ra_neon__get_vcc(&vcc, sess, url, pool));
  if (vcc == NULL)
    {
      /* ### better error reporting... */

      /* ### need an SVN_ERR here */
      return svn_error_create(APR_EGENERAL, NULL,
                              _("The VCC property was not found on the "
                                "resource"));
    }

  /* Allocate our own bc_relative path. */
  relative_path = apr_hash_get(rsrc->propset,
                               SVN_RA_NEON__PROP_BASELINE_RELPATH,
                               APR_HASH_KEY_STRING);
  if (relative_path == NULL)
    {
      /* ### better error reporting... */
      /* ### need an SVN_ERR here */
      return svn_error_create(APR_EGENERAL, NULL,
                              _("The relative-path property was not "
                                "found on the resource"));
    }

  /* don't forget to tack on the parts we lopped off in order to find
     the VCC...  We are expected to return a URI decoded relative
     path, so decode the lopped path first. */
  my_bc_relative = svn_relpath_join(relative_path->data,
                                    svn_path_uri_decode(lopped_path, pool),
                                    pool);

  /* if they want the relative path (could be, they're just trying to find
     the baseline collection), then return it */
  if (bc_relative)
    {
      bc_relative->data = my_bc_relative;
      bc_relative->len = strlen(my_bc_relative);
    }

  /* -------------------------------------------------------------------
     STEP 2

     We have the Version Controlled Configuration (VCC). From here, we
     need to reach the Baseline for specified revision.

     If the revision is SVN_INVALID_REVNUM, then we're talking about
     the HEAD revision. We have one extra step to reach the Baseline:

     *) Fetch the DAV:checked-in from the VCC; it points to the Baseline.

     If we have a specific revision, then we use a Label header when
     fetching props from the VCC. This will direct us to the Baseline
     with that label (in this case, the label == the revision number).

     From the Baseline, we fetch the following properties:

     *) DAV:baseline-collection, which is a complete tree of the Baseline
        (in SVN terms, this tree is rooted at a specific revision)

     *) DAV:version-name to get the revision of the Baseline that we are
        querying. When asking about the HEAD, this tells us its revision.
  */

  if (revision == SVN_INVALID_REVNUM)
    {
      /* Fetch the latest revision */

      const svn_string_t *baseline;

      /* Get the Baseline from the DAV:checked-in value, then fetch its
         DAV:baseline-collection property. */
      /* ### should wrap this with info about rsrc==VCC */
      SVN_ERR(svn_ra_neon__get_one_prop(&baseline, sess, vcc, NULL,
                                        &svn_ra_neon__checked_in_prop, pool));

      /* ### do we want to optimize the props we fetch, based on what the
         ### user asked for? i.e. omit version-name if latest_rev is NULL */
      SVN_ERR(svn_ra_neon__get_props_resource(&rsrc, sess,
                                              baseline->data, NULL,
                                              which_props, pool));
    }
  else
    {
      /* Fetch a specific revision */
      /* ### send Label hdr, get DAV:baseline-collection [from the baseline] */
      const char *label = apr_ltoa(pool, revision);

      /* ### do we want to optimize the props we fetch, based on what the
         ### user asked for? i.e. omit version-name if latest_rev is NULL */
      SVN_ERR(svn_ra_neon__get_props_resource(&rsrc, sess, vcc, label,
                                              which_props, pool));
    }

  /* Return the baseline rsrc, which now contains whatever set of
     props the caller wanted. */
  *bln_rsrc = rsrc;
  return SVN_NO_ERROR;
}


svn_error_t *svn_ra_neon__get_baseline_info(const char **bc_url_p,
                                            const char **bc_relative_p,
                                            svn_revnum_t *latest_rev,
                                            svn_ra_neon__session_t *sess,
                                            const char *url,
                                            svn_revnum_t revision,
                                            apr_pool_t *pool)
{
  svn_ra_neon__resource_t *baseline_rsrc;
  const svn_string_t *my_bc_url;
  svn_string_t my_bc_rel;

  /* If the server supports HTTPv2, we can bypass alot of the hard
     work here.  Otherwise, we fall back to older (less direct)
     semantics.  */
  if (SVN_RA_NEON__HAVE_HTTPV2_SUPPORT(sess))
    {
      /* Fetch youngest revision from server only if needed to construct
         baseline collection URL or return to caller. */
      if (! SVN_IS_VALID_REVNUM(revision) && (bc_url_p || latest_rev))
        {
          svn_revnum_t youngest;

          SVN_ERR(svn_ra_neon__exchange_capabilities(sess, NULL,
                                                     &youngest, pool));
          if (! SVN_IS_VALID_REVNUM(youngest))
            return svn_error_create(SVN_ERR_RA_DAV_OPTIONS_REQ_FAILED, NULL,
                                    _("The OPTIONS response did not include "
                                      "the youngest revision"));
          revision = youngest;
        }
      if (bc_url_p)
        {
          *bc_url_p = apr_psprintf(pool, "%s/%ld", sess->rev_root_stub,
                                   revision);
        }
      if (bc_relative_p)
        {
          const char *relpath = svn_uri_skip_ancestor(sess->repos_root, url,
                                                      pool);
          if (! relpath)
            return svn_error_createf(SVN_ERR_RA_REPOS_ROOT_URL_MISMATCH, NULL,
                                     _("Url '%s' is not in repository '%s'"),
                                     url, sess->repos_root);

          *bc_relative_p = relpath;
        }
      if (latest_rev)
        {
          *latest_rev = revision;
        }
      return SVN_NO_ERROR;
    }

  /* Go fetch a BASELINE_RSRC that contains specific properties we
     want.  This routine will also fill in BC_RELATIVE as best it
     can. */
  SVN_ERR(svn_ra_neon__get_baseline_props(&my_bc_rel,
                                          &baseline_rsrc,
                                          sess,
                                          url,
                                          revision,
                                          baseline_props, /* specific props */
                                          pool));

  /* baseline_rsrc now points at the Baseline. We will checkout from
     the DAV:baseline-collection.  The revision we are checking out is
     in DAV:version-name */

  /* Allocate our own copy of bc_url regardless. */
  my_bc_url = apr_hash_get(baseline_rsrc->propset,
                           SVN_RA_NEON__PROP_BASELINE_COLLECTION,
                           APR_HASH_KEY_STRING);
  if (my_bc_url == NULL)
    {
      /* ### better error reporting... */
      /* ### need an SVN_ERR here */
      return svn_error_create(APR_EGENERAL, NULL,
                              _("'DAV:baseline-collection' was not present "
                                "on the baseline resource"));
    }

  /* maybe return bc_url to the caller */
  if (bc_url_p)
    *bc_url_p = my_bc_url->data;

  if (latest_rev != NULL)
    {
      const svn_string_t *vsn_name= apr_hash_get(baseline_rsrc->propset,
                                                 SVN_RA_NEON__PROP_VERSION_NAME,
                                                 APR_HASH_KEY_STRING);
      if (vsn_name == NULL)
        {
          /* ### better error reporting... */

          /* ### need an SVN_ERR here */
          return svn_error_createf(APR_EGENERAL, NULL,
                                   _("'%s' was not present on the baseline "
                                     "resource"),
                                   "DAV:" SVN_DAV__VERSION_NAME);
        }
      *latest_rev = SVN_STR_TO_REV(vsn_name->data);
    }

  if (bc_relative_p)
    *bc_relative_p = my_bc_rel.data;

  return SVN_NO_ERROR;
}


/* Helper function for append_setprop() below. */
static svn_error_t *
get_encoding_and_cdata(const char **encoding_p,
                       const char **cdata_p,
                       const svn_string_t *value,
                       apr_pool_t *pool)
{
  const char *encoding;
  const char *xml_safe;

  /* Easy out. */
  if (value == NULL)
    {
      *encoding_p = "";
      *cdata_p = "";
      return SVN_NO_ERROR;
    }

  /* If a property is XML-safe, XML-encode it.  Else, base64-encode
     it. */
  if (svn_xml_is_xml_safe(value->data, value->len))
    {
      svn_stringbuf_t *xml_esc = NULL;
      svn_xml_escape_cdata_string(&xml_esc, value, pool);
      xml_safe = xml_esc->data;
      encoding = "";
    }
  else
    {
      const svn_string_t *base64ed = svn_base64_encode_string2(value, TRUE,
                                                               pool);
      encoding = " V:encoding=\"base64\"";
      xml_safe = base64ed->data;
    }

  *encoding_p = encoding;
  *cdata_p = xml_safe;
  return SVN_NO_ERROR;
}

/* Helper function for svn_ra_neon__do_proppatch() below. */
static svn_error_t *
append_setprop(svn_stringbuf_t *body,
               const char *name,
               const svn_string_t *const *old_value_p,
               const svn_string_t *value,
               apr_pool_t *pool)
{
  const char *encoding;
  const char *xml_safe;
  const char *xml_tag_name;
  const char *old_value_tag;

  /* Map property names to namespaces */
#define NSLEN (sizeof(SVN_PROP_PREFIX) - 1)
  if (strncmp(name, SVN_PROP_PREFIX, NSLEN) == 0)
    {
      xml_tag_name = apr_pstrcat(pool, "S:", name + NSLEN, (char *)NULL);
    }
#undef NSLEN
  else
    {
      xml_tag_name = apr_pstrcat(pool, "C:", name, (char *)NULL);
    }

  if (old_value_p)
    {
      if (*old_value_p)
        {
          const char *encoding2;
          const char *xml_safe2;
          SVN_ERR(get_encoding_and_cdata(&encoding2, &xml_safe2,
                                         *old_value_p, pool));
          old_value_tag = apr_psprintf(pool, "<%s %s>%s</%s>",
                                       "V:" SVN_DAV__OLD_VALUE, encoding2,
                                       xml_safe2, "V:" SVN_DAV__OLD_VALUE);
        }
      else
        {
#define OLD_VALUE_ABSENT_TAG \
          "<" "V:" SVN_DAV__OLD_VALUE \
              " V:" SVN_DAV__OLD_VALUE__ABSENT "=\"1\" " \
          "/>"
          old_value_tag = OLD_VALUE_ABSENT_TAG;
        }
    }
  else
    {
      old_value_tag = "";
    }

  if (old_value_p && !value)
    {
      encoding = "V:" SVN_DAV__OLD_VALUE__ABSENT "=\"1\"" ;
      xml_safe = "";
    }
  else
    {
      SVN_ERR(get_encoding_and_cdata(&encoding, &xml_safe, value, pool));
    }

  svn_stringbuf_appendcstr(body,
                           apr_psprintf(pool,"<%s %s>%s%s</%s>",
                                        xml_tag_name, encoding, old_value_tag,
                                        xml_safe, xml_tag_name));
  return SVN_NO_ERROR;
}


svn_error_t *
svn_ra_neon__do_proppatch(svn_ra_neon__session_t *ras,
                          const char *url,
                          apr_hash_t *prop_changes,
                          const apr_array_header_t *prop_deletes,
                          apr_hash_t *prop_old_values,
                          apr_hash_t *extra_headers,
                          apr_pool_t *pool)
{
  svn_error_t *err;
  svn_stringbuf_t *body;
  int code;
  apr_pool_t *subpool = svn_pool_create(pool);

  /* just punt if there are no changes to make. */
  if ((prop_changes == NULL || (! apr_hash_count(prop_changes)))
      && (prop_deletes == NULL || prop_deletes->nelts == 0)
      && (prop_old_values == NULL || (! apr_hash_count(prop_old_values))))
    return SVN_NO_ERROR;

  /* easier to roll our own PROPPATCH here than use ne_proppatch(), which
   * doesn't really do anything clever. */
  body = svn_stringbuf_create
    ("<?xml version=\"1.0\" encoding=\"utf-8\" ?>" DEBUG_CR
     "<D:propertyupdate xmlns:D=\"DAV:\" xmlns:V=\""
     SVN_DAV_PROP_NS_DAV "\" xmlns:C=\""
     SVN_DAV_PROP_NS_CUSTOM "\" xmlns:S=\""
     SVN_DAV_PROP_NS_SVN "\">" DEBUG_CR, pool);

  /* Handle property changes/deletions with expected old values. */
  if (prop_old_values)
    {
      apr_hash_index_t *hi;
      svn_stringbuf_appendcstr(body, "<D:set><D:prop>");
      for (hi = apr_hash_first(pool, prop_old_values); hi; hi = apr_hash_next(hi))
        {
          const char *name = svn__apr_hash_index_key(hi);
          svn_dav__two_props_t *both_values = svn__apr_hash_index_val(hi);
          svn_pool_clear(subpool);
          SVN_ERR(append_setprop(body, name, both_values->old_value_p,
                                 both_values->new_value, subpool));
        }
      svn_stringbuf_appendcstr(body, "</D:prop></D:set>");
    }

  /* Handle property changes. */
  if (prop_changes)
    {
      apr_hash_index_t *hi;
      svn_stringbuf_appendcstr(body, "<D:set><D:prop>");
      for (hi = apr_hash_first(pool, prop_changes); hi; hi = apr_hash_next(hi))
        {
          const void *key;
          void *val;
          svn_pool_clear(subpool);
          apr_hash_this(hi, &key, NULL, &val);
          SVN_ERR(append_setprop(body, key, NULL, val, subpool));
        }
      svn_stringbuf_appendcstr(body, "</D:prop></D:set>");
    }

  /* Handle property deletions. */
  if (prop_deletes)
    {
      int n;
      svn_stringbuf_appendcstr(body, "<D:remove><D:prop>");
      for (n = 0; n < prop_deletes->nelts; n++)
        {
          const char *name = APR_ARRAY_IDX(prop_deletes, n, const char *);
          svn_pool_clear(subpool);
          SVN_ERR(append_setprop(body, name, NULL, NULL, subpool));
        }
      svn_stringbuf_appendcstr(body, "</D:prop></D:remove>");
    }
  svn_pool_destroy(subpool);

  /* Finish up the body. */
  svn_stringbuf_appendcstr(body, "</D:propertyupdate>");

  /* Finish up the headers. */
  if (! extra_headers)
    extra_headers = apr_hash_make(pool);
  apr_hash_set(extra_headers, "Content-Type", APR_HASH_KEY_STRING,
               "text/xml; charset=UTF-8");

  err = svn_ra_neon__simple_request(&code, ras, "PROPPATCH", url,
                                    extra_headers, body->data,
                                    200, 207, pool);

  if (err && err->apr_err == SVN_ERR_RA_DAV_REQUEST_FAILED)
    switch(code)
      {
        case 423:
          return svn_error_createf(SVN_ERR_RA_NOT_LOCKED, err,
                                   _("No lock on path '%s'; "
                                     " repository is unchanged"), url);
        /* ### Add case 412 for a better error on issue #3674 */
        default:
          break;
      }

  if (err)
    return svn_error_create
      (SVN_ERR_RA_DAV_PROPPATCH_FAILED, err,
       _("At least one property change failed; repository is unchanged"));

  return SVN_NO_ERROR;
}



svn_error_t *
svn_ra_neon__do_check_path(svn_ra_session_t *session,
                           const char *path,
                           svn_revnum_t revision,
                           svn_node_kind_t *kind,
                           apr_pool_t *pool)
{
  svn_ra_neon__session_t *ras = session->priv;
  const char *url = ras->url->data;
  svn_error_t *err = SVN_NO_ERROR;
  svn_boolean_t is_dir;

  /* ### For now, using svn_ra_neon__get_starting_props() works because
     we only have three possibilities: dir, file, or none.  When we
     add symlinks, we will need to do something different.  Here's one
     way described by Greg Stein:

       That is a PROPFIND (Depth:0) for the DAV:resourcetype property.

       You can use the svn_ra_neon__get_one_prop() function to fetch
       it. If the PROPFIND fails with a 404, then you have
       svn_node_none. If the resulting property looks like:

           <D:resourcetype>
             <D:collection/>
           </D:resourcetype>

       Then it is a collection (directory; svn_node_dir). Otherwise,
       it is a regular resource (svn_node_file).

       The harder part is parsing the resourcetype property. "Proper"
       parsing means treating it as an XML property and looking for
       the DAV:collection element in there. To do that, however, means
       that get_one_prop() can't be used. I think there may be some
       Neon functions for parsing XML properties; we'd need to
       look. That would probably be the best approach. (an alternative
       is to use apr_xml_* parsing functions on the returned string;
       get back a DOM-like thing, and look for the element).
  */

  /* If we were given a relative path to append, append it. */
  if (path)
    url = svn_path_url_add_component2(url, path, pool);

  /* If we're querying HEAD, we can do so against the public URL;
     otherwise, we have to get a revision-specific URL to work with.  */
  if (SVN_IS_VALID_REVNUM(revision))
    {
      const char *bc_url;
      const char *bc_relative;

      err = svn_ra_neon__get_baseline_info(&bc_url, &bc_relative, NULL, ras,
                                           url, revision, pool);
      if (! err)
        url = svn_path_url_add_component2(bc_url, bc_relative, pool);
    }
  else
    {
      ne_uri parsed_url;

      /* svn_ra_neon__get_starting_props() wants only the path part of URL. */
      ne_uri_parse(url, &parsed_url);
      if (parsed_url.path)
        {
          url = apr_pstrdup(pool, parsed_url.path);
        }
      else
        {
          err = svn_error_createf(SVN_ERR_RA_ILLEGAL_URL, NULL,
                                  _("Neon was unable to parse URL '%s'"), url);
        }
      ne_uri_free(&parsed_url);
    }

  if (! err)
    {
      svn_ra_neon__resource_t *rsrc;
          
      /* Query the DAV:resourcetype.  */
      err = svn_ra_neon__get_starting_props(&rsrc, ras, url, pool);
      if (! err)
        is_dir = rsrc->is_collection;
    }

  if (err == SVN_NO_ERROR)
    {
      if (is_dir)
        *kind = svn_node_dir;
      else
        *kind = svn_node_file;
    }
  else if (err->apr_err == SVN_ERR_FS_NOT_FOUND)
    {

      svn_error_clear(err);
      err = SVN_NO_ERROR;
      *kind = svn_node_none;
    }

  return err;
}


svn_error_t *
svn_ra_neon__do_stat(svn_ra_session_t *session,
                     const char *path,
                     svn_revnum_t revision,
                     svn_dirent_t **dirent,
                     apr_pool_t *pool)
{
  svn_ra_neon__session_t *ras = session->priv;
  const char *url = ras->url->data;
  const char *final_url;
  apr_hash_t *resources;
  apr_hash_index_t *hi;
  svn_error_t *err;

  /* If we were given a relative path to append, append it. */
  if (path)
    url = svn_path_url_add_component2(url, path, pool);

  /* Invalid revision means HEAD, which is just the public URL. */
  if (! SVN_IS_VALID_REVNUM(revision))
    {
      final_url = url;
    }
  else
    {
      /* Else, convert (rev, path) into an opaque server-generated URL. */
      const char *bc_url;
      const char *bc_relative;

      err = svn_ra_neon__get_baseline_info(&bc_url, &bc_relative, NULL, ras,
                                           url, revision, pool);
      if (err)
        {
          if (err->apr_err == SVN_ERR_FS_NOT_FOUND)
            {
              /* easy out: */
              svn_error_clear(err);
              *dirent = NULL;
              return SVN_NO_ERROR;
            }
          else
            return err;
        }

      final_url = svn_path_url_add_component2(bc_url, bc_relative, pool);
    }

  /* Depth-zero PROPFIND is the One True DAV Way. */
  err = svn_ra_neon__get_props(&resources, ras, final_url,
                               SVN_RA_NEON__DEPTH_ZERO,
                               NULL, NULL /* all props */, pool);
  if (err)
    {
      if (err->apr_err == SVN_ERR_FS_NOT_FOUND)
        {
          /* easy out: */
          svn_error_clear(err);
          *dirent = NULL;
          return SVN_NO_ERROR;
        }
      else
        return err;
    }

  /* Copying parsing code from svn_ra_neon__get_dir() here.  The hash
     of resources only contains one item, but there's no other way to
     get the item. */
  for (hi = apr_hash_first(pool, resources); hi; hi = apr_hash_next(hi))
    {
      void *val;
      svn_ra_neon__resource_t *resource;
      const svn_string_t *propval;
      apr_hash_index_t *h;
      svn_dirent_t *entry;

      apr_hash_this(hi, NULL, NULL, &val);
      resource = val;

      entry = apr_pcalloc(pool, sizeof(*entry));

      entry->kind = resource->is_collection ? svn_node_dir : svn_node_file;

      /* entry->size is already 0 by virtue of pcalloc(). */
      if (entry->kind == svn_node_file)
        {
          propval = apr_hash_get(resource->propset,
                                 SVN_RA_NEON__PROP_GETCONTENTLENGTH,
                                 APR_HASH_KEY_STRING);
          if (propval)
            entry->size = svn__atoui64(propval->data);
        }

      /* does this resource contain any 'dead' properties? */
      for (h = apr_hash_first(pool, resource->propset);
           h; h = apr_hash_next(h))
        {
          const void *kkey;
          apr_hash_this(h, &kkey, NULL, NULL);

          if (strncmp((const char *)kkey, SVN_DAV_PROP_NS_CUSTOM,
                      sizeof(SVN_DAV_PROP_NS_CUSTOM) - 1) == 0)
            entry->has_props = TRUE;

          else if (strncmp((const char *)kkey, SVN_DAV_PROP_NS_SVN,
                           sizeof(SVN_DAV_PROP_NS_SVN) - 1) == 0)
            entry->has_props = TRUE;
        }

      /* created_rev & friends */
      propval = apr_hash_get(resource->propset,
                             SVN_RA_NEON__PROP_VERSION_NAME,
                             APR_HASH_KEY_STRING);
      if (propval != NULL)
        entry->created_rev = SVN_STR_TO_REV(propval->data);

      propval = apr_hash_get(resource->propset,
                             SVN_RA_NEON__PROP_CREATIONDATE,
                             APR_HASH_KEY_STRING);
      if (propval != NULL)
        SVN_ERR(svn_time_from_cstring(&(entry->time),
                                      propval->data, pool));

      propval = apr_hash_get(resource->propset,
                             SVN_RA_NEON__PROP_CREATOR_DISPLAYNAME,
                             APR_HASH_KEY_STRING);
      if (propval != NULL)
        entry->last_author = propval->data;

      *dirent = entry;
    }

  return SVN_NO_ERROR;
}

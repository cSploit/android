/*
 * options.c :  routines for performing OPTIONS server requests
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



#include "svn_pools.h"
#include "svn_error.h"
#include "svn_dirent_uri.h"
#include "svn_private_config.h"
#include "../libsvn_ra/ra_loader.h"

#include "private/svn_fspath.h"

#include "ra_neon.h"


/* In a debug build, setting this environment variable to "yes" will force
   the client to speak v1, even if the server is capable of speaking v2. */
#define SVN_IGNORE_V2_ENV_VAR "SVN_I_LIKE_LATENCY_SO_IGNORE_HTTPV2"

static const svn_ra_neon__xml_elm_t options_elements[] =
{
  { "DAV:", "activity-collection-set", ELEM_activity_coll_set, 0 },
  { "DAV:", "href", ELEM_href, SVN_RA_NEON__XML_CDATA },
  { "DAV:", "options-response", ELEM_options_response, 0 },

  { NULL }
};

typedef struct options_ctx_t {
  /*WARNING: WANT_CDATA should stay the first element in the baton:
    svn_ra_neon__xml_collect_cdata() assumes the baton starts with a stringbuf.
  */
  svn_stringbuf_t *want_cdata;
  svn_stringbuf_t *cdata;
  apr_pool_t *pool;
  svn_string_t *activity_coll;
} options_ctx_t;

static int
validate_element(svn_ra_neon__xml_elmid parent, svn_ra_neon__xml_elmid child)
{
  switch (parent)
    {
    case ELEM_root:
      if (child == ELEM_options_response)
        return child;
      else
        return SVN_RA_NEON__XML_INVALID;

    case ELEM_options_response:
      if (child == ELEM_activity_coll_set)
        return child;
      else
        return SVN_RA_NEON__XML_DECLINE; /* not concerned with other response */

    case ELEM_activity_coll_set:
      if (child == ELEM_href)
        return child;
      else
        return SVN_RA_NEON__XML_DECLINE; /* not concerned with unknown crud */

    default:
      return SVN_RA_NEON__XML_DECLINE;
    }

  /* NOTREACHED */
}

static svn_error_t *
start_element(int *elem, void *baton, int parent,
              const char *nspace, const char *name, const char **atts)
{
  options_ctx_t *oc = baton;
  const svn_ra_neon__xml_elm_t *elm
    = svn_ra_neon__lookup_xml_elem(options_elements, nspace, name);

  *elem = elm ? validate_element(parent, elm->id) : SVN_RA_NEON__XML_DECLINE;
  if (*elem < 1) /* Not a valid element */
    return SVN_NO_ERROR;

  if (elm->id == ELEM_href)
    oc->want_cdata = oc->cdata;
  else
    oc->want_cdata = NULL;

  return SVN_NO_ERROR;
}

static svn_error_t *
end_element(void *baton, int state,
            const char *nspace, const char *name)
{
  options_ctx_t *oc = baton;

  if (state == ELEM_href)
    oc->activity_coll =
      svn_string_create(svn_urlpath__canonicalize(oc->cdata->data, oc->pool),
                        oc->pool);

  return SVN_NO_ERROR;
}


/** Capabilities exchange. */

/* Both server and repository support the capability. */
static const char *capability_yes = "yes";
/* Either server or repository does not support the capability. */
static const char *capability_no = "no";
/* Server supports the capability, but don't yet know if repository does. */
static const char *capability_server_yes = "server-yes";


/* Store in RAS the capabilities and other interesting tidbits of
   information discovered from REQ's headers.  Use POOL for temporary
   allocation only.

   Also, if YOUNGEST_REV is not NULL, set *YOUNGEST_REV to the current
   youngest revision if we can detect that from the OPTIONS exchange, or
   to SVN_INVALID_REVNUM otherwise.  */
static void
parse_capabilities(ne_request *req,
                   svn_ra_neon__session_t *ras,
                   svn_revnum_t *youngest_rev,
                   apr_pool_t *pool)
{
  const char *val;

  if (youngest_rev)
    *youngest_rev = SVN_INVALID_REVNUM;

  /* Start out assuming all capabilities are unsupported. */
  apr_hash_set(ras->capabilities, SVN_RA_CAPABILITY_PARTIAL_REPLAY,
               APR_HASH_KEY_STRING, capability_no);
  apr_hash_set(ras->capabilities, SVN_RA_CAPABILITY_DEPTH,
               APR_HASH_KEY_STRING, capability_no);
  apr_hash_set(ras->capabilities, SVN_RA_CAPABILITY_MERGEINFO,
               APR_HASH_KEY_STRING, capability_no);
  apr_hash_set(ras->capabilities, SVN_RA_CAPABILITY_LOG_REVPROPS,
               APR_HASH_KEY_STRING, capability_no);
  apr_hash_set(ras->capabilities, SVN_RA_CAPABILITY_ATOMIC_REVPROPS,
               APR_HASH_KEY_STRING, capability_no);

  /* Then find out which ones are supported. */
  val = ne_get_response_header(req, "dav");
  if (val)
    {
      /* Multiple headers of the same name will have been merged
         together by the time we see them (either by an intermediary,
         as is permitted in HTTP, or by neon) -- merged in the sense
         that if a header "foo" appears multiple times, all the values
         will be concatenated together, with spaces at the splice
         points.  For example, if the server sent:

            DAV: 1,2
            DAV: version-control,checkout,working-resource
            DAV: merge,baseline,activity,version-controlled-collection
            DAV: http://subversion.tigris.org/xmlns/dav/svn/depth

          Here we might see:

          val == "1,2, version-control,checkout,working-resource, merge,baseline,activity,version-controlled-collection, http://subversion.tigris.org/xmlns/dav/svn/depth, <http://apache.org/dav/propset/fs/1>"

          (Deliberately not line-wrapping that, so you can see what
          we're about to parse.)
      */

      apr_array_header_t *vals =
        svn_cstring_split(val, ",", TRUE, pool);

      /* Right now we only have a few capabilities to detect, so
         just seek for them directly.  This could be written
         slightly more efficiently, but that wouldn't be worth it
         until we have many more capabilities. */

      if (svn_cstring_match_list(SVN_DAV_NS_DAV_SVN_DEPTH, vals))
        apr_hash_set(ras->capabilities, SVN_RA_CAPABILITY_DEPTH,
                     APR_HASH_KEY_STRING, capability_yes);

      if (svn_cstring_match_list(SVN_DAV_NS_DAV_SVN_MERGEINFO, vals))
        /* The server doesn't know what repository we're referring
           to, so it can't just say capability_yes. */
        apr_hash_set(ras->capabilities, SVN_RA_CAPABILITY_MERGEINFO,
                     APR_HASH_KEY_STRING, capability_server_yes);

      if (svn_cstring_match_list(SVN_DAV_NS_DAV_SVN_LOG_REVPROPS, vals))
        apr_hash_set(ras->capabilities, SVN_RA_CAPABILITY_LOG_REVPROPS,
                     APR_HASH_KEY_STRING, capability_yes);

      if (svn_cstring_match_list(SVN_DAV_NS_DAV_SVN_ATOMIC_REVPROPS, vals))
        apr_hash_set(ras->capabilities, SVN_RA_CAPABILITY_ATOMIC_REVPROPS,
                     APR_HASH_KEY_STRING, capability_yes);

      if (svn_cstring_match_list(SVN_DAV_NS_DAV_SVN_PARTIAL_REPLAY, vals))
        apr_hash_set(ras->capabilities, SVN_RA_CAPABILITY_PARTIAL_REPLAY,
                     APR_HASH_KEY_STRING, capability_yes);
    }

  /* Not strictly capabilities, but while we're here, we might as well... */
  if ((val = ne_get_response_header(req, SVN_DAV_YOUNGEST_REV_HEADER)))
    {
      if (youngest_rev)
        *youngest_rev = SVN_STR_TO_REV(val);
    }
  if ((val = ne_get_response_header(req, SVN_DAV_REPOS_UUID_HEADER)))
    {
      ras->uuid = apr_pstrdup(ras->pool, val);
    }
  if ((val = ne_get_response_header(req, SVN_DAV_ROOT_URI_HEADER)))
    {
      ne_uri root_uri = ras->root;

      root_uri.path = (char *)val;
      ras->repos_root = svn_ra_neon__uri_unparse(&root_uri, ras->pool);
    }

  /* HTTP v2 stuff */
  if ((val = ne_get_response_header(req, SVN_DAV_ME_RESOURCE_HEADER)))
    {
#ifdef SVN_DEBUG
      char *ignore_v2_env_var = getenv(SVN_IGNORE_V2_ENV_VAR);

      if (! (ignore_v2_env_var
             && apr_strnatcasecmp(ignore_v2_env_var, "yes") == 0))
        ras->me_resource = apr_pstrdup(ras->pool, val);
#else
      ras->me_resource = apr_pstrdup(ras->pool, val);
#endif
    }
  if ((val = ne_get_response_header(req, SVN_DAV_REV_ROOT_STUB_HEADER)))
    {
      ras->rev_root_stub = apr_pstrdup(ras->pool, val);
    }
  if ((val = ne_get_response_header(req, SVN_DAV_REV_STUB_HEADER)))
    {
      ras->rev_stub = apr_pstrdup(ras->pool, val);
    }
  if ((val = ne_get_response_header(req, SVN_DAV_TXN_ROOT_STUB_HEADER)))
    {
      ras->txn_root_stub = apr_pstrdup(ras->pool, val);
    }
  if ((val = ne_get_response_header(req, SVN_DAV_TXN_STUB_HEADER)))
    {
      ras->txn_stub = apr_pstrdup(ras->pool, val);
    }
  if ((val = ne_get_response_header(req, SVN_DAV_VTXN_ROOT_STUB_HEADER)))
    {
      ras->vtxn_root_stub = apr_pstrdup(ras->pool, val);
    }
  if ((val = ne_get_response_header(req, SVN_DAV_VTXN_STUB_HEADER)))
    {
      ras->vtxn_stub = apr_pstrdup(ras->pool, val);
    }
}


svn_error_t *
svn_ra_neon__exchange_capabilities(svn_ra_neon__session_t *ras,
                                   const char **relocation_location,
                                   svn_revnum_t *youngest_rev,
                                   apr_pool_t *pool)
{
  svn_ra_neon__request_t* req;
  svn_error_t *err = SVN_NO_ERROR;
  ne_xml_parser *parser = NULL;
  options_ctx_t oc = { 0 };
  int status_code;

  oc.pool = pool;
  oc.cdata = svn_stringbuf_create("", pool);

  if (youngest_rev)
    *youngest_rev = SVN_INVALID_REVNUM;
  if (relocation_location)
    *relocation_location = NULL;

  SVN_ERR(svn_ra_neon__request_create(&req, ras, "OPTIONS", ras->url->data,
                                      pool));

  /* ### Use a symbolic name somewhere for this MIME type? */
  ne_add_request_header(req->ne_req, "Content-Type", "text/xml");

  /* Create a parser to read the normal response body */
  parser = svn_ra_neon__xml_parser_create(req, ne_accept_2xx, start_element,
                                          svn_ra_neon__xml_collect_cdata,
                                          end_element, &oc);

  /* Run the request and get the resulting status code. */
  if ((err = svn_ra_neon__request_dispatch(&status_code, req, NULL,
                                           "<?xml version=\"1.0\" "
                                           "encoding=\"utf-8\"?>"
                                           "<D:options xmlns:D=\"DAV:\">"
                                           "<D:activity-collection-set/>"
                                           "</D:options>",
                                           200,
                                           relocation_location ? 301 : 0,
                                           pool)))
    goto cleanup;

  if (req->code == 301)
    {
      *relocation_location = svn_ra_neon__request_get_location(req, pool);
      goto cleanup;
    }

  /* Was there an XML parse error somewhere? */
  err = svn_ra_neon__check_parse_error("OPTIONS", parser, ras->url->data);
  if (err)
    goto cleanup;

  /* We asked for, and therefore expect, to have found an activity
     collection in the response.  */
  if (oc.activity_coll == NULL)
    {
      err = svn_error_create(SVN_ERR_RA_DAV_OPTIONS_REQ_FAILED, NULL,
                             _("The OPTIONS response did not include the "
                               "requested activity-collection-set; this often "
                               "means that the URL is not WebDAV-enabled"));
      goto cleanup;
    }

  ras->act_coll = apr_pstrdup(ras->pool, oc.activity_coll->data);
  parse_capabilities(req->ne_req, ras, youngest_rev, pool);

 cleanup:
  svn_ra_neon__request_destroy(req);

  return err;
}


svn_error_t *
svn_ra_neon__get_activity_collection(const svn_string_t **activity_coll,
                                     svn_ra_neon__session_t *ras,
                                     apr_pool_t *pool)
{
  if (! ras->act_coll)
    SVN_ERR(svn_ra_neon__exchange_capabilities(ras, NULL, NULL, pool));
  *activity_coll = svn_string_create(ras->act_coll, pool);
  return SVN_NO_ERROR;
}


svn_error_t *
svn_ra_neon__has_capability(svn_ra_session_t *session,
                            svn_boolean_t *has,
                            const char *capability,
                            apr_pool_t *pool)
{
  svn_ra_neon__session_t *ras = session->priv;
  const char *cap_result;

  /* This capability doesn't rely on anything server side. */
  if (strcmp(capability, SVN_RA_CAPABILITY_COMMIT_REVPROPS) == 0)
    {
      *has = TRUE;
      return SVN_NO_ERROR;
    }

 cap_result = apr_hash_get(ras->capabilities,
                           capability,
                           APR_HASH_KEY_STRING);

  /* If any capability is unknown, they're all unknown, so ask. */
  if (cap_result == NULL)
    {
      SVN_ERR(svn_ra_neon__exchange_capabilities(ras, NULL, NULL, pool));
    }


  /* Try again, now that we've fetched the capabilities. */
  cap_result = apr_hash_get(ras->capabilities,
                            capability, APR_HASH_KEY_STRING);

  /* Some capabilities depend on the repository as well as the server.
     NOTE: svn_ra_serf__has_capability() has a very similar code block.  If
     you change something here, check there as well. */
  if (cap_result == capability_server_yes)
    {
      if (strcmp(capability, SVN_RA_CAPABILITY_MERGEINFO) == 0)
        {
          /* Handle mergeinfo specially.  Mergeinfo depends on the
             repository as well as the server, but the server routine
             that answered our svn_ra_neon__exchange_capabilities() call
             above didn't even know which repository we were interested in
             -- it just told us whether the server supports mergeinfo.
             If the answer was 'no', there's no point checking the
             particular repository; but if it was 'yes', we still must
             change it to 'no' iff the repository itself doesn't
             support mergeinfo. */
          svn_mergeinfo_catalog_t ignored;
          svn_error_t *err;
          apr_array_header_t *paths = apr_array_make(pool, 1,
                                                     sizeof(char *));
          APR_ARRAY_PUSH(paths, const char *) = "";

          err = svn_ra_neon__get_mergeinfo(session, &ignored, paths, 0,
                                           FALSE, FALSE, pool);

          if (err)
            {
              if (err->apr_err == SVN_ERR_UNSUPPORTED_FEATURE)
                {
                  svn_error_clear(err);
                  cap_result = capability_no;
                }
              else if (err->apr_err == SVN_ERR_FS_NOT_FOUND)
                {
                  /* Mergeinfo requests use relative paths, and
                     anyway we're in r0, so this is a likely error,
                     but it means the repository supports mergeinfo! */
                  svn_error_clear(err);
                  cap_result = capability_yes;
                }
              else
                return err;

            }
          else
            cap_result = capability_yes;

            apr_hash_set(ras->capabilities,
                         SVN_RA_CAPABILITY_MERGEINFO, APR_HASH_KEY_STRING,
                         cap_result);
        }
      else
        {
          return svn_error_createf
            (SVN_ERR_UNKNOWN_CAPABILITY, NULL,
             _("Don't know how to handle '%s' for capability '%s'"),
             capability_server_yes, capability);
        }
    }

  if (cap_result == capability_yes)
    {
      *has = TRUE;
    }
  else if (cap_result == capability_no)
    {
      *has = FALSE;
    }
  else if (cap_result == NULL)
    {
      return svn_error_createf
        (SVN_ERR_UNKNOWN_CAPABILITY, NULL,
         _("Don't know anything about capability '%s'"), capability);
    }
  else  /* "can't happen" */
    {
      /* Well, let's hope it's a string. */
      return svn_error_createf
        (SVN_ERR_RA_DAV_OPTIONS_REQ_FAILED, NULL,
         _("Attempt to fetch capability '%s' resulted in '%s'"),
         capability, cap_result);
    }

  return SVN_NO_ERROR;
}

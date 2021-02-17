/*
 * merge.c :  routines for performing a MERGE server requests
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
#include <apr_hash.h>
#define APR_WANT_STRFUNC
#include <apr_want.h>

#include "svn_string.h"
#include "svn_error.h"
#include "svn_path.h"
#include "svn_ra.h"
#include "svn_pools.h"
#include "svn_props.h"
#include "svn_xml.h"

#include "private/svn_dav_protocol.h"
#include "private/svn_fspath.h"

#include "svn_private_config.h"

#include "ra_neon.h"


static const svn_ra_neon__xml_elm_t merge_elements[] =
{
  { "DAV:", "updated-set", ELEM_updated_set, 0 },
  { "DAV:", "merged-set", ELEM_merged_set, 0 },
  { "DAV:", "ignored-set", ELEM_ignored_set, 0 },
  { "DAV:", "href", ELEM_href, SVN_RA_NEON__XML_CDATA },
  { "DAV:", "merge-response", ELEM_merge_response, 0 },
  { "DAV:", "checked-in", ELEM_checked_in, 0 },
  { "DAV:", "response", ELEM_response, 0 },
  { "DAV:", "propstat", ELEM_propstat, 0 },
  { "DAV:", "status", ELEM_status, SVN_RA_NEON__XML_CDATA },
  { "DAV:", "responsedescription", ELEM_responsedescription,
    SVN_RA_NEON__XML_CDATA },
  { "DAV:", "prop", ELEM_prop, 0 },
  { "DAV:", "resourcetype", ELEM_resourcetype, 0 },
  { "DAV:", "collection", ELEM_collection, 0 },
  { "DAV:", "baseline", ELEM_baseline, 0 },
  { "DAV:", SVN_DAV__VERSION_NAME, ELEM_version_name, SVN_RA_NEON__XML_CDATA },
  { SVN_XML_NAMESPACE, "post-commit-err",
    ELEM_post_commit_err, SVN_RA_NEON__XML_CDATA },
  { "DAV:", SVN_DAV__CREATIONDATE, ELEM_creationdate, SVN_RA_NEON__XML_CDATA },
  { "DAV:", "creator-displayname", ELEM_creator_displayname,
    SVN_RA_NEON__XML_CDATA },

  { NULL }
};

enum merge_rtype {
  RTYPE_UNKNOWN,    /* unknown (haven't seen it in the response yet) */
  RTYPE_REGULAR,    /* a regular (member) resource */
  RTYPE_COLLECTION, /* a collection resource */
  RTYPE_BASELINE    /* a baseline resource */
};

typedef struct merge_ctx_t {
  /*WARNING: WANT_CDATA should stay the first element in the baton:
    svn_ra_neon__xml_collect_cdata() assumes the baton starts with a stringbuf.
  */
  svn_stringbuf_t *want_cdata;
  svn_stringbuf_t *cdata;
  apr_pool_t *pool;

  /* a clearable subpool of pool, for loops.  Do not use for anything
     that must persist beyond the scope of your function! */
  apr_pool_t *scratchpool;

  /* the BASE_HREF contains the merge target. as resources are specified in
     the merge response, we make their URLs relative to this URL, thus giving
     us a path for use in the commit callbacks. */
  const char *base_href;

  svn_revnum_t rev;        /* the new/target revision number for this commit */

  svn_boolean_t response_has_error;
  int response_parent;     /* what element did DAV:response appear within? */

  int href_parent;         /* what element is the DAV:href appearing within? */
  svn_stringbuf_t *href;   /* current response */

  int status;              /* HTTP status for this DAV:propstat */
  enum merge_rtype rtype;  /* DAV:resourcetype of this resource */

  svn_stringbuf_t *vsn_name;       /* DAV:version-name for this resource */
  svn_stringbuf_t *vsn_url;        /* DAV:checked-in for this resource */
  svn_stringbuf_t *committed_date; /* DAV:creationdate for this resource */
  svn_stringbuf_t *last_author;    /* DAV:creator-displayname for this
                                      resource */
  svn_stringbuf_t *post_commit_err;/* SVN_XML_NAMESPACE:post-commit hook's
                                      stderr */

  /* We only invoke set_prop() on targets listed in valid_targets.
     Some entities (such as directories that have had changes
     committed underneath but are not themselves targets) will be
     mentioned in the merge response but not appear in
     valid_targets. */
  apr_hash_t *valid_targets;

  /* Client callbacks */
  svn_ra_push_wc_prop_func_t push_prop;
  void *cb_baton;  /* baton for above */

} merge_ctx_t;


static void add_ignored(merge_ctx_t *mc, const char *cdata)
{
  /* ### the server didn't check in the file(!) */
  /* ### remember the file and issue a report/warning later */
}


static svn_boolean_t okay_to_bump_path(const char *path,
                                       apr_hash_t *valid_targets,
                                       apr_pool_t *pool)
{
  svn_stringbuf_t *parent_path;
  enum svn_recurse_kind r;

  /* Easy check:  if path itself is in the hash, then it's legit. */
  if (apr_hash_get(valid_targets, path, APR_HASH_KEY_STRING))
    return TRUE;
  /* Otherwise, this path is bumpable IFF one of its parents is in the
     hash and marked with a 'recursion' flag. */
  parent_path = svn_stringbuf_create(path, pool);

  do {
    apr_size_t len = parent_path->len;
    svn_path_remove_component(parent_path);
    if (len == parent_path->len)
      break;
    r = (enum svn_recurse_kind) apr_hash_get(valid_targets,
                                             parent_path->data,
                                             APR_HASH_KEY_STRING);
    if (r == svn_recursive)
      return TRUE;

  } while (! svn_path_is_empty(parent_path->data));

  /* Default answer: if we get here, don't allow the bumping. */
  return FALSE;
}


/* If committed PATH appears in MC->valid_targets, and an MC->push_prop
 * function exists, then store VSN_URL as the SVN_RA_NEON__LP_VSN_URL
 * property on PATH.  Use POOL for all allocations.
 *
 * Otherwise, just return SVN_NO_ERROR.
 */
static svn_error_t *bump_resource(merge_ctx_t *mc,
                                  const char *path,
                                  char *vsn_url,
                                  apr_pool_t *pool)
{
  /* no sense in doing any more work if there's no property setting
     function at our disposal. */
  if (mc->push_prop == NULL)
    return SVN_NO_ERROR;

  /* Only invoke a client callback on PATH if PATH counts as a
     committed target.  The commit-tracking editor built this list for
     us, and took care not to include directories unless they were
     directly committed (i.e., received a property change). */
  if (! okay_to_bump_path(path, mc->valid_targets, pool))
    return SVN_NO_ERROR;

  /* Okay, NOW set the new version url. */
  {
    svn_string_t vsn_url_str;  /* prop setter wants an svn_string_t */

    vsn_url_str.data = vsn_url;
    vsn_url_str.len = strlen(vsn_url);

    return (*mc->push_prop)(mc->cb_baton, path,
                            SVN_RA_NEON__LP_VSN_URL, &vsn_url_str,
                            pool);
  }
}

static svn_error_t * handle_resource(merge_ctx_t *mc,
                                     apr_pool_t *pool)
{
  const char *relative;

  if (mc->response_has_error)
    {
      /* ### what to do? */
      /* ### return "no error", presuming whatever set response_has_error
         ### has already handled the problem. */
      return SVN_NO_ERROR;
    }
  if (mc->response_parent == ELEM_merged_set)
    {
      /* ### shouldn't have happened. we told the server "don't merge" */
      /* ### need something better than APR_EGENERAL */
      return svn_error_createf(APR_EGENERAL, NULL,
                               _("Protocol error: we told the server not to "
                                 "auto-merge any resources, but it said that "
                                 "'%s' was merged"), mc->href->data);
    }
  if (mc->response_parent != ELEM_updated_set)
    {
      /* ### unknown parent for this response(!) */
      /* ### need something better than APR_EGENERAL */
      return svn_error_createf(APR_EGENERAL, NULL,
                               _("Internal error: there is an unknown parent "
                                 "(%d) for the 'DAV:response' element within"
                                 " the MERGE response"), mc->response_parent);
    }
#if 0
  /* ### right now, the server isn't sending everything for all resources.
     ### just skip this requirement. */
  if (mc->href->len == 0
      || mc->vsn_name->len == 0
      || mc->vsn_url->len == 0
      || mc->rtype == RTYPE_UNKNOWN)
    {
      /* one or more properties were missing in the DAV:response for the
         resource. */
      return svn_error_createf(APR_EGENERAL, NULL,
                               _("Protocol error: the MERGE response for the "
                                 "'%s' resource did not return all of the "
                                 "properties that we asked for (and need to "
                                 "complete the commit)"), mc->href->data);
    }
#endif

  if (mc->rtype == RTYPE_BASELINE)
    {
      /* cool. the DAV:version-name tells us the new revision */
      mc->rev = SVN_STR_TO_REV(mc->vsn_name->data);
      return SVN_NO_ERROR;
    }

  /* a collection or regular resource */
  if (! svn_urlpath__is_ancestor(mc->base_href, mc->href->data))
    {
      /* ### need something better than APR_EGENERAL */
      return svn_error_createf(APR_EGENERAL, NULL,
                               _("A MERGE response for '%s' is not a child "
                                 "of the destination ('%s')"),
                               mc->href->data, mc->base_href);
    }

  /* given HREF of the form: BASE "/" RELATIVE, extract the relative portion */
  relative = svn_urlpath__is_child(mc->base_href, mc->href->data, NULL);
  if (! relative) /* the paths are equal */
    relative = "";

  /* bump the resource */
  relative = svn_path_uri_decode(relative, pool);
  return bump_resource(mc, relative, mc->vsn_url->data, pool);
}

/* Determine whether we're receiving the expected XML response.
   Return CHILD when interested in receiving the child's contents
   or one of SVN_RA_NEON__XML_INVALID and SVN_RA_NEON__XML_DECLINE
   when respectively this is the incorrect response or
   the element (and its children) are uninteresting */
static int validate_element(svn_ra_neon__xml_elmid parent,
                            svn_ra_neon__xml_elmid child)
{
  if ((child == ELEM_collection || child == ELEM_baseline)
      && parent != ELEM_resourcetype) {
    /* ### technically, they could occur elsewhere, but screw it */
    return SVN_RA_NEON__XML_INVALID;
  }

  switch (parent)
    {
    case ELEM_root:
      if (child == ELEM_merge_response)
        return child;
      else
        return SVN_RA_NEON__XML_INVALID;

    case ELEM_merge_response:
      if (child == ELEM_updated_set
          || child == ELEM_merged_set
          || child == ELEM_ignored_set)
        return child;
      else
        return SVN_RA_NEON__XML_DECLINE; /* any child is allowed */

    case ELEM_updated_set:
    case ELEM_merged_set:
      if (child == ELEM_response)
        return child;
      else
        return SVN_RA_NEON__XML_DECLINE; /* ignore if something else
                                           was in there */

    case ELEM_ignored_set:
      if (child == ELEM_href)
        return child;
      else
        return SVN_RA_NEON__XML_DECLINE; /* ignore if something else
                                           was in there */

    case ELEM_response:
      if (child == ELEM_href
          || child == ELEM_status
          || child == ELEM_propstat)
        return child;
      else if (child == ELEM_responsedescription)
        /* ### I think we want this... to save a message for the user */
        return SVN_RA_NEON__XML_DECLINE; /* valid, but we don't need to see it */
      else
        return SVN_RA_NEON__XML_DECLINE; /* ignore if something else
                                           was in there */

    case ELEM_propstat:
      if (child == ELEM_prop || child == ELEM_status)
        return child;
      else if (child == ELEM_responsedescription)
        /* ### I think we want this... to save a message for the user */
        return SVN_RA_NEON__XML_DECLINE; /* valid, but we don't need to see it */
      else
        return SVN_RA_NEON__XML_DECLINE; /* ignore if something else
                                           was in there */

    case ELEM_prop:
      if (child == ELEM_checked_in
          || child == ELEM_resourcetype
          || child == ELEM_version_name
          || child == ELEM_creationdate
          || child == ELEM_creator_displayname
          || child == ELEM_post_commit_err
          /* other props */)
        return child;
      else
        return SVN_RA_NEON__XML_DECLINE; /* ignore other props */

    case ELEM_checked_in:
      if (child == ELEM_href)
        return child;
      else
        return SVN_RA_NEON__XML_DECLINE; /* ignore if something else
                                           was in there */

    case ELEM_resourcetype:
      if (child == ELEM_collection || child == ELEM_baseline)
        return child;
      else
        return SVN_RA_NEON__XML_DECLINE; /* ignore if something else
                                           was in there */

    default:
      return SVN_RA_NEON__XML_DECLINE;
    }

  /* NOTREACHED */
}

static svn_error_t *
start_element(int *elem, void *baton, int parent,
              const char *nspace, const char *name, const char **atts)
{
  const svn_ra_neon__xml_elm_t *elm
    = svn_ra_neon__lookup_xml_elem(merge_elements, nspace, name);
  merge_ctx_t *mc = baton;

  *elem = elm ? validate_element(parent, elm->id) : SVN_RA_NEON__XML_DECLINE;
  if (*elem < 1) /* not a valid element */
    return SVN_NO_ERROR;

  switch (elm->id)
    {
    case ELEM_response:
      mc->response_has_error = FALSE;

      /* for each response (which corresponds to one resource), note that we
         haven't seen its resource type yet */
      mc->rtype = RTYPE_UNKNOWN;

      /* and we haven't seen these elements yet */
      mc->href->len = 0;
      mc->vsn_name->len = 0;
      mc->vsn_url->len = 0;

      /* FALLTHROUGH */

    case ELEM_ignored_set:
    case ELEM_checked_in:
      /* if we see an href "soon", then its parent is ELM */
      mc->href_parent = elm->id;
      break;

    case ELEM_updated_set:
    case ELEM_merged_set:
      mc->response_parent = elm->id;
      break;

    case ELEM_propstat:
      /* initialize the status so we can figure out if we ever saw a
         status element in the propstat */
      mc->status = 0;
      break;

    case ELEM_resourcetype:
      /* we've seen a DAV:resourcetype, so it will be "regular" unless we
         see something within this element */
      mc->rtype = RTYPE_REGULAR;
      break;

    case ELEM_collection:
      mc->rtype = RTYPE_COLLECTION;
      break;

    case ELEM_baseline:
      mc->rtype = RTYPE_BASELINE;
      break;

    default:
      /* one of: ELEM_href, ELEM_status, ELEM_prop,
         ELEM_version_name */
      break;
    }

  switch (elm->id)
    {
    case ELEM_href:
    case ELEM_status:
    case ELEM_version_name:
    case ELEM_post_commit_err:
    case ELEM_creationdate:
    case ELEM_creator_displayname:
      mc->want_cdata = mc->cdata;
      svn_stringbuf_setempty(mc->cdata);
      break;

    default:
      mc->want_cdata = NULL;
      break;
    }


  return SVN_NO_ERROR;
}

static svn_error_t *
end_element(void *baton, int state,
            const char *nspace, const char *name)
{
  merge_ctx_t *mc = baton;

  switch (state)
    {
    case ELEM_href:
      switch (mc->href_parent)
        {
        case ELEM_ignored_set:
          add_ignored(mc, mc->cdata->data);
          break;

        case ELEM_response:
          /* we're now working on this href... */
          SVN_ERR(svn_ra_neon__copy_href(mc->href, mc->cdata->data,
                                         mc->scratchpool));
          break;

        case ELEM_checked_in:
          SVN_ERR(svn_ra_neon__copy_href(mc->vsn_url, mc->cdata->data,
                                         mc->scratchpool));
          break;
        }
      break;

    case ELEM_responsedescription:
      /* ### I don't think we'll see this right now, due to validate_element */
      /* ### remember this for error messages? */
      break;

    case ELEM_status:
      {
        ne_status hs;

        if (ne_parse_statusline(mc->cdata->data, &hs) != 0)
          mc->response_has_error = TRUE;
        else
          {
            mc->status = hs.code;
            if (hs.code != 200)
              {
                /* ### create an error structure? */
                mc->response_has_error = TRUE;
              }
            free(hs.reason_phrase);
          }
        if (mc->response_has_error)
          {
            /* ### fix this error value */
            return svn_error_create(APR_EGENERAL, NULL,
                                    _("The MERGE property response had an "
                                      "error status"));
          }
      }
      break;

    case ELEM_propstat:
      /* ### does Neon have a symbol for 200? */
      if (mc->status == 200 /* OK */)
        {
          /* ### what to do? reset all the data? */
        }
      /* ### else issue an error? status==0 means we never saw one */
      break;

    case ELEM_response:
      {
        /* the end of a DAV:response means that we've seen all the information
           related to this resource. process it. */
        SVN_ERR(handle_resource(mc, mc->scratchpool));
        svn_pool_clear(mc->scratchpool);
      }
      break;

    case ELEM_checked_in:
      /* When we leave a DAV:checked-in element, the parents are DAV:prop,
         DAV:propstat, then DAV:response. If we see a DAV:href "on the way
         out", then it is going to belong to the DAV:response. */
      mc->href_parent = ELEM_response;
      break;

    case ELEM_version_name:
      svn_stringbuf_set(mc->vsn_name, mc->cdata->data);
      break;

    case ELEM_post_commit_err:
      svn_stringbuf_set(mc->post_commit_err, mc->cdata->data);
      break;

    case ELEM_creationdate:
      svn_stringbuf_set(mc->committed_date, mc->cdata->data);
      break;

    case ELEM_creator_displayname:
      svn_stringbuf_set(mc->last_author, mc->cdata->data);
      break;

    default:
      /* one of: ELEM_updated_set, ELEM_merged_set, ELEM_ignored_set,
         ELEM_prop, ELEM_resourcetype, ELEM_collection, ELEM_baseline */
      break;
    }

  return SVN_NO_ERROR;
}


svn_error_t * svn_ra_neon__assemble_locktoken_body(svn_stringbuf_t **body,
                                                   apr_hash_t *lock_tokens,
                                                   apr_pool_t *pool)
{
  apr_hash_index_t *hi;
  apr_size_t buf_size;
  const char *closing_tag = "</S:lock-token-list>";
  apr_size_t closing_tag_size = strlen(closing_tag);
  apr_pool_t *tmppool = svn_pool_create(pool);
  apr_hash_t *xml_locks = apr_hash_make(tmppool);
  svn_stringbuf_t *lockbuf = svn_stringbuf_create
    ("<S:lock-token-list xmlns:S=\"" SVN_XML_NAMESPACE "\">" DEBUG_CR, pool);

  buf_size = lockbuf->len;

#define SVN_LOCK "<S:lock>" DEBUG_CR
#define SVN_LOCK_LEN sizeof(SVN_LOCK)-1
#define SVN_LOCK_CLOSE "</S:lock>" DEBUG_CR
#define SVN_LOCK_CLOSE_LEN sizeof(SVN_LOCK_CLOSE)-1
#define SVN_LOCK_PATH "<S:lock-path>"
#define SVN_LOCK_PATH_LEN sizeof(SVN_LOCK_PATH)-1
#define SVN_LOCK_PATH_CLOSE "</S:lock-path>" DEBUG_CR
#define SVN_LOCK_PATH_CLOSE_LEN sizeof(SVN_LOCK_CLOSE)-1
#define SVN_LOCK_TOKEN "<S:lock-token>"
#define SVN_LOCK_TOKEN_LEN sizeof(SVN_LOCK_TOKEN)-1
#define SVN_LOCK_TOKEN_CLOSE "</S:lock-token>" DEBUG_CR
#define SVN_LOCK_TOKEN_CLOSE_LEN sizeof(SVN_LOCK_TOKEN_CLOSE)-1

  /* First, figure out how much string data we're talking about,
     and allocate a stringbuf big enough to hold it all... we *never*
     want have the stringbuf do an auto-reallocation.  While here,
     we'll be copying our hash of paths -> tokens into a hash of
     xml-escaped-paths -> tokens.  */
  for (hi = apr_hash_first(tmppool, lock_tokens); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      void *val;
      apr_ssize_t klen;
      svn_string_t lock_path;
      svn_stringbuf_t *lock_path_xml = NULL;

      apr_hash_this(hi, &key, &klen, &val);

      /* XML-escape our key and store it in our temporary hash. */
      lock_path.data = key;
      lock_path.len = klen;
      svn_xml_escape_cdata_string(&lock_path_xml, &lock_path, tmppool);
      apr_hash_set(xml_locks, lock_path_xml->data, lock_path_xml->len, val);

      /* Now, on with the stringbuf calculations. */
      buf_size += SVN_LOCK_LEN;
      buf_size += SVN_LOCK_PATH_LEN;
      buf_size += lock_path_xml->len;
      buf_size += SVN_LOCK_PATH_CLOSE_LEN;
      buf_size += SVN_LOCK_TOKEN_LEN;
      buf_size += strlen(val);
      buf_size += SVN_LOCK_TOKEN_CLOSE_LEN;
      buf_size += SVN_LOCK_CLOSE_LEN;
    }

  buf_size += closing_tag_size;

  svn_stringbuf_ensure(lockbuf, buf_size + 1);

  /* Now append all the temporary hash's keys and values into the
     stringbuf.  This is better than doing apr_pstrcat() in a loop,
     because (1) there's no need to constantly re-alloc, and (2) the
     stringbuf already knows the end of the buffer, so there's no
     seek-time to the end of the string when appending. */
  for (hi = apr_hash_first(tmppool, xml_locks); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      apr_ssize_t klen;
      void *val;

      apr_hash_this(hi, &key, &klen, &val);

      svn_stringbuf_appendcstr(lockbuf, SVN_LOCK);
      svn_stringbuf_appendcstr(lockbuf, SVN_LOCK_PATH);
      svn_stringbuf_appendbytes(lockbuf, key, klen);
      svn_stringbuf_appendcstr(lockbuf, SVN_LOCK_PATH_CLOSE);
      svn_stringbuf_appendcstr(lockbuf, SVN_LOCK_TOKEN);
      svn_stringbuf_appendcstr(lockbuf, val);
      svn_stringbuf_appendcstr(lockbuf, SVN_LOCK_TOKEN_CLOSE);
      svn_stringbuf_appendcstr(lockbuf, SVN_LOCK_CLOSE);
    }

  svn_stringbuf_appendcstr(lockbuf, closing_tag);

#undef SVN_LOCK
#undef SVN_LOCK_LEN
#undef SVN_LOCK_CLOSE
#undef SVN_LOCK_CLOSE_LEN
#undef SVN_LOCK_PATH
#undef SVN_LOCK_PATH_LEN
#undef SVN_LOCK_PATH_CLOSE
#undef SVN_LOCK_PATH_CLOSE_LEN
#undef SVN_LOCK_TOKEN
#undef SVN_LOCK_TOKEN_LEN
#undef SVN_LOCK_TOKEN_CLOSE
#undef SVN_LOCK_TOKEN_CLOSE_LEN

  *body = lockbuf;

  svn_pool_destroy(tmppool);
  return SVN_NO_ERROR;
}


/* ### FIXME: As of HTTPv2, this isn't necessarily merging an
   ### "activity".  It might be merging a transaction.  So,
   ### ACTIVITY_URL might be a transaction root URL, not an actual
   ### activity URL, etc.  Probably should rename ACTIVITY_URL to
   ### MERGE_RESOURCE_URL or something.  */
svn_error_t * svn_ra_neon__merge_activity(svn_revnum_t *new_rev,
                                          const char **committed_date,
                                          const char **committed_author,
                                          const char **post_commit_err,
                                          svn_ra_neon__session_t *ras,
                                          const char *repos_url,
                                          const char *activity_url,
                                          apr_hash_t *valid_targets,
                                          apr_hash_t *lock_tokens,
                                          svn_boolean_t keep_locks,
                                          svn_boolean_t disable_merge_response,
                                          apr_pool_t *pool)
{
  merge_ctx_t mc = { 0 };
  const char *body;
  apr_hash_t *extra_headers = NULL;
  svn_stringbuf_t *lockbuf = svn_stringbuf_create("", pool);

  mc.cdata = svn_stringbuf_create("", pool);
  mc.pool = pool;
  mc.scratchpool = svn_pool_create(pool);
  mc.base_href = repos_url;
  mc.rev = SVN_INVALID_REVNUM;

  mc.valid_targets = valid_targets;
  mc.push_prop = ras->callbacks->push_wc_prop;
  mc.cb_baton = ras->callback_baton;

  mc.href = MAKE_BUFFER(pool);
  mc.vsn_name = MAKE_BUFFER(pool);
  mc.vsn_url = MAKE_BUFFER(pool);
  mc.committed_date = MAKE_BUFFER(pool);
  mc.last_author = MAKE_BUFFER(pool);
  if (post_commit_err)
    mc.post_commit_err = MAKE_BUFFER(pool);

  if (disable_merge_response
      || (! keep_locks))
    {
      const char *value;

      value = apr_psprintf(pool, "%s %s",
                           disable_merge_response ?
                              SVN_DAV_OPTION_NO_MERGE_RESPONSE : "",
                           keep_locks ?
                              "" : SVN_DAV_OPTION_RELEASE_LOCKS);

      if (! extra_headers)
        extra_headers = apr_hash_make(pool);
      apr_hash_set(extra_headers, SVN_DAV_OPTIONS_HEADER, APR_HASH_KEY_STRING,
                   value);
    }

  /* Need to marshal the whole [path->token] hash to the server as
     a string within the body of the MERGE request. */
  if ((lock_tokens != NULL)
      && (apr_hash_count(lock_tokens) > 0))
    SVN_ERR(svn_ra_neon__assemble_locktoken_body(&lockbuf, lock_tokens, pool));

  body = apr_psprintf(pool,
                      "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                      "<D:merge xmlns:D=\"DAV:\">"
                      "<D:source><D:href>%s</D:href></D:source>"
                      "<D:no-auto-merge/><D:no-checkout/>"
                      "<D:prop><D:checked-in/>"
                      "<D:" SVN_DAV__VERSION_NAME "/><D:resourcetype/>"
                      "<D:" SVN_DAV__CREATIONDATE "/><D:creator-displayname/>"
                      "</D:prop>"
                      "%s"
                      "</D:merge>",
                      activity_url, lockbuf->data);

  SVN_ERR(svn_ra_neon__parsed_request(ras, "MERGE", repos_url,
                                      body, 0, NULL,
                                      start_element,
                                      svn_ra_neon__xml_collect_cdata,
                                      end_element, &mc, extra_headers,
                                      NULL, FALSE, pool));

  /* return some commit properties to the caller. */
  if (new_rev)
    *new_rev = mc.rev;
  if (committed_date)
    *committed_date = mc.committed_date->len
                      ? apr_pstrdup(pool, mc.committed_date->data) : NULL;
  if (committed_author)
    *committed_author = mc.last_author->len
                        ? apr_pstrdup(pool, mc.last_author->data) : NULL;
  if (post_commit_err)
    *post_commit_err = mc.post_commit_err->len
                        ? apr_pstrdup(pool, mc.post_commit_err->data) : NULL;

  svn_pool_destroy(mc.scratchpool);

  return NULL;
}

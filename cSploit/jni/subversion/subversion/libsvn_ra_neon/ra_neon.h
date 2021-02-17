/*
 * ra_neon.h : Private declarations for the Neon-based DAV RA module.
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



#ifndef SVN_LIBSVN_RA_NEON_H
#define SVN_LIBSVN_RA_NEON_H

#include <apr_pools.h>
#include <apr_tables.h>

#include <ne_request.h>
#include <ne_uri.h>
#include <ne_207.h>            /* for NE_ELM_207_UNUSED */
#include <ne_props.h>          /* for ne_propname */

#include "svn_types.h"
#include "svn_string.h"
#include "svn_delta.h"
#include "svn_ra.h"
#include "svn_dav.h"

#include "private/svn_dav_protocol.h"
#include "svn_private_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* Rename these types and constants to abstract from Neon */

#define SVN_RA_NEON__XML_DECLINE NE_XML_DECLINE
#define SVN_RA_NEON__XML_INVALID NE_XML_ABORT

#define SVN_RA_NEON__XML_CDATA   (1<<1)
#define SVN_RA_NEON__XML_COLLECT ((1<<2) | SVN_RA_NEON__XML_CDATA)

/* ### Related to anonymous enum below? */
typedef int svn_ra_neon__xml_elmid;

/** XML element */
typedef struct svn_ra_neon__xml_elm_t {
  /** XML namespace. */
  const char *nspace;

  /** XML tag name. */
  const char *name;

  /** XML tag id to be passed to a handler. */
  svn_ra_neon__xml_elmid id;

  /** Processing flags for this namespace:tag.
   *
   * 0 (zero)                - regular element, may have children,
   * SVN_RA_NEON__XML_CDATA   - child-less element,
   * SVN_RA_NEON__XML_COLLECT - complete contents of such element must be
   *                           collected as CDATA, includes *_CDATA flag. */
  unsigned int flags;

} svn_ra_neon__xml_elm_t;



typedef struct svn_ra_neon__session_t {
  apr_pool_t *pool;
  svn_stringbuf_t *url;                 /* original, unparsed session url */
  ne_uri root;                          /* parsed version of above */
  const char *repos_root;               /* URL for repository root */

  ne_session *ne_sess;                  /* HTTP session to server */
  ne_session *ne_sess2;
  svn_boolean_t main_session_busy;      /* TRUE when requests should be created
                                           and issued on sess2; currently
                                           only used by fetch.c */

  const svn_ra_callbacks2_t *callbacks; /* callbacks to get auth data */
  void *callback_baton;

  svn_auth_iterstate_t *auth_iterstate; /* state of authentication retries */
  svn_boolean_t auth_used;              /* Save authorization state after
                                           successful usage */

  svn_auth_iterstate_t *p11pin_iterstate; /* state of PKCS#11 pin retries */

  svn_boolean_t compression;            /* should we use http compression? */

  /* Each of these function as caches, and are NULL when uninitialized
     or cleared: */
  const char *vcc;                      /* version-controlled-configuration */
  const char *uuid;                     /* repository UUID */
  const char *act_coll;                 /* activity collection set */

  svn_ra_progress_notify_func_t progress_func;
  void *progress_baton;

  apr_off_t total_progress;             /* Total number of bytes sent in this
                                           session with a -1 total marker */

  /* Maps SVN_RA_CAPABILITY_foo keys to "yes" or "no" values.
     If a capability is not yet discovered, it is absent from the table.
     The table itself is allocated in the svn_ra_neon__session_t's pool;
     keys and values must have at least that lifetime.  Most likely
     the keys and values are constants anyway (and sufficiently
     well-informed internal code may just compare against those
     constants' addresses, therefore). */
  apr_hash_t *capabilities;

  /* Tri-state variable holding information about server support for
     deadprop-count property.*/
  svn_tristate_t supports_deadprop_count;

  /*** HTTP v2 protocol stuff. ***
   *
   * We assume that if mod_dav_svn sends one of the special v2 OPTIONs
   * response headers, it has sent all of them.  Specifically, we'll
   * be looking at the presence of the "me resource" as a flag that
   * the server supports v2 of our HTTP protocol.
   */

  /* The "me resource".  Typically used as a target for REPORTs that
     are path-agnostic.  If we have this, we can speak HTTP v2 to the
     server.  */
  const char *me_resource;

  /* Opaque URL "stubs".  If the OPTIONS response returns these, then
     we know we're using HTTP protocol v2. */
  const char *rev_stub;         /* for accessing revisions (i.e. revprops) */
  const char *rev_root_stub;    /* for accessing REV/PATH pairs */
  const char *txn_stub;         /* for accessing transactions (i.e. txnprops) */
  const char *txn_root_stub;    /* for accessing TXN/PATH pairs */
  const char *vtxn_stub;        /* for accessing transactions (i.e. txnprops) */
  const char *vtxn_root_stub;   /* for accessing TXN/PATH pairs */

  /*** End HTTP v2 stuff ***/

} svn_ra_neon__session_t;

#define SVN_RA_NEON__HAVE_HTTPV2_SUPPORT(ras) ((ras)->me_resource != NULL)


typedef struct svn_ra_neon__request_t {
  ne_request *ne_req;                   /* neon request structure */
  ne_session *ne_sess;                  /* neon session structure */
  svn_ra_neon__session_t *sess;          /* DAV session structure */
  const char *method;
  const char *url;
  int rv;                               /* Return value from
                                           ne_request_dispatch() or -1 if
                                           not dispatched yet. */
  int code;                             /* HTTP return code, or 0 if none */
  const char *code_desc;                /* Textual description of CODE */
  svn_error_t *err;                     /* error encountered while executing
                                           the request */
  svn_boolean_t marshalled_error;       /* TRUE if the error was server-side */
  apr_pool_t *pool;                     /* where this struct is allocated */
  apr_pool_t *iterpool;                 /* iteration pool
                                           for use within callbacks */
} svn_ra_neon__request_t;


/* Statement macro to set the request error,
 * making sure we don't leak any in case we encounter more than one error.
 *
 * Sets the 'err' field of REQ to the value obtained by evaluating NEW_ERR.
 */
#define SVN_RA_NEON__REQ_ERR(req, new_err)       \
   do {                                          \
     svn_error_t *svn_err__tmp = (new_err);      \
     if ((req)->err && !(req)->marshalled_error) \
       svn_error_clear(svn_err__tmp);            \
     else if (svn_err__tmp)                      \
       {                                         \
         svn_error_clear((req)->err);            \
         (req)->err = svn_err__tmp;              \
         (req)->marshalled_error = FALSE;        \
       }                                         \
   } while (0)


/* Allocate an internal request structure allocated in a newly created
 * subpool of POOL.  Create an associated neon request with the parameters
 * given.
 *
 * When a request is being dispatched on the primary Neon session,
 * the request is allocated to the secondary neon session of SESS.
 *
 * Register a pool cleanup for any allocated Neon resources.
 */
svn_error_t *
svn_ra_neon__request_create(svn_ra_neon__request_t **request,
                            svn_ra_neon__session_t *sess,
                            const char *method, const char *url,
                            apr_pool_t *pool);


/* Our version of ne_block_reader, which returns an
 * svn_error_t * instead of an int. */
typedef svn_error_t *(*svn_ra_neon__block_reader)(void *baton,
                                                  const char *data,
                                                  size_t len);

/* Add a response body reader function to REQ.
 *
 * Use the associated session parameters to determine the use of
 * compression.
 *
 * Register a pool cleanup on the pool of REQ to clean up any allocated
 * Neon resources.
 */
void
svn_ra_neon__add_response_body_reader(svn_ra_neon__request_t *req,
                                      ne_accept_response accpt,
                                      svn_ra_neon__block_reader reader,
                                      void *userdata);


/* Destroy request REQ and any associated resources */
#define svn_ra_neon__request_destroy(req) svn_pool_destroy((req)->pool)

#ifdef SVN_DEBUG
#define DEBUG_CR "\n"
#else
#define DEBUG_CR ""
#endif


/** vtable function prototypes */

svn_error_t *svn_ra_neon__get_latest_revnum(svn_ra_session_t *session,
                                            svn_revnum_t *latest_revnum,
                                            apr_pool_t *pool);

svn_error_t *svn_ra_neon__get_dated_revision(svn_ra_session_t *session,
                                             svn_revnum_t *revision,
                                             apr_time_t timestamp,
                                             apr_pool_t *pool);

svn_error_t *svn_ra_neon__change_rev_prop(svn_ra_session_t *session,
                                          svn_revnum_t rev,
                                          const char *name,
                                          const svn_string_t *const *old_value_p,
                                          const svn_string_t *value,
                                          apr_pool_t *pool);

svn_error_t *svn_ra_neon__rev_proplist(svn_ra_session_t *session,
                                       svn_revnum_t rev,
                                       apr_hash_t **props,
                                       apr_pool_t *pool);

svn_error_t *svn_ra_neon__rev_prop(svn_ra_session_t *session,
                                   svn_revnum_t rev,
                                   const char *name,
                                   svn_string_t **value,
                                   apr_pool_t *pool);

svn_error_t * svn_ra_neon__get_commit_editor(svn_ra_session_t *session,
                                             const svn_delta_editor_t **editor,
                                             void **edit_baton,
                                             apr_hash_t *revprop_table,
                                             svn_commit_callback2_t callback,
                                             void *callback_baton,
                                             apr_hash_t *lock_tokens,
                                             svn_boolean_t keep_locks,
                                             apr_pool_t *pool);

svn_error_t * svn_ra_neon__get_file(svn_ra_session_t *session,
                                    const char *path,
                                    svn_revnum_t revision,
                                    svn_stream_t *stream,
                                    svn_revnum_t *fetched_rev,
                                    apr_hash_t **props,
                                    apr_pool_t *pool);

svn_error_t *svn_ra_neon__get_dir(svn_ra_session_t *session,
                                  apr_hash_t **dirents,
                                  svn_revnum_t *fetched_rev,
                                  apr_hash_t **props,
                                  const char *path,
                                  svn_revnum_t revision,
                                  apr_uint32_t dirent_fields,
                                  apr_pool_t *pool);

svn_error_t * svn_ra_neon__get_mergeinfo(
  svn_ra_session_t *session,
  apr_hash_t **mergeinfo,
  const apr_array_header_t *paths,
  svn_revnum_t revision,
  svn_mergeinfo_inheritance_t inherit,
  svn_boolean_t include_descendants,
  apr_pool_t *pool);

svn_error_t * svn_ra_neon__do_update(svn_ra_session_t *session,
                                     const svn_ra_reporter3_t **reporter,
                                     void **report_baton,
                                     svn_revnum_t revision_to_update_to,
                                     const char *update_target,
                                     svn_depth_t depth,
                                     svn_boolean_t send_copyfrom_args,
                                     const svn_delta_editor_t *wc_update,
                                     void *wc_update_baton,
                                     apr_pool_t *pool);

svn_error_t * svn_ra_neon__do_status(svn_ra_session_t *session,
                                     const svn_ra_reporter3_t **reporter,
                                     void **report_baton,
                                     const char *status_target,
                                     svn_revnum_t revision,
                                     svn_depth_t depth,
                                     const svn_delta_editor_t *wc_status,
                                     void *wc_status_baton,
                                     apr_pool_t *pool);

svn_error_t * svn_ra_neon__do_switch(svn_ra_session_t *session,
                                     const svn_ra_reporter3_t **reporter,
                                     void **report_baton,
                                     svn_revnum_t revision_to_update_to,
                                     const char *update_target,
                                     svn_depth_t depth,
                                     const char *switch_url,
                                     const svn_delta_editor_t *wc_update,
                                     void *wc_update_baton,
                                     apr_pool_t *pool);

svn_error_t * svn_ra_neon__do_diff(svn_ra_session_t *session,
                                   const svn_ra_reporter3_t **reporter,
                                   void **report_baton,
                                   svn_revnum_t revision,
                                   const char *diff_target,
                                   svn_depth_t depth,
                                   svn_boolean_t ignore_ancestry,
                                   svn_boolean_t text_deltas,
                                   const char *versus_url,
                                   const svn_delta_editor_t *wc_diff,
                                   void *wc_diff_baton,
                                   apr_pool_t *pool);

svn_error_t * svn_ra_neon__get_log(svn_ra_session_t *session,
                                   const apr_array_header_t *paths,
                                   svn_revnum_t start,
                                   svn_revnum_t end,
                                   int limit,
                                   svn_boolean_t discover_changed_paths,
                                   svn_boolean_t strict_node_history,
                                   svn_boolean_t include_merged_revisions,
                                   const apr_array_header_t *revprops,
                                   svn_log_entry_receiver_t receiver,
                                   void *receiver_baton,
                                   apr_pool_t *pool);

svn_error_t *svn_ra_neon__do_check_path(svn_ra_session_t *session,
                                        const char *path,
                                        svn_revnum_t revision,
                                        svn_node_kind_t *kind,
                                        apr_pool_t *pool);

svn_error_t *svn_ra_neon__do_stat(svn_ra_session_t *session,
                                  const char *path,
                                  svn_revnum_t revision,
                                  svn_dirent_t **dirent,
                                  apr_pool_t *pool);

svn_error_t *svn_ra_neon__get_file_revs(svn_ra_session_t *session,
                                        const char *path,
                                        svn_revnum_t start,
                                        svn_revnum_t end,
                                        svn_boolean_t include_merged_revisions,
                                        svn_file_rev_handler_t handler,
                                        void *handler_baton,
                                        apr_pool_t *pool);


/* Local duplicate of svn_ra_get_path_relative_to_root(). */
svn_error_t *svn_ra_neon__get_path_relative_to_root(svn_ra_session_t *session,
                                                    const char **rel_path,
                                                    const char *url,
                                                    apr_pool_t *pool);


/*
** SVN_RA_NEON__LP_*: local properties for RA/DAV
**
** ra_neon and ra_serf store properties on the client containing information needed
** to operate against the SVN server. Some of this informations is strictly
** necessary to store, and some is simply stored as a cached value.
*/

#define SVN_RA_NEON__LP_NAMESPACE SVN_PROP_WC_PREFIX "ra_dav:"

/* store the URL where Activities can be created */
/* ### should fix the name to be "activity-coll" at some point */
#define SVN_RA_NEON__LP_ACTIVITY_COLL SVN_RA_NEON__LP_NAMESPACE "activity-url"

/* store the URL of the version resource (from the DAV:checked-in property) */
#define SVN_RA_NEON__LP_VSN_URL          SVN_RA_NEON__LP_NAMESPACE "version-url"


/*
** SVN_RA_NEON__PROP_*: properties that we fetch from the server
**
** These are simply symbolic names for some standard properties that we fetch.
*/
#define SVN_RA_NEON__PROP_BASELINE_COLLECTION    "DAV:baseline-collection"
#define SVN_RA_NEON__PROP_CHECKED_IN     "DAV:checked-in"
#define SVN_RA_NEON__PROP_VCC            "DAV:version-controlled-configuration"
#define SVN_RA_NEON__PROP_VERSION_NAME   "DAV:" SVN_DAV__VERSION_NAME
#define SVN_RA_NEON__PROP_CREATIONDATE   "DAV:creationdate"
#define SVN_RA_NEON__PROP_CREATOR_DISPLAYNAME "DAV:creator-displayname"
#define SVN_RA_NEON__PROP_GETCONTENTLENGTH "DAV:getcontentlength"

#define SVN_RA_NEON__PROP_BASELINE_RELPATH \
    SVN_DAV_PROP_NS_DAV "baseline-relative-path"

#define SVN_RA_NEON__PROP_MD5_CHECKSUM SVN_DAV_PROP_NS_DAV "md5-checksum"

#define SVN_RA_NEON__PROP_REPOSITORY_UUID SVN_DAV_PROP_NS_DAV "repository-uuid"

#define SVN_RA_NEON__PROP_DEADPROP_COUNT SVN_DAV_PROP_NS_DAV "deadprop-count"

typedef struct svn_ra_neon__resource_t {
  /* what is the URL for this resource */
  const char *url;

  /* is this resource a collection? (from the DAV:resourcetype element) */
  int is_collection;

  /* PROPSET: NAME -> VALUE (const char * -> const svn_string_t *) */
  apr_hash_t *propset;

  /* --- only used during response processing --- */
  /* when we see a DAV:href element, what element is the parent? */
  int href_parent;

  apr_pool_t *pool;

} svn_ra_neon__resource_t;

/* ### WARNING: which_props can only identify properties which props.c
   ### knows about. see the elem_definitions[] array. */

/* fetch a bunch of properties from the server. */
svn_error_t * svn_ra_neon__get_props(apr_hash_t **results,
                                     svn_ra_neon__session_t *sess,
                                     const char *url,
                                     int depth,
                                     const char *label,
                                     const ne_propname *which_props,
                                     apr_pool_t *pool);

/* fetch a single resource's props from the server. */
svn_error_t * svn_ra_neon__get_props_resource(svn_ra_neon__resource_t **rsrc,
                                              svn_ra_neon__session_t *sess,
                                              const char *url,
                                              const char *label,
                                              const ne_propname *which_props,
                                              apr_pool_t *pool);

/* fetch a single resource's starting props from the server.

   Cache the version-controlled-configuration in SESS->vcc, and the
   repository uuid in SESS->uuid. */
svn_error_t * svn_ra_neon__get_starting_props(svn_ra_neon__resource_t **rsrc,
                                              svn_ra_neon__session_t *sess,
                                              const char *url,
                                              apr_pool_t *pool);

/* Shared helper func: given a public URL which may not exist in HEAD,
   use SESS to search up parent directories until we can retrieve a
   *RSRC (allocated in POOL) containing a standard set of "starting"
   props: {VCC, resourcetype, baseline-relative-path}.

   Also return *MISSING_PATH (allocated in POOL), which is the
   trailing portion of the URL that did not exist.  If an error
   occurs, *MISSING_PATH isn't changed.

   Cache the version-controlled-configuration in SESS->vcc, and the
   repository uuid in SESS->uuid. */
svn_error_t *
svn_ra_neon__search_for_starting_props(svn_ra_neon__resource_t **rsrc,
                                       const char **missing_path,
                                       svn_ra_neon__session_t *sess,
                                       const char *url,
                                       apr_pool_t *pool);

/* fetch a single property from a single resource */
svn_error_t * svn_ra_neon__get_one_prop(const svn_string_t **propval,
                                        svn_ra_neon__session_t *sess,
                                        const char *url,
                                        const char *label,
                                        const ne_propname *propname,
                                        apr_pool_t *pool);

/* Get various Baseline-related information for a given "public" URL.

   REVISION may be SVN_INVALID_REVNUM to indicate that the operation
   should work against the latest (HEAD) revision, or whether it should
   return information about that specific revision.

   If BC_URL_P is not NULL, then it will be filled in with the URL for
   the Baseline Collection for the specified revision, or the HEAD.

   If BC_RELATIVE_P is not NULL, then it will be filled in with a
   relative pathname for the baselined resource corresponding to the
   revision of the resource specified by URL.

   If LATEST_REV is not NULL, then it will be filled in with the revision
   that this information corresponds to. Generally, this will be the same
   as the REVISION parameter, unless we are working against the HEAD. In
   that case, the HEAD revision number is returned.

   Allocation for *BC_URL_P, *BC_RELATIVE_P, and temporary data,
   will occur in POOL.

   Note: a Baseline Collection is a complete tree for a specified Baseline.
   DeltaV baselines correspond one-to-one to Subversion revisions. Thus,
   the entire state of a revision can be found in a Baseline Collection.
*/
svn_error_t *svn_ra_neon__get_baseline_info(const char **bc_url_p,
                                            const char **bc_relative_p,
                                            svn_revnum_t *latest_rev,
                                            svn_ra_neon__session_t *sess,
                                            const char *url,
                                            svn_revnum_t revision,
                                            apr_pool_t *pool);

/* Fetch a baseline resource populated with specific properties.

   Given a session SESS and a URL, set *BLN_RSRC to a baseline of
   REVISION, populated with whatever properties are specified by
   WHICH_PROPS.  To fetch all properties, pass NULL for WHICH_PROPS.

   If BC_RELATIVE is not NULL, then it will be filled in with a
   relative pathname for the baselined resource corresponding to the
   revision of the resource specified by URL.
*/
svn_error_t *svn_ra_neon__get_baseline_props(svn_string_t *bc_relative,
                                             svn_ra_neon__resource_t **bln_rsrc,
                                             svn_ra_neon__session_t *sess,
                                             const char *url,
                                             svn_revnum_t revision,
                                             const ne_propname *which_props,
                                             apr_pool_t *pool);

/* Fetch the repository's unique Version-Controlled-Configuration url.

   Given a session SESS and a URL, set *VCC to the url of the
   repository's version-controlled-configuration resource.
 */
svn_error_t *svn_ra_neon__get_vcc(const char **vcc,
                                  svn_ra_neon__session_t *sess,
                                  const char *url,
                                  apr_pool_t *pool);

/* Issue a PROPPATCH request on URL, transmitting PROP_CHANGES (a hash
   of const svn_string_t * values keyed on Subversion user-visible
   property names) and PROP_DELETES (an array of property names to
   delete). PROP_OLD_VALUES is a hash of Subversion user-visible property
   names mapped to svn_dav__two_props_t * values. Send any extra
   request headers in EXTRA_HEADERS. Use POOL for all allocations.
 */
svn_error_t *svn_ra_neon__do_proppatch(svn_ra_neon__session_t *ras,
                                       const char *url,
                                       apr_hash_t *prop_changes,
                                       const apr_array_header_t *prop_deletes,
                                       apr_hash_t *prop_old_values,
                                       apr_hash_t *extra_headers,
                                       apr_pool_t *pool);

extern const ne_propname svn_ra_neon__vcc_prop;
extern const ne_propname svn_ra_neon__checked_in_prop;


/* send an OPTIONS request to fetch the activity-collection-set */
svn_error_t *
svn_ra_neon__get_activity_collection(const svn_string_t **activity_coll,
                                     svn_ra_neon__session_t *ras,
                                     apr_pool_t *pool);


/* Call ne_set_request_body_pdovider on REQ with a provider function
 * that pulls data from BODY_FILE.
 */
svn_error_t *svn_ra_neon__set_neon_body_provider(svn_ra_neon__request_t *req,
                                                 apr_file_t *body_file);


#define SVN_RA_NEON__DEPTH_ZERO      0
#define SVN_RA_NEON__DEPTH_ONE       1
#define SVN_RA_NEON__DEPTH_INFINITE -1
/* Add a 'Depth' header to a hash of headers.
 *
 * DEPTH is one of the above defined SVN_RA_NEON__DEPTH_* values.
 */
void
svn_ra_neon__add_depth_header(apr_hash_t *extra_headers, int depth);

/** Find a given element in the table of elements.
 *
 * The table of XML elements @a table is searched until element identified by
 * namespace @a nspace and name @a name is found. If no elements are found,
 * tries to find and return element identified by @c ELEM_unknown. If that is
 * not found, returns NULL pointer. */
const svn_ra_neon__xml_elm_t *
svn_ra_neon__lookup_xml_elem(const svn_ra_neon__xml_elm_t *table,
                             const char *nspace,
                             const char *name);



/* Collect CDATA into a stringbuf.
 *
 * BATON points to a struct of which the first element is
 * assumed to be an svn_stringbuf_t *.
 */
svn_error_t *
svn_ra_neon__xml_collect_cdata(void *baton, int state,
                               const char *cdata, size_t len);


/* Our equivalent of ne_xml_startelm_cb, the difference being that it
 * returns errors in a svn_error_t, and returns the element type via
 * ELEM.  To ignore the element *ELEM should be set to
 * SVN_RA_NEON__XML_DECLINE and SVN_NO_ERROR should be returned.
 * *ELEM can be set to SVN_RA_NEON__XML_INVALID to indicate invalid XML
 * (and abort the parse).
 */
typedef svn_error_t * (*svn_ra_neon__startelm_cb_t)(int *elem,
                                                    void *baton,
                                                    int parent,
                                                    const char *nspace,
                                                    const char *name,
                                                    const char **atts);

/* Our equivalent of ne_xml_cdata_cb, the difference being that it returns
 * errors in a svn_error_t.
 */
typedef svn_error_t * (*svn_ra_neon__cdata_cb_t)(void *baton,
                                                 int state,
                                                 const char *cdata,
                                                 size_t len);

/* Our equivalent of ne_xml_endelm_cb, the difference being that it returns
 * errors in a svn_error_t.
 */
typedef svn_error_t * (*svn_ra_neon__endelm_cb_t)(void *baton,
                                                  int state,
                                                  const char *nspace,
                                                  const char *name);


/* Create a Neon xml parser with callbacks STARTELM_CB, ENDELM_CB and
 * CDATA_CB.  The created parser wraps the Neon callbacks and marshals any
 * errors returned by the callbacks through the Neon layer.  Any errors
 * raised will be returned by svn_ra_neon__request_dispatch() unless
 * an earlier error occurred.
 *
 * Register a pool cleanup on the pool of REQ to clean up any allocated
 * Neon resources.
 *
 * Return the new parser.  Also attach it to REQ if ACCPT is non-null.
 * ACCPT indicates whether the parser wants to read the response body
 * or not.  Pass NULL for ACCPT when you don't want the returned parser
 * to be attached to REQ.
 */
ne_xml_parser *
svn_ra_neon__xml_parser_create(svn_ra_neon__request_t *req,
                               ne_accept_response accpt,
                               svn_ra_neon__startelm_cb_t startelm_cb,
                               svn_ra_neon__cdata_cb_t cdata_cb,
                               svn_ra_neon__endelm_cb_t endelm_cb,
                               void *baton);

/* Send a METHOD request (e.g., "MERGE", "REPORT", "PROPFIND") to URL
 * in session SESS, and parse the response.  If BODY is non-null, it is
 * the body of the request, else use the contents of file BODY_FILE
 * as the body.
 *
 * STARTELM_CB, CDATA_CB and ENDELM_CB are start element, cdata and end
 * element handlers, respectively.  BATON is passed to each as userdata.
 *
 * SET_PARSER is a callback function which, if non-NULL, is called
 * with the XML parser and BATON.  This is useful for providers of
 * validation and element handlers which require access to the parser.
 *
 * EXTRA_HEADERS is a hash of (const char *) key/value pairs to be
 * inserted as extra headers in the request.  Can be NULL.
 *
 * STATUS_CODE is an optional 'out' parameter; if non-NULL, then set
 * *STATUS_CODE to the http status code returned by the server.  This
 * can be set to a useful value even when the function returns an error
 * however it is not always set when an error is returned.  So any caller
 * wishing to check *STATUS_CODE when an error has been returned must
 * initialise *STATUS_CODE before calling the function.
 *
 * If SPOOL_RESPONSE is set, the request response will be cached to
 * disk in a tmpfile (in full), then read back and parsed.
 *
 * Use POOL for any temporary allocation.
 */
svn_error_t *
svn_ra_neon__parsed_request(svn_ra_neon__session_t *sess,
                            const char *method,
                            const char *url,
                            const char *body,
                            apr_file_t *body_file,
                            void set_parser(ne_xml_parser *parser,
                                            void *baton),
                            svn_ra_neon__startelm_cb_t startelm_cb,
                            svn_ra_neon__cdata_cb_t cdata_cb,
                            svn_ra_neon__endelm_cb_t endelm_cb,
                            void *baton,
                            apr_hash_t *extra_headers,
                            int *status_code,
                            svn_boolean_t spool_response,
                            apr_pool_t *pool);


/* If XML_PARSER found an XML parse error, then return a Subversion error
 * saying that the error was found in the response to the DAV request METHOD
 * for the URL URL. Otherwise, return SVN_NO_ERROR. */
svn_error_t *
svn_ra_neon__check_parse_error(const char *method,
                               ne_xml_parser *xml_parser,
                               const char *url);

/* ### Related to svn_ra_neon__xml_elmid? */
/* ### add SVN_RA_NEON_ to these to prefix conflicts with (sys) headers? */
enum {
  /* Redefine Neon elements */
  /* With the new API, we need to be able to use element id also as a return
   * value from the new `startelm' callback, hence all element ids must be
   * positive. Root element id is the only id that is not positive, it's zero.
   * `Root state' is never returned by a callback, it's only passed into it.
   * Therefore, negative element ids are forbidden from now on. */
  ELEM_unknown = 1, /* was (-1), see above why it's (1) now */
  ELEM_root = NE_XML_STATEROOT, /* (0) */
  ELEM_UNUSED = 100,
  ELEM_207_first = ELEM_UNUSED,
  ELEM_multistatus = ELEM_207_first,
  ELEM_response = ELEM_207_first + 1,
  ELEM_responsedescription = ELEM_207_first + 2,
  ELEM_href = ELEM_207_first + 3,
  ELEM_propstat = ELEM_207_first + 4,
  ELEM_prop = ELEM_207_first + 5, /* `prop' tag in the DAV namespace */
  ELEM_status = ELEM_207_first + 6,
  ELEM_207_UNUSED = ELEM_UNUSED + 100,
  ELEM_PROPS_UNUSED = ELEM_207_UNUSED + 100,

  /* DAV elements */
  ELEM_activity_coll_set = ELEM_207_UNUSED,
  ELEM_baseline,
  ELEM_baseline_coll,
  ELEM_checked_in,
  ELEM_collection,
  ELEM_comment,
  ELEM_revprop,
  ELEM_creationdate,
  ELEM_creator_displayname,
  ELEM_ignored_set,
  ELEM_merge_response,
  ELEM_merged_set,
  ELEM_options_response,
  ELEM_set_prop,
  ELEM_remove_prop,
  ELEM_resourcetype,
  ELEM_get_content_length,
  ELEM_updated_set,
  ELEM_vcc,
  ELEM_version_name,
  ELEM_post_commit_err,
  ELEM_error,

  /* SVN elements */
  ELEM_absent_directory,
  ELEM_absent_file,
  ELEM_add_directory,
  ELEM_add_file,
  ELEM_baseline_relpath,
  ELEM_md5_checksum,
  ELEM_deleted_path,  /* used in log reports */
  ELEM_replaced_path,  /* used in log reports */
  ELEM_added_path,    /* used in log reports */
  ELEM_modified_path,  /* used in log reports */
  ELEM_delete_entry,
  ELEM_fetch_file,
  ELEM_fetch_props,
  ELEM_txdelta,
  ELEM_log_date,
  ELEM_log_item,
  ELEM_log_report,
  ELEM_open_directory,
  ELEM_open_file,
  ELEM_target_revision,
  ELEM_update_report,
  ELEM_resource_walk,
  ELEM_resource,
  ELEM_SVN_prop, /* `prop' tag in the Subversion namespace */
  ELEM_dated_rev_report,
  ELEM_name_version_name,
  ELEM_name_creationdate,
  ELEM_name_creator_displayname,
  ELEM_svn_error,
  ELEM_human_readable,
  ELEM_repository_uuid,
  ELEM_get_locations_report,
  ELEM_location,
  ELEM_get_location_segments_report,
  ELEM_location_segment,
  ELEM_file_revs_report,
  ELEM_file_rev,
  ELEM_rev_prop,
  ELEM_get_locks_report,
  ELEM_lock,
  ELEM_lock_path,
  ELEM_lock_token,
  ELEM_lock_owner,
  ELEM_lock_comment,
  ELEM_lock_creationdate,
  ELEM_lock_expirationdate,
  ELEM_lock_discovery,
  ELEM_lock_activelock,
  ELEM_lock_type,
  ELEM_lock_scope,
  ELEM_lock_depth,
  ELEM_lock_timeout,
  ELEM_editor_report,
  ELEM_open_root,
  ELEM_apply_textdelta,
  ELEM_change_file_prop,
  ELEM_change_dir_prop,
  ELEM_close_file,
  ELEM_close_directory,
  ELEM_deadprop_count,
  ELEM_mergeinfo_report,
  ELEM_mergeinfo_item,
  ELEM_mergeinfo_path,
  ELEM_mergeinfo_info,
  ELEM_has_children,
  ELEM_merged_revision,
  ELEM_deleted_rev_report,
  ELEM_subtractive_merge
};

/* ### docco */
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
                                          apr_pool_t *pool);


/* Make a buffer for repeated use with svn_stringbuf_set().
   ### it would be nice to start this buffer with N bytes, but there isn't
   ### really a way to do that in the string interface (yet), short of
   ### initializing it with a fake string (and copying it) */
#define MAKE_BUFFER(p) svn_stringbuf_ncreate("", 0, (p))

svn_error_t *
svn_ra_neon__copy_href(svn_stringbuf_t *dst, const char *src,
                       apr_pool_t *pool);



/* If RAS contains authentication info, attempt to store it via client
   callbacks and using POOL for temporary allocations.  */
svn_error_t *
svn_ra_neon__maybe_store_auth_info(svn_ra_neon__session_t *ras,
                                   apr_pool_t *pool);


/* Like svn_ra_neon__maybe_store_auth_info(), but conditional on ERR.

   Attempt to store auth info only if ERR is NULL or if ERR->apr_err
   is not SVN_ERR_RA_NOT_AUTHORIZED.  If ERR is not null, return it no
   matter what, otherwise return the result of the attempt (if any) to
   store auth info, else return SVN_NO_ERROR. */
svn_error_t *
svn_ra_neon__maybe_store_auth_info_after_result(svn_error_t *err,
                                                svn_ra_neon__session_t *ras,
                                                apr_pool_t *pool);


/* Create an error of type SVN_ERR_RA_DAV_MALFORMED_DATA for cases where
   we receive an element we didn't expect to see. */
#define UNEXPECTED_ELEMENT(ns, elem)                               \
        (ns ? svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA,     \
                                NULL,                              \
                                _("Got unexpected element %s:%s"), \
                                ns,                                \
                                elem)                              \
            : svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA,     \
                                NULL,                              \
                                _("Got unexpected element %s"),    \
                                elem))

/* Create an error of type SVN_ERR_RA_DAV_MALFORMED_DATA for cases where
   we don't receive a necessary attribute. */
#define MISSING_ATTR(ns, elem, attr) \
        (ns ? svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, \
                                NULL, \
                                _("Missing attribute '%s' on element %s:%s"), \
                                attr, \
                                ns, \
                                elem) \
           : svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, \
                               NULL, \
                               _("Missing attribute '%s' on element %s"), \
                               attr, \
                               elem))

/* Given a REQUEST, run it; if CODE_P is
   non-null, return the http status code in *CODE_P.  Return any
   resulting error (from Neon, a <D:error> body response, or any
   non-2XX status code) as an svn_error_t, otherwise return SVN_NO_ERROR.

   EXTRA_HEADERS is a hash with (key -> value) of
   (const char * -> const char *) where the key is the HTTP header name.

   BODY is a null terminated string containing an in-memory request
   body.  Use svn_ra_neon__set_neon_body_provider() if you want the
   request body to be read from a file.  For requests which have no
   body at all, consider passing the empty string ("") instead of
   NULL, as this will cause Neon to generate a "Content-Length: 0"
   header (which is important to some proxies).

   OKAY_1 and OKAY_2 are the "acceptable" result codes.  Anything
   other than one of these will generate an error.  OKAY_1 should
   always be specified (e.g. as 200); use 0 for OKAY_2 if additional
   result codes aren't allowed.  */
svn_error_t *
svn_ra_neon__request_dispatch(int *code_p,
                              svn_ra_neon__request_t *request,
                              apr_hash_t *extra_headers,
                              const char *body,
                              int okay_1,
                              int okay_2,
                              apr_pool_t *pool);

/* A layer over SVN_RA_NEON__REQUEST_DISPATCH() adding a
   207 response parser to extract relevant (error) information.

   Don't use this function if you're expecting 207 as a valid response.

   BODY may be NULL if the request doesn't have a body. */
svn_error_t *
svn_ra_neon__simple_request(int *code,
                            svn_ra_neon__session_t *ras,
                            const char *method,
                            const char *url,
                            apr_hash_t *extra_headers,
                            const char *body,
                            int okay_1,
                            int okay_2,
                            apr_pool_t *pool);

/* Convenience statement macro for setting headers in a hash */
#define svn_ra_neon__set_header(hash, hdr, val) \
  apr_hash_set((hash), (hdr), APR_HASH_KEY_STRING, (val))


/* Helper function layered over SVN_RA_NEON__SIMPLE_REQUEST() to issue
   a HTTP COPY request.

   DEPTH is one of the SVN_RA_NEON__DEPTH_* constants. */
svn_error_t *
svn_ra_neon__copy(svn_ra_neon__session_t *ras,
                  svn_boolean_t overwrite,
                  int depth,
                  const char *src,
                  const char *dst,
                  apr_pool_t *pool);

/* Return the Location HTTP header or NULL if none was sent.
 * (Return a canonical URL even if the header ended with a slash.)
 *
 * Do allocations in POOL.
 */
const char *
svn_ra_neon__request_get_location(svn_ra_neon__request_t *request,
                                  apr_pool_t *pool);


/*
 * Implements the get_locations RA layer function. */
svn_error_t *
svn_ra_neon__get_locations(svn_ra_session_t *session,
                           apr_hash_t **locations,
                           const char *path,
                           svn_revnum_t peg_revision,
                           const apr_array_header_t *location_revisions,
                           apr_pool_t *pool);


/*
 * Implements the get_location_segments RA layer function. */
svn_error_t *
svn_ra_neon__get_location_segments(svn_ra_session_t *session,
                                   const char *path,
                                   svn_revnum_t peg_revision,
                                   svn_revnum_t start_rev,
                                   svn_revnum_t end_rev,
                                   svn_location_segment_receiver_t receiver,
                                   void *receiver_baton,
                                   apr_pool_t *pool);

/*
 * Implements the get_locks RA layer function. */
svn_error_t *
svn_ra_neon__get_locks(svn_ra_session_t *session,
                       apr_hash_t **locks,
                       const char *path,
                       svn_depth_t depth,
                       apr_pool_t *pool);

/*
 * Implements the lock RA layer function. */
svn_error_t *
svn_ra_neon__lock(svn_ra_session_t *session,
                  apr_hash_t *path_revs,
                  const char *comment,
                  svn_boolean_t force,
                  svn_ra_lock_callback_t lock_func,
                  void *lock_baton,
                  apr_pool_t *pool);

/*
 * Implements the unlock RA layer function. */
svn_error_t *
svn_ra_neon__unlock(svn_ra_session_t *session,
                    apr_hash_t *path_tokens,
                    svn_boolean_t force,
                    svn_ra_lock_callback_t lock_func,
                    void *lock_baton,
                    apr_pool_t *pool);

/*
 * Internal implementation of get_lock RA layer function. */
svn_error_t *
svn_ra_neon__get_lock_internal(svn_ra_neon__session_t *session,
                               svn_lock_t **lock,
                               const char *path,
                               apr_pool_t *pool);

/*
 * Implements the get_lock RA layer function. */
svn_error_t *
svn_ra_neon__get_lock(svn_ra_session_t *session,
                      svn_lock_t **lock,
                      const char *path,
                      apr_pool_t *pool);

/*
 * Implements the replay RA layer function. */
svn_error_t *
svn_ra_neon__replay(svn_ra_session_t *session,
                    svn_revnum_t revision,
                    svn_revnum_t low_water_mark,
                    svn_boolean_t send_deltas,
                    const svn_delta_editor_t *editor,
                    void *edit_baton,
                    apr_pool_t *pool);

/*
 * Implements the replay_range RA layer function. */
svn_error_t *
svn_ra_neon__replay_range(svn_ra_session_t *session,
                          svn_revnum_t start_revision,
                          svn_revnum_t end_revision,
                          svn_revnum_t low_water_mark,
                          svn_boolean_t send_deltas,
                          svn_ra_replay_revstart_callback_t revstart_func,
                          svn_ra_replay_revfinish_callback_t revfinish_func,
                          void *replay_baton,
                          apr_pool_t *pool);

/*
 * Implements the has_capability RA layer function. */
svn_error_t *
svn_ra_neon__has_capability(svn_ra_session_t *session,
                            svn_boolean_t *has,
                            const char *capability,
                            apr_pool_t *pool);

/* Exchange capabilities with the server, by sending an OPTIONS
   request announcing the client's capabilities, and by filling
   RAS->capabilities with the server's capabilities as read from the
   response headers.  Use POOL only for temporary allocation.

   If the RELOCATION_LOCATION is non-NULL, allow the OPTIONS response
   to report a server-dictated redirect or relocation (HTTP 301 or 302
   error codes), setting *RELOCATION_LOCATION to the value of the
   corrected repository URL.  Otherwise, such responses from the
   server will generate an error.  (In either case, no capabilities are
   exchanged if there is, in fact, such a response from the server.)

   If the server is kind enough to tell us the current youngest
   revision of the target repository, set *YOUNGEST_REV to that value;
   set it to SVN_INVALID_REVNUM otherwise.  YOUNGEST_REV may be NULL if
   the caller is not interested in receiving this information.

   NOTE:  This function also expects the server to announce the
   activity collection.  */
svn_error_t *
svn_ra_neon__exchange_capabilities(svn_ra_neon__session_t *ras,
                                   const char **relocation_location,
                                   svn_revnum_t *youngest_rev,
                                   apr_pool_t *pool);

/*
 * Implements the get_deleted_rev RA layer function. */
svn_error_t *
svn_ra_neon__get_deleted_rev(svn_ra_session_t *session,
                             const char *path,
                             svn_revnum_t peg_revision,
                             svn_revnum_t end_revision,
                             svn_revnum_t *revision_deleted,
                             apr_pool_t *pool);

/* Helper function.  Loop over LOCK_TOKENS and assemble all keys and
   values into a stringbuf allocated in POOL.  The string will be of
   the form

    <S:lock-token-list xmlns:S="svn:">
      <S:lock>
        <S:lock-path>path</S:lock-path>
        <S:lock-token>token</S:lock-token>
      </S:lock>
      [...]
    </S:lock-token-list>

   Callers can then send this in the request bodies, as a way of
   reliably marshalling potentially unbounded lists of locks.  (We do
   this because httpd has limits on how much data can be sent in 'If:'
   headers.)
 */
svn_error_t *
svn_ra_neon__assemble_locktoken_body(svn_stringbuf_t **body,
                                     apr_hash_t *lock_tokens,
                                     apr_pool_t *pool);


/* Wrapper around ne_uri_unparse(). Turns a URI structure back into a string.
 * The returned string is allocated in POOL. */
const char *
svn_ra_neon__uri_unparse(const ne_uri *uri,
                         apr_pool_t *pool);

/* Wrapper around ne_uri_parse() which parses a URL and returns only
   the server path portion thereof. */
svn_error_t *
svn_ra_neon__get_url_path(const char **urlpath,
                          const char *url,
                          apr_pool_t *pool);

/* Sets *SUPPORTS_DEADPROP_COUNT to non-zero if server supports
 * deadprop-count property. Uses FINAL_URL to discover this informationn
 * if it is not already cached. */
svn_error_t *
svn_ra_neon__get_deadprop_count_support(svn_boolean_t *supported,
                                        svn_ra_neon__session_t *ras,
                                        const char *final_url,
                                        apr_pool_t *pool);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* SVN_LIBSVN_RA_NEON_H */

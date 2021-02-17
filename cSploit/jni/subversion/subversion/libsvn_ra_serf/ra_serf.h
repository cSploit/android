/*
 * ra_serf.h : Private declarations for the Serf-based DAV RA module.
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

#ifndef SVN_LIBSVN_RA_SERF_RA_SERF_H
#define SVN_LIBSVN_RA_SERF_RA_SERF_H


#include <serf.h>
#include <expat.h>
#include <apr_uri.h>

#include "svn_types.h"
#include "svn_string.h"
#include "svn_pools.h"
#include "svn_ra.h"
#include "svn_delta.h"
#include "svn_version.h"
#include "svn_dav.h"
#include "svn_dirent_uri.h"

#include "private/svn_dav_protocol.h"

#include "blncache.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Enforce the minimum version of serf. */
#if !SERF_VERSION_AT_LEAST(0, 7, 1)
#error Please update your version of serf to at least 0.7.1.
#endif

/** Use this to silence compiler warnings about unused parameters. */
#define UNUSED_CTX(x) ((void)(x))

/** Our User-Agent string. */
#define USER_AGENT "SVN/" SVN_VER_NUMBER " serf/" \
                   APR_STRINGIFY(SERF_MAJOR_VERSION) "." \
                   APR_STRINGIFY(SERF_MINOR_VERSION) "." \
                   APR_STRINGIFY(SERF_PATCH_VERSION)


/* Forward declarations. */
typedef struct svn_ra_serf__session_t svn_ra_serf__session_t;

/* A serf connection and optionally associated SSL context.  */
typedef struct svn_ra_serf__connection_t {
  /* Our connection to a server. */
  serf_connection_t *conn;

  /* Bucket allocator for this connection. */
  serf_bucket_alloc_t *bkt_alloc;

  /* Host name */
  const char *hostname;

  /* Are we using ssl */
  svn_boolean_t using_ssl;
  int server_cert_failures; /* Collected cert failures in chain */

  /* Should we ask for compressed responses? */
  svn_boolean_t using_compression;

  /* What was the last HTTP status code we got on this connection? */
  int last_status_code;

  /* Optional SSL context for this connection. */
  serf_ssl_context_t *ssl_context;
  svn_auth_iterstate_t *ssl_client_auth_state;
  svn_auth_iterstate_t *ssl_client_pw_auth_state;

  svn_ra_serf__session_t *session;

  /* user agent string */
  const char *useragent;

} svn_ra_serf__connection_t;

/*
 * The master serf RA session.
 *
 * This is stored in the ra session ->priv field.
 */
struct svn_ra_serf__session_t {
  /* Pool for allocations during this session */
  apr_pool_t *pool;

  /* The current context */
  serf_context_t *context;

  /* Are we using ssl */
  svn_boolean_t using_ssl;

  /* Should we ask for compressed responses? */
  svn_boolean_t using_compression;

  /* The current connection */
  svn_ra_serf__connection_t **conns;
  int num_conns;
  int cur_conn;

  /* The URL that was passed into _open() */
  apr_uri_t session_url;
  const char *session_url_str;

  /* The actual discovered root; may be NULL until we know it. */
  apr_uri_t repos_root;
  const char *repos_root_str;

  /* Our Version-Controlled-Configuration; may be NULL until we know it. */
  const char *vcc_url;

  /* Authentication related properties. */
  svn_auth_iterstate_t *auth_state;
  int auth_attempts;

  /* Callback functions to get info from WC */
  const svn_ra_callbacks2_t *wc_callbacks;
  void *wc_callback_baton;

  /* Callback function to send progress info to the client */
  svn_ra_progress_notify_func_t progress_func;
  void *progress_baton;

  /* Callback function to handle cancellation */
  svn_cancel_func_t cancel_func;
  void *cancel_baton;

  /* Error that we've received but not yet returned upstream. */
  svn_error_t *pending_error;

  /* List of authn types supported by the client.*/
  int authn_types;

  /* Maps SVN_RA_CAPABILITY_foo keys to "yes" or "no" values.
     If a capability is not yet discovered, it is absent from the table.
     The table itself is allocated in the svn_ra_serf__session_t's pool;
     keys and values must have at least that lifetime.  Most likely
     the keys and values are constants anyway (and sufficiently
     well-informed internal code may just compare against those
     constants' addresses, therefore). */
  apr_hash_t *capabilities;

  /* Activity collection URL.  (Cached from the initial OPTIONS
     request when run against HTTPv1 servers.)  */
  const char *activity_collection_url;

  /* Are we using a proxy? */
  int using_proxy;

  const char *proxy_username;
  const char *proxy_password;
  int proxy_auth_attempts;

  /* SSL server certificates */
  svn_boolean_t trust_default_ca;
  const char *ssl_authorities;

  /* Repository UUID */
  const char *uuid;

  /* Connection timeout value */
  long timeout;

  /* HTTPv1 flags */
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

  svn_ra_serf__blncache_t *blncache;
};

#define SVN_RA_SERF__HAVE_HTTPV2_SUPPORT(sess) ((sess)->me_resource != NULL)

/*
 * Structure which represents a DAV element with a NAMESPACE and NAME.
 */
typedef struct svn_ra_serf__dav_props_t {
  /* Element namespace */
  const char *namespace;
  /* Element name */
  const char *name;
} svn_ra_serf__dav_props_t;

/*
 * Structure which represents an XML namespace.
 */
typedef struct ns_t {
  /* The assigned name. */
  const char *namespace;
  /* The full URL for this namespace. */
  const char *url;
  /* The next namespace in our list. */
  struct ns_t *next;
} svn_ra_serf__ns_t;

/*
 * An incredibly simple list.
 */
typedef struct ra_serf_list_t {
  void *data;
  struct ra_serf_list_t *next;
} svn_ra_serf__list_t;

/** DAV property sets **/

static const svn_ra_serf__dav_props_t base_props[] =
{
  { "DAV:", "version-controlled-configuration" },
  { "DAV:", "resourcetype" },
  { SVN_DAV_PROP_NS_DAV, "baseline-relative-path" },
  { SVN_DAV_PROP_NS_DAV, "repository-uuid" },
  { NULL }
};

static const svn_ra_serf__dav_props_t checked_in_props[] =
{
  { "DAV:", "checked-in" },
  { NULL }
};

static const svn_ra_serf__dav_props_t baseline_props[] =
{
  { "DAV:", "baseline-collection" },
  { "DAV:", SVN_DAV__VERSION_NAME },
  { NULL }
};

static const svn_ra_serf__dav_props_t all_props[] =
{
  { "DAV:", "allprop" },
  { NULL }
};

static const svn_ra_serf__dav_props_t vcc_props[] =
{
  { "DAV:", "version-controlled-configuration" },
  { NULL }
};

static const svn_ra_serf__dav_props_t check_path_props[] =
{
  { "DAV:", "resourcetype" },
  { NULL }
};

static const svn_ra_serf__dav_props_t uuid_props[] =
{
  { SVN_DAV_PROP_NS_DAV, "repository-uuid" },
  { NULL }
};

static const svn_ra_serf__dav_props_t repos_root_props[] =
{
  { SVN_DAV_PROP_NS_DAV, "baseline-relative-path" },
  { NULL }
};

static const svn_ra_serf__dav_props_t href_props[] =
{
  { "DAV:", "href" },
  { NULL }
};

/* WC props compatibility with ra_neon. */
#define SVN_RA_SERF__WC_NAMESPACE SVN_PROP_WC_PREFIX "ra_dav:"
#define SVN_RA_SERF__WC_ACTIVITY_URL SVN_RA_SERF__WC_NAMESPACE "activity-url"
#define SVN_RA_SERF__WC_CHECKED_IN_URL SVN_RA_SERF__WC_NAMESPACE "version-url"

/** Serf utility functions **/

apr_status_t
svn_ra_serf__conn_setup(apr_socket_t *sock,
                        serf_bucket_t **read_bkt,
                        serf_bucket_t **write_bkt,
                        void *baton,
                        apr_pool_t *pool);

serf_bucket_t*
svn_ra_serf__accept_response(serf_request_t *request,
                             serf_bucket_t *stream,
                             void *acceptor_baton,
                             apr_pool_t *pool);

void
svn_ra_serf__conn_closed(serf_connection_t *conn,
                         void *closed_baton,
                         apr_status_t why,
                         apr_pool_t *pool);


/* Helper function to provide SSL client certificates.
 *
 * NOTE: This function sets the session's 'pending_error' member when
 *       returning an non-success status.
 */
apr_status_t
svn_ra_serf__handle_client_cert(void *data,
                                const char **cert_path);

/* Helper function to provide SSL client certificate passwords.
 *
 * NOTE: This function sets the session's 'pending_error' member when
 *       returning an non-success status.
 */
apr_status_t
svn_ra_serf__handle_client_cert_pw(void *data,
                                   const char *cert_path,
                                   const char **password);

/*
 * Create a REQUEST with an associated REQ_BKT in the SESSION.
 *
 * If HDRS_BKT is not-NULL, it will be set to a headers_bucket that
 * corresponds to the new request.
 *
 * The request will be METHOD at URL.
 *
 * If BODY_BKT is not-NULL, it will be sent as the request body.
 *
 * If CONTENT_TYPE is not-NULL, it will be sent as the Content-Type header.
 */
svn_error_t *
svn_ra_serf__setup_serf_req(serf_request_t *request,
                            serf_bucket_t **req_bkt, serf_bucket_t **hdrs_bkt,
                            svn_ra_serf__connection_t *conn,
                            const char *method, const char *url,
                            serf_bucket_t *body_bkt, const char *content_type);

/*
 * This function will run the serf context in SESS until *DONE is TRUE.
 */
svn_error_t *
svn_ra_serf__context_run_wait(svn_boolean_t *done,
                              svn_ra_serf__session_t *sess,
                              apr_pool_t *scratch_pool);

/* Callback for response handlers */
typedef svn_error_t *
(*svn_ra_serf__response_handler_t)(serf_request_t *request,
                                   serf_bucket_t *response,
                                   void *handler_baton,
                                   apr_pool_t *pool);

/* Callback for setting up a complete serf request */
typedef svn_error_t *
(*svn_ra_serf__request_setup_t)(serf_request_t *request,
                                void *setup_baton,
                                serf_bucket_t **req_bkt,
                                serf_response_acceptor_t *acceptor,
                                void **acceptor_baton,
                                svn_ra_serf__response_handler_t *handler,
                                void **handler_baton,
                                apr_pool_t *pool);

/* Callback for when a request body is needed. */
typedef svn_error_t *
(*svn_ra_serf__request_body_delegate_t)(serf_bucket_t **body_bkt,
                                        void *baton,
                                        serf_bucket_alloc_t *alloc,
                                        apr_pool_t *pool);

/* Callback for when request headers are needed. */
typedef svn_error_t *
(*svn_ra_serf__request_header_delegate_t)(serf_bucket_t *headers,
                                          void *baton,
                                          apr_pool_t *pool);

/* Callback for when a response has an error. */
typedef svn_error_t *
(*svn_ra_serf__response_error_t)(serf_request_t *request,
                                 serf_bucket_t *response,
                                 int status_code,
                                 void *baton);

/*
 * Structure that can be passed to our default handler to guide the
 * execution of the request through its lifecycle.
 */
typedef struct svn_ra_serf__handler_t {
  /* The HTTP method string of the request */
  const char *method;

  /* The resource to the execute the method on. */
  const char *path;

  /* The content-type of the request body. */
  const char *body_type;

  /* The handler and baton pair for our handler. */
  svn_ra_serf__response_handler_t response_handler;
  void *response_baton;

  /* The handler and baton pair to be executed when a non-recoverable error
   * is detected.  If it is NULL in the presence of an error, an abort() may
   * be triggered.
   */
  svn_ra_serf__response_error_t response_error;
  void *response_error_baton;

  /* This function and baton will be executed when the request is about
   * to be delivered by serf.
   *
   * This just passes through serf's raw request creation parameters.
   * None of the other parameters will be utilized if this field is set.
   */
  svn_ra_serf__request_setup_t setup;
  void *setup_baton;

  /* This function and baton pair allows for custom request headers to
   * be set.
   *
   * It will be executed after the request has been set up but before it is
   * delivered.
   */
  svn_ra_serf__request_header_delegate_t header_delegate;
  void *header_delegate_baton;

  /* This function and baton pair allows a body to be created right before
   * delivery.
   *
   * It will be executed after the request has been set up but before it is
   * delivered.
   *
   * May be NULL if there is no body to send.
   *
   */
  svn_ra_serf__request_body_delegate_t body_delegate;
  void *body_delegate_baton;

  /* The connection and session to be used for this request. */
  svn_ra_serf__connection_t *conn;
  svn_ra_serf__session_t *session;
} svn_ra_serf__handler_t;

/*
 * Helper function to queue a request in the @a handler's connection.
 */
void svn_ra_serf__request_create(svn_ra_serf__handler_t *handler);

/* XML helper callbacks. */

typedef struct svn_ra_serf__xml_state_t {
  /* A numeric value that represents the current state in parsing.
   *
   * Value 0 is reserved for use as the default state.
   */
  int current_state;

  /* Private pointer set by the parsing code. */
  void *private;

  /* Allocations should be made in this pool to match the lifetime of the
   * state.
   */
  apr_pool_t *pool;

  /* The currently-declared namespace for this state. */
  svn_ra_serf__ns_t *ns_list;

  /* Our previous states. */
  struct svn_ra_serf__xml_state_t *prev;
} svn_ra_serf__xml_state_t;

/* Forward declaration of the XML parser structure. */
typedef struct svn_ra_serf__xml_parser_t svn_ra_serf__xml_parser_t;

/* Callback invoked with @a baton by our XML @a parser when an element with
 * the @a name containing @a attrs is opened.
 */
typedef svn_error_t *
(*svn_ra_serf__xml_start_element_t)(svn_ra_serf__xml_parser_t *parser,
                                    void *baton,
                                    svn_ra_serf__dav_props_t name,
                                    const char **attrs);

/* Callback invoked with @a baton by our XML @a parser when an element with
 * the @a name is closed.
 */
typedef svn_error_t *
(*svn_ra_serf__xml_end_element_t)(svn_ra_serf__xml_parser_t *parser,
                                  void *baton,
                                  svn_ra_serf__dav_props_t name);

/* Callback invoked with @a baton by our XML @a parser when a CDATA portion
 * of @a data with size @a len is encountered.
 *
 * This may be invoked multiple times for the same tag.
 *
 * @see svn_ra_serf__expand_string
 */
typedef svn_error_t *
(*svn_ra_serf__xml_cdata_chunk_handler_t)(svn_ra_serf__xml_parser_t *parser,
                                          void *baton,
                                          const char *data,
                                          apr_size_t len);

/*
 * Helper structure associated with handle_xml_parser handler that will
 * specify how an XML response will be processed.
 */
struct svn_ra_serf__xml_parser_t {
  /* Temporary allocations should be made in this pool. */
  apr_pool_t *pool;

  /* Caller-specific data passed to the start, end, cdata callbacks.  */
  void *user_data;

  /* Callback invoked when a tag is opened. */
  svn_ra_serf__xml_start_element_t start;

  /* Callback invoked when a tag is closed. */
  svn_ra_serf__xml_end_element_t end;

  /* Callback invoked when a cdata chunk is received. */
  svn_ra_serf__xml_cdata_chunk_handler_t cdata;

  /* Our associated expat-based XML parser. */
  XML_Parser xmlp;

  /* Our current state. */
  svn_ra_serf__xml_state_t *state;

  /* Our previously used states (will be reused). */
  svn_ra_serf__xml_state_t *free_state;

  /* If non-NULL, the status code of the response will be stored here.
   *
   * If this is NULL and an error is received, an abort will be triggered.
   */
  int *status_code;

  /* If non-NULL, this is the value of the response's Location header.
   */
  const char *location;

  /* If non-NULL, this value will be set to TRUE when the response is
   * completed.
   */
  svn_boolean_t *done;

  /* If non-NULL, when this parser completes, it will add done_item to
   * the list.
   */
  svn_ra_serf__list_t **done_list;

  /* A pointer to the item that will be inserted into the list upon
   * completeion.
   */
  svn_ra_serf__list_t *done_item;

  /* If this flag is TRUE, errors during parsing will be ignored.
   *
   * This is mainly used when we are processing an error XML response to
   * avoid infinite loops.
   */
  svn_boolean_t ignore_errors;

  /* If an error occurred, this value will be non-NULL. */
  svn_error_t *error;

  /* Deciding whether to pause, or not, is performed within the parsing
     callbacks. If a callback decides to set this flag, then the loop
     driving the parse (generally, a series of calls to serf_context_run())
     is going to need to coordinate the un-pausing of the parser by
     processing pending content. Thus, deciding to pause the parser is a
     coordinate effort rather than merely setting this flag.

     When an XML parsing callback sets this flag, note that additional
     elements may be parsed (as the current buffer is consumed). At some
     point, the flag will be recognized and arriving network content will
     be stashed away in the PENDING structure (see below).

     At some point, the controlling loop should clear this value. The
     underlying network processing will note the change and begin passing
     content into the XML callbacks.

     Note that the controlling loop should also process pending content
     since the arriving network content will typically finish first.  */
  svn_boolean_t paused;

  /* While the XML parser is paused, content arriving from the server
     must be saved locally. We cannot stop reading, or the server may
     decide to drop the connection. The content will be stored in memory
     up to a certain limit, and will then be spilled over to disk.

     See libsvn_ra_serf/util.c  */
  struct svn_ra_serf__pending_t *pending;

  /* Response restart support */
  const void *headers_baton; /* Last pointer to headers */
  apr_off_t skip_size; /* Number of bytes to skip */
  apr_off_t read_size; /* Number of bytes read from response */
};

/*
 * Parses a server-side error message into a local Subversion error.
 */
typedef struct svn_ra_serf__server_error_t {
  /* Our local representation of the error. */
  svn_error_t *error;

  /* Have we checked to see if there's an XML error in this response? */
  svn_boolean_t init;

  /* Was there an XML error response? */
  svn_boolean_t has_xml_response;

  /* Are we done with the response? */
  svn_boolean_t done;

  /* Have we seen an error tag? */
  svn_boolean_t in_error;

  /* Have we seen a HTTP "412 Precondition Failed" error? */
  svn_boolean_t contains_precondition_error;

  /* Should we be collecting the XML cdata? */
  svn_boolean_t collect_cdata;

  /* Collected cdata. NULL if cdata not needed. */
  svn_stringbuf_t *cdata;

  /* XML parser and namespace used to parse the remote response */
  svn_ra_serf__xml_parser_t parser;
} svn_ra_serf__server_error_t;

/* A simple request context that can be passed to handle_status_only. */
typedef struct svn_ra_serf__simple_request_context_t {
  apr_pool_t *pool;

  /* The HTTP status code of the response */
  int status;

  /* The HTTP status line of the response */
  const char *reason;

  /* The Location header value of the response, or NULL if there
     wasn't one. */
  const char *location;

  /* This value is set to TRUE when the response is completed. */
  svn_boolean_t done;

  /* If an error occurred, this value will be initialized. */
  svn_ra_serf__server_error_t server_error;
} svn_ra_serf__simple_request_context_t;

/*
 * Serf handler for @a request / @a response pair that takes in a
 * @a baton (@see svn_ra_serf__simple_request_context_t).
 * Implements svn_ra_serf__response_handler_t.
 *
 * Temporary allocations are made in @a pool.
 */
svn_error_t *
svn_ra_serf__handle_status_only(serf_request_t *request,
                                serf_bucket_t *response,
                                void *baton,
                                apr_pool_t *pool);

/*
 * Handler that discards the entire @a response body associated with a
 * @a request.  Implements svn_ra_serf__response_handler_t.
 *
 * If @a baton is a svn_ra_serf__server_error_t (i.e. non-NULL) and an
 * error is detected, it will be populated for later detection.
 *
 * All temporary allocations will be made in a @a pool.
 */
svn_error_t *
svn_ra_serf__handle_discard_body(serf_request_t *request,
                                 serf_bucket_t *response,
                                 void *baton,
                                 apr_pool_t *pool);

/*
 * Handler that retrieves the embedded XML error response from the
 * the @a response body associated with a @a request.
 * Implements svn_ra_serf__response_handler_t.
 *
 * All temporary allocations will be made in a @a pool.
 */
svn_error_t *
svn_ra_serf__handle_server_error(serf_request_t *request,
                                 serf_bucket_t *response,
                                 apr_pool_t *pool);

/*
 * Handler that retrieves the embedded XML multistatus response from the
 * the @a RESPONSE body associated with a @a REQUEST. *DONE is set to TRUE.
 * Implements svn_ra_serf__response_handler_t.
 *
 * The @a BATON should be of type svn_ra_serf__simple_request_context_t.
 *
 * All temporary allocations will be made in a @a pool.
 */
svn_error_t *
svn_ra_serf__handle_multistatus_only(serf_request_t *request,
                                     serf_bucket_t *response,
                                     void *baton,
                                     apr_pool_t *pool);

/*
 * This function will feed the RESPONSE body into XMLP.  When parsing is
 * completed (i.e. an EOF is received), *DONE is set to TRUE.
 * Implements svn_ra_serf__response_handler_t.
 *
 * If an error occurs during processing RESP_ERR is invoked with the
 * RESP_ERR_BATON.
 *
 * Temporary allocations are made in POOL.
 */
svn_error_t *
svn_ra_serf__handle_xml_parser(serf_request_t *request,
                               serf_bucket_t *response,
                               void *handler_baton,
                               apr_pool_t *pool);

/* serf_response_handler_t implementation that completely discards
 * the response.
 *
 * All temporary allocations will be made in @a pool.
 */
apr_status_t
svn_ra_serf__response_discard_handler(serf_request_t *request,
                                      serf_bucket_t *response,
                                      void *baton,
                                      apr_pool_t *pool);

/* Return the value of the RESPONSE's Location header if any, or NULL
 * otherwise.  All allocations will be made in POOL.
 */
const char *
svn_ra_serf__response_get_location(serf_bucket_t *response,
                                   apr_pool_t *pool);

/** XML helper functions. **/

/*
 * Advance the internal XML @a parser to the @a state.
 */
void
svn_ra_serf__xml_push_state(svn_ra_serf__xml_parser_t *parser,
                            int state);

/*
 * Return to the previous internal XML @a parser state.
 */
void
svn_ra_serf__xml_pop_state(svn_ra_serf__xml_parser_t *parser);


svn_error_t *
svn_ra_serf__process_pending(svn_ra_serf__xml_parser_t *parser,
                             apr_pool_t *scratch_pool);


/*
 * Add the appropriate serf buckets to @a agg_bucket represented by
 * the XML * @a tag and @a value.
 *
 * The bucket will be allocated from @a bkt_alloc.
 */
void
svn_ra_serf__add_tag_buckets(serf_bucket_t *agg_bucket,
                             const char *tag,
                             const char *value,
                             serf_bucket_alloc_t *bkt_alloc);

/*
 * Add the appropriate serf buckets to AGG_BUCKET with standard XML header:
 *  <?xml version="1.0" encoding="utf-8"?>
 *
 * The bucket will be allocated from BKT_ALLOC.
 */
void
svn_ra_serf__add_xml_header_buckets(serf_bucket_t *agg_bucket,
                                    serf_bucket_alloc_t *bkt_alloc);

/*
 * Add the appropriate serf buckets to AGG_BUCKET representing xml tag open
 * with name TAG.
 *
 * Take the tag's attributes from varargs, a NULL-terminated list of
 * alternating <tt>char *</tt> key and <tt>char *</tt> val.  Do xml-escaping
 * on each val. Attribute will be ignored if it's value is NULL.
 *
 * The bucket will be allocated from BKT_ALLOC.
 */
void
svn_ra_serf__add_open_tag_buckets(serf_bucket_t *agg_bucket,
                                  serf_bucket_alloc_t *bkt_alloc,
                                  const char *tag,
                                  ...);

/*
 * Add the appropriate serf buckets to AGG_BUCKET representing xml tag close
 * with name TAG.
 *
 * The bucket will be allocated from BKT_ALLOC.
 */
void
svn_ra_serf__add_close_tag_buckets(serf_bucket_t *agg_bucket,
                                   serf_bucket_alloc_t *bkt_alloc,
                                   const char *tag);

/*
 * Add the appropriate serf buckets to AGG_BUCKET with xml-escaped
 * version of DATA.
 *
 * The bucket will be allocated from BKT_ALLOC.
 */
void
svn_ra_serf__add_cdata_len_buckets(serf_bucket_t *agg_bucket,
                                   serf_bucket_alloc_t *bkt_alloc,
                                   const char *data, apr_size_t len);
/*
 * Look up the @a attrs array for namespace definitions and add each one
 * to the @a ns_list of namespaces.
 *
 * New namespaces will be allocated in @a pool.
 */
void
svn_ra_serf__define_ns(svn_ra_serf__ns_t **ns_list,
                       const char **attrs,
                       apr_pool_t *pool);

/*
 * Look up @a name in the @a ns_list list for previously declared namespace
 * definitions.
 *
 * Return (in @a *returned_prop_name) a #svn_ra_serf__dav_props_t tuple
 * representing the expanded name.
 */
void
svn_ra_serf__expand_ns(svn_ra_serf__dav_props_t *returned_prop_name,
                       svn_ra_serf__ns_t *ns_list,
                       const char *name);

/*
 * Expand the string represented by @a cur with a current size of @a
 * cur_len by appending @a new with a size of @a new_len.
 *
 * The reallocated string is made in @a pool.
 */
void
svn_ra_serf__expand_string(const char **cur, apr_size_t *cur_len,
                           const char *new, apr_size_t new_len,
                           apr_pool_t *pool);

/** PROPFIND-related functions **/

/* Opaque structure representing PROPFINDs. */
typedef struct svn_ra_serf__propfind_context_t svn_ra_serf__propfind_context_t;

/*
 * Returns a flag representing whether the PROPFIND @a ctx is completed.
 */
svn_boolean_t
svn_ra_serf__propfind_is_done(svn_ra_serf__propfind_context_t *ctx);

/*
 * Returns the response status code of the PROPFIND @a ctx.
 */
int
svn_ra_serf__propfind_status_code(svn_ra_serf__propfind_context_t *ctx);

/*
 * This function will deliver a PROP_CTX PROPFIND request in the SESS
 * serf context for the properties listed in LOOKUP_PROPS at URL for
 * DEPTH ("0","1","infinity").
 *
 * This function will not block waiting for the response. Callers are
 * expected to call svn_ra_serf__wait_for_props().
 */
svn_error_t *
svn_ra_serf__deliver_props(svn_ra_serf__propfind_context_t **prop_ctx,
                           apr_hash_t *prop_vals,
                           svn_ra_serf__session_t *sess,
                           svn_ra_serf__connection_t *conn,
                           const char *url,
                           svn_revnum_t rev,
                           const char *depth,
                           const svn_ra_serf__dav_props_t *lookup_props,
                           svn_ra_serf__list_t **done_list,
                           apr_pool_t *pool);

/*
 * This helper function will block until the PROP_CTX indicates that is done
 * or another error is returned.
 */
svn_error_t *
svn_ra_serf__wait_for_props(svn_ra_serf__propfind_context_t *prop_ctx,
                            svn_ra_serf__session_t *sess,
                            apr_pool_t *pool);

/* This is a blocking version of deliver_props.

   The properties are fetched and placed into RESULTS, allocated in
   RESULT_POOL.

   ### more docco about the other params.

   Temporary allocations are made in SCRATCH_POOL.
*/
svn_error_t *
svn_ra_serf__retrieve_props(apr_hash_t **results,
                            svn_ra_serf__session_t *sess,
                            svn_ra_serf__connection_t *conn,
                            const char *url,
                            svn_revnum_t rev,
                            const char *depth,
                            const svn_ra_serf__dav_props_t *props,
                            apr_pool_t *result_pool,
                            apr_pool_t *scratch_pool);

/* Set PROPS for PATH at REV revision with a NS:NAME VAL.
 *
 * The POOL governs allocation.
 */
void
svn_ra_serf__set_ver_prop(apr_hash_t *props,
                          const char *path, svn_revnum_t rev,
                          const char *ns, const char *name,
                          const svn_string_t *val, apr_pool_t *pool);
#define svn_ra_serf__set_rev_prop svn_ra_serf__set_ver_prop

/** Property walker functions **/

typedef svn_error_t *
(*svn_ra_serf__walker_visitor_t)(void *baton,
                                 const char *ns,
                                 const char *name,
                                 const svn_string_t *val,
                                 apr_pool_t *pool);

svn_error_t *
svn_ra_serf__walk_all_props(apr_hash_t *props,
                            const char *name,
                            svn_revnum_t rev,
                            svn_ra_serf__walker_visitor_t walker,
                            void *baton,
                            apr_pool_t *pool);

typedef svn_error_t *
(*svn_ra_serf__path_rev_walker_t)(void *baton,
                                  const char *path, apr_ssize_t path_len,
                                  const char *ns, apr_ssize_t ns_len,
                                  const char *name, apr_ssize_t name_len,
                                  const svn_string_t *val,
                                  apr_pool_t *pool);
svn_error_t *
svn_ra_serf__walk_all_paths(apr_hash_t *props,
                            svn_revnum_t rev,
                            svn_ra_serf__path_rev_walker_t walker,
                            void *baton,
                            apr_pool_t *pool);


/* Map a property name, as passed over the wire, into its corresponding
   Subversion-internal name. The returned name will be a static value,
   or allocated within RESULT_POOL.

   If the property should be ignored (eg. some DAV properties), then NULL
   will be returned.  */
const char *
svn_ra_serf__svnname_from_wirename(const char *ns,
                                   const char *name,
                                   apr_pool_t *result_pool);


/* Select the basic revision properties from the set of "all" properties.
   Return these in *REVPROPS, allocated from RESULT_POOL.  */
svn_error_t *
svn_ra_serf__select_revprops(apr_hash_t **revprops,
                             const char *name,
                             svn_revnum_t rev,
                             apr_hash_t *all_revprops,
                             apr_pool_t *result_pool,
                             apr_pool_t *scratch_pool);


/* PROPS is nested hash tables mapping REV -> PATH -> NS -> NAME -> VALUE.
   This function takes the tree of tables identified by PATH and REVISION
   (resulting in NS:NAME:VALUE hashes) and flattens them into a set of
   names to VALUE. The names are composed of NS:NAME, with specific
   rewrite from wire names (DAV) to SVN names. This mapping is managed
   by the svn_ra_serf__set_baton_props() function.

   FLAT_PROPS is allocated in RESULT_POOL.
   ### right now, we do a shallow copy from PROPS to FLAT_PROPS. therefore,
   ### the names and values in PROPS must be in the proper pool.

   Temporary allocations are made in SCRATCH_POOL.  */
svn_error_t *
svn_ra_serf__flatten_props(apr_hash_t **flat_props,
                           apr_hash_t *props,
                           const char *path,
                           svn_revnum_t revision,
                           apr_pool_t *result_pool,
                           apr_pool_t *scratch_pool);


/* Return the property value for PATH at REV revision with a NS:NAME.
 * PROPS is a four-level nested hash: (svn_revnum_t => char *path =>
 * char *ns => char *name => svn_string_t *). */
const svn_string_t *
svn_ra_serf__get_ver_prop_string(apr_hash_t *props,
                                 const char *path, svn_revnum_t rev,
                                 const char *ns, const char *name);

/* Same as svn_ra_serf__get_ver_prop_string(), but returns a C string. */
const char *
svn_ra_serf__get_ver_prop(apr_hash_t *props,
                          const char *path, svn_revnum_t rev,
                          const char *ns, const char *name);

/* Same as svn_ra_serf__get_ver_prop_string(), but for the unknown revision. */
const svn_string_t *
svn_ra_serf__get_prop_string(apr_hash_t *props,
                             const char *path,
                             const char *ns,
                             const char *name);

/* Same as svn_ra_serf__get_ver_prop(), but for the unknown revision. */
const char *
svn_ra_serf__get_prop(apr_hash_t *props,
                      const char *path,
                      const char *ns,
                      const char *name);

/* Same as svn_ra_serf__set_rev_prop(), but for the unknown revision. */
void
svn_ra_serf__set_prop(apr_hash_t *props, const char *path,
                      const char *ns, const char *name,
                      const svn_string_t *val, apr_pool_t *pool);

svn_error_t *
svn_ra_serf__get_resource_type(svn_node_kind_t *kind,
                               apr_hash_t *props,
                               const char *url,
                               svn_revnum_t revision);


/** MERGE-related functions **/

typedef struct svn_ra_serf__merge_context_t svn_ra_serf__merge_context_t;

svn_boolean_t*
svn_ra_serf__merge_get_done_ptr(svn_ra_serf__merge_context_t *ctx);

svn_commit_info_t*
svn_ra_serf__merge_get_commit_info(svn_ra_serf__merge_context_t *ctx);

int
svn_ra_serf__merge_get_status(svn_ra_serf__merge_context_t *ctx);

void
svn_ra_serf__merge_lock_token_list(apr_hash_t *lock_tokens,
                                   const char *parent,
                                   serf_bucket_t *body,
                                   serf_bucket_alloc_t *alloc,
                                   apr_pool_t *pool);

/* Create an MERGE request aimed at the SESSION url, requesting the
   merge of the resource identified by MERGE_RESOURCE_URL.
   LOCK_TOKENS is a hash mapping paths to lock tokens owned by the
   client.  If KEEP_LOCKS is set, instruct the server to not release
   locks set on the paths included in this commit.  */
svn_error_t *
svn_ra_serf__merge_create_req(svn_ra_serf__merge_context_t **merge_ctx,
                              svn_ra_serf__session_t *session,
                              svn_ra_serf__connection_t *conn,
                              const char *merge_resource_url,
                              apr_hash_t *lock_tokens,
                              svn_boolean_t keep_locks,
                              apr_pool_t *pool);

/** OPTIONS-related functions **/

typedef struct svn_ra_serf__options_context_t svn_ra_serf__options_context_t;

/* Is this OPTIONS-request done yet? */
svn_boolean_t*
svn_ra_serf__get_options_done_ptr(svn_ra_serf__options_context_t *ctx);

const char *
svn_ra_serf__options_get_activity_collection(svn_ra_serf__options_context_t *ctx);

svn_revnum_t
svn_ra_serf__options_get_youngest_rev(svn_ra_serf__options_context_t *ctx);

/* Create an OPTIONS request.  When run, ask for an
   activity-collection-set in the request body (retrievable via
   accessor above) and also parse the server's capability headers into
   the SESSION->capabilites hash. */
svn_error_t *
svn_ra_serf__create_options_req(svn_ra_serf__options_context_t **opt_ctx,
                                svn_ra_serf__session_t *session,
                                svn_ra_serf__connection_t *conn,
                                const char *path,
                                apr_pool_t *pool);

/* Set @a VCC_URL to the default VCC for our repository based on @a
 * ORIG_PATH for the session @a SESSION, ensuring that the VCC URL and
 * repository root URLs are cached in @a SESSION.  Use @a CONN for any
 * required network communications if it is non-NULL; otherwise use the
 * default connection.
 *
 * All temporary allocations will be made in @a POOL. */
svn_error_t *
svn_ra_serf__discover_vcc(const char **vcc_url,
                          svn_ra_serf__session_t *session,
                          svn_ra_serf__connection_t *conn,
                          apr_pool_t *pool);

/* Set @a REPORT_TARGET to the URI of the resource at which generic
 * (path-agnostic) REPORTs should be aimed for @a SESSION.  Use @a
 * CONN for any required network communications if it is non-NULL;
 * otherwise use the default connection.
 *
 * All temporary allocations will be made in @a POOL.
 */
svn_error_t *
svn_ra_serf__report_resource(const char **report_target,
                             svn_ra_serf__session_t *session,
                             svn_ra_serf__connection_t *conn,
                             apr_pool_t *pool);

/* Set @a REL_PATH to a path (not URI-encoded) relative to the root of
 * the repository pointed to by @a SESSION, based on original path
 * (URI-encoded) @a ORIG_PATH.  Use @a CONN for any required network
 * communications if it is non-NULL; otherwise use the default
 * connection.  Use POOL for allocations.  */
svn_error_t *
svn_ra_serf__get_relative_path(const char **rel_path,
                               const char *orig_path,
                               svn_ra_serf__session_t *session,
                               svn_ra_serf__connection_t *conn,
                               apr_pool_t *pool);

/* Set *BC_URL to the baseline collection url, and set *BC_RELATIVE to
 * the path relative to that url for URL in REVISION using SESSION.
 * BC_RELATIVE will be URI decoded.
 *
 * REVISION may be SVN_INVALID_REVNUM (to mean "the current HEAD
 * revision").  If URL is NULL, use SESSION's session url.
 *
 * If LATEST_REVNUM is not NULL, set it to the baseline revision. If
 * REVISION was set to SVN_INVALID_REVNUM, this will return the current
 * HEAD revision.
 *
 * If non-NULL, use CONN for communications with the server;
 * otherwise, use the default connection.
 *
 * Use POOL for all allocations.
 */
svn_error_t *
svn_ra_serf__get_baseline_info(const char **bc_url,
                               const char **bc_relative,
                               svn_ra_serf__session_t *session,
                               svn_ra_serf__connection_t *conn,
                               const char *url,
                               svn_revnum_t revision,
                               svn_revnum_t *latest_revnum,
                               apr_pool_t *pool);

/** RA functions **/

svn_error_t *
svn_ra_serf__get_log(svn_ra_session_t *session,
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

svn_error_t *
svn_ra_serf__get_locations(svn_ra_session_t *session,
                           apr_hash_t **locations,
                           const char *path,
                           svn_revnum_t peg_revision,
                           const apr_array_header_t *location_revisions,
                           apr_pool_t *pool);

svn_error_t *
svn_ra_serf__get_location_segments(svn_ra_session_t *session,
                                   const char *path,
                                   svn_revnum_t peg_revision,
                                   svn_revnum_t start_rev,
                                   svn_revnum_t end_rev,
                                   svn_location_segment_receiver_t receiver,
                                   void *receiver_baton,
                                   apr_pool_t *pool);

svn_error_t *
svn_ra_serf__do_diff(svn_ra_session_t *session,
                     const svn_ra_reporter3_t **reporter,
                     void **report_baton,
                     svn_revnum_t revision,
                     const char *diff_target,
                     svn_depth_t depth,
                     svn_boolean_t ignore_ancestry,
                     svn_boolean_t text_deltas,
                     const char *versus_url,
                     const svn_delta_editor_t *diff_editor,
                     void *diff_baton,
                     apr_pool_t *pool);

svn_error_t *
svn_ra_serf__do_status(svn_ra_session_t *ra_session,
                       const svn_ra_reporter3_t **reporter,
                       void **report_baton,
                       const char *status_target,
                       svn_revnum_t revision,
                       svn_depth_t depth,
                       const svn_delta_editor_t *status_editor,
                       void *status_baton,
                       apr_pool_t *pool);

svn_error_t *
svn_ra_serf__do_update(svn_ra_session_t *ra_session,
                       const svn_ra_reporter3_t **reporter,
                       void **report_baton,
                       svn_revnum_t revision_to_update_to,
                       const char *update_target,
                       svn_depth_t depth,
                       svn_boolean_t send_copyfrom_args,
                       const svn_delta_editor_t *update_editor,
                       void *update_baton,
                       apr_pool_t *pool);

svn_error_t *
svn_ra_serf__do_switch(svn_ra_session_t *ra_session,
                       const svn_ra_reporter3_t **reporter,
                       void **report_baton,
                       svn_revnum_t revision_to_switch_to,
                       const char *switch_target,
                       svn_depth_t depth,
                       const char *switch_url,
                       const svn_delta_editor_t *switch_editor,
                       void *switch_baton,
                       apr_pool_t *pool);

svn_error_t *
svn_ra_serf__get_file_revs(svn_ra_session_t *session,
                           const char *path,
                           svn_revnum_t start,
                           svn_revnum_t end,
                           svn_boolean_t include_merged_revisions,
                           svn_file_rev_handler_t handler,
                           void *handler_baton,
                           apr_pool_t *pool);

svn_error_t *
svn_ra_serf__get_dated_revision(svn_ra_session_t *session,
                                svn_revnum_t *revision,
                                apr_time_t tm,
                                apr_pool_t *pool);

svn_error_t *
svn_ra_serf__get_commit_editor(svn_ra_session_t *session,
                               const svn_delta_editor_t **editor,
                               void **edit_baton,
                               apr_hash_t *revprop_table,
                               svn_commit_callback2_t callback,
                               void *callback_baton,
                               apr_hash_t *lock_tokens,
                               svn_boolean_t keep_locks,
                               apr_pool_t *pool);

svn_error_t *
svn_ra_serf__get_file(svn_ra_session_t *session,
                      const char *path,
                      svn_revnum_t revision,
                      svn_stream_t *stream,
                      svn_revnum_t *fetched_rev,
                      apr_hash_t **props,
                      apr_pool_t *pool);

svn_error_t *
svn_ra_serf__change_rev_prop(svn_ra_session_t *session,
                             svn_revnum_t rev,
                             const char *name,
                             const svn_string_t *const *old_value_p,
                             const svn_string_t *value,
                             apr_pool_t *pool);

svn_error_t *
svn_ra_serf__replay(svn_ra_session_t *ra_session,
                    svn_revnum_t revision,
                    svn_revnum_t low_water_mark,
                    svn_boolean_t text_deltas,
                    const svn_delta_editor_t *editor,
                    void *edit_baton,
                    apr_pool_t *pool);

svn_error_t *
svn_ra_serf__replay_range(svn_ra_session_t *ra_session,
                          svn_revnum_t start_revision,
                          svn_revnum_t end_revision,
                          svn_revnum_t low_water_mark,
                          svn_boolean_t send_deltas,
                          svn_ra_replay_revstart_callback_t revstart_func,
                          svn_ra_replay_revfinish_callback_t revfinish_func,
                          void *replay_baton,
                          apr_pool_t *pool);

svn_error_t *
svn_ra_serf__lock(svn_ra_session_t *ra_session,
                  apr_hash_t *path_revs,
                  const char *comment,
                  svn_boolean_t force,
                  svn_ra_lock_callback_t lock_func,
                  void *lock_baton,
                  apr_pool_t *pool);

svn_error_t *
svn_ra_serf__unlock(svn_ra_session_t *ra_session,
                    apr_hash_t *path_tokens,
                    svn_boolean_t force,
                    svn_ra_lock_callback_t lock_func,
                    void *lock_baton,
                    apr_pool_t *pool);

svn_error_t *
svn_ra_serf__get_lock(svn_ra_session_t *ra_session,
                      svn_lock_t **lock,
                      const char *path,
                      apr_pool_t *pool);

svn_error_t *
svn_ra_serf__get_locks(svn_ra_session_t *ra_session,
                       apr_hash_t **locks,
                       const char *path,
                       svn_depth_t depth,
                       apr_pool_t *pool);

svn_error_t * svn_ra_serf__get_mergeinfo(
  svn_ra_session_t *ra_session,
  apr_hash_t **mergeinfo,
  const apr_array_header_t *paths,
  svn_revnum_t revision,
  svn_mergeinfo_inheritance_t inherit,
  svn_boolean_t include_descendants,
  apr_pool_t *pool);

/* Exchange capabilities with the server, by sending an OPTIONS
 * request announcing the client's capabilities, and by filling
 * SERF_SESS->capabilities with the server's capabilities as read from
 * the response headers.  Use POOL only for temporary allocation.
 *
 * If the CORRECTED_URL is non-NULL, allow the OPTIONS response to
 * report a server-dictated redirect or relocation (HTTP 301 or 302
 * error codes), setting *CORRECTED_URL to the value of the corrected
 * repository URL.  Otherwise, such responses from the server will
 * generate an error.  (In either case, no capabilities are exchanged
 * if there is, in fact, such a response from the server.)
 */
svn_error_t *
svn_ra_serf__exchange_capabilities(svn_ra_serf__session_t *serf_sess,
                                   const char **corrected_url,
                                   apr_pool_t *pool);

/* Implements the has_capability RA layer function. */
svn_error_t *
svn_ra_serf__has_capability(svn_ra_session_t *ra_session,
                            svn_boolean_t *has,
                            const char *capability,
                            apr_pool_t *pool);

/* Implements the get_deleted_rev RA layer function. */
svn_error_t *
svn_ra_serf__get_deleted_rev(svn_ra_session_t *session,
                             const char *path,
                             svn_revnum_t peg_revision,
                             svn_revnum_t end_revision,
                             svn_revnum_t *revision_deleted,
                             apr_pool_t *pool);

/*** Authentication handler declarations ***/

/**
 * Callback function that loads the credentials for Basic and Digest
 * authentications, both for server and proxy authentication.
 */
apr_status_t
svn_ra_serf__credentials_callback(char **username, char **password,
                                  serf_request_t *request, void *baton,
                                  int code, const char *authn_type,
                                  const char *realm,
                                  apr_pool_t *pool);


/*** General utility functions ***/

/**
 * Convert an HTTP STATUS_CODE resulting from a WebDAV request against
 * PATH to the relevant error code.  Use the response-supplied LOCATION
 * where it necessary.
 */
svn_error_t *
svn_ra_serf__error_on_status(int status_code,
                             const char *path,
                             const char *location);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_LIBSVN_RA_SERF_RA_SERF_H */

/*
 * mirror.c: Use a transparent proxy to mirror Subversion instances.
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

#include <assert.h>

#include <apr_strmatch.h>

#include <httpd.h>
#include <http_core.h>

#include "private/svn_fspath.h"

#include "dav_svn.h"


/* Tweak the request record R, and add the necessary filters, so that
   the request is ready to be proxied away.  MASTER_URI is the URI
   specified in the SVNMasterURI Apache configuration value.
   URI_SEGMENT is the URI bits relative to the repository root (but if
   non-empty, *does* have a leading slash delimiter).
   MASTER_URI and URI_SEGMENT are not URI-encoded. */
static int proxy_request_fixup(request_rec *r,
                               const char *master_uri,
                               const char *uri_segment)
{
    if (uri_segment[0] != '\0' && uri_segment[0] != '/')
      {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, SVN_ERR_BAD_CONFIG_VALUE, r,
                     "Invalid URI segment '%s' in slave fixup",
                      uri_segment);
        return HTTP_INTERNAL_SERVER_ERROR;
      }

    r->proxyreq = PROXYREQ_REVERSE;
    r->uri = r->unparsed_uri;
    r->filename = (char *) svn_path_uri_encode(apr_pstrcat(r->pool, "proxy:",
                                                           master_uri,
                                                           uri_segment,
                                                           (char *)NULL),
                                               r->pool);
    r->handler = "proxy-server";
    ap_add_output_filter("LocationRewrite", NULL, r, r->connection);
    ap_add_output_filter("ReposRewrite", NULL, r, r->connection);
    ap_add_input_filter("IncomingRewrite", NULL, r, r->connection);
    return OK;
}


int dav_svn__proxy_request_fixup(request_rec *r)
{
    const char *root_dir, *master_uri, *special_uri;

    root_dir = dav_svn__get_root_dir(r);
    master_uri = dav_svn__get_master_uri(r);
    special_uri = dav_svn__get_special_uri(r);

    if (root_dir && master_uri) {
        const char *seg;

        /* We know we can always safely handle these. */
        if (r->method_number == M_REPORT ||
            r->method_number == M_OPTIONS) {
            return OK;
        }

        /* These are read-only requests -- the kind we like to handle
           ourselves -- but we need to make sure they aren't aimed at
           resources that only exist on the master server such as
           working resource URIs or the HTTPv2 transaction root and
           transaction tree resouces. */
        if (r->method_number == M_PROPFIND ||
            r->method_number == M_GET) {
            if ((seg = ap_strstr(r->uri, root_dir))) {
                if (ap_strstr_c(seg, apr_pstrcat(r->pool, special_uri,
                                                 "/wrk/", (char *)NULL))
                    || ap_strstr_c(seg, apr_pstrcat(r->pool, special_uri,
                                                    "/txn/", (char *)NULL))
                    || ap_strstr_c(seg, apr_pstrcat(r->pool, special_uri,
                                                    "/txr/", (char *)NULL))) {
                    int rv;
                    seg += strlen(root_dir);
                    rv = proxy_request_fixup(r, master_uri, seg);
                    if (rv) return rv;
                }
            }
            return OK;
        }

        /* If this is a write request aimed at a public URI (such as
           MERGE, LOCK, UNLOCK, etc.) or any as-yet-unhandled request
           using a "special URI", we have to doctor it a bit for proxying. */
        seg = ap_strstr(r->uri, root_dir);
        if (seg && (r->method_number == M_MERGE ||
                    r->method_number == M_LOCK ||
                    r->method_number == M_UNLOCK ||
                    ap_strstr_c(seg, special_uri))) {
            int rv;
            seg += strlen(root_dir);
            rv = proxy_request_fixup(r, master_uri, seg);
            if (rv) return rv;
            return OK;
        }
    }
    return OK;
}

typedef struct locate_ctx_t
{
    const apr_strmatch_pattern *pattern;
    apr_size_t pattern_len;
    const char *localpath;
    apr_size_t  localpath_len;
    const char *remotepath;
    apr_size_t  remotepath_len;
} locate_ctx_t;

apr_status_t dav_svn__location_in_filter(ap_filter_t *f,
                                         apr_bucket_brigade *bb,
                                         ap_input_mode_t mode,
                                         apr_read_type_e block,
                                         apr_off_t readbytes)
{
    request_rec *r = f->r;
    locate_ctx_t *ctx = f->ctx;
    apr_status_t rv;
    apr_bucket *bkt;
    const char *master_uri, *root_dir, *canonicalized_uri;
    apr_uri_t uri;

    /* Don't filter if we're in a subrequest or we aren't setup to
       proxy anything. */
    master_uri = dav_svn__get_master_uri(r);
    if (r->main || !master_uri) {
        ap_remove_input_filter(f);
        return ap_get_brigade(f->next, bb, mode, block, readbytes);
    }

    /* And don't filter if our search-n-replace would be a noop anyway
       (that is, if our root path matches that of the master server). */
    apr_uri_parse(r->pool, master_uri, &uri);
    root_dir = dav_svn__get_root_dir(r);
    canonicalized_uri = svn_urlpath__canonicalize(uri.path, r->pool);
    if (strcmp(canonicalized_uri, root_dir) == 0) {
        ap_remove_input_filter(f);
        return ap_get_brigade(f->next, bb, mode, block, readbytes);
    }

    /* ### FIXME: While we want to fix up any locations in proxied XML
       ### requests, we do *not* want to be futzing with versioned (or
       ### to-be-versioned) data, such as the file contents present in
       ### PUT requests and properties in PROPPATCH requests.
       ### See issue #3445 for details. */

    /* We are url encoding the current url and the master url
       as incoming(from client) request body has it encoded already. */
    canonicalized_uri = svn_path_uri_encode(canonicalized_uri, r->pool);
    root_dir = svn_path_uri_encode(root_dir, r->pool);
    if (!f->ctx) {
        ctx = f->ctx = apr_pcalloc(r->pool, sizeof(*ctx));
        ctx->remotepath = canonicalized_uri;
        ctx->remotepath_len = strlen(ctx->remotepath);
        ctx->localpath = root_dir;
        ctx->localpath_len = strlen(ctx->localpath);
        ctx->pattern = apr_strmatch_precompile(r->pool, ctx->localpath, 1);
        ctx->pattern_len = ctx->localpath_len;
    }

    rv = ap_get_brigade(f->next, bb, mode, block, readbytes);
    if (rv) {
        return rv;
    }

    bkt = APR_BRIGADE_FIRST(bb);
    while (bkt != APR_BRIGADE_SENTINEL(bb)) {

        const char *data, *match;
        apr_size_t len;

        if (APR_BUCKET_IS_METADATA(bkt)) {
            bkt = APR_BUCKET_NEXT(bkt);
            continue;
        }

        /* read */
        apr_bucket_read(bkt, &data, &len, APR_BLOCK_READ);
        match = apr_strmatch(ctx->pattern, data, len);
        if (match) {
            apr_bucket *next_bucket;
            apr_bucket_split(bkt, match - data);
            next_bucket = APR_BUCKET_NEXT(bkt);
            apr_bucket_split(next_bucket, ctx->pattern_len);
            bkt = APR_BUCKET_NEXT(next_bucket);
            apr_bucket_delete(next_bucket);
            next_bucket = apr_bucket_pool_create(ctx->remotepath,
                                                 ctx->remotepath_len,
                                                 r->pool, bb->bucket_alloc);
            APR_BUCKET_INSERT_BEFORE(bkt, next_bucket);
        }
        else {
            bkt = APR_BUCKET_NEXT(bkt);
        }
    }
    return APR_SUCCESS;
}

apr_status_t dav_svn__location_header_filter(ap_filter_t *f,
                                             apr_bucket_brigade *bb)
{
    request_rec *r = f->r;
    const char *master_uri;
    const char *location, *start_foo = NULL;

    /* Don't filter if we're in a subrequest or we aren't setup to
       proxy anything. */
    master_uri = dav_svn__get_master_uri(r);
    master_uri = svn_path_uri_encode(master_uri, r->pool);
    if (r->main || !master_uri) {
        ap_remove_output_filter(f);
        return ap_pass_brigade(f->next, bb);
    }

    location = apr_table_get(r->headers_out, "Location");
    if (location) {
        start_foo = ap_strstr_c(location, master_uri);
    }
    if (start_foo) {
        const char *new_uri;
        start_foo += strlen(master_uri);
        new_uri = ap_construct_url(r->pool,
                                   apr_pstrcat(r->pool,
                                               dav_svn__get_root_dir(r), "/",
                                               start_foo, (char *)NULL),
                                   r);
        apr_table_set(r->headers_out, "Location", new_uri);
    }
    return ap_pass_brigade(f->next, bb);
}

apr_status_t dav_svn__location_body_filter(ap_filter_t *f,
                                           apr_bucket_brigade *bb)
{
    request_rec *r = f->r;
    locate_ctx_t *ctx = f->ctx;
    apr_bucket *bkt;
    const char *master_uri, *root_dir, *canonicalized_uri;
    apr_uri_t uri;

    /* Don't filter if we're in a subrequest or we aren't setup to
       proxy anything. */
    master_uri = dav_svn__get_master_uri(r);
    if (r->main || !master_uri) {
        ap_remove_output_filter(f);
        return ap_pass_brigade(f->next, bb);
    }

    /* And don't filter if our search-n-replace would be a noop anyway
       (that is, if our root path matches that of the master server). */
    apr_uri_parse(r->pool, master_uri, &uri);
    root_dir = dav_svn__get_root_dir(r);
    canonicalized_uri = svn_urlpath__canonicalize(uri.path, r->pool);
    if (strcmp(canonicalized_uri, root_dir) == 0) {
        ap_remove_output_filter(f);
        return ap_pass_brigade(f->next, bb);
    }

    /* ### FIXME: GET and PROPFIND requests that make it here must be
       ### referring to data inside commit transactions-in-progress.
       ### We've got to be careful not to munge the versioned data
       ### they return in the process of trying to do URI fix-ups.
       ### See issue #3445 for details. */

    /* We are url encoding the current url and the master url
       as incoming(from master) request body has it encoded already. */
    canonicalized_uri = svn_path_uri_encode(canonicalized_uri, r->pool);
    root_dir = svn_path_uri_encode(root_dir, r->pool);
    if (!f->ctx) {
        ctx = f->ctx = apr_pcalloc(r->pool, sizeof(*ctx));
        ctx->remotepath = canonicalized_uri;
        ctx->remotepath_len = strlen(ctx->remotepath);
        ctx->localpath = root_dir;
        ctx->localpath_len = strlen(ctx->localpath);
        ctx->pattern = apr_strmatch_precompile(r->pool, ctx->remotepath, 1);
        ctx->pattern_len = ctx->remotepath_len;
    }

    bkt = APR_BRIGADE_FIRST(bb);
    while (bkt != APR_BRIGADE_SENTINEL(bb)) {

        const char *data, *match;
        apr_size_t len;

        /* read */
        apr_bucket_read(bkt, &data, &len, APR_BLOCK_READ);
        match = apr_strmatch(ctx->pattern, data, len);
        if (match) {
            apr_bucket *next_bucket;
            apr_bucket_split(bkt, match - data);
            next_bucket = APR_BUCKET_NEXT(bkt);
            apr_bucket_split(next_bucket, ctx->pattern_len);
            bkt = APR_BUCKET_NEXT(next_bucket);
            apr_bucket_delete(next_bucket);
            next_bucket = apr_bucket_pool_create(ctx->localpath,
                                                 ctx->localpath_len,
                                                 r->pool, bb->bucket_alloc);
            APR_BUCKET_INSERT_BEFORE(bkt, next_bucket);
        }
        else {
            bkt = APR_BUCKET_NEXT(bkt);
        }
    }
    return ap_pass_brigade(f->next, bb);
}

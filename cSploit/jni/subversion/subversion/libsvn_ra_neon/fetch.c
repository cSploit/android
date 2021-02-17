/*
 * fetch.c :  routines for fetching updates and checkouts
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



#include <stdlib.h> /* for free() */

#define APR_WANT_STRFUNC
#include <apr_want.h> /* for strcmp() */

#include <apr_pools.h>
#include <apr_tables.h>
#include <apr_strings.h>
#include <apr_xml.h>

#include <ne_basic.h>

#include "svn_error.h"
#include "svn_pools.h"
#include "svn_delta.h"
#include "svn_io.h"
#include "svn_base64.h"
#include "svn_ra.h"
#include "../libsvn_ra/ra_loader.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_xml.h"
#include "svn_dav.h"
#include "svn_time.h"
#include "svn_props.h"

#include "private/svn_dav_protocol.h"
#include "svn_private_config.h"

#include "ra_neon.h"


typedef struct file_read_ctx_t {
  apr_pool_t *pool;

  /* these two are the handler that the editor gave us */
  svn_txdelta_window_handler_t handler;
  void *handler_baton;

  /* if we're receiving an svndiff, this is a parser which places the
     resulting windows into the above handler/baton. */
  svn_stream_t *stream;

} file_read_ctx_t;

typedef struct file_write_ctx_t {
  svn_boolean_t do_checksum;  /* only accumulate checksum if set */
  svn_checksum_ctx_t *checksum_ctx; /* accumulating checksum of file contents */
  svn_stream_t *stream;       /* stream to write file contents to */
} file_write_ctx_t;

typedef struct custom_get_ctx_t {
  svn_ra_neon__request_t *req;  /* Used to propagate errors out of the reader */
  int checked_type;             /* have we processed ctype yet? */

  void *subctx;
} custom_get_ctx_t;

typedef svn_error_t * (*prop_setter_t)(void *baton,
                                       const char *name,
                                       const svn_string_t *value,
                                       apr_pool_t *pool);

typedef struct dir_item_t {
  /* The baton returned by the editor's open_root/open_dir */
  void *baton;

  /* Should we fetch properties for this directory when the close tag
     is found? */
  svn_boolean_t fetch_props;

  /* The version resource URL for this directory. */
  const char *vsn_url;

  /* A buffer which stores the relative directory name. We also use this
     for temporary construction of relative file names. */
  svn_stringbuf_t *pathbuf;

  /* If a directory, this may contain a hash of prophashes returned
     from doing a depth 1 PROPFIND. */
  apr_hash_t *children;

  /* A subpool.  It's about memory.  Ya dig? */
  apr_pool_t *pool;

} dir_item_t;

typedef struct report_baton_t {
  svn_ra_neon__session_t *ras;

  apr_file_t *tmpfile;

  /* The pool of the report baton; used for things that must live during the
     whole editing operation. */
  apr_pool_t *pool;
  /* Pool initialized when the report_baton is created, and meant for
     quick scratchwork.  This is like a loop pool, but since the loop
     that drives ra_neon callbacks is in the wrong scope for us to use
     the normal loop pool idiom, we must resort to this.  Always clear
     this pool right after using it; only YOU can prevent forest fires. */
  apr_pool_t *scratch_pool;

  svn_boolean_t fetch_content;
  svn_boolean_t fetch_props;

  const svn_delta_editor_t *editor;
  void *edit_baton;

  /* Stack of directory batons/vsn_urls. */
  apr_array_header_t *dirs;

#define TOP_DIR(rb)      (APR_ARRAY_IDX((rb)->dirs, (rb)->dirs->nelts - 1, \
                          dir_item_t))
#define DIR_DEPTH(rb)    ((rb)->dirs->nelts)
#define PUSH_BATON(rb,b) (APR_ARRAY_PUSH((rb)->dirs, void *) = (b))

  /* These items are only valid inside add- and open-file tags! */
  void *file_baton;
  apr_pool_t *file_pool;
  const char *result_checksum; /* hex md5 digest of result; may be null */

  svn_stringbuf_t *namestr;
  svn_stringbuf_t *cpathstr;
  svn_stringbuf_t *href;

  /* Empty string means no encoding, "base64" means base64. */
  svn_stringbuf_t *encoding;

  /* These are used when receiving an inline txdelta, and null at all
     other times. */
  svn_txdelta_window_handler_t whandler;
  void *whandler_baton;
  svn_stream_t *svndiff_decoder;
  svn_stream_t *base64_decoder;

  /* A generic accumulator for elements that have small bits of cdata,
     like md5_checksum, href, etc.  Uh, or where our own API gives us
     no choice about holding them in memory, as with prop values, ahem.
     This is always the empty stringbuf when not in use. */
  svn_stringbuf_t *cdata_accum;

  /* Are we inside a resource element? */
  svn_boolean_t in_resource;
  /* Valid if in_resource is true. */
  svn_stringbuf_t *current_wcprop_path;
  svn_boolean_t is_switch;

  /* Named target, or NULL if none.  For example, in 'svn up wc/foo',
     this is "wc/foo", but in 'svn up' it is "".

     The target helps us determine whether a response received from
     the server should be acted on.  Take 'svn up wc/foo': the server
     may send back a new vsn-rsrc-url wcprop for 'wc' (because the
     report had to be anchored there just in case the update deletes
     wc/foo).  While this is correct behavior for the server, the
     client should ignore the new wcprop, because the client knows
     it's not really updating the top level directory. */
  const char *target;

  /* Whether the server should try to send copyfrom arguments. */
  svn_boolean_t send_copyfrom_args;

  /* Use an intermediate tmpfile for the REPORT response. */
  svn_boolean_t spool_response;

  /* If this report is for a switch, update, or status (but not a
     merge/diff), then we made the update report request with the "send-all"
     attribute.  The server reponds to this by putting a "send-all" attribute
     in its response.  If we see that attribute, we set this to true,
     otherwise, it stays false. */
  svn_boolean_t receiving_all;

  /* Hash mapping 'const char *' paths -> 'const char *' lock tokens. */
  apr_hash_t *lock_tokens;

} report_baton_t;

static const svn_ra_neon__xml_elm_t report_elements[] =
{
  { SVN_XML_NAMESPACE, "update-report", ELEM_update_report, 0 },
  { SVN_XML_NAMESPACE, "resource-walk", ELEM_resource_walk, 0 },
  { SVN_XML_NAMESPACE, "resource", ELEM_resource, 0 },
  { SVN_XML_NAMESPACE, "target-revision", ELEM_target_revision, 0 },
  { SVN_XML_NAMESPACE, "open-directory", ELEM_open_directory, 0 },
  { SVN_XML_NAMESPACE, "add-directory", ELEM_add_directory, 0 },
  { SVN_XML_NAMESPACE, "absent-directory", ELEM_absent_directory, 0 },
  { SVN_XML_NAMESPACE, "open-file", ELEM_open_file, 0 },
  { SVN_XML_NAMESPACE, "add-file", ELEM_add_file, 0 },
  { SVN_XML_NAMESPACE, "txdelta", ELEM_txdelta, 0 },
  { SVN_XML_NAMESPACE, "absent-file", ELEM_absent_file, 0 },
  { SVN_XML_NAMESPACE, "delete-entry", ELEM_delete_entry, 0 },
  { SVN_XML_NAMESPACE, "fetch-props", ELEM_fetch_props, 0 },
  { SVN_XML_NAMESPACE, "set-prop", ELEM_set_prop, 0 },
  { SVN_XML_NAMESPACE, "remove-prop", ELEM_remove_prop, 0 },
  { SVN_XML_NAMESPACE, "fetch-file", ELEM_fetch_file, 0 },
  { SVN_XML_NAMESPACE, "prop", ELEM_SVN_prop, 0 },
  { SVN_DAV_PROP_NS_DAV, "repository-uuid",
    ELEM_repository_uuid, SVN_RA_NEON__XML_CDATA },

  { SVN_DAV_PROP_NS_DAV, "md5-checksum", ELEM_md5_checksum,
    SVN_RA_NEON__XML_CDATA },

  { "DAV:", "version-name", ELEM_version_name, SVN_RA_NEON__XML_CDATA },
  { "DAV:", SVN_DAV__CREATIONDATE, ELEM_creationdate, SVN_RA_NEON__XML_CDATA },
  { "DAV:", "creator-displayname", ELEM_creator_displayname,
     SVN_RA_NEON__XML_CDATA },

  { "DAV:", "checked-in", ELEM_checked_in, 0 },
  { "DAV:", "href", ELEM_href, SVN_RA_NEON__XML_CDATA },

  { NULL }
};

static svn_error_t *simple_store_vsn_url(const char *vsn_url,
                                         void *baton,
                                         prop_setter_t setter,
                                         apr_pool_t *pool)
{
  /* store the version URL as a property */
  SVN_ERR_W((*setter)(baton, SVN_RA_NEON__LP_VSN_URL,
                      svn_string_create(vsn_url, pool), pool),
            _("Could not save the URL of the version resource"));

  return NULL;
}

/* helper func which maps certain DAV: properties to svn:wc:
   properties.  Used during checkouts and updates.  */
static svn_error_t *set_special_wc_prop(const char *key,
                                        const svn_string_t *val,
                                        prop_setter_t setter,
                                        void *baton,
                                        apr_pool_t *pool)
{
  const char *name = NULL;

  if (strcmp(key, SVN_RA_NEON__PROP_VERSION_NAME) == 0)
    name = SVN_PROP_ENTRY_COMMITTED_REV;
  else if (strcmp(key, SVN_RA_NEON__PROP_CREATIONDATE) == 0)
    name = SVN_PROP_ENTRY_COMMITTED_DATE;
  else if (strcmp(key, SVN_RA_NEON__PROP_CREATOR_DISPLAYNAME) == 0)
    name = SVN_PROP_ENTRY_LAST_AUTHOR;
  else if (strcmp(key, SVN_RA_NEON__PROP_REPOSITORY_UUID) == 0)
    name = SVN_PROP_ENTRY_UUID;

  /* If we got a name we care about it, call the setter function. */
  if (name)
    SVN_ERR((*setter)(baton, name, val, pool));

  return SVN_NO_ERROR;
}


static svn_error_t *add_props(apr_hash_t *props,
                              prop_setter_t setter,
                              void *baton,
                              apr_pool_t *pool)
{
  apr_hash_index_t *hi;

  for (hi = apr_hash_first(pool, props); hi; hi = apr_hash_next(hi))
    {
      const void *vkey;
      void *vval;
      const char *key;
      const svn_string_t *val;

      apr_hash_this(hi, &vkey, NULL, &vval);
      key = vkey;
      val = vval;

#define NSLEN (sizeof(SVN_DAV_PROP_NS_CUSTOM) - 1)
      if (strncmp(key, SVN_DAV_PROP_NS_CUSTOM, NSLEN) == 0)
        {
          /* for props in the 'custom' namespace, we strip the
             namespace and just use whatever name the user gave the
             property. */
          SVN_ERR((*setter)(baton, key + NSLEN, val, pool));
          continue;
        }
#undef NSLEN

#define NSLEN (sizeof(SVN_DAV_PROP_NS_SVN) - 1)
      if (strncmp(key, SVN_DAV_PROP_NS_SVN, NSLEN) == 0)
        {
          /* This property is an 'svn:' prop, recognized by client, or
             server, or both.  Convert the URI namespace into normal
             'svn:' prefix again before pushing it at the wc. */
          SVN_ERR((*setter)(baton, apr_pstrcat(pool, SVN_PROP_PREFIX,
                                               key + NSLEN, (char *)NULL),
                            val, pool));
        }
#undef NSLEN

      else
        {
          /* If we get here, then we have a property that is neither
             in the 'custom' space, nor in the 'svn' space.  So it
             must be either in the 'network' space or 'DAV:' space.
             The following routine converts a handful of DAV: props
             into 'svn:wc:' or 'svn:entry:' props that libsvn_wc
             wants. */
          SVN_ERR(set_special_wc_prop(key, val, setter, baton, pool));
        }
    }
  return SVN_NO_ERROR;
}


static svn_error_t *custom_get_request(svn_ra_neon__session_t *ras,
                                       const char *url,
                                       const char *editor_relpath,
                                       svn_ra_neon__block_reader reader,
                                       void *subctx,
                                       svn_ra_get_wc_prop_func_t get_wc_prop,
                                       void *cb_baton,
                                       svn_boolean_t use_base,
                                       apr_pool_t *pool)
{
  custom_get_ctx_t cgc = { 0 };
  const char *delta_base = NULL;
  svn_ra_neon__request_t *request;
  svn_error_t *err;

  if (use_base && editor_relpath != NULL)
    {
      /* See if we can get a version URL for this resource. This will
         refer to what we already have in the working copy, thus we
         can get a diff against this particular resource. */

      if (get_wc_prop != NULL)
        {
          const svn_string_t *value;

          SVN_ERR(get_wc_prop(cb_baton, editor_relpath,
                              SVN_RA_NEON__LP_VSN_URL,
                              &value, pool));

          delta_base = value ? value->data : NULL;
        }
    }

  SVN_ERR(svn_ra_neon__request_create(&request, ras, "GET", url, pool));

  if (delta_base)
    {
      /* The HTTP delta draft uses an If-None-Match header holding an
         entity tag corresponding to the copy we have. It is much more
         natural for us to use a version URL to specify what we have.
         Thus, we want to use the If: header to specify the URL. But
         mod_dav sees all "State-token" items as lock tokens. When we
         get mod_dav updated and the backend APIs expanded, then we
         can switch to using the If: header. For now, use a custom
         header to specify the version resource to use as the base. */
      ne_add_request_header(request->ne_req,
                            SVN_DAV_DELTA_BASE_HEADER, delta_base);
    }

  svn_ra_neon__add_response_body_reader(request, ne_accept_2xx, reader, &cgc);

  /* complete initialization of the body reading context */
  cgc.req = request;
  cgc.subctx = subctx;

  /* run the request */
  err = svn_ra_neon__request_dispatch(NULL, request, NULL, NULL,
                                      200 /* OK */,
                                      226 /* IM Used */,
                                      pool);
  svn_ra_neon__request_destroy(request);

  /* The request runner raises internal errors before Neon errors,
     pass a returned error to our callers */

  return err;
}

/* This implements the svn_ra_neon__block_reader() callback interface. */
static svn_error_t *
fetch_file_reader(void *userdata, const char *buf, size_t len)
{
  custom_get_ctx_t *cgc = userdata;
  file_read_ctx_t *frc = cgc->subctx;

  if (len == 0)
    {
      /* file is complete. */
      return 0;
    }

  if (!cgc->checked_type)
    {
      ne_content_type ctype = { 0 };
      int rv = ne_get_content_type(cgc->req->ne_req, &ctype);

      if (rv != 0)
        return
          svn_error_create(SVN_ERR_RA_DAV_RESPONSE_HEADER_BADNESS, NULL,
                           _("Could not get content-type from response"));

      /* Neon guarantees non-NULL values when rv==0 */
      if (!strcmp(ctype.type, "application")
          && !strcmp(ctype.subtype, "vnd.svn-svndiff"))
        {
          /* we are receiving an svndiff. set things up. */
          frc->stream = svn_txdelta_parse_svndiff(frc->handler,
                                                  frc->handler_baton,
                                                  TRUE,
                                                  frc->pool);
        }

      if (ctype.value)
        free(ctype.value);

      cgc->checked_type = 1;
    }

  if (frc->stream == NULL)
    {
      /* receiving plain text. construct a window for it. */

      svn_txdelta_window_t window = { 0 };
      svn_txdelta_op_t op;
      svn_string_t data;

      data.data = buf;
      data.len = len;

      op.action_code = svn_txdelta_new;
      op.offset = 0;
      op.length = len;

      window.tview_len = len;       /* result will be this long */
      window.num_ops = 1;
      window.ops = &op;
      window.new_data = &data;

      /* We can't really do anything useful if we get an error here.  Pass
         it off to someone who can. */
      SVN_RA_NEON__REQ_ERR
        (cgc->req,
         (*frc->handler)(&window, frc->handler_baton));
    }
  else
    {
      /* receiving svndiff. feed it to the svndiff parser. */

      apr_size_t written = len;

      SVN_ERR(svn_stream_write(frc->stream, buf, &written));

      /* ### the svndiff stream parser does not obey svn_stream semantics
         ### in its write handler. it does not output the number of bytes
         ### consumed by the handler. specifically, it may decrement the
         ### number by 4 for the header, then never touch it again. that
         ### makes it appear like an incomplete write.
         ### disable this check for now. the svndiff parser actually does
         ### consume all bytes, all the time.
      */
#if 0
      if (written != len && cgc->err == NULL)
        cgc->err = svn_error_createf(SVN_ERR_INCOMPLETE_DATA, NULL,
                                     "Unable to completely write the svndiff "
                                     "data to the parser stream "
                                     "(wrote " APR_SIZE_T_FMT " "
                                     "of " APR_SIZE_T_FMT " bytes)",
                                     written, len);
#endif
    }

  return 0;
}

static svn_error_t *simple_fetch_file(svn_ra_neon__session_t *ras,
                                      const char *url,
                                      const char *relpath,
                                      svn_boolean_t text_deltas,
                                      void *file_baton,
                                      const char *base_checksum,
                                      const svn_delta_editor_t *editor,
                                      svn_ra_get_wc_prop_func_t get_wc_prop,
                                      void *cb_baton,
                                      apr_pool_t *pool)
{
  file_read_ctx_t frc = { 0 };

  SVN_ERR_W((*editor->apply_textdelta)(file_baton,
                                       base_checksum,
                                       pool,
                                       &frc.handler,
                                       &frc.handler_baton),
            _("Could not save file"));

  /* Only bother with text-deltas if our caller cares. */
  if (! text_deltas)
    return (*frc.handler)(NULL, frc.handler_baton);

  frc.pool = pool;

  SVN_ERR(custom_get_request(ras, url, relpath,
                             fetch_file_reader, &frc,
                             get_wc_prop, cb_baton,
                             TRUE, pool));

  /* close the handler, since the file reading completed successfully. */
  if (frc.stream)
    return svn_stream_close(frc.stream);
  else
    return (*frc.handler)(NULL, frc.handler_baton);
}

/* Helper for svn_ra_neon__get_file.  This implements
   the svn_ra_neon__block_reader() callback interface. */
static svn_error_t *
get_file_reader(void *userdata, const char *buf, size_t len)
{
  custom_get_ctx_t *cgc = userdata;

  /* The stream we want to push data at. */
  file_write_ctx_t *fwc = cgc->subctx;
  svn_stream_t *stream = fwc->stream;

  if (fwc->do_checksum)
    SVN_ERR(svn_checksum_update(fwc->checksum_ctx, buf, len));

  /* Write however many bytes were passed in by neon. */
  return svn_stream_write(stream, buf, &len);
}


/* minor helper for svn_ra_neon__get_file, of type prop_setter_t */
static svn_error_t *
add_prop_to_hash(void *baton,
                 const char *name,
                 const svn_string_t *value,
                 apr_pool_t *pool)
{
  apr_hash_t *ht = (apr_hash_t *) baton;
  apr_hash_set(ht, name, APR_HASH_KEY_STRING, value);
  return SVN_NO_ERROR;
}


/* Helper for svn_ra_neon__get_file(), svn_ra_neon__get_dir(), and
   svn_ra_neon__rev_proplist().

   Loop over the properties in RSRC->propset, examining namespaces and
   such to filter Subversion, custom, etc. properties.

   User-visible props get added to the PROPS hash (alloced in POOL).

   If ADD_ENTRY_PROPS is true, then "special" working copy entry-props
   are added to the hash by set_special_wc_prop().
*/
static svn_error_t *
filter_props(apr_hash_t *props,
             svn_ra_neon__resource_t *rsrc,
             svn_boolean_t add_entry_props,
             apr_pool_t *pool)
{
  apr_hash_index_t *hi;

  for (hi = apr_hash_first(pool, rsrc->propset); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      const char *name;
      void *val;
      const svn_string_t *value;

      apr_hash_this(hi, &key, NULL, &val);
      name = key;
      value = svn_string_dup(val, pool);

      /* If the property is in the 'custom' namespace, then it's a
         normal user-controlled property coming from the fs.  Just
         strip off this prefix and add to the hash. */
#define NSLEN (sizeof(SVN_DAV_PROP_NS_CUSTOM) - 1)
      if (strncmp(name, SVN_DAV_PROP_NS_CUSTOM, NSLEN) == 0)
        {
          apr_hash_set(props, name + NSLEN, APR_HASH_KEY_STRING, value);
          continue;
        }
#undef NSLEN

      /* If the property is in the 'svn' namespace, then it's a
         normal user-controlled property coming from the fs.  Just
         strip off the URI prefix, add an 'svn:', and add to the hash. */
#define NSLEN (sizeof(SVN_DAV_PROP_NS_SVN) - 1)
      if (strncmp(name, SVN_DAV_PROP_NS_SVN, NSLEN) == 0)
        {
          apr_hash_set(props,
                       apr_pstrcat(pool, SVN_PROP_PREFIX, name + NSLEN,
                                   (char *)NULL),
                       APR_HASH_KEY_STRING,
                       value);
          continue;
        }
#undef NSLEN
      else if (strcmp(name, SVN_RA_NEON__PROP_CHECKED_IN) == 0)
        {
          /* For files, we currently only have one 'wc' prop. */
          apr_hash_set(props, SVN_RA_NEON__LP_VSN_URL,
                       APR_HASH_KEY_STRING, value);
        }
      else
        {
          /* If it's one of the 'entry' props, this func will
             recognize the DAV: name & add it to the hash mapped to a
             new name recognized by libsvn_wc. */
          if (add_entry_props)
            SVN_ERR(set_special_wc_prop(name, value, add_prop_to_hash,
                                        props, pool));
        }
    }

  return SVN_NO_ERROR;
}

static const ne_propname restype_props[] =
{
  { "DAV:", "resourcetype" },
  { NULL }
};

static const ne_propname restype_checksum_props[] =
{
  { "DAV:", "resourcetype" },
  { SVN_DAV_PROP_NS_DAV, "md5-checksum" },
  { NULL }
};

svn_error_t *svn_ra_neon__get_file(svn_ra_session_t *session,
                                   const char *path,
                                   svn_revnum_t revision,
                                   svn_stream_t *stream,
                                   svn_revnum_t *fetched_rev,
                                   apr_hash_t **props,
                                   apr_pool_t *pool)
{
  svn_ra_neon__resource_t *rsrc;
  const char *final_url;
  svn_ra_neon__session_t *ras = session->priv;
  const char *url = svn_path_url_add_component2(ras->url->data, path, pool);
  const ne_propname *which_props;

  /* If the revision is invalid (head), then we're done.  Just fetch
     the public URL, because that will always get HEAD. */
  if ((! SVN_IS_VALID_REVNUM(revision)) && (fetched_rev == NULL))
    final_url = url;

  /* If the revision is something specific, we need to create a bc_url. */
  else
    {
      svn_revnum_t got_rev;
      const char *bc_url;
      const char *bc_relative;

      SVN_ERR(svn_ra_neon__get_baseline_info(&bc_url, &bc_relative, &got_rev,
                                             ras, url, revision, pool));
      final_url = svn_path_url_add_component2(bc_url, bc_relative, pool);
      if (fetched_rev != NULL)
        *fetched_rev = got_rev;
    }

  if (props)
    {
      /* Request all properties if caller requested them. */
      which_props = NULL;
    }
  else if (stream)
    {
      /* Request md5 checksum and resource type properties if caller
         requested file contents. */
      which_props = restype_checksum_props;
    }
  else
    {
      /* Request only resource type on other cases. */
      which_props = restype_props;
    }

  SVN_ERR(svn_ra_neon__get_props_resource(&rsrc, ras, final_url, NULL,
                                          which_props, pool));
  if (rsrc->is_collection)
    {
      return svn_error_create(SVN_ERR_FS_NOT_FILE, NULL,
                              _("Can't get text contents of a directory"));
    }

  if (props)
    {
      *props = apr_hash_make(pool);
      SVN_ERR(filter_props(*props, rsrc, TRUE, pool));
    }

  if (stream)
    {
      const svn_string_t *expected_checksum;
      file_write_ctx_t fwc;

      expected_checksum = apr_hash_get(rsrc->propset,
                                       SVN_RA_NEON__PROP_MD5_CHECKSUM,
                                       APR_HASH_KEY_STRING);

      /* Older servers don't serve checksum prop, but that's okay. */
      /* ### temporary hack for 0.17. if the server doesn't have the prop,
         ### then __get_one_prop returns an empty string. deal with it.  */
      if (!expected_checksum
          || (expected_checksum && expected_checksum->data[0] == '\0'))
        {
          fwc.do_checksum = FALSE;
        }
      else
        {
          fwc.do_checksum = TRUE;
        }

      fwc.stream = stream;

      if (fwc.do_checksum)
        fwc.checksum_ctx = svn_checksum_ctx_create(svn_checksum_md5, pool);

      /* Fetch the file, shoving it at the provided stream. */
      SVN_ERR(custom_get_request(ras, final_url, path,
                                 get_file_reader, &fwc,
                                 ras->callbacks->get_wc_prop,
                                 ras->callback_baton,
                                 FALSE, pool));

      if (fwc.do_checksum)
        {
          const char *hex_digest;
          svn_checksum_t *checksum;

          SVN_ERR(svn_checksum_final(&checksum, fwc.checksum_ctx, pool));
          hex_digest = svn_checksum_to_cstring_display(checksum, pool);

          if (strcmp(hex_digest, expected_checksum->data) != 0)
            return svn_error_createf
              (SVN_ERR_CHECKSUM_MISMATCH, NULL,
               apr_psprintf(pool, "%s:\n%s\n%s\n",
                            _("Checksum mismatch for '%s'"),
                            _("   expected:  %s"),
                            _("     actual:  %s")),
               path, expected_checksum->data, hex_digest);
        }
    }

  return SVN_NO_ERROR;
}

svn_error_t *svn_ra_neon__get_dir(svn_ra_session_t *session,
                                  apr_hash_t **dirents,
                                  svn_revnum_t *fetched_rev,
                                  apr_hash_t **props,
                                  const char *path,
                                  svn_revnum_t revision,
                                  apr_uint32_t dirent_fields,
                                  apr_pool_t *pool)
{
  svn_ra_neon__resource_t *rsrc;
  apr_hash_index_t *hi;
  apr_hash_t *resources;
  const char *final_url;
  apr_size_t final_url_n_components;
  svn_ra_neon__session_t *ras = session->priv;
  const char *url = svn_path_url_add_component2(ras->url->data, path, pool);

  /* If the revision is invalid (HEAD), then we're done -- just fetch
     the public URL, because that will always get HEAD.  Otherwise, we
     need to create a bc_url. */
  if ((! SVN_IS_VALID_REVNUM(revision)) && (fetched_rev == NULL))
    {
      SVN_ERR(svn_ra_neon__get_url_path(&final_url, url, pool));
    }
  else
    {
      svn_revnum_t got_rev;
      const char *bc_url;
      const char *bc_relative;

      SVN_ERR(svn_ra_neon__get_baseline_info(&bc_url, &bc_relative, &got_rev,
                                             ras, url, revision, pool));
      final_url = svn_path_url_add_component2(bc_url, bc_relative, pool);
      if (fetched_rev != NULL)
        *fetched_rev = got_rev;
    }

  if (dirents)
    {
      ne_propname *which_props;
      svn_boolean_t supports_deadprop_count;

      /* For issue 2151: See if we are dealing with a server that
         understands the deadprop-count property.  If it doesn't, we'll
         need to do an allprop PROPFIND.  If it does, we'll execute a more
         targeted PROPFIND. */
      if (dirent_fields & SVN_DIRENT_HAS_PROPS)
        {
          SVN_ERR(svn_ra_neon__get_deadprop_count_support(
                    &supports_deadprop_count, ras, final_url, pool));
        }

      /* if we didn't ask for the has_props field, we can get individual
         properties. */
      if ((SVN_DIRENT_HAS_PROPS & dirent_fields) == 0
          || supports_deadprop_count)
        {
          int num_props = 1; /* start with one for the final NULL */

          if (dirent_fields & SVN_DIRENT_KIND)
            ++num_props;

          if (dirent_fields & SVN_DIRENT_SIZE)
            ++num_props;

          if (dirent_fields & SVN_DIRENT_HAS_PROPS)
            ++num_props;

          if (dirent_fields & SVN_DIRENT_CREATED_REV)
            ++num_props;

          if (dirent_fields & SVN_DIRENT_TIME)
            ++num_props;

          if (dirent_fields & SVN_DIRENT_LAST_AUTHOR)
            ++num_props;

          which_props = apr_pcalloc(pool, num_props * sizeof(ne_propname));

          --num_props; /* damn zero based arrays... */

          /* first, null out the end... */
          which_props[num_props].nspace = NULL;
          which_props[num_props--].name = NULL;

          /* Now, go through and fill in the ones we care about, moving along
             the array as we go. */

          if (dirent_fields & SVN_DIRENT_KIND)
            {
              which_props[num_props].nspace = "DAV:";
              which_props[num_props--].name = "resourcetype";
            }

          if (dirent_fields & SVN_DIRENT_SIZE)
            {
              which_props[num_props].nspace = "DAV:";
              which_props[num_props--].name = "getcontentlength";
            }

          if (dirent_fields & SVN_DIRENT_HAS_PROPS)
            {
              which_props[num_props].nspace = SVN_DAV_PROP_NS_DAV;
              which_props[num_props--].name = "deadprop-count";
            }

          if (dirent_fields & SVN_DIRENT_CREATED_REV)
            {
              which_props[num_props].nspace = "DAV:";
              which_props[num_props--].name = "version-name";
            }

          if (dirent_fields & SVN_DIRENT_TIME)
            {
              which_props[num_props].nspace = "DAV:";
              which_props[num_props--].name = SVN_DAV__CREATIONDATE;
            }

          if (dirent_fields & SVN_DIRENT_LAST_AUTHOR)
            {
              which_props[num_props].nspace = "DAV:";
              which_props[num_props--].name = "creator-displayname";
            }

          SVN_ERR_ASSERT(num_props == -1);
        }
      else
        {
          /* get all props, since we need them all to do has_props */
          which_props = NULL;
        }

      /* Just like Nautilus, Cadaver, or any other browser, we do a
         PROPFIND on the directory of depth 1. */
      SVN_ERR(svn_ra_neon__get_props(&resources, ras,
                                     final_url, SVN_RA_NEON__DEPTH_ONE,
                                     NULL, which_props, pool));

      /* Count the number of path components in final_url. */
      final_url_n_components = svn_path_component_count(final_url);

      /* Now we have a hash that maps a bunch of url children to resource
         objects.  Each resource object contains the properties of the
         child.   Parse these resources into svn_dirent_t structs. */
      *dirents = apr_hash_make(pool);
      for (hi = apr_hash_first(pool, resources);
           hi;
           hi = apr_hash_next(hi))
        {
          const void *key;
          void *val;
          const char *childname;
          svn_ra_neon__resource_t *resource;
          const svn_string_t *propval;
          apr_hash_index_t *h;
          svn_dirent_t *entry;

          apr_hash_this(hi, &key, NULL, &val);
          childname = svn_relpath_canonicalize(key, pool);
          resource = val;

          /* Skip the effective '.' entry that comes back from
             SVN_RA_NEON__DEPTH_ONE. The children must have one more
             component then final_url.
             Note that we can't just strcmp the URLs because of URL encoding
             differences (i.e. %3c vs. %3C etc.) */
          if (svn_path_component_count(childname) == final_url_n_components)
            continue;

          entry = apr_pcalloc(pool, sizeof(*entry));

          if (dirent_fields & SVN_DIRENT_KIND)
            {
              /* node kind */
              entry->kind = resource->is_collection ? svn_node_dir
                                                    : svn_node_file;
            }

          if (dirent_fields & SVN_DIRENT_SIZE)
            {
              /* size */
              propval = apr_hash_get(resource->propset,
                                     SVN_RA_NEON__PROP_GETCONTENTLENGTH,
                                     APR_HASH_KEY_STRING);
              if (propval == NULL)
                entry->size = 0;
              else
                entry->size = svn__atoui64(propval->data);
            }

          if (dirent_fields & SVN_DIRENT_HAS_PROPS)
            {
              /* Does this resource contain any 'svn' or 'custom'
                 properties (e.g. ones actually created and set by the
                 user)? */
              if (supports_deadprop_count)
                {
                  propval = apr_hash_get(resource->propset,
                                         SVN_RA_NEON__PROP_DEADPROP_COUNT,
                                         APR_HASH_KEY_STRING);

                  if (propval == NULL)
                    {
                      /* we thought that the server supported the
                         deadprop-count property.  apparently not. */
                      return svn_error_create(SVN_ERR_INCOMPLETE_DATA, NULL,
                                              _("Server response missing the "
                                                "expected deadprop-count "
                                                "property"));
                    }
                  else
                    {
                      apr_int64_t prop_count = svn__atoui64(propval->data);
                      entry->has_props = (prop_count > 0);
                    }
                }
              else
                {
                   /* The server doesn't support the deadprop_count prop,
                      fallback */
                  for (h = apr_hash_first(pool, resource->propset);
                       h; h = apr_hash_next(h))
                    {
                      const void *kkey;
                      apr_hash_this(h, &kkey, NULL, NULL);

                      if (strncmp((const char *) kkey, SVN_DAV_PROP_NS_CUSTOM,
                                  sizeof(SVN_DAV_PROP_NS_CUSTOM) - 1) == 0
                          || strncmp((const char *) kkey, SVN_DAV_PROP_NS_SVN,
                                     sizeof(SVN_DAV_PROP_NS_SVN) - 1) == 0)
                        entry->has_props = TRUE;
                    }
                }
            }

          if (dirent_fields & SVN_DIRENT_CREATED_REV)
            {
              /* created_rev & friends */
              propval = apr_hash_get(resource->propset,
                                     SVN_RA_NEON__PROP_VERSION_NAME,
                                     APR_HASH_KEY_STRING);
              if (propval != NULL)
                entry->created_rev = SVN_STR_TO_REV(propval->data);
            }

          if (dirent_fields & SVN_DIRENT_TIME)
            {
              propval = apr_hash_get(resource->propset,
                                     SVN_RA_NEON__PROP_CREATIONDATE,
                                     APR_HASH_KEY_STRING);
              if (propval != NULL)
                SVN_ERR(svn_time_from_cstring(&(entry->time),
                                              propval->data, pool));
            }

          if (dirent_fields & SVN_DIRENT_LAST_AUTHOR)
            {
              propval = apr_hash_get(resource->propset,
                                     SVN_RA_NEON__PROP_CREATOR_DISPLAYNAME,
                                     APR_HASH_KEY_STRING);
              if (propval != NULL)
                entry->last_author = propval->data;
            }

          apr_hash_set(*dirents,
                       svn_path_uri_decode(svn_relpath_basename(childname,
                                                                pool),
                                           pool),
                       APR_HASH_KEY_STRING, entry);
        }
    }

  if (props)
    {
      SVN_ERR(svn_ra_neon__get_props_resource(&rsrc, ras, final_url,
                                              NULL, NULL /* all props */,
                                              pool));

      *props = apr_hash_make(pool);
      SVN_ERR(filter_props(*props, rsrc, TRUE, pool));
    }

  return SVN_NO_ERROR;
}


/* ------------------------------------------------------------------------- */

svn_error_t *svn_ra_neon__get_latest_revnum(svn_ra_session_t *session,
                                            svn_revnum_t *latest_revnum,
                                            apr_pool_t *pool)
{
  svn_ra_neon__session_t *ras = session->priv;
  return svn_ra_neon__get_baseline_info(NULL, NULL, latest_revnum,
                                        ras, ras->root.path,
                                        SVN_INVALID_REVNUM, pool);
}

/* ------------------------------------------------------------------------- */


svn_error_t *svn_ra_neon__change_rev_prop(svn_ra_session_t *session,
                                          svn_revnum_t rev,
                                          const char *name,
                                          const svn_string_t *const *old_value_p,
                                          const svn_string_t *value,
                                          apr_pool_t *pool)
{
  svn_ra_neon__session_t *ras = session->priv;
  svn_error_t *err;
  apr_hash_t *prop_changes = NULL;
  apr_array_header_t *prop_deletes = NULL;
  apr_hash_t *prop_old_values = NULL;
  const char *proppatch_target;

  if (old_value_p)
    {
      svn_boolean_t capable;
      SVN_ERR(svn_ra_neon__has_capability(session, &capable,
                                          SVN_RA_CAPABILITY_ATOMIC_REVPROPS,
                                          pool));

      /* How did you get past the same check in svn_ra_change_rev_prop2()? */
      SVN_ERR_ASSERT(capable);
    }

  /* Main objective: do a PROPPATCH (allprops) on a baseline object */

  /* ### A Word From Our Sponsor:  see issue #916.

     Be it heretofore known that this Subversion behavior is
     officially in violation of WebDAV/DeltaV.  DeltaV has *no*
     concept of unversioned properties, anywhere.  If you proppatch
     something, some new version of *something* is created.

     In particular, we've decided that a 'baseline' maps to an svn
     revision; if we attempted to proppatch a baseline, a *normal*
     DeltaV server would do an auto-checkout, patch the working
     baseline, auto-checkin, and create a new baseline.  But
     mod_dav_svn just changes the baseline destructively.
  */

  if (SVN_RA_NEON__HAVE_HTTPV2_SUPPORT(ras))
    {
      proppatch_target = apr_psprintf(pool, "%s/%ld", ras->rev_stub, rev);
    }
  else
    {
      svn_ra_neon__resource_t *baseline;
      static const ne_propname wanted_props[] =
        {
          { "DAV:", "auto-version" },
          { NULL }
        };
      /* Get the baseline resource. */
      SVN_ERR(svn_ra_neon__get_baseline_props(NULL, &baseline,
                                              ras,
                                              ras->url->data,
                                              rev,
                                              wanted_props, /* DAV:auto-version */
                                              pool));
      /* ### TODO: if we got back some value for the baseline's
             'DAV:auto-version' property, interpret it.  We *don't* want
             to attempt the PROPPATCH if the deltaV server is going to do
             auto-versioning and create a new baseline! */

      proppatch_target = baseline->url;
    }

  if (old_value_p)
    {
      svn_dav__two_props_t *both_values;

      both_values = apr_palloc(pool, sizeof(*both_values));
      both_values->old_value_p = old_value_p;
      both_values->new_value = value;

      prop_old_values = apr_hash_make(pool);
      apr_hash_set(prop_old_values, name, APR_HASH_KEY_STRING, both_values);
    }
  else
    {
      if (value)
        {
          prop_changes = apr_hash_make(pool);
          apr_hash_set(prop_changes, name, APR_HASH_KEY_STRING, value);
        }
      else
        {
          prop_deletes = apr_array_make(pool, 1, sizeof(const char *));
          APR_ARRAY_PUSH(prop_deletes, const char *) = name;
        }
    }

  err = svn_ra_neon__do_proppatch(ras, proppatch_target, prop_changes,
                                  prop_deletes, prop_old_values, NULL, pool);
  if (err)
    return
      svn_error_create
      (SVN_ERR_RA_DAV_REQUEST_FAILED, err,
       _("DAV request failed; it's possible that the repository's "
         "pre-revprop-change hook either failed or is non-existent"));

  return SVN_NO_ERROR;
}


svn_error_t *svn_ra_neon__rev_proplist(svn_ra_session_t *session,
                                       svn_revnum_t rev,
                                       apr_hash_t **props,
                                       apr_pool_t *pool)
{
  svn_ra_neon__session_t *ras = session->priv;
  svn_ra_neon__resource_t *bln;
  const char *label;
  const char *url;

  *props = apr_hash_make(pool);

  /* Main objective: do a PROPFIND (allprops) on a baseline object. If we
     have HTTP v2 support available, we can build the URI of that object.
     Otherwise, we have to hunt for a bit.  (We pass NULL for 'which_props'
     in these functions because we want 'em all.)  */
  if (SVN_RA_NEON__HAVE_HTTPV2_SUPPORT(ras))
    {
      url = apr_psprintf(pool, "%s/%ld", ras->rev_stub, rev);
      label = NULL;
    }
  else
    {
      SVN_ERR(svn_ra_neon__get_vcc(&url, ras, ras->url->data, pool));
      label = apr_psprintf(pool, "%ld", rev);
    }

    SVN_ERR(svn_ra_neon__get_props_resource(&bln, ras, url,
                                            label, NULL, pool));

  /* Build a new property hash, based on the one in the baseline
     resource.  In particular, convert the xml-property-namespaces
     into ones that the client understands.  Strip away the DAV:
     liveprops as well. */
  return filter_props(*props, bln, FALSE, pool);
}


svn_error_t *svn_ra_neon__rev_prop(svn_ra_session_t *session,
                                   svn_revnum_t rev,
                                   const char *name,
                                   svn_string_t **value,
                                   apr_pool_t *pool)
{
  apr_hash_t *props;

  /* We just call svn_ra_neon__rev_proplist() and filter its results here
   * because sending the property name to the server may create an error
   * if it has a colon in its name.  While more costly this allows DAV
   * clients to still gain access to all the allowed property names.
   * See Issue #1807 for more details. */
  SVN_ERR(svn_ra_neon__rev_proplist(session, rev, &props, pool));

  *value = apr_hash_get(props, name, APR_HASH_KEY_STRING);

  return SVN_NO_ERROR;
}




/* -------------------------------------------------------------------------
**
** UPDATE HANDLING
**
** ### docco...
**
** DTD of the update report:
** ### open/add file/dir. first child is always checked-in/href (vsn_url).
** ### next are subdir elems, possibly fetch-file, then fetch-prop.
*/

/* Determine whether we're receiving the expected XML response.
   Return CHILD when interested in receiving the child's contents
   or one of SVN_RA_NEON__XML_INVALID and SVN_RA_NEON__XML_DECLINE
   when respectively this is the incorrect response or
   the element (and its children) are uninteresting */
static int validate_element(svn_ra_neon__xml_elmid parent,
                            svn_ra_neon__xml_elmid child)
{
  /* We're being very strict with the validity of XML elements here. If
     something exists that we don't know about, then we might not update
     the client properly. We also make various assumptions in the element
     processing functions, and the strong validation enables those
     assumptions. */

  switch (parent)
    {
    case ELEM_root:
      if (child == ELEM_update_report)
        return child;
      else
        return SVN_RA_NEON__XML_INVALID;

    case ELEM_update_report:
      if (child == ELEM_target_revision
          || child == ELEM_open_directory
          || child == ELEM_resource_walk)
        return child;
      else
        return SVN_RA_NEON__XML_INVALID;

    case ELEM_resource_walk:
      if (child == ELEM_resource)
        return child;
      else
        return SVN_RA_NEON__XML_INVALID;

    case ELEM_resource:
      if (child == ELEM_checked_in)
        return child;
      else
        return SVN_RA_NEON__XML_INVALID;

    case ELEM_open_directory:
      if (child == ELEM_absent_directory
          || child == ELEM_open_directory
          || child == ELEM_add_directory
          || child == ELEM_absent_file
          || child == ELEM_open_file
          || child == ELEM_add_file
          || child == ELEM_fetch_props
          || child == ELEM_set_prop
          || child == ELEM_remove_prop
          || child == ELEM_delete_entry
          || child == ELEM_SVN_prop
          || child == ELEM_checked_in)
        return child;
      else
        return SVN_RA_NEON__XML_INVALID;

    case ELEM_add_directory:
      if (child == ELEM_absent_directory
          || child == ELEM_add_directory
          || child == ELEM_absent_file
          || child == ELEM_add_file
          || child == ELEM_remove_prop
          || child == ELEM_set_prop
          || child == ELEM_SVN_prop
          || child == ELEM_checked_in)
        return child;
      else
        return SVN_RA_NEON__XML_INVALID;

    case ELEM_open_file:
      if (child == ELEM_checked_in
          || child == ELEM_fetch_file
          || child == ELEM_SVN_prop
          || child == ELEM_txdelta
          || child == ELEM_fetch_props
          || child == ELEM_set_prop
          || child == ELEM_remove_prop)
        return child;
      else
        return SVN_RA_NEON__XML_INVALID;

    case ELEM_add_file:
      if (child == ELEM_checked_in
          || child == ELEM_txdelta
          || child == ELEM_set_prop
          || child == ELEM_remove_prop
          || child == ELEM_SVN_prop)
        return child;
      else
        return SVN_RA_NEON__XML_INVALID;

    case ELEM_checked_in:
      if (child == ELEM_href)
        return child;
      else
        return SVN_RA_NEON__XML_INVALID;

    case ELEM_set_prop:
      /* Prop name is an attribute, prop value is CDATA, so no child elts. */
      return child;

    case ELEM_SVN_prop:
      /*      if (child == ELEM_version_name
              || child == ELEM_creationdate
              || child == ELEM_creator_displayname
              || child == ELEM_md5_checksum
              || child == ELEM_repository_uuid
              || child == ELEM_remove_prop)
              return child;
              else
              return SVN_RA_NEON__XML_DECLINE;
      */
      /* ### TODO:  someday uncomment the block above, and make the
         else clause return NE_XML_IGNORE.  But first, neon needs to
         define that value.  :-) */
      return child;

    default:
      return SVN_RA_NEON__XML_DECLINE;
    }

  /* NOTREACHED */
}

static void push_dir(report_baton_t *rb,
                     void *baton,
                     svn_stringbuf_t *pathbuf,
                     apr_pool_t *pool)
{
  dir_item_t *di = apr_array_push(rb->dirs);

  memset(di, 0, sizeof(*di));
  di->baton = baton;
  di->pathbuf = pathbuf;
  di->pool = pool;
}

/* This implements the `ne_xml_startelm_cb' prototype. */
static svn_error_t *
start_element(int *elem, void *userdata, int parent, const char *nspace,
              const char *elt_name, const char **atts)
{
  report_baton_t *rb = userdata;
  const char *att;
  svn_revnum_t base;
  const char *name;
  const char *bc_url;
  svn_stringbuf_t *cpath = NULL;
  svn_revnum_t crev = SVN_INVALID_REVNUM;
  dir_item_t *parent_dir;
  void *new_dir_baton;
  svn_stringbuf_t *pathbuf;
  apr_pool_t *subpool;
  const char *base_checksum = NULL;
  const svn_ra_neon__xml_elm_t *elm;

  elm = svn_ra_neon__lookup_xml_elem(report_elements, nspace, elt_name);
  *elem = elm ? validate_element(parent, elm->id) : SVN_RA_NEON__XML_DECLINE;
  if (*elem < 1) /* not a valid element */
    return SVN_NO_ERROR;

  switch (elm->id)
    {
    case ELEM_update_report:
      att = svn_xml_get_attr_value("send-all", atts);
      if (att && (strcmp(att, "true") == 0))
        rb->receiving_all = TRUE;
      break;

    case ELEM_target_revision:
      att = svn_xml_get_attr_value("rev", atts);
      if (att == NULL)
        return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                 _("Missing rev attr in target-revision"
                                   " element"));
      SVN_ERR((*rb->editor->set_target_revision)(rb->edit_baton,
                                                 SVN_STR_TO_REV(att),
                                                 rb->pool));
      break;

    case ELEM_absent_directory:
      name = svn_xml_get_attr_value("name", atts);
      if (name == NULL)
        return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                 _("Missing name attr in absent-directory"
                                   " element"));

      parent_dir = &TOP_DIR(rb);
      pathbuf = svn_stringbuf_dup(parent_dir->pathbuf, parent_dir->pool);
      svn_path_add_component(pathbuf, name);

      SVN_ERR((*rb->editor->absent_directory)(pathbuf->data,
                                              parent_dir->baton,
                                              parent_dir->pool));
      break;

    case ELEM_absent_file:
      name = svn_xml_get_attr_value("name", atts);
      if (name == NULL)
        return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                 _("Missing name attr in absent-file"
                                   " element"));
      parent_dir = &TOP_DIR(rb);
      pathbuf = svn_stringbuf_dup(parent_dir->pathbuf, parent_dir->pool);
      svn_path_add_component(pathbuf, name);

      SVN_ERR((*rb->editor->absent_file)(pathbuf->data,
                                         parent_dir->baton,
                                         parent_dir->pool));
      break;

    case ELEM_resource:
      att = svn_xml_get_attr_value("path", atts);
      if (att == NULL)
        return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                 _("Missing path attr in resource element"));
      svn_stringbuf_set(rb->current_wcprop_path, att);
      rb->in_resource = TRUE;
      break;

    case ELEM_open_directory:
      att = svn_xml_get_attr_value("rev", atts);
      if (att == NULL)
        return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                 _("Missing rev attr in open-directory"
                                   " element"));
      base = SVN_STR_TO_REV(att);

      if (DIR_DEPTH(rb) == 0)
        {
          /* pathbuf has to live for the whole edit! */
          pathbuf = svn_stringbuf_create("", rb->pool);

          /* During switch operations, we need to invalidate the
             tree's version resource URLs in case something goes
             wrong. */
          if (rb->is_switch && rb->ras->callbacks->invalidate_wc_props)
            {
              SVN_ERR(rb->ras->callbacks->invalidate_wc_props
                      (rb->ras->callback_baton, rb->target,
                       SVN_RA_NEON__LP_VSN_URL, rb->pool));
            }

          subpool = svn_pool_create(rb->pool);
          SVN_ERR((*rb->editor->open_root)(rb->edit_baton, base,
                                           subpool, &new_dir_baton));

          /* push the new baton onto the directory baton stack */
          push_dir(rb, new_dir_baton, pathbuf, subpool);
        }
      else
        {
          name = svn_xml_get_attr_value("name", atts);
          if (name == NULL)
            return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                     _("Missing name attr in open-directory"
                                       " element"));
          svn_stringbuf_set(rb->namestr, name);

          parent_dir = &TOP_DIR(rb);
          subpool = svn_pool_create(parent_dir->pool);

          pathbuf = svn_stringbuf_dup(parent_dir->pathbuf, subpool);
          svn_path_add_component(pathbuf, rb->namestr->data);

          SVN_ERR((*rb->editor->open_directory)(pathbuf->data,
                                                parent_dir->baton, base,
                                                subpool,
                                                &new_dir_baton));

          /* push the new baton onto the directory baton stack */
          push_dir(rb, new_dir_baton, pathbuf, subpool);
        }

      /* Property fetching is NOT implied in replacement. */
      TOP_DIR(rb).fetch_props = FALSE;
      break;

    case ELEM_add_directory:
      name = svn_xml_get_attr_value("name", atts);
      if (name == NULL)
        return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                 _("Missing name attr in add-directory"
                                   " element"));
      svn_stringbuf_set(rb->namestr, name);

      att = svn_xml_get_attr_value("copyfrom-path", atts);
      if (att != NULL)
        {
          cpath = rb->cpathstr;
          svn_stringbuf_set(cpath, att);

          att = svn_xml_get_attr_value("copyfrom-rev", atts);
          if (att == NULL)
            return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                     _("Missing copyfrom-rev attr in"
                                       " add-directory element"));
          crev = SVN_STR_TO_REV(att);
        }

      parent_dir = &TOP_DIR(rb);
      subpool = svn_pool_create(parent_dir->pool);

      pathbuf = svn_stringbuf_dup(parent_dir->pathbuf, subpool);
      svn_path_add_component(pathbuf, rb->namestr->data);

      SVN_ERR((*rb->editor->add_directory)(pathbuf->data, parent_dir->baton,
                                           cpath ? cpath->data : NULL,
                                           crev, subpool,
                                           &new_dir_baton));

      /* push the new baton onto the directory baton stack */
      push_dir(rb, new_dir_baton, pathbuf, subpool);

      /* Property fetching is implied in addition. */
      TOP_DIR(rb).fetch_props = TRUE;

      bc_url = svn_xml_get_attr_value("bc-url", atts);

      /* If we are not in send-all mode, we're just told to fetch the
         props later.  In that case, we can at least do a pre-emptive
         depth-1 propfind on the directory right now; this prevents
         individual propfinds on added-files later on, thus reducing
         the number of network turnarounds. */
      if ((! rb->receiving_all) && bc_url)
        {
          apr_hash_t *bc_children;
          SVN_ERR(svn_ra_neon__get_props(&bc_children,
                                         rb->ras,
                                         bc_url,
                                         SVN_RA_NEON__DEPTH_ONE,
                                         NULL, NULL /* allprops */,
                                         TOP_DIR(rb).pool));

          /* re-index the results into a more usable hash.
             bc_children maps bc-url->resource_t, but we want the
             dir_item_t's hash to map vc-url->resource_t. */
          if (bc_children)
            {
              apr_hash_index_t *hi;
              TOP_DIR(rb).children = apr_hash_make(TOP_DIR(rb).pool);

              for (hi = apr_hash_first(TOP_DIR(rb).pool, bc_children);
                   hi; hi = apr_hash_next(hi))
                {
                  void *val;
                  svn_ra_neon__resource_t *rsrc;
                  const svn_string_t *vc_url;

                  apr_hash_this(hi, NULL, NULL, &val);
                  rsrc = val;

                  vc_url = apr_hash_get(rsrc->propset,
                                        SVN_RA_NEON__PROP_CHECKED_IN,
                                        APR_HASH_KEY_STRING);
                  if (vc_url)
                    apr_hash_set(TOP_DIR(rb).children,
                                 vc_url->data, vc_url->len,
                                 rsrc->propset);
                }
            }
        }

      break;

    case ELEM_open_file:
      att = svn_xml_get_attr_value("rev", atts);
      if (att == NULL)
        return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                 _("Missing rev attr in open-file"
                                   " element"));
      base = SVN_STR_TO_REV(att);

      name = svn_xml_get_attr_value("name", atts);
      if (name == NULL)
        return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                 _("Missing name attr in open-file"
                                   " element"));
      svn_stringbuf_set(rb->namestr, name);

      parent_dir = &TOP_DIR(rb);
      rb->file_pool = svn_pool_create(parent_dir->pool);
      rb->result_checksum = NULL;

      /* Add this file's name into the directory's path buffer. It will be
         removed in end_element() */
      svn_path_add_component(parent_dir->pathbuf, rb->namestr->data);

      SVN_ERR((*rb->editor->open_file)(parent_dir->pathbuf->data,
                                       parent_dir->baton, base,
                                       rb->file_pool,
                                       &rb->file_baton));

      /* Property fetching is NOT implied in replacement. */
      rb->fetch_props = FALSE;

      break;

    case ELEM_add_file:
      name = svn_xml_get_attr_value("name", atts);
      if (name == NULL)
        return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                 _("Missing name attr in add-file"
                                   " element"));
      svn_stringbuf_set(rb->namestr, name);

      att = svn_xml_get_attr_value("copyfrom-path", atts);
      if (att != NULL)
        {
          cpath = rb->cpathstr;
          svn_stringbuf_set(cpath, att);

          att = svn_xml_get_attr_value("copyfrom-rev", atts);
          if (att == NULL)
            return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                     _("Missing copyfrom-rev attr in add-file"
                                       " element"));
          crev = SVN_STR_TO_REV(att);
        }

      parent_dir = &TOP_DIR(rb);
      rb->file_pool = svn_pool_create(parent_dir->pool);
      rb->result_checksum = NULL;

      /* Add this file's name into the directory's path buffer. It will be
         removed in end_element() */
      svn_path_add_component(parent_dir->pathbuf, rb->namestr->data);

      SVN_ERR((*rb->editor->add_file)(parent_dir->pathbuf->data,
                                      parent_dir->baton,
                                      cpath ? cpath->data : NULL,
                                      crev, rb->file_pool,
                                      &rb->file_baton));

      /* Property fetching is implied in addition. */
      rb->fetch_props = TRUE;

      break;

    case ELEM_txdelta:
      /* Pre 1.2, mod_dav_svn was using <txdelta> tags (in addition to
         <fetch-file>s and such) when *not* in "send-all" mode.  As a
         client, we're smart enough to know that's wrong, so when not
         in "receiving-all" mode, we'll ignore <txdelta> tags
         altogether. */
      if (! rb->receiving_all)
        break;

      base_checksum = svn_xml_get_attr_value("base-checksum", atts);

      SVN_ERR((*rb->editor->apply_textdelta)(rb->file_baton,
                                             base_checksum,
                                             rb->file_pool,
                                             &(rb->whandler),
                                             &(rb->whandler_baton)));

      rb->svndiff_decoder = svn_txdelta_parse_svndiff(rb->whandler,
                                                      rb->whandler_baton,
                                                      TRUE, rb->file_pool);

      rb->base64_decoder = svn_base64_decode(rb->svndiff_decoder,
                                             rb->file_pool);
      break;

    case ELEM_set_prop:
      {
        const char *encoding = svn_xml_get_attr_value("encoding", atts);
        name = svn_xml_get_attr_value("name", atts);
        if (name == NULL)
          return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                   _("Missing name attr in set-prop element"));
        svn_stringbuf_set(rb->namestr, name);
        if (encoding)
          svn_stringbuf_set(rb->encoding, encoding);
        else
          svn_stringbuf_setempty(rb->encoding);
      }

      break;

    case ELEM_remove_prop:
      name = svn_xml_get_attr_value("name", atts);
      if (name == NULL)
        return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                 _("Missing name attr in remove-prop element"));
      svn_stringbuf_set(rb->namestr, name);

      /* Removing a prop.  */
      if (rb->file_baton == NULL)
        SVN_ERR(rb->editor->change_dir_prop(TOP_DIR(rb).baton,
                                            rb->namestr->data,
                                            NULL, TOP_DIR(rb).pool));
      else
        SVN_ERR(rb->editor->change_file_prop(rb->file_baton, rb->namestr->data,
                                             NULL, rb->file_pool));
      break;

    case ELEM_fetch_props:
      if (!rb->fetch_content)
        {
          /* If this is just a status check, the specifics of the
             property change are uninteresting.  Simply call our
             editor function with bogus data so it registers a
             property mod. */
          svn_stringbuf_set(rb->namestr, SVN_PROP_PREFIX "BOGOSITY");

          if (rb->file_baton == NULL)
            SVN_ERR(rb->editor->change_dir_prop(TOP_DIR(rb).baton,
                                                rb->namestr->data,
                                                NULL, TOP_DIR(rb).pool));
          else
            SVN_ERR(rb->editor->change_file_prop(rb->file_baton,
                                                 rb->namestr->data,
                                                 NULL, rb->file_pool));
        }
      else
        {
          /* Note that we need to fetch props for this... */
          if (rb->file_baton == NULL)
            TOP_DIR(rb).fetch_props = TRUE; /* ...directory. */
          else
            rb->fetch_props = TRUE; /* ...file. */
        }
      break;

    case ELEM_fetch_file:
      base_checksum = svn_xml_get_attr_value("base-checksum", atts);
      rb->result_checksum = NULL;

      /* If we aren't expecting to see the file contents inline, we
         should ignore server requests to fetch them.

         ### This conditional was added to counteract a little bug in
         Subversion 0.33.0's mod_dav_svn whereby both the <txdelta>
         and <fetch-file> tags were being transmitted.  Someday, we
         should remove the conditional again to give the server the
         option of sending inline text-deltas for some files while
         telling the client to fetch others. */
      if (! rb->receiving_all)
        {
          /* assert: rb->href->len > 0 */
          SVN_ERR(simple_fetch_file(rb->ras,
                                    rb->href->data,
                                    TOP_DIR(rb).pathbuf->data,
                                    rb->fetch_content,
                                    rb->file_baton,
                                    base_checksum,
                                    rb->editor,
                                    rb->ras->callbacks->get_wc_prop,
                                    rb->ras->callback_baton,
                                    rb->file_pool));
        }
      break;

    case ELEM_delete_entry:
      name = svn_xml_get_attr_value("name", atts);
      if (name == NULL)
        return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                 _("Missing name attr in delete-entry"
                                   " element"));
      svn_stringbuf_set(rb->namestr, name);

      att = svn_xml_get_attr_value("rev", atts);
      if (att) /* Not available on older repositories! */
        crev = SVN_STR_TO_REV(att);

      parent_dir = &TOP_DIR(rb);

      /* Pool use is a little non-standard here.  When lots of items in the
         same directory get deleted each one will trigger a call to
         editor->delete_entry, but we don't have a pool that readily fits
         the usual iteration pattern and so memory use could grow without
         bound (see issue 1635).  To avoid such growth we use a temporary,
         short-lived, pool. */
      subpool = svn_pool_create(parent_dir->pool);

      pathbuf = svn_stringbuf_dup(parent_dir->pathbuf, subpool);
      svn_path_add_component(pathbuf, rb->namestr->data);

      SVN_ERR((*rb->editor->delete_entry)(pathbuf->data,
                                          crev,
                                          parent_dir->baton,
                                          subpool));
      svn_pool_destroy(subpool);
      break;

    default:
      break;
    }

  *elem = elm->id;

  return SVN_NO_ERROR;
}


static svn_error_t *
add_node_props(report_baton_t *rb, apr_pool_t *pool)
{
  svn_ra_neon__resource_t *rsrc = NULL;
  apr_hash_t *props = NULL;

  /* Do nothing if parsing a send-all-style report, because the properties
     already come inline. */
  if (rb->receiving_all)
    return SVN_NO_ERROR;

  /* Do nothing (else) if we aren't fetching content.  */
  if (!rb->fetch_content)
    return SVN_NO_ERROR;

  if (rb->file_baton)
    {
      const char *lock_token = apr_hash_get(rb->lock_tokens,
                                            TOP_DIR(rb).pathbuf->data,
                                            TOP_DIR(rb).pathbuf->len);

      /* Workaround a buglet in older versions of mod_dav_svn in that it
         will not send remove-prop in the update report when a lock
         property disappears when send-all is false.  */
      if (lock_token)
        {
          svn_lock_t *lock;
          SVN_ERR(svn_ra_neon__get_lock_internal(rb->ras, &lock,
                                                 TOP_DIR(rb).pathbuf->data,
                                                 pool));
          if (! (lock
                 && lock->token
                 && (strcmp(lock->token, lock_token) == 0)))
            SVN_ERR(rb->editor->change_file_prop(rb->file_baton,
                                                 SVN_PROP_ENTRY_LOCK_TOKEN,
                                                 NULL, pool));
        }

      /* If we aren't supposed to be fetching props, don't. */
      if (! rb->fetch_props)
        return SVN_NO_ERROR;

      /* Check to see if your parent directory already has your props
         stored, possibly from a depth-1 propfind.   Otherwise just do
         a propfind directly on the file url. */
      if ( ! ((TOP_DIR(rb).children)
              && (props = apr_hash_get(TOP_DIR(rb).children, rb->href->data,
                                       APR_HASH_KEY_STRING))) )
        {
          SVN_ERR(svn_ra_neon__get_props_resource(&rsrc,
                                                  rb->ras,
                                                  rb->href->data,
                                                  NULL,
                                                  NULL,
                                                  pool));
          props = rsrc->propset;
        }

      SVN_ERR(add_props(props,
                        rb->editor->change_file_prop,
                        rb->file_baton,
                        pool));
    }
  else
    {
      if (! TOP_DIR(rb).fetch_props)
        return SVN_NO_ERROR;

      /* Check to see if your props are already stored, possibly from
         a depth-1 propfind.  Otherwise just do a propfind directly on
         the directory url. */
      if ( ! ((TOP_DIR(rb).children)
              && (props = apr_hash_get(TOP_DIR(rb).children,
                                       TOP_DIR(rb).vsn_url,
                                       APR_HASH_KEY_STRING))) )
        {
          SVN_ERR(svn_ra_neon__get_props_resource(&rsrc,
                                                  rb->ras,
                                                  TOP_DIR(rb).vsn_url,
                                                  NULL,
                                                  NULL,
                                                  pool));
          props = rsrc->propset;
        }

      SVN_ERR(add_props(props,
                        rb->editor->change_dir_prop,
                        TOP_DIR(rb).baton,
                        pool));
    }

  return SVN_NO_ERROR;
}

/* This implements the `svn_ra_neon__cdata_cb_t' prototype. */
static svn_error_t *
cdata_handler(void *userdata, int state, const char *cdata, size_t len)
{
  report_baton_t *rb = userdata;

  switch(state)
    {
    case ELEM_href:
    case ELEM_set_prop:
    case ELEM_md5_checksum:
    case ELEM_version_name:
    case ELEM_creationdate:
    case ELEM_creator_displayname:
      svn_stringbuf_appendbytes(rb->cdata_accum, cdata, len);
      break;

    case ELEM_txdelta:
      {
        apr_size_t nlen = len;

        /* Pre 1.2, mod_dav_svn was using <txdelta> tags (in addition to
           <fetch-file>s and such) when *not* in "send-all" mode.  As a
           client, we're smart enough to know that's wrong, so when not
           in "receiving-all" mode, we'll ignore <txdelta> tags
           altogether. */
        if (! rb->receiving_all)
          break;

        SVN_ERR(svn_stream_write(rb->base64_decoder, cdata, &nlen));
        if (nlen != len)
          {
            /* Short write without associated error?  "Can't happen." */
            return svn_error_createf(SVN_ERR_STREAM_UNEXPECTED_EOF, NULL,
                                     _("Error writing to '%s': unexpected EOF"),
                                     rb->namestr->data);
          }
      }
      break;
    }

  return 0; /* no error */
}

/* This implements the `svn_ra_neon_endelm_cb_t' prototype. */
static svn_error_t *
end_element(void *userdata, int state,
            const char *nspace, const char *elt_name)
{
  report_baton_t *rb = userdata;
  const svn_delta_editor_t *editor = rb->editor;
  const svn_ra_neon__xml_elm_t *elm;

  elm = svn_ra_neon__lookup_xml_elem(report_elements, nspace, elt_name);

  if (elm == NULL)
    return SVN_NO_ERROR;

  switch (elm->id)
    {
    case ELEM_resource:
      rb->in_resource = FALSE;
      break;

    case ELEM_update_report:
      /* End of report; close up the editor. */
      SVN_ERR((*rb->editor->close_edit)(rb->edit_baton, rb->pool));
      rb->edit_baton = NULL;
      break;

    case ELEM_add_directory:
    case ELEM_open_directory:

      /* fetch node props, unless this is the top dir and the real
         target of the operation is not the top dir. */
      if (! ((DIR_DEPTH(rb) == 1) && *rb->target))
        SVN_ERR(add_node_props(rb, TOP_DIR(rb).pool));

      /* Close the directory on top of the stack, and pop it.  Also,
         destroy the subpool used exclusive by this directory and its
         children.  */
      SVN_ERR((*rb->editor->close_directory)(TOP_DIR(rb).baton,
                                             TOP_DIR(rb).pool));
      svn_pool_destroy(TOP_DIR(rb).pool);
      apr_array_pop(rb->dirs);
      break;

    case ELEM_add_file:
      /* we wait until the close element to do the work. this allows us to
         retrieve the href before fetching. */

      /* fetch file */
      if (! rb->receiving_all)
        {
          SVN_ERR(simple_fetch_file(rb->ras,
                                    rb->href->data,
                                    TOP_DIR(rb).pathbuf->data,
                                    rb->fetch_content,
                                    rb->file_baton,
                                    NULL,  /* no base checksum in an add */
                                    rb->editor,
                                    NULL, NULL, /* dav_prop callback */
                                    rb->file_pool));

          /* fetch node props as necessary. */
          SVN_ERR(add_node_props(rb, rb->file_pool));
        }

      /* close the file and mark that we are no longer operating on a file */
      SVN_ERR((*rb->editor->close_file)(rb->file_baton,
                                        rb->result_checksum,
                                        rb->file_pool));
      rb->file_baton = NULL;

      /* Yank this file out of the directory's path buffer. */
      svn_path_remove_component(TOP_DIR(rb).pathbuf);
      svn_pool_destroy(rb->file_pool);
      rb->file_pool = NULL;
      break;

    case ELEM_txdelta:
      /* Pre 1.2, mod_dav_svn was using <txdelta> tags (in addition to
         <fetch-file>s and such) when *not* in "send-all" mode.  As a
         client, we're smart enough to know that's wrong, so when not
         in "receiving-all" mode, we'll ignore <txdelta> tags
         altogether. */
      if (! rb->receiving_all)
        break;

      SVN_ERR(svn_stream_close(rb->base64_decoder));
      rb->whandler = NULL;
      rb->whandler_baton = NULL;
      rb->svndiff_decoder = NULL;
      rb->base64_decoder = NULL;
      break;

    case ELEM_open_file:
      /* fetch node props as necessary. */
      SVN_ERR(add_node_props(rb, rb->file_pool));

      /* close the file and mark that we are no longer operating on a file */
      SVN_ERR((*rb->editor->close_file)(rb->file_baton,
                                        rb->result_checksum,
                                        rb->file_pool));
      rb->file_baton = NULL;

      /* Yank this file out of the directory's path buffer. */
      svn_path_remove_component(TOP_DIR(rb).pathbuf);
      svn_pool_destroy(rb->file_pool);
      rb->file_pool = NULL;
      break;

    case ELEM_set_prop:
      {
        svn_string_t decoded_value;
        const svn_string_t *decoded_value_p;
        apr_pool_t *pool;

        if (rb->file_baton)
          pool = rb->file_pool;
        else
          pool = TOP_DIR(rb).pool;

        decoded_value.data = rb->cdata_accum->data;
        decoded_value.len = rb->cdata_accum->len;

        /* Determine the cdata encoding, if any. */
        if (svn_stringbuf_isempty(rb->encoding))
          {
            decoded_value_p = &decoded_value;
          }
        else if (strcmp(rb->encoding->data, "base64") == 0)
          {
            decoded_value_p = svn_base64_decode_string(&decoded_value, pool);
            svn_stringbuf_setempty(rb->encoding);
          }
        else
          {
            return svn_error_createf(SVN_ERR_XML_UNKNOWN_ENCODING, NULL,
                                     _("Unknown XML encoding: '%s'"),
                                     rb->encoding->data);
          }

        /* Set the prop. */
        if (rb->file_baton)
          {
            SVN_ERR(rb->editor->change_file_prop(rb->file_baton,
                                                 rb->namestr->data,
                                                 decoded_value_p, pool));
          }
        else
          {
            SVN_ERR(rb->editor->change_dir_prop(TOP_DIR(rb).baton,
                                                rb->namestr->data,
                                                decoded_value_p, pool));
          }
      }

      svn_stringbuf_setempty(rb->cdata_accum);
      break;

    case ELEM_href:
      if (rb->fetch_content)
        /* record the href that we just found */
        SVN_ERR(svn_ra_neon__copy_href(rb->href, rb->cdata_accum->data,
                                       rb->scratch_pool));

      svn_stringbuf_setempty(rb->cdata_accum);

      /* do nothing if we aren't fetching content. */
      if (!rb->fetch_content)
        break;

      /* if we're within a <resource> tag, then just call the generic
         RA set_wcprop_callback directly;  no need to use the
         update-editor.  */
      if (rb->in_resource)
        {
          svn_string_t href_val;
          href_val.data = rb->href->data;
          href_val.len = rb->href->len;

          if (rb->ras->callbacks->set_wc_prop != NULL)
            SVN_ERR(rb->ras->callbacks->set_wc_prop
                    (rb->ras->callback_baton,
                     rb->current_wcprop_path->data,
                     SVN_RA_NEON__LP_VSN_URL,
                     &href_val,
                     rb->scratch_pool));
          svn_pool_clear(rb->scratch_pool);
        }
      /* else we're setting a wcprop in the context of an editor drive. */
      else if (rb->file_baton == NULL)
        {
          /* Update the wcprop here, unless this is the top directory
             and the real target of this operation is something other
             than the top directory. */
          if (! ((DIR_DEPTH(rb) == 1) && *rb->target))
            {
              SVN_ERR(simple_store_vsn_url(rb->href->data, TOP_DIR(rb).baton,
                                           rb->editor->change_dir_prop,
                                           TOP_DIR(rb).pool));

              /* save away the URL in case a fetch-props arrives after all of
                 the subdir processing. we will need this copy of the URL to
                 fetch the properties (i.e. rb->href will be toast by then). */
              TOP_DIR(rb).vsn_url = apr_pmemdup(TOP_DIR(rb).pool,
                                                rb->href->data,
                                                rb->href->len + 1);
            }
        }
      else
        {
          SVN_ERR(simple_store_vsn_url(rb->href->data, rb->file_baton,
                                       rb->editor->change_file_prop,
                                       rb->file_pool));
        }
      break;

    case ELEM_md5_checksum:
      /* We only care about file checksums. */
      if (rb->file_baton)
        {
          rb->result_checksum = apr_pstrdup(rb->file_pool,
                                            rb->cdata_accum->data);
        }
      svn_stringbuf_setempty(rb->cdata_accum);
      break;

    case ELEM_version_name:
    case ELEM_creationdate:
    case ELEM_creator_displayname:
      {
        /* The name of the xml tag is the property that we want to set. */
        apr_pool_t *pool =
          rb->file_baton ? rb->file_pool : TOP_DIR(rb).pool;
        prop_setter_t setter =
          rb->file_baton ? editor->change_file_prop : editor->change_dir_prop;
        const char *name = apr_pstrcat(pool, elm->nspace, elm->name,
                                       (char *)NULL);
        void *baton = rb->file_baton ? rb->file_baton : TOP_DIR(rb).baton;
        svn_string_t valstr;

        valstr.data = rb->cdata_accum->data;
        valstr.len = rb->cdata_accum->len;
        SVN_ERR(set_special_wc_prop(name, &valstr, setter, baton, pool));
        svn_stringbuf_setempty(rb->cdata_accum);
      }
      break;

    default:
      break;
    }

  return SVN_NO_ERROR;
}


static svn_error_t * reporter_set_path(void *report_baton,
                                       const char *path,
                                       svn_revnum_t revision,
                                       svn_depth_t depth,
                                       svn_boolean_t start_empty,
                                       const char *lock_token,
                                       apr_pool_t *pool)
{
  report_baton_t *rb = report_baton;
  const char *entry;
  svn_stringbuf_t *qpath = NULL;
  const char *tokenstring = "";
  const char *depthstring = apr_psprintf(pool, "depth=\"%s\"",
                                         svn_depth_to_word(depth));

  if (lock_token)
    {
      tokenstring = apr_psprintf(pool, "lock-token=\"%s\"", lock_token);
      apr_hash_set(rb->lock_tokens,
                   apr_pstrdup(apr_hash_pool_get(rb->lock_tokens), path),
                   APR_HASH_KEY_STRING,
                   apr_pstrdup(apr_hash_pool_get(rb->lock_tokens), lock_token));
    }

  svn_xml_escape_cdata_cstring(&qpath, path, pool);
  if (start_empty)
    entry = apr_psprintf(pool,
                         "<S:entry rev=\"%ld\" %s %s"
                         " start-empty=\"true\">%s</S:entry>" DEBUG_CR,
                         revision, depthstring, tokenstring, qpath->data);
  else
    entry = apr_psprintf(pool,
                         "<S:entry rev=\"%ld\" %s %s>"
                         "%s</S:entry>" DEBUG_CR,
                         revision, depthstring, tokenstring, qpath->data);

  return svn_error_trace(svn_io_file_write_full(rb->tmpfile, entry,
                                                strlen(entry), NULL, pool));
}


static svn_error_t * reporter_link_path(void *report_baton,
                                        const char *path,
                                        const char *url,
                                        svn_revnum_t revision,
                                        svn_depth_t depth,
                                        svn_boolean_t start_empty,
                                        const char *lock_token,
                                        apr_pool_t *pool)
{
  report_baton_t *rb = report_baton;
  const char *entry;
  svn_stringbuf_t *qpath = NULL, *qlinkpath = NULL;
  const char *bc_relative;
  const char *tokenstring = "";
  const char *depthstring = apr_psprintf(pool, "depth=\"%s\"",
                                         svn_depth_to_word(depth));

  if (lock_token)
    {
      tokenstring = apr_psprintf(pool, "lock-token=\"%s\"", lock_token);
      apr_hash_set(rb->lock_tokens,
                   apr_pstrdup(apr_hash_pool_get(rb->lock_tokens), path),
                   APR_HASH_KEY_STRING,
                   apr_pstrdup(apr_hash_pool_get(rb->lock_tokens), lock_token));
    }

  /* Convert the copyfrom_* url/rev "public" pair into a Baseline
     Collection (BC) URL that represents the revision -- and a
     relative path under that BC.  */
  SVN_ERR(svn_ra_neon__get_baseline_info(NULL, &bc_relative, NULL, rb->ras,
                                         url, revision, pool));


  svn_xml_escape_cdata_cstring(&qpath, path, pool);
  svn_xml_escape_attr_cstring(&qlinkpath, bc_relative, pool);
  if (start_empty)
    entry = apr_psprintf(pool,
                         "<S:entry rev=\"%ld\" %s %s"
                         " linkpath=\"/%s\" start-empty=\"true\""
                         ">%s</S:entry>" DEBUG_CR,
                         revision, depthstring, tokenstring,
                         qlinkpath->data, qpath->data);
  else
    entry = apr_psprintf(pool,
                         "<S:entry rev=\"%ld\" %s %s"
                         " linkpath=\"/%s\">%s</S:entry>" DEBUG_CR,
                         revision, depthstring, tokenstring,
                         qlinkpath->data, qpath->data);

  return svn_error_trace(svn_io_file_write_full(rb->tmpfile, entry,
                                                strlen(entry), NULL, pool));
}


static svn_error_t * reporter_delete_path(void *report_baton,
                                          const char *path,
                                          apr_pool_t *pool)
{
  report_baton_t *rb = report_baton;
  const char *s;
  svn_stringbuf_t *qpath = NULL;

  svn_xml_escape_cdata_cstring(&qpath, path, pool);
  s = apr_psprintf(pool,
                   "<S:missing>%s</S:missing>" DEBUG_CR,
                   qpath->data);

  return svn_error_trace(svn_io_file_write_full(rb->tmpfile, s, strlen(s),
                                                NULL, pool));
}


static svn_error_t * reporter_abort_report(void *report_baton,
                                           apr_pool_t *pool)
{
  report_baton_t *rb = report_baton;

  SVN_ERR(svn_io_file_close(rb->tmpfile, pool));

  return SVN_NO_ERROR;
}


static svn_error_t * reporter_finish_report(void *report_baton,
                                            apr_pool_t *pool)
{
  report_baton_t *rb = report_baton;
  svn_error_t *err;
  const char *report_target;
  apr_hash_t *request_headers = apr_hash_make(pool);
  apr_hash_set(request_headers, "Accept-Encoding", APR_HASH_KEY_STRING,
               "svndiff1;q=0.9,svndiff;q=0.8");


#define SVN_RA_NEON__REPORT_TAIL  "</S:update-report>" DEBUG_CR
  /* write the final closing gunk to our request body. */
  SVN_ERR(svn_io_file_write_full(rb->tmpfile,
                                 SVN_RA_NEON__REPORT_TAIL,
                                 sizeof(SVN_RA_NEON__REPORT_TAIL) - 1,
                                 NULL, pool));
#undef SVN_RA_NEON__REPORT_TAIL

  /* get the editor process prepped */
  rb->dirs = apr_array_make(rb->pool, 5, sizeof(dir_item_t));
  rb->namestr = MAKE_BUFFER(rb->pool);
  rb->cpathstr = MAKE_BUFFER(rb->pool);
  rb->encoding = MAKE_BUFFER(rb->pool);
  rb->href = MAKE_BUFFER(rb->pool);

  /* Got HTTP v2 support?  We'll report against the "me resource". */
  if (SVN_RA_NEON__HAVE_HTTPV2_SUPPORT(rb->ras))
    {
      report_target = rb->ras->me_resource;
    }
  /* Else, get the VCC.  (If this doesn't work out for us, don't
     forget to remove the tmpfile before returning the error.)  */
  else if ((err = svn_ra_neon__get_vcc(&report_target, rb->ras,
                                       rb->ras->url->data, pool)))
    {
      /* We're done with the file.  this should delete it. Note: it
         isn't a big deal if this line is never executed -- the pool
         will eventually get it. We're just being proactive here. */
      return svn_error_trace(
                svn_error_compose_create(err,
                                         svn_io_file_close(rb->tmpfile,
                                                           pool)));
    }

  /* dispatch the REPORT. */
  err = svn_ra_neon__parsed_request(rb->ras, "REPORT", report_target,
                                    NULL, rb->tmpfile, NULL,
                                    start_element,
                                    cdata_handler,
                                    end_element,
                                    rb,
                                    request_headers, NULL,
                                    rb->spool_response, pool);

  /* We're done with the file. Proactively close/delete the thing. */
  SVN_ERR(svn_error_compose_create(err,
                                   svn_io_file_close(rb->tmpfile, pool)));

  /* We got the whole HTTP response thing done.  *Whew*.  Our edit
     baton should have been closed by now, so return a failure if it
     hasn't been. */
  if (rb->edit_baton)
    {
      return svn_error_createf(
         SVN_ERR_RA_DAV_REQUEST_FAILED, NULL,
         _("REPORT response handling failed to complete the editor drive"));
    }

  return SVN_NO_ERROR;
}

static const svn_ra_reporter3_t ra_neon_reporter = {
  reporter_set_path,
  reporter_delete_path,
  reporter_link_path,
  reporter_finish_report,
  reporter_abort_report
};


/* Make a generic REPORTER / REPORT_BATON for reporting the state of
   the working copy against REVISION during updates or status checks.
   The server will drive EDITOR / EDIT_BATON to indicate how to
   transform the working copy into the requested target.

   SESSION is the RA session in use.  TARGET is an optional single
   path component will restrict the scope of the operation to an entry
   in the directory represented by the SESSION's URL, or empty if the
   entire directory is meant to be the target.

   DEPTH is the requested depth of the operation.  It will be
   transmitted to the server, which (if it understands depths) can use
   the information to limit the information it sends back.  Also store
   DEPTH in the REPORT_BATON: that way, if the server is old and does
   not understand depth requests, the client can notice this when the
   response starts streaming in, and adjust accordingly (as of this
   writnig, by wrapping REPORTER->editor and REPORTER->edit_baton in a
   filtering editor that simply tosses out the data the client doesn't
   want).

   If SEND_COPYFROM_ARGS is set, then ask the server to transmit
   copyfrom args in add_file() in add_directory() calls.

   If IGNORE_ANCESTRY is set, the server will transmit real diffs
   between the working copy and the target even if those objects are
   not historically related.  Otherwise, the response will generally
   look like a giant delete followed by a giant add.

   RESOURCE_WALK controls whether to ask the DAV server to supply an
   entire tree's worth of version-resource-URL working copy cache
   updates.

   FETCH_CONTENT is used by the REPORT response parser to determine
   whether it should bother getting the contents of files represented
   in the delta response (of if a directory delta is all that is of
   interest).

   If SEND_ALL is set, the server will be asked to embed contents into
   the main response.

   If SPOOL_RESPONSE is set, the REPORT response will be cached to
   disk in a tmpfile (in full), then read back and parsed.

   Oh, and do all this junk in POOL.  */
static svn_error_t *
make_reporter(svn_ra_session_t *session,
              const svn_ra_reporter3_t **reporter,
              void **report_baton,
              svn_revnum_t revision,
              const char *target,
              const char *dst_path,
              svn_depth_t depth,
              svn_boolean_t send_copyfrom_args,
              svn_boolean_t ignore_ancestry,
              svn_boolean_t resource_walk,
              const svn_delta_editor_t *editor,
              void *edit_baton,
              svn_boolean_t fetch_content,
              svn_boolean_t send_all,
              svn_boolean_t spool_response,
              apr_pool_t *pool)
{
  svn_ra_neon__session_t *ras = session->priv;
  report_baton_t *rb;
  const char *s;
  svn_stringbuf_t *xml_s;
  const svn_delta_editor_t *filter_editor;
  void *filter_baton;
  svn_boolean_t has_target = *target != '\0';
  svn_boolean_t server_supports_depth;

  SVN_ERR(svn_ra_neon__has_capability(session, &server_supports_depth,
                                      SVN_RA_CAPABILITY_DEPTH, pool));
  /* We can skip the depth filtering when the user requested
     depth_files or depth_infinity because the server will
     transmit the right stuff anyway. */
  if ((depth != svn_depth_files)
      && (depth != svn_depth_infinity)
      && ! server_supports_depth)
    {
      SVN_ERR(svn_delta_depth_filter_editor(&filter_editor,
                                            &filter_baton,
                                            editor,
                                            edit_baton,
                                            depth,
                                            has_target,
                                            pool));
      editor = filter_editor;
      edit_baton = filter_baton;
    }

  rb = apr_pcalloc(pool, sizeof(*rb));
  rb->ras = ras;
  rb->pool = pool;
  rb->scratch_pool = svn_pool_create(pool);
  rb->editor = editor;
  rb->edit_baton = edit_baton;
  rb->fetch_content = fetch_content;
  rb->in_resource = FALSE;
  rb->current_wcprop_path = svn_stringbuf_create("", pool);
  rb->is_switch = dst_path != NULL;
  rb->target = target;
  rb->receiving_all = FALSE;
  rb->spool_response = spool_response;
  rb->whandler = NULL;
  rb->whandler_baton = NULL;
  rb->svndiff_decoder = NULL;
  rb->base64_decoder = NULL;
  rb->cdata_accum = svn_stringbuf_create("", pool);
  rb->send_copyfrom_args = send_copyfrom_args;
  rb->lock_tokens = apr_hash_make(pool);

  /* Neon "pulls" request body content from the caller. The reporter is
     organized where data is "pushed" into self. To match these up, we use
     an intermediate file -- push data into the file, then let Neon pull
     from the file.

     Note: one day we could spin up a thread and use a pipe between this
     code and Neon. We write to a pipe, Neon reads from the pipe. Each
     thread can block on the pipe, waiting for the other to complete its
     work.
  */

  /* Create a temp file in the system area to hold the contents. Note that
     we need a file since we will be rewinding it. The file will be closed
     and deleted when the pool is cleaned up. */
  SVN_ERR(svn_io_open_unique_file3(&rb->tmpfile, NULL, NULL,
                                   svn_io_file_del_on_pool_cleanup,
                                   pool, pool));

  /* prep the file */
  s = apr_psprintf(pool, "<S:update-report send-all=\"%s\" xmlns:S=\""
                   SVN_XML_NAMESPACE "\">" DEBUG_CR,
                   send_all ? "true" : "false");
  SVN_ERR(svn_io_file_write_full(rb->tmpfile, s, strlen(s), NULL, pool));

  /* always write the original source path.  this is part of the "new
     style" update-report syntax.  if the tmpfile is used in an "old
     style' update-report request, older servers will just ignore this
     unknown xml element. */
  xml_s = NULL;
  svn_xml_escape_cdata_cstring(&xml_s, ras->url->data, pool);
  s = apr_psprintf(pool, "<S:src-path>%s</S:src-path>" DEBUG_CR, xml_s->data);
  SVN_ERR(svn_io_file_write_full(rb->tmpfile, s, strlen(s), NULL, pool));

  /* an invalid revnum means "latest". we can just omit the target-revision
     element in that case. */
  if (SVN_IS_VALID_REVNUM(revision))
    {
      s = apr_psprintf(pool,
                       "<S:target-revision>%ld</S:target-revision>" DEBUG_CR,
                       revision);
      SVN_ERR(svn_io_file_write_full(rb->tmpfile, s, strlen(s), NULL, pool));
    }

  /* Pre-0.36 servers don't like to see an empty target string.  */
  if (*target)
    {
      xml_s = NULL;
      svn_xml_escape_cdata_cstring(&xml_s, target, pool);
      s = apr_psprintf(pool, "<S:update-target>%s</S:update-target>" DEBUG_CR,
                       xml_s->data);
      SVN_ERR(svn_io_file_write_full(rb->tmpfile, s, strlen(s), NULL, pool));
    }


  /* A NULL dst_path is also no problem;  this is only passed during a
     'switch' operation.  If NULL, we don't mention it in the custom
     report, and mod_dav_svn automatically runs dir_delta() on two
     identical paths. */
  if (dst_path)
    {
      xml_s = NULL;
      svn_xml_escape_cdata_cstring(&xml_s, dst_path, pool);
      s = apr_psprintf(pool, "<S:dst-path>%s</S:dst-path>" DEBUG_CR,
                       xml_s->data);
      SVN_ERR(svn_io_file_write_full(rb->tmpfile, s, strlen(s), NULL, pool));
    }

  /* Old servers know "recursive" but not "depth"; help them DTRT. */
  if (depth == svn_depth_files || depth == svn_depth_empty)
    {
      const char *data = "<S:recursive>no</S:recursive>" DEBUG_CR;
      SVN_ERR(svn_io_file_write_full(rb->tmpfile, data, strlen(data),
                                     NULL, pool));
    }

  /* mod_dav_svn defaults to svn_depth_infinity, but we always send anyway. */
  {
    s = apr_psprintf(pool, "<S:depth>%s</S:depth>" DEBUG_CR,
                     svn_depth_to_word(depth));
    SVN_ERR(svn_io_file_write_full(rb->tmpfile, s, strlen(s), NULL, pool));
  }

  /* mod_dav_svn will use ancestry in diffs unless it finds this element. */
  if (ignore_ancestry)
    {
      const char *data = "<S:ignore-ancestry>yes</S:ignore-ancestry>" DEBUG_CR;
      SVN_ERR(svn_io_file_write_full(rb->tmpfile, data, strlen(data),
                                     NULL, pool));
    }

  /* mod_dav_svn 1.5 and later won't send copyfrom args unless it
     finds this element.  older mod_dav_svn modules should just
     ignore the unknown element. */
  if (send_copyfrom_args)
    {
      const char *data =
        "<S:send-copyfrom-args>yes</S:send-copyfrom-args>" DEBUG_CR;
      SVN_ERR(svn_io_file_write_full(rb->tmpfile, data, strlen(data),
                                     NULL, pool));
    }

  /* If we want a resource walk to occur, note that now. */
  if (resource_walk)
    {
      const char *data = "<S:resource-walk>yes</S:resource-walk>" DEBUG_CR;
      SVN_ERR(svn_io_file_write_full(rb->tmpfile, data, strlen(data),
                                     NULL, pool));
    }

  /* When in 'send-all' mode, mod_dav_svn will assume that it should
     calculate and transmit real text-deltas (instead of empty windows
     that merely indicate "text is changed") unless it finds this
     element.  When not in 'send-all' mode, mod_dav_svn will never
     send text-deltas at all.

     NOTE: Do NOT count on servers actually obeying this, as some exist
     which obey send-all, but do not check for this directive at all! */
  if (send_all && (! fetch_content))
    {
      const char *data = "<S:text-deltas>no</S:text-deltas>" DEBUG_CR;
      SVN_ERR(svn_io_file_write_full(rb->tmpfile, data, strlen(data),
                                     NULL, pool));
    }

  *reporter = &ra_neon_reporter;
  *report_baton = rb;

  return SVN_NO_ERROR;
}


svn_error_t * svn_ra_neon__do_update(svn_ra_session_t *session,
                                     const svn_ra_reporter3_t **reporter,
                                     void **report_baton,
                                     svn_revnum_t revision_to_update_to,
                                     const char *update_target,
                                     svn_depth_t depth,
                                     svn_boolean_t send_copyfrom_args,
                                     const svn_delta_editor_t *wc_update,
                                     void *wc_update_baton,
                                     apr_pool_t *pool)
{
  return make_reporter(session,
                       reporter,
                       report_baton,
                       revision_to_update_to,
                       update_target,
                       NULL,
                       depth,
                       send_copyfrom_args,
                       FALSE,
                       FALSE,
                       wc_update,
                       wc_update_baton,
                       TRUE, /* fetch_content */
                       TRUE, /* send_all */
                       FALSE, /* spool_response */
                       pool);
}


svn_error_t * svn_ra_neon__do_status(svn_ra_session_t *session,
                                     const svn_ra_reporter3_t **reporter,
                                     void **report_baton,
                                     const char *status_target,
                                     svn_revnum_t revision,
                                     svn_depth_t depth,
                                     const svn_delta_editor_t *wc_status,
                                     void *wc_status_baton,
                                     apr_pool_t *pool)
{
  return make_reporter(session,
                       reporter,
                       report_baton,
                       revision,
                       status_target,
                       NULL,
                       depth,
                       FALSE,
                       FALSE,
                       FALSE,
                       wc_status,
                       wc_status_baton,
                       FALSE, /* fetch_content */
                       TRUE, /* send_all */
                       FALSE, /* spool_response */
                       pool);
}


svn_error_t * svn_ra_neon__do_switch(svn_ra_session_t *session,
                                     const svn_ra_reporter3_t **reporter,
                                     void **report_baton,
                                     svn_revnum_t revision_to_update_to,
                                     const char *update_target,
                                     svn_depth_t depth,
                                     const char *switch_url,
                                     const svn_delta_editor_t *wc_update,
                                     void *wc_update_baton,
                                     apr_pool_t *pool)
{
  return make_reporter(session,
                       reporter,
                       report_baton,
                       revision_to_update_to,
                       update_target,
                       switch_url,
                       depth,
                       FALSE,  /* ### TODO(sussman): no copyfrom args */
                       TRUE,
                       FALSE, /* ### Disabled, pre-1.2 servers sometimes
                                 return incorrect resource-walk data */
                       wc_update,
                       wc_update_baton,
                       TRUE, /* fetch_content */
                       TRUE, /* send_all */
                       FALSE, /* spool_response */
                       pool);
}


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
                                   apr_pool_t *pool)
{
  return make_reporter(session,
                       reporter,
                       report_baton,
                       revision,
                       diff_target,
                       versus_url,
                       depth,
                       FALSE,
                       ignore_ancestry,
                       FALSE,
                       wc_diff,
                       wc_diff_baton,
                       text_deltas, /* fetch_content */
                       FALSE, /* send_all */
                       TRUE, /* spool_response */
                       pool);
}

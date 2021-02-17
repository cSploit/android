/*
 * main.c: Subversion dump stream filtering tool.
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


#include <stdlib.h>

#include <apr_file_io.h>

#include "svn_private_config.h"
#include "svn_cmdline.h"
#include "svn_error.h"
#include "svn_string.h"
#include "svn_opt.h"
#include "svn_utf.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_hash.h"
#include "svn_repos.h"
#include "svn_fs.h"
#include "svn_pools.h"
#include "svn_sorts.h"
#include "svn_props.h"
#include "svn_mergeinfo.h"
#include "svn_version.h"

#include "private/svn_mergeinfo_private.h"


/*** Code. ***/

/* Helper to open stdio streams */

/* NOTE: we used to call svn_stream_from_stdio(), which wraps a stream
   around a standard stdio.h FILE pointer.  The problem is that these
   pointers operate through C Run Time (CRT) on Win32, which does all
   sorts of translation on them: LF's become CRLF's, and ctrl-Z's
   embedded in Word documents are interpreted as premature EOF's.

   So instead, we use apr_file_open_std*, which bypass the CRT and
   directly wrap the OS's file-handles, which don't know or care about
   translation.  Thus dump/load works correctly on Win32.
*/
static svn_error_t *
create_stdio_stream(svn_stream_t **stream,
                    APR_DECLARE(apr_status_t) open_fn(apr_file_t **,
                                                      apr_pool_t *),
                    apr_pool_t *pool)
{
  apr_file_t *stdio_file;
  apr_status_t apr_err = open_fn(&stdio_file, pool);

  if (apr_err)
    return svn_error_wrap_apr(apr_err, _("Can't open stdio file"));

  *stream = svn_stream_from_aprfile2(stdio_file, TRUE, pool);
  return SVN_NO_ERROR;
}


/* Writes a property in dumpfile format to given stringbuf. */
static void
write_prop_to_stringbuf(svn_stringbuf_t **strbuf,
                        const char *name,
                        const svn_string_t *value)
{
  int bytes_used;
  size_t namelen;
  char buf[SVN_KEYLINE_MAXLEN];

  /* Output name length, then name. */
  namelen = strlen(name);
  svn_stringbuf_appendbytes(*strbuf, "K ", 2);

  bytes_used = apr_snprintf(buf, sizeof(buf), "%" APR_SIZE_T_FMT, namelen);
  svn_stringbuf_appendbytes(*strbuf, buf, bytes_used);
  svn_stringbuf_appendbyte(*strbuf, '\n');

  svn_stringbuf_appendbytes(*strbuf, name, namelen);
  svn_stringbuf_appendbyte(*strbuf, '\n');

  /* Output value length, then value. */
  svn_stringbuf_appendbytes(*strbuf, "V ", 2);

  bytes_used = apr_snprintf(buf, sizeof(buf), "%" APR_SIZE_T_FMT, value->len);
  svn_stringbuf_appendbytes(*strbuf, buf, bytes_used);
  svn_stringbuf_appendbyte(*strbuf, '\n');

  svn_stringbuf_appendbytes(*strbuf, value->data, value->len);
  svn_stringbuf_appendbyte(*strbuf, '\n');
}


/* Prefix matching function to compare node-path with set of prefixes. */
static svn_boolean_t
ary_prefix_match(const apr_array_header_t *pfxlist, const char *path)
{
  int i;
  size_t path_len = strlen(path);

  for (i = 0; i < pfxlist->nelts; i++)
    {
      const char *pfx = APR_ARRAY_IDX(pfxlist, i, const char *);
      size_t pfx_len = strlen(pfx);

      if (path_len < pfx_len)
        continue;
      if (strncmp(path, pfx, pfx_len) == 0
          && (path[pfx_len] == '\0' || path[pfx_len] == '/'))
        return TRUE;
    }

  return FALSE;
}


/* Check whether we need to skip this PATH based on its presence in
   the PREFIXES list, and the DO_EXCLUDE option. */
static APR_INLINE svn_boolean_t
skip_path(const char *path, const apr_array_header_t *prefixes,
          svn_boolean_t do_exclude, svn_boolean_t glob)
{
  const svn_boolean_t matches =
    (glob
     ? svn_cstring_match_glob_list(path, prefixes)
     : ary_prefix_match(prefixes, path));

  /* NXOR */
  return (matches ? do_exclude : !do_exclude);
}



/* Note: the input stream parser calls us with events.
   Output of the filtered dump occurs for the most part streamily with the
   event callbacks, to avoid caching large quantities of data in memory.
   The exceptions this are:
   - All revision data (headers and props) must be cached until a non-skipped
     node within the revision is found, or the revision is closed.
   - Node headers and props must be cached until all props have been received
     (to allow the Prop-content-length to be found). This is signalled either
     by the node text arriving, or the node being closed.
   The writing_begun members of the associated object batons track the state.
   output_revision() and output_node() are called to cause this flushing of
   cached data to occur.
*/


/* Filtering batons */

struct revmap_t
{
  svn_revnum_t rev; /* Last non-dropped revision to which this maps. */
  svn_boolean_t was_dropped; /* Was this revision dropped? */
};

struct parse_baton_t
{
  /* Command-line options values. */
  svn_boolean_t do_exclude;
  svn_boolean_t quiet;
  svn_boolean_t glob;
  svn_boolean_t drop_empty_revs;
  svn_boolean_t do_renumber_revs;
  svn_boolean_t preserve_revprops;
  svn_boolean_t skip_missing_merge_sources;
  apr_array_header_t *prefixes;

  /* Input and output streams. */
  svn_stream_t *in_stream;
  svn_stream_t *out_stream;

  /* State for the filtering process. */
  apr_int32_t rev_drop_count;
  apr_hash_t *dropped_nodes;
  apr_hash_t *renumber_history;  /* svn_revnum_t -> struct revmap_t */
  svn_revnum_t last_live_revision;
  /* The oldest original revision, greater than r0, in the input
     stream which was not filtered. */
  svn_revnum_t oldest_original_rev;
};

struct revision_baton_t
{
  /* Reference to the global parse baton. */
  struct parse_baton_t *pb;

  /* Does this revision have node or prop changes? */
  svn_boolean_t has_nodes;
  svn_boolean_t has_props;

  /* Did we drop any nodes? */
  svn_boolean_t had_dropped_nodes;

  /* Written to output stream? */
  svn_boolean_t writing_begun;

  /* The original and new (re-mapped) revision numbers. */
  svn_revnum_t rev_orig;
  svn_revnum_t rev_actual;

  /* Pointers to dumpfile data. */
  svn_stringbuf_t *header;
  apr_hash_t *props;
};

struct node_baton_t
{
  /* Reference to the current revision baton. */
  struct revision_baton_t *rb;

  /* Are we skipping this node? */
  svn_boolean_t do_skip;

  /* Have we been instructed to change or remove props on, or change
     the text of, this node? */
  svn_boolean_t has_props;
  svn_boolean_t has_text;

  /* Written to output stream? */
  svn_boolean_t writing_begun;

  /* The text content length according to the dumpfile headers, because we
     need the length before we have the actual text. */
  svn_filesize_t tcl;

  /* Pointers to dumpfile data. */
  svn_stringbuf_t *header;
  svn_stringbuf_t *props;
};



/* Filtering vtable members */

/* New revision: set up revision_baton, decide if we skip it. */
static svn_error_t *
new_revision_record(void **revision_baton,
                    apr_hash_t *headers,
                    void *parse_baton,
                    apr_pool_t *pool)
{
  struct revision_baton_t *rb;
  apr_hash_index_t *hi;
  const char *rev_orig;
  svn_stream_t *header_stream;

  *revision_baton = apr_palloc(pool, sizeof(struct revision_baton_t));
  rb = *revision_baton;
  rb->pb = parse_baton;
  rb->has_nodes = FALSE;
  rb->has_props = FALSE;
  rb->had_dropped_nodes = FALSE;
  rb->writing_begun = FALSE;
  rb->header = svn_stringbuf_create("", pool);
  rb->props = apr_hash_make(pool);

  header_stream = svn_stream_from_stringbuf(rb->header, pool);

  rev_orig = apr_hash_get(headers, SVN_REPOS_DUMPFILE_REVISION_NUMBER,
                          APR_HASH_KEY_STRING);
  rb->rev_orig = SVN_STR_TO_REV(rev_orig);

  if (rb->pb->do_renumber_revs)
    rb->rev_actual = rb->rev_orig - rb->pb->rev_drop_count;
  else
    rb->rev_actual = rb->rev_orig;

  SVN_ERR(svn_stream_printf(header_stream, pool,
                            SVN_REPOS_DUMPFILE_REVISION_NUMBER ": %ld\n",
                            rb->rev_actual));

  for (hi = apr_hash_first(pool, headers); hi; hi = apr_hash_next(hi))
    {
      const char *key = svn__apr_hash_index_key(hi);
      const char *val = svn__apr_hash_index_val(hi);

      if ((!strcmp(key, SVN_REPOS_DUMPFILE_CONTENT_LENGTH))
          || (!strcmp(key, SVN_REPOS_DUMPFILE_PROP_CONTENT_LENGTH))
          || (!strcmp(key, SVN_REPOS_DUMPFILE_REVISION_NUMBER)))
        continue;

      /* passthru: put header into header stringbuf. */

      SVN_ERR(svn_stream_printf(header_stream, pool, "%s: %s\n",
                                key, val));
    }

  SVN_ERR(svn_stream_close(header_stream));

  return SVN_NO_ERROR;
}


/* Output revision to dumpstream
   This may be called by new_node_record(), iff rb->has_nodes has been set
   to TRUE, or by close_revision() otherwise. This must only be called
   if rb->writing_begun is FALSE. */
static svn_error_t *
output_revision(struct revision_baton_t *rb)
{
  int bytes_used;
  char buf[SVN_KEYLINE_MAXLEN];
  apr_hash_index_t *hi;
  apr_pool_t *hash_pool = apr_hash_pool_get(rb->props);
  svn_stringbuf_t *props = svn_stringbuf_create("", hash_pool);
  apr_pool_t *subpool = svn_pool_create(hash_pool);

  rb->writing_begun = TRUE;

  /* If this revision has no nodes left because the ones it had were
     dropped, and we are not dropping empty revisions, and we were not
     told to preserve revision props, then we want to fixup the
     revision props to only contain:
       - the date
       - a log message that reports that this revision is just stuffing. */
  if ((! rb->pb->preserve_revprops)
      && (! rb->has_nodes)
      && rb->had_dropped_nodes
      && (! rb->pb->drop_empty_revs))
    {
      apr_hash_t *old_props = rb->props;
      rb->has_props = TRUE;
      rb->props = apr_hash_make(hash_pool);
      apr_hash_set(rb->props, SVN_PROP_REVISION_DATE, APR_HASH_KEY_STRING,
                   apr_hash_get(old_props, SVN_PROP_REVISION_DATE,
                                APR_HASH_KEY_STRING));
      apr_hash_set(rb->props, SVN_PROP_REVISION_LOG, APR_HASH_KEY_STRING,
                   svn_string_create(_("This is an empty revision for "
                                       "padding."), hash_pool));
    }

  /* Now, "rasterize" the props to a string, and append the property
     information to the header string.  */
  if (rb->has_props)
    {
      for (hi = apr_hash_first(subpool, rb->props);
           hi;
           hi = apr_hash_next(hi))
        {
          const char *pname = svn__apr_hash_index_key(hi);
          const svn_string_t *pval = svn__apr_hash_index_val(hi);

          write_prop_to_stringbuf(&props, pname, pval);
        }
      svn_stringbuf_appendcstr(props, "PROPS-END\n");
      svn_stringbuf_appendcstr(rb->header,
                               SVN_REPOS_DUMPFILE_PROP_CONTENT_LENGTH);
      bytes_used = apr_snprintf(buf, sizeof(buf), ": %" APR_SIZE_T_FMT,
                                props->len);
      svn_stringbuf_appendbytes(rb->header, buf, bytes_used);
      svn_stringbuf_appendbyte(rb->header, '\n');
    }

  svn_stringbuf_appendcstr(rb->header, SVN_REPOS_DUMPFILE_CONTENT_LENGTH);
  bytes_used = apr_snprintf(buf, sizeof(buf), ": %" APR_SIZE_T_FMT, props->len);
  svn_stringbuf_appendbytes(rb->header, buf, bytes_used);
  svn_stringbuf_appendbyte(rb->header, '\n');

  /* put an end to headers */
  svn_stringbuf_appendbyte(rb->header, '\n');

  /* put an end to revision */
  svn_stringbuf_appendbyte(props, '\n');

  /* write out the revision */
  /* Revision is written out in the following cases:
     1. No --drop-empty-revs has been supplied.
     2. --drop-empty-revs has been supplied,
     but revision has not all nodes dropped
     3. Revision had no nodes to begin with.
  */
  if (rb->has_nodes
      || (! rb->pb->drop_empty_revs)
      || (! rb->had_dropped_nodes))
    {
      /* This revision is a keeper. */
      SVN_ERR(svn_stream_write(rb->pb->out_stream,
                               rb->header->data, &(rb->header->len)));
      SVN_ERR(svn_stream_write(rb->pb->out_stream,
                               props->data, &(props->len)));

      /* Stash the oldest original rev not dropped. */
      if (rb->rev_orig > 0
          && !SVN_IS_VALID_REVNUM(rb->pb->oldest_original_rev))
        rb->pb->oldest_original_rev = rb->rev_orig;

      if (rb->pb->do_renumber_revs)
        {
          svn_revnum_t *rr_key;
          struct revmap_t *rr_val;
          apr_pool_t *rr_pool = apr_hash_pool_get(rb->pb->renumber_history);
          rr_key = apr_palloc(rr_pool, sizeof(*rr_key));
          rr_val = apr_palloc(rr_pool, sizeof(*rr_val));
          *rr_key = rb->rev_orig;
          rr_val->rev = rb->rev_actual;
          rr_val->was_dropped = FALSE;
          apr_hash_set(rb->pb->renumber_history, rr_key,
                       sizeof(*rr_key), rr_val);
          rb->pb->last_live_revision = rb->rev_actual;
        }

      if (! rb->pb->quiet)
        SVN_ERR(svn_cmdline_fprintf(stderr, subpool,
                                    _("Revision %ld committed as %ld.\n"),
                                    rb->rev_orig, rb->rev_actual));
    }
  else
    {
      /* We're dropping this revision. */
      rb->pb->rev_drop_count++;
      if (rb->pb->do_renumber_revs)
        {
          svn_revnum_t *rr_key;
          struct revmap_t *rr_val;
          apr_pool_t *rr_pool = apr_hash_pool_get(rb->pb->renumber_history);
          rr_key = apr_palloc(rr_pool, sizeof(*rr_key));
          rr_val = apr_palloc(rr_pool, sizeof(*rr_val));
          *rr_key = rb->rev_orig;
          rr_val->rev = rb->pb->last_live_revision;
          rr_val->was_dropped = TRUE;
          apr_hash_set(rb->pb->renumber_history, rr_key,
                       sizeof(*rr_key), rr_val);
        }

      if (! rb->pb->quiet)
        SVN_ERR(svn_cmdline_fprintf(stderr, subpool,
                                    _("Revision %ld skipped.\n"),
                                    rb->rev_orig));
    }
  svn_pool_destroy(subpool);
  return SVN_NO_ERROR;
}


/* UUID record here: dump it, as we do not filter them. */
static svn_error_t *
uuid_record(const char *uuid, void *parse_baton, apr_pool_t *pool)
{
  struct parse_baton_t *pb = parse_baton;
  SVN_ERR(svn_stream_printf(pb->out_stream, pool,
                            SVN_REPOS_DUMPFILE_UUID ": %s\n\n", uuid));
  return SVN_NO_ERROR;
}


/* New node here. Set up node_baton by copying headers. */
static svn_error_t *
new_node_record(void **node_baton,
                apr_hash_t *headers,
                void *rev_baton,
                apr_pool_t *pool)
{
  struct parse_baton_t *pb;
  struct node_baton_t *nb;
  char *node_path, *copyfrom_path;
  apr_hash_index_t *hi;
  const char *tcl;

  *node_baton = apr_palloc(pool, sizeof(struct node_baton_t));
  nb          = *node_baton;
  nb->rb      = rev_baton;
  pb          = nb->rb->pb;

  node_path = apr_hash_get(headers, SVN_REPOS_DUMPFILE_NODE_PATH,
                           APR_HASH_KEY_STRING);
  copyfrom_path = apr_hash_get(headers,
                               SVN_REPOS_DUMPFILE_NODE_COPYFROM_PATH,
                               APR_HASH_KEY_STRING);

  /* Ensure that paths start with a leading '/'. */
  if (node_path[0] != '/')
    node_path = apr_pstrcat(pool, "/", node_path, (char *)NULL);
  if (copyfrom_path && copyfrom_path[0] != '/')
    copyfrom_path = apr_pstrcat(pool, "/", copyfrom_path, (char *)NULL);

  nb->do_skip = skip_path(node_path, pb->prefixes,
                          pb->do_exclude, pb->glob);

  /* If we're skipping the node, take note of path, discarding the
     rest.  */
  if (nb->do_skip)
    {
      apr_hash_set(pb->dropped_nodes,
                   apr_pstrdup(apr_hash_pool_get(pb->dropped_nodes),
                               node_path),
                   APR_HASH_KEY_STRING, (void *)1);
      nb->rb->had_dropped_nodes = TRUE;
    }
  else
    {
      tcl = apr_hash_get(headers, SVN_REPOS_DUMPFILE_TEXT_CONTENT_LENGTH,
                         APR_HASH_KEY_STRING);

      /* Test if this node was copied from dropped source. */
      if (copyfrom_path &&
          skip_path(copyfrom_path, pb->prefixes, pb->do_exclude, pb->glob))
        {
          /* This node was copied from a dropped source.
             We have a problem, since we did not want to drop this node too.

             However, there is one special case we'll handle.  If the node is
             a file, and this was a copy-and-modify operation, then the
             dumpfile should contain the new contents of the file.  In this
             scenario, we'll just do an add without history using the new
             contents.  */
          const char *kind;
          kind = apr_hash_get(headers, SVN_REPOS_DUMPFILE_NODE_KIND,
                              APR_HASH_KEY_STRING);

          /* If there is a Text-content-length header, and the kind is
             "file", we just fallback to an add without history. */
          if (tcl && (strcmp(kind, "file") == 0))
            {
              apr_hash_set(headers, SVN_REPOS_DUMPFILE_NODE_COPYFROM_PATH,
                           APR_HASH_KEY_STRING, NULL);
              apr_hash_set(headers, SVN_REPOS_DUMPFILE_NODE_COPYFROM_REV,
                           APR_HASH_KEY_STRING, NULL);
              copyfrom_path = NULL;
            }
          /* Else, this is either a directory or a file whose contents we
             don't have readily available.  */
          else
            {
              return svn_error_createf
                (SVN_ERR_INCOMPLETE_DATA, 0,
                 _("Invalid copy source path '%s'"), copyfrom_path);
            }
        }

      nb->has_props = FALSE;
      nb->has_text = FALSE;
      nb->writing_begun = FALSE;
      nb->tcl = tcl ? svn__atoui64(tcl) : 0;
      nb->header = svn_stringbuf_create("", pool);
      nb->props = svn_stringbuf_create("", pool);

      /* Now we know for sure that we have a node that will not be
         skipped, flush the revision if it has not already been done. */
      nb->rb->has_nodes = TRUE;
      if (! nb->rb->writing_begun)
        SVN_ERR(output_revision(nb->rb));

      for (hi = apr_hash_first(pool, headers); hi; hi = apr_hash_next(hi))
        {
          const char *key = svn__apr_hash_index_key(hi);
          const char *val = svn__apr_hash_index_val(hi);

          if ((!strcmp(key, SVN_REPOS_DUMPFILE_CONTENT_LENGTH))
              || (!strcmp(key, SVN_REPOS_DUMPFILE_PROP_CONTENT_LENGTH))
              || (!strcmp(key, SVN_REPOS_DUMPFILE_TEXT_CONTENT_LENGTH)))
            continue;

          /* Rewrite Node-Copyfrom-Rev if we are renumbering revisions.
             The number points to some revision in the past. We keep track
             of revision renumbering in an apr_hash, which maps original
             revisions to new ones. Dropped revision are mapped to -1.
             This should never happen here.
          */
          if (pb->do_renumber_revs
              && (!strcmp(key, SVN_REPOS_DUMPFILE_NODE_COPYFROM_REV)))
            {
              svn_revnum_t cf_orig_rev;
              struct revmap_t *cf_renum_val;

              cf_orig_rev = SVN_STR_TO_REV(val);
              cf_renum_val = apr_hash_get(pb->renumber_history,
                                          &cf_orig_rev,
                                          sizeof(svn_revnum_t));
              if (! (cf_renum_val && SVN_IS_VALID_REVNUM(cf_renum_val->rev)))
                return svn_error_createf
                  (SVN_ERR_NODE_UNEXPECTED_KIND, NULL,
                   _("No valid copyfrom revision in filtered stream"));
              SVN_ERR(svn_stream_printf
                      (nb->rb->pb->out_stream, pool,
                       SVN_REPOS_DUMPFILE_NODE_COPYFROM_REV ": %ld\n",
                       cf_renum_val->rev));
              continue;
            }

          /* passthru: put header straight to output */

          SVN_ERR(svn_stream_printf(nb->rb->pb->out_stream,
                                    pool, "%s: %s\n",
                                    key, val));
        }
    }

  return SVN_NO_ERROR;
}


/* Output node header and props to dumpstream
   This will be called by set_fulltext() after setting nb->has_text to TRUE,
   if the node has any text, or by close_node() otherwise. This must only
   be called if nb->writing_begun is FALSE. */
static svn_error_t *
output_node(struct node_baton_t *nb)
{
  int bytes_used;
  char buf[SVN_KEYLINE_MAXLEN];

  nb->writing_begun = TRUE;

  /* when there are no props nb->props->len would be zero and won't mess up
     Content-Length. */
  if (nb->has_props)
    svn_stringbuf_appendcstr(nb->props, "PROPS-END\n");

  /* 1. recalculate & check text-md5 if present. Passed through right now. */

  /* 2. recalculate and add content-lengths */

  if (nb->has_props)
    {
      svn_stringbuf_appendcstr(nb->header,
                               SVN_REPOS_DUMPFILE_PROP_CONTENT_LENGTH);
      bytes_used = apr_snprintf(buf, sizeof(buf), ": %" APR_SIZE_T_FMT,
                                nb->props->len);
      svn_stringbuf_appendbytes(nb->header, buf, bytes_used);
      svn_stringbuf_appendbyte(nb->header, '\n');
    }
  if (nb->has_text)
    {
      svn_stringbuf_appendcstr(nb->header,
                               SVN_REPOS_DUMPFILE_TEXT_CONTENT_LENGTH);
      bytes_used = apr_snprintf(buf, sizeof(buf), ": %" SVN_FILESIZE_T_FMT,
                                nb->tcl);
      svn_stringbuf_appendbytes(nb->header, buf, bytes_used);
      svn_stringbuf_appendbyte(nb->header, '\n');
    }
  svn_stringbuf_appendcstr(nb->header, SVN_REPOS_DUMPFILE_CONTENT_LENGTH);
  bytes_used = apr_snprintf(buf, sizeof(buf), ": %" SVN_FILESIZE_T_FMT,
                            (svn_filesize_t) (nb->props->len + nb->tcl));
  svn_stringbuf_appendbytes(nb->header, buf, bytes_used);
  svn_stringbuf_appendbyte(nb->header, '\n');

  /* put an end to headers */
  svn_stringbuf_appendbyte(nb->header, '\n');

  /* 3. output all the stuff */

  SVN_ERR(svn_stream_write(nb->rb->pb->out_stream,
                           nb->header->data , &(nb->header->len)));
  SVN_ERR(svn_stream_write(nb->rb->pb->out_stream,
                           nb->props->data , &(nb->props->len)));

  return SVN_NO_ERROR;
}


/* Examine the mergeinfo in INITIAL_VAL, omitting missing merge
   sources or renumbering revisions in rangelists as appropriate, and
   return the (possibly new) mergeinfo in *FINAL_VAL (allocated from
   POOL). */
static svn_error_t *
adjust_mergeinfo(svn_string_t **final_val, const svn_string_t *initial_val,
                 struct revision_baton_t *rb, apr_pool_t *pool)
{
  apr_hash_t *mergeinfo;
  apr_hash_t *final_mergeinfo = apr_hash_make(pool);
  apr_hash_index_t *hi;
  apr_pool_t *subpool = svn_pool_create(pool);

  SVN_ERR(svn_mergeinfo_parse(&mergeinfo, initial_val->data, subpool));

  /* Issue #3020: If we are skipping missing merge sources, then also
     filter mergeinfo ranges as old or older than the oldest revision in the
     dump stream.  Those older than the oldest obviously refer to history
     outside of the dump stream.  The oldest rev itself is present in the
     dump, but cannot be a valid merge source revision since it is the
     start of all history.  E.g. if we dump -r100:400 then dumpfilter the
     result with --skip-missing-merge-sources, any mergeinfo with revision
     100 implies a change of -r99:100, but r99 is part of the history we
     want filtered.  This is analogous to how r1 is always meaningless as
     a merge source revision.

     If the oldest rev is r0 then there is nothing to filter. */
  if (rb->pb->skip_missing_merge_sources && rb->pb->oldest_original_rev > 0)
    SVN_ERR(svn_mergeinfo__filter_mergeinfo_by_ranges(
      &mergeinfo, mergeinfo,
      rb->pb->oldest_original_rev, 0,
      FALSE, subpool, subpool));

  for (hi = apr_hash_first(subpool, mergeinfo); hi; hi = apr_hash_next(hi))
    {
      const char *merge_source = svn__apr_hash_index_key(hi);
      apr_array_header_t *rangelist = svn__apr_hash_index_val(hi);
      struct parse_baton_t *pb = rb->pb;

      /* Determine whether the merge_source is a part of the prefix. */
      if (skip_path(merge_source, pb->prefixes, pb->do_exclude, pb->glob))
        {
          if (pb->skip_missing_merge_sources)
            continue;
          else
            return svn_error_createf(SVN_ERR_INCOMPLETE_DATA, 0,
                                     _("Missing merge source path '%s'; try "
                                       "with --skip-missing-merge-sources"),
                                     merge_source);
        }

      /* Possibly renumber revisions in merge source's rangelist. */
      if (pb->do_renumber_revs)
        {
          int i;

          for (i = 0; i < rangelist->nelts; i++)
            {
              struct revmap_t *revmap_start;
              struct revmap_t *revmap_end;
              svn_merge_range_t *range = APR_ARRAY_IDX(rangelist, i,
                                                       svn_merge_range_t *);

              revmap_start = apr_hash_get(pb->renumber_history,
                                          &range->start, sizeof(svn_revnum_t));
              if (! (revmap_start && SVN_IS_VALID_REVNUM(revmap_start->rev)))
                return svn_error_createf
                  (SVN_ERR_NODE_UNEXPECTED_KIND, NULL,
                   _("No valid revision range 'start' in filtered stream"));

              revmap_end = apr_hash_get(pb->renumber_history,
                                        &range->end, sizeof(svn_revnum_t));
              if (! (revmap_end && SVN_IS_VALID_REVNUM(revmap_end->rev)))
                return svn_error_createf
                  (SVN_ERR_NODE_UNEXPECTED_KIND, NULL,
                   _("No valid revision range 'end' in filtered stream"));

              range->start = revmap_start->rev;
              range->end = revmap_end->rev;
            }
        }
      apr_hash_set(final_mergeinfo, merge_source,
                   APR_HASH_KEY_STRING, rangelist);
    }

  SVN_ERR(svn_mergeinfo_sort(final_mergeinfo, subpool));
  SVN_ERR(svn_mergeinfo_to_string(final_val, final_mergeinfo, pool));
  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


static svn_error_t *
set_revision_property(void *revision_baton,
                      const char *name,
                      const svn_string_t *value)
{
  struct revision_baton_t *rb = revision_baton;
  apr_pool_t *hash_pool = apr_hash_pool_get(rb->props);

  rb->has_props = TRUE;
  apr_hash_set(rb->props, apr_pstrdup(hash_pool, name),
               APR_HASH_KEY_STRING, svn_string_dup(value, hash_pool));
  return SVN_NO_ERROR;
}


static svn_error_t *
set_node_property(void *node_baton,
                  const char *name,
                  const svn_string_t *value)
{
  struct node_baton_t *nb = node_baton;
  struct revision_baton_t *rb = nb->rb;

  if (nb->do_skip)
    return SVN_NO_ERROR;

  if (!nb->has_props)
    return svn_error_create(SVN_ERR_UNSUPPORTED_FEATURE, NULL,
                            _("Delta property block detected - "
                              "not supported by svndumpfilter"));

  if (strcmp(name, SVN_PROP_MERGEINFO) == 0)
    {
      svn_string_t *filtered_mergeinfo;  /* Avoid compiler warning. */
      apr_pool_t *pool = apr_hash_pool_get(rb->props);
      SVN_ERR(adjust_mergeinfo(&filtered_mergeinfo, value, rb, pool));
      value = filtered_mergeinfo;
    }

  write_prop_to_stringbuf(&(nb->props), name, value);

  return SVN_NO_ERROR;
}


static svn_error_t *
remove_node_props(void *node_baton)
{
  struct node_baton_t *nb = node_baton;

  /* In this case, not actually indicating that the node *has* props,
     rather that we know about all the props that it has, since it now
     has none. */
  nb->has_props = TRUE;

  return SVN_NO_ERROR;
}


static svn_error_t *
set_fulltext(svn_stream_t **stream, void *node_baton)
{
  struct node_baton_t *nb = node_baton;

  if (!nb->do_skip)
    {
      nb->has_text = TRUE;
      if (! nb->writing_begun)
        SVN_ERR(output_node(nb));
      *stream = nb->rb->pb->out_stream;
    }

  return SVN_NO_ERROR;
}


/* Finalize node */
static svn_error_t *
close_node(void *node_baton)
{
  struct node_baton_t *nb = node_baton;
  apr_size_t len = 2;

  /* Get out of here if we can. */
  if (nb->do_skip)
    return SVN_NO_ERROR;

  /* If the node was not flushed already to output its text, do it now. */
  if (! nb->writing_begun)
    SVN_ERR(output_node(nb));

  /* put an end to node. */
  SVN_ERR(svn_stream_write(nb->rb->pb->out_stream, "\n\n", &len));

  return SVN_NO_ERROR;
}


/* Finalize revision */
static svn_error_t *
close_revision(void *revision_baton)
{
  struct revision_baton_t *rb = revision_baton;

  /* If no node has yet flushed the revision, do it now. */
  if (! rb->writing_begun)
    return output_revision(rb);
  else
    return SVN_NO_ERROR;
}


/* Filtering vtable */
svn_repos_parse_fns2_t filtering_vtable =
  {
    new_revision_record,
    uuid_record,
    new_node_record,
    set_revision_property,
    set_node_property,
    NULL,
    remove_node_props,
    set_fulltext,
    NULL,
    close_node,
    close_revision
  };



/** Subcommands. **/

static svn_opt_subcommand_t
  subcommand_help,
  subcommand_exclude,
  subcommand_include;

enum
  {
    svndumpfilter__drop_empty_revs = SVN_OPT_FIRST_LONGOPT_ID,
    svndumpfilter__renumber_revs,
    svndumpfilter__preserve_revprops,
    svndumpfilter__skip_missing_merge_sources,
    svndumpfilter__targets,
    svndumpfilter__quiet,
    svndumpfilter__glob,
    svndumpfilter__version
  };

/* Option codes and descriptions.
 *
 * The entire list must be terminated with an entry of nulls.
 */
static const apr_getopt_option_t options_table[] =
  {
    {"help",          'h', 0,
     N_("show help on a subcommand")},

    {NULL,            '?', 0,
     N_("show help on a subcommand")},

    {"version",            svndumpfilter__version, 0,
     N_("show program version information") },
    {"quiet",              svndumpfilter__quiet, 0,
     N_("Do not display filtering statistics.") },
    {"pattern",            svndumpfilter__glob, 0,
     N_("Treat the path prefixes as file glob patterns.") },
    {"drop-empty-revs",    svndumpfilter__drop_empty_revs, 0,
     N_("Remove revisions emptied by filtering.")},
    {"renumber-revs",      svndumpfilter__renumber_revs, 0,
     N_("Renumber revisions left after filtering.") },
    {"skip-missing-merge-sources",
     svndumpfilter__skip_missing_merge_sources, 0,
     N_("Skip missing merge sources.") },
    {"preserve-revprops",  svndumpfilter__preserve_revprops, 0,
     N_("Don't filter revision properties.") },
    {"targets", svndumpfilter__targets, 1,
     N_("Read additional prefixes, one per line, from\n"
        "                             file ARG.")},
    {NULL}
  };


/* Array of available subcommands.
 * The entire list must be terminated with an entry of nulls.
 */
static const svn_opt_subcommand_desc2_t cmd_table[] =
  {
    {"exclude", subcommand_exclude, {0},
     N_("Filter out nodes with given prefixes from dumpstream.\n"
        "usage: svndumpfilter exclude PATH_PREFIX...\n"),
     {svndumpfilter__drop_empty_revs, svndumpfilter__renumber_revs,
      svndumpfilter__skip_missing_merge_sources, svndumpfilter__targets,
      svndumpfilter__preserve_revprops, svndumpfilter__quiet,
      svndumpfilter__glob} },

    {"include", subcommand_include, {0},
     N_("Filter out nodes without given prefixes from dumpstream.\n"
        "usage: svndumpfilter include PATH_PREFIX...\n"),
     {svndumpfilter__drop_empty_revs, svndumpfilter__renumber_revs,
      svndumpfilter__skip_missing_merge_sources, svndumpfilter__targets,
      svndumpfilter__preserve_revprops, svndumpfilter__quiet,
      svndumpfilter__glob} },

    {"help", subcommand_help, {"?", "h"},
     N_("Describe the usage of this program or its subcommands.\n"
        "usage: svndumpfilter help [SUBCOMMAND...]\n"),
     {0} },

    { NULL, NULL, {0}, NULL, {0} }
  };


/* Baton for passing option/argument state to a subcommand function. */
struct svndumpfilter_opt_state
{
  svn_opt_revision_t start_revision;     /* -r X[:Y] is         */
  svn_opt_revision_t end_revision;       /* not implemented.    */
  svn_boolean_t quiet;                   /* --quiet             */
  svn_boolean_t glob;                    /* --pattern           */
  svn_boolean_t version;                 /* --version           */
  svn_boolean_t drop_empty_revs;         /* --drop-empty-revs   */
  svn_boolean_t help;                    /* --help or -?        */
  svn_boolean_t renumber_revs;           /* --renumber-revs     */
  svn_boolean_t preserve_revprops;       /* --preserve-revprops */
  svn_boolean_t skip_missing_merge_sources;
                                         /* --skip-missing-merge-sources */
  const char *targets_file;              /* --targets-file       */
  apr_array_header_t *prefixes;          /* mainargs.           */
};


static svn_error_t *
parse_baton_initialize(struct parse_baton_t **pb,
                       struct svndumpfilter_opt_state *opt_state,
                       svn_boolean_t do_exclude,
                       apr_pool_t *pool)
{
  struct parse_baton_t *baton = apr_palloc(pool, sizeof(*baton));

  /* Read the stream from STDIN.  Users can redirect a file. */
  SVN_ERR(create_stdio_stream(&(baton->in_stream),
                              apr_file_open_stdin, pool));

  /* Have the parser dump results to STDOUT. Users can redirect a file. */
  SVN_ERR(create_stdio_stream(&(baton->out_stream),
                              apr_file_open_stdout, pool));

  baton->do_exclude = do_exclude;

  /* Ignore --renumber-revs if there can't possibly be
     anything to renumber. */
  baton->do_renumber_revs =
    (opt_state->renumber_revs && opt_state->drop_empty_revs);

  baton->drop_empty_revs = opt_state->drop_empty_revs;
  baton->preserve_revprops = opt_state->preserve_revprops;
  baton->quiet = opt_state->quiet;
  baton->glob = opt_state->glob;
  baton->prefixes = opt_state->prefixes;
  baton->skip_missing_merge_sources = opt_state->skip_missing_merge_sources;
  baton->rev_drop_count = 0; /* used to shift revnums while filtering */
  baton->dropped_nodes = apr_hash_make(pool);
  baton->renumber_history = apr_hash_make(pool);
  baton->last_live_revision = SVN_INVALID_REVNUM;
  baton->oldest_original_rev = SVN_INVALID_REVNUM;

  /* This is non-ideal: We should pass through the version of the
   * input dumpstream.  However, our API currently doesn't allow that.
   * Hardcoding version 2 is acceptable because:
   *   - We currently do not accept version 3 or greater.
   *   - Dumpstream version 1 is so ancient as to be ignorable
   *     (0.17.x and earlier)
   */
  SVN_ERR(svn_stream_printf(baton->out_stream, pool,
                            SVN_REPOS_DUMPFILE_MAGIC_HEADER ": %d\n\n",
                            2));

  *pb = baton;
  return SVN_NO_ERROR;
}

/* This implements `help` subcommand. */
static svn_error_t *
subcommand_help(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  struct svndumpfilter_opt_state *opt_state = baton;
  const char *header =
    _("general usage: svndumpfilter SUBCOMMAND [ARGS & OPTIONS ...]\n"
      "Type 'svndumpfilter help <subcommand>' for help on a "
      "specific subcommand.\n"
      "Type 'svndumpfilter --version' to see the program version.\n"
      "\n"
      "Available subcommands:\n");

  SVN_ERR(svn_opt_print_help3(os, "svndumpfilter",
                              opt_state ? opt_state->version : FALSE,
                              opt_state ? opt_state->quiet : FALSE, NULL,
                              header, cmd_table, options_table, NULL,
                              NULL, pool));

  return SVN_NO_ERROR;
}


/* Version compatibility check */
static svn_error_t *
check_lib_versions(void)
{
  static const svn_version_checklist_t checklist[] =
    {
      { "svn_subr",  svn_subr_version },
      { "svn_repos", svn_repos_version },
      { "svn_delta", svn_delta_version },
      { NULL, NULL }
    };

  SVN_VERSION_DEFINE(my_version);
  return svn_ver_check_list(&my_version, checklist);
}


/* Do the real work of filtering. */
static svn_error_t *
do_filter(apr_getopt_t *os,
          void *baton,
          svn_boolean_t do_exclude,
          apr_pool_t *pool)
{
  struct svndumpfilter_opt_state *opt_state = baton;
  struct parse_baton_t *pb;
  apr_hash_index_t *hi;
  apr_array_header_t *keys;
  int i, num_keys;

  if (! opt_state->quiet)
    {
      apr_pool_t *subpool = svn_pool_create(pool);

      if (opt_state->glob)
        {
          SVN_ERR(svn_cmdline_fprintf(stderr, subpool,
                                      do_exclude
                                      ? opt_state->drop_empty_revs
                                      ? _("Excluding (and dropping empty "
                                          "revisions for) prefix patterns:\n")
                                      : _("Excluding prefix patterns:\n")
                                      : opt_state->drop_empty_revs
                                      ? _("Including (and dropping empty "
                                          "revisions for) prefix patterns:\n")
                                      : _("Including prefix patterns:\n")));
        }
      else
        {
          SVN_ERR(svn_cmdline_fprintf(stderr, subpool,
                                      do_exclude
                                      ? opt_state->drop_empty_revs
                                      ? _("Excluding (and dropping empty "
                                          "revisions for) prefixes:\n")
                                      : _("Excluding prefixes:\n")
                                      : opt_state->drop_empty_revs
                                      ? _("Including (and dropping empty "
                                          "revisions for) prefixes:\n")
                                      : _("Including prefixes:\n")));
        }

      for (i = 0; i < opt_state->prefixes->nelts; i++)
        {
          svn_pool_clear(subpool);
          SVN_ERR(svn_cmdline_fprintf
                  (stderr, subpool, "   '%s'\n",
                   APR_ARRAY_IDX(opt_state->prefixes, i, const char *)));
        }

      SVN_ERR(svn_cmdline_fputs("\n", stderr, subpool));
      svn_pool_destroy(subpool);
    }

  SVN_ERR(parse_baton_initialize(&pb, opt_state, do_exclude, pool));
  SVN_ERR(svn_repos_parse_dumpstream2(pb->in_stream, &filtering_vtable, pb,
                                      NULL, NULL, pool));

  /* The rest of this is just reporting.  If we aren't reporting, get
     outta here. */
  if (opt_state->quiet)
    return SVN_NO_ERROR;

  SVN_ERR(svn_cmdline_fputs("\n", stderr, pool));

  if (pb->rev_drop_count)
    SVN_ERR(svn_cmdline_fprintf(stderr, pool,
                                Q_("Dropped %d revision.\n\n",
                                   "Dropped %d revisions.\n\n",
                                   pb->rev_drop_count),
                                pb->rev_drop_count));

  if (pb->do_renumber_revs)
    {
      apr_pool_t *subpool = svn_pool_create(pool);
      SVN_ERR(svn_cmdline_fputs(_("Revisions renumbered as follows:\n"),
                                stderr, subpool));

      /* Get the keys of the hash, sort them, then print the hash keys
         and values, sorted by keys. */
      num_keys = apr_hash_count(pb->renumber_history);
      keys = apr_array_make(pool, num_keys + 1, sizeof(svn_revnum_t));
      for (hi = apr_hash_first(pool, pb->renumber_history);
           hi;
           hi = apr_hash_next(hi))
        {
          const svn_revnum_t *revnum = svn__apr_hash_index_key(hi);

          APR_ARRAY_PUSH(keys, svn_revnum_t) = *revnum;
        }
      qsort(keys->elts, keys->nelts,
            keys->elt_size, svn_sort_compare_revisions);
      for (i = 0; i < keys->nelts; i++)
        {
          svn_revnum_t this_key;
          struct revmap_t *this_val;

          svn_pool_clear(subpool);
          this_key = APR_ARRAY_IDX(keys, i, svn_revnum_t);
          this_val = apr_hash_get(pb->renumber_history, &this_key,
                                  sizeof(this_key));
          if (this_val->was_dropped)
            SVN_ERR(svn_cmdline_fprintf(stderr, subpool,
                                        _("   %ld => (dropped)\n"),
                                        this_key));
          else
            SVN_ERR(svn_cmdline_fprintf(stderr, subpool,
                                        "   %ld => %ld\n",
                                        this_key, this_val->rev));
        }
      SVN_ERR(svn_cmdline_fputs("\n", stderr, subpool));
      svn_pool_destroy(subpool);
    }

  if ((num_keys = apr_hash_count(pb->dropped_nodes)))
    {
      apr_pool_t *subpool = svn_pool_create(pool);
      SVN_ERR(svn_cmdline_fprintf(stderr, subpool,
                                  Q_("Dropped %d node:\n",
                                     "Dropped %d nodes:\n",
                                     num_keys),
                                  num_keys));

      /* Get the keys of the hash, sort them, then print the hash keys
         and values, sorted by keys. */
      keys = apr_array_make(pool, num_keys + 1, sizeof(const char *));
      for (hi = apr_hash_first(pool, pb->dropped_nodes);
           hi;
           hi = apr_hash_next(hi))
        {
          const char *path = svn__apr_hash_index_key(hi);

          APR_ARRAY_PUSH(keys, const char *) = path;
        }
      qsort(keys->elts, keys->nelts, keys->elt_size, svn_sort_compare_paths);
      for (i = 0; i < keys->nelts; i++)
        {
          svn_pool_clear(subpool);
          SVN_ERR(svn_cmdline_fprintf
                  (stderr, subpool, "   '%s'\n",
                   (const char *)APR_ARRAY_IDX(keys, i, const char *)));
        }
      SVN_ERR(svn_cmdline_fputs("\n", stderr, subpool));
      svn_pool_destroy(subpool);
    }

  return SVN_NO_ERROR;
}

/* This implements `exclude' subcommand. */
static svn_error_t *
subcommand_exclude(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  return do_filter(os, baton, TRUE, pool);
}


/* This implements `include` subcommand. */
static svn_error_t *
subcommand_include(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  return do_filter(os, baton, FALSE, pool);
}



/** Main. **/

int
main(int argc, const char *argv[])
{
  svn_error_t *err;
  apr_status_t apr_err;
  apr_allocator_t *allocator;
  apr_pool_t *pool;

  const svn_opt_subcommand_desc2_t *subcommand = NULL;
  struct svndumpfilter_opt_state opt_state;
  apr_getopt_t *os;
  int opt_id;
  apr_array_header_t *received_opts;
  int i;


  /* Initialize the app. */
  if (svn_cmdline_init("svndumpfilter", stderr) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  /* Create our top-level pool.  Use a separate mutexless allocator,
   * given this application is single threaded.
   */
  if (apr_allocator_create(&allocator))
   return EXIT_FAILURE;

  apr_allocator_max_free_set(allocator, SVN_ALLOCATOR_RECOMMENDED_MAX_FREE);

  pool = svn_pool_create_ex(NULL, allocator);
  apr_allocator_owner_set(allocator, pool);

  /* Check library versions */
  err = check_lib_versions();
  if (err)
    return svn_cmdline_handle_exit_error(err, pool, "svndumpfilter: ");

  received_opts = apr_array_make(pool, SVN_OPT_MAX_OPTIONS, sizeof(int));

  /* Initialize the FS library. */
  err = svn_fs_initialize(pool);
  if (err)
    return svn_cmdline_handle_exit_error(err, pool, "svndumpfilter: ");

  if (argc <= 1)
    {
      SVN_INT_ERR(subcommand_help(NULL, NULL, pool));
      svn_pool_destroy(pool);
      return EXIT_FAILURE;
    }

  /* Initialize opt_state. */
  memset(&opt_state, 0, sizeof(opt_state));
  opt_state.start_revision.kind = svn_opt_revision_unspecified;
  opt_state.end_revision.kind = svn_opt_revision_unspecified;

  /* Parse options. */
  err = svn_cmdline__getopt_init(&os, argc, argv, pool);
  if (err)
    return svn_cmdline_handle_exit_error(err, pool, "svndumpfilter: ");

  os->interleave = 1;
  while (1)
    {
      const char *opt_arg;

      /* Parse the next option. */
      apr_err = apr_getopt_long(os, options_table, &opt_id, &opt_arg);
      if (APR_STATUS_IS_EOF(apr_err))
        break;
      else if (apr_err)
        {
          SVN_INT_ERR(subcommand_help(NULL, NULL, pool));
          svn_pool_destroy(pool);
          return EXIT_FAILURE;
        }

      /* Stash the option code in an array before parsing it. */
      APR_ARRAY_PUSH(received_opts, int) = opt_id;

      switch (opt_id)
        {
        case 'h':
        case '?':
          opt_state.help = TRUE;
          break;
        case svndumpfilter__version:
          opt_state.version = TRUE;
          break;
        case svndumpfilter__quiet:
          opt_state.quiet = TRUE;
          break;
        case svndumpfilter__glob:
          opt_state.glob = TRUE;
          break;
        case svndumpfilter__drop_empty_revs:
          opt_state.drop_empty_revs = TRUE;
          break;
        case svndumpfilter__renumber_revs:
          opt_state.renumber_revs = TRUE;
          break;
        case svndumpfilter__preserve_revprops:
          opt_state.preserve_revprops = TRUE;
          break;
        case svndumpfilter__skip_missing_merge_sources:
          opt_state.skip_missing_merge_sources = TRUE;
          break;
        case svndumpfilter__targets:
          opt_state.targets_file = opt_arg;
          break;
        default:
          {
            SVN_INT_ERR(subcommand_help(NULL, NULL, pool));
            svn_pool_destroy(pool);
            return EXIT_FAILURE;
          }
        }  /* close `switch' */
    }  /* close `while' */

  /* If the user asked for help, then the rest of the arguments are
     the names of subcommands to get help on (if any), or else they're
     just typos/mistakes.  Whatever the case, the subcommand to
     actually run is subcommand_help(). */
  if (opt_state.help)
    subcommand = svn_opt_get_canonical_subcommand2(cmd_table, "help");

  /* If we're not running the `help' subcommand, then look for a
     subcommand in the first argument. */
  if (subcommand == NULL)
    {
      if (os->ind >= os->argc)
        {
          if (opt_state.version)
            {
              /* Use the "help" subcommand to handle the "--version" option. */
              static const svn_opt_subcommand_desc2_t pseudo_cmd =
                { "--version", subcommand_help, {0}, "",
                  {svndumpfilter__version,  /* must accept its own option */
                   svndumpfilter__quiet,
                  } };

              subcommand = &pseudo_cmd;
            }
          else
            {
              svn_error_clear(svn_cmdline_fprintf
                              (stderr, pool,
                               _("Subcommand argument required\n")));
              SVN_INT_ERR(subcommand_help(NULL, NULL, pool));
              svn_pool_destroy(pool);
              return EXIT_FAILURE;
            }
        }
      else
        {
          const char *first_arg = os->argv[os->ind++];
          subcommand = svn_opt_get_canonical_subcommand2(cmd_table, first_arg);
          if (subcommand == NULL)
            {
              const char* first_arg_utf8;
              if ((err = svn_utf_cstring_to_utf8(&first_arg_utf8, first_arg,
                                                 pool)))
                return svn_cmdline_handle_exit_error(err, pool,
                                                     "svndumpfilter: ");

              svn_error_clear(svn_cmdline_fprintf(stderr, pool,
                                                  _("Unknown command: '%s'\n"),
                                                  first_arg_utf8));
              SVN_INT_ERR(subcommand_help(NULL, NULL, pool));
              svn_pool_destroy(pool);
              return EXIT_FAILURE;
            }
        }
    }

  /* If there's a second argument, it's probably [one of] prefixes.
     Every subcommand except `help' requires at least one, so we parse
     them out here and store in opt_state. */

  if (subcommand->cmd_func != subcommand_help)
    {

      opt_state.prefixes = apr_array_make(pool, os->argc - os->ind,
                                          sizeof(const char *));
      for (i = os->ind ; i< os->argc; i++)
        {
          const char *prefix;

          /* Ensure that each prefix is UTF8-encoded, in internal
             style, and absolute. */
          SVN_INT_ERR(svn_utf_cstring_to_utf8(&prefix, os->argv[i], pool));
          prefix = svn_relpath__internal_style(prefix, pool);
          if (prefix[0] != '/')
            prefix = apr_pstrcat(pool, "/", prefix, (char *)NULL);
          APR_ARRAY_PUSH(opt_state.prefixes, const char *) = prefix;
        }

      if (opt_state.targets_file)
        {
          svn_stringbuf_t *buffer, *buffer_utf8;
          const char *utf8_targets_file;
          apr_array_header_t *targets = apr_array_make(pool, 0,
                                                       sizeof(const char *));

          /* We need to convert to UTF-8 now, even before we divide
             the targets into an array, because otherwise we wouldn't
             know what delimiter to use for svn_cstring_split().  */

          SVN_INT_ERR(svn_utf_cstring_to_utf8(&utf8_targets_file,
                                              opt_state.targets_file, pool));

          SVN_INT_ERR(svn_stringbuf_from_file2(&buffer, utf8_targets_file,
                                               pool));
          SVN_INT_ERR(svn_utf_stringbuf_to_utf8(&buffer_utf8, buffer, pool));

          targets = apr_array_append(pool,
                         svn_cstring_split(buffer_utf8->data, "\n\r",
                                           TRUE, pool),
                         targets);

          for (i = 0; i < targets->nelts; i++)
            {
              const char *prefix = APR_ARRAY_IDX(targets, i, const char *);
              if (prefix[0] != '/')
                prefix = apr_pstrcat(pool, "/", prefix, (char *)NULL);
              APR_ARRAY_PUSH(opt_state.prefixes, const char *) = prefix;
            }
        }

      if (apr_is_empty_array(opt_state.prefixes))
        {
          svn_error_clear(svn_cmdline_fprintf
                          (stderr, pool,
                           _("\nError: no prefixes supplied.\n")));
          svn_pool_destroy(pool);
          return EXIT_FAILURE;
        }
    }


  /* Check that the subcommand wasn't passed any inappropriate options. */
  for (i = 0; i < received_opts->nelts; i++)
    {
      opt_id = APR_ARRAY_IDX(received_opts, i, int);

      /* All commands implicitly accept --help, so just skip over this
         when we see it. Note that we don't want to include this option
         in their "accepted options" list because it would be awfully
         redundant to display it in every commands' help text. */
      if (opt_id == 'h' || opt_id == '?')
        continue;

      if (! svn_opt_subcommand_takes_option3(subcommand, opt_id, NULL))
        {
          const char *optstr;
          const apr_getopt_option_t *badopt =
            svn_opt_get_option_from_code2(opt_id, options_table, subcommand,
                                          pool);
          svn_opt_format_option(&optstr, badopt, FALSE, pool);
          if (subcommand->name[0] == '-')
            SVN_INT_ERR(subcommand_help(NULL, NULL, pool));
          else
            svn_error_clear(svn_cmdline_fprintf
                            (stderr, pool,
                             _("Subcommand '%s' doesn't accept option '%s'\n"
                               "Type 'svndumpfilter help %s' for usage.\n"),
                             subcommand->name, optstr, subcommand->name));
          svn_pool_destroy(pool);
          return EXIT_FAILURE;
        }
    }

  /* Run the subcommand. */
  err = (*subcommand->cmd_func)(os, &opt_state, pool);
  if (err)
    {
      /* For argument-related problems, suggest using the 'help'
         subcommand. */
      if (err->apr_err == SVN_ERR_CL_INSUFFICIENT_ARGS
          || err->apr_err == SVN_ERR_CL_ARG_PARSING_ERROR)
        {
          err = svn_error_quick_wrap(err,
                                     _("Try 'svndumpfilter help' for more "
                                       "info"));
        }
      return svn_cmdline_handle_exit_error(err, pool, "svndumpfilter: ");
    }
  else
    {
      svn_pool_destroy(pool);

      /* Flush stdout, making sure the user will see any print errors. */
      SVN_INT_ERR(svn_cmdline_fflush(stdout));
      return EXIT_SUCCESS;
    }
}

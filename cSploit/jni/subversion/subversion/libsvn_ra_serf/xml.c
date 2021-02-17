/*
 * xml.c :  standard XML parsing routines for ra_serf
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



#include <apr_uri.h>

#include <expat.h>

#include <serf.h>

#include "svn_pools.h"
#include "svn_ra.h"
#include "svn_dav.h"
#include "svn_xml.h"
#include "../libsvn_ra/ra_loader.h"
#include "svn_config.h"
#include "svn_delta.h"
#include "svn_path.h"
#include "svn_private_config.h"

#include "ra_serf.h"


void
svn_ra_serf__define_ns(svn_ra_serf__ns_t **ns_list,
                       const char **attrs,
                       apr_pool_t *pool)
{
  const char **tmp_attrs = attrs;

  while (*tmp_attrs)
    {
      if (strncmp(*tmp_attrs, "xmlns", 5) == 0)
        {
          svn_ra_serf__ns_t *new_ns, *cur_ns;
          int found = 0;

          /* Have we already defined this ns previously? */
          for (cur_ns = *ns_list; cur_ns; cur_ns = cur_ns->next)
            {
              if (strcmp(cur_ns->namespace, tmp_attrs[0] + 6) == 0)
                {
                  found = 1;
                  break;
                }
            }

          if (!found)
            {
              new_ns = apr_palloc(pool, sizeof(*new_ns));
              new_ns->namespace = apr_pstrdup(pool, tmp_attrs[0] + 6);
              new_ns->url = apr_pstrdup(pool, tmp_attrs[1]);

              new_ns->next = *ns_list;

              *ns_list = new_ns;
            }
        }
      tmp_attrs += 2;
    }
}

/*
 * Look up NAME in the NS_LIST list for previously declared namespace
 * definitions and return a DAV_PROPS_T-tuple that has values.
 */
void
svn_ra_serf__expand_ns(svn_ra_serf__dav_props_t *returned_prop_name,
                       svn_ra_serf__ns_t *ns_list,
                       const char *name)
{
  const char *colon;

  colon = strchr(name, ':');
  if (colon)
    {
      svn_ra_serf__ns_t *ns;

      for (ns = ns_list; ns; ns = ns->next)
        {
          if (strncmp(ns->namespace, name, colon - name) == 0)
            {
              returned_prop_name->namespace = ns->url;
              returned_prop_name->name = colon + 1;
              return;
            }
        }
    }

  /* If there is no prefix, or if the prefix is not found, then the
     name is NOT within a namespace.  */
  returned_prop_name->namespace = "";
  returned_prop_name->name = name;
}

void
svn_ra_serf__expand_string(const char **cur, apr_size_t *cur_len,
                           const char *new, apr_size_t new_len,
                           apr_pool_t *pool)
{
  if (!*cur)
    {
      *cur = apr_pstrmemdup(pool, new, new_len);
      *cur_len = new_len;
    }
  else
    {
      char *new_cur;

      /* append the data we received before. */
      new_cur = apr_palloc(pool, *cur_len + new_len + 1);

      memcpy(new_cur, *cur, *cur_len);
      memcpy(new_cur + *cur_len, new, new_len);

      /* NULL-term our new string */
      new_cur[*cur_len + new_len] = '\0';

      /* update our length */
      *cur_len += new_len;
      *cur = new_cur;
    }
}

#define XML_HEADER "<?xml version=\"1.0\" encoding=\"utf-8\"?>"

void
svn_ra_serf__add_xml_header_buckets(serf_bucket_t *agg_bucket,
                                    serf_bucket_alloc_t *bkt_alloc)
{
  serf_bucket_t *tmp;

  tmp = SERF_BUCKET_SIMPLE_STRING_LEN(XML_HEADER, sizeof(XML_HEADER) - 1,
                                      bkt_alloc);
  serf_bucket_aggregate_append(agg_bucket, tmp);
}

void
svn_ra_serf__add_open_tag_buckets(serf_bucket_t *agg_bucket,
                                  serf_bucket_alloc_t *bkt_alloc,
                                  const char *tag, ...)
{
  va_list ap;
  const char *key;
  serf_bucket_t *tmp;

  tmp = SERF_BUCKET_SIMPLE_STRING_LEN("<", 1, bkt_alloc);
  serf_bucket_aggregate_append(agg_bucket, tmp);

  tmp = SERF_BUCKET_SIMPLE_STRING(tag, bkt_alloc);
  serf_bucket_aggregate_append(agg_bucket, tmp);

  va_start(ap, tag);
  while ((key = va_arg(ap, char *)) != NULL)
    {
      const char *val = va_arg(ap, const char *);
      if (val)
        {
          tmp = SERF_BUCKET_SIMPLE_STRING_LEN(" ", 1, bkt_alloc);
          serf_bucket_aggregate_append(agg_bucket, tmp);

          tmp = SERF_BUCKET_SIMPLE_STRING(key, bkt_alloc);
          serf_bucket_aggregate_append(agg_bucket, tmp);

          tmp = SERF_BUCKET_SIMPLE_STRING_LEN("=\"", 2, bkt_alloc);
          serf_bucket_aggregate_append(agg_bucket, tmp);

          tmp = SERF_BUCKET_SIMPLE_STRING(val, bkt_alloc);
          serf_bucket_aggregate_append(agg_bucket, tmp);

          tmp = SERF_BUCKET_SIMPLE_STRING_LEN("\"", 1, bkt_alloc);
          serf_bucket_aggregate_append(agg_bucket, tmp);
        }
    }
  va_end(ap);

  tmp = SERF_BUCKET_SIMPLE_STRING_LEN(">", 1, bkt_alloc);
  serf_bucket_aggregate_append(agg_bucket, tmp);
}

void
svn_ra_serf__add_close_tag_buckets(serf_bucket_t *agg_bucket,
                                   serf_bucket_alloc_t *bkt_alloc,
                                   const char *tag)
{
  serf_bucket_t *tmp;

  tmp = SERF_BUCKET_SIMPLE_STRING_LEN("</", 2, bkt_alloc);
  serf_bucket_aggregate_append(agg_bucket, tmp);

  tmp = SERF_BUCKET_SIMPLE_STRING(tag, bkt_alloc);
  serf_bucket_aggregate_append(agg_bucket, tmp);

  tmp = SERF_BUCKET_SIMPLE_STRING_LEN(">", 1, bkt_alloc);
  serf_bucket_aggregate_append(agg_bucket, tmp);
}

void
svn_ra_serf__add_cdata_len_buckets(serf_bucket_t *agg_bucket,
                                   serf_bucket_alloc_t *bkt_alloc,
                                   const char *data, apr_size_t len)
{
  const char *end = data + len;
  const char *p = data, *q;
  serf_bucket_t *tmp_bkt;

  while (1)
    {
      /* Find a character which needs to be quoted and append bytes up
         to that point.  Strictly speaking, '>' only needs to be
         quoted if it follows "]]", but it's easier to quote it all
         the time.

         So, why are we escaping '\r' here?  Well, according to the
         XML spec, '\r\n' gets converted to '\n' during XML parsing.
         Also, any '\r' not followed by '\n' is converted to '\n'.  By
         golly, if we say we want to escape a '\r', we want to make
         sure it remains a '\r'!  */
      q = p;
      while (q < end && *q != '&' && *q != '<' && *q != '>' && *q != '\r')
        q++;


      tmp_bkt = SERF_BUCKET_SIMPLE_STRING_LEN(p, q - p, bkt_alloc);
      serf_bucket_aggregate_append(agg_bucket, tmp_bkt);

      /* We may already be a winner.  */
      if (q == end)
        break;

      /* Append the entity reference for the character.  */
      if (*q == '&')
        {
          tmp_bkt = SERF_BUCKET_SIMPLE_STRING_LEN("&amp;", sizeof("&amp;") - 1,
                                                  bkt_alloc);
          serf_bucket_aggregate_append(agg_bucket, tmp_bkt);
        }
      else if (*q == '<')
        {
          tmp_bkt = SERF_BUCKET_SIMPLE_STRING_LEN("&lt;", sizeof("&lt;") - 1,
                                                  bkt_alloc);
          serf_bucket_aggregate_append(agg_bucket, tmp_bkt);
        }
      else if (*q == '>')
        {
          tmp_bkt = SERF_BUCKET_SIMPLE_STRING_LEN("&gt;", sizeof("&gt;") - 1,
                                                  bkt_alloc);
          serf_bucket_aggregate_append(agg_bucket, tmp_bkt);
        }
      else if (*q == '\r')
        {
          tmp_bkt = SERF_BUCKET_SIMPLE_STRING_LEN("&#13;", sizeof("&#13;") - 1,
                                                  bkt_alloc);
          serf_bucket_aggregate_append(agg_bucket, tmp_bkt);
        }

      p = q + 1;
    }
}

void svn_ra_serf__add_tag_buckets(serf_bucket_t *agg_bucket, const char *tag,
                                  const char *value,
                                  serf_bucket_alloc_t *bkt_alloc)
{
  svn_ra_serf__add_open_tag_buckets(agg_bucket, bkt_alloc, tag, NULL);

  if (value)
    {
      svn_ra_serf__add_cdata_len_buckets(agg_bucket, bkt_alloc,
                                         value, strlen(value));
    }

  svn_ra_serf__add_close_tag_buckets(agg_bucket, bkt_alloc, tag);
}

void
svn_ra_serf__xml_push_state(svn_ra_serf__xml_parser_t *parser,
                            int state)
{
  svn_ra_serf__xml_state_t *new_state;

  if (!parser->free_state)
    {
      new_state = apr_palloc(parser->pool, sizeof(*new_state));
      new_state->pool = svn_pool_create(parser->pool);
    }
  else
    {
      new_state = parser->free_state;
      parser->free_state = parser->free_state->prev;

      svn_pool_clear(new_state->pool);
    }

  if (parser->state)
    {
      new_state->private = parser->state->private;
      new_state->ns_list = parser->state->ns_list;
    }
  else
    {
      new_state->private = NULL;
      new_state->ns_list = NULL;
    }

  new_state->current_state = state;

  /* Add it to the state chain. */
  new_state->prev = parser->state;
  parser->state = new_state;
}

void svn_ra_serf__xml_pop_state(svn_ra_serf__xml_parser_t *parser)
{
  svn_ra_serf__xml_state_t *cur_state;

  cur_state = parser->state;
  parser->state = cur_state->prev;
  cur_state->prev = parser->free_state;
  parser->free_state = cur_state;
}

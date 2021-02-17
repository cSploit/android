/*
 * props.c: Utility functions for property handling
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

/* ==================================================================== */



/*** Includes. ***/

#include <apr_hash.h>
#include "svn_cmdline.h"
#include "svn_string.h"
#include "svn_error.h"
#include "svn_sorts.h"
#include "svn_subst.h"
#include "svn_props.h"
#include "svn_string.h"
#include "svn_opt.h"
#include "svn_xml.h"
#include "svn_base64.h"
#include "cl.h"

#include "private/svn_cmdline_private.h"

#include "svn_private_config.h"



svn_error_t *
svn_cl__revprop_prepare(const svn_opt_revision_t *revision,
                        const apr_array_header_t *targets,
                        const char **URL,
                        svn_client_ctx_t *ctx,
                        apr_pool_t *pool)
{
  const char *target;

  if (revision->kind != svn_opt_revision_number
      && revision->kind != svn_opt_revision_date
      && revision->kind != svn_opt_revision_head)
    return svn_error_create
      (SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
       _("Must specify the revision as a number, a date or 'HEAD' "
         "when operating on a revision property"));

  /* There must be exactly one target at this point.  If it was optional and
     unspecified by the user, the caller has already added the implicit '.'. */
  if (targets->nelts != 1)
    return svn_error_create(SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                            _("Wrong number of targets specified"));

  /* (The docs say the target must be either a URL or implicit '.', but
     explicit WC targets are also accepted.) */
  target = APR_ARRAY_IDX(targets, 0, const char *);
  SVN_ERR(svn_client_url_from_path2(URL, target, ctx, pool, pool));
  if (*URL == NULL)
    return svn_error_create
      (SVN_ERR_UNVERSIONED_RESOURCE, NULL,
       _("Either a URL or versioned item is required"));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_cl__print_prop_hash(svn_stream_t *out,
                        apr_hash_t *prop_hash,
                        svn_boolean_t names_only,
                        apr_pool_t *pool)
{
  apr_array_header_t *sorted_props;
  int i;

  sorted_props = svn_sort__hash(prop_hash, svn_sort_compare_items_lexically,
                                pool);
  for (i = 0; i < sorted_props->nelts; i++)
    {
      svn_sort__item_t item = APR_ARRAY_IDX(sorted_props, i, svn_sort__item_t);
      const char *pname = item.key;
      svn_string_t *propval = item.value;
      const char *pname_stdout;
      apr_size_t len;

      if (svn_prop_needs_translation(pname))
        SVN_ERR(svn_subst_detranslate_string(&propval, propval,
                                             TRUE, pool));

      SVN_ERR(svn_cmdline_cstring_from_utf8(&pname_stdout, pname, pool));

      if (out)
        {
          pname_stdout = apr_psprintf(pool, "  %s\n", pname_stdout);
          SVN_ERR(svn_subst_translate_cstring2(pname_stdout, &pname_stdout,
                                              APR_EOL_STR,  /* 'native' eol */
                                              FALSE, /* no repair */
                                              NULL,  /* no keywords */
                                              FALSE, /* no expansion */
                                              pool));

          len = strlen(pname_stdout);
          SVN_ERR(svn_stream_write(out, pname_stdout, &len));
        }
      else
        {
          /* ### We leave these printfs for now, since if propval wasn't
             translated above, we don't know anything about its encoding.
             In fact, it might be binary data... */
          printf("  %s\n", pname_stdout);
        }

      if (!names_only)
        {
          /* Add an extra newline to the value before indenting, so that
           * every line of output has the indentation whether the value
           * already ended in a newline or not. */
          const char *newval = apr_psprintf(pool, "%s\n", propval->data);
          const char *indented_newval = svn_cl__indent_string(newval,
                                                              "    ",
                                                              pool);
          if (out)
            {
              len = strlen(indented_newval);
              SVN_ERR(svn_stream_write(out, indented_newval, &len));
            }
          else
            {
              printf("%s", indented_newval);
            }
        }
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_cl__print_xml_prop_hash(svn_stringbuf_t **outstr,
                            apr_hash_t *prop_hash,
                            svn_boolean_t names_only,
                            apr_pool_t *pool)
{
  apr_array_header_t *sorted_props;
  int i;

  if (*outstr == NULL)
    *outstr = svn_stringbuf_create("", pool);

  sorted_props = svn_sort__hash(prop_hash, svn_sort_compare_items_lexically,
                                pool);
  for (i = 0; i < sorted_props->nelts; i++)
    {
      svn_sort__item_t item = APR_ARRAY_IDX(sorted_props, i, svn_sort__item_t);
      const char *pname = item.key;
      svn_string_t *propval = item.value;

      if (names_only)
        {
          svn_xml_make_open_tag(outstr, pool, svn_xml_self_closing, "property",
                                "name", pname, NULL);
        }
      else
        {
          const char *pname_out;

          if (svn_prop_needs_translation(pname))
            SVN_ERR(svn_subst_detranslate_string(&propval, propval,
                                                 TRUE, pool));

          SVN_ERR(svn_cmdline_cstring_from_utf8(&pname_out, pname, pool));

          svn_cmdline__print_xml_prop(outstr, pname_out, propval, pool);
        }
    }

    return SVN_NO_ERROR;
}


void
svn_cl__check_boolean_prop_val(const char *propname, const char *propval,
                               apr_pool_t *pool)
{
  svn_stringbuf_t *propbuf;

  if (!svn_prop_is_boolean(propname))
    return;

  propbuf = svn_stringbuf_create(propval, pool);
  svn_stringbuf_strip_whitespace(propbuf);

  if (propbuf->data[0] == '\0'
      || strcmp(propbuf->data, "no") == 0
      || strcmp(propbuf->data, "off") == 0
      || strcmp(propbuf->data, "false") == 0)
    {
      svn_error_t *err = svn_error_createf
        (SVN_ERR_BAD_PROPERTY_VALUE, NULL,
         _("To turn off the %s property, use 'svn propdel';\n"
           "setting the property to '%s' will not turn it off."),
           propname, propval);
      svn_handle_warning2(stderr, err, "svn: ");
      svn_error_clear(err);
    }
}


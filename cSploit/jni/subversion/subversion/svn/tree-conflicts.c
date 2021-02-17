/*
 * tree-conflicts.c: Tree conflicts.
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

#include "tree-conflicts.h"
#include "svn_xml.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "private/svn_token.h"

#include "cl.h"

#include "svn_private_config.h"


/* A map for svn_wc_conflict_action_t values to human-readable strings */
static const svn_token_map_t map_conflict_action_human[] =
{
  { N_("edit"),         svn_wc_conflict_action_edit },
  { N_("delete"),       svn_wc_conflict_action_delete },
  { N_("add"),          svn_wc_conflict_action_add },
  { N_("replace"),      svn_wc_conflict_action_replace },
  { NULL,               0 }
};

/* A map for svn_wc_conflict_action_t values to XML strings */
static const svn_token_map_t map_conflict_action_xml[] =
{
  { "edit",             svn_wc_conflict_action_edit },
  { "delete",           svn_wc_conflict_action_delete },
  { "add",              svn_wc_conflict_action_add },
  { "replace",          svn_wc_conflict_action_replace },
  { NULL,               0 }
};

/* A map for svn_wc_conflict_reason_t values to human-readable strings */
static const svn_token_map_t map_conflict_reason_human[] =
{
  { N_("edit"),         svn_wc_conflict_reason_edited },
  { N_("delete"),       svn_wc_conflict_reason_deleted },
  { N_("missing"),      svn_wc_conflict_reason_missing },
  { N_("obstruction"),  svn_wc_conflict_reason_obstructed },
  { N_("add"),          svn_wc_conflict_reason_added },
  { N_("replace"),      svn_wc_conflict_reason_replaced },
  { N_("unversioned"),  svn_wc_conflict_reason_unversioned },
  { NULL,               0 }
};

/* A map for svn_wc_conflict_reason_t values to XML strings */
static const svn_token_map_t map_conflict_reason_xml[] =
{
  { "edit",             svn_wc_conflict_reason_edited },
  { "delete",           svn_wc_conflict_reason_deleted },
  { "missing",          svn_wc_conflict_reason_missing },
  { "obstruction",      svn_wc_conflict_reason_obstructed },
  { "add",              svn_wc_conflict_reason_added },
  { "replace",          svn_wc_conflict_reason_replaced },
  { "unversioned",      svn_wc_conflict_reason_unversioned },
  { NULL,               0 }
};

/* Return a localized string representation of CONFLICT->action. */
static const char *
action_str(const svn_wc_conflict_description2_t *conflict)
{
  return _(svn_token__to_word(map_conflict_action_human, conflict->action));
}

/* Return a localized string representation of CONFLICT->reason. */
static const char *
reason_str(const svn_wc_conflict_description2_t *conflict)
{
  return _(svn_token__to_word(map_conflict_reason_human, conflict->reason));
}


svn_error_t *
svn_cl__get_human_readable_tree_conflict_description(
  const char **desc,
  const svn_wc_conflict_description2_t *conflict,
  apr_pool_t *pool)
{
  const char *action, *reason, *operation;
  reason = reason_str(conflict);
  action = action_str(conflict);
  operation = svn_cl__operation_str_human_readable(conflict->operation, pool);
  SVN_ERR_ASSERT(action && reason);
  *desc = apr_psprintf(pool, _("local %s, incoming %s upon %s"),
                       reason, action, operation);
  return SVN_NO_ERROR;
}


/* Helper for svn_cl__append_tree_conflict_info_xml().
 * Appends the attributes of the given VERSION to ATT_HASH.
 * SIDE is the content of the version tag's side="..." attribute,
 * currently one of "source-left" or "source-right".*/
static svn_error_t *
add_conflict_version_xml(svn_stringbuf_t **pstr,
                         const char *side,
                         const svn_wc_conflict_version_t *version,
                         apr_pool_t *pool)
{
  apr_hash_t *att_hash = apr_hash_make(pool);


  apr_hash_set(att_hash, "side", APR_HASH_KEY_STRING, side);

  if (version->repos_url)
    apr_hash_set(att_hash, "repos-url", APR_HASH_KEY_STRING,
                 version->repos_url);

  if (version->path_in_repos)
    apr_hash_set(att_hash, "path-in-repos", APR_HASH_KEY_STRING,
                 version->path_in_repos);

  if (SVN_IS_VALID_REVNUM(version->peg_rev))
    apr_hash_set(att_hash, "revision", APR_HASH_KEY_STRING,
                 apr_ltoa(pool, version->peg_rev));

  if (version->node_kind != svn_node_unknown)
    apr_hash_set(att_hash, "kind", APR_HASH_KEY_STRING,
                 svn_cl__node_kind_str_xml(version->node_kind));

  svn_xml_make_open_tag_hash(pstr, pool, svn_xml_self_closing,
                             "version", att_hash);
  return SVN_NO_ERROR;
}


svn_error_t *
svn_cl__append_tree_conflict_info_xml(
  svn_stringbuf_t *str,
  const svn_wc_conflict_description2_t *conflict,
  apr_pool_t *pool)
{
  apr_hash_t *att_hash = apr_hash_make(pool);
  const char *tmp;

  apr_hash_set(att_hash, "victim", APR_HASH_KEY_STRING,
               svn_dirent_basename(conflict->local_abspath, pool));

  apr_hash_set(att_hash, "kind", APR_HASH_KEY_STRING,
               svn_cl__node_kind_str_xml(conflict->node_kind));

  apr_hash_set(att_hash, "operation", APR_HASH_KEY_STRING,
               svn_cl__operation_str_xml(conflict->operation, pool));

  tmp = svn_token__to_word(map_conflict_action_xml, conflict->action);
  apr_hash_set(att_hash, "action", APR_HASH_KEY_STRING, tmp);

  tmp = svn_token__to_word(map_conflict_reason_xml, conflict->reason);
  apr_hash_set(att_hash, "reason", APR_HASH_KEY_STRING, tmp);

  /* Open the tree-conflict tag. */
  svn_xml_make_open_tag_hash(&str, pool, svn_xml_normal,
                             "tree-conflict", att_hash);

  /* Add child tags for OLDER_VERSION and THEIR_VERSION. */

  if (conflict->src_left_version)
    SVN_ERR(add_conflict_version_xml(&str,
                                     "source-left",
                                     conflict->src_left_version,
                                     pool));

  if (conflict->src_right_version)
    SVN_ERR(add_conflict_version_xml(&str,
                                     "source-right",
                                     conflict->src_right_version,
                                     pool));

  svn_xml_make_close_tag(&str, pool, "tree-conflict");

  return SVN_NO_ERROR;
}


/*
 * checkout.c : read a repository and drive a checkout editor.
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

#include "ra_local.h"
#include <string.h>
#include "svn_path.h"
#include "svn_dirent_uri.h"
#include "svn_private_config.h"


svn_error_t *
svn_ra_local__split_URL(svn_repos_t **repos,
                        const char **repos_url,
                        const char **fs_path,
                        const char *URL,
                        apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  const char *repos_dirent;
  const char *repos_root_dirent;
  svn_stringbuf_t *urlbuf;

  SVN_ERR(svn_uri_get_dirent_from_file_url(&repos_dirent, URL, pool));

  /* Search for a repository in the full path. */
  repos_root_dirent = svn_repos_find_root_path(repos_dirent, pool);
  if (!repos_root_dirent)
    return svn_error_createf(SVN_ERR_RA_LOCAL_REPOS_OPEN_FAILED, NULL,
                             _("Unable to open repository '%s'"), URL);

  /* Attempt to open a repository at URL. */
  err = svn_repos_open2(repos, repos_root_dirent, NULL, pool);
  if (err)
    return svn_error_createf(SVN_ERR_RA_LOCAL_REPOS_OPEN_FAILED, err,
                             _("Unable to open repository '%s'"), URL);

  /* Assert capabilities directly, since client == server. */
  {
    apr_array_header_t *caps = apr_array_make(pool, 1, sizeof(const char *));
    APR_ARRAY_PUSH(caps, const char *) = SVN_RA_CAPABILITY_MERGEINFO;
    SVN_ERR(svn_repos_remember_client_capabilities(*repos, caps));
  }

  *fs_path = &repos_dirent[strlen(repos_root_dirent)];

  if (**fs_path == '\0')
    *fs_path = "/";

  /* Remove the path components in *fs_path from the original URL, to get
     the URL to the repository root. */
  urlbuf = svn_stringbuf_create(URL, pool);
  svn_path_remove_components(urlbuf,
                             svn_path_component_count(repos_dirent)
                             - svn_path_component_count(repos_root_dirent));
  *repos_url = urlbuf->data;

  return SVN_NO_ERROR;
}

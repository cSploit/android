/*
 * version.c:  library version number and utilities
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



#include "svn_error.h"
#include "svn_version.h"

#include "svn_private_config.h"

const svn_version_t *
svn_subr_version(void)
{
  SVN_VERSION_BODY;
}


svn_boolean_t svn_ver_compatible(const svn_version_t *my_version,
                                 const svn_version_t *lib_version)
{
  /* With normal development builds the matching rules are strict, to
     avoid inadvertantly using the wrong libraries.  For backward
     compatibility testing use --disable-full-version-match to
     configure 1.7 and then the libraries that get built can be used
     to replace those in 1.6 or earlier builds.  */

#ifndef SVN_DISABLE_FULL_VERSION_MATCH
  if (lib_version->tag[0] != '\0')
    /* Development library; require exact match. */
    return svn_ver_equal(my_version, lib_version);
  else if (my_version->tag[0] != '\0')
    /* Development client; must be newer than the library
       and have the same major and minor version. */
    return (my_version->major == lib_version->major
            && my_version->minor == lib_version->minor
            && my_version->patch > lib_version->patch);
#endif

  /* General compatibility rules for released versions. */
  return (my_version->major == lib_version->major
          && my_version->minor <= lib_version->minor);
}


svn_boolean_t svn_ver_equal(const svn_version_t *my_version,
                            const svn_version_t *lib_version)
{
  return (my_version->major == lib_version->major
          && my_version->minor == lib_version->minor
          && my_version->patch == lib_version->patch
          && 0 == strcmp(my_version->tag, lib_version->tag));
}


svn_error_t *
svn_ver_check_list(const svn_version_t *my_version,
                   const svn_version_checklist_t *checklist)
{
  svn_error_t *err = SVN_NO_ERROR;
  int i;

  for (i = 0; checklist[i].label != NULL; ++i)
    {
      const svn_version_t *lib_version = checklist[i].version_query();
      if (!svn_ver_compatible(my_version, lib_version))
        err = svn_error_createf(SVN_ERR_VERSION_MISMATCH, err,
                                _("Version mismatch in '%s':"
                                  " found %d.%d.%d%s,"
                                  " expected %d.%d.%d%s"),
                                checklist[i].label,
                                lib_version->major, lib_version->minor,
                                lib_version->patch, lib_version->tag,
                                my_version->major, my_version->minor,
                                my_version->patch, my_version->tag);
    }

  return err;
}

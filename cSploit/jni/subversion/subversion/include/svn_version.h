/**
 * @copyright
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
 * @endcopyright
 *
 * @file svn_version.h
 * @brief Version information.
 */

#ifndef SVN_VERSION_H
#define SVN_VERSION_H

/* Hack to prevent the resource compiler from including
   apr_general.h.  It doesn't resolve the include paths
   correctly and blows up without this.
 */
#ifndef APR_STRINGIFY
#include <apr_general.h>
#endif

#include "svn_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Symbols that define the version number. */

/* Version numbers: <major>.<minor>.<micro>
 *
 * The version numbers in this file follow the rules established by:
 *
 *   http://apr.apache.org/versioning.html
 */

/** Major version number.
 *
 * Modify when incompatible changes are made to published interfaces.
 */
#define SVN_VER_MAJOR      1

/** Minor version number.
 *
 * Modify when new functionality is added or new interfaces are
 * defined, but all changes are backward compatible.
 */
#define SVN_VER_MINOR      7

/**
 * Patch number.
 *
 * Modify for every released patch.
 *
 * @since New in 1.1.
 */
#define SVN_VER_PATCH      13


/** @deprecated Provided for backward compatibility with the 1.0 API. */
#define SVN_VER_MICRO      SVN_VER_PATCH

/** @deprecated Provided for backward compatibility with the 1.0 API. */
#define SVN_VER_LIBRARY    SVN_VER_MAJOR


/** Version tag: a string describing the version.
 *
 * This tag remains " (dev build)" in the repository so that we can
 * always see from "svn --version" that the software has been built
 * from the repository rather than a "blessed" distribution.
 *
 * When rolling a tarball, we automatically replace this text with " (r1234)"
 * (where 1234 is the last revision on the branch prior to the release)
 * for final releases; in prereleases, it becomes " (Alpha 1)",
 * " (Beta 1)", etc., as appropriate.
 *
 * Always change this at the same time as SVN_VER_NUMTAG.
 */
#define SVN_VER_TAG        " (r1516569)"


/** Number tag: a string describing the version.
 *
 * This tag is used to generate a version number string to identify
 * the client and server in HTTP requests, for example. It must not
 * contain any spaces. This value remains "-dev" in the repository.
 *
 * When rolling a tarball, we automatically replace this text with ""
 * for final releases; in prereleases, it becomes "-alpha1, "-beta1",
 * etc., as appropriate.
 *
 * Always change this at the same time as SVN_VER_TAG.
 */
#define SVN_VER_NUMTAG     ""


/** Revision number: The repository revision number of this release.
 *
 * This constant is used to generate the build number part of the Windows
 * file version. Its value remains 0 in the repository.
 *
 * When rolling a tarball, we automatically replace it with what we
 * guess to be the correct revision number.
 */
#define SVN_VER_REVISION   1516569


/* Version strings composed from the above definitions. */

/** Version number */
#define SVN_VER_NUM        APR_STRINGIFY(SVN_VER_MAJOR) \
                           "." APR_STRINGIFY(SVN_VER_MINOR) \
                           "." APR_STRINGIFY(SVN_VER_PATCH)

/** Version number with tag (contains no whitespace) */
#define SVN_VER_NUMBER     SVN_VER_NUM SVN_VER_NUMTAG

/** Complete version string */
#define SVN_VERSION        SVN_VER_NUMBER SVN_VER_TAG



/* Version queries and compatibility checks */

/**
 * Version information. Each library contains a function called
 * svn_<i>libname</i>_version() that returns a pointer to a statically
 * allocated object of this type.
 *
 * @since New in 1.1.
 */
struct svn_version_t
{
  int major;                    /**< Major version number */
  int minor;                    /**< Minor version number */
  int patch;                    /**< Patch number */

  /**
   * The version tag (#SVN_VER_NUMTAG). Must always point to a
   * statically allocated string.
   */
  const char *tag;
};

/**
 * Define a static svn_version_t object.
 *
 * @since New in 1.1.
 */
#define SVN_VERSION_DEFINE(name) \
  static const svn_version_t name = \
    { \
      SVN_VER_MAJOR, \
      SVN_VER_MINOR, \
      SVN_VER_PATCH, \
      SVN_VER_NUMTAG \
    } \

/**
 * Generate the implementation of a version query function.
 *
 * @since New in 1.1.
 */
#define SVN_VERSION_BODY \
  SVN_VERSION_DEFINE(versioninfo);              \
  return &versioninfo

/**
 * Check library version compatibility. Return #TRUE if the client's
 * version, given in @a my_version, is compatible with the library
 * version, provided in @a lib_version.
 *
 * This function checks for version compatibility as per our
 * guarantees, but requires an exact match when linking to an
 * unreleased library. A development client is always compatible with
 * a previous released library.
 *
 * @since New in 1.1.
 */
svn_boolean_t
svn_ver_compatible(const svn_version_t *my_version,
                   const svn_version_t *lib_version);

/**
 * Check if @a my_version and @a lib_version encode the same version number.
 *
 * @since New in 1.2.
 */
svn_boolean_t
svn_ver_equal(const svn_version_t *my_version,
              const svn_version_t *lib_version);


/**
 * An entry in the compatibility checklist.
 * @see svn_ver_check_list()
 *
 * @since New in 1.1.
 */
typedef struct svn_version_checklist_t
{
  const char *label;            /**< Entry label */

  /** Version query function for this entry */
  const svn_version_t *(*version_query)(void);
} svn_version_checklist_t;


/**
 * Perform a series of version compatibility checks. Checks if @a
 * my_version is compatible with each entry in @a checklist. @a
 * checklist must end with an entry whose label is @c NULL.
 *
 * @see svn_ver_compatible()
 *
 * @since New in 1.1.
 */
svn_error_t *
svn_ver_check_list(const svn_version_t *my_version,
                   const svn_version_checklist_t *checklist);


/**
 * Type of function returning library version.
 *
 * @since New in 1.6.
 */
typedef const svn_version_t *(*svn_version_func_t)(void);


/* libsvn_subr doesn't have an svn_subr header, so put the prototype here. */
/**
 * Get libsvn_subr version information.
 *
 * @since New in 1.1.
 */
const svn_version_t *
svn_subr_version(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_VERSION_H */

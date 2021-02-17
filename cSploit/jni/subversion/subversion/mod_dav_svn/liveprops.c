/*
 * liveprops.c: mod_dav_svn live property provider functions for Subversion
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

#include <apr_tables.h>

#include <httpd.h>
#include <http_core.h>
#include <http_log.h>
#include <util_xml.h>
#include <mod_dav.h>

#include "svn_checksum.h"
#include "svn_pools.h"
#include "svn_time.h"
#include "svn_dav.h"
#include "svn_props.h"

#include "private/svn_dav_protocol.h"

#include "dav_svn.h"


/*
** The namespace URIs that we use. This list and the enumeration must
** stay in sync.
*/
static const char * const namespace_uris[] =
{
  "DAV:",
  SVN_DAV_PROP_NS_DAV,

  NULL        /* sentinel */
};
enum {
  NAMESPACE_URI_DAV,  /* the DAV: namespace URI */
  NAMESPACE_URI       /* the dav<->ra_dav namespace URI */
};

#define SVN_RO_DAV_PROP(name) \
        { NAMESPACE_URI_DAV, #name, DAV_PROPID_##name, 0 }
#define SVN_RW_DAV_PROP(name) \
        { NAMESPACE_URI_DAV, #name, DAV_PROPID_##name, 1 }
#define SVN_RO_DAV_PROP2(sym,name) \
        { NAMESPACE_URI_DAV, #name, DAV_PROPID_##sym, 0 }
#define SVN_RW_DAV_PROP2(sym,name) \
        { NAMESPACE_URI_DAV, #name, DAV_PROPID_##sym, 1 }

#define SVN_RO_SVN_PROP(sym,name) \
        { NAMESPACE_URI, #name, SVN_PROPID_##sym, 0 }
#define SVN_RW_SVN_PROP(sym,name) \
        { NAMESPACE_URI, #name, SVN_PROPID_##sym, 1 }


enum {
  SVN_PROPID_baseline_relative_path = 1,
  SVN_PROPID_md5_checksum,
  SVN_PROPID_repository_uuid,
  SVN_PROPID_deadprop_count
};


static const dav_liveprop_spec props[] =
{
  /* ### don't worry about these for a bit */
#if 0
  /* WebDAV properties */
  SVN_RO_DAV_PROP(getcontentlanguage),  /* ### make this r/w? */
#endif
  SVN_RO_DAV_PROP(getcontentlength),
  SVN_RO_DAV_PROP(getcontenttype),      /* ### make this r/w? */
  SVN_RO_DAV_PROP(getetag),
  SVN_RO_DAV_PROP(creationdate),
  SVN_RO_DAV_PROP(getlastmodified),

  /* DeltaV properties */
  SVN_RO_DAV_PROP2(baseline_collection, baseline-collection),
  SVN_RO_DAV_PROP2(checked_in, checked-in),
  SVN_RO_DAV_PROP2(version_controlled_configuration,
                   version-controlled-configuration),
  SVN_RO_DAV_PROP2(version_name, version-name),
  SVN_RO_DAV_PROP2(creator_displayname, creator-displayname),
  SVN_RO_DAV_PROP2(auto_version, auto-version),

  /* SVN properties */
  SVN_RO_SVN_PROP(baseline_relative_path, baseline-relative-path),
  SVN_RO_SVN_PROP(md5_checksum, md5-checksum),
  SVN_RO_SVN_PROP(repository_uuid, repository-uuid),
  SVN_RO_SVN_PROP(deadprop_count, deadprop-count),

  { 0 } /* sentinel */
};


/* Set *PROPVAL to the value for the revision property PROPNAME on
   COMMITTED_REV, in the repository identified by RESOURCE, if
   RESOURCE's path is readable.  If it is not readable, set *PROPVAL
   to NULL and return SVN_NO_ERROR.  Use POOL for temporary
   allocations and the allocation of *PROPVAL.

   Note that this function does not check the readability of the
   revision property, but the readability of a path.  The true
   readability of a revision property is determined by investigating
   the readability of all changed paths in the revision.  For certain
   revision properties (e.g. svn:author and svn:date) to be readable,
   it is enough if at least one changed path is readable.  When we
   already have a changed path, we can skip the check for the other
   changed paths in the revision and save a lot of work.  This means
   that we will make a mistake when our path is unreadable and another
   changed path is readable, but we will at least only hide too much
   and not leak any protected properties.

   WARNING: This method of only checking the readability of a path is
   only valid to get revision properties for which it is enough if at
   least one changed path is readable.  Using this function to get
   revision properties for which all changed paths must be readable
   might leak protected information because we will only test the
   readability of a single changed path.
*/
static svn_error_t *
get_path_revprop(svn_string_t **propval,
                 const dav_resource *resource,
                 svn_revnum_t committed_rev,
                 const char *propname,
                 apr_pool_t *pool)
{
  *propval = NULL;

  if (! dav_svn__allow_read_resource(resource, committed_rev, pool))
    return SVN_NO_ERROR;

  /* Get the property of the created revision. The authz is already
     performed, so we don't need to do it here too. */
  return svn_repos_fs_revision_prop(propval,
                                    resource->info->repos->repos,
                                    committed_rev,
                                    propname,
                                    NULL, NULL, pool);
}


enum time_format {
  time_format_iso8601,
  time_format_rfc1123
};


/* Given a mod_dav_svn @a resource, set @a *timeval and @a *datestring
   to the last-modified-time of the resource.  The datestring will be
   formatted according to @a format.  Use @a pool for both
   scratchwork, and to allocate @a *datestring.

   If @a timeval or @a datestring is NULL, don't touch it.

   Return zero on success, non-zero if an error occurs. */
static int
get_last_modified_time(const char **datestring,
                       apr_time_t *timeval,
                       const dav_resource *resource,
                       enum time_format format,
                       apr_pool_t *pool)
{
  svn_revnum_t committed_rev = SVN_INVALID_REVNUM;
  svn_string_t *committed_date = NULL;
  svn_error_t *serr;
  apr_time_t timeval_tmp;

  if ((datestring == NULL) && (timeval == NULL))
    return 0;

  if (resource->baselined && resource->type == DAV_RESOURCE_TYPE_VERSION)
    {
      /* A baseline URI. */
      committed_rev = resource->info->root.rev;
    }
  else if (resource->type == DAV_RESOURCE_TYPE_REGULAR
           || resource->type == DAV_RESOURCE_TYPE_WORKING
           || resource->type == DAV_RESOURCE_TYPE_VERSION)
    {
      serr = svn_fs_node_created_rev(&committed_rev,
                                     resource->info->root.root,
                                     resource->info->repos_path, pool);
      if (serr != NULL)
        {
          svn_error_clear(serr);
          return 1;
        }
    }
  else
    {
      /* unsupported resource kind -- has no mod-time */
      return 1;
    }

  serr = get_path_revprop(&committed_date,
                          resource,
                          committed_rev,
                          SVN_PROP_REVISION_DATE,
                          pool);
  if (serr)
    {
      svn_error_clear(serr);
      return 1;
    }

  if (committed_date == NULL)
    return 1;

  /* return the ISO8601 date as an apr_time_t */
  serr = svn_time_from_cstring(&timeval_tmp, committed_date->data, pool);
  if (serr != NULL)
    {
      svn_error_clear(serr);
      return 1;
    }

  if (timeval)
    memcpy(timeval, &timeval_tmp, sizeof(*timeval));

  if (! datestring)
    return 0;

  if (format == time_format_iso8601)
    {
      *datestring = committed_date->data;
    }
  else if (format == time_format_rfc1123)
    {
      apr_time_exp_t tms;
      apr_status_t status;

      /* convert the apr_time_t into an apr_time_exp_t */
      status = apr_time_exp_gmt(&tms, timeval_tmp);
      if (status != APR_SUCCESS)
        return 1;

      /* stolen from dav/fs/repos.c   :-)  */
      *datestring = apr_psprintf(pool, "%s, %.2d %s %d %.2d:%.2d:%.2d GMT",
                                 apr_day_snames[tms.tm_wday],
                                 tms.tm_mday, apr_month_snames[tms.tm_mon],
                                 tms.tm_year + 1900,
                                 tms.tm_hour, tms.tm_min, tms.tm_sec);
    }
  else /* unknown time format */
    {
      return 1;
    }

  return 0;
}

static dav_prop_insert
insert_prop_internal(const dav_resource *resource,
                     int propid,
                     dav_prop_insert what,
                     apr_text_header *phdr,
                     apr_pool_t *scratch_pool,
                     apr_pool_t *result_pool)
{
  const char *value = NULL;
  const char *s;
  const dav_liveprop_spec *info;
  int global_ns;
  svn_error_t *serr;

  /*
  ** Almost none of the SVN provider properties are defined if the
  ** resource does not exist.  We do need to return the one VCC
  ** property and baseline-relative-path on lock-null resources,
  ** however, so that svn clients can run 'svn unlock' and 'svn info'
  ** on these things.
  **
  ** Even though we state that the SVN properties are not defined, the
  ** client cannot store dead values -- we deny that thru the is_writable
  ** hook function.
  */
  if ((! resource->exists)
      && (propid != DAV_PROPID_version_controlled_configuration)
      && (propid != SVN_PROPID_baseline_relative_path))
    return DAV_PROP_INSERT_NOTSUPP;

  /* ### we may want to respond to DAV_PROPID_resourcetype for PRIVATE
     ### resources. need to think on "proper" interaction with mod_dav */

  switch (propid)
    {
    case DAV_PROPID_getlastmodified:
    case DAV_PROPID_creationdate:
      {
        /* In subversion terms, the date attached to a file's CR is
           the true "last modified" time.  However, we're defining
           creationdate in the same way.  IMO, the "creationdate" is
           really the date attached to the revision in which the item
           *first* came into existence; this would found by tracing
           back through the log of the file -- probably via
           svn_fs_revisions_changed.  gstein, is it a bad thing that
           we're currently using 'creationdate' to mean the same thing
           as 'last modified date'?  */
        const char *datestring;
        apr_time_t timeval;
        enum time_format format;

        /* ### for now, our global VCC has no such property. */
        if (resource->type == DAV_RESOURCE_TYPE_PRIVATE
            && (resource->info->restype == DAV_SVN_RESTYPE_VCC
                || resource->info->restype == DAV_SVN_RESTYPE_ME))
          {
            return DAV_PROP_INSERT_NOTSUPP;
          }

        if (propid == DAV_PROPID_creationdate)
          {
            /* Return an ISO8601 date; this is what the svn client
               expects, and rfc2518 demands it. */
            format = time_format_iso8601;
          }
        else /* propid == DAV_PROPID_getlastmodified */
          {
            format = time_format_rfc1123;
          }

        if (0 != get_last_modified_time(&datestring, &timeval,
                                        resource, format, scratch_pool))
          {
            return DAV_PROP_INSERT_NOTDEF;
          }

        value = apr_xml_quote_string(scratch_pool, datestring, 1);
        break;
      }

    case DAV_PROPID_creator_displayname:
      {
        svn_revnum_t committed_rev = SVN_INVALID_REVNUM;
        svn_string_t *last_author = NULL;

        /* ### for now, our global VCC has no such property. */
        if (resource->type == DAV_RESOURCE_TYPE_PRIVATE
            && (resource->info->restype == DAV_SVN_RESTYPE_VCC
                || resource->info->restype == DAV_SVN_RESTYPE_ME))
          {
            return DAV_PROP_INSERT_NOTSUPP;
          }

        if (resource->baselined && resource->type == DAV_RESOURCE_TYPE_VERSION)
          {
            /* A baseline URI. */
            committed_rev = resource->info->root.rev;
          }
        else if (resource->type == DAV_RESOURCE_TYPE_REGULAR
                 || resource->type == DAV_RESOURCE_TYPE_WORKING
                 || resource->type == DAV_RESOURCE_TYPE_VERSION)
          {
            /* Get the CR field out of the node's skel.  Notice that the
               root object might be an ID root -or- a revision root. */
            serr = svn_fs_node_created_rev(&committed_rev,
                                           resource->info->root.root,
                                           resource->info->repos_path,
                                           scratch_pool);
            if (serr != NULL)
              {
                ap_log_rerror(APLOG_MARK, APLOG_ERR, serr->apr_err, 
                              resource->info->r,
                              "Can't get created-rev of '%s': "
                              "%s",
                              resource->info->repos_path,
                              serr->message);
                svn_error_clear(serr);
                value = "###error###";
                break;
              }
          }
        else
          {
            return DAV_PROP_INSERT_NOTSUPP;
          }

        serr = get_path_revprop(&last_author,
                                resource,
                                committed_rev,
                                SVN_PROP_REVISION_AUTHOR,
                                scratch_pool);
        if (serr)
          {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, serr->apr_err, 
                          resource->info->r,
                          "Can't get author of r%ld: "
                          "%s",
                          committed_rev,
                          serr->message);
            svn_error_clear(serr);
            value = "###error###";
            break;
          }

        if (last_author == NULL)
          return DAV_PROP_INSERT_NOTDEF;

        value = apr_xml_quote_string(scratch_pool, last_author->data, 1);
        break;
      }

    case DAV_PROPID_getcontentlanguage:
      /* ### need something here */
      return DAV_PROP_INSERT_NOTSUPP;
      break;

    case DAV_PROPID_getcontentlength:
      {
        svn_filesize_t len = 0;

        /* our property, but not defined on collection resources */
        if (resource->type == DAV_RESOURCE_TYPE_ACTIVITY
            || resource->collection || resource->baselined)
          return DAV_PROP_INSERT_NOTSUPP;

        serr = svn_fs_file_length(&len, resource->info->root.root,
                                  resource->info->repos_path, scratch_pool);
        if (serr != NULL)
          {
            svn_error_clear(serr);
            value = "0";  /* ### what to do? */
            break;
          }

        value = apr_psprintf(scratch_pool, "%" SVN_FILESIZE_T_FMT, len);
        break;
      }

    case DAV_PROPID_getcontenttype:
      {
        /* The subversion client assumes that any file without an
           svn:mime-type property is of type text/plain.  So it seems
           safe (and consistent) to assume the same on the server.  */
        svn_string_t *pval;
        const char *mime_type = NULL;

        if (resource->type == DAV_RESOURCE_TYPE_ACTIVITY
            || (resource->baselined
                && resource->type == DAV_RESOURCE_TYPE_VERSION))
          return DAV_PROP_INSERT_NOTSUPP;

        if (resource->type == DAV_RESOURCE_TYPE_PRIVATE
            && (resource->info->restype == DAV_SVN_RESTYPE_VCC
                || resource->info->restype == DAV_SVN_RESTYPE_ME))
          {
            return DAV_PROP_INSERT_NOTSUPP;
          }

        if (resource->collection) /* defaults for directories */
          {
            if (resource->info->repos->xslt_uri)
              mime_type = "text/xml";
            else
              mime_type = "text/html; charset=UTF-8";
          }
        else
          {
            if ((serr = svn_fs_node_prop(&pval, resource->info->root.root,
                                         resource->info->repos_path,
                                         SVN_PROP_MIME_TYPE, scratch_pool)))
              {
                svn_error_clear(serr);
                pval = NULL;
              }

            if (pval)
              mime_type = pval->data;
            else if ((! resource->info->repos->is_svn_client)
                     && resource->info->r->content_type)
              mime_type = resource->info->r->content_type;
            else
              mime_type = "text/plain";

            if ((serr = svn_mime_type_validate(mime_type, scratch_pool)))
              {
                /* Probably serr->apr == SVN_ERR_BAD_MIME_TYPE, but
                   there's no point even checking.  No matter what the
                   error is, we can't claim to have a mime type for
                   this resource. */
                svn_error_clear(serr);
                return DAV_PROP_INSERT_NOTDEF;
              }
          }

        value = mime_type;
        break;
      }

    case DAV_PROPID_getetag:
      if (resource->type == DAV_RESOURCE_TYPE_PRIVATE
          && (resource->info->restype == DAV_SVN_RESTYPE_VCC
              || resource->info->restype == DAV_SVN_RESTYPE_ME))
        {
          return DAV_PROP_INSERT_NOTSUPP;
        }

      value = dav_svn__getetag(resource, scratch_pool);
      break;

    case DAV_PROPID_auto_version:
      /* we only support one autoversioning behavior, and thus only
         return this one static value; someday when we support
         locking, there are other possible values/behaviors for this. */
      if (resource->info->repos->autoversioning)
        value = "DAV:checkout-checkin";
      else
        return DAV_PROP_INSERT_NOTDEF;
      break;

    case DAV_PROPID_baseline_collection:
      /* only defined for Baselines */
      /* ### whoops. also defined for a VCC. deal with it later. */
      if (resource->type != DAV_RESOURCE_TYPE_VERSION || !resource->baselined)
        return DAV_PROP_INSERT_NOTSUPP;
      value = dav_svn__build_uri(resource->info->repos, DAV_SVN__BUILD_URI_BC,
                                 resource->info->root.rev, NULL,
                                 1 /* add_href */, scratch_pool);
      break;

    case DAV_PROPID_checked_in:
      /* only defined for VCRs (in the public space and in a BC space) */
      /* ### note that a VCC (a special VCR) is defined as _PRIVATE for now */
      if (resource->type == DAV_RESOURCE_TYPE_PRIVATE
          && (resource->info->restype == DAV_SVN_RESTYPE_VCC
              || resource->info->restype == DAV_SVN_RESTYPE_ME))
        {
          svn_revnum_t revnum;

          serr = svn_fs_youngest_rev(&revnum, resource->info->repos->fs,
                                     scratch_pool);
          if (serr != NULL)
            {
              ap_log_rerror(APLOG_MARK, APLOG_ERR, serr->apr_err, 
                            resource->info->r,
                            "Can't get youngest revision in '%s': "
                            "%s",
                            svn_fs_path(resource->info->repos->fs,
                                        scratch_pool),
                            serr->message);
              svn_error_clear(serr);
              value = "###error###";
              break;
            }
          s = dav_svn__build_uri(resource->info->repos,
                                 DAV_SVN__BUILD_URI_BASELINE,
                                 revnum, NULL, 0 /* add_href */, scratch_pool);
          value = apr_psprintf(scratch_pool, "<D:href>%s</D:href>",
                               apr_xml_quote_string(scratch_pool, s, 1));
        }
      else if (resource->type != DAV_RESOURCE_TYPE_REGULAR)
        {
          /* not defined for this resource type */
          return DAV_PROP_INSERT_NOTSUPP;
        }
      else
        {
          svn_revnum_t rev_to_use =
            dav_svn__get_safe_cr(resource->info->root.root,
                                 resource->info->repos_path, scratch_pool);

          s = dav_svn__build_uri(resource->info->repos,
                                 DAV_SVN__BUILD_URI_VERSION,
                                 rev_to_use, resource->info->repos_path,
                                0 /* add_href */, scratch_pool);
          value = apr_psprintf(scratch_pool, "<D:href>%s</D:href>",
                               apr_xml_quote_string(scratch_pool, s, 1));
        }
      break;

    case DAV_PROPID_version_controlled_configuration:
      /* only defined for VCRs */
      /* ### VCRs within the BC should not have this property! */
      /* ### note that a VCC (a special VCR) is defined as _PRIVATE for now */
      if (resource->type != DAV_RESOURCE_TYPE_REGULAR)
        return DAV_PROP_INSERT_NOTSUPP;
      value = dav_svn__build_uri(resource->info->repos, DAV_SVN__BUILD_URI_VCC,
                                 SVN_IGNORED_REVNUM, NULL,
                                 1 /* add_href */, scratch_pool);
      break;

    case DAV_PROPID_version_name:
      /* only defined for Version Resources and Baselines */
      /* ### whoops. also defined for VCRs. deal with it later. */
      if ((resource->type != DAV_RESOURCE_TYPE_VERSION)
          && (! resource->versioned))
        return DAV_PROP_INSERT_NOTSUPP;

      if (resource->type == DAV_RESOURCE_TYPE_PRIVATE
          && (resource->info->restype == DAV_SVN_RESTYPE_VCC
              || resource->info->restype == DAV_SVN_RESTYPE_ME))
        {
          return DAV_PROP_INSERT_NOTSUPP;
        }

      if (resource->baselined)
        {
          /* just the revision number for baselines */
          value = apr_psprintf(scratch_pool, "%ld",
                               resource->info->root.rev);
        }
      else
        {
          svn_revnum_t committed_rev = SVN_INVALID_REVNUM;

          /* Get the CR field out of the node's skel.  Notice that the
             root object might be an ID root -or- a revision root. */
          serr = svn_fs_node_created_rev(&committed_rev,
                                         resource->info->root.root,
                                         resource->info->repos_path,
                                         scratch_pool);
          if (serr != NULL)
            {
              ap_log_rerror(APLOG_MARK, APLOG_ERR, serr->apr_err, 
                            resource->info->r,
                            "Can't get created-rev of '%s': "
                            "%s",
                            resource->info->repos_path,
                            serr->message);
              svn_error_clear(serr);
              value = "###error###";
              break;
            }

          /* Convert the revision into a quoted string */
          s = apr_psprintf(scratch_pool, "%ld", committed_rev);
          value = apr_xml_quote_string(scratch_pool, s, 1);
        }
      break;

    case SVN_PROPID_baseline_relative_path:
      /* only defined for VCRs */
      /* ### VCRs within the BC should not have this property! */
      /* ### note that a VCC (a special VCR) is defined as _PRIVATE for now */
      if (resource->type != DAV_RESOURCE_TYPE_REGULAR)
        return DAV_PROP_INSERT_NOTSUPP;

      /* drop the leading slash, so it is relative */
      s = resource->info->repos_path + 1;
      value = apr_xml_quote_string(scratch_pool, s, 1);
      break;

    case SVN_PROPID_md5_checksum:
      if ((! resource->collection)
          && (! resource->baselined)
          && (resource->type == DAV_RESOURCE_TYPE_REGULAR
              || resource->type == DAV_RESOURCE_TYPE_WORKING
              || resource->type == DAV_RESOURCE_TYPE_VERSION))
        {
          svn_node_kind_t kind;
          svn_checksum_t *checksum;

          serr = svn_fs_check_path(&kind, resource->info->root.root,
                                   resource->info->repos_path, scratch_pool);
          if (!serr && kind == svn_node_file)
            serr = svn_fs_file_checksum(&checksum, svn_checksum_md5,
                                        resource->info->root.root,
                                        resource->info->repos_path, TRUE,
                                        scratch_pool);
          if (serr != NULL)
            {
              ap_log_rerror(APLOG_MARK, APLOG_ERR, serr->apr_err, 
                            resource->info->r,
                            "Can't fetch or compute MD5 checksum of '%s': "
                            "%s",
                            resource->info->repos_path,
                            serr->message);
              svn_error_clear(serr);
              value = "###error###";
              break;
            }

          if (kind != svn_node_file)
            return DAV_PROP_INSERT_NOTSUPP;

          value = svn_checksum_to_cstring(checksum, scratch_pool);

          if (! value)
            return DAV_PROP_INSERT_NOTSUPP;
        }
      else
        return DAV_PROP_INSERT_NOTSUPP;

      break;

    case SVN_PROPID_repository_uuid:
      serr = svn_fs_get_uuid(resource->info->repos->fs, &value, scratch_pool);
      if (serr != NULL)
        {
          ap_log_rerror(APLOG_MARK, APLOG_ERR, serr->apr_err, 
                        resource->info->r,
                        "Can't fetch UUID of '%s': "
                        "%s",
                        svn_fs_path(resource->info->repos->fs, scratch_pool),
                        serr->message);
          svn_error_clear(serr);
          value = "###error###";
          break;
        }
      break;

    case SVN_PROPID_deadprop_count:
      {
        unsigned int propcount;
        apr_hash_t *proplist;

        if (resource->type != DAV_RESOURCE_TYPE_REGULAR)
          return DAV_PROP_INSERT_NOTSUPP;

        serr = svn_fs_node_proplist(&proplist,
                                    resource->info->root.root,
                                    resource->info->repos_path, scratch_pool);
        if (serr != NULL)
          {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, serr->apr_err, 
                          resource->info->r,
                          "Can't fetch proplist of '%s': "
                          "%s",
                          resource->info->repos_path,
                          serr->message);
            svn_error_clear(serr);
            value = "###error###";
            break;
          }

        propcount = apr_hash_count(proplist);
        value = apr_psprintf(scratch_pool, "%u", propcount);
        break;
      }

    default:
      /* ### what the heck was this property? */
      return DAV_PROP_INSERT_NOTDEF;
    }

  /* assert: value != NULL */

  /* get the information and global NS index for the property */
  global_ns = dav_get_liveprop_info(propid, &dav_svn__liveprop_group, &info);

  /* assert: info != NULL && info->name != NULL */

  if (what == DAV_PROP_INSERT_NAME
      || (what == DAV_PROP_INSERT_VALUE && *value == '\0')) {
    s = apr_psprintf(result_pool, "<lp%d:%s/>" DEBUG_CR, global_ns,
                     info->name);
  }
  else if (what == DAV_PROP_INSERT_VALUE) {
    s = apr_psprintf(result_pool, "<lp%d:%s>%s</lp%d:%s>" DEBUG_CR,
                     global_ns, info->name, value, global_ns, info->name);
  }
  else {
    /* assert: what == DAV_PROP_INSERT_SUPPORTED */
    s = apr_psprintf(result_pool,
                     "<D:supported-live-property D:name=\"%s\" "
                     "D:namespace=\"%s\"/>" DEBUG_CR,
                     info->name, namespace_uris[info->ns]);
  }
  apr_text_append(result_pool, phdr, s);

  /* we inserted whatever was asked for */
  return what;
}

static dav_prop_insert
insert_prop(const dav_resource *resource,
            int propid,
            dav_prop_insert what,
            apr_text_header *phdr)
{
  apr_pool_t *result_pool = resource->pool;
  apr_pool_t *scratch_pool;
  dav_prop_insert rv;

  /* Create subpool and destroy on return, because mod_dav doesn't provide
     scratch pool for insert_prop() callback. */
  scratch_pool = svn_pool_create(result_pool);

  rv = insert_prop_internal(resource, propid, what, phdr,
                              scratch_pool, result_pool);

  svn_pool_destroy(scratch_pool);
  return rv;
}

static int
is_writable(const dav_resource *resource, int propid)
{
  const dav_liveprop_spec *info;

  (void) dav_get_liveprop_info(propid, &dav_svn__liveprop_group, &info);
  return info->is_writable;
}


static dav_error *
patch_validate(const dav_resource *resource,
               const apr_xml_elem *elem,
               int operation,
               void **context,
               int *defer_to_dead)
{
  /* NOTE: this function will not be called unless/until we have
     modifiable (writable) live properties. */
  return NULL;
}


static dav_error *
patch_exec(const dav_resource *resource,
           const apr_xml_elem *elem,
           int operation,
           void *context,
           dav_liveprop_rollback **rollback_ctx)
{
  /* NOTE: this function will not be called unless/until we have
     modifiable (writable) live properties. */
  return NULL;
}


static void
patch_commit(const dav_resource *resource,
             int operation,
             void *context,
             dav_liveprop_rollback *rollback_ctx)
{
  /* NOTE: this function will not be called unless/until we have
     modifiable (writable) live properties. */
}

static dav_error *
patch_rollback(const dav_resource *resource,
                       int operation,
                       void *context,
                       dav_liveprop_rollback *rollback_ctx)
{
  /* NOTE: this function will not be called unless/until we have
     modifiable (writable) live properties. */
  return NULL;
}


static const dav_hooks_liveprop hooks_liveprop = {
  insert_prop,
  is_writable,
  namespace_uris,
  patch_validate,
  patch_exec,
  patch_commit,
  patch_rollback,
};


const dav_liveprop_group dav_svn__liveprop_group =
{
  props,
  namespace_uris,
  &hooks_liveprop
};


void
dav_svn__gather_propsets(apr_array_header_t *uris)
{
  /* ### what should we use for a URL to describe the available prop set? */
  /* ### for now... nothing. we will *only* have DAV properties */
#if 0
    *(const char **)apr_array_push(uris) =
        "<http://subversion.tigris.org/dav/propset/svn/1>";
#endif
}


int
dav_svn__find_liveprop(const dav_resource *resource,
                       const char *ns_uri,
                       const char *name,
                       const dav_hooks_liveprop **hooks)
{
  /* don't try to find any liveprops if this isn't "our" resource */
  if (resource->hooks != &dav_svn__hooks_repository)
    return 0;

  return dav_do_find_liveprop(ns_uri, name, &dav_svn__liveprop_group, hooks);
}


void
dav_svn__insert_all_liveprops(request_rec *r,
                              const dav_resource *resource,
                              dav_prop_insert what,
                              apr_text_header *phdr)
{
  const dav_liveprop_spec *spec;
  apr_pool_t *iterpool;

  /* don't insert any liveprops if this isn't "our" resource */
  if (resource->hooks != &dav_svn__hooks_repository)
    return;

  if (!resource->exists)
    {
      /* a lock-null resource */
      /*
       ** ### technically, we should insert empty properties. dunno offhand
       ** ### what part of the spec said this, but it was essentially thus:
       ** ### "the properties should be defined, but may have no value".
       */
      return;
    }

  iterpool = svn_pool_create(resource->pool);
  for (spec = props; spec->name != NULL; ++spec)
    {
      svn_pool_clear(iterpool);
      (void) insert_prop_internal(resource, spec->propid, what, phdr,
                                  iterpool, resource->pool);
    }
  svn_pool_destroy(iterpool);

  /* ### we know the others aren't defined as liveprops */
}

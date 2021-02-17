/*
 * version.c: mod_dav_svn versioning provider functions for Subversion
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
#include <apr_uuid.h>

#include <httpd.h>
#include <http_log.h>
#include <mod_dav.h>

#include "svn_fs.h"
#include "svn_xml.h"
#include "svn_repos.h"
#include "svn_dav.h"
#include "svn_time.h"
#include "svn_pools.h"
#include "svn_props.h"
#include "svn_dav.h"
#include "svn_base64.h"
#include "private/svn_repos_private.h"
#include "private/svn_dav_protocol.h"
#include "private/svn_log.h"
#include "private/svn_fspath.h"

#include "dav_svn.h"


svn_error_t *
dav_svn__attach_auto_revprops(svn_fs_txn_t *txn,
                              const char *fs_path,
                              apr_pool_t *pool)
{
  const char *logmsg;
  svn_string_t *logval;
  svn_error_t *serr;

  logmsg = apr_psprintf(pool,
                        "Autoversioning commit:  a non-deltaV client made "
                        "a change to\n%s", fs_path);

  logval = svn_string_create(logmsg, pool);
  if ((serr = svn_repos_fs_change_txn_prop(txn, SVN_PROP_REVISION_LOG, logval,
                                           pool)))
    return serr;

  /* Notate that this revision was created by autoversioning.  (Tools
     like post-commit email scripts might not care to send an email
     for every autoversioning change.) */
  if ((serr = svn_repos_fs_change_txn_prop(txn,
                                           SVN_PROP_REVISION_AUTOVERSIONED,
                                           svn_string_create("*", pool),
                                           pool)))
    return serr;

  return SVN_NO_ERROR;
}


/* Helper: attach an auto-generated svn:log property to a txn within
   an auto-checked-out working resource. */
static dav_error *
set_auto_revprops(dav_resource *resource)
{
  svn_error_t *serr;

  if (! (resource->type == DAV_RESOURCE_TYPE_WORKING
         && resource->info->auto_checked_out))
    return dav_svn__new_error(resource->pool, HTTP_INTERNAL_SERVER_ERROR, 0,
                              "Set_auto_revprops called on invalid resource.");

  if ((serr = dav_svn__attach_auto_revprops(resource->info->root.txn,
                                            resource->info->repos_path,
                                            resource->pool)))
    return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                "Error setting a revision property "
                                " on auto-checked-out resource's txn. ",
                                resource->pool);
  return NULL;
}


static dav_error *
open_txn(svn_fs_txn_t **ptxn,
         svn_fs_t *fs,
         const char *txn_name,
         apr_pool_t *pool)
{
  svn_error_t *serr;

  serr = svn_fs_open_txn(ptxn, fs, txn_name, pool);
  if (serr != NULL)
    {
      if (serr->apr_err == SVN_ERR_FS_NO_SUCH_TRANSACTION)
        {
          /* ### correct HTTP error? */
          return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                      "The transaction specified by the "
                                      "activity does not exist",
                                      pool);
        }

      /* ### correct HTTP error? */
      return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                  "There was a problem opening the "
                                  "transaction specified by this "
                                  "activity.",
                                  pool);
    }

  return NULL;
}


static void
get_vsn_options(apr_pool_t *p, apr_text_header *phdr)
{
  /* Note: we append pieces with care for Web Folders's 63-char limit
     on the DAV: header */

  apr_text_append(p, phdr,
                  "version-control,checkout,working-resource");
  apr_text_append(p, phdr,
                  "merge,baseline,activity,version-controlled-collection");
  /* Send SVN_RA_CAPABILITY_* capabilities. */
  apr_text_append(p, phdr, SVN_DAV_NS_DAV_SVN_DEPTH);
  apr_text_append(p, phdr, SVN_DAV_NS_DAV_SVN_LOG_REVPROPS);
  apr_text_append(p, phdr, SVN_DAV_NS_DAV_SVN_ATOMIC_REVPROPS);
  apr_text_append(p, phdr, SVN_DAV_NS_DAV_SVN_PARTIAL_REPLAY);
  /* Mergeinfo is a special case: here we merely say that the server
   * knows how to handle mergeinfo -- whether the repository does too
   * is a separate matter.
   *
   * Think of it as offering the client an early out: if the server
   * can't do merge-tracking, there's no point finding out of the
   * repository can.  But if the server can, it may be worth expending
   * an extra round trip to find out if the repository can too (the
   * extra round trip being necessary because, sadly, we don't have
   * access to the repository yet here, so we can only announce the
   * server capability and remain agnostic about the repository).
   */
  apr_text_append(p, phdr, SVN_DAV_NS_DAV_SVN_MERGEINFO);

  /* ### fork-control? */
}


static dav_error *
get_option(const dav_resource *resource,
           const apr_xml_elem *elem,
           apr_text_header *option)
{
  request_rec *r = resource->info->r;
  const char *repos_root_uri =
    dav_svn__build_uri(resource->info->repos, DAV_SVN__BUILD_URI_PUBLIC,
                       SVN_IGNORED_REVNUM, "", 0, resource->pool);

  /* ### DAV:version-history-collection-set */
  if (elem->ns == APR_XML_NS_DAV_ID)
    {
      if (strcmp(elem->name, "activity-collection-set") == 0)
        {
          apr_text_append(resource->pool, option,
                          "<D:activity-collection-set>");
          apr_text_append(resource->pool, option,
                          dav_svn__build_uri(resource->info->repos,
                                             DAV_SVN__BUILD_URI_ACT_COLLECTION,
                                             SVN_INVALID_REVNUM, NULL,
                                             1 /* add_href */,
                                             resource->pool));
          apr_text_append(resource->pool, option,
                          "</D:activity-collection-set>");
        }
    }

  if (resource->info->repos->fs)
    {
      svn_error_t *serr;
      svn_revnum_t youngest;
      const char *uuid;

      /* Got youngest revision? */
      if ((serr = svn_fs_youngest_rev(&youngest, resource->info->repos->fs,
                                      resource->pool)))
        {
          return dav_svn__convert_err
            (serr, HTTP_INTERNAL_SERVER_ERROR,
             "Error fetching youngest revision from repository",
             resource->pool);
        }
      if (SVN_IS_VALID_REVNUM(youngest))
        {
          apr_table_set(r->headers_out,
                        SVN_DAV_YOUNGEST_REV_HEADER,
                        apr_psprintf(resource->pool, "%ld", youngest));
        }

      /* Got repository UUID? */
      if ((serr = svn_fs_get_uuid(resource->info->repos->fs,
                                  &uuid, resource->pool)))
        {
          return dav_svn__convert_err
            (serr, HTTP_INTERNAL_SERVER_ERROR,
             "Error fetching repository UUID",
             resource->pool);
        }
      if (uuid)
        {
          apr_table_set(r->headers_out,
                        SVN_DAV_REPOS_UUID_HEADER, uuid);
        }
    }

  /* Welcome to the 2nd generation of the svn HTTP protocol, now
     DeltaV-free!  If we're configured to advise this support, do so.  */
  if (resource->info->repos->v2_protocol)
    {
      apr_table_set(r->headers_out, SVN_DAV_ROOT_URI_HEADER, repos_root_uri);
      apr_table_set(r->headers_out, SVN_DAV_ME_RESOURCE_HEADER,
                    apr_pstrcat(resource->pool, repos_root_uri, "/",
                                dav_svn__get_me_resource_uri(r), (char *)NULL));
      apr_table_set(r->headers_out, SVN_DAV_REV_ROOT_STUB_HEADER,
                    apr_pstrcat(resource->pool, repos_root_uri, "/",
                                dav_svn__get_rev_root_stub(r), (char *)NULL));
      apr_table_set(r->headers_out, SVN_DAV_REV_STUB_HEADER,
                    apr_pstrcat(resource->pool, repos_root_uri, "/",
                                dav_svn__get_rev_stub(r), (char *)NULL));
      apr_table_set(r->headers_out, SVN_DAV_TXN_ROOT_STUB_HEADER,
                    apr_pstrcat(resource->pool, repos_root_uri, "/",
                                dav_svn__get_txn_root_stub(r), (char *)NULL));
      apr_table_set(r->headers_out, SVN_DAV_TXN_STUB_HEADER,
                    apr_pstrcat(resource->pool, repos_root_uri, "/",
                                dav_svn__get_txn_stub(r), (char *)NULL));
      apr_table_set(r->headers_out, SVN_DAV_VTXN_ROOT_STUB_HEADER,
                    apr_pstrcat(resource->pool, repos_root_uri, "/",
                                dav_svn__get_vtxn_root_stub(r), (char *)NULL));
      apr_table_set(r->headers_out, SVN_DAV_VTXN_STUB_HEADER,
                    apr_pstrcat(resource->pool, repos_root_uri, "/",
                                dav_svn__get_vtxn_stub(r), (char *)NULL));
    }

  return NULL;
}


static int
versionable(const dav_resource *resource)
{
  return 0;
}


static dav_auto_version
auto_versionable(const dav_resource *resource)
{
  /* The svn client attempts to proppatch a baseline when changing
     unversioned revision props.  Thus we allow baselines to be
     "auto-checked-out" by mod_dav.  See issue #916. */
  if (resource->type == DAV_RESOURCE_TYPE_VERSION
      && resource->baselined)
    return DAV_AUTO_VERSION_ALWAYS;

  /* No other autoversioning is allowed unless the SVNAutoversioning
     directive is used. */
  if (resource->info->repos->autoversioning)
    {
      /* This allows a straight-out PUT on a public file or collection
         VCR.  mod_dav's auto-versioning subsystem will check to see if
         it's possible to auto-checkout a regular resource. */
      if (resource->type == DAV_RESOURCE_TYPE_REGULAR)
        return DAV_AUTO_VERSION_ALWAYS;

      /* mod_dav's auto-versioning subsystem will also check to see if
         it's possible to auto-checkin a working resource that was
         auto-checked-out.  We *only* allow auto-versioning on a working
         resource if it was auto-checked-out. */
      if (resource->type == DAV_RESOURCE_TYPE_WORKING
          && resource->info->auto_checked_out)
        return DAV_AUTO_VERSION_ALWAYS;
    }

  /* Default:  whatever it is, assume it's not auto-versionable */
  return DAV_AUTO_VERSION_NEVER;
}


static dav_error *
vsn_control(dav_resource *resource, const char *target)
{
  /* All mod_dav_svn resources are versioned objects;  so it doesn't
     make sense to call vsn_control on a resource that exists . */
  if (resource->exists)
    return dav_svn__new_error(resource->pool, HTTP_BAD_REQUEST, 0,
                              "vsn_control called on already-versioned "
                              "resource.");

  /* Only allow a NULL target, which means an create an 'empty' VCR. */
  if (target != NULL)
    return dav_svn__new_error_tag(resource->pool, HTTP_NOT_IMPLEMENTED,
                                  SVN_ERR_UNSUPPORTED_FEATURE,
                                  "vsn_control called with non-null target.",
                                  SVN_DAV_ERROR_NAMESPACE,
                                  SVN_DAV_ERROR_TAG);

  /* This is kind of silly.  The docstring for this callback says it's
     supposed to "put a resource under version control".  But in
     Subversion, all REGULAR resources (bc's or public URIs) are
     already under version control. So we don't need to do a thing to
     the resource, just return. */
  return NULL;
}


dav_error *
dav_svn__checkout(dav_resource *resource,
                  int auto_checkout,
                  int is_unreserved,
                  int is_fork_ok,
                  int create_activity,
                  apr_array_header_t *activities,
                  dav_resource **working_resource)
{
  const char *txn_name;
  svn_error_t *serr;
  apr_status_t apr_err;
  dav_error *derr;
  dav_svn__uri_info parse;

  /* Auto-Versioning Stuff */
  if (auto_checkout)
    {
      const char *uuid_buf;
      void *data;
      const char *shared_activity, *shared_txn_name = NULL;

      /* Baselines can be auto-checked-out -- grudgingly -- so we can
         allow clients to proppatch unversioned rev props.  See issue
         #916. */
      if ((resource->type == DAV_RESOURCE_TYPE_VERSION)
          && resource->baselined)
        /* ### We're violating deltaV big time here, by allowing a
           dav_auto_checkout() on something that mod_dav assumes is a
           VCR, not a VR.  Anyway, mod_dav thinks we're checking out the
           resource 'in place', so that no working resource is returned.
           (It passes NULL as **working_resource.)  */
        return NULL;

      if (resource->type != DAV_RESOURCE_TYPE_REGULAR)
        return dav_svn__new_error_tag(resource->pool, HTTP_METHOD_NOT_ALLOWED,
                                      SVN_ERR_UNSUPPORTED_FEATURE,
                                      "auto-checkout attempted on non-regular "
                                      "version-controlled resource.",
                                      SVN_DAV_ERROR_NAMESPACE,
                                      SVN_DAV_ERROR_TAG);

      if (resource->baselined)
        return dav_svn__new_error_tag(resource->pool, HTTP_METHOD_NOT_ALLOWED,
                                      SVN_ERR_UNSUPPORTED_FEATURE,
                                      "auto-checkout attempted on baseline "
                                      "collection, which is not supported.",
                                      SVN_DAV_ERROR_NAMESPACE,
                                      SVN_DAV_ERROR_TAG);

      /* See if the shared activity already exists. */
      apr_err = apr_pool_userdata_get(&data,
                                      DAV_SVN__AUTOVERSIONING_ACTIVITY,
                                      resource->info->r->pool);
      if (apr_err)
        return dav_svn__convert_err(svn_error_create(apr_err, 0, NULL),
                                    HTTP_INTERNAL_SERVER_ERROR,
                                    "Error fetching pool userdata.",
                                    resource->pool);
      shared_activity = data;

      if (! shared_activity)
        {
          /* Build a shared activity for all auto-checked-out resources. */
          uuid_buf = svn_uuid_generate(resource->info->r->pool);
          shared_activity = apr_pstrdup(resource->info->r->pool, uuid_buf);

          derr = dav_svn__create_txn(resource->info->repos, &shared_txn_name,
                                     resource->info->r->pool);
          if (derr) return derr;

          derr = dav_svn__store_activity(resource->info->repos,
                                         shared_activity, shared_txn_name);
          if (derr) return derr;

          /* Save the shared activity in r->pool for others to use. */
          apr_err = apr_pool_userdata_set(shared_activity,
                                          DAV_SVN__AUTOVERSIONING_ACTIVITY,
                                          NULL, resource->info->r->pool);
          if (apr_err)
            return dav_svn__convert_err(svn_error_create(apr_err, 0, NULL),
                                        HTTP_INTERNAL_SERVER_ERROR,
                                        "Error setting pool userdata.",
                                        resource->pool);
        }

      if (! shared_txn_name)
        {
          shared_txn_name = dav_svn__get_txn(resource->info->repos,
                                             shared_activity);
          if (! shared_txn_name)
            return dav_svn__new_error(resource->pool,
                                      HTTP_INTERNAL_SERVER_ERROR, 0,
                                      "Cannot look up a txn_name by activity");
        }

      /* Tweak the VCR in-place, making it into a WR.  (Ignore the
         NULL return value.) */
      dav_svn__create_working_resource(resource,
                                       shared_activity, shared_txn_name,
                                       TRUE /* tweak in place */);

      /* Remember that this resource was auto-checked-out, so that
         auto_versionable allows us to do an auto-checkin and
         can_be_activity will allow this resource to be an
         activity. */
      resource->info->auto_checked_out = TRUE;

      /* The txn and txn_root must be open and ready to go in the
         resource's root object.  Normally prep_resource() will do
         this automatically on a WR's root object.  We're
         converting a VCR to WR forcibly, so it's now our job to
         make sure it happens. */
      derr = open_txn(&resource->info->root.txn, resource->info->repos->fs,
                      resource->info->root.txn_name, resource->pool);
      if (derr) return derr;

      serr = svn_fs_txn_root(&resource->info->root.root,
                             resource->info->root.txn, resource->pool);
      if (serr != NULL)
        return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                    "Could not open a (transaction) root "
                                    "in the repository",
                                    resource->pool);
      return NULL;
    }
  /* end of Auto-Versioning Stuff */

  if (resource->type != DAV_RESOURCE_TYPE_VERSION)
    {
      return dav_svn__new_error_tag(resource->pool, HTTP_METHOD_NOT_ALLOWED,
                                    SVN_ERR_UNSUPPORTED_FEATURE,
                                    "CHECKOUT can only be performed on a "
                                    "version resource [at this time].",
                                    SVN_DAV_ERROR_NAMESPACE,
                                    SVN_DAV_ERROR_TAG);
    }
  if (create_activity)
    {
      return dav_svn__new_error_tag(resource->pool, HTTP_NOT_IMPLEMENTED,
                                    SVN_ERR_UNSUPPORTED_FEATURE,
                                    "CHECKOUT cannot create an activity at "
                                    "this time. Use MKACTIVITY first.",
                                    SVN_DAV_ERROR_NAMESPACE,
                                    SVN_DAV_ERROR_TAG);
    }
  if (is_unreserved)
    {
      return dav_svn__new_error_tag(resource->pool, HTTP_NOT_IMPLEMENTED,
                                    SVN_ERR_UNSUPPORTED_FEATURE,
                                    "Unreserved checkouts are not yet "
                                    "available. A version history may not be "
                                    "checked out more than once, into a "
                                    "specific activity.",
                                    SVN_DAV_ERROR_NAMESPACE,
                                    SVN_DAV_ERROR_TAG);
    }
  if (activities == NULL)
    {
      return dav_svn__new_error_tag(resource->pool, HTTP_CONFLICT,
                                    SVN_ERR_INCOMPLETE_DATA,
                                    "An activity must be provided for "
                                    "checkout.",
                                    SVN_DAV_ERROR_NAMESPACE,
                                    SVN_DAV_ERROR_TAG);
    }
  /* assert: nelts > 0.  the below check effectively means > 1. */
  if (activities->nelts != 1)
    {
      return dav_svn__new_error_tag(resource->pool, HTTP_CONFLICT,
                                    SVN_ERR_INCORRECT_PARAMS,
                                    "Only one activity may be specified within "
                                    "the CHECKOUT.",
                                    SVN_DAV_ERROR_NAMESPACE,
                                    SVN_DAV_ERROR_TAG);
    }

  serr = dav_svn__simple_parse_uri(&parse, resource,
                                   APR_ARRAY_IDX(activities, 0, const char *),
                                   resource->pool);
  if (serr != NULL)
    {
      /* ### is BAD_REQUEST proper? */
      return dav_svn__convert_err(serr, HTTP_CONFLICT,
                                  "The activity href could not be parsed "
                                  "properly.",
                                  resource->pool);
    }
  if (parse.activity_id == NULL)
    {
      return dav_svn__new_error_tag(resource->pool, HTTP_CONFLICT,
                                    SVN_ERR_INCORRECT_PARAMS,
                                    "The provided href is not an activity URI.",
                                    SVN_DAV_ERROR_NAMESPACE,
                                    SVN_DAV_ERROR_TAG);
    }

  if ((txn_name = dav_svn__get_txn(resource->info->repos,
                                   parse.activity_id)) == NULL)
    {
      return dav_svn__new_error_tag(resource->pool, HTTP_CONFLICT,
                                    SVN_ERR_APMOD_ACTIVITY_NOT_FOUND,
                                    "The specified activity does not exist.",
                                    SVN_DAV_ERROR_NAMESPACE,
                                    SVN_DAV_ERROR_TAG);
    }

  /* verify the specified version resource is the "latest", thus allowing
     changes to be made. */
  if (resource->baselined || resource->info->root.rev == SVN_INVALID_REVNUM)
    {
      /* a Baseline, or a standard Version Resource which was accessed
         via a Label against a VCR within a Baseline Collection. */
      /* ### at the moment, this branch is only reached for baselines */

      svn_revnum_t youngest;

      /* make sure the baseline being checked out is the latest */
      serr = svn_fs_youngest_rev(&youngest, resource->info->repos->fs,
                                 resource->pool);
      if (serr != NULL)
        {
          /* ### correct HTTP error? */
          return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                      "Could not determine the youngest "
                                      "revision for verification against "
                                      "the baseline being checked out.",
                                      resource->pool);
        }

      if (resource->info->root.rev != youngest)
        {
          return dav_svn__new_error_tag(resource->pool, HTTP_CONFLICT,
                                        SVN_ERR_APMOD_BAD_BASELINE,
                                        "The specified baseline is not the "
                                        "latest baseline, so it may not be "
                                        "checked out.",
                                        SVN_DAV_ERROR_NAMESPACE,
                                        SVN_DAV_ERROR_TAG);
        }

      /* ### hmm. what if the transaction root's revision is different
         ### from this baseline? i.e. somebody created a new revision while
         ### we are processing this commit.
         ###
         ### first question: what does the client *do* with a working
         ### baseline? knowing that, and how it maps to our backend, then
         ### we can figure out what to do here. */
    }
  else
    {
      /* standard Version Resource */

      svn_fs_txn_t *txn;
      svn_fs_root_t *txn_root;
      svn_revnum_t txn_created_rev;
      dav_error *err;

      /* open the specified transaction so that we can verify this version
         resource corresponds to the current/latest in the transaction. */
      if ((err = open_txn(&txn, resource->info->repos->fs, txn_name,
                          resource->pool)) != NULL)
        return err;

      serr = svn_fs_txn_root(&txn_root, txn, resource->pool);
      if (serr != NULL)
        {
          /* ### correct HTTP error? */
          return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                     "Could not open the transaction tree.",
                                     resource->pool);
        }

      /* assert: repos_path != NULL (for this type of resource) */


      /* Out-of-dateness check:  compare the created-rev of the item
         in the txn against the created-rev of the version resource
         being changed. */
      serr = svn_fs_node_created_rev(&txn_created_rev,
                                     txn_root, resource->info->repos_path,
                                     resource->pool);
      if (serr != NULL)
        {
          /* ### correct HTTP error? */
          return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                      "Could not get created-rev of "
                                      "transaction node.",
                                      resource->pool);
        }

      /* If txn_created_rev is invalid, that means it's already
         mutable in the txn... which means it has already passed this
         out-of-dateness check.  (Usually, this happens when looking
         at a parent directory of an already checked-out
         resource.)

         Now, we come down to it.  If the created revision of the node
         in the transaction is different from the revision parsed from
         the version resource URL, we're in a bit of a quandry, and
         one of a few things could be true.

         - The client is trying to modify an old (out-of-date)
           revision of the resource.  This is, of course,
           unacceptable!

         - The client is trying to modify a *newer* revision.  If the
           version resource is *newer* than the transaction root, then
           the client started a commit, a new revision was created
           within the repository, the client fetched the new resource
           from that new revision, changed it (or merged in a prior
           change), and then attempted to incorporate that into the
           commit that was initially started.  We could copy that new
           node into our transaction and then modify it, but why
           bother?  We can stop the commit, and everything will be
           fine again if the user simply restarts it (because we'll
           use that new revision as the transaction root, thus
           incorporating the new resource, which they will then
           modify).

         - The path/revision that client is wishing to edit and the
           path/revision in the current transaction are actually the
           same node, and thus this created-rev comparison didn't
           really solidify anything after all. :-)
      */

      if (SVN_IS_VALID_REVNUM( txn_created_rev ))
        {
          if (resource->info->root.rev < txn_created_rev)
            {
              /* The item being modified is older than the one in the
                 transaction.  The client is out of date.  */
              return dav_svn__new_error_tag
                (resource->pool, HTTP_CONFLICT, SVN_ERR_FS_CONFLICT,
                 "resource out of date; try updating",
                 SVN_DAV_ERROR_NAMESPACE,
                 SVN_DAV_ERROR_TAG);
            }
          else if (resource->info->root.rev > txn_created_rev)
            {
              /* The item being modified is being accessed via a newer
                 revision than the one in the transaction.  We'll
                 check to see if they are still the same node, and if
                 not, return an error. */
              const svn_fs_id_t *url_noderev_id, *txn_noderev_id;

              if ((serr = svn_fs_node_id(&txn_noderev_id, txn_root,
                                         resource->info->repos_path,
                                         resource->pool)))
                {
                  err = dav_svn__new_error_tag
                    (resource->pool, HTTP_CONFLICT, serr->apr_err,
                     "Unable to fetch the node revision id of the version "
                     "resource within the transaction.",
                     SVN_DAV_ERROR_NAMESPACE,
                     SVN_DAV_ERROR_TAG);
                  svn_error_clear(serr);
                  return err;
                }
              if ((serr = svn_fs_node_id(&url_noderev_id,
                                         resource->info->root.root,
                                         resource->info->repos_path,
                                         resource->pool)))
                {
                  err = dav_svn__new_error_tag
                    (resource->pool, HTTP_CONFLICT, serr->apr_err,
                     "Unable to fetch the node revision id of the version "
                     "resource within the revision.",
                     SVN_DAV_ERROR_NAMESPACE,
                     SVN_DAV_ERROR_TAG);
                  svn_error_clear(serr);
                  return err;
                }
              if (svn_fs_compare_ids(url_noderev_id, txn_noderev_id) != 0)
                {
                  return dav_svn__new_error_tag
                    (resource->pool, HTTP_CONFLICT, SVN_ERR_FS_CONFLICT,
                     "version resource newer than txn (restart the commit)",
                     SVN_DAV_ERROR_NAMESPACE,
                     SVN_DAV_ERROR_TAG);
                }
            }
        }
    }
  *working_resource = dav_svn__create_working_resource(resource,
                                                       parse.activity_id,
                                                       txn_name,
                                                       FALSE);
  return NULL;
}


static dav_error *
uncheckout(dav_resource *resource)
{
  if (resource->type != DAV_RESOURCE_TYPE_WORKING)
    return dav_svn__new_error_tag(resource->pool, HTTP_INTERNAL_SERVER_ERROR,
                                  SVN_ERR_UNSUPPORTED_FEATURE,
                                  "UNCHECKOUT called on non-working resource.",
                                  SVN_DAV_ERROR_NAMESPACE,
                                  SVN_DAV_ERROR_TAG);

  /* Try to abort the txn if it exists;  but don't try too hard.  :-)  */
  if (resource->info->root.txn)
    svn_error_clear(svn_fs_abort_txn(resource->info->root.txn,
                                     resource->pool));

  /* Attempt to destroy the shared activity. */
  if (resource->info->root.activity_id)
    {
      dav_svn__delete_activity(resource->info->repos,
                               resource->info->root.activity_id);
      apr_pool_userdata_set(NULL, DAV_SVN__AUTOVERSIONING_ACTIVITY,
                            NULL, resource->info->r->pool);
    }

  resource->info->root.txn_name = NULL;
  resource->info->root.txn = NULL;

  /* We're no longer checked out. */
  resource->info->auto_checked_out = FALSE;

  /* Convert the working resource back into a regular one, in-place. */
  return dav_svn__working_to_regular_resource(resource);
}


/* Closure object for cleanup_deltify. */
struct cleanup_deltify_baton
{
  /* The repository in which to deltify.  We use a path instead of an
     object, because it's difficult to obtain a repos or fs object
     with the right lifetime guarantees. */
  const char *repos_path;

  /* The revision number against which to deltify. */
  svn_revnum_t revision;

  /* The pool to use for all temporary allocation while working.  This
     may or may not be the same as the pool on which the cleanup is
     registered, but obviously it must have a lifetime at least as
     long as that pool. */
  apr_pool_t *pool;
};


/* APR pool cleanup function to deltify against a just-committed
   revision.  DATA is a 'struct cleanup_deltify_baton *'.

   If any errors occur, log them in the httpd server error log, but
   return APR_SUCCESS no matter what, as this is a pool cleanup
   function and deltification is not a matter of correctness
   anyway. */
static apr_status_t
cleanup_deltify(void *data)
{
  struct cleanup_deltify_baton *cdb = data;
  svn_repos_t *repos;
  svn_error_t *err;

  /* It's okay to allocate in the pool that's being cleaned up, and
     it's also okay to register new cleanups against that pool.  But
     if you create subpools of it, you must make sure to destroy them
     at the end of the cleanup.  So we do all our work in this
     subpool, then destroy it before exiting. */
  apr_pool_t *subpool = svn_pool_create(cdb->pool);

  err = svn_repos_open2(&repos, cdb->repos_path, NULL, subpool);
  if (err)
    {
      ap_log_perror(APLOG_MARK, APLOG_ERR, err->apr_err, cdb->pool,
                    "cleanup_deltify: error opening repository '%s'",
                    cdb->repos_path);
      svn_error_clear(err);
      goto cleanup;
    }

  err = svn_fs_deltify_revision(svn_repos_fs(repos),
                                cdb->revision, subpool);
  if (err)
    {
      ap_log_perror(APLOG_MARK, APLOG_ERR, err->apr_err, cdb->pool,
                    "cleanup_deltify: error deltifying against revision %ld"
                    " in repository '%s'",
                    cdb->revision, cdb->repos_path);
      svn_error_clear(err);
    }

 cleanup:
  svn_pool_destroy(subpool);

  return APR_SUCCESS;
}


/* Register the cleanup_deltify function on POOL, which should be the
   connection pool for the request.  This way the time needed for
   deltification won't delay the response to the client.

   REPOS is the repository in which deltify, and REVISION is the
   revision against which to deltify.  POOL is both the pool on which
   to register the cleanup function and the pool that will be used for
   temporary allocations while deltifying. */
static void
register_deltification_cleanup(svn_repos_t *repos,
                               svn_revnum_t revision,
                               apr_pool_t *pool)
{
  struct cleanup_deltify_baton *cdb = apr_palloc(pool, sizeof(*cdb));

  cdb->repos_path = svn_repos_path(repos, pool);
  cdb->revision = revision;
  cdb->pool = pool;

  apr_pool_cleanup_register(pool, cdb, cleanup_deltify, apr_pool_cleanup_null);
}


dav_error *
dav_svn__checkin(dav_resource *resource,
                 int keep_checked_out,
                 dav_resource **version_resource)
{
  svn_error_t *serr;
  dav_error *err;
  apr_status_t apr_err;
  const char *uri;
  const char *shared_activity;
  void *data;

  /* ### mod_dav has a flawed architecture, in the sense that it first
     tries to auto-checkin the modified resource, then attempts to
     auto-checkin the parent resource (if the parent resource was
     auto-checked-out).  Instead, the provider should be in charge:
     mod_dav should provide a *set* of resources that need
     auto-checkin, and the provider can decide how to do it.  (One
     txn?  Many txns?  Etc.) */

  if (resource->type != DAV_RESOURCE_TYPE_WORKING)
    return dav_svn__new_error_tag(resource->pool, HTTP_INTERNAL_SERVER_ERROR,
                                  SVN_ERR_UNSUPPORTED_FEATURE,
                                  "CHECKIN called on non-working resource.",
                                  SVN_DAV_ERROR_NAMESPACE,
                                  SVN_DAV_ERROR_TAG);

  /* If the global autoversioning activity still exists, that means
     nobody's committed it yet. */
  apr_err = apr_pool_userdata_get(&data,
                                  DAV_SVN__AUTOVERSIONING_ACTIVITY,
                                  resource->info->r->pool);
  if (apr_err)
    return dav_svn__convert_err(svn_error_create(apr_err, 0, NULL),
                                HTTP_INTERNAL_SERVER_ERROR,
                                "Error fetching pool userdata.",
                                resource->pool);
  shared_activity = data;

  /* Try to commit the txn if it exists. */
  if (shared_activity
      && (strcmp(shared_activity, resource->info->root.activity_id) == 0))
    {
      const char *shared_txn_name;
      const char *conflict_msg;
      svn_revnum_t new_rev;

      shared_txn_name = dav_svn__get_txn(resource->info->repos,
                                         shared_activity);
      if (! shared_txn_name)
        return dav_svn__new_error(resource->pool, HTTP_INTERNAL_SERVER_ERROR, 0,
                                  "Cannot look up a txn_name by activity");

      /* Sanity checks */
      if (resource->info->root.txn_name
          && (strcmp(shared_txn_name, resource->info->root.txn_name) != 0))
        return dav_svn__new_error(resource->pool, HTTP_INTERNAL_SERVER_ERROR, 0,
                                  "Internal txn_name doesn't match "
                                  "autoversioning transaction.");

      if (! resource->info->root.txn)
        /* should already be open by checkout */
        return dav_svn__new_error(resource->pool, HTTP_INTERNAL_SERVER_ERROR, 0,
                                  "Autoversioning txn isn't open "
                                  "when it should be.");

      err = set_auto_revprops(resource);
      if (err)
        return err;

      serr = svn_repos_fs_commit_txn(&conflict_msg,
                                     resource->info->repos->repos,
                                     &new_rev,
                                     resource->info->root.txn,
                                     resource->pool);

      if (SVN_IS_VALID_REVNUM(new_rev))
        {
          if (serr)
            {
              const char *post_commit_err = svn_repos__post_commit_error_str
                                              (serr, resource->pool);
              ap_log_perror(APLOG_MARK, APLOG_ERR, serr->apr_err,
                            resource->pool,
                            "commit of r%ld succeeded, but an error occurred "
                            "after the commit: '%s'",
                            new_rev,
                            post_commit_err);
              svn_error_clear(serr);
              serr = SVN_NO_ERROR;
            }
        }
      else
        {
          const char *msg;
          svn_error_clear(svn_fs_abort_txn(resource->info->root.txn,
                                           resource->pool));

          /* Attempt to destroy the shared activity. */
          dav_svn__delete_activity(resource->info->repos, shared_activity);
          apr_pool_userdata_set(NULL, DAV_SVN__AUTOVERSIONING_ACTIVITY,
                                NULL, resource->info->r->pool);

          if (serr)
            {
              int status;

              if (serr->apr_err == SVN_ERR_FS_CONFLICT)
                {
                  status = HTTP_CONFLICT;
                  msg = apr_psprintf(resource->pool,
                                     "A conflict occurred during the CHECKIN "
                                     "processing. The problem occurred with  "
                                     "the \"%s\" resource.",
                                     conflict_msg);
                }
              else
                {
                  status = HTTP_INTERNAL_SERVER_ERROR;
                  msg = "An error occurred while committing the transaction.";
                }

              return dav_svn__convert_err(serr, status, msg, resource->pool);
            }
          else
            {
              return dav_svn__new_error(resource->pool,
                                        HTTP_INTERNAL_SERVER_ERROR,
                                        0,
                                        "Commit failed but there was no error "
                                        "provided.");
            }
        }

      /* Attempt to destroy the shared activity. */
      dav_svn__delete_activity(resource->info->repos, shared_activity);
      apr_pool_userdata_set(NULL, DAV_SVN__AUTOVERSIONING_ACTIVITY,
                            NULL, resource->info->r->pool);

      /* Commit was successful, so schedule deltification. */
      register_deltification_cleanup(resource->info->repos->repos,
                                     new_rev,
                                     resource->info->r->connection->pool);

      /* If caller wants it, return the new VR that was created by
         the checkin. */
      if (version_resource)
        {
          uri = dav_svn__build_uri(resource->info->repos,
                                   DAV_SVN__BUILD_URI_VERSION,
                                   new_rev, resource->info->repos_path,
                                   0, resource->pool);

          err = dav_svn__create_version_resource(version_resource, uri,
                                                 resource->pool);
          if (err)
            return err;
        }
    } /* end of commit stuff */

  /* The shared activity was either nonexistent to begin with, or it's
     been committed and is only now nonexistent.  The resource needs
     to forget about it. */
  resource->info->root.txn_name = NULL;
  resource->info->root.txn = NULL;

  /* Convert the working resource back into an regular one. */
  if (! keep_checked_out)
    {
      resource->info->auto_checked_out = FALSE;
      return dav_svn__working_to_regular_resource(resource);
    }

  return NULL;
}


static dav_error *
avail_reports(const dav_resource *resource, const dav_report_elem **reports)
{
  /* ### further restrict to the public space? */
  if (resource->type != DAV_RESOURCE_TYPE_REGULAR) {
    *reports = NULL;
    return NULL;
  }

  *reports = dav_svn__reports_list;
  return NULL;
}


static int
report_label_header_allowed(const apr_xml_doc *doc)
{
  return 0;
}


static dav_error *
deliver_report(request_rec *r,
               const dav_resource *resource,
               const apr_xml_doc *doc,
               ap_filter_t *output)
{
  int ns = dav_svn__find_ns(doc->namespaces, SVN_XML_NAMESPACE);

  if (doc->root->ns == ns)
    {
      /* ### note that these report names should have symbols... */

      if (strcmp(doc->root->name, "update-report") == 0)
        {
          return dav_svn__update_report(resource, doc, output);
        }
      else if (strcmp(doc->root->name, "log-report") == 0)
        {
          return dav_svn__log_report(resource, doc, output);
        }
      else if (strcmp(doc->root->name, "dated-rev-report") == 0)
        {
          return dav_svn__dated_rev_report(resource, doc, output);
        }
      else if (strcmp(doc->root->name, "get-locations") == 0)
        {
          return dav_svn__get_locations_report(resource, doc, output);
        }
      else if (strcmp(doc->root->name, "get-location-segments") == 0)
        {
          return dav_svn__get_location_segments_report(resource, doc, output);
        }
      else if (strcmp(doc->root->name, "file-revs-report") == 0)
        {
          return dav_svn__file_revs_report(resource, doc, output);
        }
      else if (strcmp(doc->root->name, "get-locks-report") == 0)
        {
          return dav_svn__get_locks_report(resource, doc, output);
        }
      else if (strcmp(doc->root->name, "replay-report") == 0)
        {
          return dav_svn__replay_report(resource, doc, output);
        }
      else if (strcmp(doc->root->name, SVN_DAV__MERGEINFO_REPORT) == 0)
        {
          return dav_svn__get_mergeinfo_report(resource, doc, output);
        }
      else if (strcmp(doc->root->name, "get-deleted-rev-report") == 0)
        {
          return dav_svn__get_deleted_rev_report(resource, doc, output);
        }
      /* NOTE: if you add a report, don't forget to add it to the
       *       dav_svn__reports_list[] array.
       */
    }

  /* ### what is a good error for an unknown report? */
  return dav_svn__new_error_tag(resource->pool, HTTP_NOT_IMPLEMENTED,
                                SVN_ERR_UNSUPPORTED_FEATURE,
                                "The requested report is unknown.",
                                SVN_DAV_ERROR_NAMESPACE,
                                SVN_DAV_ERROR_TAG);
}


static int
can_be_activity(const dav_resource *resource)
{
  /* If our resource is marked as auto_checked_out'd, then we allow this to
   * be an activity URL.  Otherwise, it must be a real activity URL that
   * doesn't already exist.
   */
  return (resource->info->auto_checked_out ||
          (resource->type == DAV_RESOURCE_TYPE_ACTIVITY &&
           !resource->exists));
}


static dav_error *
make_activity(dav_resource *resource)
{
  const char *activity_id = resource->info->root.activity_id;
  const char *txn_name;
  dav_error *err;

  /* sanity check:  make sure the resource is a valid activity, in
     case an older mod_dav doesn't do the check for us. */
  if (! can_be_activity(resource))
    return dav_svn__new_error_tag(resource->pool, HTTP_FORBIDDEN,
                                  SVN_ERR_APMOD_MALFORMED_URI,
                                  "Activities cannot be created at that "
                                  "location; query the "
                                  "DAV:activity-collection-set property.",
                                  SVN_DAV_ERROR_NAMESPACE,
                                  SVN_DAV_ERROR_TAG);

  err = dav_svn__create_txn(resource->info->repos, &txn_name, resource->pool);
  if (err != NULL)
    return err;

  err = dav_svn__store_activity(resource->info->repos, activity_id, txn_name);
  if (err != NULL)
    return err;

  /* everything is happy. update the resource */
  resource->info->root.txn_name = txn_name;
  resource->exists = 1;
  return NULL;
}


dav_error *
dav_svn__build_lock_hash(apr_hash_t **locks,
                         request_rec *r,
                         const char *path_prefix,
                         apr_pool_t *pool)
{
  apr_status_t apr_err;
  dav_error *derr;
  void *data = NULL;
  apr_xml_doc *doc = NULL;
  apr_xml_elem *child, *lockchild;
  int ns;
  apr_hash_t *hash = apr_hash_make(pool);

  /* Grab the request body out of r->pool, as it contains all of the
     lock tokens.  It should have been stashed already by our custom
     input filter. */
  apr_err = apr_pool_userdata_get(&data, "svn-request-body", r->pool);
  if (apr_err)
    return dav_svn__convert_err(svn_error_create(apr_err, 0, NULL),
                                HTTP_INTERNAL_SERVER_ERROR,
                                "Error fetching pool userdata.",
                                pool);
  doc = data;
  if (! doc)
    {
      *locks = hash;
      return SVN_NO_ERROR;
    }

  /* Sanity check. */
  ns = dav_svn__find_ns(doc->namespaces, SVN_XML_NAMESPACE);
  if (ns == -1)
    {
      /* If there's no svn: namespace in the body, then there are
         definitely no lock-tokens to harvest.  This is likely a
         request from an old client. */
      *locks = hash;
      return SVN_NO_ERROR;
    }

  if ((doc->root->ns == ns)
      && (strcmp(doc->root->name, "lock-token-list") == 0))
    {
      child = doc->root;
    }
  else
    {
      /* Search doc's children until we find the <lock-token-list>. */
      for (child = doc->root->first_child; child != NULL; child = child->next)
        {
          /* if this element isn't one of ours, then skip it */
          if (child->ns != ns)
            continue;

          if (strcmp(child->name, "lock-token-list") == 0)
            break;
        }
    }

  /* Did we find what we were looking for? */
  if (! child)
    {
      *locks = hash;
      return SVN_NO_ERROR;
    }

  /* Then look for N different <lock> structures within. */
  for (lockchild = child->first_child; lockchild != NULL;
       lockchild = lockchild->next)
    {
      const char *lockpath = NULL, *locktoken = NULL;
      apr_xml_elem *lfchild;

      if (strcmp(lockchild->name, "lock") != 0)
        continue;

      for (lfchild = lockchild->first_child; lfchild != NULL;
           lfchild = lfchild->next)
        {
          if (strcmp(lfchild->name, "lock-path") == 0)
            {
              const char *cdata = dav_xml_get_cdata(lfchild, pool, 0);
              if ((derr = dav_svn__test_canonical(cdata, pool)))
                return derr;

              /* Create an absolute fs-path */
              lockpath = svn_fspath__join(path_prefix, cdata, pool);
              if (lockpath && locktoken)
                {
                  apr_hash_set(hash, lockpath, APR_HASH_KEY_STRING, locktoken);
                  lockpath = NULL;
                  locktoken = NULL;
                }
            }
          else if (strcmp(lfchild->name, "lock-token") == 0)
            {
              locktoken = dav_xml_get_cdata(lfchild, pool, 1);
              if (lockpath && *locktoken)
                {
                  apr_hash_set(hash, lockpath, APR_HASH_KEY_STRING, locktoken);
                  lockpath = NULL;
                  locktoken = NULL;
                }
            }
        }
    }

  *locks = hash;
  return SVN_NO_ERROR;
}


dav_error *
dav_svn__push_locks(dav_resource *resource,
                    apr_hash_t *locks,
                    apr_pool_t *pool)
{
  svn_fs_access_t *fsaccess;
  apr_hash_index_t *hi;
  svn_error_t *serr;

  serr = svn_fs_get_access(&fsaccess, resource->info->repos->fs);
  if (serr || !fsaccess)
    {
      /* If an authenticated user name was attached to the request,
         then dav_svn_get_resource() should have already noticed and
         created an fs_access_t in the filesystem.  */
      if (serr == NULL)
        serr = svn_error_create(SVN_ERR_FS_LOCK_OWNER_MISMATCH, NULL, NULL);
      return dav_svn__sanitize_error(serr, "Lock token(s) in request, but "
                                     "missing an user name", HTTP_BAD_REQUEST,
                                     resource->info->r);
    }

  for (hi = apr_hash_first(pool, locks); hi; hi = apr_hash_next(hi))
    {
      const char *path, *token;
      const void *key;
      void *val;
      apr_hash_this(hi, &key, NULL, &val);
      path = key, token = val;

      serr = svn_fs_access_add_lock_token2(fsaccess, path, token);
      if (serr)
        return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                    "Error pushing token into filesystem.",
                                    pool);
    }

  return NULL;
}


/* Helper for merge().  Free every lock in LOCKS.  The locks
   live in REPOS.  Log any errors for REQUEST.  Use POOL for temporary
   work.*/
static svn_error_t *
release_locks(apr_hash_t *locks,
              svn_repos_t *repos,
              request_rec *r,
              apr_pool_t *pool)
{
  apr_hash_index_t *hi;
  const void *key;
  void *val;
  apr_pool_t *subpool = svn_pool_create(pool);
  svn_error_t *err;

  for (hi = apr_hash_first(pool, locks); hi; hi = apr_hash_next(hi))
    {
      svn_pool_clear(subpool);
      apr_hash_this(hi, &key, NULL, &val);

      /* The lock may be stolen or broken sometime between
         svn_fs_commit_txn() and this post-commit cleanup.  So ignore
         any errors from this command; just free as many locks as we can. */
      err = svn_repos_fs_unlock(repos, key, val, FALSE, subpool);

      if (err) /* If we got an error, just log it and move along. */
          ap_log_rerror(APLOG_MARK, APLOG_ERR, err->apr_err, r,
                        "%s", err->message);

      svn_error_clear(err);
    }

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


static dav_error *
merge(dav_resource *target,
      dav_resource *source,
      int no_auto_merge,
      int no_checkout,
      apr_xml_elem *prop_elem,
      ap_filter_t *output)
{
  apr_pool_t *pool;
  dav_error *err;
  svn_fs_txn_t *txn;
  const char *conflict;
  svn_error_t *serr;
  const char *post_commit_err = NULL;
  svn_revnum_t new_rev;
  apr_hash_t *locks;
  svn_boolean_t disable_merge_response = FALSE;

  /* We'll use the target's pool for our operation. We happen to know that
     it matches the request pool, which (should) have the proper lifetime. */
  pool = target->pool;

  /* ### what to verify on the target? */

  /* ### anything else for the source? */
  if (! (source->type == DAV_RESOURCE_TYPE_ACTIVITY
         || (source->type == DAV_RESOURCE_TYPE_PRIVATE
             && source->info->restype == DAV_SVN_RESTYPE_TXN_COLLECTION)))
    {
      return dav_svn__new_error_tag(pool, HTTP_METHOD_NOT_ALLOWED,
                                    SVN_ERR_INCORRECT_PARAMS,
                                    "MERGE can only be performed using an "
                                    "activity or transaction resource as the "
                                    "source.",
                                    SVN_DAV_ERROR_NAMESPACE,
                                    SVN_DAV_ERROR_TAG);
    }
  if (! source->exists)
    {
      return dav_svn__new_error_tag(pool, HTTP_METHOD_NOT_ALLOWED,
                                    SVN_ERR_INCORRECT_PARAMS,
                                    "MERGE activity or transaction resource "
                                    "does not exist.",
                                    SVN_DAV_ERROR_NAMESPACE,
                                    SVN_DAV_ERROR_TAG);
    }

  /* Before attempting the final commit, we need to push any incoming
     lock-tokens into the filesystem's access_t.   Normally they come
     in via 'If:' header, and dav_svn_get_resource() automatically
     notices them and does this work for us.  In the case of MERGE,
     however, svn clients are sending them in the request body. */

  err = dav_svn__build_lock_hash(&locks, target->info->r,
                                 target->info->repos_path,
                                 pool);
  if (err != NULL)
    return err;

  if (apr_hash_count(locks))
    {
      err = dav_svn__push_locks(source, locks, pool);
      if (err != NULL)
        return err;
    }

  /* We will ignore no_auto_merge and no_checkout. We can't do those, but the
     client has no way to assert that we *should* do them. This should be fine
     because, presumably, the client has no way to do the various checkouts
     and things that would necessitate an auto-merge or checkout during the
     MERGE processing. */

  /* open the transaction that we're going to commit. */
  if ((err = open_txn(&txn, source->info->repos->fs,
                      source->info->root.txn_name, pool)) != NULL)
    return err;

  /* all righty... commit the bugger. */
  serr = svn_repos_fs_commit_txn(&conflict, source->info->repos->repos,
                                 &new_rev, txn, pool);

  /* ### TODO: Figure out if the MERGE response can grow a means by
     which to marshal back both the success of the commit (and its
     commit info) and the failure of the post-commit hook.  */
  if (SVN_IS_VALID_REVNUM(new_rev))
    {
      if (serr)
        {
          /* ### Any error from svn_fs_commit_txn() itself, and not
             ### the post-commit script, should be reported to the
             ### client some other way than hijacking the post-commit
             ### error message.*/
          post_commit_err = svn_repos__post_commit_error_str(serr, pool);
          svn_error_clear(serr);
          serr = SVN_NO_ERROR;
        }

      /* HTTPv2 doesn't send DELETE after a successful MERGE so if
         using the optional vtxn name mapping then delete it here. */
      if (source->info->root.vtxn_name)
        dav_svn__delete_activity(source->info->repos,
                                 source->info->root.vtxn_name);
    }
  else
    {
      svn_error_clear(svn_fs_abort_txn(txn, pool));

      if (serr)
        {
          const char *msg;
          int status;

          if (serr->apr_err == SVN_ERR_FS_CONFLICT)
            {
              status = HTTP_CONFLICT;
              /* ### we need to convert the conflict path into a URI */
              msg = apr_psprintf(pool,
                                 "A conflict occurred during the MERGE "
                                 "processing. The problem occurred with the "
                                 "\"%s\" resource.",
                                 conflict);
            }
          else
            {
              status = HTTP_INTERNAL_SERVER_ERROR;
              msg = "An error occurred while committing the transaction.";
            }

          return dav_svn__convert_err(serr, status, msg, pool);
        }
      else
        {
          return dav_svn__new_error(pool,
                                    HTTP_INTERNAL_SERVER_ERROR,
                                    0,
                                    "Commit failed but there was no error "
                                    "provided.");
        }
    }

  /* Commit was successful, so schedule deltification. */
  register_deltification_cleanup(source->info->repos->repos, new_rev,
                                 source->info->r->connection->pool);

  /* We've detected a 'high level' svn action to log. */
  dav_svn__operational_log(target->info,
                           svn_log__commit(new_rev, target->info->r->pool));

  /* Since the commit was successful, the txn ID is no longer valid.
     If we're using activities, store an empty txn ID in the activity
     database so that when the client deletes the activity, we don't
     try to open and abort the transaction. */
  if (source->type == DAV_RESOURCE_TYPE_ACTIVITY)
    {
      err = dav_svn__store_activity(source->info->repos,
                                    source->info->root.activity_id, "");
      if (err != NULL)
        return err;
    }

  /* Check the dav_resource->info area for information about the
     special X-SVN-Options: header that may have come in the http
     request. */
  if (source->info->svn_client_options != NULL)
    {
      /* The client might want us to release all locks sent in the
         MERGE request. */
      if ((NULL != (ap_strstr_c(source->info->svn_client_options,
                                SVN_DAV_OPTION_RELEASE_LOCKS)))
          && apr_hash_count(locks))
        {
          serr = release_locks(locks, source->info->repos->repos,
                               source->info->r, pool);
          if (serr != NULL)
            return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                        "Error releasing locks", pool);
        }

      /* The client might want us to disable the merge response altogether. */
      if (NULL != (ap_strstr_c(source->info->svn_client_options,
                               SVN_DAV_OPTION_NO_MERGE_RESPONSE)))
        disable_merge_response = TRUE;
    }

  /* process the response for the new revision. */
  return dav_svn__merge_response(output, source->info->repos, new_rev,
                                 post_commit_err, prop_elem,
                                 disable_merge_response, pool);
}


const dav_hooks_vsn dav_svn__hooks_vsn = {
  get_vsn_options,
  get_option,
  versionable,
  auto_versionable,
  vsn_control,
  dav_svn__checkout,
  uncheckout,
  dav_svn__checkin,
  avail_reports,
  report_label_header_allowed,
  deliver_report,
  NULL,                 /* update */
  NULL,                 /* add_label */
  NULL,                 /* remove_label */
  NULL,                 /* can_be_workspace */
  NULL,                 /* make_workspace */
  can_be_activity,
  make_activity,
  merge,
};

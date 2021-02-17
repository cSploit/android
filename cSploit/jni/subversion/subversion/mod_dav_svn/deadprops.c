/*
 * deadprops.c: mod_dav_svn dead property provider functions for Subversion
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

#include <apr_hash.h>

#include <httpd.h>
#include <mod_dav.h>

#include "svn_xml.h"
#include "svn_pools.h"
#include "svn_dav.h"
#include "svn_base64.h"
#include "svn_props.h"
#include "private/svn_log.h"

#include "dav_svn.h"


struct dav_db {
  const dav_resource *resource;
  apr_pool_t *p;

  /* the resource's properties that we are sequencing over */
  apr_hash_t *props;
  apr_hash_index_t *hi;

  /* used for constructing repos-local names for properties */
  svn_stringbuf_t *work;

  /* passed to svn_repos_ funcs that fetch revprops. */
  svn_repos_authz_func_t authz_read_func;
  void *authz_read_baton;
};


struct dav_deadprop_rollback {
  int dummy;
};


/* retrieve the "right" string to use as a repos path */
static const char *
get_repos_path(struct dav_resource_private *info)
{
  return info->repos_path;
}


/* construct the repos-local name for the given DAV property name */
static void
get_repos_propname(dav_db *db,
                   const dav_prop_name *name,
                   const char **repos_propname)
{
  if (strcmp(name->ns, SVN_DAV_PROP_NS_SVN) == 0)
    {
      /* recombine the namespace ("svn:") and the name. */
      svn_stringbuf_set(db->work, SVN_PROP_PREFIX);
      svn_stringbuf_appendcstr(db->work, name->name);
      *repos_propname = db->work->data;
    }
  else if (strcmp(name->ns, SVN_DAV_PROP_NS_CUSTOM) == 0)
    {
      /* the name of a custom prop is just the name -- no ns URI */
      *repos_propname = name->name;
    }
  else
    {
      *repos_propname = NULL;
    }
}


static dav_error *
get_value(dav_db *db, const dav_prop_name *name, svn_string_t **pvalue)
{
  const char *propname;
  svn_error_t *serr;

  /* get the repos-local name */
  get_repos_propname(db, name, &propname);

  if (propname == NULL)
    {
      /* we know these are not present. */
      *pvalue = NULL;
      return NULL;
    }

  /* ### if db->props exists, then try in there first */

  /* We've got three different types of properties (node, txn, and
     revision), and we've got two different protocol versions to deal
     with.  Let's try to make some sense of this, shall we?

        HTTP v1:
          working baseline ('wbl') resource        -> txn prop change
          non-working, baselined resource ('bln')  -> rev prop change [*]
          working, non-baselined resource ('wrk')  -> node prop change

        HTTP v2:
          transaction resource ('txn')             -> txn prop change
          revision resource ('rev')                -> rev prop change
          transaction root resource ('txr')        -> node prop change

     [*] This is a violation of the DeltaV spec (### see issue #916).

  */

  if (db->resource->baselined)
    {
      if (db->resource->type == DAV_RESOURCE_TYPE_WORKING)
        serr = svn_fs_txn_prop(pvalue, db->resource->info->root.txn,
                               propname, db->p);
      else
        serr = svn_repos_fs_revision_prop(pvalue,
                                          db->resource->info-> repos->repos,
                                          db->resource->info->root.rev,
                                          propname, db->authz_read_func,
                                          db->authz_read_baton, db->p);
    }
  else if (db->resource->info->restype == DAV_SVN_RESTYPE_TXN_COLLECTION)
    {
      serr = svn_fs_txn_prop(pvalue, db->resource->info->root.txn,
                             propname, db->p);
    }
  else
    {
      serr = svn_fs_node_prop(pvalue, db->resource->info->root.root,
                              get_repos_path(db->resource->info),
                              propname, db->p);
    }

  if (serr != NULL)
    return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                "could not fetch a property",
                                db->resource->pool);

  return NULL;
}


static dav_error *
save_value(dav_db *db, const dav_prop_name *name,
           const svn_string_t *const *old_value_p,
           const svn_string_t *value)
{
  const char *propname;
  svn_error_t *serr;
  const dav_resource *resource = db->resource;
  apr_pool_t *subpool;

  /* get the repos-local name */
  get_repos_propname(db, name, &propname);

  if (propname == NULL)
    {
      if (db->resource->info->repos->autoversioning)
        /* ignore the unknown namespace of the incoming prop. */
        propname = name->name;
      else
        return dav_svn__new_error(db->p, HTTP_CONFLICT, 0,
                                  "Properties may only be defined in the "
                                  SVN_DAV_PROP_NS_SVN " and "
                                  SVN_DAV_PROP_NS_CUSTOM " namespaces.");
    }

  /* We've got three different types of properties (node, txn, and
     revision), and we've got two different protocol versions to deal
     with.  Let's try to make some sense of this, shall we?

        HTTP v1:
          working baseline ('wbl') resource        -> txn prop change
          non-working, baselined resource ('bln')  -> rev prop change [*]
          working, non-baselined resource ('wrk')  -> node prop change

        HTTP v2:
          transaction resource ('txn')             -> txn prop change
          revision resource ('rev')                -> rev prop change
          transaction root resource ('txr')        -> node prop change

     [*] This is a violation of the DeltaV spec (### see issue #916).

  */

  /* A subpool to cope with mod_dav making multiple calls, e.g. during
     PROPPATCH with multiple values. */
  subpool = svn_pool_create(db->resource->pool);
  if (db->resource->baselined)
    {
      if (db->resource->working)
        {
          serr = svn_repos_fs_change_txn_prop(resource->info->root.txn,
                                              propname, value,
                                              subpool);
        }
      else
        {
          serr = svn_repos_fs_change_rev_prop4(resource->info->repos->repos,
                                               resource->info->root.rev,
                                               resource->info->repos->username,
                                               propname, old_value_p, value,
                                               TRUE, TRUE,
                                               db->authz_read_func,
                                               db->authz_read_baton,
                                               subpool);

          /* Prepare any hook failure message to get sent over the wire */
          if (serr)
            {
              svn_error_t *purged_serr = svn_error_purge_tracing(serr);
              if (purged_serr->apr_err == SVN_ERR_REPOS_HOOK_FAILURE)
                purged_serr->message = apr_xml_quote_string
                                         (purged_serr->pool,
                                          purged_serr->message, 1);

              /* mod_dav doesn't handle the returned error very well, it
                 generates its own generic error that will be returned to
                 the client.  Cache the detailed error here so that it can
                 be returned a second time when the rollback mechanism
                 triggers. */
              resource->info->revprop_error = svn_error_dup(purged_serr);
            }

          /* Tell the logging subsystem about the revprop change. */
          dav_svn__operational_log(resource->info,
                                   svn_log__change_rev_prop(
                                      resource->info->root.rev,
                                      propname, subpool));
        }
    }
  else if (resource->info->restype == DAV_SVN_RESTYPE_TXN_COLLECTION)
    {
      serr = svn_repos_fs_change_txn_prop(resource->info->root.txn,
                                          propname, value, subpool);
    }
  else
    {
      serr = svn_repos_fs_change_node_prop(resource->info->root.root,
                                           get_repos_path(resource->info),
                                           propname, value, subpool);
    }
  svn_pool_destroy(subpool);

  if (serr != NULL)
    return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                NULL, resource->pool);

  /* a change to the props was made; make sure our cached copy is gone */
  db->props = NULL;

  return NULL;
}


static dav_error *
db_open(apr_pool_t *p,
        const dav_resource *resource,
        int ro,
        dav_db **pdb)
{
  dav_db *db;
  dav_svn__authz_read_baton *arb;

  /* Some resource types do not have deadprop databases.
     Specifically: REGULAR, VERSION, WORKING, and our custom
     transaction and transaction root resources have them. (SVN does
     not have WORKSPACE resources, and isn't covered here.) */

  if (resource->type == DAV_RESOURCE_TYPE_HISTORY
      || resource->type == DAV_RESOURCE_TYPE_ACTIVITY
      || (resource->type == DAV_RESOURCE_TYPE_PRIVATE
          && resource->info->restype != DAV_SVN_RESTYPE_TXN_COLLECTION
          && resource->info->restype != DAV_SVN_RESTYPE_TXNROOT_COLLECTION))
    {
      *pdb = NULL;
      return NULL;
    }

  /* If the DB is being opened R/W, and this isn't a working resource, then
     we have a problem! */
  if ((! ro)
      && resource->type != DAV_RESOURCE_TYPE_WORKING
      && resource->type != DAV_RESOURCE_TYPE_PRIVATE
      && resource->info->restype != DAV_SVN_RESTYPE_TXN_COLLECTION)
    {
      /* ### Exception: in violation of deltaV, we *are* allowing a
         baseline resource to receive a proppatch, as a way of
         changing unversioned rev props.  Remove this someday: see IZ #916. */
      if (! (resource->baselined
             && resource->type == DAV_RESOURCE_TYPE_VERSION))
        return dav_svn__new_error(p, HTTP_CONFLICT, 0,
                                  "Properties may only be changed on working "
                                  "resources.");
    }

  db = apr_pcalloc(p, sizeof(*db));

  db->resource = resource;
  db->p = svn_pool_create(p);

  /* ### temp hack */
  db->work = svn_stringbuf_ncreate("", 0, db->p);

  /* make our path-based authz callback available to svn_repos_* funcs. */
  arb = apr_pcalloc(p, sizeof(*arb));
  arb->r = resource->info->r;
  arb->repos = resource->info->repos;
  db->authz_read_baton = arb;
  db->authz_read_func = dav_svn__authz_read_func(arb);

  /* ### use RO and node's mutable status to look for an error? */

  *pdb = db;

  return NULL;
}


static void
db_close(dav_db *db)
{
  svn_pool_destroy(db->p);
}


static dav_error *
db_define_namespaces(dav_db *db, dav_xmlns_info *xi)
{
  dav_xmlns_add(xi, "S", SVN_DAV_PROP_NS_SVN);
  dav_xmlns_add(xi, "C", SVN_DAV_PROP_NS_CUSTOM);
  dav_xmlns_add(xi, "V", SVN_DAV_PROP_NS_DAV);

  /* ### we don't have any other possible namespaces right now. */

  return NULL;
}

static dav_error *
db_output_value(dav_db *db,
                const dav_prop_name *name,
                dav_xmlns_info *xi,
                apr_text_header *phdr,
                int *found)
{
  const char *prefix;
  const char *s;
  svn_string_t *propval;
  dav_error *err;
  apr_pool_t *pool = db->resource->pool;

  if ((err = get_value(db, name, &propval)) != NULL)
    return err;

  /* return whether the prop was found, then punt or handle it. */
  *found = (propval != NULL);
  if (propval == NULL)
    return NULL;

  if (strcmp(name->ns, SVN_DAV_PROP_NS_CUSTOM) == 0)
    prefix = "C:";
  else
    prefix = "S:";

  if (propval->len == 0)
    {
      /* empty value. add an empty elem. */
      s = apr_psprintf(pool, "<%s%s/>" DEBUG_CR, prefix, name->name);
      apr_text_append(pool, phdr, s);
    }
  else
    {
      /* add <prefix:name [V:encoding="base64"]>value</prefix:name> */
      const char *xml_safe;
      const char *encoding = "";

      /* Ensure XML-safety of our property values before sending them
         across the wire. */
      if (! svn_xml_is_xml_safe(propval->data, propval->len))
        {
          const svn_string_t *enc_propval
            = svn_base64_encode_string2(propval, TRUE, pool);
          xml_safe = enc_propval->data;
          encoding = " V:encoding=\"base64\"";
        }
      else
        {
          svn_stringbuf_t *xmlval = NULL;
          svn_xml_escape_cdata_string(&xmlval, propval, pool);
          xml_safe = xmlval->data;
        }

      s = apr_psprintf(pool, "<%s%s%s>", prefix, name->name, encoding);
      apr_text_append(pool, phdr, s);

      /* the value is in our pool which means it has the right lifetime. */
      /* ### at least, per the current mod_dav architecture/API */
      apr_text_append(pool, phdr, xml_safe);

      s = apr_psprintf(pool, "</%s%s>" DEBUG_CR, prefix, name->name);
      apr_text_append(pool, phdr, s);
    }

  return NULL;
}


static dav_error *
db_map_namespaces(dav_db *db,
                  const apr_array_header_t *namespaces,
                  dav_namespace_map **mapping)
{
  /* we don't need a namespace mapping right now. nothing to do */
  return NULL;
}


static dav_error *
decode_property_value(const svn_string_t **out_propval_p,
                      svn_boolean_t *absent,
                      const svn_string_t *maybe_encoded_propval,
                      const apr_xml_elem *elem,
                      apr_pool_t *pool)
{
  apr_xml_attr *attr = elem->attr;

  /* Default: no "encoding" attribute. */
  *absent = FALSE;
  *out_propval_p = maybe_encoded_propval;

  /* Check for special encodings of the property value. */
  while (attr)
    {
      if (strcmp(attr->name, "encoding") == 0) /* ### namespace check? */
        {
          const char *enc_type = attr->value;

          /* Handle known encodings here. */
          if (enc_type && (strcmp(enc_type, "base64") == 0))
            *out_propval_p = svn_base64_decode_string(maybe_encoded_propval,
                                                      pool);
          else
            return dav_svn__new_error(pool, HTTP_INTERNAL_SERVER_ERROR, 0,
                                      "Unknown property encoding");
          break;
        }

      if (strcmp(attr->name, SVN_DAV__OLD_VALUE__ABSENT) == 0)
        {
          /* ### parse attr->value */
          *absent = TRUE;
          *out_propval_p = NULL;
        }

      /* Next attribute, please. */
      attr = attr->next;
    }

  return NULL;
}

static dav_error *
db_store(dav_db *db,
         const dav_prop_name *name,
         const apr_xml_elem *elem,
         dav_namespace_map *mapping)
{
  const svn_string_t *const *old_propval_p;
  const svn_string_t *old_propval;
  const svn_string_t *propval;
  svn_boolean_t absent;
  apr_pool_t *pool = db->p;
  dav_error *derr;

  /* SVN sends property values as a big blob of bytes. Thus, there should be
     no child elements of the property-name element. That also means that
     the entire contents of the blob is located in elem->first_cdata. The
     dav_xml_get_cdata() will figure it all out for us, but (normally) it
     should be awfully fast and not need to copy any data. */

  propval = svn_string_create
    (dav_xml_get_cdata(elem, pool, 0 /* strip_white */), pool);

  derr = decode_property_value(&propval, &absent, propval, elem, pool);
  if (derr)
    return derr;

  if (absent && ! elem->first_child)
    /* ### better error check */
    return dav_svn__new_error(pool, HTTP_INTERNAL_SERVER_ERROR, 0,
                              apr_psprintf(pool,
                                           "'%s' cannot be specified on the "
                                           "value without specifying an "
                                           "expectation",
                                           SVN_DAV__OLD_VALUE__ABSENT));

  /* ### namespace check? */
  if (elem->first_child && !strcmp(elem->first_child->name, SVN_DAV__OLD_VALUE))
    {
      const char *propname;

      get_repos_propname(db, name, &propname);

      /* Parse OLD_PROPVAL. */
      old_propval = svn_string_create(dav_xml_get_cdata(elem->first_child, pool,
                                                        0 /* strip_white */),
                                      pool);
      derr = decode_property_value(&old_propval, &absent,
                                   old_propval, elem->first_child, pool);
      if (derr)
        return derr;

      old_propval_p = (const svn_string_t *const *) &old_propval;
    }
  else
    old_propval_p = NULL;


  return save_value(db, name, old_propval_p, propval);
}


static dav_error *
db_remove(dav_db *db, const dav_prop_name *name)
{
  svn_error_t *serr;
  const char *propname;
  apr_pool_t *subpool;

  /* get the repos-local name */
  get_repos_propname(db, name, &propname);

  /* ### non-svn props aren't in our repos, so punt for now */
  if (propname == NULL)
    return NULL;

  /* A subpool to cope with mod_dav making multiple calls, e.g. during
     PROPPATCH with multiple values. */
  subpool = svn_pool_create(db->resource->pool);

  /* Working Baseline or Working (Version) Resource */
  if (db->resource->baselined)
    if (db->resource->working)
      serr = svn_repos_fs_change_txn_prop(db->resource->info->root.txn,
                                          propname, NULL, subpool);
    else
      /* ### VIOLATING deltaV: you can't proppatch a baseline, it's
         not a working resource!  But this is how we currently
         (hackily) allow the svn client to change unversioned rev
         props.  See issue #916. */
      serr = svn_repos_fs_change_rev_prop4(db->resource->info->repos->repos,
                                           db->resource->info->root.rev,
                                           db->resource->info->repos->username,
                                           propname, NULL, NULL, TRUE, TRUE,
                                           db->authz_read_func,
                                           db->authz_read_baton,
                                           subpool);
  else
    serr = svn_repos_fs_change_node_prop(db->resource->info->root.root,
                                         get_repos_path(db->resource->info),
                                         propname, NULL, subpool);
  svn_pool_destroy(subpool);
  if (serr != NULL)
    return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                "could not remove a property",
                                db->resource->pool);

  /* a change to the props was made; make sure our cached copy is gone */
  db->props = NULL;

  return NULL;
}


static int
db_exists(dav_db *db, const dav_prop_name *name)
{
  const char *propname;
  svn_string_t *propval;
  svn_error_t *serr;
  int retval;

  /* get the repos-local name */
  get_repos_propname(db, name, &propname);

  /* ### non-svn props aren't in our repos */
  if (propname == NULL)
    return 0;

  /* Working Baseline, Baseline, or (Working) Version resource */
  if (db->resource->baselined)
    if (db->resource->type == DAV_RESOURCE_TYPE_WORKING)
      serr = svn_fs_txn_prop(&propval, db->resource->info->root.txn,
                             propname, db->p);
    else
      serr = svn_repos_fs_revision_prop(&propval,
                                        db->resource->info->repos->repos,
                                        db->resource->info->root.rev,
                                        propname,
                                        db->authz_read_func,
                                        db->authz_read_baton, db->p);
  else
    serr = svn_fs_node_prop(&propval, db->resource->info->root.root,
                            get_repos_path(db->resource->info),
                            propname, db->p);

  /* ### try and dispose of the value? */

  retval = (serr == NULL && propval != NULL);
  svn_error_clear(serr);
  return retval;
}

static void get_name(dav_db *db, dav_prop_name *pname)
{
  if (db->hi == NULL)
    {
      pname->ns = pname->name = NULL;
    }
  else
    {
      const void *name;

      apr_hash_this(db->hi, &name, NULL, NULL);

#define PREFIX_LEN (sizeof(SVN_PROP_PREFIX) - 1)
      if (strncmp(name, SVN_PROP_PREFIX, PREFIX_LEN) == 0)
#undef PREFIX_LEN
        {
          pname->ns = SVN_DAV_PROP_NS_SVN;
          pname->name = (const char *)name + 4;
        }
      else
        {
          pname->ns = SVN_DAV_PROP_NS_CUSTOM;
          pname->name = name;
        }
    }
}


static dav_error *
db_first_name(dav_db *db, dav_prop_name *pname)
{
  /* for operational logging */
  const char *action = NULL;

  /* if we don't have a copy of the properties, then get one */
  if (db->props == NULL)
    {
      svn_error_t *serr;

      /* Working Baseline, Baseline, or (Working) Version resource */
      if (db->resource->baselined)
        {
          if (db->resource->type == DAV_RESOURCE_TYPE_WORKING)
            serr = svn_fs_txn_proplist(&db->props,
                                       db->resource->info->root.txn,
                                       db->p);
          else
            {
              action = svn_log__rev_proplist(db->resource->info->root.rev,
                                             db->resource->pool);
              serr = svn_repos_fs_revision_proplist
                (&db->props,
                 db->resource->info->repos->repos,
                 db->resource->info->root.rev,
                 db->authz_read_func,
                 db->authz_read_baton,
                 db->p);
            }
        }
      else
        {
          svn_node_kind_t kind;
          serr = svn_fs_node_proplist(&db->props,
                                      db->resource->info->root.root,
                                      get_repos_path(db->resource->info),
                                      db->p);
          if (! serr)
            serr = svn_fs_check_path(&kind, db->resource->info->root.root,
                                     get_repos_path(db->resource->info),
                                     db->p);

          if (! serr)
            {
              if (kind == svn_node_dir)
                action = svn_log__get_dir(db->resource->info->repos_path,
                                          db->resource->info->root.rev,
                                          FALSE, TRUE, 0, db->resource->pool);
              else
                action = svn_log__get_file(db->resource->info->repos_path,
                                           db->resource->info->root.rev,
                                           FALSE, TRUE, db->resource->pool);
            }
        }
      if (serr != NULL)
        return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                    "could not begin sequencing through "
                                    "properties",
                                    db->resource->pool);
    }

  /* begin the iteration over the hash */
  db->hi = apr_hash_first(db->p, db->props);

  /* fetch the first key */
  get_name(db, pname);

  /* If we have a high-level action to log, do so. */
  if (action != NULL)
    dav_svn__operational_log(db->resource->info, action);

  return NULL;
}


static dav_error *
db_next_name(dav_db *db, dav_prop_name *pname)
{
  /* skip to the next hash entry */
  if (db->hi != NULL)
    db->hi = apr_hash_next(db->hi);

  /* fetch the key */
  get_name(db, pname);

  return NULL;
}


static dav_error *
db_get_rollback(dav_db *db,
                const dav_prop_name *name,
                dav_deadprop_rollback **prollback)
{
  /* This gets called by mod_dav in preparation for a revprop change.
     mod_dav_svn doesn't need to make any changes during rollback, but
     we want the rollback mechanism to trigger.  Making changes in
     response to post-revprop-change hook errors would be positively
     wrong. */

  *prollback = apr_palloc(db->p, sizeof(dav_deadprop_rollback));

  return NULL;
}


static dav_error *
db_apply_rollback(dav_db *db, dav_deadprop_rollback *rollback)
{
  dav_error *derr;

  if (! db->resource->info->revprop_error)
    return NULL;

  /* Returning the original revprop change error here will cause this
     detailed error to get returned to the client in preference to the
     more generic error created by mod_dav. */
  derr = dav_svn__convert_err(db->resource->info->revprop_error,
                              HTTP_INTERNAL_SERVER_ERROR, NULL,
                              db->resource->pool);
  db->resource->info->revprop_error = NULL;

  return derr;
}


const dav_hooks_propdb dav_svn__hooks_propdb = {
  db_open,
  db_close,
  db_define_namespaces,
  db_output_value,
  db_map_namespaces,
  db_store,
  db_remove,
  db_exists,
  db_first_name,
  db_next_name,
  db_get_rollback,
  db_apply_rollback,
};

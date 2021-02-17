/* addpartial-overlay.c */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2014 The OpenLDAP Foundation.
 * Portions Copyright (C) 2004 Virginia Tech, David Hawes.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * http://www.OpenLDAP.org/license.html.
 */
/* ACKNOLEDGEDMENTS:
 * This work was initially developed by David Hawes of Virginia Tech
 * for inclusion in OpenLDAP Software.
 */
/* addpartial-overlay
 *
 * This is an OpenLDAP overlay that intercepts ADD requests, determines if a
 * change has actually taken place for that record, and then performs a modify
 * request for those values that have changed (modified, added, deleted).  If
 * the record has not changed in any way, it is ignored.  If the record does not
 * exist, the record falls through to the normal add mechanism.  This overlay is
 * useful for replicating from sources that are not LDAPs where it is easier to
 * build entire records than to determine the changes (i.e. a database). 
 */

#include "portable.h" 
#include "slap.h"

static int collect_error_msg_cb( Operation *op, SlapReply *rs);

static slap_overinst addpartial;

/**
 *  The meat of the overlay.  Search for the record, determine changes, take
 *  action or fall through.
 */
static int addpartial_add( Operation *op, SlapReply *rs)
{
    Operation nop = *op;
    Entry *toAdd = NULL;
    Entry *found = NULL;
    slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
    int rc;

    toAdd = op->oq_add.rs_e;

    Debug(LDAP_DEBUG_TRACE, "%s: toAdd->e_nname.bv_val: %s\n",
          addpartial.on_bi.bi_type, toAdd->e_nname.bv_val,0);

    /* if the user doesn't have access, fall through to the normal ADD */
    if(!access_allowed(op, toAdd, slap_schema.si_ad_entry,
                       NULL, ACL_WRITE, NULL))
    {
        return SLAP_CB_CONTINUE;
    }

    rc = overlay_entry_get_ov(&nop, &nop.o_req_ndn, NULL, NULL, 0, &found, on);

    if(rc != LDAP_SUCCESS)
    {
        Debug(LDAP_DEBUG_TRACE,
              "%s: no entry found, falling through to normal add\n",
              addpartial.on_bi.bi_type, 0, 0);
        return SLAP_CB_CONTINUE;
    }
    else
    { 
        Debug(LDAP_DEBUG_TRACE, "%s: found the dn\n", addpartial.on_bi.bi_type,
              0,0);

        if(found)
        {
            Attribute *attr = NULL;
            Attribute *at = NULL;
            int ret;
            Modifications *mods = NULL;
            Modifications **modtail = &mods;
            Modifications *mod = NULL;

            Debug(LDAP_DEBUG_TRACE, "%s: have an entry!\n",
                  addpartial.on_bi.bi_type,0,0);

           /* determine if the changes are in the found entry */ 
            for(attr = toAdd->e_attrs; attr; attr = attr->a_next)
            {
                if(attr->a_desc->ad_type->sat_atype.at_usage != 0) continue;

                at = attr_find(found->e_attrs, attr->a_desc);
                if(!at)
                {
                    Debug(LDAP_DEBUG_TRACE, "%s: Attribute %s not found!\n",
                          addpartial.on_bi.bi_type,
                          attr->a_desc->ad_cname.bv_val,0);
                    mod = (Modifications *) ch_malloc(sizeof(
                                                            Modifications));
                    mod->sml_flags = 0;
                    mod->sml_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
                    mod->sml_op &= LDAP_MOD_OP;
                    mod->sml_next = NULL;
                    mod->sml_desc = attr->a_desc;
                    mod->sml_type = attr->a_desc->ad_cname;
                    mod->sml_values = attr->a_vals;
                    mod->sml_nvalues = attr->a_nvals;
                    mod->sml_numvals = attr->a_numvals;
                    *modtail = mod;
                    modtail = &mod->sml_next;
                }
                else
                {
                    MatchingRule *mr = attr->a_desc->ad_type->sat_equality;
                    struct berval *bv;
                    const char *text;
                    int acount , bcount;
                    Debug(LDAP_DEBUG_TRACE, "%s: Attribute %s found\n",
                          addpartial.on_bi.bi_type,
                          attr->a_desc->ad_cname.bv_val,0);

                    for(bv = attr->a_vals, acount = 0; bv->bv_val != NULL; 
                        bv++, acount++)
                    {
                        /* count num values for attr */
                    }
                    for(bv = at->a_vals, bcount = 0; bv->bv_val != NULL; 
                        bv++, bcount++)
                    {
                        /* count num values for attr */
                    }
                    if(acount != bcount)
                    {
                        Debug(LDAP_DEBUG_TRACE, "%s: acount != bcount, %s\n",
                              addpartial.on_bi.bi_type,
                              "replace all",0);
                        mod = (Modifications *) ch_malloc(sizeof(
                                                                Modifications));
                        mod->sml_flags = 0;
                        mod->sml_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
                        mod->sml_op &= LDAP_MOD_OP;
                        mod->sml_next = NULL;
                        mod->sml_desc = attr->a_desc;
                        mod->sml_type = attr->a_desc->ad_cname;
                        mod->sml_values = attr->a_vals;
                        mod->sml_nvalues = attr->a_nvals;
                        mod->sml_numvals = attr->a_numvals;
                        *modtail = mod;
                        modtail = &mod->sml_next;
                        continue;
                    }
                    
                    for(bv = attr->a_vals; bv->bv_val != NULL; bv++)
                    {
                        struct berval *v;
                        ret = -1;
                        
                        for(v = at->a_vals; v->bv_val != NULL; v++)
                        {
                            int r;
                            if(mr && ((r = value_match(&ret, attr->a_desc, mr,
                                           SLAP_MR_VALUE_OF_ASSERTION_SYNTAX,
                                           bv, v, &text)) == 0))
                            {
                                if(ret == 0)
                                    break;
                            }
                            else
                            {
                                Debug(LDAP_DEBUG_TRACE,
                                      "%s: \tvalue DNE, r: %d \n",
                                      addpartial.on_bi.bi_type,
                                      r,0);
                                ret = strcmp(bv->bv_val, v->bv_val);
                                if(ret == 0)
                                    break;
                            }
                        }

                        if(ret == 0)
                        {
                            Debug(LDAP_DEBUG_TRACE,
                                  "%s: \tvalue %s exists, ret: %d\n",
                                  addpartial.on_bi.bi_type, bv->bv_val, ret);
                        }
                        else
                        {
                            Debug(LDAP_DEBUG_TRACE,
                                  "%s: \tvalue %s DNE, ret: %d\n",
                                  addpartial.on_bi.bi_type, bv->bv_val, ret);
                            mod = (Modifications *) ch_malloc(sizeof(
                                                                Modifications));
                            mod->sml_flags = 0;
                            mod->sml_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
                            mod->sml_op &= LDAP_MOD_OP;
                            mod->sml_next = NULL;
                            mod->sml_desc = attr->a_desc;
                            mod->sml_type = attr->a_desc->ad_cname;
                            mod->sml_values = attr->a_vals;
                            mod->sml_nvalues = attr->a_nvals;
                            mod->sml_numvals = attr->a_numvals;
                            *modtail = mod;
                            modtail = &mod->sml_next;
                            break;
                        }
                    }
                }
            }

            /* determine if any attributes were deleted */
            for(attr = found->e_attrs; attr; attr = attr->a_next)
            {
                if(attr->a_desc->ad_type->sat_atype.at_usage != 0) continue;

                at = NULL;
                at = attr_find(toAdd->e_attrs, attr->a_desc);
                if(!at)
                {
                    Debug(LDAP_DEBUG_TRACE,
                          "%s: Attribute %s not found in new entry!!!\n",
                          addpartial.on_bi.bi_type,
                          attr->a_desc->ad_cname.bv_val, 0);
                    mod = (Modifications *) ch_malloc(sizeof(
                                                        Modifications));
                    mod->sml_flags = 0;
                    mod->sml_op = LDAP_MOD_REPLACE;
                    mod->sml_next = NULL;
                    mod->sml_desc = attr->a_desc;
                    mod->sml_type = attr->a_desc->ad_cname;
                    mod->sml_values = NULL;
                    mod->sml_nvalues = NULL;
                    mod->sml_numvals = 0;
                    *modtail = mod;
                    modtail = &mod->sml_next;
                }
                else
                {
                    Debug(LDAP_DEBUG_TRACE,
                          "%s: Attribute %s found in new entry\n",
                          addpartial.on_bi.bi_type,
                          at->a_desc->ad_cname.bv_val, 0);
                }
            }

            overlay_entry_release_ov(&nop, found, 0, on);

            if(mods)
            {
                Modifications *m = NULL;
                Modifications *toDel;
                int modcount;
                slap_callback nullcb = { NULL, collect_error_msg_cb, 
                                         NULL, NULL };

                Debug(LDAP_DEBUG_TRACE, "%s: mods to do...\n",
                      addpartial.on_bi.bi_type, 0, 0);

                nop.o_tag = LDAP_REQ_MODIFY;
                nop.orm_modlist = mods;
                nop.orm_no_opattrs = 0;
                nop.o_callback = &nullcb;
                nop.o_bd->bd_info = (BackendInfo *) on->on_info;

                for(m = mods, modcount = 0; m; m = m->sml_next, 
                    modcount++)
                {
                    /* count number of mods */
                }

                Debug(LDAP_DEBUG_TRACE, "%s: number of mods: %d\n",
                      addpartial.on_bi.bi_type, modcount, 0);

                if(nop.o_bd->be_modify)
                {
                    SlapReply nrs = { REP_RESULT };
                    rc = (nop.o_bd->be_modify)(&nop, &nrs);
                }

                if(rc == LDAP_SUCCESS)
                {
                    Debug(LDAP_DEBUG_TRACE,
                          "%s: modify successful\n",
                          addpartial.on_bi.bi_type, 0, 0);
                }
                else
                {
                    Debug(LDAP_DEBUG_TRACE, "%s: modify unsuccessful: %d\n",
                          addpartial.on_bi.bi_type, rc, 0);
                    rs->sr_err = rc;
                    if(nullcb.sc_private)
                    {
                        rs->sr_text = nullcb.sc_private;
                    }
                }

                Debug(LDAP_DEBUG_TRACE, "%s: freeing mods...\n",
                      addpartial.on_bi.bi_type, 0, 0);

                for(toDel = mods; toDel; toDel = mods)
                {
                    mods = mods->sml_next;
                    ch_free(toDel);
                }
            }
            else
            {
                Debug(LDAP_DEBUG_TRACE, "%s: no mods to process\n",
                      addpartial.on_bi.bi_type, 0, 0);
            }
        }
        else
        {
            Debug(LDAP_DEBUG_TRACE, "%s: no entry!\n",
                  addpartial.on_bi.bi_type, 0, 0);
        }

        op->o_callback = NULL;
        send_ldap_result( op, rs );
        ch_free((void *)rs->sr_text);
        rs->sr_text = NULL;

        return LDAP_SUCCESS;
    }
}

static int collect_error_msg_cb( Operation *op, SlapReply *rs)
{
    if(rs->sr_text)
    {
        op->o_callback->sc_private = (void *) ch_strdup(rs->sr_text);
    }

    return LDAP_SUCCESS;
}

int addpartial_init() 
{
    addpartial.on_bi.bi_type = "addpartial";
    addpartial.on_bi.bi_op_add = addpartial_add;

    return (overlay_register(&addpartial));
}

int init_module(int argc, char *argv[]) 
{
    return addpartial_init();
}

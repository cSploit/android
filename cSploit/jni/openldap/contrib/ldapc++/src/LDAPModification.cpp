// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#include "LDAPModification.h"
#include "debug.h"

using namespace std;

LDAPModification::LDAPModification(const LDAPAttribute& attr, mod_op op){
    DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPModification::LDAPModification()" << endl);
    DEBUG(LDAP_DEBUG_CONSTRUCT | LDAP_DEBUG_PARAMETER,
            "   attr:" << attr << endl);
    m_attr = attr;
    m_mod_op = op;
}

LDAPMod* LDAPModification::toLDAPMod() const  {
    DEBUG(LDAP_DEBUG_TRACE,"LDAPModification::toLDAPMod()" << endl);
    LDAPMod* ret=m_attr.toLDAPMod();

    //The mod_op value of the LDAPMod-struct needs to be ORed with the right
    // LDAP_MOD_* constant to preserve the BIN-flag (see CAPI-draft for 
    // explanation of the LDAPMod struct)
    switch (m_mod_op){
	case OP_ADD :
	    ret->mod_op |= LDAP_MOD_ADD;
	break;
	case OP_DELETE :
	    ret->mod_op |= LDAP_MOD_DELETE;
	break;
	case OP_REPLACE :
	    ret->mod_op |= LDAP_MOD_REPLACE;
	break;
    }
    return ret;
}

const LDAPAttribute* LDAPModification::getAttribute() const {
	return &m_attr;
}

LDAPModification::mod_op LDAPModification::getOperation() const {
	return m_mod_op;
}

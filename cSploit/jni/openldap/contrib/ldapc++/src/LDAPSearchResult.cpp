// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#include <iostream>

#include "debug.h"
#include "LDAPSearchResult.h"
#include "LDAPRequest.h"

using namespace std;

LDAPSearchResult::LDAPSearchResult(const LDAPRequest *req,
        LDAPMessage *msg) : LDAPMsg(msg){
	DEBUG(LDAP_DEBUG_CONSTRUCT,
		"LDAPSearchResult::LDAPSearchResult()" << endl);
    entry = new LDAPEntry(req->getConnection(), msg);
    //retrieve the controls here
    LDAPControl** srvctrls=0;
    int err = ldap_get_entry_controls(req->getConnection()->getSessionHandle(),
            msg,&srvctrls);
    if(err != LDAP_SUCCESS){
        ldap_controls_free(srvctrls);
    }else{
        if (srvctrls){
            m_srvControls = LDAPControlSet(srvctrls);
            m_hasControls = true;
            ldap_controls_free(srvctrls);
        }else{
            m_hasControls = false;
        }
    }
}

LDAPSearchResult::LDAPSearchResult(const LDAPSearchResult& res) :
        LDAPMsg(res){
    entry = new LDAPEntry(*(res.entry));
}

LDAPSearchResult::~LDAPSearchResult(){
	DEBUG(LDAP_DEBUG_DESTROY,"LDAPSearchResult::~LDAPSearchResult()" << endl);
	delete entry;
}

const LDAPEntry* LDAPSearchResult::getEntry() const{
	DEBUG(LDAP_DEBUG_TRACE,"LDAPSearchResult::getEntry()" << endl);
	return entry;
}


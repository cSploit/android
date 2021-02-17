// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#include "LDAPMessage.h"

#include "LDAPResult.h"
#include "LDAPExtResult.h"
#include "LDAPSaslBindResult.h"
#include "LDAPRequest.h"
#include "LDAPSearchResult.h"
#include "LDAPSearchReference.h"
#include "debug.h"
#include <iostream>

using namespace std;

LDAPMsg::LDAPMsg(LDAPMessage *msg){
    DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPMsg::LDAPMsg()" << endl);
    msgType=ldap_msgtype(msg);
    m_hasControls=false;
}

LDAPMsg::LDAPMsg(int type, int id=0){
    DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPMsg::LDAPMsg()" << endl);
    msgType = type;
    msgID = id;
    m_hasControls=false;
}

LDAPMsg* LDAPMsg::create(const LDAPRequest *req, LDAPMessage *msg){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPMsg::create()" << endl);
    switch(ldap_msgtype(msg)){
        case SEARCH_ENTRY :
            return new LDAPSearchResult(req,msg);
        break;
        case SEARCH_REFERENCE :
            return new LDAPSearchReference(req, msg);
        break;
        case EXTENDED_RESPONSE :
            return new LDAPExtResult(req,msg);
        break;
        case BIND_RESPONSE :
            return new LDAPSaslBindResult(req,msg);
        default :
            return new LDAPResult(req, msg);
    }
    return 0;
}


int LDAPMsg::getMessageType(){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPMsg::getMessageType()" << endl);
    return msgType;
}

int LDAPMsg::getMsgID(){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPMsg::getMsgID()" << endl);
    return msgID;
}

bool LDAPMsg::hasControls() const{
    return m_hasControls;
}

const LDAPControlSet& LDAPMsg::getSrvControls() const {
    return m_srvControls;
}


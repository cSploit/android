// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#include "LDAPControl.h"
#include "debug.h"

using namespace std;

LDAPCtrl::LDAPCtrl(const char *oid, bool critical, const char* data,
        int length){
    DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPCtrl::LDAPCtrl()" << endl);
    DEBUG(LDAP_DEBUG_CONSTRUCT | LDAP_DEBUG_PARAMETER,
            "   oid:" << oid << endl << "   critical:" << critical << endl);
    m_oid = oid;
    m_isCritical=critical;
    if (data != 0 && length !=0){
        m_data.assign(data,length);
        m_noData=false;
    }else{
        m_data=string();
        m_noData=true;
    }
}

LDAPCtrl::LDAPCtrl(const string& oid, bool critical, const string& data){
    DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPCtrl::LDAPCtrl()" << endl);
    DEBUG(LDAP_DEBUG_CONSTRUCT | LDAP_DEBUG_PARAMETER,
            "   oid:" << oid << endl << "   critical:" << critical << endl);
    m_oid=oid;
    m_isCritical=critical;
    m_data=data;
    m_noData=false;
}

LDAPCtrl::LDAPCtrl(const LDAPControl* ctrl){
    DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPCtrl::LDAPCtrl()" << endl);
    m_oid = string(ctrl->ldctl_oid);
    m_oid = ctrl->ldctl_iscritical ? true : false;
    m_oid = string(ctrl->ldctl_value.bv_val, ctrl->ldctl_value.bv_len );
}

LDAPCtrl::~LDAPCtrl(){
    DEBUG(LDAP_DEBUG_DESTROY,"LDAPCtrl::~LDAPCtrl" << endl);
}

string LDAPCtrl::getOID() const{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPCtrl::getOID()" << endl);
    return m_oid;
}

bool LDAPCtrl::isCritical()const {
    DEBUG(LDAP_DEBUG_TRACE,"LDAPCtrl::isCritical()" << endl);
    return m_isCritical;
}

bool LDAPCtrl::hasData() const{
    return !m_noData;
}
 
string LDAPCtrl::getData() const {
    DEBUG(LDAP_DEBUG_TRACE,"LDAPCtrl::getData()" << endl);
    return m_data;
}
         
LDAPControl* LDAPCtrl::getControlStruct() const {
    DEBUG(LDAP_DEBUG_TRACE,"LDAPCtrl::getControlStruct()" << endl);
    LDAPControl* ret = new LDAPControl;
    ret->ldctl_oid= new char[m_oid.size() + 1];
    m_oid.copy(ret->ldctl_oid,string::npos);
    ret->ldctl_oid[m_oid.size()]=0;
    if ( m_noData ) {
        ret->ldctl_value.bv_len = 0;
        ret->ldctl_value.bv_val = NULL;
    } else {
        ret->ldctl_value.bv_len=m_data.size();
        ret->ldctl_value.bv_val= new char[m_data.size()];
        m_data.copy(ret->ldctl_value.bv_val,string::npos);
    }
    ret->ldctl_iscritical = ( m_isCritical ? 1:0);
    return ret;
}

void LDAPCtrl::freeLDAPControlStruct(LDAPControl *ctrl){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPCtrl::freeControlStruct()" << endl);
    delete[] ctrl->ldctl_oid;
    delete[] ctrl->ldctl_value.bv_val;
    delete ctrl;
}


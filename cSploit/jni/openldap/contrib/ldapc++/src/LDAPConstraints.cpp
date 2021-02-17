// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#include "debug.h"
#include "config.h"
#include "ac/time.h"
#include "LDAPConstraints.h"
#include "LDAPControlSet.h"

using namespace std;

LDAPConstraints::LDAPConstraints(){
    DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPConstraints::LDAPConstraints()" << endl);
    m_aliasDeref=LDAPConstraints::DEREF_NEVER;
	m_maxTime=LDAP_NO_LIMIT;
	m_maxSize=LDAP_NO_LIMIT;
	m_referralChase=false;
    m_HopLimit=7;
    m_serverControls=0;
    m_clientControls=0;
    m_refRebind=0;
}

LDAPConstraints::LDAPConstraints(const LDAPConstraints& c){
    DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPConstraints::LDAPConstraints(&)" << endl);
    m_aliasDeref=c.m_aliasDeref;
    m_maxTime=c.m_maxTime;
    m_maxSize=c.m_maxSize;
    m_referralChase=c.m_referralChase;
    m_HopLimit=c.m_HopLimit;
    m_deref=c.m_deref;
    if(c.m_serverControls){
        m_serverControls=new LDAPControlSet(*c.m_serverControls);
    }else{
        m_serverControls=0;
    }
    if(c.m_clientControls){
        m_clientControls=new LDAPControlSet(*c.m_clientControls);
    }else{
        m_clientControls=0;
    }
    m_refRebind=c.m_refRebind;
}

LDAPConstraints::~LDAPConstraints(){
    DEBUG(LDAP_DEBUG_DESTROY,"LDAPConstraints::~LDAPConstraints()" << endl);
    delete m_clientControls;
    delete m_serverControls;
}

void LDAPConstraints::setAliasDeref(int deref){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPConstraints::setAliasDeref()" << endl);
    DEBUG(LDAP_DEBUG_TRACE | LDAP_DEBUG_PARAMETER,
            "   deref:" << deref << endl);
    if( (deref == LDAPConstraints::DEREF_NEVER) || 
            (deref == LDAPConstraints::DEREF_SEARCHING) ||
            (deref == LDAPConstraints::DEREF_FINDING) ||
            (deref == LDAPConstraints::DEREF_ALWAYS) 
        ){
        m_aliasDeref=deref;
    }
}
        

void LDAPConstraints::setMaxTime(int t){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPConstraints::setMaxTime()" << endl);
    DEBUG(LDAP_DEBUG_TRACE | LDAP_DEBUG_PARAMETER,"   time:" << t << endl);
    m_maxTime=t;
}

void LDAPConstraints::setSizeLimit(int s){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPConstraints::setSizeLimit()" << endl);
    DEBUG(LDAP_DEBUG_TRACE | LDAP_DEBUG_PARAMETER,"   size:" << s << endl);
	m_maxSize=s;
}

void LDAPConstraints::setReferralChase(bool rc){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPConstraints::setReferralChase()" << endl);
    DEBUG(LDAP_DEBUG_TRACE | LDAP_DEBUG_PARAMETER,"   rc:" << rc << endl);
    m_referralChase=rc;
}

void LDAPConstraints::setHopLimit(int limit){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPConstraints::setHopLimit()" << endl);
    DEBUG(LDAP_DEBUG_TRACE | LDAP_DEBUG_PARAMETER,
            "   limit:" << limit << endl);
    m_HopLimit=limit;
}

void LDAPConstraints::setReferralRebind(const LDAPRebind* rebind){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPConstraints::setReferralRebind()" << endl);
    m_refRebind = rebind;
}

void LDAPConstraints::setServerControls(const LDAPControlSet* ctrls){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPConstraints::setServerControls()" << endl);
    m_serverControls=new LDAPControlSet(*ctrls);
}

void LDAPConstraints::setClientControls(const LDAPControlSet* ctrls){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPConstraints::setClientControls()" << endl);
    m_clientControls=new LDAPControlSet(*ctrls);
}

int LDAPConstraints::getAliasDeref() const {
    DEBUG(LDAP_DEBUG_TRACE,"LDAPConstraints::getAliasDeref()" << endl);
    return m_aliasDeref;
}

int LDAPConstraints::getMaxTime() const {
    DEBUG(LDAP_DEBUG_TRACE,"LDAPConstraints::getMaxTime()" << endl);
	return m_maxTime;
}

int LDAPConstraints::getSizeLimit() const {
    DEBUG(LDAP_DEBUG_TRACE,"LDAPConstraints::getSizeLimit()" << endl);
	return m_maxSize;
}

const LDAPRebind* LDAPConstraints::getReferralRebind() const{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPConstraints::getReferralRebind()" << endl);
    return m_refRebind;
}

const LDAPControlSet* LDAPConstraints::getServerControls() const{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPConstraints::getServerControls()" << endl);
    return m_serverControls;
}

const LDAPControlSet* LDAPConstraints::getClientControls() const{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPConstraints::getClientControls()" << endl);
    return m_clientControls;
}

LDAPControl** LDAPConstraints::getSrvCtrlsArray() const {
    DEBUG(LDAP_DEBUG_TRACE,"LDAPContstraints::getSrvCtrlsArray()" << endl);
    if(m_serverControls){
        return m_serverControls->toLDAPControlArray();
    }else{
        return 0;
    }
}

LDAPControl** LDAPConstraints::getClCtrlsArray() const {
    DEBUG(LDAP_DEBUG_TRACE,"LDAPContstraints::getClCtrlsArray()" << endl);
    if(m_clientControls){
        return m_clientControls->toLDAPControlArray(); 
    }else{
        return 0;
    }
}

timeval* LDAPConstraints::getTimeoutStruct() const {
    DEBUG(LDAP_DEBUG_TRACE,"LDAPContstraints::getTimeoutStruct()" << endl);
    if(m_maxTime == LDAP_NO_LIMIT){
        return 0;
    }else{
        timeval *ret = new timeval;
        ret->tv_sec=m_maxTime;
        ret->tv_usec=0;
        return ret;
    }
}

bool LDAPConstraints::getReferralChase() const {
    DEBUG(LDAP_DEBUG_TRACE,"LDAPContstraints::getReferralChase()" << endl);
	return m_referralChase;
}

int LDAPConstraints::getHopLimit() const{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPContstraints::getHopLimit()" << endl);
    return m_HopLimit;
}


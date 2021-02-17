// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#include "config.h"
#include "debug.h"
#include "LDAPMessageQueue.h"
#include "LDAPRequest.h"
#include "LDAPResult.h"
#include "LDAPSearchReference.h"
#include "LDAPSearchRequest.h"
#include "LDAPUrl.h"
#include "LDAPUrlList.h"
#include "LDAPException.h"

using namespace std;

// TODO: How to handle unsolicited notifications, like notice of
//       disconnection

LDAPMessageQueue::LDAPMessageQueue(LDAPRequest *req){
    DEBUG(LDAP_DEBUG_CONSTRUCT, "LDAPMessageQueue::LDAPMessageQueue()" << endl);
	m_activeReq.push(req);
    m_issuedReq.push_back(req);
}

LDAPMessageQueue::~LDAPMessageQueue(){
    DEBUG(LDAP_DEBUG_DESTROY, "LDAPMessageQueue::~LDAPMessageQueue()" << endl);
    for(LDAPRequestList::iterator i=m_issuedReq.begin(); 
            i != m_issuedReq.end(); i++){
        delete *i;
    }
    m_issuedReq.clear();
}


LDAPMsg *LDAPMessageQueue::getNext(){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPMessageQueue::getNext()" << endl);

    if ( m_activeReq.empty() ) {
        return 0;
    }

    LDAPRequest *req=m_activeReq.top();
    LDAPMsg *ret=0;

    try{
        ret = req->getNextMessage();
    }catch(LDAPException e){
        //do some clean up
        m_activeReq.pop();
        throw;   
    }

    const LDAPConstraints *constr=req->getConstraints();
    switch (ret->getMessageType()) {
        case LDAPMsg::SEARCH_REFERENCE : 
            if (constr->getReferralChase() ){
                //throws Exception (limit Exceeded)
                LDAPRequest *refReq=chaseReferral(ret);
                if(refReq != 0){
                    m_activeReq.push(refReq);
                    m_issuedReq.push_back(refReq);
                    delete ret;
                    return getNext();
                }
            }
            return ret;
        break;
        case LDAPMsg::SEARCH_ENTRY :
            return ret;
        break;
        case LDAPMsg::SEARCH_DONE :
            if(req->isReferral()){
                req->unbind();
            }
            switch ( ((LDAPResult*)ret)->getResultCode()) {
                case LDAPResult::REFERRAL :
                    if(constr->getReferralChase()){
                        //throws Exception (limit Exceeded)
                        LDAPRequest *refReq=chaseReferral(ret);
                        if(refReq != 0){
                            m_activeReq.pop();
                            m_activeReq.push(refReq);
                            m_issuedReq.push_back(refReq);
                            delete ret;
                            return getNext();
                        }
                    }    
                    return ret;
                break;
                case LDAPResult::SUCCESS :
                    if(req->isReferral()){
                        delete ret;
                        m_activeReq.pop();
                        return getNext();
                    }else{
                        m_activeReq.pop();
                        return ret;
                    }
                break;
                default:
                    m_activeReq.pop();
                    return ret;
                break;
            }
        break;
        //must be some kind of LDAPResultMessage
        default:
            if(req->isReferral()){
                req->unbind();
            }
            LDAPResult* res_p=(LDAPResult*)ret;
            switch (res_p->getResultCode()) {
                case LDAPResult::REFERRAL :
                    if(constr->getReferralChase()){
                        //throws Exception (limit Exceeded)
                        LDAPRequest *refReq=chaseReferral(ret);
                        if(refReq != 0){
                            m_activeReq.pop();
                            m_activeReq.push(refReq);
                            m_issuedReq.push_back(refReq);
                            delete ret;
                            return getNext();
                        }
                    }    
                    return ret;
                break;
                default:
                    m_activeReq.pop();
                    return ret;
            }
        break;
    }
}

// TODO Maybe moved to LDAPRequest::followReferral seems more reasonable
//there
LDAPRequest* LDAPMessageQueue::chaseReferral(LDAPMsg* ref){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPMessageQueue::chaseReferral()" << endl);
    LDAPRequest *req=m_activeReq.top();
    LDAPRequest *refReq=req->followReferral(ref);
    if(refReq !=0){
        if(refReq->getConstraints()->getHopLimit() < refReq->getHopCount()){
            delete(refReq);
            throw LDAPException(LDAP_REFERRAL_LIMIT_EXCEEDED);
        }
        if(refReq->isCycle()){
            delete(refReq);
            throw LDAPException(LDAP_CLIENT_LOOP);
        }
        try {
            refReq->sendRequest();
            return refReq;
        }catch (LDAPException e){
            DEBUG(LDAP_DEBUG_TRACE,"   caught exception" << endl);
            return 0;
        }
    }else{ 
        return 0;
    }
}

LDAPRequestStack* LDAPMessageQueue::getRequestStack(){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPMessageQueue::getRequestStack()" << endl);
    return &m_activeReq;
}


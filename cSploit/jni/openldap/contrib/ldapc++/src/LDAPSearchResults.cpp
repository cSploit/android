// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#include "LDAPException.h"
#include "LDAPSearchResult.h"
#include "LDAPResult.h"

#include "LDAPSearchResults.h"

LDAPSearchResults::LDAPSearchResults(){
    entryPos = entryList.begin();
    refPos = refList.begin();
}

LDAPResult* LDAPSearchResults::readMessageQueue(LDAPMessageQueue* msg){
    if(msg != 0){
        LDAPMsg* res=0;
        for(;;){
            try{
                res = msg->getNext();
            }catch (LDAPException e){
                throw;
            }
            switch(res->getMessageType()){ 
                case LDAPMsg::SEARCH_ENTRY :
                    entryList.addEntry(*((LDAPSearchResult*)res)->getEntry());
                break;
                case LDAPMsg::SEARCH_REFERENCE :
                    refList.addReference(*((LDAPSearchReference*)res));
                break;
                default:
                    entryPos=entryList.begin();
                    refPos=refList.begin();
                    return ((LDAPResult*) res);
            }
            delete res;
            res=0;
        }
    }
    return 0;
}

LDAPEntry* LDAPSearchResults::getNext(){
    if( entryPos != entryList.end() ){
        LDAPEntry* ret= new LDAPEntry(*entryPos);
        entryPos++;
        return ret;
    }
    if( refPos != refList.end() ){
        LDAPUrlList urls= refPos->getUrls();
        refPos++;
        throw(LDAPReferralException(urls));
    }
    return 0;
}


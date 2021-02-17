// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef LDAP_EXT_REQUEST_H
#define LDAP_EXT_REQUEST_H

#include <LDAPRequest.h>

class LDAPExtRequest : LDAPRequest {

    public:
        LDAPExtRequest(const LDAPExtRequest& req);
        LDAPExtRequest(const std::string& oid, const std::string& data, 
                LDAPAsynConnection *connect, const LDAPConstraints *cons,
                bool isReferral=false, const LDAPRequest* parent=0);
        virtual ~LDAPExtRequest();
        virtual LDAPMessageQueue* sendRequest();
        virtual LDAPRequest* followReferral(LDAPMsg* urls);
    
    private:
        std::string m_oid;
        std::string m_data;
};

#endif // LDAP_EXT_REQUEST_H

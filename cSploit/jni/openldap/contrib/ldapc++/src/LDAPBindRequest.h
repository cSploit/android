// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef LDAP_BIND_REQUEST_H
#define LDAP_BIND_REQUEST_H

#include <LDAPRequest.h>
#include <LDAPResult.h>
#include <SaslInteractionHandler.h>

class LDAPBindRequest : LDAPRequest {
    private:
        std::string m_dn;
        std::string m_cred;
        std::string m_mech;

    public:
        LDAPBindRequest( const LDAPBindRequest& req);
        //just for simple authentication
        LDAPBindRequest(const std::string&, const std::string& passwd, 
                LDAPAsynConnection *connect, const LDAPConstraints *cons, 
                bool isReferral=false);
        virtual ~LDAPBindRequest();
        virtual LDAPMessageQueue *sendRequest();
};

class LDAPSaslBindRequest : LDAPRequest
{
    public:
        LDAPSaslBindRequest( const std::string& mech, const std::string& cred, 
        LDAPAsynConnection *connect, const LDAPConstraints *cons, 
                bool isReferral=false);
        virtual LDAPMessageQueue *sendRequest();
        virtual ~LDAPSaslBindRequest();

    private:
        std::string m_mech;
        std::string m_cred;
};

class LDAPSaslInteractiveBind : LDAPRequest
{
    public:
        LDAPSaslInteractiveBind( const std::string& mech, int flags,
                SaslInteractionHandler *sih, LDAPAsynConnection *connect, 
                const LDAPConstraints *cons, bool isReferral=false);
        virtual LDAPMessageQueue *sendRequest();
        virtual LDAPMsg* getNextMessage() const;
        virtual ~LDAPSaslInteractiveBind();

    private:
        std::string m_mech;
        int m_flags;
        SaslInteractionHandler *m_sih;
        LDAPResult *m_res;
};
#endif //LDAP_BIND_REQUEST_H


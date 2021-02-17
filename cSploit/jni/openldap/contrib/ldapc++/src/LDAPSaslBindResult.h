// $OpenLDAP$
/*
 * Copyright 2007-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef LDAP_SASL_BIND_RESULT_H
#define LDAP_SASL_BIND_RESULT_H

#include <ldap.h>

#include <LDAPResult.h>

class LDAPRequest;

/**
 * Object of this class are created by the LDAPMsg::create method if
 * results for an Extended Operation were returned by a LDAP server.
 */
class LDAPSaslBindResult : public LDAPResult {
    public :
        /**
         * Constructor that creates an LDAPExtResult-object from the C-API
         * structures
         */
        LDAPSaslBindResult(const LDAPRequest* req, LDAPMessage* msg);

        /**
         * The Destructor
         */
        virtual ~LDAPSaslBindResult();

        /**
         * @returns If the result contained data this method will return
         *          the data to the caller as a std::string.
         */
        const std::string& getServerCreds() const;

    private:
        std::string m_creds;
};

#endif // LDAP_SASL_BIND_RESULT_H

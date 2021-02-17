// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef LDAP_REBIND_AUTH_H
#define LDAP_REBIND_AUTH_H

#include<string>

/**
 * This class represent Authenication information for the case that the
 * library is chasing referrals.
 *
 * The LDAPRebind::getRebindAuth() method returns an object of this type.
 * And the library uses it to authentication to the destination server of a  
 * referral.
 * @note currently only SIMPLE authentication is supported by the library
 */
class LDAPRebindAuth{
    public:
        /**
         * @param dn  The DN that should be used for the authentication 
         * @param pwd   The password that belongs to the DN
         */
        LDAPRebindAuth(const std::string& dn="", const std::string& pwd="");
        
        /**
         * Copy-constructor
         */
        LDAPRebindAuth(const LDAPRebindAuth& lra);

        /**
         * Destructor
         */
        virtual ~LDAPRebindAuth();

        /**
         * @return The DN that was set in the constructor
         */
        const std::string& getDN() const;

        /**
         * @return The password that was set in the constructor
         */
        const std::string& getPassword() const;
        
    private:
        std::string m_dn;
        std::string m_password;
};

#endif //LDAP_REBIND_AUTH_H


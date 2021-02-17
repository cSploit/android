// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#ifndef LDAP_EXCEPTION_H
#define LDAP_EXCEPTION_H

#include <iostream>
#include <string>
#include <stdexcept>

#include <LDAPUrlList.h>

class LDAPAsynConnection;

/**
 * This class is only thrown as an Exception and used to signalize error
 * conditions during LDAP-operations
 */
class LDAPException : public std::runtime_error
{
		
    public :
        /**
         * Constructs a LDAPException-object from the parameters
         * @param res_code A valid LDAP result code.
         * @param err_string    An addional error message for the error
         *                      that happend (optional)
         */
        LDAPException(int res_code, 
                const std::string& err_string=std::string()) throw();
		
        /**
         * Constructs a LDAPException-object from the error state of a
         * LDAPAsynConnection-object
         * @param lc A LDAP-Connection for that an error has happend. The
         *          Constructor tries to read its error state.
         */
        LDAPException(const LDAPAsynConnection *lc) throw();

        /**
         * Destructor
         */
        virtual ~LDAPException() throw();

        /**
         * @return The Result code of the object
         */
        int getResultCode() const throw();

        /**
         * @return The error message that is corresponding to the result
         *          code .
         */
        const std::string& getResultMsg() const throw();
        
        /**
         * @return The addional error message of the error (if it was set)
         */
        const std::string& getServerMsg() const throw();

        
        virtual const char* what() const throw();

        /**
         * This method can be used to dump the data of a LDAPResult-Object.
         * It is only useful for debugging purposes at the moment
         */
        friend std::ostream& operator << (std::ostream &s, LDAPException e) throw();

    private :
        int m_res_code;
        std::string m_res_string;
        std::string m_err_string;
};

/**
 * This class extends LDAPException and is used to signalize Referrals
 * there were received during synchronous LDAP-operations
 */
class LDAPReferralException : public LDAPException
{

    public :
        /**
         * Creates an object that is initialized with a list of URLs
         */
        LDAPReferralException(const LDAPUrlList& urls) throw();

        /**
         * Destructor
         */
        ~LDAPReferralException() throw();

        /**
         * @return The List of URLs of the Referral/Search Reference
         */
        const LDAPUrlList& getUrls() throw();

    private :
        LDAPUrlList m_urlList;
};

#endif //LDAP_EXCEPTION_H

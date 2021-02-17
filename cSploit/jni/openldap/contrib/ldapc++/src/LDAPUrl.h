// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#ifndef LDAP_URL_H
#define LDAP_URL_H

#include <StringList.h>

class LDAPUrlException;
/**
 * This class is used to analyze and store LDAP-Urls as returned by a
 * LDAP-Server as Referrals and Search References. LDAP-URLs are defined
 * in RFC1959 and have the following format: <BR>
 * <code>
 * ldap://host:port/baseDN[?attr[?scope[?filter]]] <BR>
 * </code>
 */
class LDAPUrl{

    public : 
        /**
         * Create a new object from a string that contains a LDAP-Url
         * @param url The URL String
         */
        LDAPUrl(const std::string &url="");

        /**
         * Destructor
         */
        ~LDAPUrl();

        /**
         * @return The part of the URL that is representing the network
         * port
         */
        int getPort() const;
        
        /**
         * Set the port value of the URL
         * @param dn The port value
         */
        void setPort(int port);

        /**
         * @return The scope part of the URL is returned. 
         */
        int getScope() const;

        /**
         * Set the Scope part of the URL
         * @param scope The new scope
         */
        void setScope(const std::string& scope);

        /**
         * @return The complete URL as a string
         */
        const std::string& getURLString() const;

        /**
         * Set the URL member attribute
         * @param url The URL String
         */
        void setURLString(const std::string &url);

        /**
         * @return The hostname or IP-Address of the destination host.
         */
        const std::string& getHost() const;

        /**
         * Set the Host part of the URL
         * @param host The new host part
         */
        void setHost( const std::string &host);

        /**
         * @return The Protocol Scheme of the URL.
         */
        const std::string& getScheme() const;

        /**
         * Set the Protocol Scheme of the URL
         * @param host The Protcol scheme. Allowed values are 
         *       ldap,ldapi,ldaps and cldap
         */
        void setScheme( const std::string &scheme );

        /**
         * @return The Base-DN part of the URL
         */
        const std::string& getDN() const;
        
        /**
         * Set the DN part of the URL
         * @param dn The new DN part
         */
        void setDN( const std::string &dn);

        
        /**
         * @return The Filter part of the URL
         */
        const std::string& getFilter() const;
        
        /**
         * Set the Filter part of the URL
         * @param filter The new Filter
         */
        void setFilter( const std::string &filter);

        /**
         * @return The List of attributes  that was in the URL
         */
        const StringList& getAttrs() const;
        
        /**
         * Set the Attributes part of the URL
         * @param attrs StringList constaining the List of Attributes
         */
        void setAttrs( const StringList &attrs);
        void setExtensions( const StringList &ext);
        const StringList& getExtensions() const;

        /**
         * Percent-decode a string
         * @param src The string that is to be decoded
         * @param dest The decoded result string
         */
        void percentDecode( const std::string& src, std::string& dest );
        
        /**
         * Percent-encoded a string
         * @param src The string that is to be encoded
         * @param dest The encoded result string
         * @param flags
         */
        std::string& percentEncode( const std::string& src, 
                    std::string& dest, 
                    int flags=0 ) const;
   
    protected : 
        /**
         * Split the url string that is associated with this Object into
         * it components. The compontens of the URL can be access via the 
         * get...() methods.
         * (this function is mostly for internal use and gets called 
         * automatically whenever necessary)
         */
        void parseUrl();
        
        /**
         * Generate an URL string from the components that were set with
         * the various set...() methods
         * (this function is mostly for internal use and gets called 
         * automatically whenever necessary)
         */
        void components2Url() const;
        
        void string2list(const std::string &src, StringList& sl,
                bool percentDecode=false);

    protected :
        mutable bool regenerate;
        int m_Port;
        int m_Scope;
        std::string m_Host;
        std::string m_DN;
        std::string m_Filter;
        StringList m_Attrs;
        StringList m_Extensions;
        mutable std::string m_urlString;
        std::string m_Scheme;
        enum mode { base, attrs, scope, filter, extensions };
};

/// @cond
struct code2string_s {
    int code;
    const char* string;
};
/// @endcond

class LDAPUrlException {
    public :
        LDAPUrlException(int code, const std::string &msg="" );

        int getCode() const;
        const std::string getErrorMessage() const;
        const std::string getAdditionalInfo() const;

        static const int INVALID_SCHEME      = 1;
        static const int INVALID_PORT        = 2;
        static const int INVALID_SCOPE       = 3;
        static const int INVALID_URL         = 4;
        static const int URL_DECODING_ERROR  = 5; 
        static const code2string_s code2string[]; 

    private:
        int m_code;
        std::string m_addMsg;
};
#endif //LDAP_URL_H

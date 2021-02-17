// $OpenLDAP$
/*
 * Copyright 2010-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
#ifndef TLS_OPTIONS_H
#define TLS_OPTIONS_H
#include <string>
#include <ldap.h>

/**
 * Class to access the global (and connection specific) TLS Settings
 * To access the global TLS Settings just instantiate a TlsOption object
 * using the default constructor.
 *
 * To access connection specific settings instantiate a TlsOption object
 * through the getTlsOptions() method from the corresponding
 * LDAPConnection/LDAPAsynConnection object.
 *
 */
class TlsOptions {
    public:

        /**
         * Available TLS Options
         */
        enum tls_option {
            CACERTFILE=0, 
            CACERTDIR,
            CERTFILE,
            KEYFILE,
            REQUIRE_CERT,
            PROTOCOL_MIN,
            CIPHER_SUITE,
            RANDOM_FILE,
            CRLCHECK,
            DHFILE,
            /// @cond
            LASTOPT /* dummy */
            /// @endcond
        };

        /**
         * Possible Values for the REQUIRE_CERT option
         */
        enum verifyMode {
            NEVER=0,
            HARD,
            DEMAND,
            ALLOW,
            TRY
        };

        /**
         * Possible Values for the CRLCHECK option
         */
        enum crlMode {
            CRL_NONE=0,
            CRL_PEER,
            CRL_ALL
        };


        /**
         * Default constructor. Gives access to the global TlsSettings
         */
        TlsOptions();

        /**
         * Set string valued options.
         * @param opt The following string valued options are available:
         *      - TlsOptions::CACERTFILE 
         *      - TlsOptions::CACERTDIR
         *      - TlsOptions::CERTFILE
         *      - TlsOptions::KEYFILE
         *      - TlsOptions::CIPHER_SUITE
         *      - TlsOptions::RANDOM_FILE
         *      - TlsOptions::DHFILE
         *  @param value The value to apply to that option, 
         *      - TlsOptions::CACERTFILE:
         *          The path to the file containing all recognized Certificate
         *          Authorities
         *      - TlsOptions::CACERTDIR:
         *          The path to a directory containing individual files of all
         *          recognized Certificate Authority certificates
         *      - TlsOptions::CERTFILE:
         *          The path to the client certificate
         *      - TlsOptions::KEYFILE:
         *          The path to the file containing the private key matching the 
         *          Certificate that as configured with TlsOptions::CERTFILE
         *      - TlsOptions::CIPHER_SUITE
         *          Specifies the cipher suite and preference order
         *      - TlsOptions::RANDOM_FILE
         *          Specifies the file to obtain random bits from when 
         *          /dev/[u]random is not available.
         *      - TlsOptions::DHFILE
         *          File containing DH parameters
         */
        void setOption(tls_option opt, const std::string& value) const;

        /** 
         * Set integer valued options.
         * @param opt The following string valued options are available:
         *      - TlsOptions::REQUIRE_CERT
         *      - TlsOptions::PROTOCOL_MIN
         *      - TlsOptions::CRLCHECK
         * @param value The value to apply to that option, 
         *      - TlsOptions::REQUIRE_CERT:
         *          Possible Values (For details see the ldap.conf(5) man-page):
         *              - TlsOptions::NEVER
         *              - TlsOptions::DEMAND
         *              - TlsOptions::ALLOW
         *              - TlsOptions::TRY
         *      - TlsOptions::PROTOCOL_MIN
         *      - TlsOptions::CRLCHECK
         *          Possible Values:
         *              - TlsOptions::CRL_NONE
         *              - TlsOptions::CRL_PEER
         *              - TlsOptions::CRL_ALL
         */
        void setOption(tls_option opt, int value) const;

        /**
         * Generic setOption variant. Generally you should prefer to use one 
         * of the other variants
         */
        void setOption(tls_option opt, void *value) const;

        /**
         * Read integer valued options
         * @return Option value
         * @throws LDAPException in case of error (invalid on non-integer 
         *      valued option is requested)
         */
        int getIntOption(tls_option opt) const;

        /**
         * Read string valued options
         * @return Option value
         * @throws LDAPException in case of error (invalid on non-string 
         *      valued option is requested)
         */
        std::string getStringOption(tls_option opt) const;

        /**
         * Read options value. Usually you should prefer to use either 
         * getIntOption() or getStringOption()
         * @param value points to a buffer containing the option value
         * @throws LDAPException in case of error (invalid on non-string 
         *      valued option is requested)
         */
        void getOption(tls_option opt, void *value ) const;
        
    private:
        TlsOptions( LDAP* ld );
        void newCtx() const;
        LDAP *m_ld;

    friend class LDAPAsynConnection;
};

#endif /* TLS_OPTIONS_H */

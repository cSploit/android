// $OpenLDAP$
/*
 * Copyright 2010-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include <iostream>
#include <string>
#include "LDAPAsynConnection.h"
#include "TlsOptions.h"

int main( int argc, char* argv[]){
    if ( argc != 4 ){
        std::cerr << "usage: " << argv[0] << " <ldap-uri> <cacertfile> <cacertdir>" << std::endl;
        return(-1);
    }
    std::string uri(argv[1]);
    std::string cacert(argv[2]);
    std::string cadir(argv[3]);
    TlsOptions tls;
    std::cout << "Current global settings:" << std::endl;
    std::cout << "    CaCertfile: " << tls.getStringOption( TlsOptions::CACERTFILE) << std::endl;
    std::cout << "    CaCertDir: " << tls.getStringOption( TlsOptions::CACERTDIR ) << std::endl;
    std::cout << "    Require Cert: " << tls.getIntOption( TlsOptions::REQUIRE_CERT ) << std::endl;
    std::cout << "Applying new settings:" << std::endl;
    tls.setOption( TlsOptions::CACERTFILE, cacert );
    tls.setOption( TlsOptions::REQUIRE_CERT, TlsOptions::DEMAND );
    std::cout << "    CaCertfile: " << tls.getStringOption( TlsOptions::CACERTFILE ) << std::endl;
    std::cout << "    Require Cert: " << tls.getIntOption( TlsOptions::REQUIRE_CERT ) << std::endl;

    try {
        // 1. connect using global options
        LDAPAsynConnection l(uri);
        try {
            l.start_tls();
            std::cout << "StartTLS successful." << std::endl;
            l.unbind();
        } catch ( LDAPException e ) {
            std::cerr << e << std::endl;
        }

        // 2. connect using connection specific option
        LDAPAsynConnection l1(uri);
        tls=l1.getTlsOptions();
        std::cout << "Current connection specific settings:" << std::endl;
        std::cout << "    CaCertfile: " << tls.getStringOption( TlsOptions::CACERTFILE) << std::endl;
        std::cout << "    CaCertDir: " << tls.getStringOption( TlsOptions::CACERTDIR ) << std::endl;
        std::cout << "    Require Cert: " << tls.getIntOption( TlsOptions::REQUIRE_CERT ) << std::endl;
        std::cout << "Applying new settings:" << std::endl;
        tls.setOption( TlsOptions::CACERTDIR, cadir );
        tls.setOption( TlsOptions::REQUIRE_CERT, TlsOptions::DEMAND );
        std::cout << "    CaCertDir: " << tls.getStringOption( TlsOptions::CACERTDIR ) << std::endl;
        std::cout << "    Require Cert: " << tls.getIntOption( TlsOptions::REQUIRE_CERT ) << std::endl;
        try {
            l1.start_tls();
            std::cout << "StartTLS successful." << std::endl;
            l1.unbind();
        } catch ( LDAPException e ) {
            std::cerr << e << std::endl;
        }

        // 3. and once again using the globals
        try {
            LDAPAsynConnection l2(uri);
            TlsOptions tls2;
            std::cout << "Current global settings:" << std::endl;
            std::cout << "    CaCertfile: " << tls2.getStringOption( TlsOptions::CACERTFILE) << std::endl;
            std::cout << "    CaCertDir: " << tls2.getStringOption( TlsOptions::CACERTDIR ) << std::endl;
            std::cout << "    Require Cert: " << tls2.getIntOption( TlsOptions::REQUIRE_CERT ) << std::endl;
            l2.start_tls();
            std::cout << "StartTLS successful." << std::endl;
            l2.unbind();
        } catch ( LDAPException e ) {
            std::cerr << e << std::endl;
        }
    } catch ( LDAPException e ) {
        std::cerr << e << std::endl;
    }
}

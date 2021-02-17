// $OpenLDAP$
/*
 * Copyright 2008-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include <LDAPUrl.h>
#include <LDAPException.h>
#include <cstdlib>
#include <iostream>

int main(int argc, char *argv[]) {
    if ( argc != 2 ) {
        std::cout << argc << std::endl;
        std::cout << "urlTest <ldap-URI>" << std::endl;
        exit(1);
    }
    std::string uristr = argv[1];
    try {
        LDAPUrl url(uristr);
        std::cout << "Host: " << url.getHost() << std::endl;
        std::cout << "Port: " << url.getPort() << std::endl;
        std::cout << "BaseDN: " << url.getDN() << std::endl;
        std::cout << "Scope: " << url.getScope() << std::endl;
        StringList attrs = url.getAttrs();
        std::cout << "Attrs: " << std::endl;
        StringList::const_iterator i = attrs.begin();
        for( ; i != attrs.end(); i++ ) {
            std::cout << "    " << *i << std::endl;
        }
        std::cout << "Filter: " << url.getFilter() << std::endl;
        std::cout << "Setting new BaseDN" << std::endl;
        url.setDN("o=Beispiel, c=DE");
        std::cout << "Url: " << url.getURLString() << std::endl;
    } catch (LDAPUrlException e) {
        std::cout << e.getCode() << std::endl;
        std::cout << e.getErrorMessage() << std::endl;
        std::cout << e.getAdditionalInfo() << std::endl;
    }

}

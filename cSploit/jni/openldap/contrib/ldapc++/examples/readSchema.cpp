// $OpenLDAP$
/*
 * Copyright 2008-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include <iostream>
#include <sstream>
#include "LDAPConnection.h"
#include "LDAPConstraints.h"
#include "LDAPSearchReference.h"
#include "LDAPSearchResults.h"
#include "LDAPAttribute.h"
#include "LDAPAttributeList.h"
#include "LDAPEntry.h"
#include "LDAPException.h"
#include "LDAPModification.h"
#include "LDAPSchema.h"

#include "debug.h"

int main(){
    LDAPConnection *lc=new LDAPConnection("192.168.3.128",389);
    std::cout << "----------------------doing bind...." <<  std::endl;
    try{
        lc->bind("uid=admin,dc=home,dc=local" , "secret");
        std::cout << lc->getHost() << std::endl;
        StringList tmp;
        tmp.add("subschemasubentry");
        LDAPSearchResults* entries = lc->search("", 
                        LDAPConnection::SEARCH_BASE,
                        "(objectClass=*)",
                        tmp );
        LDAPEntry* rootDse = entries->getNext();
        std::string schemabase="cn=subschema";

        if(rootDse){
            const LDAPAttribute* schemaAttr = rootDse->getAttributes()->getAttributeByName("subschemaSubentry");
            schemabase = *(schemaAttr->getValues().begin());   
        }
        StringList attrs;
        attrs.add("objectClasses");
        attrs.add("attributeTypes");
        entries = lc->search(schemabase, LDAPConnection::SEARCH_BASE, "(objectClass=*)",
                        attrs);
        if (entries != 0){
            LDAPEntry* entry = entries->getNext();
            if(entry != 0){
                const LDAPAttribute* oc = entry->getAttributes()->getAttributeByName("objectClasses");
                LDAPSchema schema;
                schema.setObjectClasses((oc->getValues()));
                LDAPObjClass test = schema.getObjectClassByName("inetOrgPerson");
                std::cout << test.getDesc() << std::endl;
//                StringList mustAttr = test.getMay();
//                for( StringList::const_iterator i = mustAttr.begin(); i != mustAttr.end(); i++ ){
//                    std::cout << *i << std::endl;
//                }
                StringList sup = test.getSup();
                for( StringList::const_iterator i = sup.begin(); i != sup.end(); i++ ){
                    std::cout << *i << std::endl;
                }
            }
        }
        
        lc->unbind();
        delete lc;
   }catch (LDAPException e){
        std::cout << "---------------- caught Exception ---------"<< std::endl;
        std::cout << e << std::endl;
    }

}


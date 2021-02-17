// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#ifndef LDAP_CONTROL_H
#define LDAP_CONTROL_H
#include <string>
#include <ldap.h>

/**
 * This class is used to store Controls. Controls are a mechanism to extend
 * and modify LDAP-Operations.
 */
class LDAPCtrl{
    public :
        /**
         * Constructor.
         * @param oid:  The Object Identifier of the Control
         * @param critical: "true" if the Control should be handled
         *                  critical by the server.
         * @param data: If there is data for the control, put it here.
         * @param length: The length of the data field
         */
        LDAPCtrl(const char *oid, bool critical=false, const char *data=0, 
                int length=0);

        /**
         * Constructor.
         * @param oid:  The Object Identifier of the Control
         * @param critical: "true" if the Control should be handled
         *                  critical by the server.
         * @param data: If there is data for the control, put it here.
         */
        LDAPCtrl(const std::string& oid, bool critical,
                 const std::string& data);

        /**
         * Creates a copy of the Control that "ctrl is pointing to
         */
        LDAPCtrl(const LDAPControl* ctrl);

        /**
         * Destructor
         */
        ~LDAPCtrl();
       
        /**
         * @return The OID of the control
         */
        std::string getOID() const;

        /**
         * @return true if there is no "Control Value" (there is a
         * difference between no and an empty control value)
         */
        bool hasData() const;

        /**
         * @return The Data of the control as a std::string-Object
         */
        std::string getData() const;

        /**
         * @return "true" if the control is critical
         */
        bool isCritical() const;

        /**
         * For internal use only.
         *
         * Translates the control to a LDAPControl-structure as needed by
         * the C-API
         */
        LDAPControl* getControlStruct() const;
	static void freeLDAPControlStruct(LDAPControl *ctrl);

    private :
        std::string m_oid;
        std::string m_data;
        bool m_isCritical;
        bool m_noData;
};

#endif //LDAP_CONTROL_H

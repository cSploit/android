// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include <ldap.h>
#include "config.h"
#include "LDAPException.h"

#include "LDAPAsynConnection.h"
#include "LDAPResult.h"

using namespace std;

LDAPException::LDAPException(int res_code, const string& err_string) throw()
    : std::runtime_error(err_string)
{
	m_res_code=res_code;
	m_res_string=string(ldap_err2string(res_code));
    m_err_string=err_string;
}

LDAPException::LDAPException(const LDAPAsynConnection *lc) throw()
    : std::runtime_error("")
{
    LDAP *l = lc->getSessionHandle();
    ldap_get_option(l,LDAP_OPT_RESULT_CODE,&m_res_code);
    const char *res_cstring = ldap_err2string(m_res_code);
    if ( res_cstring ) {
        m_res_string = string(res_cstring);
    } else {
        m_res_string = "";
    }
    const char* err_string;

#ifdef LDAP_OPT_DIAGNOSTIC_MESSAGE
    ldap_get_option(l,LDAP_OPT_DIAGNOSTIC_MESSAGE ,&err_string);
#else
    ldap_get_option(l,LDAP_OPT_ERROR_STRING,&err_string);
#endif
    if ( err_string ) {
        m_err_string = string(err_string);
    } else {
        m_err_string = "";
    }
}

LDAPException::~LDAPException() throw()
{
}

int LDAPException::getResultCode() const throw()
{
	return m_res_code;
}

const string& LDAPException::getResultMsg() const throw()
{
	return m_res_string;
}

const string& LDAPException::getServerMsg() const throw()
{
    return m_err_string;
}

const char* LDAPException::what() const throw()
{
    return this->m_res_string.c_str(); 
}

ostream& operator << (ostream& s, LDAPException e) throw()
{
	s << "Error " << e.m_res_code << ": " << e.m_res_string;
	if (!e.m_err_string.empty()) {
		s << endl <<  "additional info: " << e.m_err_string ;
	}
	return s;
}


LDAPReferralException::LDAPReferralException(const LDAPUrlList& urls) throw() 
        : LDAPException(LDAPResult::REFERRAL) , m_urlList(urls)
{
}

LDAPReferralException::~LDAPReferralException() throw()
{
}

const LDAPUrlList& LDAPReferralException::getUrls() throw()
{
    return m_urlList;
}


// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#include "LDAPUrl.h"
#include <sstream>
#include <iomanip>
#include "debug.h"

using namespace std;

#define PCT_ENCFLAG_NONE 0x0000U
#define PCT_ENCFLAG_COMMA 0x0001U
#define PCT_ENCFLAG_SLASH 0x0002U

#define LDAP_DEFAULT_PORT 389
#define LDAPS_DEFAULT_PORT 636

LDAPUrl::LDAPUrl(const std::string &url)
{
    DEBUG(LDAP_DEBUG_CONSTRUCT, "LDAPUrl::LDAPUrl()" << endl);
    DEBUG(LDAP_DEBUG_CONSTRUCT | LDAP_DEBUG_PARAMETER,
            "   url:" << url << endl);
    m_urlString = url;
    m_Filter = "";
    m_Scheme = "ldap";
    m_Scope = 0;
    m_Port = 0;
    regenerate = false;
    if (url != "") {
        this->parseUrl();
    }
}

LDAPUrl::~LDAPUrl()
{
    DEBUG(LDAP_DEBUG_DESTROY, "LDAPUrl::~LDAPUrl()" << endl);
    m_Attrs.clear();
}

int LDAPUrl::getPort() const 
{
    return m_Port;
}

void LDAPUrl::setPort(int port)
{
    m_Port = port;
    regenerate = true;
}

int LDAPUrl::getScope() const 
{
    return m_Scope;
}

void LDAPUrl::setScope( const std::string &scope )
{
    if (scope == "base" || scope == "" ) {
        m_Scope = 0;
    } else if (scope == "one" ) {
        m_Scope = 1;
    } else if (scope == "sub" ) {
        m_Scope = 2;
    } else {
        throw LDAPUrlException(LDAPUrlException::INVALID_SCOPE, 
                "Scope was:" + scope); 
    }
    regenerate = true;
}

const string& LDAPUrl::getURLString() const
{
    if (regenerate){
        this->components2Url();
        regenerate=false;
    }
    return m_urlString;
}

void LDAPUrl::setURLString( const std::string &url )
{
    m_urlString = url;
    if (url != "") {
        this->parseUrl();
    }
    regenerate = false;
}

const string& LDAPUrl::getHost() const 
{
    return m_Host;
}

void LDAPUrl::setHost( const std::string &host )
{
    m_Host = host;
    regenerate = true;
}

const string& LDAPUrl::getDN() const 
{
    return m_DN;
}
void LDAPUrl::setDN( const std::string &dn )
{
    m_DN = dn;
    regenerate = true;
}

const string& LDAPUrl::getFilter() const 
{
    return m_Filter;
}
void LDAPUrl::setFilter( const std::string &filter )
{
    m_Filter = filter;
    regenerate = true;
}

const StringList& LDAPUrl::getAttrs() const 
{
    return m_Attrs;
}
void LDAPUrl::setAttrs( const StringList &attrs )
{
    m_Attrs = attrs;
    regenerate = true;
}

const StringList& LDAPUrl::getExtensions() const 
{
    return m_Extensions;
}

void LDAPUrl::setExtensions( const StringList &ext )
{
    m_Extensions = ext;
    regenerate = true;
}

const std::string& LDAPUrl::getScheme() const
{
    return m_Scheme;
}

void LDAPUrl::setScheme( const std::string &scheme )
{
    if (scheme == "ldap" || scheme == "ldaps" || 
            scheme == "ldapi" || scheme == "cldap" ) 
    {
        m_Scheme = scheme;
        regenerate = true;
    } else {
        throw LDAPUrlException(LDAPUrlException::INVALID_SCHEME,
                "Unknown URL scheme: \"" + scheme + "\"");
    }
}

void LDAPUrl::parseUrl() 
{
    DEBUG(LDAP_DEBUG_TRACE, "LDAPUrl::parseUrl()" << std::endl);
    // reading Scheme
    std::string::size_type pos = m_urlString.find(':');
    std::string::size_type startpos = pos;
    if (pos == std::string::npos) {
        throw LDAPUrlException(LDAPUrlException::INVALID_URL,
                "No colon found in URL");
    }
    std::string scheme = m_urlString.substr(0, pos);
    DEBUG(LDAP_DEBUG_TRACE, "    scheme is <" << scheme << ">" << std::endl);

    if ( scheme == "ldap" ) {
        m_Scheme = scheme;
    } else if ( scheme == "ldaps" ) {
        m_Scheme = scheme;
    } else if ( scheme == "ldapi" ) {
        m_Scheme = scheme;
    } else if ( scheme == "cldap" ) {
        m_Scheme = scheme;
    } else {
        throw LDAPUrlException(LDAPUrlException::INVALID_SCHEME,
                "Unknown URL Scheme: \"" + scheme + "\"");
    }

    if ( m_urlString[pos+1] != '/' || m_urlString[pos+2] != '/' ) {
        throw LDAPUrlException(LDAPUrlException::INVALID_URL);
    } else {
        startpos = pos + 3;
    }
    if ( m_urlString[startpos] == '/' ) {
        // no hostname and port
        startpos++;
    } else {
        std::string::size_type hostend, portstart=0;
        pos = m_urlString.find('/', startpos);

        // IPv6 Address?
        if ( m_urlString[startpos] == '[' ) {
            // skip
            startpos++;
            hostend =  m_urlString.find(']', startpos);
            if ( hostend == std::string::npos ){
                throw LDAPUrlException(LDAPUrlException::INVALID_URL);
            }
            portstart = hostend + 1;
        } else {
            hostend = m_urlString.find(':', startpos);
            if ( hostend == std::string::npos || portstart > pos ) {
                hostend = pos;
            }
            portstart = hostend;
        }
        std::string host = m_urlString.substr(startpos, hostend - startpos);
        DEBUG(LDAP_DEBUG_TRACE, "    host: <" << host << ">" << std::endl);
        percentDecode(host, m_Host);

        if (portstart >= m_urlString.length() || portstart >= pos ) {
            if ( m_Scheme == "ldap" || m_Scheme == "cldap" ) {
                m_Port = LDAP_DEFAULT_PORT;
            } else if ( m_Scheme == "ldaps" ) {
                m_Port = LDAPS_DEFAULT_PORT;
            }
        } else {
            std::string port = m_urlString.substr(portstart+1, 
                    (pos == std::string::npos ? pos : pos-portstart-1) );
            if ( port.length() > 0 ) {
                std::istringstream i(port);
                i >> m_Port;
                if ( i.fail() ){
                    throw LDAPUrlException(LDAPUrlException::INVALID_PORT);
                }
            }
            DEBUG(LDAP_DEBUG_TRACE, "    Port: <" << m_Port << ">" 
                    << std::endl);
        }
        startpos = pos + 1;
    }
    int parserMode = base;
    while ( pos != std::string::npos ) {
        pos = m_urlString.find('?', startpos);
        std::string actComponent = m_urlString.substr(startpos, 
                pos - startpos);
        DEBUG(LDAP_DEBUG_TRACE, "    ParserMode:" << parserMode << std::endl);
        DEBUG(LDAP_DEBUG_TRACE, "    ActComponent: <" << actComponent << ">" 
                << std::endl);
        std::string s_scope = "";
        std::string s_ext = "";
        switch(parserMode) {
            case base :
                percentDecode(actComponent, m_DN);
                DEBUG(LDAP_DEBUG_TRACE, "    BaseDN:" << m_DN << std::endl); 
                break;
            case attrs :
                DEBUG(LDAP_DEBUG_TRACE, "    reading Attributes" << std::endl);
                if (actComponent.length() != 0 ) {
                    string2list(actComponent,m_Attrs, true);
                }
                break;
            case scope :
                percentDecode(actComponent, s_scope);
                if (s_scope == "base" || s_scope == "" ) {
                    m_Scope = 0;
                } else if (s_scope == "one" ) {
                    m_Scope = 1;
                } else if (s_scope == "sub" ) {
                    m_Scope = 2;
                } else {
                    throw LDAPUrlException(LDAPUrlException::INVALID_SCOPE);
                }
                DEBUG(LDAP_DEBUG_TRACE, "    Scope: <" << s_scope << ">"
                        << std::endl);
                break;
            case filter :
                percentDecode(actComponent, m_Filter);
                DEBUG(LDAP_DEBUG_TRACE, "    filter: <" << m_Filter << ">"
                        << std::endl);
                break;
            case extensions :
                DEBUG(LDAP_DEBUG_TRACE, "    reading Extensions" << std::endl); 
                string2list(actComponent, m_Extensions, true);
                break;
            default : 
                DEBUG(LDAP_DEBUG_TRACE, "    unknown state" << std::endl); 
                break;
        }
        startpos = pos + 1;
        parserMode++;
    }
}

void LDAPUrl::percentDecode(const std::string& src, std::string &out)
{
    DEBUG(LDAP_DEBUG_TRACE, "LDAPUrl::percentDecode()" << std::endl); 
    std::string::size_type pos = 0;
    std::string::size_type startpos = 0;
    pos = src.find('%', startpos);
    while ( pos != std::string::npos ) {
        out += src.substr(startpos, pos - startpos);
        std::string istr(src.substr(pos+1, 2));
        std::istringstream i(istr);
        i.setf(std::ios::hex, std::ios::basefield);
        i.unsetf(std::ios::showbase);
        int hex;
        i >> hex;
        if ( i.fail() ){
            throw LDAPUrlException(LDAPUrlException::URL_DECODING_ERROR, 
                    "Invalid percent encoding");
        }
        char j = hex;
        out.push_back(j);
        startpos = pos+3;
        pos = src.find('%', startpos);
    } 
    out += src.substr(startpos, pos - startpos);
}

void LDAPUrl::string2list(const std::string &src, StringList& sl, 
            bool percentDecode)
{
    std::string::size_type comma_startpos = 0;
    std::string::size_type comma_pos = 0;
    std::string actItem;
    while ( comma_pos != std::string::npos ) {
        comma_pos = src.find(',', comma_startpos);
        actItem = src.substr(comma_startpos, comma_pos - comma_startpos);
        if (percentDecode){
            std::string decoded;
            this->percentDecode(actItem,decoded);
            actItem = decoded;
        }
        sl.add(actItem);
        comma_startpos = comma_pos + 1;
    }
}


void LDAPUrl::components2Url() const
{
    std::ostringstream url; 
    std::string encoded = "";
    
    url << m_Scheme << "://";
    // IPv6 ?
    if ( m_Host.find( ':', 0 ) != std::string::npos ) {
        url <<  "[" << this->percentEncode(m_Host, encoded) <<  "]";
    } else {
        url << this->percentEncode(m_Host, encoded, PCT_ENCFLAG_SLASH);
    }

    if ( m_Port != 0 ) {
        url << ":" << m_Port;
    }

    url << "/";
    encoded = "";
    if ( m_DN != "" ) {
        this->percentEncode( m_DN, encoded );
        url << encoded;
    }
    string qm = "";
    if ( ! m_Attrs.empty() ){
        url << "?";
        bool first = true;
        for ( StringList::const_iterator i = m_Attrs.begin();
                i != m_Attrs.end(); i++) 
        {
            this->percentEncode( *i, encoded );
            if ( ! first ) {
                url << ",";
            } else {
                first = false;
            }
            url << encoded;
        }
    } else {
        qm.append("?");
    }
    if ( m_Scope == 1 ) {
        url << qm << "?one";
        qm = "";
    } else if ( m_Scope == 2 ) {
        url << qm << "?sub";
        qm = "";
    } else {
        qm.append("?");
    }
    if (m_Filter != "" ){
        this->percentEncode( m_Filter, encoded );
        url << qm << "?" << encoded;
        qm = "";
    } else {
        qm.append("?");
    }

    if ( ! m_Extensions.empty() ){
        url << qm << "?";
        bool first = true;
        for ( StringList::const_iterator i = m_Extensions.begin();
                i != m_Extensions.end(); i++) 
        {
            this->percentEncode( *i, encoded, 1);
            if ( ! first ) {
                url << ",";
            } else {
                first = false;
            }
            url << encoded;
        }
    }
    m_urlString=url.str();  
}


std::string& LDAPUrl::percentEncode( const std::string &src, 
        std::string &dest, 
        int flags) const
{
    std::ostringstream o;
    o.setf(std::ios::hex, std::ios::basefield);
    o.setf(std::ios::uppercase);
    o.unsetf(std::ios::showbase);
    bool escape=false;
    for ( std::string::const_iterator i = src.begin(); i != src.end(); i++ ){
        switch(*i){
            /* reserved */
            case '?' :
                escape = true;
            break;
            case ',' :
                if ( flags & PCT_ENCFLAG_COMMA ) {
                    escape = true;
                } else {
                    escape = false;
                }
            break;
            case ':' :
            case '/' :
                if ( flags & PCT_ENCFLAG_SLASH ) {
                    escape = true;
                } else {
                    escape = false;
                }
            break;
            case '#' :
            case '[' :
            case ']' :
            case '@' :
            case '!' :
            case '$' :
            case '&' :
            case '\'' :
            case '(' :
            case ')' :
            case '*' :
            case '+' :
            case ';' :
            case '=' :
            /* unreserved */
            case '-' :
            case '.' :
            case '_' :
            case '~' :
                escape = false;
            break;
            default :
                if (  std::isalnum(*i) ) {
                    escape = false;
                } else {
                    escape = true;
                }
            break;
        }
        if ( escape ) {
            o << "%" << std::setw(2) << std::setfill('0') << (int)(unsigned char)*i ;
        } else {
            o.put(*i);
        }
    }
    dest = o.str();
    return dest;
}

const code2string_s LDAPUrlException::code2string[] = {
   { INVALID_SCHEME,            "Invalid URL Scheme" },
   { INVALID_PORT,              "Invalid Port in Url" },
   { INVALID_SCOPE,             "Invalid Search Scope in Url" },
   { INVALID_URL,               "Invalid LDAP Url" },
   { URL_DECODING_ERROR,        "Url-decoding Error" },
   { 0, 0 }
};

LDAPUrlException::LDAPUrlException( int code, const std::string &msg) :
    m_code(code), m_addMsg(msg) {}

int LDAPUrlException::getCode() const
{
    return m_code;
}

const std::string LDAPUrlException::getAdditionalInfo() const
{
    return m_addMsg;
}

const std::string LDAPUrlException::getErrorMessage() const
{
    for ( int i = 0; code2string[i].string != 0; i++ ) {
        if ( code2string[i].code == m_code ) {
            return std::string(code2string[i].string);
        }
    }
    return "";

}

// $OpenLDAP$
/*
 * Copyright 2008-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "LdifReader.h"
#include "LDAPMessage.h"
#include "LDAPEntry.h"
#include "LDAPAttributeList.h"
#include "LDAPAttribute.h"
#include "LDAPUrl.h"
#include "debug.h"

#include <string>
#include <sstream>
#include <stdexcept>

#include <sasl/saslutil.h> // For base64 routines

typedef std::pair<std::string, std::string> stringpair;

LdifReader::LdifReader( std::istream &input ) 
        : m_ldifstream(input), m_lineNumber(0)
{
    DEBUG(LDAP_DEBUG_TRACE, "<> LdifReader::LdifReader()" << std::endl);
    this->m_version = 0;
    // read the first record to find out version and type of the LDIF
    this->readNextRecord(true);
    this->m_currentIsFirst = true;
}

int LdifReader::readNextRecord( bool first )
{
    DEBUG(LDAP_DEBUG_TRACE, "-> LdifReader::readRecord()" << std::endl);
    std::string line;
    std::string type;
    std::string value;
    int numLine = 0;
    int recordType = 0;

    if ( (! first) && this->m_currentIsFirst == true )
    {
        this->m_currentIsFirst = false;
        return m_curRecType;
    }

    m_currentRecord.clear();

    while ( !this->getLdifLine(line) )
    {
        DEBUG(LDAP_DEBUG_TRACE, "  Line: " << line << std::endl );

        // skip comments and empty lines between entries
        if ( line[0] == '#' || ( numLine == 0 && line.size() == 0 ) )
        {
            DEBUG(LDAP_DEBUG_TRACE, "skipping empty line or comment" << std::endl );
            continue;
        }
        if ( line.size() == 0 ) 
        {
            // End of Entry
            break;
        }

        this->splitLine(line, type, value);

        if ( numLine == 0 )
        {
            if ( type == "version" )
            {
                std::istringstream valuestream(value);
                valuestream >> this->m_version;
                if ( this->m_version != 1 ) // there is no other Version than LDIFv1 
                {
                    std::ostringstream err;
                    err << "Line " << this->m_lineNumber 
                        << ": Unsuported LDIF Version";
                    throw( std::runtime_error(err.str()) );
                }
                continue;
            }
            if ( type == "dn" ) // Record should start with the DN ...
            {
                DEBUG(LDAP_DEBUG_TRACE, " Record DN:" << value << std::endl);
            }
            else if ( type == "include" ) // ... or it might be an "include" line
            {
                DEBUG(LDAP_DEBUG_TRACE, " Include directive: " << value << std::endl);
                if ( this->m_version == 1 )
                {
                    std::ostringstream err;
                    err << "Line " << this->m_lineNumber 
                        << ": \"include\" not allowed in LDIF version 1.";
                    throw( std::runtime_error(err.str()) );
                }
                else
                {
                    std::ostringstream err;
                    err << "Line " << this->m_lineNumber 
                        << ": \"include\" not yet suppported.";
                    throw( std::runtime_error(err.str()) );
                }
            }
            else
            {
                DEBUG(LDAP_DEBUG_TRACE, " Record doesn't start with a DN" 
                            << std::endl);
                std::ostringstream err;
                err << "Line " << this->m_lineNumber 
                    << ": LDIF record does not start with a DN.";
                throw( std::runtime_error(err.str()) );
            }
        }
        if ( numLine == 1 ) // might contain "changtype" to indicate a change request
        {
            if ( type == "changetype" ) 
            {
                if ( first ) 
                {
                    this->m_ldifTypeRequest = true;
                }
                else if (! this->m_ldifTypeRequest )
                {
                    // Change Request in Entry record LDIF, should we accept it?
                    std::ostringstream err;
                    err << "Line " << this->m_lineNumber 
                        << ": Change Request in an entry-only LDIF.";
                    throw( std::runtime_error(err.str()) );
                }
                if ( value == "modify" )
                {
                    recordType = LDAPMsg::MODIFY_REQUEST;
                }
                else if ( value == "add" )
                {
                    recordType = LDAPMsg::ADD_REQUEST;
                }
                else if ( value == "delete" )
                {
                    recordType = LDAPMsg::DELETE_REQUEST;
                }
                else if ( value == "modrdn" )
                {   
                    recordType = LDAPMsg::MODRDN_REQUEST;
                }
                else
                {
                    DEBUG(LDAP_DEBUG_TRACE, " Unknown change request <" 
                            << value << ">" << std::endl);
                    std::ostringstream err;
                    err << "Line " << this->m_lineNumber 
                        << ": Unknown changetype: \"" << value << "\".";
                    throw( std::runtime_error(err.str()) );
                }
            }
            else
            {
                if ( first ) 
                {
                    this->m_ldifTypeRequest = false;
                }
                else if (this->m_ldifTypeRequest )
                {
                    // Entry record in Change record LDIF, should we accept 
                    // it (e.g. as AddRequest)?
                }
                recordType = LDAPMsg::SEARCH_ENTRY;
            }
        }
        m_currentRecord.push_back( stringpair(type, value) );
        numLine++;
    }
    DEBUG(LDAP_DEBUG_TRACE, "<- LdifReader::readRecord() return: " 
            << recordType << std::endl);
    m_curRecType = recordType;
    return recordType;
}

LDAPEntry LdifReader::getEntryRecord()
{
    std::list<stringpair>::const_iterator i = m_currentRecord.begin();
    if ( m_curRecType != LDAPMsg::SEARCH_ENTRY )
    {
        throw( std::runtime_error( "The LDIF record: '" + i->second +
                                   "' is not a valid LDAP Entry" ));
    }
    LDAPEntry resEntry(i->second);
    i++;
    LDAPAttribute curAttr(i->first);
    LDAPAttributeList *curAl = new LDAPAttributeList();
    for ( ; i != m_currentRecord.end(); i++ )
    {
        if ( i->first == curAttr.getName() )
        {
            curAttr.addValue(i->second);
        }
        else
        {
            const LDAPAttribute* existing = curAl->getAttributeByName( i->first );
            if ( existing )
            {
                // Attribute exists already (handle gracefully)
                curAl->addAttribute( curAttr );
                curAttr = LDAPAttribute( *existing );
                curAttr.addValue(i->second);
                curAl->delAttribute( i->first );
            }
            else
            {
                curAl->addAttribute( curAttr );
                curAttr = LDAPAttribute( i->first, i->second );
            }
        }
    }
    curAl->addAttribute( curAttr );
    resEntry.setAttributes( curAl );
    return resEntry;
}

int LdifReader::getLdifLine(std::string &ldifline)
{
    DEBUG(LDAP_DEBUG_TRACE, "-> LdifReader::getLdifLine()" << std::endl);

    this->m_lineNumber++;
    if ( ! getline(m_ldifstream, ldifline) )
    {
        return -1;
    }
    while ( m_ldifstream &&
        (m_ldifstream.peek() == ' ' || m_ldifstream.peek() == '\t'))
    {
        std::string cat;
        m_ldifstream.ignore();
        getline(m_ldifstream, cat);
        ldifline += cat;
        this->m_lineNumber++;
    }

    DEBUG(LDAP_DEBUG_TRACE, "<- LdifReader::getLdifLine()" << std::endl);
    return 0;
}

void LdifReader::splitLine(
            const std::string& line, 
            std::string &type,
            std::string &value) const
{
    std::string::size_type pos = line.find(':');
    if ( pos == std::string::npos )
    {
        DEBUG(LDAP_DEBUG_ANY, "Invalid LDIF line. No `:` separator" 
                << std::endl );
        std::ostringstream err;
        err << "Line " << this->m_lineNumber << ": Invalid LDIF line. No `:` separator";
        throw( std::runtime_error( err.str() ));
    }

    type = line.substr(0, pos);
    if ( pos == line.size() )
    {
        // empty value
        value = "";
        return;
    }

    pos++;
    char delim = line[pos];
    if ( delim == ':' || delim == '<' )
    {
        pos++;
    }

    for( ; pos < line.size() && isspace(line[pos]); pos++ )
    { /* empty */ }

    value = line.substr(pos);

    if ( delim == ':' )
    {
        // Base64 encoded value
        DEBUG(LDAP_DEBUG_TRACE, "  base64 encoded value" << std::endl );
        char outbuf[value.size()];
        int rc = sasl_decode64(value.c_str(), value.size(), 
                outbuf, value.size(), NULL);
        if( rc == SASL_OK )
        {
            value = std::string(outbuf);
        }
        else if ( rc == SASL_BADPROT )
        {
            value = "";
            DEBUG( LDAP_DEBUG_TRACE, " invalid base64 content" << std::endl );
            std::ostringstream err;
            err << "Line " << this->m_lineNumber << ": Can't decode Base64 data";
            throw( std::runtime_error( err.str() ));
        }
        else if ( rc == SASL_BUFOVER )
        {
            value = "";
            DEBUG( LDAP_DEBUG_TRACE, " not enough space in output buffer" 
                    << std::endl );
            std::ostringstream err;
            err << "Line " << this->m_lineNumber 
                << ": Can't decode Base64 data. Buffer too small";
            throw( std::runtime_error( err.str() ));
        }
    }
    else if ( delim == '<' )
    {
        // URL value
        DEBUG(LDAP_DEBUG_TRACE, "  url value" << std::endl );
        std::ostringstream err;
        err << "Line " << this->m_lineNumber 
            << ": URLs are currently not supported";
        throw( std::runtime_error( err.str() ));
    }
    else 
    {
        // "normal" value
        DEBUG(LDAP_DEBUG_TRACE, "  string value" << std::endl );
    }
    DEBUG(LDAP_DEBUG_TRACE, "  Type: <" << type << ">" << std::endl );
    DEBUG(LDAP_DEBUG_TRACE, "  Value: <" << value << ">" << std::endl );
    return;
}

std::string LdifReader::readIncludeLine( const std::string& line ) const
{
    std::string::size_type pos = sizeof("file:") - 1;
    std::string scheme = line.substr( 0, pos );
    std::string file;

    // only file:// URLs supported currently
    if ( scheme != "file:" )
    {
        DEBUG( LDAP_DEBUG_TRACE, "unsupported scheme: " << scheme 
                << std::endl);
    }
    else if ( line[pos] == '/' )
    {
        if ( line[pos+1] == '/' )
        {
            pos += 2;
        }
        file = line.substr(pos, std::string::npos);
        DEBUG( LDAP_DEBUG_TRACE, "target file: " << file << std::endl);
    }
    return file;
}

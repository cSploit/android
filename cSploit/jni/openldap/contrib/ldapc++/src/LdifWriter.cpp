// $OpenLDAP$
/*
 * Copyright 2008-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "LdifWriter.h"
#include "StringList.h"
#include "LDAPAttribute.h"
#include "debug.h"
#include <sstream>
#include <stdexcept>

LdifWriter::LdifWriter( std::ostream& output, int version ) :
        m_ldifstream(output), m_version(version), m_addSeparator(false)
{
    if ( version )
    {
        if ( version == 1 )
        {
            m_ldifstream << "version: " << version << std::endl;
            m_addSeparator = true;
        } else {
            std::ostringstream err;
            err << "Unsuported LDIF Version";
            throw( std::runtime_error(err.str()) );
        }
    }
    
}

void LdifWriter::writeRecord(const LDAPEntry& le)
{
    std::ostringstream line;

    if ( m_addSeparator )
    {
        m_ldifstream << std::endl;
    } else {
        m_addSeparator = true;
    }

    line << "dn: " << le.getDN();
    this->breakline( line.str(), m_ldifstream );

    const LDAPAttributeList *al = le.getAttributes();
    LDAPAttributeList::const_iterator i = al->begin();
    for ( ; i != al->end(); i++ )
    {
        StringList values = i->getValues();
        StringList::const_iterator j = values.begin();
        for( ; j != values.end(); j++)
        {
            // clear output stream
            line.str("");
            line << i->getName() << ": " << *j;
            this->breakline( line.str(), m_ldifstream );
        }
    }
}

void LdifWriter::writeIncludeRecord( const std::string& target )
{
    DEBUG(LDAP_DEBUG_TRACE, "writeIncludeRecord: " << target << std::endl);
    std::string scheme = target.substr( 0, sizeof("file:")-1 );
    
    if ( m_version == 1 )
    {
        std::ostringstream err;
        err << "\"include\" not allowed in LDIF version 1.";
        throw( std::runtime_error(err.str()) );
    }
    
    if ( m_addSeparator )
    {
        m_ldifstream << std::endl;
    } else {
        m_addSeparator = true;
    }

    m_ldifstream << "include: ";
    if ( scheme != "file:" )
    {
        m_ldifstream << "file://";
    }

    m_ldifstream << target << std::endl;
}

void LdifWriter::breakline( const std::string &line, std::ostream &out )
{
    std::string::size_type pos = 0;
    std::string::size_type linelength = 76;
    bool first = true;
    
    if ( line.length() >= linelength )
    {
        while ( pos < line.length() )
        {
            if (! first )
            {
                out << " ";
            }
            out << line.substr(pos, linelength) << std::endl;
            pos += linelength;
            if ( first )
            {
                first = false;
                linelength--; //account for the leading space
            }
        }
    } else {
        out << line << std::endl;
    }
}


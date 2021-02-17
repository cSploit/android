// $OpenLDAP$
/*
 * Copyright 2008-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef LDIF_READER_H
#define LDIF_READER_H

#include <LDAPEntry.h>
#include <iosfwd>
#include <list>

typedef std::list< std::pair<std::string, std::string> > LdifRecord;
class LdifReader
{
    public:
        LdifReader( std::istream &input );

        inline bool isEntryRecords() const
        {
            return !m_ldifTypeRequest;
        }

        inline bool isChangeRecords() const
        {
            return m_ldifTypeRequest;
        }

        inline int getVersion() const
        {
            return m_version;
        }

        LDAPEntry getEntryRecord();
        int readNextRecord( bool first=false );
        //LDAPRequest getChangeRecord();

    private:
        int getLdifLine(std::string &line);

        void splitLine(const std::string& line, 
                    std::string &type,
                    std::string &value ) const;

        std::string readIncludeLine( const std::string &line) const;

        std::istream &m_ldifstream;
        LdifRecord m_currentRecord;
        int m_version;
        int m_curRecType;
        int m_lineNumber;
        bool m_ldifTypeRequest;
        bool m_currentIsFirst;
};

#endif /* LDIF_READER_H */

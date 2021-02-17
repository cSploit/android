/*
 *	PROGRAM:	Firebird Trace Services
 *	MODULE:		TraceConfiguration.h
 *	DESCRIPTION:	Trace Configuration Reader
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Khorsun Vladyslav
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Khorsun Vladyslav <hvlad@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#include "TraceConfiguration.h"
#include "TraceUnicodeUtils.h"
#include "../../jrd/evl_string.h"
#include "../../jrd/SimilarToMatcher.h"
#include "../../common/isc_f_proto.h"

using namespace Firebird;


void TraceCfgReader::readTraceConfiguration(const char* text,
		const PathName& databaseName,
		TracePluginConfig& config)
{
	TraceCfgReader cfgReader(text, databaseName, config);
	cfgReader.readConfig();
}


#define PATH_PARAMETER(NAME, VALUE) \
	if (!found && el->name == #NAME) { \
		Firebird::PathName temp; \
		expandPattern(el, temp); \
		m_config.NAME = temp.c_str(); \
		found = true; \
	}
#define STR_PARAMETER(NAME, VALUE) \
	if (!found && el->name == #NAME) { \
		m_config.NAME = el->value; \
		found = true; \
	}
#define BOOL_PARAMETER(NAME, VALUE) \
	if (!found && el->name == #NAME) { \
		m_config.NAME = parseBoolean(el); \
		found = true; \
	}
#define UINT_PARAMETER(NAME, VALUE) \
	if (!found && el->name == #NAME) { \
		m_config.NAME = parseUInteger(el); \
		found = true; \
	}


namespace
{
	template <typename PrevConverter = Jrd::NullStrConverter>
	class SystemToUtf8Converter : public PrevConverter
	{
	public:
		SystemToUtf8Converter(MemoryPool& pool, Jrd::TextType* obj, const UCHAR*& str, SLONG& len)
			: PrevConverter(pool, obj, str, len)
		{
			buffer.assign(reinterpret_cast<const char*>(str), len);
			ISC_systemToUtf8(buffer);
			str = reinterpret_cast<const UCHAR*>(buffer.c_str());
			len = buffer.length();
		}

	private:
		string buffer;
	};
}

#define ERROR_PREFIX "error while parsing trace configuration\n\t"

void TraceCfgReader::readConfig()
{
	ConfigFile cfgFile(ConfigFile::USE_TEXT, m_text, ConfigFile::HAS_SUB_CONF | ConfigFile::NATIVE_ORDER);

	m_subpatterns[0].start = 0;
	m_subpatterns[0].end = m_databaseName.length();
	for (size_t i = 1; i < FB_NELEM(m_subpatterns); i++)
	{
		m_subpatterns[i].start = -1;
		m_subpatterns[i].end = -1;
	}

	bool defDB = false, defSvc = false, exactMatch = false;
	const ConfigFile::Parameters& params = cfgFile.getParameters();
	for (size_t n = 0; n < params.getCount() && !exactMatch; ++n)
	{
		const ConfigFile::Parameter* section = &params[n];

		const bool isDatabase = (section->name == "database");
		if (!isDatabase && section->name != "services")
			continue;

		const ConfigFile::String pattern = section->value;
		bool match = false;
		if (pattern.empty())
		{
			if (isDatabase)
			{
				if (defDB)
				{
					fatal_exception::raiseFmt(ERROR_PREFIX
						"line %d: second default database section is not allowed",
						section->line);
				}

				match = !m_databaseName.empty();
				//match = m_databaseName.empty();
				defDB = true;
			}
			else
			{
				if (defSvc)
				{
					fatal_exception::raiseFmt(ERROR_PREFIX
						"line %d: second default service section is not allowed",
						section->line);
				}
				match = m_databaseName.empty();
				defSvc = true;
			}
		}
		else if (isDatabase && !m_databaseName.empty())
		{
			if (m_databaseName == pattern.c_str())
				match = exactMatch = true;
			else
			{
				bool regExpOk = false;
				try
				{
#ifdef WIN_NT	// !CASE_SENSITIVITY
					typedef Jrd::UpcaseConverter<SystemToUtf8Converter<> > SimilarConverter;
#else
					typedef SystemToUtf8Converter<> SimilarConverter;
#endif

					UnicodeCollationHolder unicodeCollation(*getDefaultMemoryPool());
					Jrd::TextType* textType = unicodeCollation.getTextType();

					SimilarToMatcher<ULONG, Jrd::CanonicalConverter<SimilarConverter> > matcher(
						*getDefaultMemoryPool(), textType, (const UCHAR*) pattern.c_str(),
						pattern.length(), '\\', true);

					regExpOk = true;

					matcher.process((const UCHAR*) m_databaseName.c_str(), m_databaseName.length());
					if (matcher.result())
					{
						for (unsigned i = 0;
							 i <= matcher.getNumBranches() && i < FB_NELEM(m_subpatterns); ++i)
						{
							unsigned start, length;
							matcher.getBranchInfo(i, &start, &length);

							m_subpatterns[i].start = start;
							m_subpatterns[i].end = start + length;
						}

						match = exactMatch = true;
					}
				}
				catch (const Exception&)
				{
					if (regExpOk)
					{
						fatal_exception::raiseFmt(ERROR_PREFIX
							"line %d: error while processing string \"%s\" against regular expression \"%s\"",
							section->line, m_databaseName.c_str(), pattern.c_str());
					}
					else
					{
						fatal_exception::raiseFmt(ERROR_PREFIX
							"line %d: error while compiling regular expression \"%s\"",
							section->line, pattern.c_str());
					}
				}
			}
		}

		if (!match)
			continue;

		const ConfigFile::Parameters& elements = section->sub->getParameters();
		for (size_t p = 0; p < elements.getCount(); ++p)
		{
			const ConfigFile::Parameter* el = &elements[p];

			if (!el->value.hasData())
			{
				fatal_exception::raiseFmt(ERROR_PREFIX
					"line %d: element \"%s\" have no attribute value set",
					el->line, el->name.c_str());
			}

			bool found = false;
			if (isDatabase)
			{
#define DATABASE_PARAMS
				#include "paramtable.h"
#undef DATABASE_PARAMS
			}
			else
			{
#define SERVICE_PARAMS
				#include "paramtable.h"
#undef SERVICE_PARAMS
			}

			if (!found)
			{
				fatal_exception::raiseFmt(ERROR_PREFIX
					"line %d: element \"%s\" is unknown",
					el->line, el->name.c_str());
			}
		}
	}
}

#undef PATH_PARAMETER
#undef STR_PARAMETER
#undef BOOL_PARAMETER
#undef UINT_PARAMETER

bool TraceCfgReader::parseBoolean(const ConfigFile::Parameter* el) const
{
	ConfigFile::String tempValue(el->value);
	tempValue.upper();

	if (tempValue == "1" || tempValue == "ON" || tempValue == "YES" || tempValue == "TRUE")
		return true;
	if (tempValue == "0" || tempValue == "OFF" || tempValue == "NO" || tempValue == "FALSE")
		return false;

	fatal_exception::raiseFmt(ERROR_PREFIX
		"line %d, element \"%s\": \"%s\" is not a valid boolean value",
		el->line, el->name.c_str(), el->value.c_str());
	return false; // Silence the compiler
}

ULONG TraceCfgReader::parseUInteger(const ConfigFile::Parameter* el) const
{
	const char *value = el->value.c_str();
	ULONG result = 0;
	if (!sscanf(value, "%"ULONGFORMAT, &result))
	{
		fatal_exception::raiseFmt(ERROR_PREFIX
			"line %d, element \"%s\": \"%s\" is not a valid integer value",
			el->line, el->name.c_str(), value);
	}
	return result;
}

void TraceCfgReader::expandPattern(const ConfigFile::Parameter* el, PathName& valueToExpand)
{
	valueToExpand = el->value.ToPathName();
	PathName::size_type pos = 0;
	while (pos < valueToExpand.length())
	{
		string::char_type c = valueToExpand[pos];
		if (c == '\\')
		{
			if (pos + 1 >= valueToExpand.length())
			{
				fatal_exception::raiseFmt(ERROR_PREFIX
					"line %d, element \"%s\": pattern is invalid\n\t %s",
					el->line, el->name.c_str(), el->value.c_str());
			}

			c = valueToExpand[pos + 1];
			if (c == '\\')
			{
				// Kill one of the backslash signs and loop again
				valueToExpand.erase(pos, 1);
				pos++;
				continue;
			}

			if (c >= '0' && c <= '9')
			{
				const MatchPos* subpattern = m_subpatterns + (c - '0');
				// Replace value with piece of database name
				valueToExpand.erase(pos, 2);
				if (subpattern->end != -1 && subpattern->start != -1)
				{
					const off_t subpattern_len = subpattern->end - subpattern->start;
					valueToExpand.insert(pos,
						m_databaseName.substr(subpattern->start, subpattern_len).c_str(),
						subpattern_len);
					pos += subpattern_len;
				}
				continue;
			}

			fatal_exception::raiseFmt(ERROR_PREFIX
				"line %d, element \"%s\": pattern is invalid\n\t %s",
				el->line, el->name.c_str(), el->value.c_str());
		}

		pos++;
	}
}

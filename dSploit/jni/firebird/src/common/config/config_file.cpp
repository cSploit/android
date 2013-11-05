/*
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
 *  The Original Code was created by Dmitry Yemanov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2002 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"

#include "../../common/classes/alloc.h"
#include "../../common/classes/auto.h"
#include "../../common/config/config_file.h"
#include "../jrd/os/fbsyslog.h"
#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

// Invalid or missing CONF_FILE may lead to severe errors
// in applications. That's why for regular SERVER builds
// it's better to exit with appropriate diags rather continue
// with missing / wrong configuration.

// config_file works with OS case-sensitivity
typedef Firebird::PathName string;

/******************************************************************************
 *
 *	Strip any comments
 */

bool ConfigFile::stripComments(string& s) const
{
	if (!parsingAliases)
	{
		// Simple, fast processing for firebird.conf
		// Note that this is only a hack. It won't work in case inputLine
		// contains hash-marks embedded in quotes! Not that I know if we
		// should care about that case.
		const string::size_type commentPos = s.find('#');
		if (commentPos != string::npos)
		{
			s = s.substr(0, commentPos);
		}
		return true;
	}

	// Paranoid, slow processing for aliases.conf
	bool equalSeen = false, inString = false;
	const char* iter = s.begin();
	const char* end = s.end();

	while (iter < end)
	{
		switch (*iter)
		{
		case '"':
			if (!equalSeen) // quoted string to the left of = doesn't make sense
				return false;
			inString = !inString; // We don't support embedded quotes
			if (!inString) // we finished a quoted string
			{
				// We don't want trash after closing the quoted string, except comments
				const string::size_type startPos = s.find_first_not_of(" \t\r", iter + 1 - s.begin());
				if (startPos == string::npos || s[startPos] == '#')
				{
					s = s.substr(0, iter + 1 - s.begin());
					return true;
				}
				return false;
			}
			break;
		case '=':
			equalSeen = true;
			break;
		case '#':
			if (!inString)
			{
				s = s.substr(0, iter - s.begin());
				return true;
			}
			break;
		}

		++iter;
	}

	return !inString; // If we are still inside a string, it's error
}

/******************************************************************************
 *
 *	Check whether the given key exists or not
 */

bool ConfigFile::doesKeyExist(const string& key)
{
	checkLoadConfig();

	const string data = getString(key);

	return !data.empty();
}

/******************************************************************************
 *
 *	Return string value corresponding the given key
 */

string ConfigFile::getString(const string& key)
{
	checkLoadConfig();

	size_t pos;
	return parameters.find(key, pos) ? parameters[pos].second : string();
}

/******************************************************************************
 *
 *	Parse key
 */

string ConfigFile::parseKeyFrom(const string& inputLine, string::size_type& endPos)
{
	endPos = inputLine.find_first_of("=");
	if (endPos == string::npos)
	{
		return inputLine;
	}

	return inputLine.substr(0, endPos);
}

/******************************************************************************
 *
 *	Parse value
 */

string ConfigFile::parseValueFrom(string inputLine, string::size_type initialPos)
{
	if (initialPos == string::npos)
	{
		return string();
	}

	// skip leading white spaces
	const string::size_type startPos = inputLine.find_first_not_of("= \t", initialPos);
	if (startPos == string::npos)
	{
		return string();
	}

	inputLine.rtrim(" \t\r");
	// stringComments demands paired quotes but trimming \r may render startPos invalid.
	if (parsingAliases && inputLine.length() > startPos + 1 &&
		inputLine[startPos] == '"' && inputLine.end()[-1] == '"')
	{
		return inputLine.substr(startPos + 1, inputLine.length() - startPos - 2);
	}

	return inputLine.substr(startPos);
}

/******************************************************************************
 *
 *	Load file, if necessary
 */

void ConfigFile::checkLoadConfig()
{
	if (!isLoadedFlg)
	{
		loadConfig();
	}
}

/******************************************************************************
 *
 *	Load file immediately
 */

void ConfigFile::loadConfig()
{
	isLoadedFlg = true;

	parameters.clear();

	Firebird::AutoPtr<FILE, Firebird::FileClose> ifile(fopen(configFile.c_str(), "rt"));

	int BadLinesCount = 0;
	if (!ifile)
	{
		// config file does not exist
		lastMessage = "Missing configuration file: ";
		lastMessage += configFile;
		return;
	}
	string inputLine;

	while (!feof(ifile))
	{
		inputLine.LoadFromFile(ifile);

		const bool goodLine = stripComments(inputLine);
		inputLine.ltrim(" \t\r");

		if (!inputLine.size())
		{
			continue;	// comment-line or empty line
		}

		if (!goodLine || inputLine.find('=') == string::npos)
		{
			const Firebird::string msg =
				(configFile + ": illegal line \"" + inputLine + "\"").ToString();
			Firebird::Syslog::Record(Firebird::Syslog::Warning, msg.c_str());
			BadLinesCount++;
			continue;
		}

		string::size_type endPos;

		string key = parseKeyFrom(inputLine, endPos);
		key.rtrim(" \t\r");
		// TODO: here we must check for correct parameter spelling !
		const string value = parseValueFrom(inputLine, endPos);

		parameters.add(Parameter(getPool(), key, value));
	}
	if (BadLinesCount)
	{
		lastMessage.printf("%d bad lines in %s", BadLinesCount, configFile.c_str());
	}
}

/******************************************************************************
 *
 *	Check for parse/load error
 */

const char* ConfigFile::getMessage()
{
	return lastMessage.nullStr();
}

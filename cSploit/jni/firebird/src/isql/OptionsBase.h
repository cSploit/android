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
 *  The Original Code was created by Claudio Valderrama on 7-Oct-2007
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2007 Claudio Valderrama
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */


#ifndef FB_OPTIONSBASE_H
#define FB_OPTIONSBASE_H

#include <stdio.h>

class OptionsBase
{
public:
	struct optionsMap
	{
		int kw;
		const char* text;
		size_t abbrlen;
	};

	OptionsBase(const optionsMap* inmap, size_t insize, int wrongval);
	int getCommand(const char* cmd) const;
	void showCommands(FILE* out) const;

private:
	const optionsMap* const m_options;
	const size_t m_size;
	int m_wrong;
};

inline OptionsBase::OptionsBase(const optionsMap* inmap, size_t insize, int wrongval)
	: m_options(inmap), m_size(insize), m_wrong(wrongval)
{
}

#endif // FB_OPTIONSBASE_H


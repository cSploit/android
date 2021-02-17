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


#include "firebird.h"
#include "OptionsBase.h"
//#include "../common/utils_proto.h"  // strnicmp
#include "../common/gdsassert.h"


int OptionsBase::getCommand(const char* cmd) const
{
	const size_t swlen = strlen(cmd);
	if (!swlen)
		return m_wrong;

	for (size_t i = 0; i < m_size; ++i)
	{
		const optionsMap& item = m_options[i];
		// As long as we know our input is already uppercased, the call to
		// fb_utils::strnicmp has been replaced by strncmp until the need arises.
		// Same for strcmp v/s stricmp/strcasecmp.
		if (!item.abbrlen)
		{
			if (!strcmp(cmd, item.text))
				return item.kw;
		}
		else if (swlen >= item.abbrlen && !strncmp(cmd, item.text, swlen))
		{
			return item.kw;
		}
	}
	return m_wrong;
}

void OptionsBase::showCommands(FILE* out) const
{
	// Assumes at least the first letter in the commands is uppercased!
	int newline = 0;
	for (char cap = 'A'; cap <= 'Z'; ++cap)
	{
		for (size_t i = 0; i < m_size; ++i)
		{
			const optionsMap& item = m_options[i];
			if (item.text[0] != cap)
				continue;

			const size_t swlen = strlen(item.text);
			fb_assert(swlen >= item.abbrlen || !item.abbrlen);

			if (!item.abbrlen)
				fprintf(out, "%-25s", item.text);
			else
			{
				size_t j = 0;
				for (; j < item.abbrlen; ++j)
					fputc(item.text[j], out);

				for (; j < swlen; ++j)
				{
					char c = item.text[j];
					if (c >= 'A' && c <= 'Z')
						c = c - 'A' + 'a';

					fputc(c, out);
				}

				for (; j < 25; ++j)
					fputc(' ', out);
			}

			if (newline == 2)
			{
				fputc('\n', out);
				newline = 0;
			}
			else
				++newline;
		}
	}

	if (newline) // Last line was without newline.
		fputc('\n', out);
}


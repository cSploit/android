/*
 *	PROGRAM:	Prepare svn's log output to become our ChangeLog
 *	MODULE:		smallog.cpp
 *	DESCRIPTION:	Removes unneeded data from input file.
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
 *  The Original Code was created by Alexander Peshkoff
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2010 Alexander Peshkoff <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include <string.h>
#include <stdio.h>

#define CH_PATHS "Changed paths:"
#define IBN "increment build number"
#define NU "nightly update"

#define TRUNK "/firebird/trunk/"
#define BRANCHES "/firebird/branches/"
#define TAGS "/firebird/tags/"

int main(int ac, char** av)
{
	char s[32 * 1024];
	long retPos = -1;
	bool firstLine = false;
	bool skipped = true;
	const char* master = av[1] ? av[1] : TRUNK;
	char buf[1024];
	const char* branch = NULL;
	if (av[1])
	{
		strcpy(s, av[1]);
		if (s[strlen(s) - 1] == '/')
		    s[strlen(s) - 1] = '\0';
		sprintf(buf, "A %s (from", s);
		branch = buf;
	}

	while (gets(s))
	{
		if (firstLine)
		{
			// r51338 | dimitr | 2010-07-13 16:32:36 +0400 (Tue, 13 Jul 2010) | 1 line
			char* rev = strtok(s, "|");
			char* nam = strtok(NULL, "|");
			char* dat = strtok(NULL, "|");
			char* colon = dat ? strrchr(dat, ':') : NULL;
			if (colon)
				*colon = '\0';
			printf("%s %s\n", dat, nam);
			firstLine = false;
			continue;
		}
		if (!strncmp(s, "----", 4))
		{
			// new item
			if (!skipped)
				puts("");
			firstLine = true;
			skipped = false;
			retPos = ftell(stdout);
			continue;
		}

		if ((strstr(s, IBN) == s) || (strstr(s, NU) == s))
		{
			if (fseek(stdout, retPos, SEEK_SET) < 0)
			{
				perror("fseek");
				return 1;
			}
			skipped = true;
			continue;
		}
		if (skipped)
			continue;
		if (!strcmp(s, CH_PATHS))
			continue;

		char* trunk = strstr(s, master);
		if (branch && strstr(s, branch))
		{
			// branch was created here
			branch = NULL;
			master = TRUNK;
		}
		else if (trunk)
		{
			int l = strlen(trunk + strlen(master));
			memmove(trunk, trunk + strlen(master), l + 1);
		}
		else
		{
			if (strstr(s, TAGS))
				continue;
			if (strstr(s, BRANCHES))
				continue;
			if (strstr(s, TRUNK))
				continue;
		}

		if (strlen(s))
			puts(s);
	}

	return 0;
}

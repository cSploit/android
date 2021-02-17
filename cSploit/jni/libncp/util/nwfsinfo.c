
/*
    nwfsinfo.c - Print the info strings of a server, maybe sometime more.
    Copyright (C) 1996 by Volker Lendecke
  
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


    Revision history:

	0.00  1996			Volker Lendecke
		Initial revision.
	
	1.00  2001, January 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		List server's NCP extensions.

 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ncp/nwcalls.h>
#include <ncp/nwfse.h>

#include "private/libintl.h"
#define _(X) gettext(X)

static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [pattern]\n"), progname);
}

static void
help(void)
{
	printf(_("\n"
		 "usage: %s [options]\n"), progname);
	printf(_("\n"
	       "-h             Print this help text\n"
	       "-S server      Server name to be used\n"
	       "\n"
	       "-d             Print Description Strings\n"
	       "-t             Print File Server's time\n"
	       "-i             Print File Server Information\n"
	       "-e             List Server's NCP Extensions\n"
	       "\n"));
}

static const char* ynval(char* buf, unsigned int i) {
	switch (i) {
		case 0: return _("no");
		case 1: return _("yes");
		default:
			sprintf(buf, _("unknown (%02X)"), i);
			return buf;
	}
}

static void
print_info(struct ncp_file_server_info_2 *info) {
	char b1[20], b2[20];

	printf(_("\n"
	         "Fileservername    %-48s\n\n"), info->ServerName);
	printf(_("Version           %d.%d Revision %c\n"),
	       info->FileServiceVersion, info->FileServiceSubVersion,
	       info->Revision + 'A');
	printf(_("Max. Connections  %d\n"),
	       info->MaximumServiceConnections);
	printf(_("currently in use  %d\n"),
	       info->ConnectionsInUse);
	printf(_("peak connections  %d\n"),
	       info->MaxConnectionsEverUsed);
	printf(_("Max. Volumes      %d\n"),
	       info->NumberMountedVolumes);
	printf(_("SFTLevel          %d\n"),
	       info->SFTLevel);
	printf(_("TTSLevel          %d\n"),
	       info->TTSLevel);
	printf(_("Accountversion    %d\n"),
	       info->AccountVersion);
	printf(_("Queueversion      %d\n"),
	       info->QueueVersion);
	printf(_("Printversion      %d\n"),
	       info->PrintVersion);
	printf(_("Virt.Consolvers.  %d\n"),
	       info->VirtualConsoleVersion);
	printf(_("RestrictionLevel  %d\n"),
	       info->RestrictionLevel);
	printf(_("VAP Version       %d\n"), info->VAPVersion);
	printf(_("Internet Bridge   %d\n"), info->InternetBridge);
	printf(_("Mixed Mode Path   %s\n"), ynval(b1, info->MixedModePathFlag));
	printf(_("Local Login Code  0x%02X\n"), info->LocalLoginInfoCcode);
	printf(_("Product Version   %u.%u.%u\n"), info->ProductMajorVersion,
		info->ProductMinorVersion, info->ProductRevisionVersion);
	printf(_("OS Language       %u\n"), info->OSLanguageID);
	printf(_("Large files       %s\n"), ynval(b2, info->_64BitOffsetsSupportedFlag));
	printf("\n");
	return;
}

static inline size_t my_strftime(char *s, size_t max, const char *fmt, const struct tm *tm) {
	return strftime(s, max, fmt, tm);
}


int
main(int argc, char **argv)
{
	struct ncp_conn *conn;
	int opt;
	long err;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	if ((conn = ncp_initialize(&argc, argv, 2, &err)) == NULL)
	{
		com_err(argv[0], err, _("when initializing"));
		return 1;
	}
	while ((opt = getopt(argc, argv, "h?dtiem")) != EOF)
	{
		switch (opt)
		{
		case 'h':
		case '?':
			help();
			break;
		case 'd':
			{
				char strings[512];
				char *s;
				int err2;

				err2 = ncp_get_file_server_description_strings(conn,
								      strings);
				if (err2) {
					fprintf(stderr, "%s: %s\n", _("could not get strings"),
						strnwerror(err2));
					ncp_close(conn);
					return 1;
				}
				s = strings;
				while (s < strings + 512)
				{
					if (strlen(s) == 0)
					{
						break;
					}
					puts(s);
					s += strlen(s) + 1;
				}
				break;
			}
		case 't':
			{
				time_t t;
				int err2;

				err2 = ncp_get_file_server_time(conn, &t);
				if (err2) {
					fprintf(stderr, "%s: %s\n", _("could not get server time"),
						strnwerror(err2));
					ncp_close(conn);
					return 1;
				}
				{
					char text_server_time[200];
					struct tm* tm;
					
					tm = localtime(&t);
					my_strftime(text_server_time, sizeof(text_server_time), "%c", tm);
					printf("%s\n", text_server_time);
				}
				break;
			}
		case 'i':
			{
				struct ncp_file_server_info_2 info;
				int err2;

				err2 = ncp_get_file_server_information_2(conn, &info, sizeof(info));
				if (err2) {
					fprintf(stderr, "%s: %s\n", _("Could not get server information"),
						strnwerror(err2));
					ncp_close(conn);
					return 1;
				}
				print_info(&info);
				break;
			}
		case 'e':
			{
				char name[MAX_NCP_EXTENSION_NAME_BYTES];
				u_int32_t iterHandle;
				u_int8_t majorV, minorV, rev;
				NWCCODE err2;
				unsigned int cnt;
				
				cnt = 0;
				iterHandle = ~0;
				while ((err2 = NWScanNCPExtensions(conn, &iterHandle, name, &majorV, &minorV, &rev,
						NULL)) == 0) {
					if (!cnt) {
						printf(_("Installed NCP Extensions:\n"
						         "  Name                                Number    Version\n"
							 "  -------------------------------------------------------\n"));
					}
					cnt++;
					printf(_("  %-33s   %08X  %u.%u.%u\n"), name, iterHandle, majorV, minorV, rev);
				}
				if (!cnt) {
					printf(_("No NCP Extensions registered\n"));
				}
				printf("\n");
			}
			break;
		case 'm':
			{
				NWFSE_NLM_LOADED_LIST l;
				u_int32_t iterHandle;
				NWCCODE err2;
				
				iterHandle = 0;
				while ((err2 = NWGetNLMLoadedList(conn, iterHandle, &l)) == 0) {
					unsigned int i;
					
					if (l.NLMsInList == 0) {
						break;
					}
					printf("NLMs loaded %u, returned %u\n", l.numberNLMsLoaded, l.NLMsInList);
					for (i = 0; i < l.NLMsInList; i++) {
						printf("NLM %u\n", l.NLMNums[i]);
					}
					iterHandle = l.NLMNums[l.NLMsInList - 1] + 1;
				}
				printf("Error: %s\n", strnwerror(err2));
			}
			break;
		default:
			usage();
			goto finished;
		}
	}

finished:
	ncp_close(conn);
	return 0;
}

/*
    nwuserlist.c - Lists users attached to server
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

	0.01  1999			Petr Vandrovec <vandrove@vc.cvut.cz>
		Non-IPX addresses listing.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.

	1.01  2000, May 28		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added -q to print object ID.
		
	1.02  2001, January 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Dump open files.

	1.03  2001, February 11		Petr Vandrovec <vandrove@vc.cvut.cz>
		Dump open semaphores.

	1.04  2002, July 21		Petr Vandrovec <vandrove@vc.cvut.cz>
		Dump some statistical information.

 */

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ncp/nwcalls.h>
#include <ncp/nwfse.h>

#include "private/libintl.h"
#define _(X) gettext(X)
#define N_(X) (X)

static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [pattern]\n"), progname);
}

static void
str_trim_right(char *s, char c)
{
	int len = strlen(s) - 1;

	while ((len > 0) && (s[len] == c))
	{
		s[len] = '\0';
		len -= 1;
	}
}

static void
help(void)
{
	printf(_("\n"
		 "usage: %s [options]\n"), progname);
	printf(_("\n"
	       "-h             Print this help text\n"
	       "-S server      Server name to be used\n"
	       "-a             Print Station's addr\n"
	       "-q             Print object ID\n"
	       "-f             List open files\n"
	       "-ft            Print raw information about open files\n"
	       "-fd            Print detailed information about each open file\n"
	       "-fD            Print DOS filename\n"
	       "-s             List open semaphores\n"
	       "-i             List statistical information\n"
	       "-ih            List read/written bytes in human readable format (e.g. 1K, 234M)\n"
	       "-iH            Same as above, but use units of 1000 bytes instead of 1024\n"
	       "\n"));
}

static const char* conntype(unsigned int id) {
	static const char* ct[] = {
		"unset", "CLIB", "NCP", "NLM",     "AFP",  "FTAM",
		"ANCP",  "ACP",  "SMB", "WINSOCK", "HTTP", "UDP",
	};
	if (id > 11) {
		return _("Unknown");
	}
	return ct[id];
}

struct mt {
	unsigned int mask;
	const char* text;
};

static const char* connstatus(unsigned int id) {
	static char tmp[1000];
	static const struct mt fns[] = {
		{ FSE_LOGGED_IN,			N_("logged in") },
		{ FSE_BEING_ABORTED,			N_("being aborted") },
		{ FSE_AUDITED,				N_("audited") },
		{ FSE_NEEDS_SECURITY_CHANGE,		N_("needs security change") },
		{ FSE_MAC_STATION,			N_("MAC station") },
		{ FSE_AUTHENTICATED_TEMPORARY,		N_("temporary authenticated") },
		{ FSE_AUDIT_CONNECTION_RECORDED,	N_("audit connection recorded") },
		{ FSE_DSAUDIT_CONNECTION_RECORDED,	N_("DS audit connection recorded") },
		{ ~0xFF,				N_("???") },
		{ 0,					N_("none") },
	};
	const struct mt* ptr;
	char* dst = tmp;
	for (ptr = fns; ptr->mask; ptr++) {
		if (ptr->mask & id) {
			if (dst != tmp) {
				strcpy(dst, ", ");
				dst += 2;
			}
			strcpy(dst, _(ptr->text));
			dst = strchr(dst, 0);
		}
	}
	if (dst == tmp) {
		strcpy(dst, _(ptr->text));
	}
	return tmp;
}

static void print_value(int format, const char* title, const char* units, u_int64_t value) {
	static const char* si_prefixes = " kMGTPE";
	const char* unitptr;
	unsigned int div;

	if (format == 0) {
		printf(_("        %-21s%llu %s\n"), title, value, units);
		return;
	}
	if (format == 1) {
		div = 1024;
	} else {
		div = 1000;
	}
	unitptr = si_prefixes;
	while (value >= div * 10 && unitptr[1]) {
		value /= div;
		unitptr++;
	}
	if (unitptr == si_prefixes) {
		printf(_("        %-21s%5llu %s\n"), title, value, units);
	} else {
		printf(_("        %-21s%5llu %c%s\n"), title, value, *unitptr, units);
	}
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
	struct ncp_file_server_info info;
	struct ncp_bindery_object user;
	time_t login_time;
	int i;
	int do_help = 0;
	int print_addr = 0;
	int print_id = 0;
	int print_file = 0;
	int print_techinfo = 0;
	int print_details = 0;
	int print_dos = 0;
	int print_semaphores = 0;
	int print_stat = 0;
	int print_stat_k = 0;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	if ((conn = ncp_initialize(&argc, argv, 1, &err)) == NULL)
	{
		com_err(argv[0], err, _("when initializing"));
		goto finished;
	}
	while ((opt = getopt(argc, argv, "h?aqftdDsiH")) != EOF)
	{
		switch (opt)
		{
		case 'h':
			if (!do_help) {
				do_help = 1;
				break;
			}
		case '?':
			help();
			goto finished;
		case 'a':
			print_addr = 1;
			break;
		case 'q':
			print_id = 1;
			break;
		case 'f':
			print_file = 1;
			break;
		case 't':
			print_techinfo = 1;
			break;
		case 'd':
			print_details = 1;
			break;
		case 'D':
			print_dos = 1;
			break;
		case 's':
			print_semaphores = 1;
			break;
		case 'i':
			print_stat = 1;
			break;
		case 'H':
			print_stat_k = 2;
			break;
		default:
			usage();
			goto finished;
		}
	}
	if (do_help) {
		if (!print_stat || print_stat_k) {
			help();
			goto finished;
		}
		print_stat_k = 1;
	}
	if (ncp_get_file_server_information(conn, &info) != 0)
	{
		perror(_("Could not get server information"));
		ncp_close(conn);
		return 1;
	}
	
	if (isatty(1))
	{
		switch (print_id * 2 + print_addr) {
			case 0:
			printf(_("\n%-6s%-21s%-12s\n"
			       "---------------------------------------------"
			       "------\n"),
			       _("Conn"),
			       _("User name"),
			       _("Login time"));
			break;
			case 1:
			printf(_("\n%-6s%-21s%-27s%-12s\n"
			       "---------------------------------------------"
			       "---------------------------------\n"),
			       _("Conn"),
			       _("User name"),
			       _("Station Address"),
			       _("Login time"));
			break;
			case 2:
			printf(_("\n%-6s%-9s%-21s%-12s\n"
			       "------------------------------------------------------"
			       "------\n"),
			       _("Conn"),
			       _("ObjectID"),
			       _("User name"),
			       _("Login time"));
			break;
			case 3:
			printf(_("\n%-6s%-9s%-21s%-27s%-12s\n"
			       "------------------------------------------------------"
			       "---------------------------------\n"),
			       _("Conn"),
			       _("ObjectID"),
			       _("User name"),
			       _("Station Address"),
			       _("Login time"));
			break;
		}
	}
	for (i = 0; i <= info.MaximumServiceConnections; i++)
	{
		char name[49];
		name[48] = '\0';
		if (ncp_get_stations_logged_info(conn, i, &user,
						 &login_time) != 0)
		{
			continue;
		}
		memcpy(name, user.object_name, 48);
		str_trim_right(name, ' ');
		if (print_id)
			printf(_("%4d: %08X %-20s "), i, (unsigned int)user.object_id, name);
		else
			printf(_("%4d: %-20s "), i, name);

		if (print_addr != 0)
		{
			union ncp_sockaddr addr;
			u_int8_t conn_type;

			memset(&addr, 0, sizeof(addr));
			if (ncp_get_internet_address(conn, i, &addr.any, &conn_type)) {
				printf("XXXXXXXX:YYZZYYXXTTXX:QQQQ");
			} else switch (addr.any.sa_family) {
#ifdef NCP_IPX_SUPPORT
				case AF_IPX:	ipx_print_saddr(&addr.ipx);
						break;
#endif
#ifdef NCP_IN_SUPPORT
				case AF_INET:	{
							char q[30];
							u_int32_t sa = ntohl(addr.inet.sin_addr.s_addr);
							sprintf(q, "%d.%d.%d.%d/%d", (u_int8_t)(sa >> 24), (u_int8_t)(sa >> 16),
										     (u_int8_t)(sa >> 8), (u_int8_t)(sa), 
										     ntohs(addr.inet.sin_port));
     							printf("%-26s", q);
     						}
     						break;
#endif
				default:	printf("%-26s", _("Unknown format"));
						break;
			}
			printf(" ");
		}
		{
			char text_login_time[200];
			struct tm* tm;
			
			tm = localtime(&login_time);
			my_strftime(text_login_time, sizeof(text_login_time), "%c", tm);
			printf("%s\n", text_login_time);
		}
		if (print_file) {
			OPEN_FILE_CONN_CTRL ofcc;
			OPEN_FILE_CONN ofc;
			nuint16 ih;
			
			ih = 0;
			while (!NWScanOpenFilesByConn2(conn, i, &ih, &ofcc, &ofc)) {
				char tmp[1000];
				NWCCODE err2;
				char* fname;
				
				err2 = ncp_ns_get_full_name(conn, NW_NS_DOS, print_dos ? NW_NS_DOS : ofc.nameSpace, 1, 
					ofc.volNumber, ofc.dirEntry, NULL, 0, tmp, sizeof(tmp));
				if (err2) {
					fname = ofc.fileName;
				} else {
					fname = tmp;
				}
				if (print_techinfo) {
					printf(_("        File: (%02X:%08X) %s\n"), ofc.volNumber, ofc.dirEntry, fname);
				} else {
					printf(_("        File: %s\n"), fname);
				}
				if (print_details) {
					static const char* lock_bits[] = {N_("locked"), N_("open shareable"),
					                                 N_("logged"), N_("open normal"),
									 N_("rsvd"), N_("rsvd"),
									 N_("TTS locked"), N_("TTS")};
					static const char* acc_bits[] = {N_("read"), N_("write"),
									 N_("deny read"), N_("deny write"),
									 N_("detached"), N_("TTS holding detach"),
									 N_("TTS holding open"), N_("rsvd")};
					char lstr[200];
					char accstr[200];
					char* l2str;
					const char* nmstr;
					char* p;
					int li;
					
					p = NULL;
					for (li = 0; li < 8; li++) {
						if (ofc.lockType & (1 << li)) {
							if (p) {
								strcpy(p, ", ");
								p = strchr(p, 0);
							} else {
								p = lstr;
							}
							strcpy(p, _(lock_bits[li]));
							p = strchr(p, 0);
						}
					}
					if (!p) {
						strcpy(p, _("unlocked"));
					}
					switch (ofc.lockFlag) {
						case 0x00:	l2str = _("Not locked"); break;
						case 0xFE:	l2str = _("Locked by a file lock"); break;
						case 0xFF:	l2str = _("Locked by Begin Share File Set"); break;
						default:	l2str = _("Unknown lock state"); break;
					}
					nmstr = ncp_namespace_to_str(NULL, ofc.nameSpace);
					p = NULL;
					for (li = 0; li < 8; li++) {
						if (ofc.accessControl & (1 << li)) {
							if (p) {
								strcpy(p, ", ");
								p = strchr(p, 0);
							} else {
								p = accstr;
							}
							strcpy(p, _(acc_bits[li]));
							p = strchr(p, 0);
						}
					}
					if (!p) {
						strcpy(p, _("unlocked"));
					}
					if (print_techinfo) {
						printf(_("          Task: %-5u             Lock:   (%02X) %s\n"), ofc.taskNumber, ofc.lockType, lstr);
						printf(_("          Fork count: %-3u                 (%02X) %s\n"), ofc.forkCount, ofc.lockFlag, l2str);
						printf(_("          Namespace:  (%02X) %-5s  Access: (%02X) %s\n"), ofc.nameSpace, nmstr, ofc.accessControl, accstr);
					} else {
						printf(_("          Task: %-5u        Lock:   %s\n"), ofc.taskNumber, lstr);
						printf(_("          Fork count: %-3u            %s\n"), ofc.forkCount, l2str);
						printf(_("          Namespace:  %-5s  Access: %s\n"), nmstr, accstr);
					}
				}
			}
		}
		if (print_semaphores) {
			CONN_SEMAPHORES css;
			CONN_SEMAPHORE cs;
			nuint16 ih;
			
			ih = 0;
			while (!NWScanSemaphoresByConn(conn, i, &ih, &cs, &css) && cs.openCount) {
				printf(_("        Semaphore: %s\n"), cs.semaphoreName);
				if (print_details) {
					printf(_("          Task: %-5u     Value: %-5d     Open Count: %-5u\n"),
						cs.taskNumber, cs.semaphoreValue, cs.openCount);
				}
			}
		}
		if (print_stat) {
			NWFSE_USER_INFO fseui;
			
			err = NWGetUserInfo(conn, i, NULL, &fseui);
			if (!err) {
#define ui fseui.userInfo				
				if (print_techinfo) {
					printf(_("        Type: (%02X) %-9.9s Status: (%08X) %s\n"),
						ui.connServiceType, conntype(ui.connServiceType),
						ui.status, connstatus(ui.status));
					printf(_("        Use count: %-9u ExpTime: %08X  ObjType: %08X\n"), ui.useCount,
						ui.expirationTime, ui.objType);
					printf(_("        Transaction flag: %9u  Filler: %18u\n"),
						ui.transactionFlag, ui.filler);
					printf(_("        Logical lock threshold: %3u  Record lock threshold: %3u\n"),
						ui.logicalLockThreshold, ui.recordLockThreshold);
					printf(_("        File write flags:      0x%02X  File write state:     0x%02X\n"),
						ui.fileWriteFlags, ui.fileWriteState);
					printf(_("        File lock count: %10u  Record lock count: %7u\n"),
						ui.fileLockCount, ui.recordLockCount);
				} else {
					printf(_("        Type: %-14s Status: %s\n"),
						conntype(ui.connServiceType),
						connstatus(ui.status));
				}
				print_value(print_stat_k, _("Bytes read:"), _("B"), ui.totalBytesRead);
				print_value(print_stat_k, _("Bytes written:"), _("B"), ui.totalBytesWritten);
				print_value(print_stat_k, _("Requests:"), "", ui.totalRequests);
				if (ui.heldRequests || ui.heldBytesRead || ui.heldBytesWritten) {
					print_value(print_stat_k, _("Held bytes read:"), _("B"), ui.heldBytesRead);
					print_value(print_stat_k, _("Held bytes written:"), _("B"), ui.heldBytesWritten);
					print_value(print_stat_k, _("Held requests:"), "", ui.heldRequests);
				}
#undef ui
			}
		}
	}

finished:
	ncp_close(conn);
	return 0;
}

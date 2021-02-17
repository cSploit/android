/*
    nwbpvalues.c - List the contents of a SET property of a bindery object on a NetWare server
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
		
	1.01  2001, May 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Add include strings.h.

	1.02  2001, September 23	Petr Vandrovec <vandrove@vc.cvut.cz>
		Return proper error message on error.

 */

#include <ncp/nwcalls.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>

#include "private/libintl.h"
#define _(X) gettext(X)
#define N_(X) (X)

static char *progname;

static void
 print_property(char *prop_name, u_int8_t * val, int segments);

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [options]\n"), progname);
}

static void
help(void)
{
	printf(_("\n"
		 "usage: %s [options]\n"), progname);
	printf(_("\n"
	       "-h             Print this help text\n"
	       "-S server      Server name to be used\n"
	       "-U username    Username sent to server\n"
	       "-P password    Use this password\n"
	       "-n             Do not use any password\n"
	       "-C             Don't convert password to uppercase\n"
	       "\n"
	       "-o object_name Name of object\n"
	       "-t type        Object type (decimal value)\n"
	       "-p property    Name of property to be listed\n"
	       "-v             Verbose object listing\n"
	       "-c             Canonical output, for use with nwbpadd\n"
	       "\n"));
}

int
main(int argc, char *argv[])
{
	struct ncp_conn *conn;
	char *object_name = NULL;
	int object_type = -1;
	char *property_name = NULL;
	u_int8_t property_value[255 * 128];
	int segno;
	int verbose = 0;
	int canonical = 0;
	struct nw_property segment;
	struct ncp_property_info info;
	long err;
	int useConn = 0;

	int result = 1;

	int opt;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	if ((conn = ncp_initialize_2(&argc, argv, 1, NCP_BINDERY_USER, &err, 0)) == NULL)
	{
		useConn = 1;
	}
	while ((opt = getopt(argc, argv, "h?o:t:p:vc")) != EOF)
	{
		switch (opt)
		{
		case 'o':
			object_name = optarg;
			str_upper(object_name);
			break;
		case 't':
			object_type = strtol(optarg, NULL, 0);
			break;
		case 'p':
			property_name = optarg;
			if (strlen(property_name) > 15)
			{
				fprintf(stderr, _("%s: Property Name too long\n"),
					progname);
				exit(1);
			}
			str_upper(property_name);
			break;
		case 'v':
			verbose = 1;
			break;
		case 'c':
			canonical = 1;
			break;
		case 'h':
		case '?':
			help();
			goto finished;
		default:
			usage();
			goto finished;
		}
	}

	if (useConn) {
		const char* path;

		if (optind < argc)
			path = argv[optind];
		else
			path = ".";
		if (NWParsePath(path, NULL, &conn, NULL, NULL) || !conn) {
			fprintf(stderr, _("%s: %s is not ncpfs file or directory\n"), progname, path);
			goto finished;
		}
	}
	if (object_type < 0)
	{
		fprintf(stderr, _("%s: You must specify an object type\n"),
			progname);
		goto finished;
	}
	if (object_name == NULL)
	{
		fprintf(stderr, _("%s: You must specify an object name\n"),
			progname);
		goto finished;
	}
	if (property_name == NULL)
	{
		fprintf(stderr, _("%s: You must specify a property name\n"),
			progname);
		goto finished;
	}
	if ((err = ncp_scan_property(conn, object_type, object_name,
			      0xffffffff, property_name, &info)) != 0)
	{
		const char* errmsg;
		
		if (err == NWE_NCP_NOT_SUPPORTED) {
			errmsg = _("No such property");
		} else {
			errmsg = strnwerror(err);
		}
		fprintf(stderr, _("%s: Could not find property: %s\n"), progname, errmsg);
		goto finished;
	}
	if (canonical != 0)
	{
		printf("%-4.4x\n%s\n", object_type, object_name);
		printf("%s\n%d\n%x\n",
		       info.property_name, info.property_flags, info.property_security);

	}
	segno = 0;
	while ((err = ncp_read_property_value(conn, object_type, object_name,
				       segno + 1, property_name, &segment)) == 0)
	{
		memcpy(&(property_value[segno * 128]), segment.value, 128);
		segno++;
		if (segno == 255) {
			break;
		}
	}

	if (!segno) {
		fprintf(stderr, _("%s: Could not read property value: %s\n"), progname, strnwerror(err));
		goto finished;
	}
	if ((info.property_flags & 2) == 0)
	{
		/* ITEM property */
		if (canonical != 0)
		{
			int i;
			for (i = 0; i < segno * 128; i++)
			{
				printf("%-2.2x\n", property_value[i]);
			}
		} else
		{
			print_property(property_name, property_value, segno);
		}
	} else
	{
		int objects = 32 * segno;
		u_int32_t *value = (u_int32_t *) property_value;
		int i = 0;

		while (i < objects)
		{
			struct ncp_bindery_object o;

			if ((value[i] == 0) || (value[i] == 0xffffffff))
			{
				/* Continue with next segment */
				i = ((i / 32) + 1) * 32;
				continue;
			}
			if (ncp_get_bindery_object_name(conn, ntohl(value[i]),
							&o) == 0)
			{
				if (canonical != 0)
				{
					printf("%-4.4x\n%s\n",
					       (unsigned int) o.object_type,
					       o.object_name);
				} else if (verbose != 0)
				{
					printf("%s %08X %04X\n",
					       o.object_name,
					       (unsigned int) o.object_id,
					       (unsigned int) o.object_type);
				} else
				{
					printf("%s\n", o.object_name);
				}
			}
			i += 1;
		}
	}
	result = 0;

      finished:
	ncp_close(conn);
	return result;
}

static void
print_unknown(u_int8_t * val)
{
	int j = (128 / 16);
	while (1)
	{
		int i;
		for (i = 0; i < 16; i++)
		{
			printf("%02X ", val[i]);
		}
		printf("   [");
		for (i = 0; i < 16; i++)
		{
			printf("%c", isprint(val[i]) ? val[i] : '.');
		}
		j -= 1;
		if (j == 0)
		{
			printf("]\n");
			return;
		}
		printf("]+\n");
		val += 16;
	}
}

static void
print_string(u_int8_t * val, int segments)
{
	puts(val);
	(void)segments;
}

static char *
print_station_addr(const char *fmt, const struct ncp_station_addr *addr, char *buff)
{
	char *ret = buff;

	while (*fmt != 0)
	{
		switch (*fmt)
		{
		case '%':
			switch (*(++fmt))
			{
			case 'N':	/* node */
				{
					int i;
					for (i = 0; i < 6; buff += 2, i++)
					{
						sprintf(buff, "%02X", addr->Node[i]);
					}
				}
				break;
			case 'S':	/* Socket */
				sprintf(buff, "%04X", htons(addr->Socket));
				buff += 4;
				break;
			case 'L':	/* Lan */
				sprintf(buff, "%08X", (u_int32_t)htonl(addr->NetWork));
				buff += 8;
				break;
			case '%':
				*buff++ = '%';
			default:
				break;
			}
			if (*fmt)
			{
				fmt++;
			}
			break;
		default:
			*buff++ = *fmt++;
		}
	}
	*buff = 0;
	return ret;
}

static inline size_t my_strftime(char *s, size_t max, const char *fmt, const struct tm *tm) {
	return strftime(s, max, fmt, tm);
}


static void
print_login_control(u_int8_t * val, int segments)
{
	static const time_t zero_time_t = 0;
	int i, j, mask;
	char buff[32];
	struct ncp_prop_login_control *a = (struct ncp_prop_login_control *) val;

	if (a->LastLogin[2] || a->LastLogin[1] || a->LastLogin[0] ||
	    a->LastLogin[3] || a->LastLogin[4] || a->LastLogin[5])
	{
		char text[200];
		struct tm* tm;
		
		tm = gmtime(&zero_time_t);
		tm->tm_year = a->LastLogin[0];
		if (tm->tm_year < 80)
			tm->tm_year += 100;
		tm->tm_mon = a->LastLogin[1] - 1;
		tm->tm_mday = a->LastLogin[2];
		tm->tm_hour = a->LastLogin[3];
		tm->tm_min = a->LastLogin[4];
		tm->tm_sec = a->LastLogin[5];
		tm->tm_isdst = 0;
		my_strftime(text, sizeof(text), _("Last Login: %x, %X"), tm);
		puts(text);
	} else
	{
		printf(_("Never logged in\n"));
	}
	if (a->Disabled != 0)
	{
		printf(_(" --- Account disabled ---\n"));
	}
	if (a->AccountExpireDate[2] || a->AccountExpireDate[1] ||
	    a->AccountExpireDate[0])
	{
		char text[200];
		struct tm* tm;

		tm = gmtime(&zero_time_t);		
		tm->tm_year = a->AccountExpireDate[0];
		if (tm->tm_year < 80)
			tm->tm_year += 100;
		tm->tm_mon = a->AccountExpireDate[1] - 1;
		tm->tm_mday = a->AccountExpireDate[2];
		tm->tm_isdst = 0;
		my_strftime(text, sizeof(text), _("Account expires on: %x"), tm);
		puts(text);
	}
	if (a->PasswordExpireDate[2] || a->PasswordExpireDate[1] ||
	    a->PasswordExpireDate[0])
	{
		char text[200];
		struct tm* tm;
		
		tm = gmtime(&zero_time_t);
		tm->tm_year = a->PasswordExpireDate[0];
		if (tm->tm_year < 80)
			tm->tm_year += 100;
		tm->tm_mon = a->PasswordExpireDate[1] - 1;
		tm->tm_mday = a->PasswordExpireDate[2];
		tm->tm_isdst = 0;
		my_strftime(text, sizeof(text), _("Password expires on: %x"), tm);
		puts(text);
		printf(_("GraceLogins left: %d\nof max.         : %d\n"),
		       a->GraceLogins, a->MaxGraceLogins);
		printf(_("PasswortChangeInterval   : %d days\n"),
		       ntohs(a->PasswordExpireInterval));
	}
	if ((a->RestrictionMask & 2) != 0)
	{
		printf(_("New password must be different when changing\n"));
	}
	if ((a->RestrictionMask & 1) != 0)
	{
		printf(_("User is not allowed to change password\n"));
	}
	printf(_("Minimal password length  : %d\n"), a->MinPasswordLength);
	if (ntohs(a->MaxConnections) != 0)
	{
		printf(_("Maximum no of connections: %d\n"),
		       ntohs(a->MaxConnections));
	}
	if (a->MaxDiskUsage != 0xFFFFFF7FL)
	{
		printf(_("Maximum DiskQuota : %8d blocks\n"),
		       (u_int32_t)ntohl(a->MaxDiskUsage));
	}
	printf(_("Failed Logins: %5d\n"), ntohs(a->BadLoginCount));

	if (a->BadLoginCountDown != 0L)
	{
		printf(_("Account disabled still %8d seconds\n"),
		       (u_int32_t)ntohl(a->BadLoginCountDown));
	}
	if (a->LastIntruder.NetWork != 0L)
	{
		printf(_("Last \'intruder\' address: %s\n"),
		       print_station_addr("(%L): %N[%S]",
					  &(a->LastIntruder), buff));
	}
	if (a->RestrictionMask & 0xFC)
	{
		printf(_("RestrictionMask : %02X\n"), a->RestrictionMask);
	}
	for (i = 0; i < 42; i++)
	{
		if (a->ConnectionTimeMask[i] != 0xFF)
		{
			i = 101;
		}
	}
	if (i < 100)
	{
		return;
	}
	val = a->ConnectionTimeMask;
	printf(_("Time restrictions:         1 1 1 1 1 1 1 1 1 1 2 2 2 2 ]\n"));
	printf(_("  Day [0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 ]\n"));

	for (i = 0; i < 7; i++)
	{
		char day[20];
		struct tm* tm;
		
		tm = gmtime(&zero_time_t);
		tm->tm_wday = i;
		tm->tm_isdst = 0;
		strftime(day, sizeof(day), "%a", tm);
		printf("  %-4.4s[", day);
		for (j = 0; j < 6; j++)
		{
			for (mask = 1; mask < 0x100; mask <<= 1)
			{
				putchar((*val & mask) ? '*' : ' ');
			}
			val++;
		}
		printf("]\n");
	}
	(void)segments;
}

static void
print_addr(u_int8_t * val, int segments)
{
	char buff[50];
	print_station_addr("(%L): %N[%S]",
			   (struct ncp_station_addr *) val, buff);
	printf("%s\n", buff);
	(void)segments;
}

static void
print_hex(u_int8_t *val, int segments) {
	int i;

	for (i = 0; i < segments; i++) {
		printf(_("Segment: %03d\n"), i + 1);
		print_unknown(val + i * 128);
		printf("\n");
	}
}

static const struct
{
	void (*func) (u_int8_t *, int);
	const char *pname;
}
formats[] =
{
	{ print_string,		"DESCRIPTION"		},
	{ print_string,		"SURNAME"		},
	{ print_string,		"GIVEN_NAME"		},
	{ print_string,		"OBJECT_CLASS"		},
	{ print_string,		"IDENTIFICATION"	},
	{ print_string,		"Q_DIRECTORY"		},
	{ print_login_control,	"LOGIN_CONTROL"		},
	{ print_addr,		"NET_ADDRESS"		},
	{ print_hex,		NULL			},
};

static void
print_property(char *prop_name, u_int8_t * val, int segments)
{
	int i;

	for (i = 0; formats[i].pname != NULL; i++)
	{
		if (strcasecmp(prop_name, formats[i].pname) == 0)
		{
			break;
		}
	}
	formats[i].func(val, segments);
}

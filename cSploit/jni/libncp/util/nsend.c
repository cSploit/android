/*
    nsend.c - Send Messages to users or groups
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

	0.01  1997, April - 1999, April	Philippe Andersson
		Group handling.

	0.02  1999			Petr Vandrovec <vandrove@vc.cvut.cz>
		Connections above 255.
		Send to connection, to object ID.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <ncp/nwcalls.h>

#include "private/libintl.h"
#define _(X) gettext(X)
#define N_(X) (X)

static const char* program_name;

static void sendmessage(NWCONN_HANDLE conn, const char* message, size_t conns, NWCONN_NUM* conn_list, const char* name) {
	size_t i;
	nuint8 results[conns];
	NWCCODE err;
	char server[NW_MAX_SERVER_NAME_LEN + 100]; /* 100 is space for error string */

	err = NWCCGetConnInfo(conn, NWCC_INFO_SERVER_NAME, sizeof(server), server);
	if (err)
		sprintf(server, "<%s>", strnwerror(err));

	/* send message */
	if ((err = NWSendBroadcastMessage(conn, message, conns, conn_list, results)) != 0) {
		fprintf(stderr, _("Unable to send message to %s/%s: %s\n"),
			server, name, strnwerror(err));
		return;
	}
	for (i = 0; i < conns; i++) {
		if (results[i]) {
			const char* ecode;
			
			if (results[i] <= 4) {
				static const char* delivery[] = {
					N_("Illegal station number"),
					N_("Client not logged in"),
					N_("Client not accepting messages"),
					N_("Client already has message")};
				ecode = gettext(delivery[results[i]-1]);
			} else
				ecode = strnwerror(NWE_SERVER_ERROR | results[i]);
				
			fprintf(stderr, _("Message was not sent to %s/%s (station %d): %s\n"),
				server, name, conn_list[i], ecode);
		} else {
			fprintf(stdout, _("Message sent to %s/%s (station %d)\n"),
				server, name, conn_list[i]);
		}
	}
}

static void perform_send(NWCONN_HANDLE conn, const char* message,
		const char* name, nuint16 type) {
	size_t no_conn;
	NWCONN_NUM conn_list[512]; /* should be enough... */
	NWCCODE err;
		
	if ((err = NWGetObjectConnectionNumbers(conn, name, type, 
	           &no_conn, conn_list, sizeof(conn_list)/sizeof(NWCONN_NUM))) != 0) {
		fprintf(stderr, _("Unable to get connection list for %s: %s\n"),
			name, strnwerror(err));
		return;
	}
	if (no_conn) {
		sendmessage(conn, message, no_conn, conn_list, name);
	} else {
		char server[NW_MAX_SERVER_NAME_LEN + 100]; /* 100 is space for error string */

		err = NWCCGetConnInfo(conn, NWCC_INFO_SERVER_NAME, sizeof(server), server);
		if (err)
			sprintf(server, "<%s>", strnwerror(err));

		fprintf(stderr, _("No connection found for %s/%s\n"), server, name);
	}
}

static void usage(void) {
	fprintf(stderr, _("usage: %s [options] [user|group] message\n"), program_name);
}

static void help(void) {
	printf("\n");
	printf(_("usage: %s [options] [user|group] message\n"), program_name);
	printf(_("\n"
	        "-h              Prints this help text\n"
		"-S server       Server name to be used\n"
		"-U user         Username sent to server\n"
		"-P password     Use this password\n"
		"-n              Do not use any password\n"
		"-C              Do not convert password to uppercase\n"
		"\n"
		"-o object_name  Recipient name\n"
		"-t object_type  Recipient type (default=any)\n"
		"-c connid       Recipient connection number\n"
		"-i object_ID    Recipient object ID\n"
		"-a              Do not prepend 'From xxx[x]:' to message\n"
		"\n"));
}

int
main(int argc, char **argv)
{
	struct ncp_conn *conn;
	struct ncp_bindery_object object, user;
	u_int16_t objtype = NCP_BINDERY_WILD;
	char *message = NULL;
	char* target = NULL;
	int adminmode = 0;
	long err;
	int found;
	int opt = 0;
	NWCONN_NUM list[512];
	size_t list_conns = 0;
	u_int32_t objID = 0;
	int last_opt = 0;
	
	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);

	program_name = argv[0];
		
	if ((conn = ncp_initialize(&argc, argv, 1, &err)) == NULL)
	{
		com_err(program_name, err, _("when initializing"));
		exit(1);
	}
	while ((opt = getopt(argc, argv, "ah?o:t:c:i:")) != EOF) {
		switch (opt) {
			case 'a':	adminmode = 1;
					break;
			case 'o':	if (last_opt && (last_opt != 't')) {
						if (opt == last_opt)
							fprintf(stderr, _("%s: multiple -%c are not allowed\n"), program_name, opt);
						else
							fprintf(stderr, _("%s: -%c cannot be used together with -%c\n"), program_name, opt, last_opt);
						exit(1);
					}
					target = optarg;
					last_opt = opt;
					break;
			case 't':	objtype = strtoul(optarg, NULL, 0);
					if (!last_opt)
						last_opt = opt;
					break;
			case 'c':	if (last_opt && (last_opt != opt)) {
						fprintf(stderr, _("%s: -%c cannot be used together with -%c\n"), program_name, opt, last_opt);
						exit(1);
					}
					{
						char* p;
						
						for (p = strtok(optarg, ","); p; p = strtok(NULL, ",")) {
							if (*p) {
								list[list_conns++] = strtoul(p, NULL, 0);
							}
						}
					}
					last_opt = opt;
					break;
			case 'i':	if (last_opt) {
						if (opt == last_opt)
							fprintf(stderr, _("%s: multiple -%c are not allowed\n"), program_name, opt);
						else
							fprintf(stderr, _("%s: -%c cannot be used together with -%c\n"), program_name, opt, last_opt);
						exit(1);
					}
					objID = strtoul(optarg, NULL, 16);
					last_opt = opt;
					break;
			case 'h':
			case '?':
					help();
					exit(1);
			default:
					usage();
					exit(1);
		}
	}
	if ((!last_opt) || (last_opt == 't')) {
		if (optind >= argc) {
			usage();
			exit(1);
		}
		target = argv[optind++];
		last_opt = 'o';
	}
	if (!message) {
		if (optind >= argc) {
			usage();
			exit(1);
		}
		message = argv[optind++];
	}
	/*-------------------------------------------------------*/
	if (!adminmode) {
		char username[1000];
		NWCCODE err2;
		const char* uptr = username;
		char* tmessage = message;
		message = (char *) malloc (300 * sizeof (char));
		message[0] = '\0';

		err2 = NWCCGetConnInfo(conn, NWCC_INFO_USER_NAME, 
			sizeof(username), username);
		if (err2)
			uptr = "?";

		snprintf(message, 300, _("From %s[%d]: %s"), uptr, 
			ncp_get_conn_number(conn), tmessage);
	}
	
	if (last_opt == 'c') {
		sendmessage(conn, message, list_conns, list, "?");
		ncp_close(conn);
		return 0;
	} else if (last_opt == 'i') {
		err = NWGetConnListFromObject(conn, objID, 0, &list_conns, list);
		if (err) {
			fprintf(stderr, _("%s: Unable to get connection list for %08X: %s\n"),
				program_name, objID, strnwerror(err));
			ncp_close(conn);
			return 1;
		}
		if (list_conns)
			sendmessage(conn, message, list_conns, list, "?");
		else {
			char server[NW_MAX_SERVER_NAME_LEN + 100]; /* 100 is space for error string */

			err = NWCCGetConnInfo(conn, NWCC_INFO_SERVER_NAME, sizeof(server), server);
			if (err)
				sprintf(server, "<%s>", strnwerror(err));

			fprintf(stderr, _("No connection found for %s/%08X\n"), server, objID);
		}
		return 0;
	}
	str_upper(target);
  
	/* --------------------------- Scan bindery for users... */
	found = 0;
	object.object_id = NCP_BINDERY_ID_WILDCARD;
	while ((err = ncp_scan_bindery_object(conn, object.object_id,
			objtype, target, &object)) == 0) {
			
		if (object.object_type == NCP_BINDERY_UGROUP) {
			int nseg;
			struct nw_property property;

			found++;
			/* we have a group - let's get it's MEMBER_LIST property */
			nseg = 1;
			do {
				if ((err = ncp_read_property_value(conn, object.object_type, object.object_name,
				       nseg, "GROUP_MEMBERS", &property)) == 0) {
				       	int offset;
					u_int32_t user_id;
					
					/* process id's from prop. string, with conv. to hi-lo format */
					for (offset = 0; (offset < 128) && (user_id = DVAL_HL(property.value, offset)) != 0; offset += 4) {
						/* try to get user name for that id */
						if ((err = ncp_get_bindery_object_name(conn, user_id, &user)) != 0) {
							/* skip NDS members :-( */
							fprintf(stderr, _("%s: Cannot convert member ID 0x%08lX to name: %s\n"),
									program_name, (unsigned long)user_id, strnwerror(err));
						} else {
							perform_send(conn, message, user.object_name, user.object_type);
						}
					}
				} else {
					fprintf(stderr, _("%s: Cannot read members of group %s: %s\n"),
						argv[0], object.object_name, strnwerror(err));
					break;
				}
				nseg++;
			} while (property.more_flag);
		} else if (object.object_type == NCP_BINDERY_USER) {
			found++;
			perform_send(conn, message, object.object_name, object.object_type);
		} /* else other types... servers, print servers... */
	}
	/*-------------------------------------------------------*/
	if (!found) {
		/* execute compatiblility code */
		/* NDS users are available through this call */
		perform_send(conn, message, target, objtype);
	}
	ncp_close(conn);
	return 0;
}

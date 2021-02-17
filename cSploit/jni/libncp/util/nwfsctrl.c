/*
    nwfsctrl.c - Down file server, open bindery, close bindery
    Copyright (C) 1998, 1999  Petr Vandrovec

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

	0.00  1998			Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.
 */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ncp/nwcalls.h>

#include "private/libintl.h"
#define _(X) gettext(X)

#define UNUSED(x) x __attribute__((unused))

static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [options]\n"), progname);
	return;
}

static void
help(void)
{
	printf(_("\n"
	         "usage: %s [options] command\n"), progname);
	printf(_("\n"
	       "-h             Print this help text\n"
	       "-S server      Server name to be used\n"
	       "-U username    Username sent to server\n"
	       "-P password    Use this password\n"
	       "-n             Do not use any password\n"
	       "-C             Don't convert password to uppercase\n"
	       "-p path        Permanent connection to server to be used\n"
	       "\n"
	       "command is one of:\n"
	       "load XXX       load module XXX\n"
	       "unload XXX     unload module XXX\n"
	       "mount XXX      mount volume XXX\n"
	       "dismount XXX   dismount volume XXX\n"
	       "down [force]   down fileserver\n"
	       "open bindery   open bindery\n"
	       "close bindery  close bindery\n"
	       "disable login  disable login to fileserver\n"
	       "enable login   enable login to fileserver\n"
	       "disable tts    disable TTS\n"
	       "enable tts     enable TTS\n"
	       "set XXX=YYY    set server variable XXX to value YYY\n"
	       "clear station XXX   log-outs specified connection\n"
	       "XXX            try execute XXX as .NCF file\n"
	       "\n"
	       "Warning: If you do not have sufficient priviledge level,\n"
	       "         some of commands (load, unload, mount, dismount,\n"
	       "         set, run .NCF) will emit server console broadcast\n"
	       "         to all conected users!\n"
	       "\n"));
}

static const char* skipspaces(const char* str) {
	while (*str && isspace(*str)) str++;
	return str;
}

static const char* skipword(const char* str) {
	while (*str && !isspace(*str)) str++;
	return str;
}

struct cmdlist {
	const char* cmd;
	int (*proc)(const char*);
	int opt;
};

static int cmdexec(const struct cmdlist* cmds, const char* cmd, size_t cmdl,
		const char* param) {
	while (cmds->cmd) {
		size_t l;

		l = strlen(cmds->cmd);
		if ((l == cmdl) && !memcmp(cmd, cmds->cmd, l)) {
			if ((!*param) && (cmds->opt&1)) {
				fprintf(stderr, _("%s: \"%s\" what?\n"), progname, cmds->cmd);
				return 120;
			}
			return cmds->proc(param);
		}
		cmds++;
	}
	return cmds->proc(cmd);
}

static struct ncp_conn *conn;

static int cmd_load(const char* module) {
	NWCCODE nwerr;
	
	nwerr = NWSMLoadNLM(conn, module);
	if (nwerr) {
		fprintf(stderr, _("%s: Unable to load `%s', error 0x%04X (%u)\n"),
			progname, module, nwerr, nwerr);
		return 100;
	}
	return 0;
}

static int cmd_unload(const char* module) {
	NWCCODE nwerr;

	nwerr = NWSMUnloadNLM(conn, module);
	if (nwerr) {
		fprintf(stderr, _("%s: Unable to unload `%s', error 0x%04X (%u)\n"),
			progname, module, nwerr, nwerr);
		return 100;
	}
	return 0;
}

static int cmd_mount(const char* volume) {
	NWCCODE nwerr;
	nuint32 volID;
	
	nwerr = NWSMMountVolume(conn, volume, &volID);
	if (nwerr) {
		fprintf(stderr, _("%s: Unable to mount `%s', error 0x%04X (%u)\n"),
			progname, volume, nwerr, nwerr);
		return 100;
	}
	printf(_("Volume `%s' was mounted as #%u\n"), volume, volID);
	return 0;
}

static int cmd_dismount(const char* volume) {
	NWCCODE nwerr;

	nwerr = NWSMDismountVolumeByName(conn, volume);
	if (nwerr) {
		fprintf(stderr, _("%s: Unable to dismount `%s', error 0x%04X (%u)\n"),
			progname, volume, nwerr, nwerr);
		return 100;
	}
	return 0;
}

static int cmd_down(const char* cmd) {
	NWCCODE nwerr;
	
	nwerr = NWDownFileServer(conn, *cmd == 'f');
	if (nwerr) {
		fprintf(stderr, _("%s: Unable to down file server, error 0x%04X (%u)\n"),
			progname, nwerr, nwerr);
		return 100;
	}
	return 0;
}

static int cmd_run(const char* cmd) {
	NWCCODE nwerr;

	nwerr = NWSMExecuteNCFFile(conn, cmd);
	if (nwerr) {
		fprintf(stderr, _("%s: Unable to execute `%s', error 0x%04X (%u)\n"),
			progname, cmd, nwerr, nwerr);
		return 100;
	}
	return 0;
}

static int cmd_set(const char* cmd) {
	const char* begin = cmd;
	const char* val;
	int numeric;
	u_int32_t value;
	NWCCODE nwerr;
	char* varname;
	size_t varnamelen;

	while (*cmd && (*cmd != '=')) cmd++;
	if (*cmd != '=') {
		fprintf(stderr, _("%s: Syntax error in `set' command\n"),
			progname);
		return 99;
	}
	val = skipspaces(cmd + 1);
	do {
		--cmd;
	} while ((cmd >= begin) && isspace(*cmd));

	varnamelen = cmd - begin + 1;
	varname = (char*)malloc(varnamelen + 1);
	if (!varname) {
		fprintf(stderr, "%s: %s\n",
			progname, strerror(ENOMEM));
		return 99;
	}
	memcpy(varname, begin, varnamelen);
	varname[varnamelen] = 0;

	numeric = 1;
	if (!strcmp(val, "on") || !strcmp(val, "enabled")) {
		value = 1;
	} else if (!strcmp(val, "off") || !strcmp(val, "disabled")) {
		value = 0;
	} else {
		char* eptr2;
		const char* eptr;
		
		value = strtoul(val, &eptr2, 0);
		eptr = eptr2;
		if (eptr) eptr = skipspaces(eptr);
		if (eptr && *eptr) numeric = 0;
	}
	if (numeric) {
		nwerr = NWSMSetDynamicCmdIntValue(conn, varname, value);
		if (!nwerr) return 0;
		if (nwerr != 0x0206) {
			fprintf(stderr, _("%s: Unable to set variable `%s' to value `%s', error 0x%04X (%u)\n"),
			progname, varname, val, nwerr, nwerr);
			free(varname);
			return 100;
		}
	}
	nwerr = NWSMSetDynamicCmdStrValue(conn, varname, val);
	if (nwerr) {
		fprintf(stderr, _("%s: Unable to set variable `%s' to value `%s', error 0x%04X (%u)\n"),
			progname, varname, val, nwerr, nwerr);
		free(varname);
		return 100;
	}
	free(varname);
	return 0;
}

static int opclo;

static int cmd_login(UNUSED(const char* cmd)) {
	NWCCODE nwerr;

	nwerr = opclo ? NWEnableFileServerLogin(conn) : NWDisableFileServerLogin(conn);
	if (nwerr) {
		fprintf(stderr, _("%s: Unable to %s file server login, error 0x%04X (%u)\n"),
			progname, opclo?_("enable"):_("disable"), nwerr, nwerr);
		return 100;
	}
	return 0;
}

static int cmd_tts(UNUSED(const char* cmd)) {
	NWCCODE nwerr;

	nwerr = opclo ? NWEnableTTS(conn) : NWDisableTTS(conn);
	if (nwerr) {
		fprintf(stderr, _("%s: Unable to %s TTS, error 0x%04X (%u)\n"),
			progname, opclo?_("enable"):_("disable"), nwerr, nwerr);
		return 100;
	}
	return 0;
}

static int cmd_bindery(UNUSED(const char* cmd)) {
	NWCCODE nwerr;

	if ((!opclo) && (ncp_get_conn_type(conn) == NCP_CONN_TEMPORARY)) {
		fprintf(stderr, _("%s: Warning: Bindery cannot be closed by temporary connection\n"), progname);
	}
	nwerr = opclo ? NWOpenBindery(conn) : NWCloseBindery(conn);
	if (nwerr) {
		fprintf(stderr, _("%s: Unable to %s bindery, error 0x%04X (%u)\n"),
			progname, opclo?_("open"):_("close"), nwerr, nwerr);
		return 100;
	}
	return 0;
}

static int cmd_clear_station(const char* id) {
	int err = 0;
	
	while (*id) {
		NWCONN_NUM connNum;
		char* ptr2;
		const char* ptr;

		connNum = strtoul(id, &ptr2, 0);
		ptr = ptr2;
		if (ptr && (!*ptr || isspace(*ptr))) {
			NWCCODE nwerr;
			
			nwerr = NWClearConnectionNumber(conn, connNum);
			if (nwerr) {
				fprintf(stderr, _("%s: Unable to clear station %u, error 0x%04X (%u)\n"),
					progname, connNum, nwerr, nwerr);
				err = 100;
			}
		} else {
			ptr = skipword(id);
			fprintf(stderr, _("%s: `%.*s' is not a number, skipping\n"), progname, ptr-id, id);
			if (!err) err = 121;
		}
		id = skipspaces(ptr);
	}
	return err;
}

static int cmd_what(const char* what) {
	fprintf(stderr, _("%s: What is `%s'?\n"), progname, what);
	return 98;
}

static struct cmdlist cmds_endis[] = {
	{ "login", cmd_login, 2 },
	{ "tts", cmd_tts, 2 },
	{ NULL, cmd_what, 0 }};

static struct cmdlist cmds_opclo[] = {
	{ "bindery", cmd_bindery, 2 },
	{ NULL, cmd_what, 0 }};

static struct cmdlist cmds_clear[] = {
	{ "station", cmd_clear_station, 1 },
	{ NULL, cmd_what, 0 }};
	
static int cmd_open(const char* cmd) {
	opclo = 0;
	return cmdexec(cmds_opclo, cmd, strlen(cmd), "");
}

static int cmd_close(const char* cmd) {
	opclo = 1;
	return cmdexec(cmds_opclo, cmd, strlen(cmd), "");
}

static int cmd_disable(const char* cmd) {
	opclo = 0;
	return cmdexec(cmds_endis, cmd, strlen(cmd), "");
}

static int cmd_enable(const char* cmd) {
	opclo = 1;
	return cmdexec(cmds_endis, cmd, strlen(cmd), "");
}

static int cmd_clear(const char* cmd) {
	const char* cmde = skipword(cmd);
	return cmdexec(cmds_clear, cmd, cmde-cmd, skipspaces(cmde));
}

static struct cmdlist cmds[] = {
	{ "load", cmd_load, 1 },
	{ "unload", cmd_unload, 1 },
	{ "mount", cmd_mount, 1 },
	{ "dismount", cmd_dismount, 1 },
	{ "down", cmd_down, 0 },
	{ "open", cmd_open, 1 },
	{ "close", cmd_close, 1 },
	{ "disable", cmd_disable, 1 },
	{ "enable", cmd_enable, 1 },
	{ "set", cmd_set, 1 },
	{ "clear", cmd_clear, 1},
	{ NULL, cmd_run, 0 }};

int
main(int argc, char **argv)
{
	long err;
	int opt;
	int forceDown = 1;
	int useConn = 0;
	int act = 0;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
		
	progname = argv[0];

	if ((conn = ncp_initialize_2(&argc, argv, 1, NCP_BINDERY_USER, &err, 0)) == NULL)
	{
		useConn = 1;
	}
	while ((opt = getopt(argc, argv, "h?fp:cod")) != EOF)
	{
		switch (opt)
		{
		case 'h':
		case '?':
			help();
			exit(1);
		case 'f':
			forceDown = 0;
			act = 3;
			break;
		case 'c':
			act = 1;
			break;
		case 'o':
			act = 2;
			break;
		case 'd':
			act = 3;
			break;
		case 'p':
			if (!useConn) {
				fprintf(stderr, _("-p and -S are incompatible\n"));
				return 123;
			}
			if (NWParsePath(optarg, NULL, &conn, NULL, NULL) || !conn) {
				fprintf(stderr, _("%s: %s is not Netware directory/file\n"),
					progname, optarg);
				return 122;
			}
			break;
		default:
			usage();
			exit(1);
		}
	}
	if (!conn) {
		fprintf(stderr, _("%s: You must specify connection to server\n"), progname);
		return 121;
	}
	if (act) {
		if (optind < argc) {
			fprintf(stderr, _("Compatibility option entered, rest of commandline ignored\n"));
		}
		switch (act) {
			case 1:	opclo = 0;
				err = cmd_bindery("");
				break;
			case 2:	opclo = 1;
				err = cmd_bindery("");
				break;
			default:
				err = cmd_down(forceDown?"force":"");
				break;
		}
	} else {
		int i;
		int l;
		char* cmdl;
		char* cmd2;
		const char* cmd3;
		const char* begin;
		const char* end;

		l = 1;
		for (i = optind; i < argc; i++)
			l += strlen(argv[i]) + 1;
		cmdl = (char*)malloc(l);
		if (!cmdl) {
			fprintf(stderr, _("%s: Out of memory!\n"), progname);
			return 123;
		}
		cmd2 = cmdl;
		for (i = optind; i < argc; i++) {
			if (cmd2 != cmdl)
				*cmd2++ = ' ';
			l = strlen(argv[i]);
			memcpy(cmd2, argv[i], l);
			cmd2 += l;
		}
		*cmd2 = 0;

		cmd3 = skipspaces(cmdl);
		begin = cmd3;
		while (*cmd3 && !isspace(*cmd3)) cmd3++;
		end = cmd3;
		if (begin == end) {
			fprintf(stderr, _("%s: Missing command\n"), progname);
			return 122;
		}
		err = cmdexec(cmds, begin, end-begin, skipspaces(cmd3));
	}
	ncp_close(conn);
	return err;
}

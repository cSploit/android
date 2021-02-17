/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2010  Craig A. Berry	craigberry@mac.com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.

/*
 * Mapping table for the ISQL command.
 */

#include <stdlib.h>

int vmsarg_mapping (int *nvargs, char *vms_arg[], char *unix_arg[],
	char *unix_narg[], char *vms_key[], char *unix_key[],
	char *separator[], int flags[], char *pattern[],
	char **outverb, int *action_flags, char **arg_symbol,
	char **image_name)
{
 *nvargs = 26;
 *action_flags = 1301;
 vms_arg[1] = "USERNAME";
 flags[1] = 1;
 unix_arg[1] = "-U";

 vms_arg[2] = "PASSWORD";
 flags[2] = 1;
 unix_arg[2] = "-P";

 vms_arg[3] = "SERVER_NAME";
 flags[3] = 1;
 unix_arg[3] = "-S";

 vms_arg[4] = "TIMEOUT";
 flags[4] = 1;
 unix_arg[4] = "-t";

 vms_arg[5] = "INPUT";
 flags[5] = 1;
 unix_arg[5] = "-i";

 vms_arg[6] = "OUTPUT";
 flags[6] = 1;
 unix_arg[6] = "-o";

 vms_arg[7] = "VERSION";
 flags[7] = 1;
 unix_arg[7] = "-v";

 vms_arg[8] = "ECHO";
 flags[8] = 1;
 unix_arg[8] = "-e";

 vms_arg[9] = "FIPS";
 flags[9] = 1;
 unix_arg[9] = "-F";

 vms_arg[10] = "STATISTICS";
 flags[10] = 1;
 unix_arg[10] = "-p";

 vms_arg[11] = "NOPROMPT";
 flags[11] = 1;
 unix_arg[11] = "-n";

 vms_arg[12] = "ENCRYPT";
 flags[12] = 1;
 unix_arg[12] = "-X";

 vms_arg[13] = "CHAIN_XACT";
 flags[13] = 1;
 unix_arg[13] = "-Y";

 vms_arg[14] = "DISPCHARSET";
 flags[14] = 1;
 unix_arg[14] = "-a";

 vms_arg[15] = "TDSPACKETSIZE";
 flags[15] = 1;
 unix_arg[15] = "-A";

 vms_arg[16] = "TERMINATOR";
 flags[16] = 1;
 unix_arg[16] = "-c";

 vms_arg[17] = "EDITOR";
 flags[17] = 1;
 unix_arg[17] = "-E";

 vms_arg[18] = "ROWSINPAGE";
 flags[18] = 1;
 unix_arg[18] = "-h";

 vms_arg[19] = "HOSTNAME";
 flags[19] = 1;
 unix_arg[19] = "-H";

 vms_arg[20] = "INTERFACE";
 flags[20] = 1;
 unix_arg[20] = "-I";

 vms_arg[21] = "CLIENTCHARSET";
 flags[21] = 1;
 unix_arg[21] = "-J";

 vms_arg[22] = "ERRORLEVEL";
 flags[22] = 1;
 unix_arg[22] = "-m";

 vms_arg[23] = "LOGINTIME";
 flags[23] = 1;
 unix_arg[23] = "-l";

 vms_arg[24] = "COLSEPARATOR";
 flags[24] = 1;
 unix_arg[24] = "-s";

 vms_arg[25] = "WIDTH";
 flags[25] = 1;
 unix_arg[25] = "-w";

 vms_arg[26] = "LANGUAGE";
 flags[26] = 1;
 unix_arg[26] = "-z";

 /* Because vi is not a reasonable default on VMS.
  */
 if (setenv("EDITOR", "edit", 0) != 0)
	return 0;

 return 1;
}

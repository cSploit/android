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
 * Mapping table for the BCP command.
 */

int vmsarg_mapping (int *nvargs, char *vms_arg[], char *unix_arg[],
	char *unix_narg[], char *vms_key[], char *unix_key[],
	char *separator[], int flags[], char *pattern[],
	char **outverb, int *action_flags, char **arg_symbol,
	char **image_name)
{
 *nvargs = 26;
 *action_flags = 1301;
 vms_arg[1] = "TABLE_NAME";
 flags[1] = 1;
 unix_arg[1] = "$";

 vms_arg[2] = "DIRECTION";
 flags[2] = 1;
 unix_arg[2] = "$";

 vms_arg[3] = "DATA_FILE";
 flags[3] = 1;
 unix_arg[3] = "$";

 vms_arg[4] = "MAX_ERRORS";
 flags[4] = 1;
 unix_arg[4] = "-m";

 vms_arg[5] = "FORMAT_FILE";
 flags[5] = 1;
 unix_arg[5] = "-f";

 vms_arg[6] = "ERRORFILE";
 flags[6] = 1;
 unix_arg[6] = "-e";

 vms_arg[7] = "FIRST_ROW";
 flags[7] = 1;
 unix_arg[7] = "-F";

 vms_arg[8] = "LAST_ROW";
 flags[8] = 1;
 unix_arg[8] = "-L";

 vms_arg[9] = "BATCH_SIZE";
 flags[9] = 1;
 unix_arg[9] = "-b";

 vms_arg[10] = "NATIVE_DEFAULT";
 flags[10] = 1;
 unix_arg[10] = "-n";

 vms_arg[11] = "CHARACTER_DEFAULT";
 flags[11] = 1;
 unix_arg[11] = "-c";

 vms_arg[12] = "COLUMN_TERMINATOR";
 flags[12] = 1;
 unix_arg[12] = "-t";

 vms_arg[13] = "ROW_TERMINATOR";
 flags[13] = 1;
 unix_arg[13] = "-r";

 vms_arg[14] = "USERNAME";
 flags[14] = 1;
 unix_arg[14] = "-U";

 vms_arg[15] = "PASSWORD";
 flags[15] = 1;
 unix_arg[15] = "-P";

 vms_arg[16] = "SERVER_NAME";
 flags[16] = 1;
 unix_arg[16] = "-S";

 vms_arg[17] = "INPUT";
 flags[17] = 1;
 unix_arg[17] = "-i";

 vms_arg[18] = "OUTPUT";
 flags[18] = 1;
 unix_arg[18] = "-o";

 vms_arg[19] = "TDSPACKETSIZE";
 flags[19] = 1;
 unix_arg[19] = "-A";

 vms_arg[20] = "TEXTSIZE";
 flags[20] = 1;
 unix_arg[20] = "-T";

 vms_arg[21] = "VERSION";
 flags[21] = 1;
 unix_arg[21] = "-v";

 vms_arg[22] = "DISPCHARSET";
 flags[22] = 1;
 unix_arg[22] = "-a";

 vms_arg[23] = "LANGUAGE";
 flags[23] = 1;
 unix_arg[23] = "-z";

 vms_arg[24] = "CLIENTCHARSET";
 flags[24] = 1;
 unix_arg[24] = "-J";

 vms_arg[25] = "IDENTITY";
 flags[25] = 1;
 unix_arg[25] = "-E";

 vms_arg[26] = "ENCRYPT";
 flags[26] = 1;
 unix_arg[26] = "-X";

 return 1;
}

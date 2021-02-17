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
 * Mapping table for the DEFNCOPY command.
 */

int vmsarg_mapping (int *nvargs, char *vms_arg[], char *unix_arg[],
	char *unix_narg[], char *vms_key[], char *unix_key[],
	char *separator[], int flags[], char *pattern[],
	char **outverb, int *action_flags, char **arg_symbol,
	char **image_name)
{
 *nvargs = 12;
 *action_flags = 1301;
 vms_arg[1] = "DIRECTION";
 flags[1] = 1;
 unix_arg[1] = "$";

 vms_arg[2] = "FILE_NAME";
 flags[2] = 1;
 unix_arg[2] = "$";

 vms_arg[3] = "DATABASE_NAME";
 flags[3] = 1;
 unix_arg[3] = "$";

 vms_arg[4] = "OBJECT_NAME";
 flags[4] = 1;
 unix_arg[4] = "$";

 vms_arg[5] = "USERNAME";
 flags[5] = 1;
 unix_arg[5] = "-U";

 vms_arg[6] = "PASSOWRD";
 flags[6] = 1;
 unix_arg[6] = "-P";

 vms_arg[7] = "SERVER_NAME";
 flags[7] = 1;
 unix_arg[7] = "-S";

 vms_arg[8] = "VERSION";
 flags[8] = 1;
 unix_arg[8] = "-v";

 vms_arg[9] = "INTERFACES";
 flags[9] = 1;
 unix_arg[9] = "-I";

 vms_arg[10] = "DISPCHARSET";
 flags[10] = 1;
 unix_arg[10] = "-a";

 vms_arg[11] = "LANGUAGE";
 flags[11] = 1;
 unix_arg[11] = "-z";

 vms_arg[12] = "CLIENTCHARSET";
 flags[12] = 1;
 unix_arg[12] = "-J";

 return 1;
}

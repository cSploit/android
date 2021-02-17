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
 */

/*      $Id: edit.c,v */

#include <namdef.h>
#include <descrip.h>
#include <stsdef.h>
#include <tpu$routines.h>
#include <string.h>
#include <unixlib.h>

#if defined(NAML$C_MAXRSS) && !defined(__VAX)
#  define VMS_MAXRSS (NAML$C_MAXRSS+1)
#else
#  define VMS_MAXRSS (NAM$C_MAXRSS+1)
#endif

char vms_fnm[VMS_MAXRSS];
static int get_vms_name(char *, int);

int
edit(const char *editor, const char *arg)
{
    $DESCRIPTOR(fnm_dsc, "");
    int status;

    decc$to_vms(arg, get_vms_name, 0, 0);

    fnm_dsc.dsc$a_pointer = vms_fnm;
    fnm_dsc.dsc$w_length = strlen(vms_fnm);

    if (fnm_dsc.dsc$a_pointer == NULL
        || fnm_dsc.dsc$w_length == 0)
        return -1;

    status = TPU$EDIT(&fnm_dsc, &fnm_dsc);

    if ($VMS_STATUS_SUCCESS(status))
        return 0;
    else
        return -1;
}

static int
get_vms_name(char *name, int type)
{
    strncpy(vms_fnm, name, VMS_MAXRSS);
    return 1;
}

int
reset_term()
{
        return 0;
}


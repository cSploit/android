/* Free ISQL - An isql for DB-Library (C) 2007 Nicholas S. Castellano
 *
 * This program  is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <string.h>
#include <sybfront.h>
#include <sybdb.h>
#include "handlers.h"

int global_errorlevel = -1;

int
err_handler(DBPROCESS * dbproc, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr)
{
	if ((dbproc == NULL) || (DBDEAD(dbproc))) {
		return (INT_EXIT);
	}
	if (dberr != SYBESMSG) {
		fprintf(stdout, "DB-LIBRARY error:\n\t%s\n", dberrstr);
	}
	if (oserr != DBNOERR) {
		fprintf(stdout, "Operating-system error:\n\t%s\n", oserrstr);
	}
	return (INT_CANCEL);
}

int
msg_handler(DBPROCESS * dbproc, DBINT msgno, int msgstate, int severity, char *msgtext, char *srvname, char *procname, int line)
{
	if (severity > 10) {
		if ((global_errorlevel == -1) || (severity >= global_errorlevel)) {
			fprintf(stdout, "Msg %ld, Level %d, State %d:\n", (long) msgno, severity, msgstate);
		}
		if (global_errorlevel == -1) {
			if (strlen(srvname) > 0) {
				fprintf(stdout, "Server '%s', ", srvname);
			}
			if (strlen(procname) > 0) {
				fprintf(stdout, "Procedure '%s', ", procname);
			}
			if (line > 0) {
				fprintf(stdout, "Line %d:\n", line);
			}
		}
	}
	if (global_errorlevel == -1) {
		fprintf(stdout, "%s\n", msgtext);
	}
	return (0);
}

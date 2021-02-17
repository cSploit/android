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

/*	$Id: handlers.h,v 1.4 2007-01-20 06:32:27 castellano Exp $	*/
extern int global_errorlevel;

int err_handler(DBPROCESS * dbproc, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr);

int msg_handler(DBPROCESS * dbproc, DBINT msgno, int msgstate,
		int severity, char *msgtext, char *srvname, char *procname, int line);

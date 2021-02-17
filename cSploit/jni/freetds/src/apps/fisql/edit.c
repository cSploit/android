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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sybfront.h>
#include <sybdb.h>
#include "edit.h"
#include "terminal.h"

int
edit(const char *editor, const char *arg)
{
	int pid;

      retry_fork:
	pid = fork();
	switch (pid) {
	case -1:
		if (errno == EAGAIN) {
			sleep(5);
			goto retry_fork;
		}
		perror("fisql");
		reset_term();
		dbexit();
		exit(EXIT_FAILURE);
		break;
	case 0:
		execlp(editor, editor, arg, (char *) 0);
		fprintf(stderr, "Unable to invoke the '%s' editor.\n", editor);
		exit(EXIT_FAILURE);
		break;
	default:
		waitpid(pid, (int *) 0, 0);
		break;
	}
	return 0;
}

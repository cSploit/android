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
#include <unistd.h>
#include <termios.h>
#include "terminal.h"

static struct termios term;
static struct termios oterm;
static int term_init = 0;

int
save_term()
{
	int r;

	if (!isatty(fileno(stdin))) {
		return 1;
	}
	if (term_init) {
		return 0;
	}
	if ((r = tcgetattr(fileno(stdin), &oterm)) != 0) {
		return r;
	}
	term_init = 1;
	return 0;
}

int
set_term_noecho()
{
	int r;

	if (!isatty(fileno(stdin))) {
		return 1;
	}
	if ((r = save_term()) != 0) {
		return r;
	}
	if ((r = tcgetattr(fileno(stdin), &term)) != 0) {
		return r;
	}
	term.c_lflag &= ~(ICANON | ECHO);
	if ((r = tcsetattr(fileno(stdin), TCSANOW, &term)) != 0) {
		return r;
	}
	return 0;
}

int
reset_term()
{
	int r;

	if (!isatty(fileno(stdin))) {
		return 1;
	}
	if ((r = save_term()) != 0) {
		return r;
	}
	if ((r = tcsetattr(fileno(stdin), TCSANOW, &oterm)) != 0) {
		return r;
	}
	return 0;
}

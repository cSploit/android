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

/*	$Id: interrupt.h,v 1.4 2007-01-20 06:32:27 castellano Exp $	*/
extern sigjmp_buf restart;

void inactive_interrupt_handler(int sig);
void active_interrupt_handler(int sig);
void maybe_handle_active_interrupt(void);
int active_interrupt_pending(DBPROCESS * dbproc);
int active_interrupt_servhandler(DBPROCESS * dbproc);

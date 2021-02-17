/*
 *      PROGRAM:        UTILITIES shared functions prototypes.
 *      MODULE:         util_proto.h
 *      DESCRIPTION:    Prototype header file for util.cpp
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#ifndef UTIL_PROTO_H
#define UTIL_PROTO_H

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif


pid_t	UTIL_start_process(const char* process, char** argv, const char* prog_name);
int 	UTIL_wait_for_child(pid_t child_pid, const volatile sig_atomic_t& shutting_down);
int 	UTIL_shutdown_child(pid_t child_pid, unsigned timeout_term, unsigned timeout_kill);
int 	UTIL_ex_lock(const char* file);
void	UTIL_ex_unlock(int fd_file);
int 	UTIL_set_handler(int sig, void (*handler) (int), bool restart);

#endif // UTIL_PROTO_H


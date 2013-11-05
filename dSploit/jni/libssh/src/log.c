/*
 * log.c - logging and debugging functions
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2008      by Aris Adamantiadis
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "libssh/priv.h"
#include "libssh/session.h"

/**
 * @defgroup libssh_log The SSH logging functions.
 * @ingroup libssh
 *
 * Logging functions for debugging and problem resolving.
 *
 * @{
 */

/** @internal
 * @brief do the actual work of logging an event
 */

static void do_ssh_log(struct ssh_common_struct *common, int verbosity,
    const char *buffer){
  char indent[256];
  int min;
  if (common->callbacks && common->callbacks->log_function) {
    common->callbacks->log_function((ssh_session)common, verbosity, buffer,
        common->callbacks->userdata);
  } else if (verbosity == SSH_LOG_FUNCTIONS) {
    if (common->log_indent > 255) {
      min = 255;
    } else {
      min = common->log_indent;
    }

    memset(indent, ' ', min);
    indent[min] = '\0';

    fprintf(stderr, "[func] %s%s\n", indent, buffer);
  } else {
    fprintf(stderr, "[%d] %s\n", verbosity, buffer);
  }
}

/**
 * @brief Log a SSH event.
 *
 * @param session       The SSH session.
 *
 * @param verbosity     The verbosity of the event.
 *
 * @param format        The format string of the log entry.
 */
void ssh_log(ssh_session session, int verbosity, const char *format, ...) {
  char buffer[1024];
  va_list va;

  if (verbosity <= session->common.log_verbosity) {
    va_start(va, format);
    vsnprintf(buffer, sizeof(buffer), format, va);
    va_end(va);
    do_ssh_log(&session->common, verbosity, buffer);
  }
}

/** @internal
 * @brief log a SSH event with a common pointer
 * @param common       The SSH/bind session.
 * @param verbosity     The verbosity of the event.
 * @param format        The format string of the log entry.
 */
void ssh_log_common(struct ssh_common_struct *common, int verbosity, const char *format, ...){
  char buffer[1024];
  va_list va;

  if (verbosity <= common->log_verbosity) {
    va_start(va, format);
    vsnprintf(buffer, sizeof(buffer), format, va);
    va_end(va);
    do_ssh_log(common, verbosity, buffer);
  }
}

/** @} */

/* vim: set ts=4 sw=4 et cindent: */

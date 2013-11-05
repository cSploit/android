/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		fbsyslog.h
 *	DESCRIPTION:	System log facility (posix)
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
 * Created by: Alex Peshkov <peshkoff@mail.ru>
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../jrd/os/fbsyslog.h"

#include <syslog.h>
#include <unistd.h>
#include <string.h>

namespace Firebird {
void Syslog::Record(Severity level, const char* msg)
{
	int priority = LOG_ERR;
	switch (level)
	{
	case Warning:
		priority = LOG_WARNING;
		break;
	case Error:
	default:
		priority = LOG_ERR;
		break;
	}
	syslog(priority | LOG_DAEMON | LOG_PID, "%s", msg);

	// try to put it also on controlling tty
	int fd = 2;
	if (! isatty(fd))
	{
		fd = 1;
	}
	if (isatty(fd))
	{
		write(fd, msg, strlen(msg));
		write(fd, "\n", 1);
	}
}
} // namespace Firebird


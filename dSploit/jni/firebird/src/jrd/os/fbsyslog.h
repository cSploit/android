/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		fbsyslog.h
 *	DESCRIPTION:	System log facility (platform specific)
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

#ifndef FIREBIRD_SYSLOG_H
#define FIREBIRD_SYSLOG_H

#include "fb_types.h"

namespace Firebird {
class Syslog
{
public:
	enum Severity {Warning, Error};
	static void Record(Severity, const char*);
};
} // namespace Firebird

#endif  // FIREBIRD_SYSLOG_H


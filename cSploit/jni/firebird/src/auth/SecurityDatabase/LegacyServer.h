/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		LegacyServer.h
 *	DESCRIPTION:	User information database name
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
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 * 2003.02.02 Dmitry Yemanov: Implemented cached security database connection
 */

#ifndef AUTH_LEGACY_SERVER_H
#define AUTH_LEGACY_SERVER_H

#include "../jrd/ibase.h"
#include "../common/utils_proto.h"
#include "../common/sha.h"
#include "gen/iberror.h"
#include "../common/classes/ClumpletWriter.h"
#include "../common/classes/ImplementHelper.h"

#include "firebird/Auth.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <time.h>

namespace Auth {

class SecurityDatabaseServer FB_FINAL : public Firebird::StdPlugin<IServer, FB_AUTH_SERVER_VERSION>
{
public:
	explicit SecurityDatabaseServer(Firebird::IPluginConfig* p)
		: iParameter(p)
	{ }

	// IServer implementation
	int FB_CARG authenticate(Firebird::IStatus* status, IServerBlock* sBlock, IWriter* writerInterface);
	int FB_CARG release();

private:
	Firebird::RefPtr<Firebird::IPluginConfig> iParameter;
};

void registerLegacyServer(Firebird::IPluginManager* iPlugin);

} // namespace Auth

#endif // AUTH_LEGACY_SERVER_H

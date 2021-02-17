/*
 *	PROGRAM:		Firebird interface.
 *	MODULE:			MasterImplementation.h
 *	DESCRIPTION:	Main firebird interface, used to get other interfaces.
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Alex Peshkov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2011 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef YVALVE_MASTER_IMPLEMENTATION_H
#define YVALVE_MASTER_IMPLEMENTATION_H

#include "firebird.h"
#include "firebird/Interface.h"
#include "../yvalve/YObjects.h"
#include "../common/classes/ImplementHelper.h"

namespace Firebird
{
	class Mutex;
}

namespace Why
{
	class Dtc : public Firebird::AutoIface<Firebird::IDtc, FB_DTC_VERSION>
	{
	public:
		// IDtc implementation
		virtual YTransaction* FB_CARG start(Firebird::IStatus* status,
			unsigned int cnt, Firebird::DtcStart* components);
		virtual YTransaction* FB_CARG join(Firebird::IStatus* status,
			Firebird::ITransaction* one, Firebird::ITransaction* two);
	};

	class MasterImplementation : public Firebird::AutoIface<Firebird::IMaster, FB_MASTER_VERSION>
	{
	public:
		static Firebird::Static<Dtc> dtc;

	public:
		// IMaster implementation
		Firebird::IStatus* FB_CARG getStatus();
		Firebird::IProvider* FB_CARG getDispatcher();
		Firebird::IPluginManager* FB_CARG getPluginManager();
		int FB_CARG upgradeInterface(Firebird::IVersioned* toUpgrade, int desiredVersion,
									 Firebird::UpgradeInfo* upgradeInfo);
		const char* FB_CARG circularAlloc(const char* s, size_t len, intptr_t thr);
		Firebird::ITimerControl* FB_CARG getTimerControl();
		Firebird::IAttachment* FB_CARG registerAttachment(Firebird::IProvider* provider,
			Firebird::IAttachment* attachment);
		Firebird::ITransaction* FB_CARG registerTransaction(Firebird::IAttachment* attachment,
			Firebird::ITransaction* transaction);
		Dtc* FB_CARG getDtc();
		int FB_CARG same(IVersioned* first, IVersioned* second);
		Firebird::IMetadataBuilder* FB_CARG getMetadataBuilder(Firebird::IStatus* status, unsigned fieldCount);
		Firebird::IDebug* FB_CARG getDebug();
		int FB_CARG serverMode(int mode);
		Firebird::IUtl* FB_CARG getUtlInterface();
		Firebird::IConfigManager* FB_CARG getConfigManager();
	};

	void shutdownTimers();
	void releaseUpgradeTabs(Firebird::IPluginModule* module);

	Firebird::Mutex& pauseTimer();
} // namespace Why

#endif // YVALVE_MASTER_IMPLEMENTATION_H

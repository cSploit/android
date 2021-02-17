/*
 *	PROGRAM:		Firebird interface.
 *	MODULE:			firebird/Interface.h
 *	DESCRIPTION:	Base class for all FB interfaces / plugins.
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
 *  Copyright (c) 2010 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef FB_INCLUDE_INTERFACE
#define FB_INCLUDE_INTERFACE

#include "ibase.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#	define FB_CARG __cdecl
#else
#	define FB_CARG
#endif

namespace Firebird {

// Forward declaration - used to identify client and provider of upgraded interface
class IPluginModule;

// Versioned interface - base for all FB interfaces
class IVersioned
{
public:
	virtual int FB_CARG getVersion() = 0;
	virtual IPluginModule* FB_CARG getModule() = 0;
};
// If this is changed, types of all interfaces must be changed
#define FB_VERSIONED_VERSION 2

// Reference counted interface - base for refCounted FB interfaces
class IRefCounted : public IVersioned
{
public:
	virtual void FB_CARG addRef() = 0;
	virtual int FB_CARG release() = 0;
};
// If this is changed, types of refCounted interfaces must be changed
#define FB_REFCOUNTED_VERSION (FB_VERSIONED_VERSION + 2)

// Disposable interface - base for disposable FB interfaces
class IDisposable : public IVersioned
{
public:
	virtual void FB_CARG dispose() = 0;
};
// If this is changed, types of disposable interfaces must be changed
#define FB_DISPOSABLE_VERSION (FB_VERSIONED_VERSION + 1)

// Interface to work with status vector
// Created by master interface by request
// Also may be implemented on stack by internal FB code
class IStatus : public IDisposable
{
public:
	virtual void FB_CARG set(unsigned int length, const ISC_STATUS* value) = 0;
	virtual void FB_CARG set(const ISC_STATUS* value) = 0;
	virtual void FB_CARG init() = 0;

	virtual const ISC_STATUS* FB_CARG get() const = 0;
	virtual int FB_CARG isSuccess() const = 0;
};
#define FB_STATUS_VERSION (FB_DISPOSABLE_VERSION + 5)

class IProvider;
class IUtl;
class IPluginManager;
class ITimerControl;
class IAttachment;
class ITransaction;
class IDtc;
class IMetadataBuilder;
class IDebug;
class IConfigManager;

struct UpgradeInfo
{
	void* missingFunctionClass;
	IPluginModule* clientModule;
};

// Master interface is used to access almost all other interfaces.
class IMaster : public IVersioned
{
public:
	virtual IStatus* FB_CARG getStatus() = 0;
	virtual IProvider* FB_CARG getDispatcher() = 0;
	virtual IPluginManager* FB_CARG getPluginManager() = 0;
	virtual int FB_CARG upgradeInterface(IVersioned* toUpgrade, int desiredVersion,
										 struct UpgradeInfo* upgradeInfo) = 0;
	virtual const char* FB_CARG circularAlloc(const char* s, size_t len, intptr_t thr) = 0;
	virtual ITimerControl* FB_CARG getTimerControl() = 0;
	virtual IDtc* FB_CARG getDtc() = 0;
	virtual IAttachment* FB_CARG registerAttachment(IProvider* provider, IAttachment* attachment) = 0;
	virtual ITransaction* FB_CARG registerTransaction(IAttachment* attachment, ITransaction* transaction) = 0;

	// This function is required to compare interfaces based on vtables of them
	virtual int FB_CARG same(IVersioned* first, IVersioned* second) = 0;

	virtual IMetadataBuilder* FB_CARG getMetadataBuilder(IStatus* status, unsigned fieldCount) = 0;
	virtual Firebird::IDebug* FB_CARG getDebug() = 0;
	virtual int FB_CARG serverMode(int mode) = 0;
	virtual IUtl* FB_CARG getUtlInterface() = 0;
	virtual IConfigManager* FB_CARG getConfigManager() = 0;
};
#define FB_MASTER_VERSION (FB_VERSIONED_VERSION + 15)

} // namespace Firebird

extern "C"
{
	// Additional API function.
	// Should be used only in non-plugin modules.
	// All plugins including providers should use passed at init time interface instead.
	Firebird::IMaster* ISC_EXPORT fb_get_master_interface();
}

#endif // FB_INCLUDE_INTERFACE

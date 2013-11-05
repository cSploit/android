/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		flu.cpp
 *	DESCRIPTION:	Function Lookup Code
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
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "EPSON" define
 *
 * 2002-02-23 Sean Leyne - Code Cleanup, removed old M88K and NCR3000 port
 *
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "UNIXWARE" port
 *
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "SGI" port
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "HP700" port
 *
 * 2002.11.28 Sean Leyne - Code cleanup, removed obsolete "HM300" port
 *
 * 2003.04.12 Alex Peshkoff - Security code cleanup:
 *		1.	Drop EXT_LIB_PATH verification
 *		2.	Drop IB_UDF_DIR & IB_INTL_DIR support
 *		3.	Created common for all platforms search_for_module,
 *			using dir_list and PathUtils. Platform specific
 *			macros defined after ISC-lookup-entrypoint()
 *			for each platform.
 *
 * 2004.11.27 Alex Peshkoff - removed results of Borland's copyNpaste
 *		programming style. Almost all completely rewritten.
 *
 */

#include "firebird.h"
#include "../common/config/config.h"
#include "../common/config/dir_list.h"
#include "../jrd/os/path_utils.h"
#include "../common/classes/init.h"
#include "../jrd/jrd.h"

#include "../jrd/common.h"
#include "../jrd/flu.h"
#include "../jrd/gdsassert.h"

#include "../jrd/flu_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/err_proto.h"

#include "gen/iberror.h"

#include <string.h>

#if (defined SOLARIS || defined LINUX || defined AIX_PPC || defined FREEBSD || defined NETBSD || defined HPUX)
#define DYNAMIC_SHARED_LIBRARIES
#endif

using namespace Firebird;

namespace {
	Firebird::InitInstance<Jrd::Module::LoadedModules> loadedModules;
	Firebird::GlobalPtr<Firebird::Mutex> modulesMutex;

	template <typename S>
	void terminate_at_space(S& s, const char* psz)
	{
		const char *p = psz;
		while (*p && *p != ' ')
		{
			++p;
		}
		s.assign(psz, p - psz);
	}

	// we have to keep prefix/suffix per-OS mechanism
	// here to help dir_list correctly locate module
	// in one of it's directories

	enum ModKind {MOD_PREFIX, MOD_SUFFIX};
	struct Libfix
	{
		ModKind kind;
		const char* txt;
		bool permanent;
	};

	const Libfix libfixes[] =
	{

#ifdef WIN_NT
		// to avoid implicit .dll suffix
		{MOD_SUFFIX, ".", false},
		{MOD_SUFFIX, ".DLL", false},
#endif

		// always try to use module "as is"
		{MOD_SUFFIX, "", false},

#ifdef HPUX
		{MOD_SUFFIX, ".sl", true},
#endif

#ifdef DYNAMIC_SHARED_LIBRARIES
		{MOD_SUFFIX, ".so", true},
		{MOD_PREFIX, "lib", true},
#endif

#ifdef DARWIN
		{MOD_SUFFIX, ".dylib", true},
#endif

	};

	// UDF/BLOB filter verifier
	class UdfDirectoryList : public Firebird::DirectoryList
	{
	private:
		const Firebird::PathName getConfigString() const
		{
			return Firebird::PathName(Config::getUdfAccess());
		}
	public:
		explicit UdfDirectoryList(MemoryPool& p)
			: DirectoryList(p)
		{
			initialize();
		}
	};
	Firebird::InitInstance<UdfDirectoryList> iUdfDirectoryList;
}

namespace Jrd
{
	bool Module::operator>(const Module &im) const
	{
		// we need it to sort on some key
		return interMod > im.interMod;
	}

	Module::InternalModule* Module::scanModule(const Firebird::PathName& name)
	{
		typedef Module::InternalModule** itr;
		for (itr it = loadedModules().begin(); it != loadedModules().end(); ++it)
		{
			if (**it == name)
			{
				return *it;
			}
		}
		return 0;
	}


	FPTR_INT Module::lookup(const char* module, const char* name, DatabaseModules& interest)
	{
		FPTR_INT function = FUNCTIONS_entrypoint(module, name);
		if (function)
		{
			return function;
		}

		// Try to find loadable module
		Module m = lookupModule(module, true);
		if (! m)
		{
			return 0;
		}

		Firebird::string symbol;
		terminate_at_space(symbol, name);
		void* rc = m.lookupSymbol(symbol);
		if (rc)
		{
			if (!interest.exist(m))
			{
				interest.add(m);
			}
		}

		return (FPTR_INT)rc;
	}

	FPTR_INT Module::lookup(const TEXT* module, const TEXT* name)
	{
		FPTR_INT function = FUNCTIONS_entrypoint(module, name);
		if (function)
		{
			return function;
		}

		// Try to find loadable module
		Module m = lookupModule(module, false);
		if (! m)
		{
			return 0;
		}

		Firebird::string symbol;
		terminate_at_space(symbol, name);
		return (FPTR_INT)(m.lookupSymbol(symbol));
	}

	// flag 'udf' means pass name-path through UdfDirectoryList
	Module Module::lookupModule(const char* name, bool udf)
	{
		Firebird::MutexLockGuard lg(modulesMutex);
		Firebird::PathName initialModule;
		terminate_at_space(initialModule, name);

		// Look for module in array of already loaded
		InternalModule* im = scanModule(initialModule);
		if (im)
		{
			return Module(im);
		}

		// apply suffix (and/or prefix) and try that name
		Firebird::PathName module(initialModule);
		for (size_t i = 0; i < sizeof(libfixes) / sizeof(Libfix); i++)
		{
			const Libfix* l = &libfixes[i];
			// os-dependent module name modification
			Firebird::PathName fixedModule(module);
			switch (l->kind)
			{
			case MOD_PREFIX:
				fixedModule = l->txt + fixedModule;
				break;
			case MOD_SUFFIX:
				fixedModule += l->txt;
			}
			if (l->permanent)
			{
				module = fixedModule;
			}

			// Look for module with fixed name
			im = scanModule(fixedModule);
			if (im)
			{
				return Module(im);
			}

			if (udf)
			{
				// UdfAccess verification
				Firebird::PathName path, relative;

				// Search for module name in UdfAccess restricted
				// paths list
				PathUtils::splitLastComponent(path, relative, fixedModule);
				if (path.length() == 0 && PathUtils::isRelative(fixedModule))
				{
					path = fixedModule;
					if (! iUdfDirectoryList().expandFileName(fixedModule, path))
					{
						// relative path was used, but no appropriate file present
						continue;
					}
				}

				// The module name, including directory path,
				// must satisfy UdfAccess entry in config file.
				if (! iUdfDirectoryList().isPathInList(fixedModule))
				{
					ERR_post(Arg::Gds(isc_conf_access_denied) << Arg::Str("UDF/BLOB-filter module") <<
																 Arg::Str(initialModule));
				}

				ModuleLoader::Module* mlm = ModuleLoader::loadModule(fixedModule);
				if (mlm)
				{
					im = FB_NEW(*getDefaultMemoryPool())
						InternalModule(*getDefaultMemoryPool(), mlm, initialModule, fixedModule);
					loadedModules().add(im);
					return Module(im);
				}
			}
			else
			{
				// try to load permanent module
				ModuleLoader::Module* mlm = ModuleLoader::loadModule(fixedModule);
				if (mlm)
				{
					im = FB_NEW(*getDefaultMemoryPool())
						InternalModule(*getDefaultMemoryPool(), mlm, initialModule, fixedModule);
					loadedModules().add(im);
					im->acquire();	// make permanent
					return Module(im);
				}
			}
		}

		// let others raise 'missing library' error if needed
		return Module();
	}

	Module::~Module()
	{
		if (interMod)
		{
			Firebird::MutexLockGuard lg(modulesMutex);
			if (interMod->release() == 0)
			{
				for (size_t m = 0; m < loadedModules().getCount(); m++)
				{
					if (loadedModules()[m] == interMod)
					{
						loadedModules().remove(m);
						delete interMod;
						return;
					}
				}
				fb_assert(false);
				// In production server we delete interMod here
				// (though this is not normal case)
				delete interMod;
			}
		}
	}

} // namespace Jrd

/*
 *  mod_loader.cpp
 *
 */

// required to use activation context API structures
#define _WIN32_WINNT 0x0501

#include "firebird.h"
#include "../../../common/dllinst.h"
#include "../common/os/mod_loader.h"
#include <windows.h>
#include "../common/os/path_utils.h"
#include "../common/config/config.h"
#include "../common/classes/ImplementHelper.h"

using namespace Firebird;

/// This is the Win32 implementation of the mod_loader abstraction.

/// activation context API prototypes
typedef HANDLE (WINAPI * PFN_CAC)(PCACTCTXA pActCtx);

typedef BOOL (WINAPI * PFN_FINDAC)(DWORD dwFlags,
								   const GUID *lpExtensionGuid,
								   ULONG ulSectionId,
								   LPCSTR lpStringToFind,
								   PACTCTX_SECTION_KEYED_DATA ReturnedData);

typedef void (WINAPI * PFN_RAC)(HANDLE hActCtx);

typedef BOOL (WINAPI * PFN_AAC)(HANDLE hActCtx, ULONG_PTR *lpCookie);

typedef BOOL (WINAPI * PFN_DAC)(DWORD dwFlags, ULONG_PTR ulCookie);
/// end of activation context API prototypes


template <typename PFN>
class WinApiFunction
{
public:
	WinApiFunction(const char *dllName, const char *fnName)
	{
		m_ptr = NULL;
		const HMODULE hDll = GetModuleHandle(dllName);
		if (hDll)
			m_ptr = (PFN) GetProcAddress(hDll, fnName);
	}

	~WinApiFunction()
	{}

	PFN operator* () const { return m_ptr; }

	operator bool() const { return (m_ptr != NULL); }

private:
	PFN m_ptr;
};

const char* const KERNEL32_DLL = "kernel32.dll";


class ContextActivator
{
public:
	ContextActivator() :
	  mFindActCtxSectionString(KERNEL32_DLL, "FindActCtxSectionStringA"),
	  mCreateActCtx(KERNEL32_DLL, "CreateActCtxA"),
	  mReleaseActCtx(KERNEL32_DLL, "ReleaseActCtx"),
	  mActivateActCtx(KERNEL32_DLL, "ActivateActCtx"),
	  mDeactivateActCtx(KERNEL32_DLL, "DeactivateActCtx")
	{
		hActCtx = INVALID_HANDLE_VALUE;

// if we don't use MSVC then we don't use MS CRT ?
// NS: versions of MSVC before 2005 and, as preliminary reports suggest,
// after 2008 do not need this hack
#if !defined(_MSC_VER) || (_MSC_VER < 1400)
		return;
#else

		if (!mCreateActCtx)
			return;

		ACTCTX_SECTION_KEYED_DATA ackd;
		memset(&ackd, 0, sizeof(ackd));
		ackd.cbSize = sizeof(ackd);

		// if CRT already present in some activation context then nothing to do
		if ((*mFindActCtxSectionString)
				(0, NULL,
				ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
#if _MSC_VER == 1400
                    "msvcr80.dll",
#elif _MSC_VER == 1500
                    "msvcr90.dll",
#elif _MSC_VER == 1600
                    "msvcr100.dll",
#else
                    #error Specify CRT DLL name here !
#endif
				&ackd))
		{
			return;
		}

		// create and use activation context from our own manifest
		ACTCTXA actCtx;
		memset(&actCtx, 0, sizeof(actCtx));
		actCtx.cbSize = sizeof(actCtx);
		actCtx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
		actCtx.lpResourceName = ISOLATIONAWARE_MANIFEST_RESOURCE_ID;
		actCtx.hModule = Firebird::hDllInst;

		if (actCtx.hModule)
		{
			char name[1024];
			GetModuleFileName(actCtx.hModule, name, sizeof(name));
			actCtx.lpSource = name;

			hActCtx = (*mCreateActCtx) (&actCtx);
			if (hActCtx != INVALID_HANDLE_VALUE)
				(*mActivateActCtx) (hActCtx, &mCookie);
		}
#endif // !_MSC_VER
	}

	~ContextActivator()
	{
		if (hActCtx != INVALID_HANDLE_VALUE)
		{
			(*mDeactivateActCtx)(0, mCookie);
			(*mReleaseActCtx)(hActCtx);
		}
	}

private:
	WinApiFunction<PFN_FINDAC> mFindActCtxSectionString;
	WinApiFunction<PFN_CAC> mCreateActCtx;
	WinApiFunction<PFN_RAC> mReleaseActCtx;
	WinApiFunction<PFN_AAC> mActivateActCtx;
	WinApiFunction<PFN_DAC> mDeactivateActCtx;

	HANDLE		hActCtx;
	ULONG_PTR	mCookie;
};


class Win32Module : public ModuleLoader::Module
{
public:
	Win32Module(MemoryPool& pool, const PathName& aFileName, HMODULE m)
		: Module(pool, aFileName),
		  module(m)
	{
	}

	~Win32Module();

	void *findSymbol(const string&);

private:
	const HMODULE module;
};

bool ModuleLoader::isLoadableModule(const PathName& module)
{
	ContextActivator ctx;

	const HMODULE hMod =
		LoadLibraryEx(module.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH | LOAD_LIBRARY_AS_DATAFILE);

	if (hMod) {
		FreeLibrary(hMod);
	}
	return hMod != 0;
}

void ModuleLoader::doctorModuleExtension(PathName& name)
{
	const PathName::size_type pos = name.rfind(".dll");
	if (pos != PathName::npos && pos == name.length() - 4)
		return;
	name += ".dll";
}

ModuleLoader::Module* ModuleLoader::loadModule(const PathName& modPath)
{
	ContextActivator ctx;

	// supress error message box if it is not done yet
	const UINT oldErrorMode =
		SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);

	HMODULE module = 0;
	if (PathUtils::isRelative(modPath))
	{
		MasterInterfacePtr master;
		const char* baseDir = master->getConfigManager()->getDirectory(DirType::FB_DIR_BIN);

		PathName fullName;
		PathUtils::concatPath(fullName, baseDir, modPath);

		module = LoadLibraryEx(fullName.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
	}

	if (!module)
		module = LoadLibraryEx(modPath.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);

	// Restore old mode in case we are embedded into user application
	SetErrorMode(oldErrorMode);

	if (!module)
		return 0;

	char fileName[MAX_PATH];
	GetModuleFileName(module, fileName, sizeof(fileName));

	return FB_NEW(*getDefaultMemoryPool()) Win32Module(*getDefaultMemoryPool(), fileName, module);
}

Win32Module::~Win32Module()
{
	if (module)
		FreeLibrary(module);
}

void* Win32Module::findSymbol(const string& symName)
{
	FARPROC result = GetProcAddress(module, symName.c_str());
	if (!result)
	{
		string newSym = '_' + symName;
		result = GetProcAddress(module, newSym.c_str());
	}
	return (void*) result;
}

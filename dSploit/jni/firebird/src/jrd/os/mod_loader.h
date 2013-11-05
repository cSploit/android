/*
 *	PROGRAM:		JRD Module Loader
 *	MODULE:			mod_loader.h
 *	DESCRIPTION:	Abstract class for loadable modules.
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
 *  The Original Code was created by John Bellardo
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2002 John Bellardo <bellardo at cs.ucsd.edu>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#ifndef JRD_OS_MOD_LOADER_H
#define JRD_OS_MOD_LOADER_H

#include "../common/classes/fb_string.h"

/***
	The ModuleLoader class is an abstraction of the dynamic code loading
	facilities provided by the host operating system.  The class provides
	functions to determine if the file at a given path is a loadable module,
	to load that module, and to modify the filename in a way that is
	appropiate to the host computer.

	All implementations of this interface are expected to provide definitions
	for the 3 static functions in the ModuleLoader class, and provide a
	subclass of ModuleLoader::Loader that implements findSymbol.
***/

class ModuleLoader
{
public:
	/** ModuleLoader::Module is the abstract base class for all disk
		based loadable modules after they are loaded into memory.
		It provides a method to locate a pointer for a given symbol,
		and automatically unloads the module from memory when the
		object is destructed.  Instances of this class are created
		using the ModuleLoader::loadModule function.
	**/
	class Module
	{
	public:
		/** findSymbol searchs through the module after it has been loaded into
			memory, and returns a pointer to that symbol's location in memory.
			If the symbol can't be found or doesn't exist the function returns
			NULL.
		**/
		virtual void* findSymbol(const Firebird::string&) = 0;

		template <typename T> T& findSymbol(const Firebird::string& symbol, T& ptr)
		{
			return (ptr = (T)(findSymbol(symbol)));
		}

		/// Destructor
		virtual ~Module() {}
	protected:
		/// The constructor is protected so normal code can't allocate instances
		/// of the class, but the class itself is still able to be subclassed.
		Module() {}
	private:
		/// Copy construction is not supported, hence the copy constructor is private
		Module(const Module&);		// no impl
		/// assignment of Modules isn't supported so the assignment operator is private
		const Module& operator=(const Module&);		// no impl
	};

	/** loadModule is given as a string the path to the module to load.  It
		attempts to load the module.  If successful it returns the ModuleLoader::Module
		object that represents the loaded module in memory and can be used to
		perform symbol lookups on the module.  If unsuccessful it returns NULL.
		It is the callers responsibility to delete the returned module object
		when it is no longer needed.
	**/
	static Module* loadModule(const Firebird::PathName&);

	/** doctorModuleExtention modifies the given path name to add the platform
		specific module extention.  This allows the user to provide the root name
		of the file, and the code to append the correct extention regardless of the
		host operating system.  This function is typically called after a failed attempt
		to load the module without the extention.
	**/
	static void doctorModuleExtention(Firebird::PathName&);

	/** isLoadableModule checks the given file to see if it is a loadable
		module.  This function is required because different operating
		systems require different checks.
	**/
	static bool isLoadableModule(const Firebird::PathName&);
};

#endif // JRD_OS_MOD_LOADER_H


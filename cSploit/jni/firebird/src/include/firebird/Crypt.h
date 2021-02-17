/*
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
 *  The Original Code was created by Alexander Peshkov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2012 Alex Peshkov <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef FIREBIRD_CRYPT_PLUGIN_H
#define FIREBIRD_CRYPT_PLUGIN_H

#include "./Plugin.h"

namespace Firebird {

// Part 1. Network crypt.

// Plugins of this type are used to crypt data, sent over the wire
// Plugin must support encrypt and decrypt operations.
// Interface of plugin is the same for both client and server,
// and it may have different or same implementations for client and server.
class IWireCryptPlugin : public IPluginBase
{
public:
	// getKnownTypes() function must return list of acceptable keys' types
	// special type 'builtin' means that crypt plugin knows itself where to get the key from
	virtual const char* FB_CARG getKnownTypes(IStatus* status) = 0;
	virtual void FB_CARG setKey(IStatus* status, FbCryptKey* key) = 0;
	virtual void FB_CARG encrypt(IStatus* status, unsigned int length, const void* from, void* to) = 0;
	virtual void FB_CARG decrypt(IStatus* status, unsigned int length, const void* from, void* to) = 0;
};

#define FB_WIRECRYPT_PLUGIN_VERSION (FB_PLUGIN_VERSION + 4)

// Part 2. Database crypt.

// This interface is used to transfer some data (related to crypt keys)
// between different components of firebird.
class ICryptKeyCallback : public IVersioned
{
public:
	virtual unsigned int FB_CARG callback(unsigned int dataLength, const void* data,
		unsigned int bufferLength, void* buffer) = 0;
};

#define FB_CRYPT_CALLBACK_VERSION (FB_VERSIONED_VERSION + 1)

// Key holder accepts key(s) from attachment at database attach time
// (or gets them it some other arbitrary way)
// and sends it to database crypt plugin on request.
class IKeyHolderPlugin : public IPluginBase
{
public:
	// keyCallback() is called when new attachment is probably ready to provide keys
	// to key holder plugin, ICryptKeyCallback interface is provided by attachment.
	virtual int FB_CARG keyCallback(IStatus* status, ICryptKeyCallback* callback) = 0;
	// Crypt plugin calls keyHandle() when it needs a key, stored in key holder.
	// Key is not returned directly - instead of it callback interface is returned.
	// Missing key with given name is not an error condition for keyHandle().
	// It should just return NULL in this case
	virtual ICryptKeyCallback* FB_CARG keyHandle(IStatus* status, const char* keyName) = 0;
};

#define FB_KEYHOLDER_PLUGIN_VERSION (FB_PLUGIN_VERSION + 2)

class IDbCryptPlugin : public IPluginBase
{
public:
	// When database crypt plugin is loaded, setKey() is called to provide information
	// about key holders, available for a given database.
	// It's supposed that crypt plugin will invoke keyHandle() function from them
	// to access callback interface for getting actual crypt key.
	// If crypt plugin fails to find appropriate key in sources, it should raise error.
	virtual void FB_CARG setKey(IStatus* status, unsigned int length, IKeyHolderPlugin** sources) = 0;
	virtual void FB_CARG encrypt(IStatus* status, unsigned int length, const void* from, void* to) = 0;
	virtual void FB_CARG decrypt(IStatus* status, unsigned int length, const void* from, void* to) = 0;
};

#define FB_DBCRYPT_PLUGIN_VERSION (FB_PLUGIN_VERSION + 3)

}	// namespace Firebird


#endif	// FIREBIRD_CRYPT_PLUGIN_H

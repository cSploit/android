/*
 *	PROGRAM:		Firebird basic API
 *	MODULE:			firebird/Provider.h
 *	DESCRIPTION:	Interfaces, used by yValve
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

#ifndef FB_PROVIDER_INTERFACE
#define FB_PROVIDER_INTERFACE

#include "./Plugin.h"

namespace Firebird {

// This interfaces are implemented by yvalve code and by each of providers.

class IAttachment;				// Forward
class ICryptKeyCallback;		// From Crypt.h

class IEventCallback : public IRefCounted
{
public:
	virtual void FB_CARG eventCallbackFunction(unsigned int length, const unsigned char* events) = 0;
};
#define FB_EVENT_CALLBACK_VERSION (FB_REFCOUNTED_VERSION + 1)

class IBlob : public IRefCounted
{
public:
	virtual void FB_CARG getInfo(IStatus* status,
						 unsigned int itemsLength, const unsigned char* items,
						 unsigned int bufferLength, unsigned char* buffer) = 0;
	virtual unsigned int FB_CARG getSegment(IStatus* status, unsigned int length,
											void* buffer) = 0;	// returns real length
	virtual void FB_CARG putSegment(IStatus* status, unsigned int length,
									const void* buffer) = 0;
	virtual void FB_CARG cancel(IStatus* status) = 0;
	virtual void FB_CARG close(IStatus* status) = 0;
	virtual int FB_CARG seek(IStatus* status, int mode, int offset) = 0;			// returns position
};
#define FB_BLOB_VERSION (FB_REFCOUNTED_VERSION + 6)

class ITransaction : public IRefCounted
{
public:
	virtual void FB_CARG getInfo(IStatus* status,
						 unsigned int itemsLength, const unsigned char* items,
						 unsigned int bufferLength, unsigned char* buffer) = 0;
	virtual void FB_CARG prepare(IStatus* status,
						 unsigned int msgLength = 0, const unsigned char* message = 0) = 0;
	virtual void FB_CARG commit(IStatus* status) = 0;
	virtual void FB_CARG commitRetaining(IStatus* status) = 0;
	virtual void FB_CARG rollback(IStatus* status) = 0;
	virtual void FB_CARG rollbackRetaining(IStatus* status) = 0;
	virtual void FB_CARG disconnect(IStatus* status) = 0;
	virtual ITransaction* FB_CARG join(IStatus* status, ITransaction* transaction) = 0;
	virtual ITransaction* FB_CARG validate(IStatus* status, IAttachment* attachment) = 0;
	virtual ITransaction* FB_CARG enterDtc(IStatus* status) = 0;
};
#define FB_TRANSACTION_VERSION (FB_REFCOUNTED_VERSION + 10)

class IMetadataBuilder;			// Forward

class IMessageMetadata : public IRefCounted
{
public:
	virtual unsigned FB_CARG getCount(IStatus* status) = 0;
	virtual const char* FB_CARG getField(IStatus* status, unsigned index) = 0;
	virtual const char* FB_CARG getRelation(IStatus* status, unsigned index) = 0;
	virtual const char* FB_CARG getOwner(IStatus* status, unsigned index) = 0;
	virtual const char* FB_CARG getAlias(IStatus* status, unsigned index) = 0;
	virtual unsigned FB_CARG getType(IStatus* status, unsigned index) = 0;
	virtual FB_BOOLEAN FB_CARG isNullable(IStatus* status, unsigned index) = 0;
	virtual int FB_CARG getSubType(IStatus* status, unsigned index) = 0;
	virtual unsigned FB_CARG getLength(IStatus* status, unsigned index) = 0;
	virtual int FB_CARG getScale(IStatus* status, unsigned index) = 0;
	virtual unsigned FB_CARG getCharSet(IStatus* status, unsigned index) = 0;
	virtual unsigned FB_CARG getOffset(IStatus* status, unsigned index) = 0;
	virtual unsigned FB_CARG getNullOffset(IStatus* status, unsigned index) = 0;

	virtual IMetadataBuilder* FB_CARG getBuilder(IStatus* status) = 0;
	virtual unsigned FB_CARG getMessageLength(IStatus* status) = 0;
};
#define FB_MESSAGE_METADATA_VERSION (FB_REFCOUNTED_VERSION + 15)

class IMetadataBuilder : public IRefCounted
{
public:
	virtual void FB_CARG setType(IStatus* status, unsigned index, unsigned type) = 0;
	virtual void FB_CARG setSubType(IStatus* status, unsigned index, int subType) = 0;
	virtual void FB_CARG setLength(IStatus* status, unsigned index, unsigned length) = 0;
	virtual void FB_CARG setCharSet(IStatus* status, unsigned index, unsigned charSet) = 0;
	virtual void FB_CARG setScale(IStatus* status, unsigned index, unsigned scale) = 0;

	virtual void FB_CARG truncate(IStatus* status, unsigned count) = 0;
	virtual void FB_CARG moveNameToIndex(IStatus* status, const char* name, unsigned index) = 0;
	virtual void FB_CARG remove(IStatus* status, unsigned index) = 0;
	virtual unsigned FB_CARG addField(IStatus* status) = 0;

	virtual IMessageMetadata* FB_CARG getMetadata(IStatus* status) = 0;
};
#define FB_METADATA_BUILDER_VERSION (FB_REFCOUNTED_VERSION + 10)

// This item is for ISC API emulation only
// It may be gone in future versions
// Please do not use it!
static IMessageMetadata* const DELAYED_OUT_FORMAT = (IMessageMetadata*)(1);

class IResultSet : public IRefCounted
{
public:
	virtual FB_BOOLEAN FB_CARG fetchNext(IStatus* status, void* message) = 0;
	virtual FB_BOOLEAN FB_CARG fetchPrior(IStatus* status, void* message) = 0;
	virtual FB_BOOLEAN FB_CARG fetchFirst(IStatus* status, void* message) = 0;
	virtual FB_BOOLEAN FB_CARG fetchLast(IStatus* status, void* message) = 0;
	virtual FB_BOOLEAN FB_CARG fetchAbsolute(IStatus* status, unsigned int position, void* message) = 0;
	virtual FB_BOOLEAN FB_CARG fetchRelative(IStatus* status, int offset, void* message) = 0;
	virtual FB_BOOLEAN FB_CARG isEof(IStatus* status) = 0;
	virtual FB_BOOLEAN FB_CARG isBof(IStatus* status) = 0;
	virtual IMessageMetadata* FB_CARG getMetadata(IStatus* status) = 0;
	virtual void FB_CARG setCursorName(IStatus* status, const char* name) = 0;
	virtual void FB_CARG close(IStatus* status) = 0;

	// This item is for ISC API emulation only
	// It may be gone in future versions
	// Please do not use it!
	virtual void FB_CARG setDelayedOutputFormat(IStatus* status, IMessageMetadata* format) = 0;
};
#define FB_RESULTSET_VERSION (FB_REFCOUNTED_VERSION + 12)

class IStatement : public IRefCounted
{
public:
	// Prepare flags.
	static const unsigned PREPARE_PREFETCH_NONE 				= 0x00;
	static const unsigned PREPARE_PREFETCH_TYPE 				= 0x01;
	static const unsigned PREPARE_PREFETCH_INPUT_PARAMETERS 	= 0x02;
	static const unsigned PREPARE_PREFETCH_OUTPUT_PARAMETERS	= 0x04;
	static const unsigned PREPARE_PREFETCH_LEGACY_PLAN			= 0x08;
	static const unsigned PREPARE_PREFETCH_DETAILED_PLAN		= 0x10;
	static const unsigned PREPARE_PREFETCH_AFFECTED_RECORDS		= 0x20;	// not used yet
	static const unsigned PREPARE_PREFETCH_FLAGS				= 0x40;
	static const unsigned PREPARE_PREFETCH_METADATA =
		PREPARE_PREFETCH_TYPE | PREPARE_PREFETCH_FLAGS |
		PREPARE_PREFETCH_INPUT_PARAMETERS | PREPARE_PREFETCH_OUTPUT_PARAMETERS;
	static const unsigned PREPARE_PREFETCH_ALL =
		PREPARE_PREFETCH_METADATA | PREPARE_PREFETCH_LEGACY_PLAN | PREPARE_PREFETCH_DETAILED_PLAN |
		PREPARE_PREFETCH_AFFECTED_RECORDS;

	// Statement flags.
	static const unsigned FLAG_HAS_CURSOR	 					= 0x01;
	static const unsigned FLAG_REPEAT_EXECUTE				 	= 0x02;

	virtual void FB_CARG getInfo(IStatus* status,
								 unsigned int itemsLength, const unsigned char* items,
								 unsigned int bufferLength, unsigned char* buffer) = 0;
	virtual unsigned FB_CARG getType(IStatus* status) = 0;
	virtual const char* FB_CARG getPlan(IStatus* status, FB_BOOLEAN detailed) = 0;
	virtual ISC_UINT64 FB_CARG getAffectedRecords(IStatus* status) = 0;
	virtual IMessageMetadata* FB_CARG getInputMetadata(IStatus* status) = 0;
	virtual IMessageMetadata* FB_CARG getOutputMetadata(IStatus* status) = 0;
	virtual ITransaction* FB_CARG execute(IStatus* status, ITransaction* transaction,
		IMessageMetadata* inMetadata, void* inBuffer, IMessageMetadata* outMetadata, void* outBuffer) = 0;
	virtual IResultSet* FB_CARG openCursor(IStatus* status, ITransaction* transaction,
		IMessageMetadata* inMetadata, void* inBuffer, IMessageMetadata* outMetadata) = 0;
	virtual void FB_CARG free(IStatus* status) = 0;
	virtual unsigned FB_CARG getFlags(IStatus* status) = 0;
};
#define FB_STATEMENT_VERSION (FB_REFCOUNTED_VERSION + 10)

class IRequest : public IRefCounted
{
public:
	virtual void FB_CARG receive(IStatus* status, int level, unsigned int msgType,
						 unsigned int length, unsigned char* message) = 0;
	virtual void FB_CARG send(IStatus* status, int level, unsigned int msgType,
					  unsigned int length, const unsigned char* message) = 0;
	virtual void FB_CARG getInfo(IStatus* status, int level,
						 unsigned int itemsLength, const unsigned char* items,
						 unsigned int bufferLength, unsigned char* buffer) = 0;
	virtual void FB_CARG start(IStatus* status, ITransaction* tra, int level) = 0;
	virtual void FB_CARG startAndSend(IStatus* status, ITransaction* tra, int level, unsigned int msgType,
							  unsigned int length, const unsigned char* message) = 0;
	virtual void FB_CARG unwind(IStatus* status, int level) = 0;
	virtual void FB_CARG free(IStatus* status) = 0;
};
#define FB_REQUEST_VERSION (FB_REFCOUNTED_VERSION + 7)

class IEvents : public IRefCounted
{
public:
	virtual void FB_CARG cancel(IStatus* status) = 0;
};
#define FB_EVENTS_VERSION (FB_REFCOUNTED_VERSION + 1)

class IAttachment : public IRefCounted
{
public:
	virtual void FB_CARG getInfo(IStatus* status,
						 unsigned int itemsLength, const unsigned char* items,
						 unsigned int bufferLength, unsigned char* buffer) = 0;
	virtual ITransaction* FB_CARG startTransaction(IStatus* status,
		unsigned int tpbLength, const unsigned char* tpb) = 0;
	virtual ITransaction* FB_CARG reconnectTransaction(IStatus* status,
		unsigned int length, const unsigned char* id) = 0;
	virtual IRequest* FB_CARG compileRequest(IStatus* status,
		unsigned int blrLength, const unsigned char* blr) = 0;
	virtual void FB_CARG transactRequest(IStatus* status, ITransaction* transaction,
								 unsigned int blrLength, const unsigned char* blr,
								 unsigned int inMsgLength, const unsigned char* inMsg,
								 unsigned int outMsgLength, unsigned char* outMsg) = 0;
	virtual IBlob* FB_CARG createBlob(IStatus* status, ITransaction* transaction, ISC_QUAD* id,
							 unsigned int bpbLength = 0, const unsigned char* bpb = 0) = 0;
	virtual IBlob* FB_CARG openBlob(IStatus* status, ITransaction* transaction, ISC_QUAD* id,
						   unsigned int bpbLength = 0, const unsigned char* bpb = 0) = 0;
	virtual int FB_CARG getSlice(IStatus* status, ITransaction* transaction, ISC_QUAD* id,
						 unsigned int sdlLength, const unsigned char* sdl,
						 unsigned int paramLength, const unsigned char* param,
						 int sliceLength, unsigned char* slice) = 0;
	virtual void FB_CARG putSlice(IStatus* status, ITransaction* transaction, ISC_QUAD* id,
						  unsigned int sdlLength, const unsigned char* sdl,
						  unsigned int paramLength, const unsigned char* param,
						  int sliceLength, unsigned char* slice) = 0;
	virtual void FB_CARG executeDyn(IStatus* status, ITransaction* transaction, unsigned int length,
		const unsigned char* dyn) = 0;
	virtual IStatement* FB_CARG prepare(IStatus* status, ITransaction* tra,
		unsigned int stmtLength, const char* sqlStmt, unsigned dialect, unsigned int flags) = 0;
	virtual ITransaction* FB_CARG execute(IStatus* status, ITransaction* transaction,
		unsigned int stmtLength, const char* sqlStmt, unsigned dialect,
		IMessageMetadata* inMetadata, void* inBuffer, IMessageMetadata* outMetadata, void* outBuffer) = 0;
	virtual IResultSet* FB_CARG openCursor(IStatus* status, ITransaction* transaction,
		unsigned int stmtLength, const char* sqlStmt, unsigned dialect,
		IMessageMetadata* inMetadata, void* inBuffer, IMessageMetadata* outMetadata) = 0;
	virtual IEvents* FB_CARG queEvents(IStatus* status, IEventCallback* callback,
						   unsigned int length, const unsigned char* events) = 0;
	virtual void FB_CARG cancelOperation(IStatus* status, int option) = 0;
	virtual void FB_CARG ping(IStatus* status) = 0;
	virtual void FB_CARG detach(IStatus* status) = 0;
	virtual void FB_CARG dropDatabase(IStatus* status) = 0;
};
#define FB_ATTACHMENT_VERSION (FB_REFCOUNTED_VERSION + 18)

class IService : public IRefCounted
{
public:
	virtual void FB_CARG detach(IStatus* status) = 0;
	virtual void FB_CARG query(IStatus* status,
					   unsigned int sendLength, const unsigned char* sendItems,
					   unsigned int receiveLength, const unsigned char* receiveItems,
					   unsigned int bufferLength, unsigned char* buffer) = 0;
	virtual void FB_CARG start(IStatus* status,
					   unsigned int spbLength, const unsigned char* spb) = 0;
};
#define FB_SERVICE_VERSION (FB_REFCOUNTED_VERSION + 3)

class IProvider : public IPluginBase
{
public:
	virtual IAttachment* FB_CARG attachDatabase(IStatus* status, const char* fileName,
		unsigned int dpbLength, const unsigned char* dpb) = 0;
	virtual IAttachment* FB_CARG createDatabase(IStatus* status, const char* fileName,
		unsigned int dpbLength, const unsigned char* dpb) = 0;
	virtual IService* FB_CARG attachServiceManager(IStatus* status, const char* service,
		 unsigned int spbLength, const unsigned char* spb) = 0;
	virtual void FB_CARG shutdown(IStatus* status, unsigned int timeout, const int reason) = 0;
	virtual void FB_CARG setDbCryptCallback(IStatus* status, ICryptKeyCallback* cryptCallback) = 0;
};
#define FB_PROVIDER_VERSION (FB_PLUGIN_VERSION + 5)

// DtcStart - structure to start transaction over >1 attachments (former TEB)
struct DtcStart
{
	IAttachment* attachment;
	const unsigned char* tpb;
	unsigned int tpbLength;
};

// Distributed transactions coordinator
class IDtc : public IVersioned
{
public:
	virtual ITransaction* FB_CARG start(IStatus* status, unsigned int cnt, DtcStart* components) = 0;
	virtual ITransaction* FB_CARG join(IStatus* status, ITransaction* one, ITransaction* two) = 0;
};
#define FB_DTC_VERSION (FB_VERSIONED_VERSION + 2)

} // namespace Firebird


#endif // FB_PROVIDER_INTERFACE

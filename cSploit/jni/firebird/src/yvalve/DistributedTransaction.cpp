/*
 *	PROGRAM:		Firebird interface.
 *	MODULE:			DistributedTransaction.cpp
 *	DESCRIPTION:	DTC and distributed transaction (also known as 2PC).
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

#include "firebird.h"

#include "../yvalve/MasterImplementation.h"
#include "../yvalve/YObjects.h"
#include "../common/classes/ImplementHelper.h"
#include "../common/classes/rwlock.h"
#include "../common/classes/array.h"
#include "../common/StatusHolder.h"
#include "../jrd/inf_pub.h"
#include "../common/isc_proto.h"
#include "../jrd/acl.h"

using namespace Firebird;
using namespace Why;

namespace {

class DTransaction FB_FINAL : public RefCntIface<ITransaction, FB_TRANSACTION_VERSION>
{
public:
	DTransaction()
		: sub(getPool()), limbo(false)
	{ }

	// ITransaction implementation
	virtual int FB_CARG release()
	{
		if (--refCounter == 0)
		{
			delete this;
			return 0;
		}

		return 1;
	}

	virtual void FB_CARG getInfo(IStatus* status, unsigned int itemsLength,
		const unsigned char* items, unsigned int bufferLength, unsigned char* buffer);
	virtual void FB_CARG prepare(IStatus* status, unsigned int msgLength,
		const unsigned char* message);
	virtual void FB_CARG commit(IStatus* status);
	virtual void FB_CARG commitRetaining(IStatus* status);
	virtual void FB_CARG rollback(IStatus* status);
	virtual void FB_CARG rollbackRetaining(IStatus* status);
	virtual void FB_CARG disconnect(IStatus* status);
	virtual DTransaction* FB_CARG join(IStatus* status, ITransaction* transaction);
	virtual ITransaction* FB_CARG validate(IStatus* status, IAttachment* attachment);
	virtual DTransaction* FB_CARG enterDtc(IStatus* status);

private:
	typedef HalfStaticArray<ITransaction*, 8> SubArray;
	typedef HalfStaticArray<UCHAR, 1024> TdrBuffer;
	SubArray sub;
	RWLock rwLock;
	bool limbo;

	explicit DTransaction(const SubArray& aSub)
		: sub(getPool()), limbo(false)
	{
		sub.assign(aSub);
	}

	bool prepareCommit(IStatus* status, TdrBuffer& tdr);
	bool buildPrepareInfo(IStatus* status, TdrBuffer& tdr, ITransaction* from);

	~DTransaction();
};

bool DTransaction::buildPrepareInfo(IStatus* status, TdrBuffer& tdr, ITransaction* from)
{
	// Information items for two phase commit.
	static const UCHAR PREPARE_TR_INFO[] =
	{
		fb_info_tra_dbpath,
		isc_info_tra_id
	};

	Array<UCHAR> bigBuffer;
	// we need something really big here
	// output of chaining distributed transaction can be huge
	// limit MAX_USHORT is chosen cause for old API it was limit of all blocks
	UCHAR* buf = bigBuffer.getBuffer(MAX_USHORT);
	from->getInfo(status, sizeof(PREPARE_TR_INFO), PREPARE_TR_INFO, bigBuffer.getCount(), buf);
	if (!status->isSuccess())
		return false;

	UCHAR* const end = bigBuffer.end();

	while (buf < end)
	{
		UCHAR item = buf[0];
		++buf;
		const USHORT length = (USHORT) gds__vax_integer(buf, 2);
		// Prevent information out of sync.
		UCHAR lengthByte = length > MAX_UCHAR ? MAX_UCHAR : length;
		buf += 2;

		switch(item)
		{
			case isc_info_tra_id:
				tdr.add(TDR_TRANSACTION_ID);
				tdr.add(lengthByte);
				tdr.add(buf, lengthByte);
				break;

			case fb_info_tra_dbpath:
				tdr.add(TDR_DATABASE_PATH);
				tdr.add(lengthByte);
				tdr.add(buf, lengthByte);
				break;

			case isc_info_end:
				return true;
		}

		buf += length;
	}

	return true;
}

bool DTransaction::prepareCommit(IStatus* status, TdrBuffer& tdr)
{
	TEXT host[64];
	ISC_get_host(host, sizeof(host));
	const size_t hostlen = strlen(host);

	// Build a transaction description record containing the host site and database/transaction
	// information for the target databases.
	tdr.clear();
	tdr.add(TDR_VERSION);
	tdr.add(TDR_HOST_SITE);
	tdr.add(static_cast<UCHAR>(hostlen));
	tdr.add(reinterpret_cast<UCHAR*>(host), hostlen);

	// Get database and transaction stuff for each sub-transaction.

	for (unsigned i = 0; i < sub.getCount(); ++i)
	{
		if (sub[i])
		{
			if (! buildPrepareInfo(status, tdr, sub[i]))
				return false;
		}
	}

	return true;
}

void FB_CARG DTransaction::getInfo(IStatus* status,
								   unsigned int itemsLength, const unsigned char* items,
								   unsigned int bufferLength, unsigned char* buffer)
{
	try
	{
		status->init();

		ReadLockGuard guard(rwLock, FB_FUNCTION);

		for (unsigned int i = 0; i < sub.getCount(); ++i)
		{
			if (sub[i])
			{
				sub[i]->getInfo(status, itemsLength, items, bufferLength, buffer);
				if (!status->isSuccess())
				{
					return;
				}

				unsigned char* const end = buffer + bufferLength;
				while (buffer < end && (buffer[0] == isc_info_tra_id || buffer[0] == fb_info_tra_dbpath))
				{
					buffer += 3 + gds__vax_integer(&buffer[1], 2);
				}

				if (buffer >= end || buffer[0] != isc_info_end)
				{
					return;
				}

				bufferLength = end - buffer;
			}
		}
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
	}
}

void FB_CARG DTransaction::prepare(IStatus* status,
								   unsigned int msgLength, const unsigned char* message)
{
	try
	{
		status->init();

		WriteLockGuard guard(rwLock, FB_FUNCTION);

		if (limbo)
			return;

		TdrBuffer tdr;
		if (!msgLength)
		{
			if (!prepareCommit(status, tdr))
				return;

			msgLength = tdr.getCount();
			message = tdr.begin();
		}

		for (unsigned int i = 0; i < sub.getCount(); ++i)
		{
			if (sub[i])
			{
				sub[i]->prepare(status, msgLength, message);

				if (!status->isSuccess())
					return;
			}
		}

		limbo = true;
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
	}
}

void FB_CARG DTransaction::commit(IStatus* status)
{
	try
	{
		status->init();

		prepare(status, 0, NULL);
		if (!status->isSuccess())
		{
			return;
		}

		{	// guard scope
			WriteLockGuard guard(rwLock, FB_FUNCTION);

			for (unsigned int i = 0; i < sub.getCount(); ++i)
			{
				if (sub[i])
				{
					sub[i]->commit(status);
					if (!status->isSuccess())
						return;

					sub[i] = NULL;
				}
			}
		}

		release();
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
	}
}

/*
 *	I see problems with this approach, but keep it 'as was' for a while
 */
void FB_CARG DTransaction::commitRetaining(IStatus* status)
{
	try
	{
		status->init();

		WriteLockGuard guard(rwLock, FB_FUNCTION);

		for (unsigned int i = 0; i < sub.getCount(); ++i)
		{
			if (sub[i])
			{
				sub[i]->commitRetaining(status);
				if (!status->isSuccess())
					return;
			}
		}

		limbo = true;	// ASF: why do retaining marks limbo?
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
	}
}

void FB_CARG DTransaction::rollback(IStatus* status)
{
	try
	{
		status->init();

		{	// guard scope
			WriteLockGuard guard(rwLock, FB_FUNCTION);

			for (unsigned int i = 0; i < sub.getCount(); ++i)
			{
				if (sub[i])
				{
					sub[i]->rollback(status);
					if (!status->isSuccess())
						return;

					sub[i] = NULL;
				}
			}
		}

		release();
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
	}
}

void FB_CARG DTransaction::rollbackRetaining(IStatus* status)
{
	try
	{
		status->init();

		WriteLockGuard guard(rwLock, FB_FUNCTION);

		for (unsigned int i = 0; i < sub.getCount(); ++i)
		{
			if (sub[i])
			{
				sub[i]->rollbackRetaining(status);
				if (!status->isSuccess())
					return;
			}
		}

		limbo = true;	// ASF: why do retaining marks limbo?
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
	}
}

void FB_CARG DTransaction::disconnect(IStatus* status)
{
	try
	{
		status->init();

		WriteLockGuard guard(rwLock, FB_FUNCTION);

		if (!limbo)
			status_exception::raise(Arg::Gds(isc_no_recon));

		for (unsigned int i = 0; i < sub.getCount(); ++i)
		{
			if (sub[i])
			{
				sub[i]->disconnect(status);
				if (!status->isSuccess())
					return;

				sub[i] = NULL;
			}
		}

		release();
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
	}
}

// To do: check the maximum allowed dbs in a two phase commit.
//		  Q: what is the maximum?
DTransaction* FB_CARG DTransaction::join(IStatus* status, ITransaction* transaction)
{
	try
	{
		status->init();

		WriteLockGuard guard(rwLock, FB_FUNCTION);

		// reserve array element to make sure we have a place for copy of transaction
		size_t pos = sub.add(NULL);

		// Nothing throws exceptions after reserving space in sub-array up to the end of block
		ITransaction* traCopy = transaction->enterDtc(status);
		if (traCopy)
		{
			sub[pos] = traCopy;
			return this;
		}

		// enterDtc() failed - remove reserved array element
		sub.remove(pos);
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
	}

	return NULL;
}

ITransaction* FB_CARG DTransaction::validate(IStatus* status, IAttachment* attachment)
{
	try
	{
		status->init();

		ReadLockGuard guard(rwLock, FB_FUNCTION);

		for (unsigned int i = 0; i < sub.getCount(); ++i)
		{
			ITransaction* rc = sub[i]->validate(status, attachment);

			if (rc)
				return rc;
		}

		Arg::Gds(isc_bad_trans_handle).raise();
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
	}

	return NULL;
}

DTransaction* FB_CARG DTransaction::enterDtc(IStatus* status)
{
	try
	{
		status->init();

		WriteLockGuard guard(rwLock, FB_FUNCTION);

		RefPtr<DTransaction> traCopy(new DTransaction(sub));
		sub.clear();
		release();

		traCopy->addRef();
		return traCopy;
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
	}

	return NULL;
}

DTransaction::~DTransaction()
{
	for (unsigned int i = 0; i < sub.getCount(); ++i)
	{
		if (sub[i])
			sub[i]->release();
	}
}

} // anonymous namespace


namespace Why {

YTransaction* FB_CARG Dtc::start(IStatus* status, unsigned int cnt, DtcStart* components)
{
	try
	{
		status->init();

		RefPtr<DTransaction> dtransaction(new DTransaction);

		for (unsigned int i = 0; i < cnt; ++i)
		{
			RefPtr<ITransaction> transaction(components[i].attachment->
				startTransaction(status, components[i].tpbLength, components[i].tpb));

			if (! status->isSuccess())
				return NULL;

			dtransaction->join(status, transaction);

			if (! status->isSuccess())
			{
				LocalStatus tmp;
				dtransaction->rollback(&tmp);
				return NULL;
			}
		}

		dtransaction->addRef();
		return new YTransaction(NULL, dtransaction);
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
	}

	return NULL;
}

YTransaction* FB_CARG Dtc::join(IStatus* status, ITransaction* one, ITransaction* two)
{
	try
	{
		status->init();

		RefPtr<DTransaction> dtransaction(new DTransaction);

		dtransaction->join(status, one);
		if (! status->isSuccess())
			return NULL;

		dtransaction->join(status, two);
		/* We must not return NULL - first transaction is available only inside dtransaction
		if (! status->isSuccess())
			return NULL;
		*/

		dtransaction->addRef();
		return new YTransaction(NULL, dtransaction);
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
	}

	return NULL;
}

} // namespace Why

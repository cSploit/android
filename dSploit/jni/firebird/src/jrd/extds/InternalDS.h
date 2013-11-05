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
 *  The Original Code was created by Vlad Khorsun
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2008 Vlad Khorsun <hvlad@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef EXTDS_INTERNAL_H
#define EXTDS_INTERNAL_H


#include "ExtDS.h"

namespace EDS {

class InternalProvider : public Provider
{
public:
	explicit InternalProvider(const char* prvName) :
		Provider(prvName)
	{
		m_flags = (prvMultyStmts | prvMultyTrans);
	}

	~InternalProvider()
	{}

	virtual void initialize() {}
	virtual void jrdAttachmentEnd(Jrd::thread_db* tdbb, Jrd::Attachment* att);
	virtual void getRemoteError(ISC_STATUS* status, Firebird::string& err) const;

protected:
	virtual Connection* doCreateConnection();
};


class InternalConnection : public Connection
{
protected:
	friend class InternalProvider;

	explicit InternalConnection(InternalProvider& prov) :
	  Connection(prov),
	  m_attachment(0),
	  m_isCurrent(false)
	{}

	virtual ~InternalConnection();

public:
	virtual void attach(Jrd::thread_db* tdbb, const Firebird::string& dbName,
		const Firebird::string& user, const Firebird::string& pwd,
		const Firebird::string& role);

	virtual bool cancelExecution(Jrd::thread_db* tdbb);

	virtual bool isAvailable(Jrd::thread_db* tdbb, TraScope traScope) const;

	virtual bool isConnected() const { return (m_attachment != 0); }

	virtual bool isSameDatabase(Jrd::thread_db* tdbb, const Firebird::string& dbName,
		const Firebird::string& user, const Firebird::string& pwd,
		const Firebird::string& role) const;

	bool isCurrent() const { return m_isCurrent; }

	Jrd::Attachment* getJrdAtt() { return m_attachment; }

	virtual Blob* createBlob();

protected:
	virtual Transaction* doCreateTransaction();
	virtual Statement* doCreateStatement();
	virtual void doDetach(Jrd::thread_db* tdbb);

	Jrd::Attachment* m_attachment;
	bool m_isCurrent;
};


class InternalTransaction : public Transaction
{
protected:
	friend class InternalConnection;

	explicit InternalTransaction(InternalConnection& conn) :
	  Transaction(conn),
	  m_IntConnection(conn),
	  m_transaction(0)
	{}

	virtual ~InternalTransaction() {}

public:
	Jrd::jrd_tra* getJrdTran() { return m_transaction; }

protected:
	virtual void doStart(ISC_STATUS* status, Jrd::thread_db* tdbb, Firebird::ClumpletWriter& tpb);
	virtual void doPrepare(ISC_STATUS* status, Jrd::thread_db* tdbb, int info_len, const char* info);
	virtual void doCommit(ISC_STATUS* status, Jrd::thread_db* tdbb, bool retain);
	virtual void doRollback(ISC_STATUS* status, Jrd::thread_db* tdbb, bool retain);

	InternalConnection& m_IntConnection;
	Jrd::jrd_tra* m_transaction;
};


class InternalStatement : public Statement
{
protected:
	friend class InternalConnection;

	explicit InternalStatement(InternalConnection& conn);
	~InternalStatement();

protected:
	virtual void doPrepare(Jrd::thread_db* tdbb, const Firebird::string& sql);
	virtual void doExecute(Jrd::thread_db* tdbb);
	virtual void doOpen(Jrd::thread_db* tdbb);
	virtual bool doFetch(Jrd::thread_db* tdbb);
	virtual void doClose(Jrd::thread_db* tdbb, bool drop);

	virtual void putExtBlob(Jrd::thread_db* tdbb, dsc& src, dsc& dst);
	virtual void getExtBlob(Jrd::thread_db* tdbb, const dsc& src, dsc& dst);

	InternalTransaction* getIntTransaction()
	{
		return (InternalTransaction*) m_transaction;
	}

	InternalConnection& m_intConnection;
	InternalTransaction* m_intTransaction;

	Jrd::dsql_req* m_request;
	Firebird::UCharBuffer m_inBlr;
	Firebird::UCharBuffer m_outBlr;
};


class InternalBlob : public Blob
{
	friend class InternalConnection;
protected:
	explicit InternalBlob(InternalConnection& conn);

public:
	~InternalBlob();

public:
	virtual void open(Jrd::thread_db* tdbb, Transaction& tran, const dsc& desc, const Firebird::UCharBuffer* bpb);
	virtual void create(Jrd::thread_db* tdbb, Transaction& tran, dsc& desc, const Firebird::UCharBuffer* bpb);
	virtual USHORT read(Jrd::thread_db* tdbb, UCHAR* buff, USHORT len);
	virtual void write(Jrd::thread_db* tdbb, const UCHAR* buff, USHORT len);
	virtual void close(Jrd::thread_db* tdbb);
	virtual void cancel(Jrd::thread_db* tdbb);

private:
	InternalConnection& m_connection;
	Jrd::blb* m_blob;
	Jrd::bid m_blob_id;
};

} // namespace EDS

#endif // EXTDS_INTERNAL_H

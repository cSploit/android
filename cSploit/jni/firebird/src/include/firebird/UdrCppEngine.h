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
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2008 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef FIREBIRD_UDR_CPP_ENGINE
#define FIREBIRD_UDR_CPP_ENGINE

#include "./ExternalEngine.h"
#include "./UdrEngine.h"
#include "./Message.h"
#ifndef JRD_IBASE_H
#include "ibase.h"
#include "iberror.h"
#endif
#include <string.h>


namespace Firebird
{
	namespace Udr
	{
//------------------------------------------------------------------------------


#define FB_UDR_BEGIN_FUNCTION(name)	\
	namespace Func##name	\
	{	\
		class Impl;	\
		\
		static ::Firebird::Udr::FunctionFactoryImpl<Impl> factory(#name);	\
		\
		class Impl : public ::Firebird::Udr::Function<Impl>	\
		{	\
		public:	\
			FB__UDR_COMMON_IMPL

#define FB_UDR_END_FUNCTION	\
		};	\
	}

#define FB_UDR_EXECUTE_FUNCTION	\
	virtual void FB_CARG execute(::Firebird::IStatus* status, ::Firebird::ExternalContext* context, \
		void* in, void* out)	\
	{	\
		try	\
		{	\
			internalExecute(status, context, (InMessage::Type*) in, (OutMessage::Type*) out);	\
		}	\
		FB__UDR_CATCH	\
	}	\
	\
	void internalExecute(::Firebird::IStatus* status, ::Firebird::ExternalContext* context, \
		InMessage::Type* in, OutMessage::Type* out)


#define FB_UDR_BEGIN_PROCEDURE(name)	\
	namespace Proc##name	\
	{	\
		class Impl;	\
		\
		static ::Firebird::Udr::ProcedureFactoryImpl<Impl> factory(#name);	\
		\
		class Impl : public ::Firebird::Udr::Procedure<Impl>	\
		{	\
		public:	\
			FB__UDR_COMMON_IMPL

#define FB_UDR_END_PROCEDURE	\
			};	\
		};	\
	}

#define FB_UDR_EXECUTE_PROCEDURE	\
	virtual ::Firebird::ExternalResultSet* FB_CARG open(::Firebird::IStatus* status, \
		::Firebird::ExternalContext* context, void* in, void* out)	\
	{	\
		try	\
		{	\
			return new ResultSet(status, context, this, (InMessage::Type*) in, (OutMessage::Type*) out);	\
		}	\
		FB__UDR_CATCH	\
		\
		return NULL;	\
	}	\
	\
	class ResultSet : public ::Firebird::Udr::ResultSet<ResultSet, Impl, InMessage, OutMessage>	\
	{	\
	public:	\
		ResultSet(::Firebird::IStatus* status, ::Firebird::ExternalContext* context,	\
				Impl* const procedure, InMessage::Type* const in, OutMessage::Type* const out)	\
			: ::Firebird::Udr::ResultSet<ResultSet, Impl, InMessage, OutMessage>(	\
					context, procedure, in, out)

#define FB_UDR_FETCH_PROCEDURE	\
	virtual FB_BOOLEAN FB_CARG fetch(::Firebird::IStatus* status)	\
	{	\
		try	\
		{	\
			return (FB_BOOLEAN) internalFetch(status);	\
		}	\
		FB__UDR_CATCH	\
		\
		return FB_FALSE;	\
	}	\
	\
	bool internalFetch(::Firebird::IStatus* status)


#define FB_UDR_BEGIN_TRIGGER(name)	\
	namespace Trig##name	\
	{	\
		class Impl;	\
		\
		static ::Firebird::Udr::TriggerFactoryImpl<Impl> factory(#name);	\
		\
		class Impl : public ::Firebird::Udr::Trigger<Impl>	\
		{	\
		public:	\
			FB__UDR_COMMON_IMPL

#define FB_UDR_END_TRIGGER	\
		};	\
	}

#define FB_UDR_EXECUTE_TRIGGER	\
	virtual void FB_CARG execute(::Firebird::IStatus* status, ::Firebird::ExternalContext* context,	\
		::Firebird::ExternalTrigger::Action action, void* oldFields, void* newFields)	\
	{	\
		try	\
		{	\
			internalExecute(status, context, action,	\
				(FieldsMessage::Type*) oldFields, (FieldsMessage::Type*) newFields);	\
		}	\
		FB__UDR_CATCH	\
	}	\
	\
	void internalExecute(::Firebird::IStatus* status, ::Firebird::ExternalContext* context,	\
		::Firebird::ExternalTrigger::Action action, \
		FieldsMessage::Type* oldFields, FieldsMessage::Type* newFields)


#define FB_UDR_CONSTRUCTOR	\
	Impl(::Firebird::IStatus* const status, ::Firebird::ExternalContext* const context,	\
			const ::Firebird::IRoutineMetadata* const metadata__)	\
		: master(context->getMaster()),	\
		  metadata(metadata__)

#define FB_UDR_DESTRUCTOR	\
	~Impl()


#define FB__UDR_COMMON_IMPL	\
	Impl(const void* const, ::Firebird::ExternalContext* const context,	\
			const ::Firebird::IRoutineMetadata* const aMetadata)	\
		: master(context->getMaster()),	\
		  metadata(aMetadata)	\
	{	\
	}	\
	\
	::Firebird::IMaster* master;	\
	const ::Firebird::IRoutineMetadata* metadata;

#define FB__UDR_COMMON_TYPE(name)	\
	struct name	\
	{	\
		typedef unsigned char Type;	\
		static void setup(::Firebird::IStatus*, ::Firebird::IMetadataBuilder*) {}	\
	}

#define FB__UDR_CATCH	\
	catch (const ::Firebird::Udr::StatusException& e)	\
	{	\
		e.stuff(status);	\
	}	\
	catch (...)	\
	{	\
		ISC_STATUS statusVector[] = {	\
			isc_arg_gds, isc_random, isc_arg_string, (ISC_STATUS) "Unrecognized C++ exception",	\
			isc_arg_end};	\
		status->set(statusVector);	\
	}


class StatusException
{
public:
	StatusException(const ISC_STATUS* vector)
	{
		ISC_STATUS* p = statusVector;

		while (*vector != isc_arg_end)
		{
			switch (*vector)
			{
				case isc_arg_warning:
				case isc_arg_gds:
				case isc_arg_number:
				case isc_arg_interpreted:
				case isc_arg_vms:
				case isc_arg_unix:
				case isc_arg_win32:
					*p++ = *vector++;
					*p++ = *vector++;
					break;

				case isc_arg_string:
					*p++ = *vector++;
					*p++ = *vector++;
					break;

				case isc_arg_cstring:
					*p++ = *vector++;
					*p++ = *vector++;
					*p++ = *vector++;
					break;

				default:
					return;
			}
		}

		*p = isc_arg_end;
	}

public:
	static void check(const ISC_STATUS* vector)
	{
		if (vector[1])
			throw StatusException(vector);
	}

	static void checkStatus(ISC_STATUS status, const ISC_STATUS* vector)
	{
		if (status == 0)
			return;

		check(vector);
	}

	template <typename T>
	static T check(IStatus* status, T value)
	{
		check(status->get());
		return value;
	}

public:
	const ISC_STATUS* getStatusVector() const
	{
		return statusVector;
	}

	void stuff(IStatus* status) const
	{
		status->set(statusVector);
	}

private:
	ISC_STATUS_ARRAY statusVector;
};

class StatusImpl : public IStatus
{
public:
	StatusImpl(IMaster* master)
		: delegate(master->getStatus()),
		  success(true)
	{
	}

	virtual int FB_CARG getVersion()
	{
		return FB_STATUS_VERSION;
	}

	virtual IPluginModule* FB_CARG getModule()
	{
		return NULL;
	}

	virtual void FB_CARG dispose()
	{
		delegate->dispose();
		delete this;
	}

	virtual void FB_CARG set(unsigned int length, const ISC_STATUS* value)
	{
		delegate->set(length, value);
		success = delegate->isSuccess();
	}

	virtual void FB_CARG set(const ISC_STATUS* value)
	{
		delegate->set(value);
		success = delegate->isSuccess();
	}

	virtual void FB_CARG init()
	{
		delegate->init();
		success = true;
	}

	virtual const ISC_STATUS* FB_CARG get() const
	{
		return delegate->get();
	}

	virtual int FB_CARG isSuccess() const
	{
		return success;
	}

public:
	void check()
	{
		if (!success)
			StatusException::check(delegate->get());
	}

private:
	IStatus* delegate;
	bool success;
};


template <typename T> class Procedure;


class Helper
{
public:
	static isc_db_handle getIscDbHandle(ExternalContext* context)
	{
		StatusImpl status(context->getMaster());

		IAttachment* attachment = context->getAttachment(&status);
		status.check();

		ISC_STATUS_ARRAY statusVector = {0};
		isc_db_handle handle = 0;

		fb_get_database_handle(statusVector, &handle, attachment);
		StatusException::check(statusVector);

		return handle;
	}

	static isc_tr_handle getIscTrHandle(ExternalContext* context)
	{
		StatusImpl status(context->getMaster());

		ITransaction* transaction = context->getTransaction(&status);
		status.check();

		ISC_STATUS_ARRAY statusVector = {0};
		isc_tr_handle handle = 0;

		fb_get_transaction_handle(statusVector, &handle, transaction);
		StatusException::check(statusVector);

		return handle;
	}
};


template <typename This, typename Procedure, typename InMessage, typename OutMessage>
class ResultSet : public ExternalResultSet, public Helper
{
public:
	ResultSet(ExternalContext* aContext, Procedure* aProcedure,
				typename InMessage::Type* aIn, typename OutMessage::Type* aOut)
		: context(aContext),
		  procedure(aProcedure),
		  in(aIn),
		  out(aOut)
	{
	}

public:
	virtual int FB_CARG getVersion()
	{
		return FB_EXTERNAL_RESULT_SET_VERSION;
	}

	virtual IPluginModule* FB_CARG getModule()
	{
		return NULL;
	}

	virtual void FB_CARG dispose()
	{
		delete static_cast<This*>(this);
	}

protected:
	ExternalContext* const context;
	Procedure* const procedure;
	typename InMessage::Type* const in;
	typename OutMessage::Type* const out;
};


template <typename This>
class Function : public ExternalFunction, public Helper
{
public:
	FB__UDR_COMMON_TYPE(InMessage);
	FB__UDR_COMMON_TYPE(OutMessage);

	virtual int FB_CARG getVersion()
	{
		return FB_EXTERNAL_FUNCTION_VERSION;
	}

	virtual IPluginModule* FB_CARG getModule()
	{
		return NULL;
	}

	virtual void FB_CARG dispose()
	{
		delete static_cast<This*>(this);
	}

	virtual void FB_CARG getCharSet(IStatus* /*status*/, ExternalContext* /*context*/,
		Utf8* /*name*/, uint /*nameSize*/)
	{
	}
};


template <typename This>
class Procedure : public ExternalProcedure, public Helper
{
public:
	FB__UDR_COMMON_TYPE(InMessage);
	FB__UDR_COMMON_TYPE(OutMessage);

	virtual int FB_CARG getVersion()
	{
		return FB_EXTERNAL_PROCEDURE_VERSION;
	}

	virtual IPluginModule* FB_CARG getModule()
	{
		return NULL;
	}

	virtual void FB_CARG dispose()
	{
		delete static_cast<This*>(this);
	}

	virtual void FB_CARG getCharSet(IStatus* /*status*/, ExternalContext* /*context*/,
		Utf8* /*name*/, uint /*nameSize*/)
	{
	}
};


template <typename This>
class Trigger : public ExternalTrigger, public Helper
{
public:
	FB__UDR_COMMON_TYPE(FieldsMessage);

	virtual int FB_CARG getVersion()
	{
		return FB_EXTERNAL_TRIGGER_VERSION;
	}

	virtual IPluginModule* FB_CARG getModule()
	{
		return NULL;
	}

	virtual void FB_CARG dispose()
	{
		delete static_cast<This*>(this);
	}

	virtual void FB_CARG getCharSet(IStatus* /*status*/, ExternalContext* /*context*/,
		Utf8* /*name*/, uint /*nameSize*/)
	{
	}
};


template <typename T> class FunctionFactoryImpl : public FunctionFactory
{
public:
	explicit FunctionFactoryImpl(const char* name)
	{
		fbUdrRegFunction(name, this);
	}

	virtual void FB_CARG setup(IStatus* status, ExternalContext* /*context*/,
		const IRoutineMetadata* /*metadata*/, IMetadataBuilder* in, IMetadataBuilder* out)
	{
		T::InMessage::setup(status, in);
		T::OutMessage::setup(status, out);
	}

	virtual ExternalFunction* FB_CARG newItem(IStatus* status, ExternalContext* context,
		const IRoutineMetadata* metadata)
	{
		try
		{
			return new T(status, context, metadata);
		}
		FB__UDR_CATCH

		return NULL;
	}
};


template <typename T> class ProcedureFactoryImpl : public ProcedureFactory
{
public:
	explicit ProcedureFactoryImpl(const char* name)
	{
		fbUdrRegProcedure(name, this);
	}

	virtual void FB_CARG setup(IStatus* status, ExternalContext* /*context*/,
		const IRoutineMetadata* /*metadata*/, IMetadataBuilder* in, IMetadataBuilder* out)
	{
		T::InMessage::setup(status, in);
		T::OutMessage::setup(status, out);
	}

	virtual ExternalProcedure* FB_CARG newItem(IStatus* status, ExternalContext* context,
		const IRoutineMetadata* metadata)
	{
		try
		{
			return new T(status, context, metadata);
		}
		FB__UDR_CATCH

		return NULL;
	}
};


template <typename T> class TriggerFactoryImpl : public TriggerFactory
{
public:
	explicit TriggerFactoryImpl(const char* name)
	{
		fbUdrRegTrigger(name, this);
	}

	virtual void FB_CARG setup(IStatus* status, ExternalContext* /*context*/,
		const IRoutineMetadata* /*metadata*/, IMetadataBuilder* fields)
	{
		T::FieldsMessage::setup(status, fields);
	}

	virtual ExternalTrigger* FB_CARG newItem(IStatus* status, ExternalContext* context,
		const IRoutineMetadata* metadata)
	{
		try
		{
			return new T(status, context, metadata);
		}
		FB__UDR_CATCH

		return NULL;
	}
};


//------------------------------------------------------------------------------
	}	// namespace Udr
}	// namespace Firebird

#endif	// FIREBIRD_UDR_CPP_ENGINE

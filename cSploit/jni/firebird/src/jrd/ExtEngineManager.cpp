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

#include "firebird.h"
#include "consts_pub.h"
#include "iberror.h"
#include "inf_pub.h"
#include "../jrd/ExtEngineManager.h"
#include "../dsql/sqlda_pub.h"
#include "../common/dsc.h"
#include "../jrd/align.h"
#include "../jrd/jrd.h"
#include "../jrd/exe.h"
#include "../jrd/req.h"
#include "../jrd/status.h"
#include "../jrd/tra.h"
#include "../jrd/ibase.h"
#include "../dsql/StmtNodes.h"
#include "../common/os/path_utils.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/cvt_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/par_proto.h"
#include "../jrd/thread_proto.h"
#include "../jrd/Function.h"
#include "../common/isc_proto.h"
#include "../common/classes/auto.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/init.h"
#include "../common/classes/objects_array.h"
#include "../common/config/config.h"
#include "../common/ScanDir.h"
#include "../common/utils_proto.h"
#include "../common/classes/GetPlugins.h"

using namespace Firebird;
using namespace Jrd;


namespace
{
	// External message node.
	class ExtMessageNode : public MessageNode
	{
	public:
		ExtMessageNode(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, USHORT message,
				Array<NestConst<Parameter> >& aParameters, const Format* aFormat)
			: MessageNode(pool),
			  parameters(aParameters),
			  format(aFormat),
			  isSpecial(pool)
		{
			setup(tdbb, csb, message, format->fmt_count);
		}

		virtual USHORT setupDesc(thread_db* tdbb, CompilerScratch* csb, USHORT index,
			dsc* desc, ItemInfo* itemInfo)
		{
			*desc = format->fmt_desc[index];

			if (index % 2 == 0 && index / 2u < parameters.getCount())
			{
				const Parameter* param = parameters[index / 2];

				if (param->prm_mechanism != prm_mech_type_of &&
					!fb_utils::implicit_domain(param->prm_field_source.c_str()))
				{
					MetaNamePair namePair(param->prm_field_source, "");

					FieldInfo fieldInfo;
					bool exist = csb->csb_map_field_info.get(namePair, fieldInfo);
					MET_get_domain(tdbb, csb->csb_pool, param->prm_field_source, desc,
						(exist ? NULL : &fieldInfo));

					if (!exist)
						csb->csb_map_field_info.put(namePair, fieldInfo);

					itemInfo->field = namePair;
					itemInfo->nullable = fieldInfo.nullable;
					itemInfo->fullDomain = true;
				}

				itemInfo->name = param->prm_name;

				if (!param->prm_nullable)
					itemInfo->nullable = false;

				isSpecial.getBuffer(index / 2 + 1)[index / 2] = itemInfo->isSpecial();
			}

			return type_alignments[desc->dsc_dtype];
		}

		virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const
		{
			if (request->req_operation == jrd_req::req_evaluate)
			{
				// Clear the message. This is important for external routines.
				UCHAR* msg = request->getImpure<UCHAR>(impureOffset);
				memset(msg, 0, format->fmt_length);
			}

			return MessageNode::execute(tdbb, request, exeState);
		}

	public:
		Array<NestConst<Parameter> >& parameters;
		const Format* format;
		Array<bool> isSpecial;
	};

	// Initialize output parameters with their domains default value or NULL.
	// Kind of blr_init_variable, but for parameters.
	class ExtInitParameterNode : public TypedNode<StmtNode, StmtNode::TYPE_EXT_INIT_PARAMETER>
	{
	public:
		ExtInitParameterNode(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb,
				ExtMessageNode* aMessage, USHORT aArgNumber)
			: TypedNode<StmtNode, StmtNode::TYPE_EXT_INIT_PARAMETER>(pool),
			  message(aMessage),
			  argNumber(aArgNumber)
		{
			Parameter* parameter = message->parameters[argNumber / 2];

			if (parameter->prm_mechanism == prm_mech_type_of ||
				fb_utils::implicit_domain(parameter->prm_field_source.c_str()) ||
				!parameter->prm_default_value)
			{
				defaultValueNode = NULL;
			}
			else
				defaultValueNode = CMP_clone_node(tdbb, csb, parameter->prm_default_value);
		}

		void print(string& text) const
		{
			text = "ExtInitParameterNode";
		}

		void genBlr(DsqlCompilerScratch* /*dsqlScratch*/)
		{
		}

		ExtInitParameterNode* pass1(thread_db* tdbb, CompilerScratch* csb)
		{
			doPass1(tdbb, csb, &defaultValueNode);
			return this;
		}

		ExtInitParameterNode* pass2(thread_db* tdbb, CompilerScratch* csb)
		{
			ExprNode::doPass2(tdbb, csb, &defaultValueNode);
			return this;
		}

		const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* /*exeState*/) const
		{
			if (request->req_operation == jrd_req::req_evaluate)
			{
				dsc* defaultDesc = NULL;

				if (defaultValueNode)
				{
					defaultDesc = EVL_expr(tdbb, request, defaultValueNode);

					if (request->req_flags & req_null)
						defaultDesc = NULL;
				}

				if (defaultDesc)
				{
					// Initialize the value. The null flag is already initialized to not-null
					// by the ExtMessageNode.

					dsc desc = message->format->fmt_desc[argNumber];
					desc.dsc_address = request->getImpure<UCHAR>(
						message->impureOffset + (IPTR) desc.dsc_address);

					MOV_move(tdbb, defaultDesc, &desc);
				}
				else
				{
					SSHORT tempValue = -1;
					dsc temp;
					temp.makeShort(0, &tempValue);

					dsc desc = message->format->fmt_desc[argNumber + 1];
					desc.dsc_address = request->getImpure<UCHAR>(
						message->impureOffset + (IPTR) desc.dsc_address);

					MOV_move(tdbb, &temp, &desc);
				}

				request->req_operation = jrd_req::req_return;
			}

			return parentStmt;
		}

		private:
			ExtMessageNode* message;
			USHORT argNumber;
			ValueExprNode* defaultValueNode;
	};

	// External output parameters initialization.
	class ExtInitOutputNode : public CompoundStmtNode
	{
	public:
		ExtInitOutputNode(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb,
				ExtMessageNode* message)
			: CompoundStmtNode(pool)
		{
			// Iterate over the format items, except the EOF item.
			for (USHORT i = 0; i < (message->format->fmt_count / 2) * 2; i += 2)
			{
				ExtInitParameterNode* init = FB_NEW(pool) ExtInitParameterNode(
					tdbb, pool, csb, message, i);
				statements.add(init);
			}
		}
	};

	// External parameters validation.
	class ExtValidationNode : public CompoundStmtNode
	{
	public:
		ExtValidationNode(MemoryPool& pool, ExtMessageNode* message, bool procedure, bool input)
			: CompoundStmtNode(pool)
		{
			// Iterate over the format items, except the EOF item.
			for (USHORT i = 0; i < (message->format->fmt_count / 2) * 2; i += 2)
			{
				if (!message->isSpecial[i / 2])
					continue;

				ParameterNode* flag = FB_NEW(pool) ParameterNode(pool);
				flag->message = message;
				flag->argNumber = i + 1;

				ParameterNode* param = FB_NEW(pool) ParameterNode(pool);
				param->message = message;
				param->argNumber = i;
				param->argFlag = flag;

				AssignmentNode* assign = FB_NEW(pool) AssignmentNode(pool);
				assign->asgnFrom = param;
				statements.add(assign);

				// It's sufficient to assign input parameters to NULL, but output parameters
				// need to be assigned to themselves to validate correctly.
				if (input)
					assign->asgnTo = FB_NEW(pool) NullNode(pool);
				else
				{
					flag = FB_NEW(pool) ParameterNode(pool);
					flag->message = message;
					flag->argNumber = i + 1;

					param = FB_NEW(pool) ParameterNode(pool);
					param->message = message;
					param->argNumber = i;
					param->argFlag = flag;

					assign->asgnTo = param;
				}
			}

			// A stall is required to read req_proc_fetch state correctly.
			if (procedure)
				statements.add(FB_NEW(pool) StallNode(pool));
		}
	};

	// External function node.
	class ExtFunctionNode : public SuspendNode
	{
	public:
		ExtFunctionNode(MemoryPool& pool, const ReceiveNode* aReceiveNode,
				const ExtEngineManager::Function* aFunction)
			: SuspendNode(pool),
			  receiveNode(aReceiveNode),
			  function(aFunction)
		{
		}

		virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const
		{
			if (request->req_operation == jrd_req::req_evaluate)
			{
				UCHAR* inMsg = receiveNode ?
					request->getImpure<UCHAR>(receiveNode->message->impureOffset) : NULL;
				UCHAR* outMsg = request->getImpure<UCHAR>(message->impureOffset);

				function->execute(tdbb, inMsg, outMsg);
			}

			return SuspendNode::execute(tdbb, request, exeState);
		}

	private:
		const ReceiveNode* receiveNode;
		const ExtEngineManager::Function* function;
	};

	// External procedure node.
	class ExtProcedureNode : public SuspendNode
	{
	public:
		ExtProcedureNode(MemoryPool& pool, const ReceiveNode* aReceiveNode,
				const ExtEngineManager::Procedure* aProcedure)
			: SuspendNode(pool),
			  receiveNode(aReceiveNode),
			  procedure(aProcedure)
		{
		}

		virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const
		{
			ExtEngineManager::ResultSet*& resultSet = request->req_ext_resultset;
			UCHAR* inMsg = receiveNode ?
				request->getImpure<UCHAR>(receiveNode->message->impureOffset) : NULL;
			UCHAR* outMsg = request->getImpure<UCHAR>(message->impureOffset);
			USHORT* eof = (USHORT*) (outMsg + (IPTR) message->format->fmt_desc.back().dsc_address);

			switch (request->req_operation)
			{
				case jrd_req::req_evaluate:
					fb_assert(!resultSet && *eof == 0);
					resultSet = procedure->open(tdbb, inMsg, outMsg);

					if (resultSet)
						*eof = -1;
					else
					{
						if (!(request->req_flags & req_proc_fetch))
						{
							request->req_operation = jrd_req::req_evaluate;
							return statement;
						}
					}

					request->req_operation = jrd_req::req_return;
					// fall into

				case jrd_req::req_return:
					if (*eof == 0)
					{
						fb_assert(!resultSet);
						return parentStmt;
					}

					fb_assert(resultSet);

					if (!resultSet->fetch(tdbb) || !(request->req_flags & req_proc_fetch))
					{
						*eof = 0;
						delete resultSet;
						resultSet = NULL;
					}

					break;

				case jrd_req::req_proceed:
					request->req_operation = jrd_req::req_evaluate;
					return statement;

				case jrd_req::req_unwind:
					delete resultSet;
					resultSet = NULL;
					break;
			}

			return SuspendNode::execute(tdbb, request, exeState);
		}

	private:
		const ReceiveNode* receiveNode;
		const ExtEngineManager::Procedure* procedure;
	};

	// External trigger node.
	class ExtTriggerNode : public TypedNode<StmtNode, StmtNode::TYPE_EXT_TRIGGER>
	{
	public:
		ExtTriggerNode(MemoryPool& pool, const ExtEngineManager::Trigger* aTrigger)
			: TypedNode<StmtNode, StmtNode::TYPE_EXT_TRIGGER>(pool),
			  trigger(aTrigger)
		{
		}

		void print(string& text) const
		{
			text = "ExtTriggerNode";
		}

		void genBlr(DsqlCompilerScratch* /*dsqlScratch*/)
		{
		}

		ExtTriggerNode* pass1(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
		{
			return this;
		}

		ExtTriggerNode* pass2(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
		{
			return this;
		}

		const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* /*exeState*/) const
		{
			if (request->req_operation == jrd_req::req_evaluate)
			{
				trigger->execute(tdbb, (ExternalTrigger::Action) request->req_trigger_action,
					getRpb(request, 0), getRpb(request, 1));

				request->req_operation = jrd_req::req_return;
			}

			return parentStmt;
		}

	private:
		static record_param* getRpb(jrd_req* request, USHORT n)
		{
			return request->req_rpb.getCount() > n && request->req_rpb[n].rpb_number.isValid() ?
				&request->req_rpb[n] : NULL;
		}


	private:
		const ExtEngineManager::Trigger* trigger;
	};
}


namespace Jrd {

static MakeUpgradeInfo<> upInfo;


template <typename T> class ExtEngineManager::ContextManager
{
public:
	ContextManager(thread_db* tdbb, EngineAttachmentInfo* aAttInfo, T* obj,
				CallerName aCallerName = CallerName())
		: attInfo(aAttInfo),
		  attachment(tdbb->getAttachment()),
		  transaction(tdbb->getTransaction()),
		  charSet(attachment->att_charset),
		  attInUse(attachment->att_in_use),
		  traInUse(transaction ? transaction->tra_in_use : false)
	{
		// !!!!!  needs async lock to be safe
		attachment->att_in_use = true;

		if (transaction)
		{
			callerName = transaction->tra_caller_name;
			transaction->tra_caller_name = aCallerName;
			++transaction->tra_callback_count;
			transaction->tra_in_use = true;
		}

		attInfo->context->setTransaction(tdbb);

		setCharSet(tdbb, attInfo, obj);
	}

	ContextManager(thread_db* tdbb, EngineAttachmentInfo* aAttInfo, USHORT aCharSet,
				CallerName aCallerName = CallerName())
		: attInfo(aAttInfo),
		  attachment(tdbb->getAttachment()),
		  transaction(tdbb->getTransaction()),
		  charSet(attachment->att_charset),
		  attInUse(attachment->att_in_use),
		  traInUse(transaction ? transaction->tra_in_use : false)
	{
		attachment->att_charset = aCharSet;
		// !!!!!  needs async lock to be safe
		attachment->att_in_use = true;

		if (transaction)
		{
			callerName = transaction->tra_caller_name;
			transaction->tra_caller_name = aCallerName;
			++transaction->tra_callback_count;
			transaction->tra_in_use = true;
		}

		attInfo->context->setTransaction(tdbb);
	}

	~ContextManager()
	{
		if (transaction)
		{
			--transaction->tra_callback_count;
			transaction->tra_in_use = traInUse;
			transaction->tra_caller_name = callerName;
		}

		// !!!!!  needs async lock to be safe
		attachment->att_in_use = attInUse;
		attachment->att_charset = charSet;
	}

private:
	void setCharSet(thread_db* tdbb, EngineAttachmentInfo* attInfo, T* obj)
	{
		attachment->att_charset = attInfo->adminCharSet;

		if (!obj)
			return;

		Utf8 charSetName[MAX_SQL_IDENTIFIER_SIZE];

		{	// scope
			Attachment::Checkout attCout(attachment, FB_FUNCTION);

			LocalStatus status;
			obj->getCharSet(&status, attInfo->context, charSetName, MAX_SQL_IDENTIFIER_LEN);
			status.check();
			charSetName[MAX_SQL_IDENTIFIER_LEN] = '\0';
		}

		USHORT charSetId;

		if (!MET_get_char_coll_subtype(tdbb, &charSetId,
				reinterpret_cast<const UCHAR*>(charSetName), strlen(charSetName)))
		{
			status_exception::raise(Arg::Gds(isc_charset_not_found) << Arg::Str(charSetName));
		}

		attachment->att_charset = charSetId;
	}

private:
	EngineAttachmentInfo* attInfo;
	Jrd::Attachment* attachment;
	jrd_tra* transaction;
	// These data members are to restore the original information.
	const USHORT charSet;
	const bool attInUse;
	const bool traInUse;
	CallerName callerName;
};


//---------------------


ExtEngineManager::ExternalContextImpl::ExternalContextImpl(thread_db* tdbb,
		ExternalEngine* aEngine)
	: engine(aEngine),
	  internalAttachment(tdbb->getAttachment()),
	  internalTransaction(NULL),
	  externalAttachment(NULL),
	  externalTransaction(NULL),
	  miscInfo(*internalAttachment->att_pool)
{
	//// TODO: admin rights

	clientCharSet = INTL_charset_lookup(tdbb, internalAttachment->att_client_charset)->getName();

	internalAttachment->att_interface->addRef();

	externalAttachment = MasterInterfacePtr()->registerAttachment(JProvider::getInstance(),
		internalAttachment->att_interface);
}

ExtEngineManager::ExternalContextImpl::~ExternalContextImpl()
{
	releaseTransaction();

	if (externalAttachment)
	{
		externalAttachment->release();
		externalAttachment = NULL;
	}
}

void ExtEngineManager::ExternalContextImpl::releaseTransaction()
{
	if (externalTransaction)
	{
		externalTransaction->release();
		externalTransaction = NULL;
	}

	internalTransaction = NULL;
}

void ExtEngineManager::ExternalContextImpl::setTransaction(thread_db* tdbb)
{
	jrd_tra* newTransaction = tdbb->getTransaction();

	if (newTransaction == internalTransaction)
		return;

	releaseTransaction();
	fb_assert(!externalTransaction);

	if ((internalTransaction = newTransaction))
	{
		internalTransaction->getInterface()->addRef();

		externalTransaction = MasterInterfacePtr()->registerTransaction(externalAttachment,
			internalTransaction->getInterface());
	}
}

IMaster* ExtEngineManager::ExternalContextImpl::getMaster()
{
	MasterInterfacePtr master;
	return master;
}

ExternalEngine* ExtEngineManager::ExternalContextImpl::getEngine(IStatus* /*status*/)
{
	return engine;
}

Firebird::IAttachment* FB_CARG ExtEngineManager::ExternalContextImpl::getAttachment(IStatus* /*status*/)
{
	return externalAttachment;
}

Firebird::ITransaction* FB_CARG ExtEngineManager::ExternalContextImpl::getTransaction(IStatus* /*status*/)
{
	return externalTransaction;
}

const char* FB_CARG ExtEngineManager::ExternalContextImpl::getUserName()
{
	return internalAttachment->att_user->usr_user_name.c_str();
}

const char* FB_CARG ExtEngineManager::ExternalContextImpl::getDatabaseName()
{
	return internalAttachment->att_database->dbb_database_name.c_str();
}

const Utf8* FB_CARG ExtEngineManager::ExternalContextImpl::getClientCharSet()
{
	return clientCharSet.c_str();
}

int FB_CARG ExtEngineManager::ExternalContextImpl::obtainInfoCode()
{
	static AtomicCounter counter;
	return ++counter;
}

void* FB_CARG ExtEngineManager::ExternalContextImpl::getInfo(int code)
{
	void* value = NULL;
	miscInfo.get(code, value);
	return value;
}

void* FB_CARG ExtEngineManager::ExternalContextImpl::setInfo(int code, void* value)
{
	void* oldValue = getInfo(code);
	miscInfo.put(code, value);
	return oldValue;
}


//---------------------


ExtEngineManager::Function::Function(thread_db* tdbb, ExtEngineManager* aExtManager,
		ExternalEngine* aEngine, RoutineMetadata* aMetadata, ExternalFunction* aFunction,
		const Jrd::Function* aUdf)
	: extManager(aExtManager),
	  engine(aEngine),
	  metadata(aMetadata),
	  function(aFunction),
	  udf(aUdf),
	  database(tdbb->getDatabase())
{
}


ExtEngineManager::Function::~Function()
{
	//Database::Checkout dcoHolder(database);
	function->dispose();
}


void ExtEngineManager::Function::execute(thread_db* tdbb, UCHAR* inMsg, UCHAR* outMsg) const
{
	EngineAttachmentInfo* attInfo = extManager->getEngineAttachment(tdbb, engine);
	ContextManager<ExternalFunction> ctxManager(tdbb, attInfo, function,
		(udf->getName().package.isEmpty() ?
			CallerName(obj_udf, udf->getName().identifier) :
			CallerName(obj_package_header, udf->getName().package)));

	Attachment::Checkout attCout(tdbb->getAttachment(), FB_FUNCTION);

	LocalStatus status;
	function->execute(&status, attInfo->context, inMsg, outMsg);
	status.check();
}


//---------------------


ExtEngineManager::Procedure::Procedure(thread_db* tdbb, ExtEngineManager* aExtManager,
	    ExternalEngine* aEngine, RoutineMetadata* aMetadata, ExternalProcedure* aProcedure,
		const jrd_prc* aPrc)
	: extManager(aExtManager),
	  engine(aEngine),
	  metadata(aMetadata),
	  procedure(aProcedure),
	  prc(aPrc),
	  database(tdbb->getDatabase())
{
}


ExtEngineManager::Procedure::~Procedure()
{
	//Database::Checkout dcoHolder(database);
	procedure->dispose();
}


ExtEngineManager::ResultSet* ExtEngineManager::Procedure::open(thread_db* tdbb,
	UCHAR* inMsg, UCHAR* outMsg) const
{
	return FB_NEW(*tdbb->getDefaultPool()) ResultSet(tdbb, inMsg, outMsg, this);
}


//---------------------


ExtEngineManager::ResultSet::ResultSet(thread_db* tdbb, UCHAR* inMsg, UCHAR* outMsg,
		const ExtEngineManager::Procedure* aProcedure)
	: procedure(aProcedure),
	  attachment(tdbb->getAttachment()),
	  firstFetch(true)
{
	attInfo = procedure->extManager->getEngineAttachment(tdbb, procedure->engine);
	ContextManager<ExternalProcedure> ctxManager(tdbb, attInfo, procedure->procedure,
		(procedure->prc->getName().package.isEmpty() ?
			CallerName(obj_procedure, procedure->prc->getName().identifier) :
			CallerName(obj_package_header, procedure->prc->getName().package)));

	charSet = attachment->att_charset;

	Attachment::Checkout attCout(attachment, FB_FUNCTION);

	LocalStatus status;
	resultSet = procedure->procedure->open(&status, attInfo->context, inMsg, outMsg);
	status.check();
}


ExtEngineManager::ResultSet::~ResultSet()
{
	if (resultSet)
	{
		fb_assert(attachment == JRD_get_thread_data()->getAttachment());
		Attachment::Checkout attCout(attachment, FB_FUNCTION);
		resultSet->dispose();
	}
}


bool ExtEngineManager::ResultSet::fetch(thread_db* tdbb)
{
	bool wasFirstFetch = firstFetch;
	firstFetch = false;

	if (!resultSet)
		return wasFirstFetch;

	ContextManager<ExternalProcedure> ctxManager(tdbb, attInfo, charSet,
		(procedure->prc->getName().package.isEmpty() ?
			CallerName(obj_procedure, procedure->prc->getName().identifier) :
			CallerName(obj_package_header, procedure->prc->getName().package)));

	fb_assert(attachment == tdbb->getAttachment());
	Attachment::Checkout attCout(attachment, FB_FUNCTION);

	LocalStatus status;
	bool ret = resultSet->fetch(&status);
	status.check();

	return ret;
}


//---------------------


ExtEngineManager::Trigger::Trigger(thread_db* tdbb, MemoryPool& pool, ExtEngineManager* aExtManager,
			ExternalEngine* aEngine, RoutineMetadata* aMetadata,
			ExternalTrigger* aTrigger, const Jrd::Trigger* aTrg)
	: extManager(aExtManager),
	  engine(aEngine),
	  metadata(aMetadata),
	  trigger(aTrigger),
	  trg(aTrg),
	  fieldsPos(pool),
	  database(tdbb->getDatabase())
{
	dsc shortDesc;
	shortDesc.makeShort(0);

	jrd_rel* relation = trg->relation;

	if (relation)
	{
		GenericMap<Left<MetaName, USHORT> > fieldsMap;

		for (size_t i = 0; i < relation->rel_fields->count(); ++i)
		{
			jrd_fld* field = (*relation->rel_fields)[i];
			if (field)
				fieldsMap.put(field->fld_name, (USHORT) i);
		}

		format = Routine::createFormat(pool, metadata->triggerFields, false);

		LocalStatus status;

		for (unsigned i = 0; i < format->fmt_count / 2u; ++i)
		{
			const char* fieldName = metadata->triggerFields->getField(&status, i);
			status.check();

			USHORT pos;

			if (!fieldsMap.get(fieldName, pos))
				fb_assert(false);
			else
				fieldsPos.add(pos);
		}
	}
}


ExtEngineManager::Trigger::~Trigger()
{
	// hvlad: shouldn't we call trigger->dispose() here ?
}


void ExtEngineManager::Trigger::execute(thread_db* tdbb, ExternalTrigger::Action action,
	record_param* oldRpb, record_param* newRpb) const
{
	EngineAttachmentInfo* attInfo = extManager->getEngineAttachment(tdbb, engine);
	ContextManager<ExternalTrigger> ctxManager(tdbb, attInfo, trigger,
		CallerName(obj_trigger, trg->name));

	// ASF: Using Array instead of HalfStaticArray to not need to do alignment hacks here.
	Array<UCHAR> oldMsg;
	Array<UCHAR> newMsg;

	if (oldRpb)
		setValues(tdbb, oldMsg, oldRpb);

	if (newRpb)
		setValues(tdbb, newMsg, newRpb);

	{	// scope
		Attachment::Checkout attCout(tdbb->getAttachment(), FB_FUNCTION);

		LocalStatus status;
		trigger->execute(&status, attInfo->context, action,
			(oldMsg.hasData() ? oldMsg.begin() : NULL), (newMsg.hasData() ? newMsg.begin() : NULL));
		status.check();
	}

	if (newRpb)
	{
		// Move data back from the message to the record.

		Record* record = newRpb->rpb_record;
		UCHAR* p = newMsg.begin();

		for (unsigned i = 0; i < format->fmt_count / 2u; ++i)
		{
			USHORT fieldPos = fieldsPos[i];

			dsc target;
			bool readonly = !EVL_field(newRpb->rpb_relation, record, fieldPos, &target) &&
				target.dsc_address && !(target.dsc_flags & DSC_null);

			if (!readonly)
			{
				SSHORT* nullSource = (SSHORT*) (p + (IPTR) format->fmt_desc[i * 2 + 1].dsc_address);

				if (*nullSource == 0)
				{
					dsc source = format->fmt_desc[i * 2];
					source.dsc_address += (IPTR) p;
					MOV_move(tdbb, &source, &target);
					record->clearNull(fieldPos);
				}
				else
					record->setNull(fieldPos);
			}
		}
	}
}


void ExtEngineManager::Trigger::setValues(thread_db* tdbb, Array<UCHAR>& msgBuffer,
	record_param* rpb) const
{
	if (!rpb || !rpb->rpb_record)
		return;

	UCHAR* p = msgBuffer.getBuffer(format->fmt_length);
	memset(p, 0, format->fmt_length);

	for (unsigned i = 0; i < format->fmt_count / 2u; ++i)
	{
		USHORT fieldPos = fieldsPos[i];

		dsc source;
		EVL_field(rpb->rpb_relation, rpb->rpb_record, fieldPos, &source);
		// CVC: I'm not sure why it's not important to check EVL_field's result.

		SSHORT* nullTarget = (SSHORT*) (p + (IPTR) format->fmt_desc[i * 2 + 1].dsc_address);
		*nullTarget = (source.dsc_flags & DSC_null) != 0 ? -1 : 0;

		if (*nullTarget == 0)
		{
			dsc target = format->fmt_desc[i * 2];
			target.dsc_address += (IPTR) p;
			MOV_move(tdbb, &source, &target);
		}
	}
}


//---------------------


ExtEngineManager::~ExtEngineManager()
{
	fb_assert(enginesAttachments.count() == 0);
/*
AP: Commented out this code due to later AV.

When engine is released, it does dlclose() plugin module (libudr_engine.so),
but that module is not actually unloaded - because UDR module (libudrcpp_example.so) is using
symbols from plugin module, therefore raising plugin module's reference count.
UDR module can be unloaded only from plugin module's global variable (ModuleMap modules) dtor,
which is not called as long as plugin module is not unloaded. As the result all this will be
unloaded only on program exit, causing at that moment AV if this code is active: it happens that
~ModuleMap dlcloses itself.

	PluginManagerInterfacePtr pi;

	EnginesMap::Accessor accessor(&engines);
	for (bool found = accessor.getFirst(); found; found = accessor.getNext())
	{
		ExternalEngine* engine = accessor.current()->second;
		pi->releasePlugin(engine);
	}
 */
}


//---------------------


void ExtEngineManager::initialize()
{
}


void ExtEngineManager::closeAttachment(thread_db* tdbb, Attachment* attachment)
{
	Array<ExternalEngine*> enginesCopy;

	{	// scope
		ReadLockGuard readGuard(enginesLock, FB_FUNCTION);

		EnginesMap::Accessor accessor(&engines);
		for (bool found = accessor.getFirst(); found; found = accessor.getNext())
			enginesCopy.add(accessor.current()->second);
	}

	RefDeb(DEB_RLS_JATT, "ExtEngineManager::closeAttachment");
	Attachment::Checkout attCout(attachment, FB_FUNCTION, true);

	for (Array<ExternalEngine*>::iterator i = enginesCopy.begin(); i != enginesCopy.end(); ++i)
	{
		ExternalEngine* engine = *i;
		EngineAttachmentInfo* attInfo = getEngineAttachment(tdbb, engine, true);

		if (attInfo)
		{
			{	// scope
				ContextManager<ExternalFunction> ctxManager(tdbb, attInfo, attInfo->adminCharSet);
				LocalStatus status;
				engine->closeAttachment(&status, attInfo->context);	//// FIXME: log status
			}

			delete attInfo;
		}
	}
}


void ExtEngineManager::makeFunction(thread_db* tdbb, CompilerScratch* csb, Jrd::Function* udf,
	const MetaName& engine, const string& entryPoint, const string& body)
{
	string entryPointTrimmed = entryPoint;
	entryPointTrimmed.trim();

	EngineAttachmentInfo* attInfo = getEngineAttachment(tdbb, engine);
	ContextManager<ExternalFunction> ctxManager(tdbb, attInfo, attInfo->adminCharSet,
		(udf->getName().package.isEmpty() ?
			CallerName(obj_udf, udf->getName().identifier) :
			CallerName(obj_package_header, udf->getName().package)));

	///MemoryPool& pool = *tdbb->getDefaultPool();
	MemoryPool& pool = *getDefaultMemoryPool();

	AutoPtr<RoutineMetadata> metadata(FB_NEW(pool) RoutineMetadata(pool));
	metadata->package = udf->getName().package;
	metadata->name = udf->getName().identifier;
	metadata->entryPoint = entryPointTrimmed;
	metadata->body = body;
	metadata->inputParameters = Routine::createMetadata(udf->getInputFields());
	metadata->outputParameters = Routine::createMetadata(udf->getOutputFields());

	LocalStatus status;

	RefPtr<IMetadataBuilder> inBuilder(REF_NO_INCR, metadata->inputParameters->getBuilder(&status));
	status.check();

	RefPtr<IMetadataBuilder> outBuilder(REF_NO_INCR, metadata->outputParameters->getBuilder(&status));
	status.check();

	ExternalFunction* externalFunction;

	{	// scope
		Attachment::Checkout attCout(tdbb->getAttachment(), FB_FUNCTION);

		externalFunction = attInfo->engine->makeFunction(&status, attInfo->context, metadata,
			inBuilder, outBuilder);
		status.check();

		if (!externalFunction)
		{
			status_exception::raise(
				Arg::Gds(isc_eem_func_not_returned) << udf->getName().toString() << engine);
		}

		metadata->inputParameters = inBuilder->getMetadata(&status);
		status.check();

		metadata->outputParameters = outBuilder->getMetadata(&status);
		status.check();
	}

	udf->setInputFormat(Routine::createFormat(pool, metadata->inputParameters, false));
	udf->setOutputFormat(Routine::createFormat(pool, metadata->outputParameters, true));

	try
	{
		udf->fun_external = FB_NEW(getPool()) Function(tdbb, this, attInfo->engine,
			metadata.release(), externalFunction, udf);

		CompoundStmtNode* mainNode = FB_NEW(getPool()) CompoundStmtNode(getPool());

		ExtMessageNode* inMessageNode = udf->getInputFields().hasData() ?
			FB_NEW(getPool()) ExtMessageNode(tdbb, getPool(), csb, 0,
				udf->getInputFields(), udf->getInputFormat()) :
			NULL;
		if (inMessageNode)
			mainNode->statements.add(inMessageNode);

		ExtMessageNode* outMessageNode = FB_NEW(getPool()) ExtMessageNode(tdbb, getPool(), csb, 1,
			udf->getOutputFields(), udf->getOutputFormat());
		mainNode->statements.add(outMessageNode);

		ExtInitOutputNode* initOutputNode = FB_NEW(getPool()) ExtInitOutputNode(
			tdbb, getPool(), csb, outMessageNode);
		mainNode->statements.add(initOutputNode);

		ReceiveNode* receiveNode = inMessageNode ?
			FB_NEW(getPool()) ReceiveNode(getPool()) : NULL;

		if (inMessageNode)
		{
			receiveNode->message = inMessageNode;
			receiveNode->statement = FB_NEW(getPool()) ExtValidationNode(
				getPool(), inMessageNode, false, true);
			mainNode->statements.add(receiveNode);
		}

		ExtFunctionNode* extFunctionNode = FB_NEW(getPool()) ExtFunctionNode(getPool(),
			receiveNode, udf->fun_external);
		mainNode->statements.add(extFunctionNode);
		extFunctionNode->message = outMessageNode;
		extFunctionNode->statement = FB_NEW(getPool()) ExtValidationNode(
			getPool(), outMessageNode, false, false);

		JrdStatement* statement = udf->getStatement();
		PAR_preparsed_node(tdbb, NULL, mainNode, NULL, &csb, &statement, false, 0);
		udf->setStatement(statement);
	}
	catch (...)
	{
		Attachment::Checkout attCout(tdbb->getAttachment(), FB_FUNCTION);
		externalFunction->dispose();
		throw;
	}
}


void ExtEngineManager::makeProcedure(thread_db* tdbb, CompilerScratch* csb, jrd_prc* prc,
	const MetaName& engine, const string& entryPoint, const string& body)
{
	string entryPointTrimmed = entryPoint;
	entryPointTrimmed.trim();

	EngineAttachmentInfo* attInfo = getEngineAttachment(tdbb, engine);
	ContextManager<ExternalProcedure> ctxManager(tdbb, attInfo, attInfo->adminCharSet,
		(prc->getName().package.isEmpty() ?
			CallerName(obj_procedure, prc->getName().identifier) :
			CallerName(obj_package_header, prc->getName().package)));

	///MemoryPool& pool = *tdbb->getDefaultPool();
	MemoryPool& pool = *getDefaultMemoryPool();

	AutoPtr<RoutineMetadata> metadata(FB_NEW(pool) RoutineMetadata(pool));
	metadata->package = prc->getName().package;
	metadata->name = prc->getName().identifier;
	metadata->entryPoint = entryPointTrimmed;
	metadata->body = body;
	metadata->inputParameters = Routine::createMetadata(prc->getInputFields());
	metadata->outputParameters = Routine::createMetadata(prc->getOutputFields());

	LocalStatus status;

	RefPtr<IMetadataBuilder> inBuilder(REF_NO_INCR, metadata->inputParameters->getBuilder(&status));
	status.check();

	RefPtr<IMetadataBuilder> outBuilder(REF_NO_INCR, metadata->outputParameters->getBuilder(&status));
	status.check();

	ExternalProcedure* externalProcedure;

	{	// scope
		Attachment::Checkout attCout(tdbb->getAttachment(), FB_FUNCTION);

		externalProcedure = attInfo->engine->makeProcedure(&status, attInfo->context, metadata,
			inBuilder, outBuilder);
		status.check();

		if (!externalProcedure)
		{
			status_exception::raise(
				Arg::Gds(isc_eem_proc_not_returned) <<
					prc->getName().toString() << engine);
		}

		metadata->inputParameters = inBuilder->getMetadata(&status);
		status.check();

		metadata->outputParameters = outBuilder->getMetadata(&status);
		status.check();
	}

	prc->setInputFormat(Routine::createFormat(pool, metadata->inputParameters, false));
	prc->setOutputFormat(Routine::createFormat(pool, metadata->outputParameters, true));

	try
	{
		prc->setExternal(FB_NEW(getPool()) Procedure(tdbb, this, attInfo->engine,
			metadata.release(), externalProcedure, prc));

		CompoundStmtNode* mainNode = FB_NEW(getPool()) CompoundStmtNode(getPool());

		ExtMessageNode* inMessageNode = prc->getInputFields().hasData() ?
			FB_NEW(getPool()) ExtMessageNode(tdbb, getPool(), csb, 0,
				prc->getInputFields(), prc->getInputFormat()) :
			NULL;
		if (inMessageNode)
			mainNode->statements.add(inMessageNode);

		ExtMessageNode* outMessageNode = FB_NEW(getPool()) ExtMessageNode(tdbb, getPool(), csb, 1,
			prc->getOutputFields(), prc->getOutputFormat());
		mainNode->statements.add(outMessageNode);

		ExtInitOutputNode* initOutputNode = FB_NEW(getPool()) ExtInitOutputNode(
			tdbb, getPool(), csb, outMessageNode);
		mainNode->statements.add(initOutputNode);

		ReceiveNode* receiveNode = inMessageNode ?
			FB_NEW(getPool()) ReceiveNode(getPool()) : NULL;

		if (inMessageNode)
		{
			receiveNode->message = inMessageNode;
			receiveNode->statement = FB_NEW(getPool()) ExtValidationNode(
				getPool(), inMessageNode, true, true);
			mainNode->statements.add(receiveNode);
		}

		ExtProcedureNode* extProcedureNode = FB_NEW(getPool()) ExtProcedureNode(getPool(),
			receiveNode, prc->getExternal());
		mainNode->statements.add(extProcedureNode);
		extProcedureNode->message = outMessageNode;
		extProcedureNode->statement = FB_NEW(getPool()) ExtValidationNode(
			getPool(), outMessageNode, true, false);

		JrdStatement* statement = prc->getStatement();
		PAR_preparsed_node(tdbb, NULL, mainNode, NULL, &csb, &statement, false, 0);
		prc->setStatement(statement);
	}
	catch (...)
	{
		Attachment::Checkout attCout(tdbb->getAttachment(), FB_FUNCTION);
		externalProcedure->dispose();
		throw;
	}
}


void ExtEngineManager::makeTrigger(thread_db* tdbb, CompilerScratch* csb, Jrd::Trigger* trg,
	const MetaName& engine, const string& entryPoint, const string& body,
	ExternalTrigger::Type type)
{
	string entryPointTrimmed = entryPoint;
	entryPointTrimmed.trim();

	EngineAttachmentInfo* attInfo = getEngineAttachment(tdbb, engine);
	ContextManager<ExternalTrigger> ctxManager(tdbb, attInfo, attInfo->adminCharSet,
		CallerName(obj_trigger, trg->name));

	///MemoryPool& pool = *tdbb->getDefaultPool();
	MemoryPool& pool = *getDefaultMemoryPool();

	AutoPtr<RoutineMetadata> metadata(FB_NEW(pool) RoutineMetadata(pool));
	metadata->name = trg->name;
	metadata->entryPoint = entryPointTrimmed;
	metadata->body = body;
	metadata->triggerType = type;

	jrd_rel* relation = trg->relation;

	if (relation)
	{
		metadata->triggerTable = relation->rel_name;

		MsgMetadata* fieldsMsg = new MsgMetadata;
		metadata->triggerFields = fieldsMsg;

		Format* relFormat = relation->rel_current_format;

		for (size_t i = 0; i < relation->rel_fields->count(); ++i)
		{
			jrd_fld* field = (*relation->rel_fields)[i];
			if (field)
				fieldsMsg->addItem(field->fld_name, !field->fld_not_null, relFormat->fmt_desc[i]);
		}
	}

	LocalStatus status;

	RefPtr<IMetadataBuilder> fieldsBuilder(REF_NO_INCR, relation ?
		metadata->triggerFields->getBuilder(&status) : NULL);
	if (relation)
	{
		status.check();
	}

	ExternalTrigger* externalTrigger;

	{	// scope
		Attachment::Checkout attCout(tdbb->getAttachment(), FB_FUNCTION);

		LocalStatus status;
		externalTrigger = attInfo->engine->makeTrigger(&status, attInfo->context, metadata,
			fieldsBuilder);
		status.check();

		if (!externalTrigger)
		{
			status_exception::raise(
				Arg::Gds(isc_eem_trig_not_returned) << trg->name << engine);
		}

		if (relation)
		{
			metadata->triggerFields = fieldsBuilder->getMetadata(&status);
			status.check();
		}
	}

	try
	{
		trg->extTrigger = FB_NEW(getPool()) Trigger(tdbb, pool, this, attInfo->engine,
			metadata.release(), externalTrigger, trg);

		CompoundStmtNode* mainNode = FB_NEW(getPool()) CompoundStmtNode(getPool());

		ExtTriggerNode* extTriggerNode = FB_NEW(getPool()) ExtTriggerNode(getPool(),
			trg->extTrigger);
		mainNode->statements.add(extTriggerNode);

		PAR_preparsed_node(tdbb, trg->relation, mainNode, NULL, &csb, &trg->statement, true, 0);
	}
	catch (...)
	{
		Attachment::Checkout attCout(tdbb->getAttachment(), FB_FUNCTION);
		externalTrigger->dispose();
		throw;
	}
}


ExternalEngine* ExtEngineManager::getEngine(thread_db* tdbb, const MetaName& name)
{
	ReadLockGuard readGuard(enginesLock, FB_FUNCTION);
	ExternalEngine* engine = NULL;

	if (!engines.get(name, engine))
	{
		readGuard.release();
		WriteLockGuard writeGuard(enginesLock, FB_FUNCTION);

		if (!engines.get(name, engine))
		{
			GetPlugins<ExternalEngine> engineControl(PluginType::ExternalEngine,
				FB_EXTERNAL_ENGINE_VERSION, upInfo, name.c_str());

			if (engineControl.hasData())
			{
				EngineAttachment key(NULL, NULL);
				AutoPtr<EngineAttachmentInfo> attInfo;

				try
				{
					Attachment::Checkout attCout(tdbb->getAttachment(), FB_FUNCTION);

					engine = engineControl.plugin();
					if (engine)
					{
						Attachment::SyncGuard attGuard(tdbb->getAttachment(), FB_FUNCTION);

						key = EngineAttachment(engine, tdbb->getAttachment());
						attInfo = FB_NEW(getPool()) EngineAttachmentInfo();
						attInfo->engine = engine;
						attInfo->context = FB_NEW(getPool()) ExternalContextImpl(tdbb, engine);

						setupAdminCharSet(tdbb, engine, attInfo);

						ContextManager<ExternalFunction> ctxManager(tdbb, attInfo, attInfo->adminCharSet);
						LocalStatus status;
						engine->openAttachment(&status, attInfo->context);	//// FIXME: log status
					}
				}
				catch (...)
				{
					if (engine)
					{
						PluginManagerInterfacePtr()->releasePlugin(engine);
					}

					throw;
				}

				if (engine)
				{
					engine->addRef();
					engines.put(name, engine);
					enginesAttachments.put(key, attInfo);
					attInfo.release();
				}
			}
		}
	}

	if (!engine)
	{
		status_exception::raise(Arg::Gds(isc_eem_engine_notfound) << name);
	}

	return engine;
}


ExtEngineManager::EngineAttachmentInfo* ExtEngineManager::getEngineAttachment(
	thread_db* tdbb, const MetaName& name)
{
	ExternalEngine* engine = getEngine(tdbb, name);
	return getEngineAttachment(tdbb, engine);
}


ExtEngineManager::EngineAttachmentInfo* ExtEngineManager::getEngineAttachment(
	thread_db* tdbb, ExternalEngine* engine, bool closing)
{
	EngineAttachment key(engine, tdbb->getAttachment());
	EngineAttachmentInfo* attInfo = NULL;

	ReadLockGuard readGuard(&enginesLock, FB_FUNCTION);

	if (!enginesAttachments.get(key, attInfo) && !closing)
	{
		readGuard.release();
		WriteLockGuard writeGuard(enginesLock, FB_FUNCTION);

		if (!enginesAttachments.get(key, attInfo))
		{
			attInfo = FB_NEW(getPool()) EngineAttachmentInfo();
			attInfo->engine = engine;
			attInfo->context = FB_NEW(getPool()) ExternalContextImpl(tdbb, engine);

			setupAdminCharSet(tdbb, engine, attInfo);

			enginesAttachments.put(key, attInfo);

			ContextManager<ExternalFunction> ctxManager(tdbb, attInfo, attInfo->adminCharSet);
			Attachment::Checkout attCout(tdbb->getAttachment(), FB_FUNCTION);
			LocalStatus status;
			engine->openAttachment(&status, attInfo->context);	//// FIXME: log status
		}

		return attInfo;
	}

	if (closing && attInfo)
	{
		readGuard.release();
		WriteLockGuard writeGuard(enginesLock, FB_FUNCTION);
		enginesAttachments.remove(key);
	}

	return attInfo;
}


void ExtEngineManager::setupAdminCharSet(thread_db* tdbb, ExternalEngine* engine,
	EngineAttachmentInfo* attInfo)
{
	ContextManager<ExternalFunction> ctxManager(tdbb, attInfo, CS_UTF8);

	Utf8 charSetName[MAX_SQL_IDENTIFIER_SIZE] = "NONE";

	LocalStatus status;
	engine->open(&status, attInfo->context, charSetName, MAX_SQL_IDENTIFIER_LEN);
	status.check();

	charSetName[MAX_SQL_IDENTIFIER_LEN] = '\0';

	if (!MET_get_char_coll_subtype(tdbb, &attInfo->adminCharSet,
			reinterpret_cast<const UCHAR*>(charSetName),
			strlen(charSetName)))
	{
		status_exception::raise(
			Arg::Gds(isc_charset_not_found) <<
			Arg::Str(charSetName));
	}
}


}	// namespace Jrd

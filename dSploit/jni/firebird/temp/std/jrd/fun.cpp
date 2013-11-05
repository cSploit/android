/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-V2.5.2.26540 Firebird 2.5 **********************/
/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		fun.epp
 *	DESCRIPTION:	External Function handling code.
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
 * 2001.9.18 Claudio Valderrama: Allow return parameter by descriptor
 * to signal NULL by testing the flags of the parameter's descriptor.
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2003.07.31 Fred Polizo, Jr. - Made FUN_evaluate() correctly determine
 * the length of string types containing binary data (char. set octets).
 * 2003.08.10 Claudio Valderrama: Fix SF Bugs #544132 and #728839.
 */

#include "firebird.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../common/config/config.h"
#include "../jrd/common.h"
#include "../jrd/jrd.h"
#include "../jrd/val.h"
#include "../jrd/exe.h"
#include "../jrd/ibase.h"
#include "../jrd/req.h"
#include "../jrd/lls.h"
#include "../jrd/blb.h"
#include "../jrd/flu.h"
#include "../jrd/common.h"
#include "../jrd/ibsetjmp.h"
#include "../jrd/irq.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dsc_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/flu_proto.h"
#include "../jrd/fun_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/thread_proto.h"
#include "../jrd/isc_s_proto.h"
#include "../jrd/blb.h"
#include "../common/classes/auto.h"
#include "../common/utils_proto.h"
#include "../common/classes/FpeControl.h"

#ifdef WIN_NT
#define LIBNAME "ib_util"
#else
#define LIBNAME "libib_util"
#include <dlfcn.h>	// dladdr
#endif

using namespace Jrd;
using namespace Firebird;


#ifndef BOOT_BUILD
namespace
{
	struct IbUtilStartup
	{
		IbUtilStartup(MemoryPool& p) : libUtilPath(p)
		{
#if defined(WIN_NT)
			libUtilPath.assign(LIBNAME);
#elif defined(DARWIN)
			libUtilPath = "/Library/Frameworks/Firebird.framework/Versions/A/Libraries/" LIBNAME;
#else
			const PathName id(Config::getInstallDirectory());
			PathUtils::concatPath(libUtilPath, id, "lib/" LIBNAME);
#endif // WIN_NT
		}

		PathName libUtilPath;
	};

	InitInstance<IbUtilStartup> ibUtilStartup;
	bool volatile initDone = false;

	bool tryLibrary(PathName libName, string& message)
	{
		ModuleLoader::doctorModuleExtention(libName);

		ModuleLoader::Module* module = ModuleLoader::loadModule(libName);
		if (!module)
		{
			message.printf("%s library has not been found", libName.c_str());
			return false;
		}

		void (*ibUtilUnit)(void* (*)(long));
		if (!module->findSymbol("ib_util_init", ibUtilUnit))
		{
			message.printf("ib_util_init not found in %s", libName.c_str());
			delete module;
			return false;
		}

		ibUtilUnit(IbUtil::alloc);
		initDone = true;

		return true;
	}
}
#endif


void IbUtil::initialize()
{
#ifndef BOOT_BUILD
	if (initDone)
		return;

	string message[4];		// To suppress logs when correct library is found

#ifdef WIN_NT
	// using bin folder
	if (tryLibrary(fb_utils::getPrefix(fb_utils::FB_DIR_BIN, LIBNAME), message[1]))
		return;

	// using firebird root (takes into account environment settings)
	if (tryLibrary(fb_utils::getPrefix(fb_utils::FB_DIR_CONF, LIBNAME), message[2]))
		return;
#else
	// using install directory
	if (tryLibrary(ibUtilStartup().libUtilPath, message[0]))
		return;

	// using firebird root (takes into an account environment settings)
	if (tryLibrary(fb_utils::getPrefix(fb_utils::FB_DIR_CONF, "lib/" LIBNAME), message[1]))
		return;

	// using libraries directory
	if (tryLibrary(fb_utils::getPrefix(fb_utils::FB_DIR_LIB, LIBNAME), message[2]))
		return;
#endif // WIN_NT

	// using default paths
	if (tryLibrary(LIBNAME, message[3]))
		return;

	// all failed - log error
	gds__log("ib_util init failed, UDFs can't be used - looks like firebird misconfigured\n"
			 "\t%s\n\t%s\n\t%s\n\t%s", message[0].c_str(), message[1].c_str(),
									   message[2].c_str(), message[3].c_str());
#endif	// !BOOT_BUILD
}

void* IbUtil::alloc(long size)
{
	thread_db* tdbb = JRD_get_thread_data();

	void* const ptr = tdbb->getDefaultPool()->allocate(size);

	if (ptr)
		tdbb->getAttachment()->att_udf_pointers.add(ptr);

	return ptr;
}

bool IbUtil::free(void* ptr)
{
	if (!ptr)
		return true;

	thread_db* tdbb = JRD_get_thread_data();
	Attachment* attachment = tdbb->getAttachment();

	size_t pos;
	if (attachment->att_udf_pointers.find(ptr, pos))
	{
		attachment->att_udf_pointers.remove(pos);
		tdbb->getDefaultPool()->deallocate(ptr);
		return true;
	}

	return false;
}


typedef void* UDF_ARG;

template <typename T>
T CALL_UDF(Database* dbb, int (*entrypoint)(), UDF_ARG* args)
{
	Database::Checkout dcoHolder(dbb);
	return ((T (*)(UDF_ARG, UDF_ARG, UDF_ARG, UDF_ARG, UDF_ARG, UDF_ARG, UDF_ARG, UDF_ARG, UDF_ARG, UDF_ARG))(entrypoint)) (args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
}

/*DATABASE DB = FILENAME "ODS.RDB";*/
static const UCHAR	jrd_0 [155] =
   {	// blr string 

4,2,4,1,8,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,
0,12,0,2,7,'C',1,'K',15,0,0,'G',47,24,0,0,0,25,0,0,0,'F',1,'H',
24,0,1,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,0,7,0,25,1,
1,0,1,24,0,6,0,25,1,2,0,1,24,0,5,0,25,1,3,0,1,24,0,4,0,25,1,4,
0,1,24,0,3,0,25,1,5,0,1,24,0,2,0,25,1,6,0,1,24,0,1,0,25,1,7,0,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_12 [135] =
   {	// blr string 

4,2,4,1,6,0,41,0,0,0,1,41,0,0,32,0,41,3,0,32,0,7,0,7,0,7,0,4,
0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',14,0,0,'G',47,24,0,0,0,25,
0,0,0,255,14,1,2,1,24,0,4,0,25,1,0,0,1,24,0,5,0,25,1,1,0,1,24,
0,0,0,25,1,2,0,1,21,8,0,1,0,0,0,25,1,3,0,1,24,0,1,0,25,1,4,0,
1,24,0,6,0,25,1,5,0,255,14,1,1,21,8,0,0,0,0,0,25,1,3,0,255,255,
76
   };	// end of blr string 


#define EXCEPTION_MESSAGE "The user defined function: \t%s\n\t   referencing entrypoint: \t%s\n\t                in module: \t%s\n\tcaused the fatal exception:"

/* Blob passing structure */
/* Updated definitions from the static functions shown below */

struct udf_blob
{
	SSHORT (*blob_get_segment) (blb*, UCHAR*, USHORT, USHORT*);
	void* blob_handle;
	SLONG blob_number_segments;
	SLONG blob_max_segment;
	SLONG blob_total_length;
	void (*blob_put_segment) (blb*, const UCHAR*, USHORT);
	SLONG (*blob_seek) (blb*, USHORT, SLONG);
};


class OwnedBlobStack : public Stack<blb*>
{
public:
	explicit OwnedBlobStack(thread_db* in_tdbb) :
		m_blob_created(0), m_tdbb(in_tdbb)
	{}
	~OwnedBlobStack();
	void close();
	void setBlobCreated(blb* b)
	{
		m_blob_created = b;
	}
private:
	blb* m_blob_created;
	thread_db* m_tdbb;
};

OwnedBlobStack::~OwnedBlobStack()
{
	while (this->hasData())
	{
		// We want to close blobs opened for reading
		// and cancel blobs opened for writing.
		blb* aux = this->pop();
		try
		{
			if (aux != m_blob_created)
				BLB_close(m_tdbb, aux);
			else
				BLB_cancel(m_tdbb, aux);
		}
		catch (const Exception&)
		{
			// Ignore exception.
		}
	}
}

void OwnedBlobStack::close()
{
	while (this->hasData())
	{
		// This strange code is to ensure that read blobs that cannot be closed
		// normally are ignored by the destructor, but if the created (write)
		// blob cannot be closed normally, it will be cancelled by the destructor
		// of OwnedBlobStack.

		blb* aux = this->object();
		if (aux != m_blob_created)
		{
			this->pop();
			BLB_close(m_tdbb, aux);
		}
		else
		{
			BLB_close(m_tdbb, aux);
			this->pop();
		}
	}
}


enum UdfError
{
	UeNone,
	UeUnsupDtype,
	UeMove,
	UeDealloc
};


static SSHORT blob_get_segment(blb*, UCHAR*, USHORT, USHORT*);
static void blob_put_segment(blb*, const UCHAR*, USHORT);
static SLONG blob_lseek(blb*, USHORT, SLONG);
static SLONG get_scalar_array(fun_repeat*, DSC*, scalar_array_desc*, UCharStack&);
static void invoke(thread_db* tdbb,
				   UserFunction* function,
				   fun_repeat* return_ptr,
				   impure_value* value,
				   UDF_ARG* args,
				   const udf_blob* const return_blob_struct,
				   bool& result_is_null,
				   UdfError& udfError);
static bool private_move(Jrd::thread_db* tdbb, dsc* from, dsc* to);


void FUN_evaluate(thread_db* tdbb, UserFunction* function, jrd_nod* node, impure_value* value)
{
/**************************************
 *
 *	F U N _ e v a l u a t e
 *
 **************************************
 *
 * Functional description
 *	Evaluate a function.
 *
 **************************************/
#ifndef BOOT_BUILD
	// We may be in danger of AV in UDF if ib_util init failed
	if (!initDone)
	{
		ERR_post(Arg::Gds(isc_random) << "ib_util init failed - UDF usage disabled");
	}
#endif

	if (!function->fun_entrypoint)
	{
		Firebird::status_exception::raise(Arg::Gds(isc_funnotdef) <<
										  Arg::Str(function->fun_name) <<
										  Arg::Gds(isc_modnotfound));
	}

	UDF_ARG args[MAX_UDF_ARGUMENTS + 1];
	HalfStaticArray<UCHAR, 800> temp;

	SET_TDBB(tdbb);

	// Start by constructing argument list
	UCHAR* temp_ptr = temp.getBuffer(function->fun_temp_length + FB_DOUBLE_ALIGN);
	MOVE_CLEAR(temp_ptr, temp.getCount());
	temp_ptr = (UCHAR*) FB_ALIGN((U_IPTR) temp_ptr, FB_DOUBLE_ALIGN);

	MOVE_CLEAR(args, sizeof(args));
	UDF_ARG* arg_ptr = args;
	//Stack<blb*> blob_stack;
	//blb* blob_created = 0;
	OwnedBlobStack blob_stack(tdbb);
	UCharStack array_stack;

	jrd_req* request = tdbb->getRequest();
	// CVC: restoring the null flag seems like a Borland hack to try to
	// patch a bug with null handling. There's no evident reason to restore it
	// because EVL_expr() resets it every time it's called.	Kept it for now.
	const bool null_flag	= ((request->req_flags & req_null) == req_null);

	fun_repeat*	return_ptr = function->fun_rpt + function->fun_return_arg;
	jrd_nod** ptr = node->nod_arg;

	value->vlu_desc = return_ptr->fun_desc;
	value->vlu_desc.dsc_address = (UCHAR *) & value->vlu_misc;

// Trap any potential errors

	try {

	// If the return data type is any of the string types,
	// allocate space to hold value

	if (value->vlu_desc.dsc_dtype <= dtype_varying)
	{
		const USHORT ret_length = value->vlu_desc.dsc_length;
		VaryingString* string = value->vlu_string;
		if (string && string->str_length < ret_length)
		{
			delete string;
			string = NULL;
		}
		if (!string)
		{
			string = FB_NEW_RPT(*tdbb->getDefaultPool(), ret_length) VaryingString;
			string->str_length = ret_length;
			value->vlu_string = string;
		}
		value->vlu_desc.dsc_address = string->str_data;
	}

	// We'll use to this trick to give the UDF a way to signal
	// "I sent a null blob" when not using descriptors.
	udf_blob* return_blob_struct = 0;

	DSC temp_desc;
	double  d;

	// Process arguments
	const fun_repeat* const end = function->fun_rpt + 1 + function->fun_count;
	for (fun_repeat* tail = function->fun_rpt + 1; tail < end; ++tail)
	{
		DSC* input;
		if (tail == return_ptr)
		{
			input = &value->vlu_desc; // (DSC*) value;
			// CVC: The return param we build for the UDF is not null!!!
			// This closes SF Bug #544132.
			request->req_flags &= ~req_null;
		}
		else
		{
			input = EVL_expr(tdbb, *ptr++);
		}

		// If we're passing data type ISC descriptor, there's
		// nothing left to be done

		if (tail->fun_mechanism == FUN_descriptor)
		{
			// CVC: We have to protect the UDF from Borland's ill null signaling
			// See EVL_expr(...), case nod_field for reference: the request may be
			// signaling NULL, but the placeholder field created by EVL_field(...)
			// doesn't carry the null flag in the descriptor. Why Borland didn't
			// set on such flag is maybe because it only has local meaning.
			// This closes SF Bug #728839.
			if ((request->req_flags & req_null) && !(input && (input->dsc_flags & DSC_null)))
			{
			    *arg_ptr++ = NULL;
			}
			else
			{
			    *arg_ptr++ = input;
			}
			continue;
		}

		temp_desc = tail->fun_desc;
		temp_desc.dsc_address = temp_ptr;
		// CVC: There's a theoretical possibility of overflowing "length" here.
		USHORT length = FB_ALIGN(temp_desc.dsc_length, FB_DOUBLE_ALIGN);

		// If we've got a null argument, just pass zeros (got any better ideas?)

		if (!input || (request->req_flags & req_null))
		{
			if (tail->fun_mechanism == FUN_value)
			{
				UCHAR* p = (UCHAR *) arg_ptr;
				MOVE_CLEAR(p, (SLONG) length);
				p += length;
				arg_ptr = reinterpret_cast<UDF_ARG*>(p);
				continue;
			}

			if (tail->fun_desc.dsc_dtype == dtype_blob)
			{
				length = sizeof(udf_blob);
			}

			if (tail->fun_mechanism != FUN_ref_with_null)
				MOVE_CLEAR(temp_ptr, (SLONG) length);
			else
			{
				// Probably for arrays and blobs it's better to preserve the
				// current behavior that sends a zeroed memory chunk.
				switch (tail->fun_desc.dsc_dtype)
				{
				case dtype_quad:
				case dtype_array:
				case dtype_blob:
					MOVE_CLEAR(temp_ptr, (SLONG) length);
					break;
				default: // FUN_ref_with_null, non-blob, non-array: we send null pointer.
					*arg_ptr++ = 0;
					continue;
				}
			}
		}
		else if (tail->fun_mechanism == FUN_scalar_array)
		{
			length = get_scalar_array(tail, input, (scalar_array_desc*)temp_ptr, array_stack);
		}
		else
		{
			SLONG l;
			SLONG* lp;
			switch (tail->fun_desc.dsc_dtype)
			{
			case dtype_short:
				{
					const SSHORT s = MOV_get_long(input, (SSHORT) tail->fun_desc.dsc_scale);
					if (tail->fun_mechanism == FUN_value)
					{
						// For (apparent) portability reasons, SHORT by value
						// is always passed as a LONG.  See v3.2 release notes
						// Passing by value is not supported in SQL due to
						// these problems, but can still occur in GDML.
						// 1994-September-28 David Schnepper

						*arg_ptr++ = (UDF_ARG)(IPTR) s;
						continue;
					}

					SSHORT* sp = (SSHORT *) temp_ptr;
					*sp = s;
				}
				break;

			case dtype_long:
				l = MOV_get_long(input, (SSHORT) tail->fun_desc.dsc_scale);
				if (tail->fun_mechanism == FUN_value)
				{
					*arg_ptr++ = (UDF_ARG)(IPTR)l;
					continue;
				}
				lp = (SLONG *) temp_ptr;
				*lp = l;
				break;

			case dtype_sql_time:
				l = MOV_get_sql_time(input);
				if (tail->fun_mechanism == FUN_value)
				{
					*arg_ptr++ = (UDF_ARG)(IPTR) l;
					continue;
				}
				lp = (SLONG *) temp_ptr;
				*lp = l;
				break;

			case dtype_sql_date:
				l = MOV_get_sql_date(input);
				if (tail->fun_mechanism == FUN_value)
				{
					*arg_ptr++ = (UDF_ARG)(IPTR) l;
					continue;
				}
				lp = (SLONG *) temp_ptr;
				*lp = l;
				break;

			case dtype_int64:
				{
					SINT64* pi64;
					const SINT64 i64 = MOV_get_int64(input, (SSHORT) tail->fun_desc.dsc_scale);

					if (tail->fun_mechanism == FUN_value)
					{
						pi64 = (SINT64 *) arg_ptr;
						*pi64++ = i64;
						arg_ptr = reinterpret_cast<UDF_ARG*>(pi64);
						continue;
					}
					pi64 = (SINT64 *) temp_ptr;
					*pi64 = i64;
				}
				break;

			case dtype_real:
				{
					const float f = (float) MOV_get_double(input);
					if (tail->fun_mechanism == FUN_value)
					{
						// For (apparent) portability reasons, FLOAT by value
						// is always passed as a DOUBLE.  See v3.2 release notes
						// Passing by value is not supported in SQL due to
						// these problems, but can still occur in GDML.
						// 1994-September-28 David Schnepper

						double* dp = (double *) arg_ptr;
						*dp++ = (double) f;
						arg_ptr = reinterpret_cast<UDF_ARG*>(dp);
						continue;
					}
					float* fp = (float *) temp_ptr;
					*fp = f;
				}
				break;

			case dtype_double:
				d = MOV_get_double(input);
				double* dp;
				if (tail->fun_mechanism == FUN_value)
				{
					dp = (double *) arg_ptr;
					*dp++ = d;
					arg_ptr = reinterpret_cast<UDF_ARG*>(dp);
					continue;
				}
				dp = (double *) temp_ptr;
				*dp = d;
				break;

			case dtype_text:
			case dtype_dbkey:
			case dtype_cstring:
			case dtype_varying:
				if (tail == return_ptr)
				{
					//temp_ptr = value->vlu_desc.dsc_address;
					//length = 0;
					*arg_ptr++ = value->vlu_desc.dsc_address;
					continue;
				}

				MOV_move(tdbb, input, &temp_desc);
				break;

			// CVC: There's no other solution for now: timestamp can't be returned
			//		by value and the other way is to force the user to pass a dummy value as
			//		an argument to keep the engine happy. So, here's the hack.
			case dtype_timestamp:
			    if (tail == return_ptr)
				{
					//temp_ptr = value->vlu_desc.dsc_address;
					//length = sizeof(GDS_TIMESTAMP);
					*arg_ptr++ = value->vlu_desc.dsc_address;
					continue;
				}

				MOV_move(tdbb, input, &temp_desc);
				break;

			case dtype_quad:
			case dtype_array:
				MOV_move(tdbb, input, &temp_desc);
				break;

			case dtype_blob:
				{
					// This is not a descriptor pointing to a blob. This is a blob struct.
					udf_blob* blob_desc = (udf_blob*) temp_ptr;
					blb* blob;
					length = sizeof(udf_blob);
					if (tail == return_ptr)
					{
						blob = BLB_create(tdbb, tdbb->getRequest()->req_transaction,
										  (bid*) &value->vlu_misc);
						return_blob_struct = blob_desc;
						blob_stack.setBlobCreated(blob);
					}
					else
					{
						bid blob_id;
						if (request->req_flags & req_null)
						{
							memset(&blob_id, 0, sizeof(bid));
						}
						else
						{
							if (input->dsc_dtype != dtype_quad && input->dsc_dtype != dtype_blob)
							{
								ERR_post(Arg::Gds(isc_wish_list) <<
										 Arg::Gds(isc_blobnotsup) << Arg::Str("conversion"));
							}
							blob_id = *(bid*) input->dsc_address;
						}
						blob = BLB_open(tdbb, tdbb->getRequest()->req_transaction, &blob_id);
					}
					blob_stack.push(blob);
					blob_desc->blob_get_segment = blob_get_segment;
					blob_desc->blob_put_segment = blob_put_segment;
					blob_desc->blob_seek = blob_lseek;
					blob_desc->blob_handle = blob;
					blob_desc->blob_number_segments = blob->blb_count;
					blob_desc->blob_max_segment = blob->blb_max_segment;
					blob_desc->blob_total_length = blob->blb_length;
				}
				break;

			default:
				fb_assert(FALSE);
				MOV_move(tdbb, input, &temp_desc);
				break;

			}
		}

		*arg_ptr++ = temp_ptr;
		temp_ptr += length;
	} // for

	// Did the udf manage to signal null in some way?
	// We acknowledge null in three cases:
	// a) rc_ptr = udf(); returns a null pointer -> result_was_null becomes true
	// b) Udf used RETURNS PARAMETER <n> for a descriptor whose DSC_null flag is activated
	// c) Udf used RETURNS PARAMETER <n> for a blob and made the blob handle null,
	// because there's no current way to do that. Notice that it doesn't affect
	// the engine internals, since blob_struct is a mere wrapper around the blob.
	// Udfs work in the assumption that they ignore that the handle is the real
	// internal blob and this has been always the tale.
	bool result_was_null = false;

	// Did the udf send an unknown data type or one we can't handle?
	// Did the udf mismanage memory marked with FREE_IT,
	// that should be deallocated by the engine?
	// Did the udf send an ill-formed descriptor back?
	UdfError udfError = UeNone;

	invoke(tdbb, function, return_ptr, value, args, return_blob_struct, result_was_null, udfError);

	switch (udfError)
	{
	case UeUnsupDtype:
		IBERROR(169);			// msg 169 return data type not supported
		break;
	case UeMove:
		//CVT_conversion_error(&desc, ERR_post);
		ERR_punt(); // The error is already in the thread's tdbb_status_vector
		break;
	case UeDealloc:
		status_exception::raise(Arg::Gds(isc_bad_udf_freeit));
		break;
	}

	if (result_was_null)
	{
		request->req_flags |= req_null;
		value->vlu_desc.dsc_flags |= DSC_null; // redundant, but be safe
	}
	else
	{
		if (value->vlu_desc.dsc_dtype == dtype_double)
		{
			if (isinf(value->vlu_misc.vlu_double))
			{
				status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_udf_fp_overflow) << Arg::Str(function->fun_name));
			}
			else if (isnan(value->vlu_misc.vlu_double))
			{
				status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_udf_fp_nan) << Arg::Str(function->fun_name));
			}
		}
		request->req_flags &= ~req_null;
	}

	while (array_stack.hasData())
	{
		delete[] array_stack.pop();
	}
	blob_stack.close();

	} // try
	catch (const Exception&)
	{
		while (array_stack.hasData()) {
			delete[] array_stack.pop();
		}

		throw;
	}

	if (null_flag)
	{
		request->req_flags |= req_null;
	}
}


UserFunction* FUN_lookup_function(thread_db* tdbb, const MetaName& name) //, bool ShowAccessError)
{
   struct {
          SSHORT jrd_4;	// gds__utility 
          SSHORT jrd_5;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_6;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_7;	// RDB$FIELD_LENGTH 
          SSHORT jrd_8;	// RDB$FIELD_SCALE 
          SSHORT jrd_9;	// RDB$FIELD_TYPE 
          SSHORT jrd_10;	// RDB$MECHANISM 
          SSHORT jrd_11;	// RDB$ARGUMENT_POSITION 
   } jrd_3;
   struct {
          TEXT  jrd_2 [32];	// RDB$FUNCTION_NAME 
   } jrd_1;
   struct {
          TEXT  jrd_16 [256];	// RDB$MODULE_NAME 
          TEXT  jrd_17 [32];	// RDB$ENTRYPOINT 
          TEXT  jrd_18 [32];	// RDB$FUNCTION_NAME 
          SSHORT jrd_19;	// gds__utility 
          SSHORT jrd_20;	// RDB$FUNCTION_TYPE 
          SSHORT jrd_21;	// RDB$RETURN_ARGUMENT 
   } jrd_15;
   struct {
          TEXT  jrd_14 [32];	// RDB$FUNCTION_NAME 
   } jrd_13;
/**************************************
 *
 *	F U N _ l o o k u p _ f u n c t i o n
 *
 **************************************
 *
 * Functional description
 *	Lookup function by name.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	UserFunction* function = NULL;
	if (dbb->dbb_functions.get(name, function))
		return function;

	fun_repeat temp[MAX_UDF_ARGUMENTS + 1];
	UserFunction* prior = NULL;

	jrd_req* request_fun = CMP_find_request(tdbb, irq_l_functions, IRQ_REQUESTS);
	jrd_req* request_arg = CMP_find_request(tdbb, irq_l_args, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request_fun)
		X IN RDB$FUNCTIONS
		WITH X.RDB$FUNCTION_NAME EQ name.c_str()*/
	{
	if (!request_fun)
	   request_fun = CMP_compile2 (tdbb, (UCHAR*) jrd_12, sizeof(jrd_12), true);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_13.jrd_14, 32);
	EXE_start (tdbb, request_fun, dbb->dbb_sys_trans);
	EXE_send (tdbb, request_fun, 0, 32, (UCHAR*) &jrd_13);
	while (1)
	   {
	   EXE_receive (tdbb, request_fun, 1, 326, (UCHAR*) &jrd_15);
	   if (!jrd_15.jrd_19) break;

		if (!REQUEST(irq_l_functions))
			REQUEST(irq_l_functions) = request_fun;

		USHORT count = 0;
		USHORT args = 0;

		MOVE_CLEAR(temp, (SLONG) sizeof(temp));
		ULONG length = 0;

		/*FOR(REQUEST_HANDLE request_arg)
			Y IN RDB$FUNCTION_ARGUMENTS
			WITH Y.RDB$FUNCTION_NAME EQ X.RDB$FUNCTION_NAME
			SORTED BY Y.RDB$ARGUMENT_POSITION*/
		{
		if (!request_arg)
		   request_arg = CMP_compile2 (tdbb, (UCHAR*) jrd_0, sizeof(jrd_0), true);
		gds__vtov ((const char*) jrd_15.jrd_18, (char*) jrd_1.jrd_2, 32);
		EXE_start (tdbb, request_arg, dbb->dbb_sys_trans);
		EXE_send (tdbb, request_arg, 0, 32, (UCHAR*) &jrd_1);
		while (1)
		   {
		   EXE_receive (tdbb, request_arg, 1, 16, (UCHAR*) &jrd_3);
		   if (!jrd_3.jrd_4) break;

			if (!REQUEST(irq_l_args))
				REQUEST(irq_l_args) = request_arg;

			fun_repeat* tail = temp + /*Y.RDB$ARGUMENT_POSITION*/
						  jrd_3.jrd_11;
			tail->fun_mechanism = (FUN_T) /*Y.RDB$MECHANISM*/
						      jrd_3.jrd_10;
			count = MAX(count, /*Y.RDB$ARGUMENT_POSITION*/
					   jrd_3.jrd_11);
			DSC_make_descriptor(&tail->fun_desc, /*Y.RDB$FIELD_TYPE*/
							     jrd_3.jrd_9,
								/*Y.RDB$FIELD_SCALE*/
								jrd_3.jrd_8, /*Y.RDB$FIELD_LENGTH*/
  jrd_3.jrd_7,
								/*Y.RDB$FIELD_SUB_TYPE*/
								jrd_3.jrd_6, /*Y.RDB$CHARACTER_SET_ID*/
  jrd_3.jrd_5,
								0);

			if (tail->fun_desc.dsc_dtype == dtype_cstring)
				tail->fun_desc.dsc_length++;

			if (/*Y.RDB$ARGUMENT_POSITION*/
			    jrd_3.jrd_11 != /*X.RDB$RETURN_ARGUMENT*/
    jrd_15.jrd_21)
				++args;

			USHORT l = FB_ALIGN(tail->fun_desc.dsc_length, FB_DOUBLE_ALIGN);

			if (tail->fun_desc.dsc_dtype == dtype_blob)
				l = sizeof(udf_blob);

			length += l;
        /*END_FOR;*/
	   }
	}

		function = FB_NEW_RPT(*dbb->dbb_permanent, count + 1) UserFunction(*dbb->dbb_permanent);
		function->fun_name = /*X.RDB$FUNCTION_NAME*/
				     jrd_15.jrd_18;
		function->fun_count = count;
		function->fun_args = args;
		function->fun_return_arg = /*X.RDB$RETURN_ARGUMENT*/
					   jrd_15.jrd_21;
		function->fun_type = /*X.RDB$FUNCTION_TYPE*/
				     jrd_15.jrd_20;
		function->fun_temp_length = length;
		memcpy(function->fun_rpt, temp, (count + 1) * sizeof(fun_repeat));

		// Prepare the exception message to be used in case this function ever
		// causes an exception.  This is done at this time to save us from preparing
		// (thus allocating) this message every time the function is called.
		function->fun_exception_message.printf(EXCEPTION_MESSAGE, name.c_str(),
			/*X.RDB$ENTRYPOINT*/
			jrd_15.jrd_17, /*X.RDB$MODULE_NAME*/
  jrd_15.jrd_16);

		function->fun_entrypoint =
			Module::lookup(/*X.RDB$MODULE_NAME*/
				       jrd_15.jrd_16, /*X.RDB$ENTRYPOINT*/
  jrd_15.jrd_17, dbb->dbb_modules);

		// Could not find a function with given MODULE, ENTRYPOINT.
		// Try the list of internally implemented functions.
		if (!function->fun_entrypoint)
		{
			function->fun_entrypoint = BUILTIN_entrypoint(/*X.RDB$MODULE_NAME*/
								      jrd_15.jrd_16, /*X.RDB$ENTRYPOINT*/
  jrd_15.jrd_17);
		}

		// ASF: All the places handling fun_homonym is something that doesn't work really.
		// Our current system table rdb$function_arguments doesn't support overloaded
		// functions as it has a rdb$function_name column instead of an ID.
		if (prior)
		{
			function->fun_homonym = prior->fun_homonym;
			prior->fun_homonym = function;
		}
		else
		{
			prior = function;
			dbb->dbb_functions.put(name, function);
		}
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_l_functions))
		REQUEST(irq_l_functions) = request_fun;
	if (!REQUEST(irq_l_args))
		REQUEST(irq_l_args) = request_arg;

	return prior;
}


UserFunction* FUN_resolve(thread_db* tdbb, CompilerScratch* csb, UserFunction* function, jrd_nod* args)
{
/**************************************
 *
 *	F U N _ r e s o l v e
 *
 **************************************
 *
 * Functional description
 *	Resolve instance of potentially overloaded function.
 *
 **************************************/
	DSC arg;

	SET_TDBB(tdbb);

	UserFunction* best = NULL;
	int best_score = 0;
	const jrd_nod* const* const end = args->nod_arg + args->nod_count;

	for (; function; function = function->fun_homonym)
	{
		if (function->fun_entrypoint && function->fun_args == args->nod_count)
		{
			int score = 0;
			jrd_nod** ptr;
			const fun_repeat* tail;

			for (ptr = args->nod_arg, tail = function->fun_rpt + 1; ptr < end; ptr++, tail++)
			{
				CMP_get_desc(tdbb, csb, *ptr, &arg);
				if (tail->fun_mechanism == FUN_descriptor)
					score += 10;
				else if (tail->fun_desc.dsc_dtype == dtype_blob || arg.dsc_dtype == dtype_blob)
				{
					score = 0;
					break;
				}
				else if (tail->fun_desc.dsc_dtype >= arg.dsc_dtype)
					score += 10 - (arg.dsc_dtype - tail->fun_desc.dsc_dtype);
				else
					score += 1;
			}

			if (!best || score > best_score)
			{
				best_score = score;
				best = function;
			}
		}
	}

	return best;
}


static SLONG blob_lseek(blb* blob, USHORT mode, SLONG offset)
{
/**************************************
 *
 *	b l o b _ l s e e k
 *
 **************************************
 *
 * Functional description
 *	lseek a blob segement.  Return the offset
 *
 **************************************/
	// As this is a call-back from a UDF, must reacquire the engine mutex
	thread_db* tdbb = JRD_get_thread_data();
	Database::SyncGuard dsGuard(tdbb->getDatabase());

	return BLB_lseek(blob, mode, offset);
}


static void blob_put_segment(blb* blob, const UCHAR* buffer, USHORT length)
{
/**************************************
 *
 *	b l o b _ p u t _ s e g m e n t
 *
 **************************************
 *
 * Functional description
 *	Put segment into a blob.  Return nothing
 *
 **************************************/
	// As this is a call-back from a UDF, must reacquire the engine mutex
	thread_db* tdbb = JRD_get_thread_data();
	Database::SyncGuard dsGuard(tdbb->getDatabase());

	BLB_put_segment(tdbb, blob, buffer, length);
}


static SSHORT blob_get_segment(blb* blob, UCHAR* buffer, USHORT length, USHORT* return_length)
{
/**************************************
 *
 *	b l o b _ g e t _ s e g m e n t
 *
 **************************************
 *
 * Functional description
 *	Get next segment of a blob.  Return the following:
 *
 *		1	-- Complete segment has been returned.
 *		0	-- End of blob (no data returned).
 *		-1	-- Current segment is incomplete.
 *
 **************************************/
	// As this is a call-back from a UDF, must reacquire the engine mutex
	thread_db* tdbb = JRD_get_thread_data();
	Database::SyncGuard dsGuard(tdbb->getDatabase());

	*return_length = BLB_get_segment(tdbb, blob, buffer, length);

	if (blob->blb_flags & BLB_eof)
		return 0;

	if (blob->blb_fragment_size)
		return -1;

	return 1;
}


static SLONG get_scalar_array(fun_repeat*	arg,
							  DSC*	value,
							  scalar_array_desc*	scalar_desc,
							  UCharStack&	stack)
{
/**************************************
 *
 *	g e t _ s c a l a r _ a r r a y
 *
 **************************************
 *
 * Functional description
 *	Get and format a scalar array descriptor, then allocate space
 *	and fetch the array.  If conversion is required, convert.
 *	Return length of array desc.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	// Get first the array descriptor, then the array

	SLONG stuff[IAD_LEN(16) / 4];
	Ods::InternalArrayDesc* array_desc = (Ods::InternalArrayDesc*) stuff;
	blb* blob = BLB_get_array(tdbb, tdbb->getRequest()->req_transaction, (bid*)value->dsc_address,
							  array_desc);

	UCHAR* data = FB_NEW(*getDefaultMemoryPool()) UCHAR[array_desc->iad_total_length];
	BLB_get_data(tdbb, blob, data, array_desc->iad_total_length);
	const USHORT dimensions = array_desc->iad_dimensions;

	// Convert array, if necessary

	dsc to = arg->fun_desc;
	dsc from = array_desc->iad_rpt[0].iad_desc;

	if (to.dsc_dtype != from.dsc_dtype ||
		to.dsc_scale != from.dsc_scale || to.dsc_length != from.dsc_length)
	{
		SLONG n = array_desc->iad_count;
		UCHAR* const temp = FB_NEW(*getDefaultMemoryPool()) UCHAR[(SLONG) to.dsc_length * n];
		to.dsc_address = temp;
		from.dsc_address = data;

		// This loop may call ERR_post indirectly.
		try
		{
			for (; n; --n, to.dsc_address += to.dsc_length,
				from.dsc_address += array_desc->iad_element_length)
			{
				MOV_move(tdbb, &from, &to);
			}
		}
		catch (const Exception&)
		{
			delete[] data;
			delete[] temp;
			throw;
		}

		delete[] data;
		data = temp;
	}

	// Fill out the scalar array descriptor

	stack.push(data);
	scalar_desc->sad_desc = arg->fun_desc;
	scalar_desc->sad_desc.dsc_address = data;
	scalar_desc->sad_dimensions = dimensions;

	const Ods::InternalArrayDesc::iad_repeat* tail1 = array_desc->iad_rpt;
	scalar_array_desc::sad_repeat* tail2 = scalar_desc->sad_rpt;
	for (const scalar_array_desc::sad_repeat* const end = tail2 + dimensions; tail2 < end;
		++tail1, ++tail2)
	{
		tail2->sad_upper = tail1->iad_upper;
		tail2->sad_lower = tail1->iad_lower;
	}

	return sizeof(scalar_array_desc) + (dimensions - 1) * sizeof(scalar_array_desc::sad_repeat);
}


static void invoke(thread_db* tdbb,
				   UserFunction* function,
				   fun_repeat* return_ptr,
				   impure_value* value,
				   UDF_ARG* args,
				   const udf_blob* const return_blob_struct,
				   bool& result_is_null,
				   UdfError& udfError)
{
/**************************************
 *
 *	i n v o k e
 *
 **************************************
 *
 * Functional description
 *	Real UDF call moved to separate function in order to
 *	use CHECK_FOR_EXCEPTIONS macros without conflicts with destructors
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	START_CHECK_FOR_EXCEPTIONS(function->fun_exception_message.c_str());
	if (function->fun_return_arg)
	{
		CALL_UDF<void>(dbb, function->fun_entrypoint, args);
		result_is_null = return_ptr->fun_mechanism == FUN_descriptor &&
			(value->vlu_desc.dsc_flags & DSC_null) ||
			return_ptr->fun_mechanism == FUN_blob_struct && return_blob_struct &&
			!return_blob_struct->blob_handle;
	}
	else if (return_ptr->fun_mechanism == FUN_value)
	{
		result_is_null = false;

		switch (value->vlu_desc.dsc_dtype)
		{
		case dtype_sql_time:
		case dtype_sql_date:
		case dtype_long:
			value->vlu_misc.vlu_long = CALL_UDF<SLONG>(dbb, function->fun_entrypoint, args);
			break;

		case dtype_short:
			// For (apparent) portability reasons, SHORT by value
			// must always be returned as a LONG.  See v3.2 release notes
			// 1994-September-28 David Schnepper

			value->vlu_misc.vlu_short = (SSHORT) CALL_UDF<SLONG>(dbb, function->fun_entrypoint, args);
			break;

		case dtype_real:
			// For (apparent) portability reasons, FLOAT by value
			// must always be returned as a DOUBLE.  See v3.2 release notes
			// 1994-September-28 David Schnepper

			value->vlu_misc.vlu_float = (float) CALL_UDF<double>(dbb, function->fun_entrypoint, args);
			break;

		case dtype_int64:
			value->vlu_misc.vlu_int64 = CALL_UDF<SINT64>(dbb, function->fun_entrypoint, args);
			break;

		case dtype_double:
			value->vlu_misc.vlu_double = CALL_UDF<double>(dbb, function->fun_entrypoint, args);
			break;

		case dtype_timestamp:
		default:
			udfError = UeUnsupDtype;
			break;
		}
	}
	else
	{
		UCHAR* temp_ptr = CALL_UDF<UCHAR*>(dbb, function->fun_entrypoint, args);

		if (temp_ptr != NULL)
		{
			// CVC: Allow processing of return by descriptor.
			//		If the user did modify the return type, then we'll try to
			//		convert it to the declared return type of the UDF.
			dsc* return_dsc = 0;
			result_is_null = false;
			if ((FUN_T) abs(return_ptr->fun_mechanism) == FUN_descriptor)
			{
				// The formal param's type is contained in value->vlu_desc.dsc_dtype
				// but I want to know if the UDF changed it to a compatible type
				// from its returned descriptor, that will be return_dsc.
				return_dsc = reinterpret_cast<dsc*>(temp_ptr);
				temp_ptr = return_dsc->dsc_address;
				if (!temp_ptr || (return_dsc->dsc_flags & DSC_null))
					result_is_null = true;
			}

			const bool must_free = (SLONG) return_ptr->fun_mechanism < 0;

			if (result_is_null)
				; // nothing to do
			else
			if (return_dsc)
			{
				if (!private_move(tdbb, return_dsc, &value->vlu_desc))
					udfError = UeMove;
			}
			else
			{
				DSC temp_desc;

				switch (value->vlu_desc.dsc_dtype)
				{
				case dtype_sql_date:
				case dtype_sql_time:
					value->vlu_misc.vlu_long = *(SLONG *) temp_ptr;
					break;

				case dtype_long:
						value->vlu_misc.vlu_long = *(SLONG *) temp_ptr;
					break;

				case dtype_short:
					value->vlu_misc.vlu_short = *(SSHORT *) temp_ptr;
					break;

				case dtype_real:
					value->vlu_misc.vlu_float = *(float *) temp_ptr;
					break;

				case dtype_int64:
					value->vlu_misc.vlu_int64 = *(SINT64 *) temp_ptr;
					break;

				case dtype_double:
					value->vlu_misc.vlu_double = *(double *) temp_ptr;
					break;

				case dtype_text:
					temp_desc = value->vlu_desc;
					temp_desc.dsc_address = temp_ptr;
					if (!private_move(tdbb, &temp_desc, &value->vlu_desc))
						udfError = UeMove;
					break;

				case dtype_cstring:
					// For the ttype_binary char. set, this will truncate
					// the string after the first zero octet copied.
					temp_desc = value->vlu_desc;
					temp_desc.dsc_address = temp_ptr;
					temp_desc.dsc_length = strlen(reinterpret_cast<char*>(temp_ptr)) + 1;
					if (!private_move(tdbb, &temp_desc, &value->vlu_desc))
						udfError = UeMove;
					break;

				case dtype_varying:
					temp_desc = value->vlu_desc;
					temp_desc.dsc_address = temp_ptr;
					temp_desc.dsc_length = reinterpret_cast<vary*>(temp_ptr)->vary_length + sizeof(USHORT);
					if (!private_move(tdbb, &temp_desc, &value->vlu_desc))
						udfError = UeMove;
					break;

				case dtype_timestamp:
					{
						const SLONG* ip = (SLONG *) temp_ptr;
						value->vlu_misc.vlu_dbkey[0] = *ip++;
						value->vlu_misc.vlu_dbkey[1] = *ip;
					}
					break;

				default:
					udfError = UeUnsupDtype;
					break;
				}
			}

			// Check if this is function has the FREE_IT set, if set and
			// return_value is not null, then free the return value.
			if (must_free)
			{
				if (temp_ptr && !IbUtil::free(temp_ptr) && udfError == UeNone)
					udfError = UeDealloc;

				// CVC: Let's free the descriptor, too.
				if (return_dsc && !IbUtil::free(return_dsc) && udfError == UeNone)
					udfError = UeDealloc;
			}
		}
		else
			result_is_null = true;
	}
	END_CHECK_FOR_EXCEPTIONS(function->fun_exception_message.c_str());
}


static bool private_move(Jrd::thread_db* tdbb, dsc* from, dsc* to)
{
	SET_TDBB(tdbb);

	try
	{
		ThreadStatusGuard tempStatus(tdbb);
		MOV_move(tdbb, from, to);
		return true;
	}
	catch (Firebird::status_exception& e)
	{
		e.stuff_exception(tdbb->tdbb_status_vector);
		return false;
	}
}

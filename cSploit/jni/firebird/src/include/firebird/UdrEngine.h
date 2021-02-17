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

#ifndef FIREBIRD_UDR_H
#define FIREBIRD_UDR_H

#include "./ExternalEngine.h"

#ifndef FB_DLL_EXPORT
#if defined(WIN_NT)
#define FB_DLL_EXPORT __declspec(dllexport)
#elif defined(DARWIN)
#define FB_DLL_EXPORT API_ROUTINE
#else
#define FB_DLL_EXPORT
#endif // OS choice (WIN_NT, DARWIN)
#endif // FB_DLL_EXPORT


namespace Firebird
{
	namespace Udr
	{
//------------------------------------------------------------------------------


// Factory classes. They should be singletons instances created by user's modules and
// registered. When UDR engine is going to load a routine, it calls newItem.

class FunctionFactory
{
public:
	virtual void FB_CARG setup(IStatus* status, ExternalContext* context, const IRoutineMetadata* metadata,
		IMetadataBuilder* inBuilder, IMetadataBuilder* outBuilder) = 0;
	virtual ExternalFunction* FB_CARG newItem(IStatus* status, ExternalContext* context,
		const IRoutineMetadata* metadata) = 0;
};

class ProcedureFactory
{
public:
	virtual void FB_CARG setup(IStatus* status, ExternalContext* context, const IRoutineMetadata* metadata,
		IMetadataBuilder* inBuilder, IMetadataBuilder* outBuilder) = 0;
	virtual ExternalProcedure* FB_CARG newItem(IStatus* status, ExternalContext* context,
		const IRoutineMetadata* metadata) = 0;
};

class TriggerFactory
{
public:
	virtual void FB_CARG setup(IStatus* status, ExternalContext* context, const IRoutineMetadata* metadata,
		IMetadataBuilder* fieldsBuilder) = 0;
	virtual ExternalTrigger* FB_CARG newItem(IStatus* status, ExternalContext* context,
		const IRoutineMetadata* metadata) = 0;
};

// Routine registration functions.
extern "C" void FB_DLL_EXPORT fbUdrRegFunction(const char* name, FunctionFactory* factory);
extern "C" void FB_DLL_EXPORT fbUdrRegProcedure(const char* name, ProcedureFactory* factory);
extern "C" void FB_DLL_EXPORT fbUdrRegTrigger(const char* name, TriggerFactory* factory);


//------------------------------------------------------------------------------
	}	// namespace Udr
}	// namespace Firebird

#endif	// FIREBIRD_UDR_H

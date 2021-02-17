/*
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
 * Adriano dos Santos Fernandes
 */

#include "firebird.h"
#include "../jrd/Routine.h"
#include "../jrd/JrdStatement.h"
#include "../jrd/jrd.h"
#include "../jrd/exe.h"
#include "../common/StatusHolder.h"
#include "../jrd/lck_proto.h"
#include "../jrd/par_proto.h"

using namespace Firebird;


namespace Jrd {


// Create a MsgMetadata from a parameters array.
MsgMetadata* Routine::createMetadata(const Array<NestConst<Parameter> >& parameters)
{
	RefPtr<MsgMetadata> metadata(new MsgMetadata);

	for (Array<NestConst<Parameter> >::const_iterator i = parameters.begin();
		 i != parameters.end();
		 ++i)
	{
		metadata->addItem((*i)->prm_name, (*i)->prm_nullable, (*i)->prm_desc);
	}

	metadata->addRef();
	return metadata;
}

// Create a Format based on an IMessageMetadata.
Format* Routine::createFormat(MemoryPool& pool, IMessageMetadata* params, bool addEof)
{
	LocalStatus status;

	unsigned count = params->getCount(&status);
	status.check();

	Format* format = Format::newFormat(pool, count * 2 + (addEof ? 1 : 0));
	unsigned runOffset = 0;

	dsc* desc = format->fmt_desc.begin();

	for (unsigned i = 0; i < count; ++i)
	{
		unsigned descOffset, nullOffset, descDtype, descLength;

		runOffset = fb_utils::sqlTypeToDsc(runOffset, params->getType(&status, i),
			params->getLength(&status, i), &descDtype, &descLength,
			&descOffset, &nullOffset);
		status.check();

		desc->clear();
		desc->dsc_dtype = descDtype;
		desc->dsc_length = descLength;
		desc->dsc_scale = params->getScale(&status, i);
		desc->dsc_sub_type = params->getSubType(&status, i);
		desc->setTextType(params->getCharSet(&status, i));
		desc->dsc_address = (UCHAR*)(IPTR) descOffset;
		desc->dsc_flags = (params->isNullable(&status, i) ? DSC_nullable : 0);

		++desc;
		desc->makeShort(0, (SSHORT*)(IPTR) nullOffset);
		status.check();

		++desc;
	}

	if (addEof)
	{
		// Next item is aligned on USHORT, so as the previous one.
		desc->makeShort(0, (SSHORT*)(IPTR) runOffset);
		runOffset += sizeof(USHORT);
	}

	format->fmt_length = runOffset;

	return format;
}

// Parse routine BLR.
void Routine::parseBlr(thread_db* tdbb, CompilerScratch* csb, bid* blob_id)
{
	Jrd::Attachment* attachment = tdbb->getAttachment();

	UCharBuffer tmp;

	if (blob_id)
	{
		blb* blob = blb::open(tdbb, attachment->getSysTransaction(), blob_id);
		ULONG length = blob->blb_length + 10;
		UCHAR* temp = tmp.getBuffer(length);
		length = blob->BLB_get_data(tdbb, temp, length);
		tmp.resize(length);
	}

	parseMessages(tdbb, csb, BlrReader(tmp.begin(), (unsigned) tmp.getCount()));

	JrdStatement* statement = getStatement();
	PAR_blr(tdbb, NULL, tmp.begin(), (ULONG) tmp.getCount(), NULL, &csb, &statement, false, 0);
	setStatement(statement);

	if (!blob_id)
		setImplemented(false);
}

// Parse the messages of a blr request. For specified message, allocate a format (Format) block.
void Routine::parseMessages(thread_db* tdbb, CompilerScratch* csb, BlrReader blrReader)
{
	if (blrReader.getLength() < 2)
		status_exception::raise(Arg::Gds(isc_metadata_corrupt));

	csb->csb_blr_reader = blrReader;

	const SSHORT version = csb->csb_blr_reader.getByte();

	switch (version)
	{
		case blr_version4:
		case blr_version5:
		//case blr_version6:
			break;

		default:
			status_exception::raise(
				Arg::Gds(isc_metadata_corrupt) <<
				Arg::Gds(isc_wroblrver2) << Arg::Num(blr_version4) << Arg::Num(blr_version5/*6*/) <<
					Arg::Num(version));
	}

	if (csb->csb_blr_reader.getByte() != blr_begin)
		status_exception::raise(Arg::Gds(isc_metadata_corrupt));

	while (csb->csb_blr_reader.getByte() == blr_message)
	{
		const USHORT msgNumber = csb->csb_blr_reader.getByte();
		USHORT count = csb->csb_blr_reader.getWord();
		Format* format = Format::newFormat(*tdbb->getDefaultPool(), count);

		USHORT padField;
		const bool shouldPad = csb->csb_message_pad.get(msgNumber, padField);

		USHORT maxAlignment = 0;
		ULONG offset = 0;
		USHORT i = 0;

		for (Format::fmt_desc_iterator desc = format->fmt_desc.begin(); i < count; ++i, ++desc)
		{
			const USHORT align = PAR_desc(tdbb, csb, &*desc);
			if (align)
				offset = FB_ALIGN(offset, align);

			desc->dsc_address = (UCHAR*)(IPTR) offset;
			offset += desc->dsc_length;

			maxAlignment = MAX(maxAlignment, align);

			if (maxAlignment && shouldPad && i + 1 == padField)
				offset = FB_ALIGN(offset, maxAlignment);
		}

		format->fmt_length = offset;

		switch (msgNumber)
		{
			case 0:
				setInputFormat(format);
				break;

			case 1:
				setOutputFormat(format);
				break;

			default:
				delete format;
		}
	}
}

// Decrement the routine's use count.
void Routine::release(thread_db* tdbb)
{
	// Actually, it's possible for routines to have intermixed dependencies, so
	// this routine can be called for the routine which is being freed itself.
	// Hence we should just silently ignore such a situation.

	if (!useCount)
		return;

	if (intUseCount > 0)
		intUseCount--;

	--useCount;

#ifdef DEBUG_PROCS
	{
		string buffer;
		buffer.printf(
			"Called from CMP_decrement():\n\t Decrementing use count of %s\n",
			getName().toString().c_str());
		JRD_print_procedure_info(tdbb, buffer.c_str());
	}
#endif

	// Call recursively if and only if the use count is zero AND the routine
	// in the cache is different than this routine.
	// The routine will be different than in the cache only if it is a
	// floating copy, i.e. an old copy or a deleted routine.
	if (useCount == 0 && !checkCache(tdbb))
	{
		if (getStatement())
			releaseStatement(tdbb);

		flags &= ~Routine::FLAG_BEING_ALTERED;
		remove(tdbb);
	}
}

void Routine::releaseStatement(thread_db* tdbb)
{
	if (getStatement())
	{
		getStatement()->release(tdbb);
		setStatement(NULL);
	}

	setInputFormat(NULL);
	setOutputFormat(NULL);

	flags &= ~FLAG_SCANNED;
}

// Remove a routine from cache.
void Routine::remove(thread_db* tdbb)
{
	SET_TDBB(tdbb);
	Jrd::Attachment* att = tdbb->getAttachment();

	// MET_procedure locks it. Lets unlock it now to avoid troubles later
	if (existenceLock)
		LCK_release(tdbb, existenceLock);

	// Routine that is being altered may have references
	// to it by other routines via pointer to current meta
	// data structure, so don't lose the structure or the pointer.
	if (checkCache(tdbb) && !(flags & Routine::FLAG_BEING_ALTERED))
		clearCache(tdbb);

	// deallocate all structure which were allocated.  The routine
	// blr is originally read into a new pool from which all request
	// are allocated.  That will not be freed up.

	if (existenceLock)
	{
		delete existenceLock;
		existenceLock = NULL;
	}

	// deallocate input param structures

	for (Array<NestConst<Parameter> >::iterator i = getInputFields().begin();
		 i != getInputFields().end(); ++i)
	{
		if (*i)
			delete i->getObject();
	}

	getInputFields().clear();

	// deallocate output param structures

	for (Array<NestConst<Parameter> >::iterator i = getOutputFields().begin();
		 i != getOutputFields().end(); ++i)
	{
		if (*i)
			delete i->getObject();
	}

	getOutputFields().clear();

	if (!useCount)
		releaseFormat();

	if (!(flags & Routine::FLAG_BEING_ALTERED) && useCount == 0)
		delete this;
	else
	{
		// Fully clear routine block. Some pieces of code check for empty
		// routine name and ID, this is why we do it.
		setName(QualifiedName());
		setSecurityName("");
		setId(0);
		setDefaultCount(0);
	}
}


bool jrd_prc::checkCache(thread_db* tdbb) const
{
	return tdbb->getAttachment()->att_procedures[getId()] == this;
}

void jrd_prc::clearCache(thread_db* tdbb)
{
	tdbb->getAttachment()->att_procedures[getId()] = NULL;
}


}	// namespace Jrd

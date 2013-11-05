/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		msg_encode.h
 *	DESCRIPTION:	Message encoding definition file
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
 */
#ifndef MSG_ENCODE_H
#define MSG_ENCODE_H

const ISC_STATUS ISC_MASK	= 0x14000000;	/* Defines the code as a valid ISC code */
const ISC_STATUS FAC_MASK	= 0x00FF0000;	/* Specifies the facility where the code is located */
const ISC_STATUS CODE_MASK	= 0x0000FFFF;	/* Specifies the code in the message file */
const ISC_STATUS CLASS_MASK	= 0xF0000000;	/* Defines the code as warning, error, info, or other */

/* The following definitions can be used to specify the context in
 * which a status code is used.
 */
//#define CLASS_ERROR		0L		/* Code represents an error */
//#define CLASS_WARNING		1L	/* Code represents a warning */
//#define CLASS_INFO		2L		/* Code represents an information msg */

//#define MAKE_ERROR(code)	(code | (CLASS_ERROR & 0x3L) << 30)
//#define MAKE_WARNING(code)	(code | (CLASS_WARNING & 0x3L) << 30)
//#define MAKE_INFO(code)		(code | (CLASS_INFO & 0x3L) << 30)

/* The procedure for encoding an error message is as follows:
 * Be sure to update gds.c::gds__decode if this calculation changes
 * since gds__decode returns the error code, facility, and error type
 * from a given error message */

inline ISC_STATUS ENCODE_ISC_MSG(ISC_STATUS code, USHORT facility)
{
	return ((ISC_STATUS(facility & 0x1F) << 16) | (code & 0x3FFF) | ISC_MASK);
}

inline USHORT GET_FACILITY(ISC_STATUS code)
{
	return (code & FAC_MASK) >> 16;
}

inline USHORT GET_CLASS(ISC_STATUS code)
{
	return (code & CLASS_MASK) >> 30;
}

inline ISC_STATUS GET_CODE(ISC_STATUS code)
{
	return (code & CODE_MASK);
}

#endif // MSG_ENCODE_H

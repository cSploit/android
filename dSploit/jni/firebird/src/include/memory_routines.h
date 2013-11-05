/*
 *  PROGRAM:  Client/Server Common Code
 *  MODULE:   memory_routines.h
 *  DESCRIPTION:  Misc memory content routines
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.firebirdsql.org/index.php?op=doc&id=ipl
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
 * Created by: Dimitry Sibiryakov <aafemt@users.sourceforge.net>
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#ifndef MEMORY_ROUTINES_H
#define MEMORY_ROUTINES_H


template <typename T>
inline void copy_toptr(void* to, const T from)
{
#ifndef I386
	memcpy(to, &from, sizeof(T));
#else
	*((T*) to) = from;
#endif
}

template <typename T>
inline void copy_fromptr(T& to, const void* from)
{
#ifndef I386
	memcpy(&to, from, sizeof(T));
#else
	to = *(T*) from;
#endif
}




inline USHORT get_short(const UCHAR* p)
{
/**************************************
 *
 *      g e t _ s h o r t
 *
 **************************************
 *
 * Functional description
 *    Gather one unsigned short int
 *    from two chars
 *
 **************************************/
#ifndef WORDS_BIGENDIAN
	// little-endian
	USHORT temp;
	memcpy(&temp, p, sizeof(USHORT));
	return temp;
#else
	// big-endian
	union
	{
		USHORT n;
		UCHAR c[2];
	} temp;

	temp.c[0] = p[0];
	temp.c[1] = p[1];

	return temp.n;
#endif
}

inline SLONG get_long(const UCHAR* p)
{
/**************************************
 *
 *      g e t _ l o n g
 *
 **************************************
 *
 * Functional description
 *    Gather one signed long int
 *    from four chars
 *
 **************************************/
#ifndef WORDS_BIGENDIAN
  // little-endian
	SLONG temp;
	memcpy(&temp, p, sizeof(SLONG));
	return temp;
#else
	// big-endian
	union
	{
		SLONG n;
		UCHAR c[4];
	} temp;

	temp.c[0] = p[0];
	temp.c[1] = p[1];
	temp.c[2] = p[2];
	temp.c[3] = p[3];

	return temp.n;
#endif
}

inline void put_short(UCHAR* p, USHORT value)
{
/**************************************
 *
 *      p u t _ s h o r t
 *
 **************************************
 *
 * Functional description
 *    Store one unsigned short int as
 *    two chars
 *
 **************************************/
#ifndef WORDS_BIGENDIAN
	// little-endian
	memcpy(p, &value, sizeof(USHORT));
#else
  // big-endian
	union
	{
		USHORT n;
		UCHAR c[2];
	} temp;

	temp.n = value;

	p[0] = temp.c[0];
	p[1] = temp.c[1];
#endif
}

inline void put_long(UCHAR* p, SLONG value)
{
/**************************************
 *
 *      p u t _ l o n g
 *
 **************************************
 *
 * Functional description
 *    Store one signed long int as
 *    four chars
 *
 **************************************/
#ifndef WORDS_BIGENDIAN
	// little-endian
	memcpy(p, &value, sizeof(SLONG));
#else
	// big-endian
	union
	{
		SLONG n;
		UCHAR c[4];
	} temp;

	temp.n = value;

	p[0] = temp.c[0];
	p[1] = temp.c[1];
	p[2] = temp.c[2];
	p[3] = temp.c[3];
#endif
}

inline void put_vax_short(UCHAR* p, SSHORT value)
{
/**************************************
 *
 *      p u t _ v a x _ s h o r t
 *
 **************************************
 *
 * Functional description
 *    Store one signed short int as
 *    two chars in VAX format
 *
 **************************************/
#ifndef WORDS_BIGENDIAN
	// little-endian
	memcpy(p, &value, sizeof(value));
#else
	// big-endian
	union
	{
		SSHORT n;
		UCHAR c[2];
	} temp;

	temp.n = value;

	p[0] = temp.c[1];
	p[1] = temp.c[0];
#endif
}

inline void put_vax_long(UCHAR* p, SLONG value)
{
/**************************************
 *
 *      p u t _ v a x _ l o n g
 *
 **************************************
 *
 * Functional description
 *    Store one signed long int as
 *    four chars in VAX format
 *
 **************************************/
#ifndef WORDS_BIGENDIAN
	// little-endian
	memcpy(p, &value, sizeof(SLONG));
#else
	// big-endian
	union
	{
		SLONG n;
		UCHAR c[4];
	} temp;

	temp.n = value;

	p[0] = temp.c[3];
	p[1] = temp.c[2];
	p[2] = temp.c[1];
	p[3] = temp.c[0];
#endif
}

inline void put_vax_int64(UCHAR* p, SINT64 value)
{
/**************************************
 *
 *      p u t _ v a x _ i n t 6 4
 *
 **************************************
 *
 * Functional description
 *    Store one signed long long int as
 *    eight chars in VAX format
 *
 **************************************/
#ifndef WORDS_BIGENDIAN
	// little-endian
	memcpy(p, &value, sizeof(SINT64));
#else
	// big-endian
	union
	{
		SINT64 n;
		UCHAR c[8];
	} temp;

	temp.n = value;

	p[0] = temp.c[7];
	p[1] = temp.c[6];
	p[2] = temp.c[5];
	p[3] = temp.c[4];
	p[4] = temp.c[3];
	p[5] = temp.c[2];
	p[6] = temp.c[1];
	p[7] = temp.c[0];
#endif
}

#endif // MEMORY_ROUTINES_H

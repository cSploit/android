/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		DbImplementation.h
 *	DESCRIPTION:	Database implementation
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
 *  The Original Code was created by Alexander Peshkoff
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Alexander Peshkoff <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "../common/classes/fb_string.h"

namespace Ods {

struct header_page;

}

namespace Firebird {

class DbImplementation
{
public:
	explicit DbImplementation(const Ods::header_page* h);
	DbImplementation (UCHAR p_cpu, UCHAR p_os, UCHAR p_cc, UCHAR p_flags)
		: di_cpu(p_cpu), di_os(p_os), di_cc(p_cc), di_flags(p_flags)
	{ }
	DbImplementation (UCHAR p_compatImpl);
	~DbImplementation() { }

private:
	UCHAR di_cpu, di_os, di_cc, di_flags;

public:
	const char* cpu() const;
	const char* os() const;
	const char* cc() const;
	const char* endianess() const;
	string implementation() const;

	bool compatible(const DbImplementation& v) const;
	void store(Ods::header_page* h) const;
	void stuff(UCHAR** info) const;
	static DbImplementation pick(const UCHAR* info);
	UCHAR backwardCompatibleImplementation() const;
	static DbImplementation fromBackwardCompatibleByte(UCHAR bcImpl);

	static const DbImplementation current;
};

} //namespace Firebird

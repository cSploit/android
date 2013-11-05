/*
 *	PROGRAM:		Firebird exceptions classes
 *	MODULE:			StatusHolder.h
 *	DESCRIPTION:	Firebird's exception classes
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
 *  The Original Code was created by Vlad Khorsun
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2007 Vlad Khorsun <hvlad at users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef FB_STATUS_HOLDER
#define FB_STATUS_HOLDER


namespace Firebird {

class StatusHolder
{
public:
	explicit StatusHolder(const ISC_STATUS* status = NULL)
	{
		memset(m_status_vector, 0, sizeof(m_status_vector));
		m_raised = false;

		if (status)
			save(status);
	}

	~StatusHolder()
	{ clear(); }

	ISC_STATUS save(const ISC_STATUS* status);
	void clear();
	void raise();

	ISC_STATUS getError()
	{
		if (m_raised) {
			clear();
		}
		return m_status_vector[1];
	}

	const ISC_STATUS* value()
	{
		if (m_raised) {
			clear();
		}
		return m_status_vector;
	}

private:
	ISC_STATUS_ARRAY m_status_vector;
	bool m_raised;
};


} // namespace Firebird


#endif // FB_STATUS_HOLDER

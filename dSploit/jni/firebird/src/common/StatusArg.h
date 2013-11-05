/*
 *	PROGRAM:		Firebird exceptions classes
 *	MODULE:			StatusArg.h
 *	DESCRIPTION:	Build status vector with variable number of elements
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
 *  The Original Code was created by Alex Peshkov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2008 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef FB_STATUS_ARG
#define FB_STATUS_ARG

namespace Firebird {

class AbstractString;
class MetaName;

namespace Arg {

// forward
class Warning;
class StatusVector;

class Base
{
#ifdef __HP_aCC
// aCC gives error, cannot access protected member class ImplBase
public:
#else
protected:
#endif
	class ImplBase
	{
	private:
		ISC_STATUS kind, code;

	public:
		ISC_STATUS getKind() const throw() { return kind; }
		ISC_STATUS getCode() const throw() { return code; }

		virtual const ISC_STATUS* value() const throw() { return NULL; }
		virtual int length() const throw() { return 0; }
		virtual int firstWarning() const throw() { return 0; }
		virtual bool hasData() const throw() { return false; }
		virtual void clear() throw() { }
		virtual void append(const StatusVector&) throw() { }
		virtual ISC_STATUS copyTo(ISC_STATUS*) const throw() { return 0; }

		virtual void shiftLeft(const Base&) throw() { }
		virtual void shiftLeft(const Warning&) throw() { }
		virtual void shiftLeft(const char*) throw() { }
		virtual void shiftLeft(const AbstractString&) throw() { }
		virtual void shiftLeft(const MetaName&) throw() { }

		virtual bool compare(const StatusVector& v) const throw() { return false; }

		ImplBase(ISC_STATUS k, ISC_STATUS c) throw() : kind(k), code(c) { }
		virtual ~ImplBase() { }
	};

	Base(ISC_STATUS k, ISC_STATUS c);// : implementation(new ImplBase(k, c)) { }
	explicit Base(ImplBase* i) throw() : implementation(i) { }
	~Base() { delete implementation; }

	ImplBase* const implementation;

public:
	ISC_STATUS getKind() const throw() { return implementation->getKind(); }
	ISC_STATUS getCode() const throw() { return implementation->getCode(); }
};

class StatusVector : public Base
{
protected:
	class ImplStatusVector : public ImplBase
	{
	private:
		ISC_STATUS_ARRAY m_status_vector;
		int m_length, m_warning;

		bool appendErrors(const ImplBase* const v) throw();
		bool appendWarnings(const ImplBase* const v) throw();
		bool append(const ISC_STATUS* const from, const int count) throw();

	public:
		virtual const ISC_STATUS* value() const throw() { return m_status_vector; }
		virtual int length() const throw() { return m_length; }
		virtual int firstWarning() const throw() { return m_warning; }
		virtual bool hasData() const throw() { return m_length > 0; }
		virtual void clear() throw();
		virtual void append(const StatusVector& v) throw();
		virtual ISC_STATUS copyTo(ISC_STATUS* dest) const throw();
		virtual void shiftLeft(const Base& arg) throw();
		virtual void shiftLeft(const Warning& arg) throw();
		virtual void shiftLeft(const char* text) throw();
		virtual void shiftLeft(const AbstractString& text) throw();
		virtual void shiftLeft(const MetaName& text) throw();

		virtual bool compare(const StatusVector& v) const throw();

		ImplStatusVector(ISC_STATUS k, ISC_STATUS c) throw() : ImplBase(k, c)
		{
			clear();
		}

		explicit ImplStatusVector(const ISC_STATUS* s) throw();
	};

	StatusVector(ISC_STATUS k, ISC_STATUS v);

public:
	explicit StatusVector(const ISC_STATUS* s);
	StatusVector();
	~StatusVector() { }

	const ISC_STATUS* value() const throw() { return implementation->value(); }
	int length() const throw() { return implementation->length(); }
	bool hasData() const throw() { return implementation->hasData(); }

	void clear() throw() { implementation->clear(); }
	void append(const StatusVector& v) throw() { implementation->append(v); }
	void raise() const;
	ISC_STATUS copyTo(ISC_STATUS* dest) const throw() { return implementation->copyTo(dest); }

	// generic argument insert
	StatusVector& operator<<(const Base& arg) throw()
	{
		implementation->shiftLeft(arg);
		return *this;
	}

	// warning special case - to setup first warning location
	StatusVector& operator<<(const Warning& arg) throw()
	{
		implementation->shiftLeft(arg);
		return *this;
	}

	// Str special case - make the code simpler & better readable
	StatusVector& operator<<(const char* text) throw()
	{
		implementation->shiftLeft(text);
		return *this;
	}

	StatusVector& operator<<(const AbstractString& text) throw()
	{
		implementation->shiftLeft(text);
		return *this;
	}

	StatusVector& operator<<(const MetaName& text) throw()
	{
		implementation->shiftLeft(text);
		return *this;
	}

	bool operator==(const StatusVector& arg) const throw()
	{
		return implementation->compare(arg);
	}

	bool operator!=(const StatusVector& arg) const throw()
	{
		return !(*this == arg);
	}

private:
};


class Gds : public StatusVector
{
public:
	explicit Gds(ISC_STATUS s) throw();
};

class Str : public Base
{
public:
	explicit Str(const char* text) throw();
	explicit Str(const AbstractString& text) throw();
	explicit Str(const MetaName& text) throw();
};

class Num : public Base
{
public:
	explicit Num(ISC_STATUS s) throw();
};

class Interpreted : public StatusVector
{
public:
	explicit Interpreted(const char* text) throw();
	explicit Interpreted(const AbstractString& text) throw();
};

class Unix : public Base
{
public:
	explicit Unix(ISC_STATUS s) throw();
};

class Mach : public Base
{
public:
	explicit Mach(ISC_STATUS s) throw();
};

class Windows : public Base
{
public:
	explicit Windows(ISC_STATUS s) throw();
};

class Warning : public StatusVector
{
public:
	explicit Warning(ISC_STATUS s) throw();
};

class SqlState : public Base
{
public:
	explicit SqlState(const char* text) throw();
	explicit SqlState(const AbstractString& text) throw();
};

class OsError : public Base
{
public:
	OsError() throw();
};

} // namespace Arg

} // namespace Firebird


#endif // FB_STATUS_ARG

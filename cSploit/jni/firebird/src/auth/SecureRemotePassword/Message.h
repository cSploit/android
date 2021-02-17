#ifdef INCLUDE_Firebird_H		// Internal build
#define INTERNAL_FIREBIRD
#endif

#include "firebird/Provider.h"

#ifdef INTERNAL_FIREBIRD

#include "../common/classes/alloc.h"
#include "../common/StatusHolder.h"
#include "../common/classes/ImplementHelper.h"

#else // INTERNAL_FIREBIRD

#include <assert.h>
#define fb_assert(x) assert(x)
#include <string.h>

#endif // INTERNAL_FIREBIRD

#ifdef INTERNAL_FIREBIRD
// This class helps to work with metadata iface
class Meta : public Firebird::RefPtr<Firebird::IMessageMetadata>
{
public:
	Meta(Firebird::IStatement* stmt, bool out)
	{
		Firebird::LocalStatus st;
		Firebird::IMessageMetadata* m = out ? stmt->getOutputMetadata(&st) : stmt->getInputMetadata(&st);
		if (!st.isSuccess())
		{
			Firebird::status_exception::raise(st.get());
		}
		assignRefNoIncr(m);
	}
};
#endif // INTERNAL_FIREBIRD


// Linked list of various fields
class FieldLink
{
public:
	virtual ~FieldLink() { }
	virtual void linkWithMessage(const unsigned char* buf) = 0;

	FieldLink* next;
};


// This class helps to exchange values with a message
class Message
// : public AutoStorage
{
public:
	Message(Firebird::IMessageMetadata* aMeta = NULL)
		: metadata(NULL), buffer(NULL), builder(NULL),
		  fieldCount(0), fieldList(NULL)
	{
#ifdef INTERNAL_FIREBIRD
		s = &st;
#else
		s = fb_get_master_interface()->getStatus();
#endif

		try
		{
			if (aMeta)
			{
				createBuffer(aMeta);
				metadata = aMeta;
				metadata->addRef();
			}
			else
			{
				Firebird::IMetadataBuilder* bld =
#ifdef INTERNAL_FIREBIRD
					Firebird::MasterInterfacePtr()->
#else
					fb_get_master_interface()->
#endif
						getMetadataBuilder(s, 0);
				check(s);
				builder = bld;
				builder->addRef();
			}
		}
		catch(...)
		{
			s->dispose();
			throw;
		}
	}

	~Message()
	{
		delete buffer;
#ifndef INTERNAL_FIREBIRD
		s->dispose();
#endif
		if (builder)
			builder->release();
		if (metadata)
			metadata->release();
	}

public:
	template <typename T>
	static bool checkType(unsigned t, unsigned /*sz*/)
	{
		return T::unknownDataType();
	}

	template <typename T>
	static unsigned getType(unsigned& sz)
	{
		return T::SQL_UnknownDataType;
	}

	template <typename T>
	unsigned add(unsigned& t, unsigned& sz, FieldLink* lnk)
	{
		if (metadata)
		{
			unsigned l = metadata->getCount(s);
			check(s);
			if (fieldCount >= l)
			{
#ifdef INTERNAL_FIREBIRD
				(Firebird::Arg::Gds(isc_random) <<
					"Attempt to add to the message more variables than possible").raise();
#else
				fatalErrorHandler("Attempt to add to the message more variables than possible");
#endif
			}

			t = metadata->getType(s, fieldCount);
			check(s);
			sz = metadata->getLength(s, fieldCount);
			check(s);
			if (!checkType<T>(t, sz))
			{
#ifdef INTERNAL_FIREBIRD
				(Firebird::Arg::Gds(isc_random) << "Incompatible data type").raise();
#else
				fatalErrorHandler("Incompatible data type");
#endif
			}
		}
		else
		{
			fb_assert(builder);

			unsigned f = builder->addField(s);
			check(s);

			fb_assert(f == fieldCount);

			t = getType<T>(sz);
			builder->setType(s, f, t);
			check(s);
			builder->setLength(s, f, sz);
			check(s);

			lnk->next = fieldList;
			fieldList = lnk;
		}

		return fieldCount++;
	}

	static void check(Firebird::IStatus* status)
	{
		if (!status->isSuccess())
		{
#ifdef INTERNAL_FIREBIRD
			Firebird::status_exception::raise(status->get());
#else
			char s[100];
			const ISC_STATUS* st = status->get();
			fb_interpret(s, sizeof(s), &st);
			fatalErrorHandler(s);
#endif
		}
	}

	// Attention!
	// No addRef/release interface here!
	// Lifetime is equal at least to Message lifetime
	Firebird::IMessageMetadata* getMetadata()
	{
		if (!metadata)
		{
			fb_assert(builder);
			Firebird::IMessageMetadata* aMeta = builder->getMetadata(s);
			check(s);
			metadata = aMeta;
			metadata->addRef();
			builder->release();
			builder = NULL;
		}

		return metadata;
	}

	bool hasMetadata() const
	{
		return metadata ? true : false;
	}

	// access to message's data buffer
	unsigned char* getBuffer()
	{
		if (!buffer)
		{
			getMetadata();

			createBuffer(metadata);
			while(fieldList)
			{
				fieldList->linkWithMessage(buffer);
				fieldList = fieldList->next;
			}
		}

		return buffer;
	}

private:
	void createBuffer(Firebird::IMessageMetadata* aMeta)
	{
		unsigned l = aMeta->getMessageLength(s);
		check(s);
		buffer = new unsigned char[l];
	}

public:
	Firebird::IStatus* s;

private:
#ifdef INTERNAL_FIREBIRD
	Firebird::LocalStatus st;
#endif
	Firebird::IMessageMetadata* metadata;
	unsigned char* buffer;
	Firebird::IMetadataBuilder* builder;
	unsigned fieldCount;
	FieldLink* fieldList;
};


template <typename T>
class Field : public FieldLink
{
public:
	class Null
	{
	public:
		explicit Null(Message* m)
			: msg(m), ptr(NULL)
		{ }

		void linkMessage(short* p)
		{
			ptr = p;
			*ptr = -1;	// mark as null initially
		}

		operator FB_BOOLEAN() const
		{
			msg->getBuffer();
			return (*ptr) ? FB_TRUE : FB_FALSE;
		}

		FB_BOOLEAN operator=(FB_BOOLEAN val)
		{
			msg->getBuffer();
			*ptr = val ? -1 : 0;
			return val;
		}

	private:
		Message* msg;
		short* ptr;
	};

	explicit Field(Message& m, unsigned sz = 0)
		: ptr(NULL), charBuffer(NULL), msg(&m), null(msg), ind(~0), type(0), size(sz)
	{
		ind = msg->add<T>(type, size, this);

		if (msg->hasMetadata())
			setPointers(msg->getBuffer());
	}

	~Field()
	{
		delete charBuffer;
	}

	operator T()
	{
		msg->getBuffer();
		return *ptr;
	}

	T* operator&()
	{
		msg->getBuffer();
		return ptr;
	}

	T* operator->()
	{
		msg->getBuffer();
		return ptr;
	}

	T operator= (T newVal)
	{
		msg->getBuffer();
		*ptr = newVal;
		null = FB_FALSE;
		return newVal;
	}

	operator const char*()
	{
		msg->getBuffer();

		if (!charBuffer)
		{
			charBuffer = new char[size + 1];
		}
		getStrValue(charBuffer);
		return charBuffer;
	}

	const char* operator= (const char* newVal)
	{
		msg->getBuffer();
		setStrValue(newVal, strnlen(newVal, size));
		null = FB_FALSE;
		return newVal;
	}

	void set(unsigned length, const void* newVal)
	{
		msg->getBuffer();
		setStrValue(newVal, length);
		null = FB_FALSE;
	}

private:
	void linkWithMessage(const unsigned char* buf)
	{
		setPointers(buf);
	}

	void setPointers(const unsigned char* buf)
	{
		unsigned tmp = msg->getMetadata()->getOffset(msg->s, ind);
		Message::check(msg->s);
		ptr = (T*) (buf + tmp);

		tmp = msg->getMetadata()->getNullOffset(msg->s, ind);
		Message::check(msg->s);
		null.linkMessage((short*) (buf + tmp));
	}

	void getStrValue(char* to)
	{
		T::incompatibleDataType();
	}

	void setStrValue(const void* from, unsigned len)
	{
		T::incompatibleDataType();
	}

	T* ptr;
	char* charBuffer;
	Message* msg;

public:
	Null null;

private:
	unsigned ind, type, size;
};


// ---------------------------------------------
struct Varying
{
	short len;
	char data[1];
};

template <>
inline bool Message::checkType<Varying>(unsigned t, unsigned /*sz*/)
{
	return t == SQL_VARYING;
}

template <>
inline unsigned Message::getType<Varying>(unsigned& sz)
{
	if (!sz)
		sz = 1;
	sz += sizeof(short);
	return SQL_VARYING;
}

template<>
inline void Field<Varying>::getStrValue(char* to)
{
	unsigned len = ptr->len;
	if (len > size)
		len = size;
	memcpy(to, ptr->data, len);
	to[len] = 0;
}

template<>
inline void Field<Varying>::setStrValue(const void* from, unsigned len)
{
	if (len > size)
		len = size;
	memcpy(ptr->data, from, len);
	ptr->len = len;
}

struct Text
{
	char data[1];
};

template <>
inline bool Message::checkType<Text>(unsigned t, unsigned /*sz*/)
{
	return t == SQL_TEXT;
}

template <>
inline unsigned Message::getType<Text>(unsigned& sz)
{
	if (!sz)
		sz = 1;
	return SQL_TEXT;
}

template<>
inline void Field<Text>::getStrValue(char* to)
{
	memcpy(to, ptr->data, size);
	to[size] = 0;
	unsigned len = size;
	while (len--)
	{
		if (to[len] == ' ')
			to[len] = 0;
		else
			break;
	}
}

template<>
inline void Field<Text>::setStrValue(const void* from, unsigned len)
{
	if (len > size)
		len = size;
	memcpy(ptr->data, from, len);
	if (len < size)
		memset(&ptr->data[len], ' ', size - len);
}

template <>
inline bool Message::checkType<ISC_SHORT>(unsigned t, unsigned sz)
{
	return t == SQL_SHORT && sz == sizeof(ISC_SHORT);
}

template <>
inline bool Message::checkType<ISC_LONG>(unsigned t, unsigned sz)
{
	return t == SQL_LONG && sz == sizeof(ISC_LONG);
}

template <>
inline bool Message::checkType<ISC_QUAD>(unsigned t, unsigned sz)
{
	return (t == SQL_BLOB || t == SQL_QUAD) && sz == sizeof(ISC_QUAD);
}

template <>
inline bool Message::checkType<ISC_INT64>(unsigned t, unsigned sz)
{
	return t == SQL_INT64 && sz == sizeof(ISC_INT64);
}

template <>
inline bool Message::checkType<FB_BOOLEAN>(unsigned t, unsigned sz)
{
	return t == SQL_BOOLEAN && sz == sizeof(FB_BOOLEAN);
}

template <>
inline unsigned Message::getType<ISC_SHORT>(unsigned& sz)
{
	sz = sizeof(ISC_SHORT);
	return SQL_SHORT;
}

template <>
inline unsigned Message::getType<ISC_LONG>(unsigned& sz)
{
	sz = sizeof(ISC_LONG);
	return SQL_LONG;
}

template <>
inline unsigned Message::getType<ISC_QUAD>(unsigned& sz)
{
	sz = sizeof(ISC_QUAD);
	return SQL_BLOB;
}

template <>
inline unsigned Message::getType<ISC_INT64>(unsigned& sz)
{
	sz = sizeof(ISC_INT64);
	return SQL_INT64;
}

template <>
inline unsigned Message::getType<FB_BOOLEAN>(unsigned& sz)
{
	sz = sizeof(FB_BOOLEAN);
	return SQL_BOOLEAN;
}

/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		Hash.h
 *	DESCRIPTION:	Hash of objects
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
 *
 */

#ifndef CLASSES_HASH_H
#define CLASSES_HASH_H

#include "../common/classes/vector.h"

namespace Firebird
{
	template <typename K>
	class DefaultHash
	{
	public:
		static size_t hash(const void* value, size_t length, size_t hashSize)
		{
			size_t sum = 0;
			size_t val;

			const char* data = static_cast<const char*>(value);

			while (length >= sizeof(size_t))
			{
				memcpy(&val, data, sizeof(size_t));
				sum += val;
				data += sizeof(size_t);
				length -= sizeof(size_t);
			}

			if (length)
			{
				val = 0;
				memcpy(&val, data, length);
				sum += val;
			}

			size_t rc = 0;
			while (sum)
			{
				rc += (sum % hashSize);
				sum /= hashSize;
			}

			return rc % hashSize;
		}

		static size_t hash(const K& value, size_t hashSize)
		{
			return hash(&value, sizeof value, hashSize);
		}

		const static size_t DEFAULT_SIZE = 97;		// largest prime number < 100
	};

	template <typename C,
			  size_t HASHSIZE = DefaultHash<C>::DEFAULT_SIZE,
			  typename K = C,						// default key
			  typename KeyOfValue = DefaultKeyValue<C>,	// default keygen
			  typename F = DefaultHash<K> >			// hash function definition
	class Hash
	{
	public:
		// This class is supposed to be used as a BASE class for class to be hashed
		class Entry
		{
		private:
			Entry** previousElement;
			Entry* nextElement;

		public:
			Entry() : previousElement(NULL) { }

			virtual ~Entry()
			{
				unLink();
			}

			void link(Entry** where)
			{
				unLink();

				// set our pointers
				previousElement = where;
				nextElement = previousElement ? *previousElement : NULL;

				// make next element (if present) to point to us
				if (nextElement)
				{
					nextElement->previousElement = &nextElement;
				}

				// make previous element point to us
				*previousElement = this;
			}

			void unLink()
			{
				// if we are linked
				if (previousElement)
				{
					if (nextElement)
					{
						// adjust previous pointer in next element ...
						nextElement->previousElement = previousElement;
					}
					// ... and next pointer in previous element
					*previousElement = nextElement;
					// finally mark ourselves not linked
					previousElement = NULL;
				}
			}

			Entry** nextPtr()
			{
				return &nextElement;
			}

			Entry* next() const
			{
				return nextElement;
			}

			virtual bool isEqual(const K&) const = 0;
			virtual C* get() = 0;
		}; // class Entry

	private:
		Hash(const Hash&);	// not implemented

	public:
		explicit Hash(MemoryPool&)
		{
			memset(data, 0, sizeof data);
		}

		Hash()
		{
			memset(data, 0, sizeof data);
		}

		~Hash()
		{
			for (size_t n = 0; n < HASHSIZE; ++n)
			{
				while (data[n])
				{
					 data[n]->unLink();
				}
			}
		}

	private:
		Entry* data[HASHSIZE];

		Entry** locate(const K& key, size_t h)
		{
			Entry** pointer = &data[h];
			while (*pointer)
			{
				if ((*pointer)->isEqual(key))
				{
					break;
				}
				pointer = (*pointer)->nextPtr();
			}

			return pointer;
		}

		Entry** locate(const K& key)
		{
			size_t hashValue = F::hash(key, HASHSIZE);
			fb_assert(hashValue < HASHSIZE);
			return locate(key, hashValue % HASHSIZE);
		}

	public:
		bool add(C* value)
		{
			Entry** e = locate(KeyOfValue::generate(this, *value));
			if (*e)
			{
				return false;	// sorry, duplicate
			}
			value->link(e);
			return true;
		}

		C* lookup(const K& key)
		{
			Entry** e = locate(key);
			return (*e) ? (*e)->get() : NULL;
		}

		C* remove(const K& key)
		{
			Entry** e = locate(key);
			if (*e)
			{
				Entry* entry = *e;
				entry->unLink();
				return entry->get();
			}
			return NULL;
		}

	private:
		// disable use of default operator=
		Hash& operator= (const Hash&);

	public:
		class iterator
		{
		private:
			const Hash* hash;
			size_t elem;
			Entry* current;

			iterator(const iterator& i);
			iterator& operator= (const iterator& i);

			void next()
			{
				while (!current)
				{
					if (++elem >= HASHSIZE)
					{
						break;
					}
					current = hash->data[elem];
				}
			}

		public:
			explicit iterator(const Hash& h)
				: hash(&h), elem(0), current(hash->data[elem])
			{
				next();
			}

			iterator& operator++()
			{
				if (hasData())
				{
					current = current->next();
					next();
				}
				return *this;
			}

			bool hasData() const
			{
				return current ? true : false;
			}

			operator C*() const
			{
				fb_assert(hasData());
				return current->get();
			}

			C* operator ->() const
			{
				fb_assert(hasData());
				return current->get();
			}

			bool operator== (const iterator& h) const
			{
				return (hash == h.hash) && (elem == h.elem) && (current == h.current);
			}

			bool operator!= (const iterator& h) const
			{
				return !(*this == h);
			}
		}; // class iterator
	}; // class Hash

} // namespace Firebird

#endif // CLASSES_HASH_H


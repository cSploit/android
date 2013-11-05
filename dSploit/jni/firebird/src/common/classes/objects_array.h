/*
 *	PROGRAM:	Common class definition
 *	MODULE:		object_array.h
 *	DESCRIPTION:	half-static array of any objects,
 *			having MemoryPool'ed constructor.
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
 *  Copyright (c) 2004 Alexander Peshkoff <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef CLASSES_OBJECTS_ARRAY_H
#define CLASSES_OBJECTS_ARRAY_H

#include "../common/classes/alloc.h"
#include "../common/classes/array.h"

namespace Firebird
{
	template <typename T, typename A = Array<T*, InlineStorage<T*, 8> > >
	class ObjectsArray : protected A
	{
	private:
		typedef A inherited;
	public:
		class const_iterator; // fwd decl.

		class iterator
		{
			friend class ObjectsArray<T, A>;
			friend class const_iterator;
		private:
			ObjectsArray *lst;
			size_t pos;
			iterator(ObjectsArray *l, size_t p) : lst(l), pos(p) { }
		public:
			iterator() : lst(0), pos(0) { }
			/*
			iterator& operator=(ObjectsArray& a)
			{
				lst = &a;
				pos = 0;
			}
			*/
			iterator& operator++()
			{
				++pos;
				return (*this);
			}
			iterator operator++(int)
			{
				iterator tmp = *this;
				++pos;
				 return tmp;
			}
			iterator& operator--()
			{
				fb_assert(pos > 0);
				--pos;
				return (*this);
			}
			iterator operator--(int)
			{
				fb_assert(pos > 0);
				iterator tmp = *this;
				--pos;
				 return tmp;
			}
			T* operator->()
			{
				fb_assert(lst);
				T* pointer = lst->getPointer(pos);
				return pointer;
			}
			T& operator*()
			{
				fb_assert(lst);
				T* pointer = lst->getPointer(pos);
				return *pointer;
			}
			bool operator!=(const iterator& v) const
			{
				fb_assert(lst == v.lst);
				return lst ? pos != v.pos : true;
			}
			bool operator==(const iterator& v) const
			{
				fb_assert(lst == v.lst);
				return lst ? pos == v.pos : false;
			}
		};

		class const_iterator
		{
			friend class ObjectsArray<T, A>;
		private:
			const ObjectsArray *lst;
			size_t pos;
			const_iterator(const ObjectsArray *l, size_t p) : lst(l), pos(p) { }
		public:
			const_iterator() : lst(0), pos(0) { }
			explicit const_iterator(const iterator& it) : lst(it.lst), pos(it.pos) {}
			explicit const_iterator(iterator& it) : lst(it.lst), pos(it.pos) {}
			/*
			const_iterator& operator=(const ObjectsArray& a)
			{
				lst = &a;
				pos = 0;
			}
			*/
			const_iterator& operator++()
			{
				++pos;
				return (*this);
			}
			const_iterator operator++(int)
			{
				const_iterator tmp = *this;
				++pos;
				 return tmp;
			}
			const_iterator& operator--()
			{
				fb_assert(pos > 0);
				--pos;
				return (*this);
			}
			const_iterator operator--(int)
			{
				fb_assert(pos > 0);
				const_iterator tmp = *this;
				--pos;
				 return tmp;
			}
			const T* operator->()
			{
				fb_assert(lst);
				const T* pointer = lst->getPointer(pos);
				return pointer;
			}
			const T& operator*()
			{
				fb_assert(lst);
				const T* pointer = lst->getPointer(pos);
				return *pointer;
			}
			bool operator!=(const const_iterator& v) const
			{
				fb_assert(lst == v.lst);
				return lst ? pos != v.pos : true;
			}
			bool operator==(const const_iterator& v) const
			{
				fb_assert(lst == v.lst);
				return lst ? pos == v.pos : false;
			}
			// Against iterator
			bool operator!=(const iterator& v) const
			{
				fb_assert(lst == v.lst);
				return lst ? pos != v.pos : true;
			}
			bool operator==(const iterator& v) const
			{
				fb_assert(lst == v.lst);
				return lst ? pos == v.pos : false;
			}

		};

	public:
		void insert(size_t index, const T& item)
		{
			T* dataL = FB_NEW(this->getPool()) T(this->getPool(), item);
			inherited::insert(index, dataL);
		}
		size_t add(const T& item)
		{
			T* dataL = FB_NEW(this->getPool()) T(this->getPool(), item);
			return inherited::add(dataL);
		}
		T& add()
		{
			T* dataL = FB_NEW(this->getPool()) T(this->getPool());
			inherited::add(dataL);
			return *dataL;
		}
		void push(const T& item)
		{
			add(item);
		}
		T pop()
		{
			T* pntr = inherited::pop();
			T rc = *pntr;
			delete pntr;
			return rc;
		}
		void remove(size_t index)
		{
			fb_assert(index < getCount());
			delete getPointer(index);
			inherited::remove(index);
		}
		void remove(iterator itr)
		{
  			fb_assert(itr.lst == this);
			remove(itr.pos);
		}
		void shrink(size_t newCount)
		{
			for (size_t i = newCount; i < getCount(); i++) {
				delete getPointer(i);
			}
			inherited::shrink(newCount);
		}
		iterator begin()
		{
			return iterator(this, 0);
		}
		iterator end()
		{
			return iterator(this, getCount());
		}
		iterator back()
		{
  			fb_assert(getCount() > 0);
			return iterator(this, getCount() - 1);
		}
		const_iterator begin() const
		{
			return const_iterator(this, 0);
		}
		const_iterator end() const
		{
			return const_iterator(this, getCount());
		}
		const T& operator[](size_t index) const
		{
  			return *getPointer(index);
		}
		const T* getPointer(size_t index) const
		{
  			return inherited::getElement(index);
		}
		T& operator[](size_t index)
		{
  			return *getPointer(index);
		}
		T* getPointer(size_t index)
		{
  			return inherited::getElement(index);
		}
		explicit ObjectsArray(MemoryPool& p) : A(p) { }
		ObjectsArray() : A() { }
		~ObjectsArray()
		{
			for (size_t i = 0; i < getCount(); i++) {
				delete getPointer(i);
			}
		}

		size_t getCount() const {return inherited::getCount();}
		size_t getCapacity() const {return inherited::getCapacity();}

		bool isEmpty() const
		{
			return getCount() == 0;
		}

		void clear()
		{
			for (size_t i = 0; i < getCount(); i++) {
				delete getPointer(i);
			}
			inherited::clear();
		}
		ObjectsArray<T, A>& operator =(const ObjectsArray<T, A>& L)
		{
			while (this->count > L.count)
			{
				delete inherited::pop();
			}
			for (size_t i = 0; i < L.count; i++)
			{
				if (i < this->count)
				{
					(*this)[i] = L[i];
				}
				else
				{
					add(L[i]);
				}
			}
			return *this;
		}
	};

	// Template to convert object value to index directly
	template <typename T>
	class ObjectKeyValue
	{
	public:
		static const T& generate(const void* /*sender*/, const T* item) { return item; }
	};

	// Template for default value comparator
	template <typename T>
	class ObjectComparator
	{
	public:
		static bool greaterThan(const T i1, const T i2)
		{
			return *i1 > *i2;
		}
	};

	// Dynamic sorted array of simple objects
	template <typename ObjectValue,
		typename ObjectStorage = InlineStorage<ObjectValue*, 32>,
		typename ObjectKey = ObjectValue,
		typename ObjectKeyOfValue = DefaultKeyValue<ObjectValue*>,
		typename ObjectCmp = ObjectComparator<const ObjectKey*> >
	class SortedObjectsArray : public ObjectsArray<ObjectValue,
			SortedArray <ObjectValue*, ObjectStorage, const ObjectKey*,
			ObjectKeyOfValue, ObjectCmp> >
	{
	private:
		typedef ObjectsArray <ObjectValue, SortedArray<ObjectValue*,
				ObjectStorage, const ObjectKey*, ObjectKeyOfValue,
				ObjectCmp> > inherited;

	public:
		explicit SortedObjectsArray(MemoryPool& p) :
			ObjectsArray <ObjectValue, SortedArray<ObjectValue*,
				ObjectStorage, const ObjectKey*, ObjectKeyOfValue,
				ObjectCmp> >(p) { }
		bool find(const ObjectKey& item, size_t& pos) const
		{
			const ObjectKey* const pItem = &item;
			return static_cast<const SortedArray<ObjectValue*,
				ObjectStorage, const ObjectKey*, ObjectKeyOfValue,
				ObjectCmp>*>(this)->find(pItem, pos);
		}
		bool exist(const ObjectKey& item) const
		{
			size_t pos;
			return find(item, pos);
		}
		size_t add(const ObjectValue& item)
		{
			return inherited::add(item);
		}

	private:
		ObjectValue& add();	// Unusable when sorted
	};

	// Sorted pointers array - contains 2 arrays: simple for values (POD)
	// and sorted for pointers to them. Effective for big sizeof(POD).
	template <typename Value,
		typename Storage = EmptyStorage<Value>,
		typename Key = Value,
		typename KeyOfValue = DefaultKeyValue<Value*>,
		typename Cmp = ObjectComparator<const Key*> >
	class PointersArray
	{
	private:
		Array<Value, Storage> values;
		SortedArray<Value*, InlineStorage<Value*, 8>, const Key*, KeyOfValue, Cmp> pointers;

		void checkPointers(const Value* oldBegin)
		{
			Value* newBegin = values.begin();
			if (newBegin != oldBegin)
			{
				for (Value** ptr = pointers.begin(); ptr < pointers.end(); ++ptr)
				{
					*ptr = newBegin + (*ptr - oldBegin);
				}
			}
		}

	public:
		class const_iterator
		{
		private:
			const Value* const* ptr;

		public:
			const_iterator() : ptr(NULL) { }
			const_iterator(const const_iterator& it) : ptr(it.ptr) { }
			explicit const_iterator(const PointersArray& a) : ptr(a.pointers.begin()) { }

			const_iterator& operator++()
			{
				fb_assert(ptr);
				ptr++;
				return *this;
			}

			const_iterator operator++(int)
			{
				fb_assert(ptr);
				const_iterator tmp = *this;
				ptr++;
				return tmp;
			}

			const_iterator& operator--()
			{
				fb_assert(ptr);
				ptr--;
				return *this;
			}

			const_iterator operator--(int)
			{
				fb_assert(ptr);
				const_iterator tmp = *this;
				ptr--;
				return tmp;
			}

			const_iterator& operator+=(size_t v)
			{
				fb_assert(ptr);
				ptr += v;
				return *this;
			}

			const_iterator& operator-=(size_t v)
			{
				fb_assert(ptr);
				ptr -= v;
				return *this;
			}

			const Value* operator->()
			{
				fb_assert(ptr);
				return *ptr;
			}

			const Value& operator*()
			{
				fb_assert(ptr && *ptr);
				return **ptr;
			}

			bool operator==(const const_iterator& v) const
			{
				return ptr && ptr == v.ptr;
			}

			bool operator!=(const const_iterator& v) const
			{
				return !operator==(v);
			}
		};

		class iterator
		{
		private:
			Value** ptr;

		public:
			iterator() : ptr(NULL) { }
			iterator(const iterator& it) : ptr(it.ptr) { }
			explicit iterator(PointersArray& a) : ptr(a.pointers.begin()) { }

			iterator& operator++()
			{
				fb_assert(ptr);
				ptr++;
				return *this;
			}

			iterator operator++(int)
			{
				fb_assert(ptr);
				iterator tmp = *this;
				ptr++;
				return tmp;
			}

			iterator& operator--()
			{
				fb_assert(ptr);
				ptr--;
				return *this;
			}

			iterator operator--(int)
			{
				fb_assert(ptr);
				iterator tmp = *this;
				ptr--;
				return tmp;
			}

			iterator& operator+=(size_t v)
			{
				fb_assert(ptr);
				ptr += v;
				return *this;
			}

			iterator& operator-=(size_t v)
			{
				fb_assert(ptr);
				ptr -= v;
				return *this;
			}

			Value* operator->()
			{
				fb_assert(ptr);
				return *ptr;
			}

			Value& operator*()
			{
				fb_assert(ptr && *ptr);
				return **ptr;
			}

			bool operator==(const iterator& v) const
			{
				return ptr && ptr == v.ptr;
			}

			bool operator!=(const iterator& v) const
			{
				return !operator==(v);
			}
		};

	public:
		size_t add(const Value& item)
		{
			const Value* oldBegin = values.begin();
			values.add(item);
			checkPointers(oldBegin);
			return pointers.add(values.end() - 1);
		}

		const_iterator begin() const
		{
			return const_iterator(*this);
		}

		const_iterator end() const
		{
			const_iterator rc(*this);
			rc += getCount();
			return rc;
		}

		iterator begin()
		{
			return iterator(*this);
		}

		iterator end()
		{
			iterator rc(*this);
			rc += getCount();
			return rc;
		}

		const Value& operator[](size_t index) const
		{
  			return *getPointer(index);
		}

		const Value* getPointer(size_t index) const
		{
  			return pointers[index];
		}

		Value& operator[](size_t index)
		{
  			return *getPointer(index);
		}

		Value* getPointer(size_t index)
		{
  			return pointers[index];
		}

		explicit PointersArray(MemoryPool& p) : values(p), pointers(p) { }
		PointersArray() : values(), pointers() { }
		~PointersArray() { }

		size_t getCount() const
		{
			fb_assert(values.getCount() == pointers.getCount());
			return values.getCount();
		}

		size_t getCapacity() const
		{
			return values.getCapacity();
		}

		void clear()
		{
			pointers.clear();
			values.clear();
		}

		PointersArray& operator=(const PointersArray& o)
		{
			values = o.values;
			pointers = o.pointers;
			checkPointers(o.values.begin());
			return *this;
		}

		bool find(const Key& item, size_t& pos) const
		{
			return pointers.find(&item, pos);
		}

		bool exist(const Key& item) const
		{
			return pointers.exist(item);
		}

		void insert(size_t pos, const Value& item)
		{
			const Value* oldBegin = values.begin();
			values.add(item);
			checkPointers(oldBegin);
			pointers.insert(pos, values.end() - 1);
		}
	};

} // namespace Firebird

#endif	// CLASSES_OBJECTS_ARRAY_H

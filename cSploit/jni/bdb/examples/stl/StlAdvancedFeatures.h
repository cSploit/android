/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$ 
 */

#ifndef _DB_STL_AFE_H__
#define _DB_STL_AFE_H__

#include <time.h>

#include <utility>
#include <functional>
#include <algorithm>
#include <iostream>
#include <vector>
#include <list>
#include <queue>
#include <stack>
#include <fstream>
#include <map>
#include <set>
#include <string>



#define DB_STL_HAVE_DB_TIMESPEC 1
#include "dbstl_map.h"
#include "dbstl_set.h"
#include "dbstl_vector.h"

using namespace std;
using namespace dbstl;

/////////////////////////////////////////////////////////////////////////
///////////////////////// Macro and typedef definitions ///////////

#define check_expr(expression) do {			\
	if (!(expression)) {				\
		FailedAssertionException ex(__FILE__, __LINE__, #expression);\
		throw ex; } } while (0)

#define ptint int
#define TOINT (int)
typedef db_vector<ptint, ElementHolder<ptint> > intvec_t;
typedef db_vector<ptint, ElementHolder<ptint> > ptint_vector;
typedef db_map<ptint, ptint, ElementHolder<ptint> > dm_int_t;
typedef db_multimap<ptint, ptint, ElementHolder<ptint> > dmm_int_t;
typedef db_set<ptint, ElementHolder<ptint> > dms_int_t;
typedef db_multiset<ptint, ElementHolder<ptint> > dmms_int_t;
typedef bool (*ptintless_ft)(const ptint& a, const ptint& b);

extern "C" {
extern int getopt(int, char * const *, const char *);
extern char *optarg;
extern int optind;
}
extern int g_test_start_txn;
extern DbEnv *g_env;
///////////////////////////////////////////////////////////////////////
//////////////////////// Function Declarations ////////////////////
// XXX!!! Function templates can't be declared here otherwise the declarations
// here will be deemed as the definition, so at link time these symbols are 
// not resolved. So like class templates, function templates can't be separated
// as declarations and definitions, only definitions and only be built into one
// object file otherwise there will be "multiple symbol definitions". OTOH, compilers
// can avoid the multiple definitions if we are building a class template instantiation
// in multiple object files, so class tempaltes are recommended to use rather than
// function templates. Only use function templates if it is a simple one and used
// only in one code file.
//

int get_dest_secdb_callback(Db *secondary, const Dbt *key, 
    const Dbt *data, Dbt *result);
void using_charstr(TCHAR*str);

class RGBB;
class SMSMsg2;
void SMSMsgRestore(SMSMsg2& dest, const void *srcdata);
u_int32_t SMSMsgSize(const SMSMsg2& elem);
void SMSMsgCopy(void *dest, const SMSMsg2&elem);
u_int32_t rgblen(const RGBB * seq);
void rgbcpy(RGBB *seq, const RGBB *, size_t);


/////////////////////////////////////////////////////////////////////////////////
////////////////////////// Utility class definitions  ///////////////////////////

class BaseMsg
{
public:
	time_t when;
	int to;
	int from;

	BaseMsg()
	{
		to = from = 0;
		when = 0;
	}

	BaseMsg(const BaseMsg& msg)
	{
		to = msg.to;
		from = msg.from;
		when = msg.when;
	}	

	bool operator==(const BaseMsg& msg2) const 
	{
		return when == msg2.when && to == msg2.to && from == msg2.from;
	}

	bool operator<(const BaseMsg& msg2) const
	{
		return to < msg2.to;
	}
};

// used to test arbitary obj storage(not in one chunk)
class SMSMsg2 : public BaseMsg
{
public:
	typedef SMSMsg2 self;
	SMSMsg2(time_t tm, const char *msgp, int t)
	{
		memset(this, 0, sizeof(*this));
		when = tm;
		szmsg = strlen(msgp) + 1;
		msg = (char *)DbstlMalloc(szmsg);
		strncpy(msg, msgp, szmsg);
		to = t;
		
		mysize = sizeof(*this); //+ szmsg;
	}

	SMSMsg2()
	{
		memset(this, 0, sizeof(SMSMsg2));
	}

	SMSMsg2(const self& obj) : BaseMsg(obj)
	{
		mysize = obj.mysize;
		szmsg = obj.szmsg;
		if (szmsg > 0 && obj.msg != NULL) {
			msg = (char *)DbstlMalloc(szmsg);
			strncpy(msg, obj.msg, szmsg);
		} else 
			msg = NULL;
	}

	~SMSMsg2()
	{
		if (msg)
			free(msg);
	}

	const self& operator = (const self &obj)
	{

		this->from = obj.from;
		to = obj.to;
		when = obj.when;
		mysize = obj.mysize;
		szmsg = obj.szmsg;
		if (szmsg > 0 && obj.msg != NULL) {
			msg = (char *)DbstlReAlloc(msg, szmsg);
			strncpy(msg, obj.msg, szmsg);
		}
		return obj;
	}

	bool operator == (const self&obj) const
	{
		return BaseMsg::operator==(obj) && strcmp(obj.msg, msg) == 0;
	}

	const static size_t BUFLEN = 256;
	size_t mysize;
	size_t szmsg;
	char *msg;
	
};//SMSMsg2

// SMS message class
class SMSMsg : public BaseMsg
{

	
public:

	size_t mysize;
	size_t szmsg;
	char msg[1];
	static SMSMsg* make_sms_msg(time_t t, const char*msg, int dest)
	{
		size_t mlen = 0, totalsz = 0;

		SMSMsg *p = (SMSMsg *)DbstlMalloc(totalsz = (sizeof(SMSMsg) + (mlen = strlen(msg) + 4)));
		memset(p, 0, totalsz);
		// adding sizeof(p->to) to avoid memory alignment issues
		p->mysize = sizeof(SMSMsg) + mlen;
		p->when = t;
		p->szmsg = mlen - 3;
		p->to = dest;
		p->from = 0;
		strcpy(&(p->msg[0]), msg);

		return p;
	}

	SMSMsg()
	{

	}
protected:
	SMSMsg(time_t t, const char*msg1, int dest)
	{
		size_t mlen = 0;

		when = t;
		szmsg = strlen(msg1) + 1;
		mlen = strlen(msg1);
		strncpy((char*)&(this->msg[0]), msg1, mlen);
		*(int*)(((char*)&(this->msg[0])) + mlen + 1) = dest;
	}

};// SMSMsg

class RGBB 
{
public:
	typedef unsigned char color_t;

	color_t r_, g_, b_, bright_;

	RGBB()
	{
		memset(this, 0, sizeof(RGBB));// complete 0 means invalid
	}

	RGBB(color_t r, color_t g, color_t b, color_t brightness)
	{
		r_ = r;
		g_ = g;
		b_ = b;
		bright_ = brightness;
	}

};// RGBB

class rand_str_dbt
{
public:
	static const size_t BUFLEN = 2048;
	static bool init;
	static char buf[BUFLEN];

	rand_str_dbt() 
	{
		int len = BUFLEN, i;
		
		if (!init) {
			init = true;
			
			for (i = 0; i < len - 1; i++) {
				buf[i] = 'a' + rand() % 26;
			}
			buf[i] = '\0';
		}
	}
	// dbt is of DB_DBT_USERMEM, mem allocated by DbstlMalloc
	void operator()(Dbt&dbt, string&str, 
	    size_t shortest = 30, size_t longest = 150) 
	{
		int rd = rand();

		if (rd < 0)
			rd = -rd;
		str.clear();

		check_expr(shortest > 0 && longest < BUFLEN);
		check_expr(dbt.get_flags() & DB_DBT_USERMEM);// USER PROVIDE MEM
		size_t len = (u_int32_t)(rd % longest);
		if (len < shortest)
			len = shortest;
		else if (len >= BUFLEN)
			len = BUFLEN - 1;
		// start must be less than BUFLEN - len, otherwise we have no 
		// len bytes to offer
		size_t start = rand() % (BUFLEN - len);

		char c = buf[start + len];

		buf[start + len] = '\0';
		str = buf + start;
		if (dbt.get_ulen() < (len + 1)) {
			free(dbt.get_data());
			dbt.set_data(DbstlMalloc(len + 1));
			check_expr(dbt.get_data() != NULL);
		}
		memcpy(dbt.get_data(), (void*)(buf + start), len + 1);
		dbt.set_size(u_int32_t(len + 1));// store the '\0' at the end
		buf[start + len] = c; 
	}
}; // rand_str_dbt
bool rand_str_dbt::init = false;
char rand_str_dbt::buf[BUFLEN];

struct TestParam{
	int flags, setflags, test_autocommit, dboflags, explicit_txn;
	DBTYPE dbtype;
	DbEnv *dbenv;
};

// a mobile phone SMS structure for test. will add more members in future
class sms_t
{
public:
	size_t sz;
	time_t when;
	int from;
	int to;
	char msg[512];
	
	const sms_t& operator=(const sms_t&me)
	{
		memcpy(this, &me, sizeof(*this));
		return me;
	}

	bool operator==(const sms_t& me) const
	{
		return memcmp(this, &me, sizeof(me)) == 0;
	}
	bool operator!=(const sms_t& me)
	{
		return memcmp(this, &me, sizeof(me)) != 0;
	}
};

bool ptintless(const ptint& a, const ptint& b)
{
	return a < b;
}

template<Typename T>
void fill(db_vector<T, ElementHolder<T> >&v, 
    vector<T>&sv, T start = 0, int n = 5 )
{
	int i;
	
	v.clear();
	sv.clear();
	for (i = 0; i < n; i++) {
		v.push_back(i + start);
		sv.push_back(i + start);
	}
}



template <Typename T>
void fill(db_map<T, T, ElementHolder<T> >&m, map<T, T>&sm, 
    T start = 0, int n = 5) 
{
	int i;
	T pi;

	m.clear();
	sm.clear();
	for (i = 0; i < n; i++) {
		pi = i + start;
		m.insert(make_pair(pi, pi));
		sm.insert(make_pair(pi, pi));
	}
	
}




template <Typename T>
void fill(db_set<T, ElementHolder<T> >&m, set<T>&sm, 
    T start = 0, int n = 5) 
{
	int i;
	T pi;

	m.clear();
	sm.clear();
	for (i = 0; i < n; i++) {
		pi = i + start;
		m.insert(pi);
		sm.insert(pi);
	}
	
}




template <Typename T>
void fill(db_multimap<T, T, ElementHolder<T> >&m, multimap<T, T>&sm, 
    T start = 0, int n = 5, size_t randn = 5) 
{
	int i;
	size_t j, cnt = 0;

	if (randn < 5)
		randn = 5;

	m.clear();
	sm.clear();
	for (i = 0; i < n; i++) {
		cnt = abs(rand()) % randn;
		if (cnt == 0)
			cnt = randn;
		i += start;
		for (j = 0; j < cnt; j++) {// insert duplicates
			m.insert(make_pair(i, i));
			sm.insert(make_pair(i, i));
		}
		i -= start;
	}
	
}




template <Typename T>
void fill(db_multiset<T, ElementHolder<T> >&m, multiset<T>&sm, 
    T start = 0, int n = 5 , size_t randn = 5) 
{
	int i;
	size_t j, cnt;

	if (randn < 5)
		randn = 5;
	
	m.clear();
	sm.clear();
	for (i = 0; i < n; i++) {
		cnt = abs(rand()) % randn;
		if (cnt == 0)
			cnt = randn;
		i += start;
		for (j = 0; j < cnt; j++) {// insert duplicates
			m.insert(i);
			sm.insert(i);
		}
		i -= start;
	}
}

template <Typename T1, Typename T2, Typename T3, Typename T4> 
bool is_equal(db_map<T1, T2, ElementHolder<T2> >& dv, map<T3, T4>&v)
{
	size_t s1, s2;
	bool ret;
	typename db_map<T1, T2, ElementHolder<T2> >::iterator itr1;
	typename map<T3, T4>::iterator itr2;

	if (g_test_start_txn)
		begin_txn(0, g_env);
	if ((s1 = dv.size()) != (s2 = v.size())){
		ret = false;
		goto done;
	}
		 
	for (itr1 = dv.begin(), itr2 = v.begin(); 
	    itr1 != dv.end(); ++itr1, ++itr2) {
		if (itr1->first != itr2->first || itr1->second != itr2->second){
			ret = false;
			goto done;
		}
			 
	}
	
	ret = true;
done:
	if (g_test_start_txn)
		commit_txn(g_env);
	return ret;

}

template <Typename T1, Typename T2, Typename T3, Typename T4> 
bool is_equal(db_map<T1, T2, ElementRef<T2> >& dv, map<T3, T4>&v)
{
	size_t s1, s2;
	bool ret;
	typename db_map<T1, T2, ElementRef<T2> >::iterator itr1;
	typename map<T3, T4>::iterator itr2;

	if (g_test_start_txn)
		begin_txn(0, g_env);
	if ((s1 = dv.size()) != (s2 = v.size())){
		 
		ret = false;
		goto done;
	}


	for (itr1 = dv.begin(), itr2 = v.begin(); 
	    itr1 != dv.end(); ++itr1, ++itr2) {
		if (itr1->first != itr2->first || itr1->second != itr2->second){
			ret = false;
			goto done;
		}
	}
	
	ret = true;
done:
	if (g_test_start_txn)
		commit_txn(g_env);
	return ret;

}


template<Typename T1, Typename T2>
bool is_equal(const db_set<T1, ElementHolder<T1> >&s1, const set<T2>&s2)
{
	bool ret;
	typename db_set<T1, ElementHolder<T1> >::iterator itr;

	if (g_test_start_txn)
		begin_txn(0, g_env);
	if (s1.size() != s2.size()){
		ret = false;
		goto done;
	}

	for (itr = s1.begin(); itr != s1.end(); itr++) {
		if (s2.count(*itr) == 0) {
			ret = false;
			goto done;
		}
	}
	ret = true;
done:
	if (g_test_start_txn)
		commit_txn(g_env);
	return ret;

}


template <Typename T1, Typename T2>
bool is_equal(const db_vector<T1>& dv, const vector<T2>&v)
{
	size_t s1, s2;
	bool ret;
	T1 t1;
	size_t i, sz = v.size();

	if (g_test_start_txn)
		begin_txn(0, g_env);
	if ((s1 = dv.size()) != (s2 = v.size())) {
		ret = false;
		goto done;
	}

	
	for (i = 0; i < sz; i++) {
		t1 = T1(dv[(index_type)i] );
		if (t1 != v[i]){
			ret = false;
			goto done;
		}
	}
	ret = true;
done:

	if (g_test_start_txn)
		commit_txn(g_env);
	return ret;
}

// The following four functions are designed to work with is_equal to compare
// char*/wchar_t* strings properly, unforturnately they can not override
// the default pointer value comparison behavior.
bool operator==(ElementHolder<const char *>s1, const char *s2);
bool operator==(ElementHolder<const wchar_t *>s1, const wchar_t *s2);
bool operator!=(ElementHolder<const char *>s1, const char *s2);
bool operator!=(ElementHolder<const wchar_t *>s1, const wchar_t *s2);

template <Typename T1>
bool is_equal(const db_vector<T1, ElementHolder<T1> >& dv, const vector<T1>&v)
{
	size_t s1, s2;
	bool ret;
	size_t i, sz = v.size();

	if (g_test_start_txn)
		begin_txn(0, g_env);
	if ((s1 = dv.size()) != (s2 = v.size())) {
		ret = false;
		goto done;
	}
	
	for (i = 0; i < sz; i++) {
		if (dv[(index_type)i] != v[i]) {
			ret = false;
			goto done;
		}
	}
	ret = true;
done:

	if (g_test_start_txn)
		commit_txn(g_env);
	return ret;
}


template <Typename T1, Typename T2>
bool is_equal(db_vector<T1>& dv, list<T2>&v)
{
	size_t s1, s2;
	bool ret;
	typename db_vector<T1>::iterator itr;
	typename list<T2>::iterator itr2;

	if (g_test_start_txn)
		begin_txn(0, g_env);
	if ((s1 = dv.size()) != (s2 = v.size())) {
		ret = false;
		goto done;
	}

	
	for (itr = dv.begin(), itr2 = v.begin(); 
	    itr2 != v.end(); ++itr, ++itr2) {
		if (*itr != *itr2) {
			ret = false;
			goto done;
		}
	}
	ret = true;
done:

	if (g_test_start_txn)
		commit_txn(g_env);
	return ret;
}

bool is_equal(db_vector<char *, ElementHolder<char *> >&v1, 
    std::list<string> &v2)
{
	db_vector<char *, ElementHolder<char *> >::iterator itr;
	std::list<string>::iterator itr2;

	if (v1.size() != v2.size())
		return false;

	for (itr = v1.begin(), itr2 = v2.begin(); 
	    itr2 != v2.end(); ++itr, ++itr2)
		if (strcmp(*itr, (*itr2).c_str()) != 0)
			return false;

	return true;
}

template <Typename T1>
class atom_equal {
public:
	bool operator()(T1 a, T1 b)
	{
		return a == b;
	}
};
template<>
class atom_equal<const char *> {
public:
	bool operator()(const char *s1, const char *s2)
	{
		return strcmp(s1, s2) == 0;
	}
};

template <Typename T1>
bool is_equal(const db_vector<T1, ElementHolder<T1> >& dv, const list<T1>&v)
{
	size_t s1, s2;
	bool ret;
	typename db_vector<T1, ElementHolder<T1> >::const_iterator itr;
	typename list<T1>::const_iterator itr2;
	atom_equal<T1> eqcmp;
	if (g_test_start_txn)
		begin_txn(0, g_env);
	if ((s1 = dv.size()) != (s2 = v.size())) {
		ret = false;
		goto done;
	}

	
	for (itr = dv.begin(), itr2 = v.begin(); 
	    itr2 != v.end(); ++itr, ++itr2) {
		if (!eqcmp(*itr, *itr2)) {
			ret = false;
			goto done;
		}
	}
	ret = true;
done:

	if (g_test_start_txn)
		commit_txn(g_env);
	return ret;
}

#endif // ! _DB_STL_AFE_H__

/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$ 
 */

#include "StlAdvancedFeatures.h"

static void usage();

class StlAdvancedFeaturesExample
{
public:
	typedef map<int, int> m_int_t;
	typedef ptint tpint;
	~StlAdvancedFeaturesExample();
	StlAdvancedFeaturesExample(void *param1);
	void run()
	{
		arbitrary_object_storage();
		storing_std_strings();
		secondary_containers();
		char_star_string_storage();
		arbitray_sequence_storage();
		bulk_retrieval_read();
		primitive();
		queue_stack();
	}
private:

	// Store primitive types of data into dbstl containers.
	void primitive();

	// Use std::queue and std::stack as adapters, and dbstl::db_vector 
	// as container, to make a queue and a stack.
	void queue_stack();

	// Use two ways to store an object of arbitrary length. The object
	// contains some varying length members, char* string for example.
	void arbitrary_object_storage();

	// Store std::string types of strings.
	void storing_std_strings();

	// Open a secondary db H and associate it with an exisitng db handle
	// which is being used by a container C1, then use H to create another
	// container C2, verify we can get C1's data via C2.
	// This technique works for all types of db and containers.
	void secondary_containers();

	// Direct storage of char* strings.
	void char_star_string_storage();

	// Storage of arbitrary element type of sequence.
	void arbitray_sequence_storage();

	// Reading with bulk retrieval flag.
	void bulk_retrieval_read();


	int flags, setflags, explicit_txn, test_autocommit, n;
	DBTYPE dbtype;
	dm_int_t::difference_type oddcnt;
	Db *db3, *dmdb1, *dmdb2, *dmmdb1, *dmmdb2, *dmsdb1, 
	    *dmsdb2, *dmmsdb1, *dmmsdb2, *dbstrv, *pquedb, *quedb;
	Db *dbp3;
	Db *dmdb6;
	Db *dbp3sec;
	Db *dmmdb4, *dbstrmap;
	Db *dmstringdb;
	Db *dbprim;
	DbEnv *penv;
	u_int32_t dboflags;

	map<int, int> m1;
	multimap<int, int> mm1;
};

StlAdvancedFeaturesExample::~StlAdvancedFeaturesExample()
{
}

StlAdvancedFeaturesExample::StlAdvancedFeaturesExample(void *param1)
{
	check_expr(param1 != NULL);
	TestParam *param = (TestParam*)param1;
	TestParam *ptp = param;

	flags = 0, setflags = 0, explicit_txn = 1, test_autocommit = 0;
	dbtype = DB_BTREE;
	penv = param->dbenv;
	dmdb1 = dmdb2 = dmmdb1 = dmmdb2 = dmsdb1 = dmsdb2 = dmmsdb1 = 
	    dmmsdb2 = dbstrv = NULL;
	flags = param->flags;
	dbtype = param->dbtype;
	setflags = param->setflags;
	test_autocommit = param->test_autocommit;
	explicit_txn = param->explicit_txn;
	dboflags = ptp->dboflags;
	n = 10;

	dmdb1 = dbstl::open_db(penv, "db_map.db", 
	    dbtype, DB_CREATE | ptp->dboflags, 0);
	dmdb2 = dbstl::open_db(penv, "db_map2.db", 
	    dbtype, DB_CREATE | ptp->dboflags, 0);
	dmdb6 = dbstl::open_db(penv, "db_map6.db", 
	    dbtype, DB_CREATE | ptp->dboflags, 0);

	dmmdb1 = dbstl::open_db(penv, 
	    "db_multimap.db", dbtype, DB_CREATE | ptp->dboflags, DB_DUP);
	dmmdb2 = dbstl::open_db(penv, 
	    "db_multimap2.db", dbtype, DB_CREATE | ptp->dboflags, DB_DUP);
	
	dmsdb1 = dbstl::open_db(penv, "db_set.db", 
	    dbtype, DB_CREATE | ptp->dboflags, 0);
	dmsdb2 = dbstl::open_db(penv, "db_set2.db", 
	    dbtype, DB_CREATE | ptp->dboflags, 0);
	
	dmmsdb1 = dbstl::open_db(penv, 
	    "db_multiset.db", dbtype, DB_CREATE | ptp->dboflags, DB_DUP);
	dmmsdb2 = dbstl::open_db(penv, 
	    "db_multiset2.db", dbtype, DB_CREATE | ptp->dboflags, DB_DUP);

	dbstrv = dbstl::open_db(penv, "dbstr.db", 
	    DB_RECNO, DB_CREATE | ptp->dboflags, DB_RENUMBER);
	dbp3sec = dbstl::open_db(penv, "db_map_sec.db", 
	    dbtype, DB_CREATE | ptp->dboflags, DB_DUP);

	dmmdb4 = dbstl::open_db(penv, 
	    "db_multimap4.db", dbtype, DB_CREATE | dboflags, DB_DUPSORT);
	dbstrmap = dbstl::open_db(penv, "dbstrmap.db", 
	    DB_BTREE, DB_CREATE, 0);

	dmstringdb = dbstl::open_db(penv, "db_map_stringdb.db", 
	    dbtype, DB_CREATE | dboflags, 0);
	db3 = dbstl::open_db(penv, "db3.db", 
	    DB_RECNO, DB_CREATE | ptp->dboflags, DB_RENUMBER);

	// NO DB_RENUMBER needed 
	quedb = dbstl::open_db(penv, "dbquedb.db", 
	    DB_RECNO, DB_CREATE | ptp->dboflags | DB_THREAD, 0);

	pquedb = dbstl::open_db(penv, "dbpquedb.db", 
	    DB_RECNO, DB_CREATE | ptp->dboflags | DB_THREAD, DB_RENUMBER);
	dbprim = dbstl::open_db(penv, "dbprim.db",
	    DB_RECNO, DB_CREATE | ptp->dboflags | DB_THREAD, DB_RENUMBER);

	dbp3 = dbstl::open_db(penv, "dbp3.db", 
	    dbtype, DB_CREATE | ptp->dboflags, 0);
	
}

void StlAdvancedFeaturesExample::arbitrary_object_storage()
{
	int i;

	if (explicit_txn)
		begin_txn(0, penv);
	// varying length objects test
	cout<<"\nArbitary object storage using Dbt..\n";
	
	rand_str_dbt smsdbt;
	DbstlDbt dbt, dbtmsg;
	string msgstr;
	SMSMsg *smsmsgs[10];

	dbtmsg.set_flags(DB_DBT_USERMEM);
	dbt.set_data(DbstlMalloc(256));
	dbt.set_flags(DB_DBT_USERMEM);
	dbt.set_ulen(256);
	db_map<int, DbstlDbt> msgmap(dbp3, penv);
	for (i = 0; i < 10; i++) {
		smsdbt(dbt, msgstr, 10, 200);
		SMSMsg *pmsg = SMSMsg::make_sms_msg(time(NULL), 
		    (char *)dbt.get_data(), i);
		smsmsgs[i] = SMSMsg::make_sms_msg(time(NULL), 
		    (char *)dbt.get_data(), i);
		dbtmsg.set_data(pmsg);
		dbtmsg.set_ulen((u_int32_t)(pmsg->mysize));
		dbtmsg.set_size((u_int32_t)(pmsg->mysize));
		dbtmsg.set_flags(DB_DBT_USERMEM);
		msgmap.insert(make_pair(i, dbtmsg));
		free(pmsg);
		memset(&dbtmsg, 0, sizeof(dbtmsg));
	}
	dbtmsg.set_data(NULL);

	SMSMsg *psmsmsg;
	for (i = 0; i < 10; i++) {
		db_map<int, DbstlDbt>::data_type_wrap msgref = msgmap[i];
		psmsmsg = (SMSMsg *)msgref.get_data();
		check_expr(memcmp(smsmsgs[i], psmsmsg, 
		    smsmsgs[i]->mysize) == 0);
	}

	i = 0;
	for (db_map<int, DbstlDbt>::iterator msgitr = 
	    msgmap.begin(ReadModifyWriteOption::
	    read_modify_write()); msgitr != msgmap.end(); ++msgitr, i++) {
		db_map<int, DbstlDbt>::reference smsmsg = *msgitr;
		(((SMSMsg*)(smsmsg.second.get_data())))->when = time(NULL);
		smsmsg.second._DB_STL_StoreElement();
		
	}

	for (i = 0; i < 10; i++) 
		free(smsmsgs[i]);
	
	msgmap.clear();


	cout<<"\nArbitary object(sparse, varying length) storage support using registered callbacks.\n";
	db_map<int, SMSMsg2> msgmap2(dbp3, penv);
	SMSMsg2 smsmsgs2[10];
	DbstlElemTraits<SMSMsg2>::instance()->set_copy_function(SMSMsgCopy);
	DbstlElemTraits<SMSMsg2>::instance()->set_size_function(SMSMsgSize);
	DbstlElemTraits<SMSMsg2>::instance()->set_restore_function(SMSMsgRestore);
	// use new technique to store varying length and inconsecutive objs
	for (i = 0; i < 10; i++) {
		smsdbt(dbt, msgstr, 10, 200);
		SMSMsg2 msg2(time(NULL), msgstr.c_str(), i);
		smsmsgs2[i] = msg2;
		
		msgmap2.insert(make_pair(i, msg2));
	 
	}
	
	// check that retrieved data is identical to stored data
	SMSMsg2 tmpmsg2;
	for (i = 0; i < 10; i++) {
		tmpmsg2 = msgmap2[i];
		check_expr(smsmsgs2[i] == tmpmsg2);
	}
	for (db_map<int, SMSMsg2>::iterator msgitr = 
	    msgmap2.begin(ReadModifyWriteOption::
	    read_modify_write()); msgitr != msgmap2.end(); msgitr++) {
		    db_map<int, SMSMsg2>::reference smsmsg = *msgitr;
		smsmsg.second.when = time(NULL);
		smsmsg.second._DB_STL_StoreElement();
		
	}
	msgmap2.clear();
	if (explicit_txn)
		commit_txn(penv);
} // arbitrary_object_storage

// std::string persistent test.
void StlAdvancedFeaturesExample::storing_std_strings()
{	
	string kstring = "hello world", *sstring = new string("hi there");
	if (explicit_txn)
		begin_txn(0, penv);

	db_map<string, string> pmap(dmstringdb, NULL);

	pmap[kstring] = *sstring + "!";
	*sstring = pmap[kstring];
	map<string, string> spmap;
	spmap.insert(make_pair(kstring, *sstring));
	cout<<"sstring append ! is : "<<pmap[kstring]
	    <<" ; *sstring is : "<<*sstring;
	delete sstring;
	for (db_map<string, string>::iterator ii = pmap.begin();
	    ii != pmap.end();
	    ++ii) {
		cout << (*ii).first << ": " << (*ii).second << endl;
	} 
	close_db(dmstringdb);
	
	dmstringdb = dbstl::open_db(penv, "db_map_stringdb.db", 
	    dbtype, DB_CREATE | dboflags, 0);
	db_map<string, string> pmap2(dmstringdb, NULL);
	for (db_map<string, string>::iterator ii = pmap2.begin();
	    ii != pmap2.end(); ++ii) {
		cout << (*ii).first << ": " << (*ii).second << endl;
		// assert key/data pair set equal
		check_expr((spmap.count(ii->first) == 1) && 
		    (spmap[ii->first] == ii->second));
	} 
	if (explicit_txn)
		commit_txn(penv);

	db_vector<string> strvctor(10);
	vector<string> sstrvctor(10);
	for (int i = 0; i < 10; i++) {
		strvctor[i] = "abc";
		sstrvctor[i] = strvctor[i];
	}
	check_expr(is_equal(strvctor, sstrvctor));
}

void StlAdvancedFeaturesExample::secondary_containers()
{
	int i;

	if (explicit_txn)
		begin_txn(0, penv);
	// test secondary db
	cout<<"\ndb container backed by secondary database.";
	
	dbp3->associate(dbstl::current_txn(penv), dbp3sec, 
	    get_dest_secdb_callback, DB_CREATE);
	typedef db_multimap<int, BaseMsg>  sec_mmap_t;
	sec_mmap_t secmmap(dbp3sec, penv);// index "to" field
	db_map<int, BaseMsg> basemsgs(dbp3, penv);
	basemsgs.clear();
	BaseMsg tmpmsg;
	multiset<BaseMsg> bsmsgs, bsmsgs2;
	multiset<BaseMsg>::iterator bsitr1, bsitr2;
	// populate primary and sec db
	for (i = 0; i < 10; i++) {
		tmpmsg.when = time(NULL);
		tmpmsg.to = 100 - i % 3;// sec index multiple
		tmpmsg.from = i + 20;
		bsmsgs.insert( tmpmsg);
		basemsgs.insert(make_pair(i, tmpmsg));

	}
	check_expr(basemsgs.size() == 10);
	// check retrieved data is identical to those fed in
	sec_mmap_t::iterator itrsec;
	for (itrsec = secmmap.begin(
	    ReadModifyWriteOption::no_read_modify_write(), true);
	    itrsec != secmmap.end(); itrsec++) {
		bsmsgs2.insert(itrsec->second);
	}
	for (bsitr1 = bsmsgs.begin(), bsitr2 = bsmsgs2.begin(); 
	    bsitr1 != bsmsgs.end() && bsitr2 != bsmsgs2.end(); 
	    bsitr1++, bsitr2++) {
		check_expr(*bsitr1 == *bsitr2);
	}
	check_expr(bsitr1 == bsmsgs.end() && bsitr2 == bsmsgs2.end());

	// search using sec index, check the retrieved data is expected
	// and exists in bsmsgs
	check_expr(secmmap.size() == 10);
	pair<sec_mmap_t::iterator, sec_mmap_t::iterator> secrg = 
	    secmmap.equal_range(98);

	for (itrsec = secrg.first; itrsec != secrg.second; itrsec++) {
		check_expr(itrsec->second.to == 98 && 
		    bsmsgs.count(itrsec->second) > 0);
	}
	// delete via sec db
	size_t nersd = secmmap.erase(98);
	check_expr(10 - nersd == basemsgs.size());
	secrg = secmmap.equal_range(98);
	check_expr(secrg.first == secrg.second);

	if (explicit_txn)
		dbstl::commit_txn(penv);
	 
} // secondary_containers

void StlAdvancedFeaturesExample::char_star_string_storage()
{
	int i;
	// Varying length data element storage/retrieval
	cout<<"\nchar*/wchar_t* string storage support...\n";
	
	if (explicit_txn)
		dbstl::begin_txn(0, penv);
	// Use Dbt to wrap any object and store them. This is rarely needed, 
	// so this piece of code is only for test purpose.
	db_vector<DbstlDbt> strv(dbstrv, penv);
	vector<string> strsv;
	vector<DbstlDbt> strvdbts;
	strv.clear();
	
	int strlenmax = 256, strlenmin = 64;
	string str;
	DbstlDbt dbt;
	rand_str_dbt rand_str_maker;
	dbt.set_flags(DB_DBT_USERMEM);
	dbt.set_data(DbstlMalloc(strlenmax + 10));
	dbt.set_ulen(strlenmax + 10);

	for (int jj = 0; jj < 10; jj++) {
		rand_str_maker(dbt, str, strlenmin, strlenmax);
		strsv.push_back(str);
		strv.push_back(dbt);
	}
	
	cout<<"\nstrings:\n";
	for (i = 0; i < 10; i++) {
		db_vector<DbstlDbt>::value_type_wrap elemref = strv[i];
		strvdbts.push_back(elemref);
		printf("\n%s\n%s",  (char*)(strvdbts[i].get_data()), 
		    strsv[i].c_str());
		check_expr(strcmp((char*)(elemref.get_data()), 
		    strsv[i].c_str()) == 0);
		check_expr(strcmp((char*)(strvdbts[i].get_data()), 
		    strsv[i].c_str()) == 0);
	}
	strv.clear();
	
	if (explicit_txn) {
		dbstl::commit_txn(penv);
		dbstl::begin_txn(0, penv);
	}

	// Use ordinary way to store strings.
	TCHAR cstr1[32], cstr2[32], cstr3[32];
	strcpy(cstr1, "abc");
	strcpy(cstr2, "defcd");
	strcpy(cstr3, "edggsefcd");

	typedef db_map<int, TCHAR*, ElementHolder<TCHAR*> > strmap_t;
	strmap_t strmap(dmdb6, penv);
	strmap.clear();
	strmap.insert(make_pair(1, cstr1));
	strmap.insert(make_pair(2, cstr2));
	strmap.insert(make_pair(3, cstr3));
	cout<<"\n strings in strmap:\n";
	for (strmap_t::const_iterator citr = strmap.begin(); 
	    citr != strmap.end(); citr++)
		cout<<(*citr).second<<'\t';
	cout<<strmap[1]<<strmap[2]<<strmap[3];
	TCHAR cstr4[32], cstr5[32], cstr6[32];
	_tcscpy(cstr4, strmap[1]);
	_tcscpy(cstr5, strmap[2]);
	_tcscpy(cstr6, strmap[3]);
	_tprintf(_T("%s, %s, %s"), cstr4, cstr5, cstr6);
	strmap_t::value_type_wrap::second_type vts = strmap[1];
	using_charstr(vts);
	vts._DB_STL_StoreElement();
	_tcscpy(cstr4, _T("hello world"));
	vts = cstr4;
	vts._DB_STL_StoreElement();
	cout<<"\n\nstrmap[1]: "<<strmap[1];
	check_expr(_tcscmp(strmap[1], cstr4) == 0);
	vts[0] = _T('H');// it is wrong to do it this way
	vts._DB_STL_StoreElement();
	check_expr(_tcscmp(strmap[1], _T("Hello world")) == 0);
	TCHAR *strbase = vts._DB_STL_value();
	strbase[6] = _T('W');
	vts._DB_STL_StoreElement();
	check_expr(_tcscmp(strmap[1], _T("Hello World")) == 0);
	strmap.clear();

	typedef db_map<const char *, const char *, 
	    ElementHolder<const char *> > cstrpairs_t;
	cstrpairs_t strpairs(dmdb6, penv);
	strpairs["abc"] = "def";
	strpairs["ghi"] = "jkl";
	strpairs["mno"] = "pqrs";
	strpairs["tuv"] = "wxyz";
	cstrpairs_t::const_iterator ciitr;
	cstrpairs_t::iterator iitr;
	for (ciitr = strpairs.begin(), iitr = strpairs.begin(); 
	    iitr != strpairs.end(); ++iitr, ++ciitr) {
		cout<<"\n"<<iitr->first<<"\t"<<iitr->second;
		cout<<"\n"<<ciitr->first<<"\t"<<ciitr->second;
		check_expr(strcmp(ciitr->first, iitr->first) == 0 && 
		    strcmp(ciitr->second, iitr->second) == 0);
	}

	typedef db_map<char *, char *, ElementHolder<char *> > strpairs_t;
	typedef std::map<string, string> sstrpairs_t;
	sstrpairs_t sstrpairs2;
	strpairs_t strpairs2;
	rand_str_dbt randstr;
	
	for (i = 0; i < 100; i++) {
		string rdstr, rdstr2;
		randstr(dbt, rdstr);
		randstr(dbt, rdstr2);
		strpairs2[(char *)rdstr.c_str()] = (char *)rdstr2.c_str();
		sstrpairs2[rdstr] = rdstr2;
	}
	strpairs_t::iterator itr;
	strpairs_t::const_iterator citr;
	
	for (itr = strpairs2.begin(); itr != strpairs2.end(); ++itr) {
		check_expr(strcmp(strpairs2[itr->first], itr->second) == 0);
		check_expr(string(itr->second) == 
		    sstrpairs2[string(itr->first)]);
		strpairs_t::value_type_wrap::second_type&secref = itr->second;
		std::reverse((char *)secref, (char *)secref + strlen(secref));
		secref._DB_STL_StoreElement();
		std::reverse(sstrpairs2[itr->first].begin(), 
		    sstrpairs2[itr->first].end());
	}

	check_expr(strpairs2.size() == sstrpairs2.size());
	for (citr = strpairs2.begin(
	    ReadModifyWriteOption::no_read_modify_write(), 
	    true, BulkRetrievalOption::bulk_retrieval()); 
	    citr != strpairs2.end(); ++citr) {
		check_expr(strcmp(strpairs2[citr->first], citr->second) == 0);
		check_expr(string(citr->second) == 
		    sstrpairs2[string(citr->first)]);
	}

	
	if (explicit_txn) 
		dbstl::commit_txn(penv);

	db_vector<const char *, ElementHolder<const char *> > csvct(10);
	vector<const char *> scsvct(10);
	const char *pconststr = "abc";
	for (i = 0; i < 10; i++) {
		scsvct[i] = pconststr;
		csvct[i] = pconststr;
		csvct[i] = scsvct[i];
		// scsvct[i] = csvct[i]; assignment won't work because scsvct 
		// only stores pointer but do not copy the sequence, thus it 
		// will refer to an invalid pointer when i changes.
	}
	for (i = 0; i < 10; i++) {
		check_expr(strcmp(csvct[i], scsvct[i]) == 0);
		cout<<endl<<(const char *)(csvct[i]);
	}

	db_vector<const wchar_t *, ElementHolder<const wchar_t *> > wcsvct(10);
	vector<const wchar_t *> wscsvct(10);
	const wchar_t *pconstwstr = L"abc";
	for (i = 0; i < 10; i++) {
		wscsvct[i] = pconstwstr;
		wcsvct[i] = pconstwstr;
		wcsvct[i] = wscsvct[i];
		// scsvct[i] = csvct[i]; assignment won't work because scsvct 
		// only stores pointer but do not copy the sequence, thus it 
		// will refer to an invalid pointer when i changes.
	}
	for (i = 0; i < 10; i++) {
		check_expr(wcscmp(wcsvct[i], wscsvct[i]) == 0);

	}
	
} // char_star_string_storage

void StlAdvancedFeaturesExample::arbitray_sequence_storage()
{
	int i, j;

	if (explicit_txn) 
		dbstl::begin_txn(0, penv);
	// storing arbitary sequence test .  
	cout<<endl<<"Arbitary type of sequence storage support.\n";
	RGBB *rgbs[10], *prgb1, *prgb2;
	typedef db_map<int, RGBB *, ElementHolder<RGBB *> > rgbmap_t;
	rgbmap_t rgbsmap(dmdb6, penv);
	
	map<int, RGBB *> srgbsmap;

	DbstlElemTraits<RGBB>::instance()->set_sequence_len_function(rgblen);
	DbstlElemTraits<RGBB>::instance()->set_sequence_copy_function(rgbcpy);
	// populate srgbsmap and rgbsmap
	for (i = 0; i < 10; i++) {
		n = abs(rand()) % 10 + 2;
		rgbs[i] = new RGBB[n];
		memset(&rgbs[i][n - 1], 0, sizeof(RGBB));//make last element 0
		for (j = 0; j < n - 1; j++) {
			rgbs[i][j].r_ = i + 128;
			rgbs[i][j].g_ = 256 - i;
			rgbs[i][j].b_ = 128 - i;
			rgbs[i][j].bright_ = 256 / (i + 1);
			
		}
		rgbsmap.insert(make_pair(i, rgbs[i]));
		srgbsmap.insert(make_pair(i, rgbs[i]));
	}

	// retrieve and assert equal, then modify and store
	for (i = 0; i < 10; i++) {
		rgbmap_t::value_type_wrap::second_type rgbelem = rgbsmap[i];
		prgb1 = rgbelem;
		check_expr(memcmp(prgb1, prgb2 = srgbsmap[i], 
		    (n = (int)rgblen(srgbsmap[i])) * sizeof(RGBB)) == 0);
		for (j = 0; j < n - 1; j++) {
			prgb1[j].r_ = 256 - prgb1[j].r_;
			prgb1[j].g_ = 256 - prgb1[j].g_;
			prgb1[j].b_ = 256 - prgb1[j].b_;
			prgb2[j].r_ = 256 - prgb2[j].r_;
			prgb2[j].g_ = 256 - prgb2[j].g_;
			prgb2[j].b_ = 256 - prgb2[j].b_;
		}
		rgbelem._DB_STL_StoreElement();
	}

	// retrieve again and assert equal
	for (i = 0; i < 10; i++) {
		rgbmap_t::value_type_wrap::second_type rgbelem = rgbsmap[i];
		// Can't use rgbsmap[i] here because container::operator[] is 
		// an temporary value.;
		prgb1 = rgbelem;
		check_expr(memcmp(prgb1, prgb2 = srgbsmap[i], 
		    sizeof(RGBB) * rgblen(srgbsmap[i])) == 0);
	}
	
	rgbmap_t::iterator rmitr;
	map<int, RGBB *>::iterator srmitr;

	for (rmitr = rgbsmap.begin();
	    rmitr != rgbsmap.end(); ++rmitr) {
		rgbmap_t::value_type_wrap::second_type 
		    rgbelem2 = (*rmitr).second;
		prgb1 = (*rmitr).second;
		srmitr = srgbsmap.find(rmitr->first);
		rmitr.refresh();
	}

	for (i = 0; i < 10; i++)
		delete []rgbs[i];
	if (explicit_txn)
		dbstl::commit_txn(penv);
} // arbitray_sequence_storage

void StlAdvancedFeaturesExample::bulk_retrieval_read()
{

	int i;
	
	typedef db_map<int, sms_t> smsmap_t;
	smsmap_t smsmap(dmdb6, penv);
	map<int, sms_t> ssmsmap;
	if (explicit_txn) 
		dbstl::begin_txn(0, penv);
		
	cout<<"\nBulk retrieval support:\n";
	sms_t smsmsg;
	time_t now;
	smsmap.clear();
	for (i = 0; i < 2008; i++) {
		smsmsg.from = 1000 + i;
		smsmsg.to = 10000 - i;
		smsmsg.sz = sizeof(smsmsg);
		time(&now);
		smsmsg.when = now;
		ssmsmap.insert(make_pair(i, smsmsg));
		smsmap.insert(make_pair(i, smsmsg));
	}

	// bulk retrieval test. 
	map<int, sms_t>::iterator ssmsitr = ssmsmap.begin();
	i = 0;
	const smsmap_t &rosmsmap = smsmap;
	smsmap_t::const_iterator smsitr;
	for (smsitr = rosmsmap.begin(
	    BulkRetrievalOption::bulk_retrieval());
	    smsitr != smsmap.end(); i++) {
		// The order may be different, so if the two key set are 
		// identical, it is right.
		check_expr((ssmsmap.count(smsitr->first) == 1)); 
		check_expr((smsitr->second == ssmsmap[smsitr->first]));
		if (i % 2)
			smsitr++;
		else 
			++smsitr; // Exercise both pre/post increment.
		if (i % 100 == 0)
			smsitr.set_bulk_buffer(
			    (u_int32_t)(smsitr.get_bulk_bufsize() * 1.1));
	}

	smsmap.clear();
	ssmsmap.clear();

	// Using db_vector. when moving its iterator sequentially to end(),
	// bulk retrieval works, if moving randomly, it dose not function
	// for db_vector iterators. Also, note that we can create a read only
	// iterator when using db_vector<>::iterator rather than 
	// db_vector<>::const_iterator.
	db_vector<sms_t> vctsms;
	db_vector<sms_t>::iterator itrv;
	vector<sms_t>::iterator sitrv;
	vector<sms_t> svctsms;
	for (i = 0; i < 2008; i++) {
		smsmsg.from = 1000 + i;
		smsmsg.to = 10000 - i;
		smsmsg.sz = sizeof(smsmsg);
		time(&now);
		smsmsg.when = now;
		vctsms.push_back(smsmsg);
		svctsms.push_back(smsmsg);
	}

	for (itrv = vctsms.begin(ReadModifyWriteOption::no_read_modify_write(), 
	    true, BulkRetrievalOption::bulk_retrieval(64 * 1024)), 
	    sitrv = svctsms.begin(), i = 0; itrv != vctsms.end();
	    ++itrv, ++sitrv, ++i) {
		check_expr(*itrv == *sitrv);
		if (i % 100 == 0)
			itrv.set_bulk_buffer(
			    (u_int32_t)(itrv.get_bulk_bufsize() * 1.1));
	}

	if (explicit_txn)
		dbstl::commit_txn(penv);

	
} //bulk_retrieval_read

void StlAdvancedFeaturesExample::primitive()
{

	int i;

	if ( explicit_txn) 
		dbstl::begin_txn(0, penv);
	

	db_vector<int, ElementHolder<int> > ivi(dbprim, penv);
	vector<int> spvi4;
	fill(ivi, spvi4);
	check_expr(is_equal(ivi, spvi4));
	ivi.clear(false);

	db_vector<int, ElementHolder<int> > ivi2(dbprim, penv);
	vector<int> spvi5;
	fill(ivi2, spvi5);
	check_expr(is_equal(ivi2, spvi5));
	size_t vsz = ivi2.size();
	for (i = 0; i < (int)vsz - 1; i++) {
		ivi2[i] += 3;
		ivi2[i]--;
		ivi2[i] <<= 2;
		ivi2[i] = (~ivi2[i] | ivi2[i] & ivi2[i + 1] ^ 
		    (2 * (-ivi2[i + 1]) + ivi2[i]) * 3) / 
		    (ivi2[i] * ivi2[i + 1] + 1);

		spvi5[i] += 3;
		spvi5[i]--;
		spvi5[i] <<= 2;
		spvi5[i] = (~spvi5[i] | spvi5[i] & spvi5[i + 1] ^ 
		    (2 * (-spvi5[i + 1]) + spvi5[i]) * 3) / 
		    (spvi5[i] * spvi5[i + 1] + 1);
	}
	check_expr(is_equal(ivi2, spvi5));
	ivi2.clear(false);

	typedef db_vector<double, ElementHolder<double> > dbl_vct_t;
	dbl_vct_t dvi(dbprim, penv);
	vector<double> dsvi;
	for (i = 0; i < 10; i++) {
		dvi.push_back(i * 3.14);
		dsvi.push_back(i * 3.14);
	}
	check_expr(is_equal(dvi, dsvi));

	dbl_vct_t::iterator ditr;
	vector<double>::iterator sditr;
	for (ditr = dvi.begin(), sditr = dsvi.begin(); 
	    ditr != dvi.end(); ++ditr, ++sditr){
		*ditr *= 2;
		*sditr *= 2;
	}

	check_expr(is_equal(dvi, dsvi));

	for (i = 0; i < 9; i++) {
		dvi[i] /= (-dvi[i] / 3 + 2 * dvi[i + 1]) / (1 + dvi[i]) + 1;
		dsvi[i] /= (-dsvi[i] / 3 + 2 * dsvi[i + 1]) / (1 + dsvi[i]) + 1;
	}

	cout<<"\ndsvi after math operations: \n";
	for (i = 0; i <= 9; i++)
		cout<<dsvi[i]<<"  ";
	for (i = 0; i < 10; i++) {
		cout<<i<<"\t";
		check_expr((int)(dvi[i] * 100000) == (int)(dsvi[i] * 100000));
	}

	if ( explicit_txn) 
		dbstl::commit_txn(penv);
} // primitive

void StlAdvancedFeaturesExample::queue_stack()
{
	int i;

	if ( explicit_txn) 
		dbstl::begin_txn(0, penv);
	// test whether db_vector works with std::queue, 
	// std::priority_queue and std::stack
	cout<<"\ndb_vector working with std::queue\n";
	
	// std::queue test
	intvec_t quev(quedb, penv);
	quev.clear();
	std::queue<ptint, intvec_t> intq(quev);
	std::queue<ptint> sintq;
	check_expr(intq.empty());
	check_expr(intq.size() == 0);
	for (i = 0; i < 100; i++) {
		intq.push(ptint(i));
		sintq.push(i);
		check_expr(intq.front() == 0);
		check_expr(intq.back() == i);
	}
	check_expr(intq.size() == 100);
	for (i = 0; i < 100; i++) {
		check_expr(intq.front() == i);
		check_expr(intq.back() == 99);
		check_expr(intq.front() == sintq.front());
		check_expr(sintq.back() == intq.back());
		sintq.pop();
		intq.pop();
	}
	check_expr(intq.size() == 0);
	check_expr(intq.empty());
	quev.clear();

	// std::priority_queue test
	cout<<"\ndb_vector working with std::priority_queue\n";

	std::vector<ptint> squev;
	intvec_t pquev(pquedb, penv);
	pquev.clear();
	std::priority_queue<ptint, intvec_t, ptintless_ft> 
	    intpq(ptintless, pquev);
	std::priority_queue<ptint, vector<ptint>, ptintless_ft> 
	    sintpq(ptintless, squev);

	check_expr(intpq.empty());
	check_expr(intpq.size() == 0);
	ptint tmppq, tmppq1;
	set<ptint> ptintset;
	for (i = 0; i < 100; i++) {
		for (;;) {// avoid duplicate
			tmppq = rand();
			if (ptintset.count(tmppq) == 0) {
				intpq.push(tmppq);		
				sintpq.push(tmppq);
				ptintset.insert(tmppq);
				break;
			} 
		}

	}
	check_expr(intpq.empty() == false);
	check_expr(intpq.size() == 100);
	for (i = 0; i < 100; i++) {
		tmppq = intpq.top();
		tmppq1 = sintpq.top();
		if (i == 98 && tmppq != tmppq1) {
			tmppq = intpq.top();
		}
		if (i < 98)
			check_expr(tmppq == tmppq1);
		if (i == 97)
			intpq.pop();
		else
			intpq.pop();
		sintpq.pop();
	}
	check_expr(intpq.empty());
	check_expr(intpq.size() == 0);


	// std::stack test
	cout<<"\ndb_vector working with std::stack\n";
	std::stack<ptint, intvec_t> intstk(quev);
	std::stack<ptint> sintstk;
	check_expr(intstk.size() == 0);
	check_expr(intstk.empty());
	for (i = 0; i < 100; i++) {
		intstk.push(ptint(i));
		sintstk.push(ptint(i));
		check_expr(intstk.top() == i);
		check_expr(intstk.size() == (size_t)i + 1);
	}
	
	for (i = 99; i >= 0; i--) {
		check_expr(intstk.top() == ptint(i));
		check_expr(intstk.top() == sintstk.top());
		sintstk.pop();
		intstk.pop();
		check_expr(intstk.size() == (size_t)i);
	}
	check_expr(intstk.size() == 0);
	check_expr(intstk.empty());
	
	// Vector with no handles specified. 
	ptint_vector simple_vct(10);
	vector<ptint> ssvct(10);
	for (i = 0; i < 10; i++) {
		simple_vct[i] = ptint(i);
		ssvct[i] = ptint(i);
	}
	check_expr(is_equal(simple_vct, ssvct));

	if ( explicit_txn)
		dbstl::commit_txn(penv);
	
	return;
} // queue_stack

DbEnv *g_env;
int g_test_start_txn;

int main(int argc, char *argv[])
{
	int c, ret;
	char *envhome = NULL, *mode = NULL;
	int flags = DB_THREAD, setflags = 0, explicit_txn = 0;
	int test_autocommit = 1, totalinsert = 100, verbose = 0;
	DBTYPE dbtype = DB_BTREE;
	int shortest = 50, longest = 200;
	u_int32_t cachesize = 8 * 1024 * 1024;
	DbEnv *penv = NULL;

	TestParam *ptp = new TestParam;

	while ((c = getopt(argc, argv, "T:c:hH:k:l:m:n:r:s:t:v")) != EOF) {
		switch (c) {
			case 'T':
				totalinsert = atoi(optarg);
				break;
			case 'c':
				cachesize = atoi(optarg);
				break;
			case 'h':
				usage();
				return 0;
				break;
			case 'H':
				envhome = strdup(optarg);
				break;
			case 'k':
				shortest = atoi(optarg);
				break;
			case 'l':
				longest = atoi(optarg);
				break;
			case 'm':
				mode = optarg;
				break;
			case 's': // db type for associative containers
				if (*optarg == 'h') // hash db type
					dbtype = DB_HASH;
				else if (*optarg == 'b')
					dbtype = DB_BTREE;
				else
					usage();
				break;
			case 't':
				explicit_txn = 1;
				if (*optarg == 'a') 
					setflags = DB_AUTO_COMMIT;
				else if (*optarg == 'e') // explicit txn
					test_autocommit = 0;
				else 
					usage();
				break;
			case 'v':
				verbose = 1;
				break;
			default:
				usage();
				break;
		}
	}

	if (mode == NULL) 
		flags |= 0;
	else if (*mode == 'c') //cds
		flags |= DB_INIT_CDB;
	else if (*mode == 't')
		flags |= DB_INIT_TXN | DB_RECOVER 
		    | DB_INIT_LOG | DB_INIT_LOCK;
	else
		flags |= 0;

	ptp->explicit_txn = explicit_txn;
	ptp->flags = flags;
	ptp->dbtype = dbtype;
	ptp->setflags = setflags;
	ptp->test_autocommit = test_autocommit;
	ptp->dboflags = DB_THREAD;

	// Call this method before any use of dbstl.
	dbstl_startup();
	penv = new DbEnv(DB_CXX_NO_EXCEPTIONS);
	BDBOP(penv->set_flags(setflags, 1), ret);
	BDBOP(penv->set_cachesize(0, cachesize, 1), ret);

	// Methods of containers returning a reference costs 
	// locker/object/lock slots.
	penv->set_lk_max_lockers(10000); 
	penv->set_lk_max_objects(10000);
	penv->set_lk_max_locks(10000);

	penv->set_flags(DB_TXN_NOSYNC, 1);

	BDBOP(penv->open(envhome, 
	    flags | DB_CREATE | DB_INIT_MPOOL, 0777), ret);
	register_db_env(penv);
	ptp->dbenv = penv;
	g_env = penv;
	g_test_start_txn = test_autocommit * explicit_txn;

	// Create the example and run.
	StlAdvancedFeaturesExample example(ptp);
	example.run();

	// Clean up and exit.
	delete ptp;
	dbstl_exit();// Call this method before the process exits.
	if (penv) 
		delete penv;
}

void usage()
{
	cout<<
		"\nUsage: StlAdvancedFeaturesExample\n\
[-c number		cache size]\n\
[-h			print this help message then exit]\n\
[-H dir			Specify dir as environment home]\n\
[-k number		shortest string inserted]\n\
[-l number		longest string inserted]\n\
[-m ds/cds/tds		use ds/cds/tds product]\n\
[-s b/h			use btree/hash type of DB for assocative \
containers] \n\
[-t a/e			for tds, use autocommit/explicit transaction in \
the test] \n\
[-v			verbose mode, output more info in multithread \
test]\n\
";
}

void using_charstr(TCHAR*str)
{
	cout<<_T("using str read only with non-const parameter type:")<<str;
	str[0] = _T('U');
	str[1] = _T('R');
}

int get_dest_secdb_callback(Db * /* secondary */, const Dbt * /* key */, 
    const Dbt *data, Dbt *result)
{
	SMSMsg *p = (SMSMsg*)data->get_data();

	result->set_data(&(p->to));
	result->set_size(sizeof(p->to));
	return 0;
}

void SMSMsgRestore(SMSMsg2& dest, const void *srcdata)
{
	char *p = dest.msg;

	memcpy(&dest, srcdata, sizeof(dest));

	dest.msg = (char *)DbstlReAlloc(p, dest.szmsg);
	strcpy(dest.msg, (char*)srcdata + sizeof(dest));
}

u_int32_t SMSMsgSize(const SMSMsg2& elem)
{
	return (u_int32_t)(sizeof(elem) + strlen(elem.msg) + 1);
}

void SMSMsgCopy(void *dest, const SMSMsg2&elem)
{
	memcpy(dest, &elem, sizeof(elem));
	strcpy((char*)dest + sizeof(elem), elem.msg);
}

u_int32_t rgblen(const RGBB *seq)
{
	size_t s = 0;
	
	const RGBB *p = seq, rgb0;
	for (s = 0, p = seq; memcmp(p, &rgb0, sizeof(rgb0)) != 0; p++, s++);
	// this size includes the all-0 last element used like '\0' 
	// for char* strings 
	return (u_int32_t)(s + 1);
}

// The seqs sequence of RGBB objects may not reside in a consecutive chunk of 
// memory but the seqd points to a consecutive chunk of mem large enough to 
// hold all objects from seqs.
void rgbcpy(RGBB *seqd, const RGBB *seqs, size_t num)
{
	const RGBB *p = seqs;
	RGBB rgb0;
	RGBB *q = seqd;

	memset((void *)&rgb0, 0, sizeof(rgb0));
	for (p = seqs, q = seqd; memcmp(p, &rgb0, sizeof(rgb0)) != 0 && 
	    num > 0; num--, p++, q++)
		memcpy((void *)q, p, sizeof(RGBB));
	memcpy((void *)q, p, sizeof(RGBB));// append trailing end token.
}

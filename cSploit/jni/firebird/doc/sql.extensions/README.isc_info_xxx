New information items

Author:
	Vlad Khorsun <hvlad at users.sourceforge.net>

New items for isc_database_info

1. isc_info_active_tran_count : 
	return number of currently active transactions

2. isc_info_creation_date :
	return date and time when database was created
	To decode call isc_vax_integer twice to extract (first) date
	and (second) time portions of ISC_TIMESTAMP. Then use
	isc_decode_timestamp as usual

3. fb_info_page_contents :
	return contents of some database page

	Query format is <fb_info_page_contents> <length> <page_number>, where 
		<length> is 2-byte length of <page_number> 
		<page_number> is 4-byte number of requested page. 
	In the future it will be possible to pass 8-byte page_number value 
	including both page number and page space number.

	Response format is <fb_info_page_contents, length, page_content>, where 
		<length> is 2-byte length of <page_content> and is equal to database 
				 page size

	This feature is allowed only for SYSDBA or database owner for security 
	reasons. Introduced in Firebird v2.5.

	See also CORE-2054.



New items for isc_transaction_info:
	
1. isc_info_tra_oldest_interesting :
	return number of oldest interesting transaction when current
	transaction started. For snapshot transactions this is also the 
	number of oldest transaction in the private TIP copy

2. isc_info_tra_oldest_active
	for read-committed transaction return number of current transaction 
	for other transactions return number of oldest active transaction 
	when current transaction started  

3. isc_info_tra_oldest_snapshot
	return minimum number of tra_oldest_active of all active transactions
	when current transaction started. 
	This value is used as garbage collection threshold

4. isc_info_tra_isolation
	return transaction isolation mode of current transaction. 
	format of returned clumplets is following:
	
	isc_info_tra_isolation, 
		1, isc_info_tra_consistency | isc_info_tra_concurrency
	|
		2, isc_info_tra_read_committed, 
			 isc_info_tra_no_rec_version | isc_info_tra_rec_version
	
	i.e. for read committed transactions returned 2 items while for 
	other transactions returned 1 item

5. isc_info_tra_access
	return read-write access mode of current transaction.
	format of returned clumplets is following:
	
	isc_info_tra_access, 1, isc_info_tra_readonly | isc_info_tra_readwrite

6. isc_info_tra_lock_timeout
	return lock timeout of current transaction

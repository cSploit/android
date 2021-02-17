		New utility fbsvcmgr in firebird 2.1.

	Firebird (like interbase 6 and before) never had a way to access service from command line. 
With the exception of -service switch of gbak and total use of services API in gsec since FB2, 
to use services one had to use third party GUI or write himself a program using C or other 
programming language. Use of GUI is almost always not a problem, when you work with local machine 
or machine in your LAN. But in case, when you connect to remote unix server using any text-only 
connection, use of services is almost impossible. And this is really a problem sometimes.

	Utility fbsvcmgr solves this problem. With it you may use any service, implemented by firebird. 
To use this utility you should be familiar with firebird services API - fbsvcmgr does NOT emulate 
traditional utilities switches, this is just frontend with services API. The first required 
parameter of command line is services manager you want to connect to. For local connection use 
simply service_mgr, to attach to remote machine something like hostname:service_mgr is OK. Later 
service parameter blocks (SPB) with values, when required, follow. Any of them may (or may not) 
be prefixed with single '-' sign. For long command lines, typical for fbsvcmgr, use of '-' makes 
command line better human-readable. Compare, please:
# fbsvcmgr service_mgr user sysdba password masterke action_db_stats dbname employee sts_hdr_pages
and
# fbsvcmgr service_mgr -user sysdba -password masterke -action_db_stats -dbname employee -sts_hdr_pages

Syntax of service parameter blocks, understood by fbsvcmgr, almost exactly match with one you may 
see in ibase.h include file or Borland InterBase 6.0 API documentation. To save typing and make 
command line a bit shorter slightly abbreviated form is used. All SPB parameters have one of two 
forms: isc_spb_VALUE or isc_VALUE1_svc_VALUE2. Accordingly in first case you should type simply 
VALUE, and for the second - VALUE1_VALUE2. For example:
	isc_spb_dbname => dbname
	isc_action_svc_backup => action_backup
	isc_spb_sec_username => sec_username
	isc_info_svc_get_env_lock => info_get_env_lock
and so on. Exception is one you see in the sample above - isc_spb_user_name may be specified as 
user_name and simply user.

	Describing of all SPB parameters is not real for README or release notes - it's about 40 pages 
in InterBase 6.0 beta documentation. Therefore here only known differences between that 
documentation and real life are noticed.

1. Using fbsvcmgr you may perform single action (and get results of it's execution when available) 
or get multiple information items from services manager. For example:
# fbsvcmgr service_mgr -user sysdba -password masterke -action_display_user
will list all users of local firebird server:
SYSDBA                       Sql Server Administrator                    0    0
QA_USER1                                                                 0    0
QA_USER2                                                                 0    0
QA_USER3                                                                 0    0
QA_USER4                                                                 0    0
QA_USER5                                                                 0    0
GUEST                                                                    0    0
SHUT1                                                                    0    0
SHUT2                                                                    0    0
QATEST                                                                   0    0
And:
# fbsvcmgr service_mgr -user sysdba -password masterke -info_server_version -info_implementation
will report both server version and it's implemenation:
Server version: LI-T2.1.0.15740 Firebird 2.1 Alpha 1
Server implementation: Firebird/linux AMD64
But attempt to mix all of this in single command line:
# fbsvcmgr service_mgr -user sysdba -password masterke -action_display_user -info_server_version -info_implementation
raises an error:
Unknown switch "-info_server_version"

2. Some parameters have buggy form in Borland beta documentation. When in trouble, consult ibase.h 
first for correct form.

3. Everything concerning licensing was removed from interbase 6.0 and therefore not supported here. 
Config file view/modification is not supported in firebird since 1.5 and therefore is not implemented 
here. 

4. isc_spb_rpr_list_limbo_trans was forgotten in Borland beta documentation, but present in fbsvcmgr.

5. Two new items, not mentioned by Borland beta documentation, were added to firebird 2.1 and are 
supported by fbsvcmgr. One of them works only for windows, this is isc_spb_trusted_auth, which (type 
as trusted_auth) forces use of windows trusted authentication by firebird. Another one is 
availability to set (isc_spb_)dbname parameter in all security database related actions, it's an 
equivalent of -database switch of gsec utility. Notice - in gsec this switch is mostly used to 
specify remote server you want to administer. In fbsvcmgr name of server is already given in services 
manager name (first parameter), therefore this parameter in most cases is not needed.

6. Items added in firebird 2.5:
- bkp_no_triggers - specify it to avoid executing database-wide triggers.

				Author: Alex Peshkov, <peshkoff at mail.ru>

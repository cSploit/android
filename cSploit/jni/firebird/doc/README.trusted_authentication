		New way to authenticate users in firebird.

	Firebird starting with version 2.1 can use Windows security for user authentication.
Current security context is passed to the server and if it's OK for that server is used to determine
firebird user name. To use Windows trusted authentication, do not put user and password parameters
in DPB/SPB. This will automatically lead to use of trusted authentication in almost all cases (see
environment below for exceptions). Suppose you have logged to the Windows server SRV as user John.
If you connect to server SRV with isql, not specifying Firebird login and password:

isql srv:employee

and do:

SELECT CURRENT_USER FROM RDB$DATABASE;

you will get something like:

USER
====================================================
SRV\John

Windows users may be granted rights to access database objects and roles in the same way as
traditional Firebird users. (This is not something new - in UNIX OS users might be granted rights
virtually always).

- If domain administrator (member of well known predefined groups) connects to Firebird using trusted
authentication, he/she may be granted 'god-like' (SYSDBA) rights depending upon settings in database, 
to which such user attachs. To keep CURRENT_USER value in a form DOMAIN\User, a new object (predefined 
system role) is added to the database. The name of that role is RDB$ADMIN, and any user, granted it, 
can attach to the database with SYSDBA rights. To configure database to auto-grant that role to
administrators, use the following command:

ALTER ROLE RDB$ADMIN SET AUTO ADMIN MAPPING;

To return to default settings (windows administrators are not granted special rights) issue:

ALTER ROLE RDB$ADMIN DROP AUTO ADMIN MAPPING;

Take into an account, that if Windows administrator attaches with role set in dpb, it will not be 
replaced with RDB$ADMIN, i.e. he/she will not get SYSDBA rights.

- New parameter is added to firebird.conf - it is used to select available authentication method.
Parameter is called Authentication and may have values Native, Trusted and Mixed. Default is
mixed authentication. Using native method you get full compatibility with previous Firebird versions,
avoiding trusted authentication. In trusted-only case security database is ignored and only Windows
authentication is used (in some aspects this is the most secure way, i.e. it is exactly as secure as
host OS). 

- To keep legacy behavior when ISC_USER/ISC_PASSWORD variables are set in environment, they
are picked and used instead of trusted authentication. In case when trusted authentication is needed
and ISC_USER/ISC_PASSWORD are set, add new DPB parameter isc_dpb_trusted_auth to DPB. In most
of Firebird command line utilities switch -trusted (may be abbreviated up to utility rules) is used
for it. Exceptions for today are qli (it uses single-letter switches, switch of interest is -K) and
nbackup (also has single-letter switches, force of trusted authentication over environment is
not implemented yet). Example:

isql srv:db                -- log using trusted authentication
set ISC_USER=user1
set ISC_PASSWORD=12345
isql srv:db                -- log as 'user1' from environment
isql -trust srv:db         -- log using trusted authentication

PS. There are plans to significantly extend abilities to map OS users/groups to database users/roles
in future versions.

				Author: Alex Peshkov, <peshkoff at mail.ru>

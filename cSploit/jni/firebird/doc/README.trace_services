		
		Trace and audit services.

	Firebird 2.5 offers new trace and audit facilities. These new abilities allow
to log various events performed inside the engine (such as statement execution,
connect\disconnect etc) and to collect corresponding performance characteristics
in real time. The base term is trace session. Each trace session has its own
configuration, state and output.

	List of available events to trace is fixed and determined by the Firebird
engine. List of events to trace, which data items to trace and placement of
trace output is specified by trace configuration when trace session is created.

	There are two kinds of trace sessions : system audit and user trace.
	
	System audit session is started by the engine itself and obtains configuration
from the text file. The name of this file is set via new setting in firebird.conf
("AuditTraceConfigFile") and by default has empty value, i.e. no system audit
is configured. There may be only one system audit trace session, obviously.
Configuration file contains list of traced events and placement of trace log(s).
It is very flexible and allows to log different set of events for different
databases into different log files. Example configuration file fbtrace.conf is
placed in the Firebird root directory and contains full description of its
format, rules and syntax.

	User trace session is managed by user via Services API. There are five new
services for this purpose - to start, stop, suspend, resume trace session and
to list all known trace sessions. See README.services_extension for details.

	When user application starts trace session it sets (optional) session name
and (mandatory) session configuration. Session configuration is a text. Rules
and syntax are the same as for configuration file mentioned above except of
placement of trace output. Engine stores output of user session in set of
temporary files (1MB each) and deletes it automatically when file was read
completely. Maximum size of user session's output is limited and set via
"MaxUserTraceLogSize" setting in firebird.conf. Default value is 10 MB.

	After application starts user trace session, it must read session's output
from service (using isc_service_query). Session output could be produced by the
engine faster than application is able to read it. When session output grows
more than "MaxUserTraceLogSize" engine automatically suspends this trace
session. When application reads part of the output so output size stay less than
"MaxUserTraceLogSize" engine automatically resumes trace session.

	When application decides to stop its trace session it just does detach from
service. Also there is ability to manage trace sessions (suspend\resume\stop).
Administrators are allowed to manage any trace session while ordinary users are 
allowed to manage their own trace sessions only.

    If user trace session was created by ordinary user it will trace only
connections established by this user.

    User trace sessions are not preserved when all Firebird processes are stopped.
I.e. if user started trace session and Firebird SS or SC process is shut down,
this session is stopped and will not be restarted with new Firebird process. For
CS this is not the case as user trace session can't live without connection with
service manager and dedicated CS process.


	There is new Trace API introduced for use with trace facilities. This API is
not stable and will be changed in next Firebird releases, therefore it is not
officially published. Trace API is a set of hooks which is implemented by external
trace plugin module and called by the engine when any traced event happens. Trace
plugin is responsible for logging these events in some form.

	There is "standard" implementation of trace plugin, fbtrace.dll (.so) located
in \plugins folder.
	

	There is new specialized standalone utility to work with trace services : 
fbtracemgr. It has the following command line switches :

Action switches :
  -STA[RT]                              Start trace session
  -STO[P]                               Stop trace session
  -SU[SPEND]                            Suspend trace session
  -R[ESUME]                             Resume trace session
  -L[IST]                               List existing trace sessions

Action parameters switches :
  -N[AME]    <string>                   Session name
  -I[D]      <number>                   Session ID
  -C[ONFIG]  <string>                   Trace configuration file name

Connection parameters switches :
  -SE[RVICE]  <string>                  Service name
  -U[SER]     <string>                  User name
  -P[ASSWORD] <string>                  Password
  -FE[TCH]    <string>                  Fetch password from file
  -T[RUSTED]  <string>                  Force trusted authentication

Also, it prints usage screen if run without parameters.


	Examples
	
I. Sample configuration files for user trace sessions:
	
a) Trace prepare, free and execution of all statements within connection 12345

<database>
	enabled			true
	connection_id		12345
	log_statement_prepare	true
	log_statement_free	true
	log_statement_start	true
	log_statement_finish	true
	time_threshold		0
</database>

b) Trace all connections of given user to database mydatabase.fdb
   Log executed INSERT, UPDATE and DELETE statements, nested calls to procedures
   and triggers and show corresponding PLAN's and performance statistics.

<database %[\\/]mydatabase.fdb>
	enabled			true
	include_filter		(%)(INSERT|UPDATE|DELETE)(%)
	log_statement_finish	true
	log_procedure_finish	true
	log_trigger_finish	true
	print_plan		true
	print_perf		true
	time_threshold		0
</database>
	
c) Trace connections and transactions in all databases except of security database

<database>
	enabled			true
	log_connections		true
	log_transactions	true
</database>

<database security2.fdb>
	enabled			false
</database>



II. Working with fbtracemgr :

a) Start user trace named "My trace" using configuration file fbtrace.conf and read
   its output on the screen :

	fbtracemgr -se service_mgr -start -name "My trace" -config fbtrace.conf
	
	To stop this trace session press Ctrl+C at fbtracemgr console window. Or, in
another console : list sessions and look for interesting session ID (b) and stop it 
using this found ID (e).

b) List trace sesions

	fbtracemgr -se service_mgr -list
	
c) Suspend trace sesson with ID 1

	fbtracemgr -se service_mgr -suspend -id 1
	
d) Resume trace sesson with ID 1

	fbtracemgr -se service_mgr -resume -id 1
	
e) Stop trace sesson with ID 1

	fbtracemgr -se service_mgr -stop -id 1
	


    There are three general use cases :

1. Constant audit of engine.

    This is served by system audit trace. Administrator edits (or creates new)
trace configuration file, sets its name in firebird.conf (AuditTraceConfigFile
setting) and restarts Firebird. Later, administrator could suspend\resume\stop
this session without need to restart Firebird. To make audit configuration
changes accepted by the engine, Firebird needs to be restarted.

2. On demand interactive trace of some (or all) activity in some (or all)
   databases.

    Application starts user trace session, reads its output and shows traced events
to user in real time on the screen. User could suspend\resume trace and at last
stop it.

3. Collect some engine activity for a relatively long period of time (few hours
   or even whole day) to analyse it later.

    Application starts user trace session and reads and saves trace output to a file
(or set of files). Session must be stopped manually by the same application or
by another one.

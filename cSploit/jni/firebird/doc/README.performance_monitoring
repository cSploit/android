	Performance analysis with Firebird.
	Document version 0.2
	Created by Nickolay Samofatov

Performance monitoring framework is consisted of 2 parts:
- Trace API
- Reference implementation of analysis plugin


Trace API.

	Trace API allows to plug a monitoring module into the engine and notify it of
processes happening there. List of analysis plugins is specified in firebird.conf 
parameter TracePlugins. Each trace plugin receives notifications on each database
opened by the engine as a call of procedure having following signature:

NTRACE_BOOLEAN trace_attach(const char* db_filename, const TracePlugin** plugin);

Conceptually TracePlugin is the plugin trace interface. It is implemented as a
structure of function pointers and void* object pointer to be passed as first 
argument for each function.

Given this interface plugin may install hooks for particular events and receive 
notifications for them. If plugin has interest in some kind of events for given 
database it sets plugin pointer to a structure containing non-null hook function 
pointers and returns NTRACE_TRUE value. Otherwise it should return NTRACE_FALSE.

One category of hooks receives the same parameters as corresponding Y-Valve 
(public API) methods plus internal objects identifiers, generated PLANs for 
statements and performance counters. One more hook receives performance 
statistics for each procedure execution.

Each hook function returns boolean value indicating success or failure. When hook
function fails engine collects error message from plugin in the same thread 
context as hook was called, writes message to firebird.log and stops calling
hooks for this particular database and trace plugin.

See jrd/ntrace.h module for definitions of structures and events supported.


Default analysis module.

	Reference implementation of analysis plugin is centered around a concept of 
performance snapshot. Snapshot detail may vary, but in any case it contains
information on all activities performed by server for database during a given 
period of time, possibly in aggregated form. All snapshots may be compared to 
allow quantitative measurements of performance improvements due to various 
optimization activities or changes in user applications and infrastucture.

Snapshot data is collected by trace module in raw form in a binary log file and 
then may be loaded into a stats database using ntrace utility.

Stored snapshots may contain data on usage of indices, statements executed, their
plans and performance counters, IO statistics and other performance data.

Level of detail for information collected is set up in analysis trace module 
configuration file on per-database basis.

Default analysis module does not have GUI and only provides a database with data 
on server activities and some stored procedures to use and maintain it.

It is expected that third parties develop GUI tools, custom trace modules and 
statistics tools to statisfy rich client needs.

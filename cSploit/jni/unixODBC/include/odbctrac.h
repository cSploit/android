/*!
 * \file
 *
 * \author  Peter Harvey <pharvey@peterharvey.org> www.peterharvey.org
 * \author  \sa AUTHORS file
 * \version 1
 * \date    2007
 * \license Copyright unixODBC Project 2007-2008, LGPL
 */

/*! 
 * \mainpage    ODBC Trace PlugIn
 *
 * \section intro_sec Introduction
 *
 *              This library provides a Driver with a cross-platform (including MS Windows) means of producing trace output. 
 *              This may also be used by Driver Managers. This is compat. with MS approach but not the same.
 *
 *              This concept is based upon the MS odbctrac method of producing ODBC trace output. The main principle of this
 *              is that the trace library can be swapped out with a custom trace library allowing the trace output to be 
 *              tailored to an organizations needs. It also allows trace code to be shared among Drivers and even the
 *              Driver Manager.
 *
 *              This library differs from the MS implementation in some significant ways. 
 *
 *              - all TraceSQL* functions return SQLPOINTER for call context... not SQLRETURN (this improves perf.)
 *              - TraceReturn accepts an SQLPOINTER for call context... not SQLRETURN (this improves perf.)
 *              - all functions accept SQLPOINTER as first arg - a trace handle (this can reduce concurrency issues)
 *
 *              A notable weakness over MS odbctrac is the fact that we work with a HTRACE and that a Driver will want to
 *              maintain this within environment and connection handles. This means that we can not produce trace output
 *              for case when the environment/connection (whichever is relevant for the call) handle is invalid. This weakness
 *              is offset by our ability to reduce conccurency issues and provide more options with regard to the granularity
 *              of the tracing (environment/driver scope or connection/dsn scope for example). The driver can, at the developers
 *              option, make a global HTRACE and thereby allow trace output even when the ODBC handles are invalid - but
 *              this is not recommended.
 *
 *              If an application is getting back an SQL_INVALID_HANDLE and no trace... then we can pretty much assume that 
 *              the application is providing an invalid handle :)
 *
 *              unixODBC provides a cross-platform (including MS platforms) helper library called 'trace'. This is the 
 *              recommended method to use tracing. The text file driver (odbctxt) included with unixODBC demonstrates
 *              how to use the 'trace' helper library.
 *
 *              Why create a custom trace plugin? Here are a few possible reasons;
 *
 *              - produce trace output showing a hierarchy (tree view) of calls
 *              - allow config to limit or filter trace output
 *              - allow trace output to be provided to some sort of UI like an IDE via shared mem or other
 *              - allow trace output to be provided to remote machine via RPC or whatever
 *              - produce trace output in XML
 *              - produce trace output in a manner which can allow the calls to be played back (ie to reproduce bug
 *              in a closed source app)
 */

#ifndef ODBCTRAC_H
#define ODBCTRAC_H

#include <stdlib.h>
#include <stdio.h>
#include <sqlext.h>

#endif


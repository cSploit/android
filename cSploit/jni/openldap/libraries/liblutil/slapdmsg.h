//
// This file contains message strings for the OpenLDAP slapd service.
//
// This file should be compiled as follows
//   mc -v slapdmsg.mc  -r $(IntDir)  
//   rc /v /r  $(IntDir)\slapdmsg.rc
// The mc (message compiler) command generates the .rc and .h files from this file. The 
// rc (resource compiler) takes the .rc file and produces a .res file that can be linked 
// with the final executable application. The application is then registered as a message
// source with by creating the appropriate entries in the system registry.
//
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: MSG_SVC_STARTED
//
// MessageText:
//
//  OpenLDAP service started. debuglevel=%1, conffile=%2, urls=%3
//
#define MSG_SVC_STARTED                  0x40000500L

//
// MessageId: MSG_SVC_STOPPED
//
// MessageText:
//
//  OpenLDAP service stopped.
//
#define MSG_SVC_STOPPED                  0x40000501L


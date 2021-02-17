;//
;// This file contains message strings for the OpenLDAP slapd service.
;//
;// This file should be compiled as follows
;//   mc -v slapdmsg.mc  -r $(IntDir)  
;//   rc /v /r  $(IntDir)\slapdmsg.rc
;// The mc (message compiler) command generates the .rc and .h files from this file. The 
;// rc (resource compiler) takes the .rc file and produces a .res file that can be linked 
;// with the final executable application. The application is then registered as a message
;// source with by creating the appropriate entries in the system registry.
;//

MessageID=0x500
Severity=Informational
SymbolicName=MSG_SVC_STARTED
Facility=Application
Language=English
OpenLDAP service started. debuglevel=%1, conffile=%2, urls=%3
.


MessageID=0x501
Severity=Informational
SymbolicName=MSG_SVC_STOPPED
Facility=Application
Language=English
OpenLDAP service stopped.
.

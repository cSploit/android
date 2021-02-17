/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

/*
 * This client is part of Test 2 of the XA test suite.  It calls bdb1, which
 * in turn calls bdb2.  In the test several clients are executed at once to test
 * how XA performs with multiple processes.
 */

#include <stdio.h>
#include <string.h>
#include "atmi.h"		/* TUXEDO  Header File */
#include "../utilities/bdb_xa_util.h"


void my_exit();
char *sendbuf, *rcvbuf;

void
my_exit(string)
char *string;
{
	tpfree(sendbuf);
	tpfree(rcvbuf);
	fprintf(stderr,"SIMPAPP:ERROR call to service %s failed \n",string);
	tpterm();
	exit(1);
}

int
main(argc, argv)
int argc;
char *argv[];
{

	extern char *optarg;
	char * msg = NULL;
	TPQCTL qctl;	/* Q control structures */
	long sendlen=10, rcvlen=10, len;
	int ret;
	char ch;
	char * service;
	int times = 1;
	int i;
	int delete = 0;
	int need_print = 0;
	int nostop =0;

	if(argc < 5) {
		fprintf(stderr, 
		    "Usage: %s -s service -m messgae [-t times][-d][-p]\n",
		    argv[0] );
		exit(1);
	}

	while ((ch = getopt(argc, argv, "s:m:t:dpn")) != EOF)
		switch (ch) {
		case 's':
			service = optarg;
			break;
		case 'm':
			msg = optarg;
			break;
		case 't':
			times = atoi(optarg);
			break;
		case 'd':
			delete = 1;
			break;
		case 'n':
			nostop = 1;
			break;
		}
	
	/* Attach to System/T as a Client Process */
	if (tpinit((TPINIT *) NULL) == -1) {
		userlog("SIMPAPP:ERROR  tpinit failed \n");
		userlog("SIMPAPP:SYSTEM ERROR %s \n", tpstrerror(tperrno));
		fprintf(stderr, "Simpapp:ERROR Tpinit failed\n");
		my_exit("tpinit");
	}
	
	if(msg){
		sendlen = strlen(msg);
	}

	/* Allocate STRING buffers for the request and the reply */

	if((sendbuf = (char *) tpalloc("STRING", NULL, sendlen+1)) == NULL) {
		fprintf(stderr,"Simpapp:ERRROR Allocating send buffer\n");
		tpterm();
		my_exit("tpalloc");
	}

	if((rcvbuf = (char *) tpalloc("STRING", NULL, sendlen+1)) == NULL) {
		fprintf(stderr,"Simpapp:ERROR allocating receive buffer\n");
		tpfree(sendbuf);
		tpterm();
		my_exit("tpalloc");
	}

		strcpy(sendbuf, msg);

	for(i=0; i<times; i++){
		ret = tpcall(service, sendbuf, 0, &rcvbuf, &rcvlen,(long)0);
		if(ret == -1) {
			fprintf(stderr,"Simapp:ERROR tpacall to service %s \n", 
			    service);
			userlog("SIMPAPP:SYSTEM ERROR %s \n", 
			    tpstrerror(tperrno));
			if(nostop && (tperrno != TPENOENT))
				continue;
			else
				my_exit("WRITE");
		}

		if(verbose) printf("Simpapp: Returned string from : %s\n", 
		    rcvbuf);
	
		if(!delete) continue;

		memset(&qctl, '\0', sizeof(qctl));
		if (tpdequeue("QSPACE", "MYQUE", (TPQCTL *)&qctl, 
		    (char **)&rcvbuf, &len, TPNOTIME) == -1) 
		{
			userlog("QM diagnostic %ld\n", qctl.diagnostic);
			my_exit("queue");
	        }
		else if(verbose){
			printf("Simpapp: get string from queue : %s\n", rcvbuf);
		}
	}

	/* Free Buffers & Detach from System/T */
	tpfree(sendbuf);
	tpfree(rcvbuf);
	tpterm();
	exit(0);
}

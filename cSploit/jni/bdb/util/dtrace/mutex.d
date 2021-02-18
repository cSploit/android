#!/usr/sbin/dtrace -qs
/*
 * Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * mutex.d - Display DB mutex wait times.
 *
 * Specify the target application with -p <pid> or -c "<program> [<args>..]"
 *
 * The optional integer maxcount parameter directs the script to exit once that
 * many mutex times have been accumulated.
 *
 * usage: apicalls.d { -p <pid> | -c "<program> [<args]" } [limit]
 *
 * Static probes: 
 *   mutex__suspend(unsigned mutex, unsigned exclusive, unsigned alloc_id);
 *   mutex__resume(unsigned mutex, unsigned exclusive, unsigned alloc_id);
 *
 *	mutex:
 *		is the integer mutex id returned by mutex_alloc()
 *	exclusive:
 *		is set to 1 except when obtaining a read-write latch with
 *		shared access (allocated with DB_MUTEX_SHARED), then it is 0.
 *	alloc_id:
 *		is one of dbinc/mutex.h's MTX_XXX definitions (i.e., the class)
 */
#pragma D option defaultargs

string idnames[int];

dtrace:::BEGIN
{
	maxcount = $1 != 0 ? $1 : -1;
	mutexcount = 0;
	printf("DB Mutex wait times for process %d; interrupt to display summary\n", $target);
	idnames[1] = "APPLICATION";
	idnames[2] = "ATOMIC_EMULATION";
	idnames[3] = "DB_HANDLE";
	idnames[4] = "ENV_DBLIST";
	idnames[5] = "ENV_HANDLE";
	idnames[6] = "ENV_REGION";
	idnames[7] = "LOCK_REGION";
	idnames[8] = "LOGICAL_LOCK";
	idnames[9] = "LOG_FILENAME";
	idnames[10] = "LOG_FLUSH";
	idnames[11] = "LOG_HANDLE";
	idnames[12] = "LOG_REGION";
	idnames[13] = "MPOOLFILE_HANDLE";
	idnames[14] = "MPOOL_BH";
	idnames[15] = "MPOOL_FH";
	idnames[16] = "MPOOL_FILE_BUCKET";
	idnames[17] = "MPOOL_HANDLE";
	idnames[18] = "MPOOL_HASH_BUCKET";
	idnames[19] = "MPOOL_REGION";
	idnames[20] = "MUTEX_REGION";
	idnames[21] = "MUTEX_TEST";
	idnames[22] = "REP_CHKPT";
	idnames[23] = "REP_DATABASE";
	idnames[24] = "REP_DIAG";
	idnames[25] = "REP_EVENT";
	idnames[26] = "REP_REGION";
	idnames[27] = "REP_START";
	idnames[28] = "REP_WAITER";
	idnames[29] = "REPMGR";
	idnames[30] = "SEQUENCE";
	idnames[31] = "TWISTER";
	idnames[32] = "TCL_EVENTS";
	idnames[33] = "TXN_ACTIVE";
	idnames[34] = "TXN_CHKPT";
	idnames[35] = "TXN_COMMIT";
	idnames[36] = "TXN_MVCC";
	idnames[37] = "TXN_REGION";
}

bdb$target:::mutex-suspend
{
	self->suspend = timestamp;
}

/* mutex-resume(unsigned mutex, boolean exclusive, unsigned alloc_id, struct __db_mutex_t *mutexp) */
bdb$target:::mutex-resume
/self->suspend/
{
	this->duration = timestamp - self->suspend;
	self->suspend = 0;
	@mutexwaits[arg0, arg1, idnames[arg2], tid] = quantize(this->duration);
	@classwaits[idnames[arg2], arg1] = quantize(this->duration);
	mutexcount++;
}

bdb$target:::mutex-resume
/mutexcount == maxcount/
{
	exit(0);
}

dtrace:::END
{
	printf("Mutex wait times grouped by (mutex, mode, thread)\n");
	printa("Mutex %d exclusive %d %s thread %p wait times in nanoseconds %@d\n", @mutexwaits);
	printf("\nAggregate mutex wait times grouped by (alloc_id, mode)\n");
	printa("Mutex class %s exclusive %d wait times in nanoseconds %@d\n", @classwaits);
}

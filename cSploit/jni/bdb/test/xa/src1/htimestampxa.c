/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

#include <sys/types.h>
#include <sys/time.h>

#include "htimestampxa.h"

void
GetTime(HTimestampData *ts)
{
	struct timeval timeNow;

	(void)gettimeofday(&timeNow, 0);
	ts->Sec = timeNow.tv_sec;
	ts->Usec = timeNow.tv_usec;
}

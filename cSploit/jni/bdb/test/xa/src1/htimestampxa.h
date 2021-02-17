/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

#ifndef HTIMESTAMPXA_H
#define	HTIMESTAMPXA_H

/*
 * Timestamp with microseconds precision
 */
typedef struct __HTimestampData {
	time_t Sec;
	time_t Usec;
} HTimestampData;

void GetTime(HTimestampData *);
#endif

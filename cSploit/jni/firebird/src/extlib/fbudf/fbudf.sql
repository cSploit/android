/*
 *
 *     The contents of this file are subject to the Initial
 *     Developer's Public License Version 1.0 (the "License");
 *     you may not use this file except in compliance with the
 *     License. You may obtain a copy of the License at
 *     http://www.ibphoenix.com/idpl.html.
 *
 *     Software distributed under the License is distributed on
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 *     express or implied.  See the License for the specific
 *     language governing rights and limitations under the License.
 *
 *
 *  The Original Code was created by Claudio Valderrama C. for IBPhoenix.
 *  The development of the Original Code was sponsored by Craig Leonardi.
 *
 *  Copyright (c) 2001 IBPhoenix
 *  All Rights Reserved.
 */


/* This file defines the new udfs for Firebird. */

set sql dialect 3;

-- The Nvl family is deprecated. Use native COALESCE instead.
--FBUDF_API paramdsc* idNvl(paramdsc* v, paramdsc* v2)
declare external function invl
int by descriptor, int by descriptor
returns int by descriptor
entry_point 'idNvl' module_name 'fbudf';

--FBUDF_API paramdsc* idNvl(paramdsc* v, paramdsc* v2)
declare external function i64nvl
numeric(18,0) by descriptor, numeric(18,0) by descriptor
returns numeric(18,0) by descriptor
entry_point 'idNvl' module_name 'fbudf';

--FBUDF_API paramdsc* idNvl(paramdsc* v, paramdsc* v2)
declare external function dnvl
double precision by descriptor, double precision by descriptor
returns double precision by descriptor
entry_point 'idNvl' module_name 'fbudf';

--FBUDF_API paramdsc* sNvl(paramdsc* v, paramdsc* v2, paramdsc* rc)
declare external function snvl
varchar(100) by descriptor, varchar(100) by descriptor,
varchar(100) by descriptor returns parameter 3
entry_point 'sNvl' module_name 'fbudf';

-- The NullIf family is deprecated. Use native NULLIF instead.
--FBUDF_API paramdsc* iNullIf(paramdsc* v, paramdsc* v2)
declare external function inullif
int by descriptor, int by descriptor
returns int by descriptor
entry_point 'iNullIf' module_name 'fbudf';

--FBUDF_API paramdsc* dNullIf(paramdsc* v, paramdsc* v2)
declare external function dnullif
double precision by descriptor, double precision by descriptor
returns double precision by descriptor
entry_point 'dNullIf' module_name 'fbudf';

--FBUDF_API paramdsc* iNullIf(paramdsc* v, paramdsc* v2)
declare external function i64nullif
numeric(18,4) by descriptor, numeric(18,4) by descriptor
returns numeric(18,4) by descriptor
entry_point 'iNullIf' module_name 'fbudf';

--FBUDF_API paramdsc* sNullIf(paramdsc* v, paramdsc* v2, paramdsc* rc)
declare external function snullif
varchar(100) by descriptor, varchar(100) by descriptor,
varchar(100) by descriptor returns parameter 3
entry_point 'sNullIf' module_name 'fbudf';

--FBUDF_API char* DOW(ISC_DATE* v, char* rc)
declare external function dow
timestamp,
varchar(15) returns parameter 2
entry_point 'DOW' module_name 'fbudf';

--FBUDF_API char* SDOW(ISC_DATE* v, char* rc)
declare external function sdow
timestamp,
varchar(5) returns parameter 2
entry_point 'SDOW' module_name 'fbudf';

--FBUDF_API paramdsc* right(const paramdsc*, const ISC_SHORT& rl, paramdsc* rc)
declare external function sright
varchar(100) by descriptor, smallint,
varchar(100) by descriptor returns parameter 3
entry_point 'right' module_name 'fbudf';

--FBUDF_API ISC_TIMESTAMP* addDay(ISC_TIMESTAMP* v, const ISC_SLONG& ndays)
declare external function addDay
timestamp, int
returns timestamp
entry_point 'addDay' module_name 'fbudf';

--FBUDF_API void addDay2(const ISC_TIMESTAMP* v0, const ISC_SLONG& ndays, ISC_TIMESTAMP* v)
declare external function addDay2
timestamp, int, timestamp
returns parameter 3
entry_point 'addDay2' module_name 'fbudf';

--FBUDF_API ISC_TIMESTAMP* addWeek(ISC_TIMESTAMP* v, const ISC_SLONG& nweeks)
declare external function addWeek
timestamp, int
returns timestamp
entry_point 'addWeek' module_name 'fbudf';

--FBUDF_API ISC_TIMESTAMP* addMonth(ISC_TIMESTAMP* v, const ISC_SLONG& nmonths)
declare external function addMonth
timestamp, int
returns timestamp
entry_point 'addMonth' module_name 'fbudf';

--FBUDF_API ISC_TIMESTAMP* addYear(ISC_TIMESTAMP* v, const ISC_SLONG& nyears)
declare external function addYear
timestamp, int
returns timestamp
entry_point 'addYear' module_name 'fbudf';

--FBUDF_API ISC_TIMESTAMP* addMilliSecond(ISC_TIMESTAMP* v, const ISC_SLONG& nseconds)
declare external function addMilliSecond
timestamp, int
returns timestamp
entry_point 'addMilliSecond' module_name 'fbudf';

--FBUDF_API ISC_TIMESTAMP* addSecond(ISC_TIMESTAMP* v, const ISC_SLONG& nseconds)
declare external function addSecond
timestamp, int
returns timestamp
entry_point 'addSecond' module_name 'fbudf';

--FBUDF_API ISC_TIMESTAMP* addMinute(ISC_TIMESTAMP* v, const ISC_SLONG& nminutes)
declare external function addMinute
timestamp, int
returns timestamp
entry_point 'addMinute' module_name 'fbudf';

--FBUDF_API ISC_TIMESTAMP* addHour(ISC_TIMESTAMP* v, const ISC_SLONG& nhours)
declare external function addHour
timestamp, int
returns timestamp
entry_point 'addHour' module_name 'fbudf';

--It will work only with Win32 until it's ported to another OS.
--FBUDF_API ISC_TIMESTAMP* getExactTimestamp(ISC_TIMESTAMP* rc)
declare external function getExactTimestamp
timestamp
returns parameter 1
entry_point 'getExactTimestamp' module_name 'fbudf';

--It will work only with Win32 until it's ported to another OS.
--FBUDF_API void getExactTimestampUTC(ISC_TIMESTAMP* rc);
declare external function getExactTimestampUTC
timestamp
returns parameter 1
entry_point 'getExactTimestampUTC' module_name 'fbudf';

--FBUDF_API int isLeapYear(const ISC_TIMESTAMP* v);
declare external function isLeapYear
timestamp
returns int by value
entry_point 'isLeapYear' module_name 'fbudf';

--FBUDF_API paramdsc* fbtruncate(const paramdsc* v, paramdsc* rc)
declare external function Truncate
int by descriptor, int by descriptor
returns parameter 2
entry_point 'fbtruncate' module_name 'fbudf';

--FBUDF_API paramdsc* fbtruncate(const paramdsc* v, paramdsc* rc)
declare external function i64Truncate
numeric(18) by descriptor, numeric(18) by descriptor
returns parameter 2
entry_point 'fbtruncate' module_name 'fbudf';

--FBUDF_API paramdsc* fbround(const paramdsc* v, paramdsc* rc)
declare external function Round
int by descriptor, int by descriptor
returns parameter 2
entry_point 'fbround' module_name 'fbudf';

--FBUDF_API paramdsc* fbround(const paramdsc* v, paramdsc* rc)
declare external function i64Round
numeric(18, 4) by descriptor, numeric(18, 4) by descriptor
returns parameter 2
entry_point 'fbround' module_name 'fbudf';

--FBUDF_API paramdsc* power(const paramdsc* v, const paramdsc* v2, paramdsc* rc)
declare external function dPower
double precision by descriptor, double precision by descriptor,
double precision by descriptor
returns parameter 3
entry_point 'power' module_name 'fbudf';

--FBUDF_API blobcallback* string2blob(const paramdsc* v, blobcallback* outblob)
declare external function string2blob
varchar(300) by descriptor,
blob returns parameter 2
entry_point 'string2blob' module_name 'fbudf';



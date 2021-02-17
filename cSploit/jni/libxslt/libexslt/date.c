/*
 * date.c: Implementation of the EXSLT -- Dates and Times module
 *
 * References:
 *   http://www.exslt.org/date/date.html
 *
 * See Copyright for the status of this software.
 *
 * Authors:
 *   Charlie Bozeman <cbozeman@HiWAAY.net>
 *   Thomas Broyer <tbroyer@ltgt.net>
 *
 * TODO:
 * elements:
 *   date-format
 * functions:
 *   format-date
 *   parse-date
 *   sum
 */

#define IN_LIBEXSLT
#include "libexslt/libexslt.h"

#if defined(WIN32) && !defined (__CYGWIN__) && (!__MINGW32__)
#include <win32config.h>
#else
#include "config.h"
#endif

#if HAVE_LOCALTIME_R	/* _POSIX_SOURCE required by gnu libc */
#ifndef _AIX51		/* but on AIX we're not using gnu libc */
#define _POSIX_SOURCE
#endif
#endif

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <libxslt/xsltconfig.h>
#include <libxslt/xsltutils.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/extensions.h>

#include "exslt.h"

#include <string.h>

#ifdef HAVE_MATH_H
#include <math.h>
#endif

/* needed to get localtime_r on Solaris */
#ifdef __sun
#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif

/*
 * types of date and/or time (from schema datatypes)
 *   somewhat ordered from least specific to most specific (i.e.
 *   most truncated to least truncated).
 */
typedef enum {
    EXSLT_UNKNOWN  =    0,
    XS_TIME        =    1,       /* time is left-truncated */
    XS_GDAY        = (XS_TIME   << 1),
    XS_GMONTH      = (XS_GDAY   << 1),
    XS_GMONTHDAY   = (XS_GMONTH | XS_GDAY),
    XS_GYEAR       = (XS_GMONTH << 1),
    XS_GYEARMONTH  = (XS_GYEAR  | XS_GMONTH),
    XS_DATE        = (XS_GYEAR  | XS_GMONTH | XS_GDAY),
    XS_DATETIME    = (XS_DATE   | XS_TIME),
    XS_DURATION    = (XS_GYEAR  << 1)
} exsltDateType;

/* Date value */
typedef struct _exsltDateValDate exsltDateValDate;
typedef exsltDateValDate *exsltDateValDatePtr;
struct _exsltDateValDate {
    long		year;
    unsigned int	mon	:4;	/* 1 <=  mon    <= 12   */
    unsigned int	day	:5;	/* 1 <=  day    <= 31   */
    unsigned int	hour	:5;	/* 0 <=  hour   <= 23   */
    unsigned int	min	:6;	/* 0 <=  min    <= 59	*/
    double		sec;
    unsigned int	tz_flag	:1;	/* is tzo explicitely set? */
    signed int		tzo	:12;	/* -1440 <= tzo <= 1440 currently only -840 to +840 are needed */
};

/* Duration value */
typedef struct _exsltDateValDuration exsltDateValDuration;
typedef exsltDateValDuration *exsltDateValDurationPtr;
struct _exsltDateValDuration {
    long	        mon;		/* mon stores years also */
    long        	day;
    double		sec;            /* sec stores min and hour also */
};

typedef struct _exsltDateVal exsltDateVal;
typedef exsltDateVal *exsltDateValPtr;
struct _exsltDateVal {
    exsltDateType       type;
    union {
        exsltDateValDate        date;
        exsltDateValDuration    dur;
    } value;
};

/****************************************************************
 *								*
 *			Compat./Port. macros			*
 *								*
 ****************************************************************/

#if defined(HAVE_TIME_H) 					\
    && (defined(HAVE_LOCALTIME) || defined(HAVE_LOCALTIME_R))	\
    && (defined(HAVE_GMTIME) || defined(HAVE_GMTIME_R))		\
    && defined(HAVE_TIME)
#define WITH_TIME
#endif

/****************************************************************
 *								*
 *		Convenience macros and functions		*
 *								*
 ****************************************************************/

#define IS_TZO_CHAR(c)						\
	((c == 0) || (c == 'Z') || (c == '+') || (c == '-'))

#define VALID_ALWAYS(num)	(num >= 0)
#define VALID_YEAR(yr)          (yr != 0)
#define VALID_MONTH(mon)        ((mon >= 1) && (mon <= 12))
/* VALID_DAY should only be used when month is unknown */
#define VALID_DAY(day)          ((day >= 1) && (day <= 31))
#define VALID_HOUR(hr)          ((hr >= 0) && (hr <= 23))
#define VALID_MIN(min)          ((min >= 0) && (min <= 59))
#define VALID_SEC(sec)          ((sec >= 0) && (sec < 60))
#define VALID_TZO(tzo)          ((tzo > -1440) && (tzo < 1440))
#define IS_LEAP(y)						\
	(((y % 4 == 0) && (y % 100 != 0)) || (y % 400 == 0))

static const unsigned long daysInMonth[12] =
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static const unsigned long daysInMonthLeap[12] =
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

#define MAX_DAYINMONTH(yr,mon)                                  \
        (IS_LEAP(yr) ? daysInMonthLeap[mon - 1] : daysInMonth[mon - 1])

#define VALID_MDAY(dt)						\
	(IS_LEAP(dt->year) ?				        \
	    (dt->day <= daysInMonthLeap[dt->mon - 1]) :	        \
	    (dt->day <= daysInMonth[dt->mon - 1]))

#define VALID_DATE(dt)						\
	(VALID_YEAR(dt->year) && VALID_MONTH(dt->mon) && VALID_MDAY(dt))

/*
    hour and min structure vals are unsigned, so normal macros give
    warnings on some compilers.
*/
#define VALID_TIME(dt)						\
	((dt->hour <=23 ) && (dt->min <= 59) &&			\
	 VALID_SEC(dt->sec) && VALID_TZO(dt->tzo))

#define VALID_DATETIME(dt)					\
	(VALID_DATE(dt) && VALID_TIME(dt))

#define SECS_PER_MIN            (60)
#define SECS_PER_HOUR           (60 * SECS_PER_MIN)
#define SECS_PER_DAY            (24 * SECS_PER_HOUR)

static const unsigned long dayInYearByMonth[12] =
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
static const unsigned long dayInLeapYearByMonth[12] =
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };

#define DAY_IN_YEAR(day, month, year)				\
        ((IS_LEAP(year) ?					\
                dayInLeapYearByMonth[month - 1] :		\
                dayInYearByMonth[month - 1]) + day)

/**
 * _exsltDateParseGYear:
 * @dt:  pointer to a date structure
 * @str: pointer to the string to analyze
 *
 * Parses a xs:gYear without time zone and fills in the appropriate
 * field of the @dt structure. @str is updated to point just after the
 * xs:gYear. It is supposed that @dt->year is big enough to contain
 * the year.
 *
 * Returns 0 or the error code
 */
static int
_exsltDateParseGYear (exsltDateValDatePtr dt, const xmlChar **str)
{
    const xmlChar *cur = *str, *firstChar;
    int isneg = 0, digcnt = 0;

    if (((*cur < '0') || (*cur > '9')) &&
	(*cur != '-') && (*cur != '+'))
	return -1;

    if (*cur == '-') {
	isneg = 1;
	cur++;
    }

    firstChar = cur;

    while ((*cur >= '0') && (*cur <= '9')) {
	dt->year = dt->year * 10 + (*cur - '0');
	cur++;
	digcnt++;
    }

    /* year must be at least 4 digits (CCYY); over 4
     * digits cannot have a leading zero. */
    if ((digcnt < 4) || ((digcnt > 4) && (*firstChar == '0')))
	return 1;

    if (isneg)
	dt->year = - dt->year;

    if (!VALID_YEAR(dt->year))
	return 2;

    *str = cur;

#ifdef DEBUG_EXSLT_DATE
    xsltGenericDebug(xsltGenericDebugContext,
		     "Parsed year %04i\n", dt->year);
#endif

    return 0;
}

/**
 * FORMAT_GYEAR:
 * @yr:  the year to format
 * @cur: a pointer to an allocated buffer
 *
 * Formats @yr in xsl:gYear format. Result is appended to @cur and
 * @cur is updated to point after the xsl:gYear.
 */
#define FORMAT_GYEAR(yr, cur)					\
	if (yr < 0) {					        \
	    *cur = '-';						\
	    cur++;						\
	}							\
	{							\
	    long year = (yr < 0) ? - yr : yr;                   \
	    xmlChar tmp_buf[100], *tmp = tmp_buf;		\
	    /* result is in reverse-order */			\
	    while (year > 0) {					\
		*tmp = '0' + (xmlChar)(year % 10);		\
		year /= 10;					\
		tmp++;						\
	    }							\
	    /* virtually adds leading zeros */			\
	    while ((tmp - tmp_buf) < 4)				\
		*tmp++ = '0';					\
	    /* restore the correct order */			\
	    while (tmp > tmp_buf) {				\
		tmp--;						\
		*cur = *tmp;					\
		cur++;						\
	    }							\
	}

/**
 * PARSE_2_DIGITS:
 * @num:  the integer to fill in
 * @cur:  an #xmlChar *
 * @func: validation function for the number
 * @invalid: an integer
 *
 * Parses a 2-digits integer and updates @num with the value. @cur is
 * updated to point just after the integer.
 * In case of error, @invalid is set to %TRUE, values of @num and
 * @cur are undefined.
 */
#define PARSE_2_DIGITS(num, cur, func, invalid)			\
	if ((cur[0] < '0') || (cur[0] > '9') ||			\
	    (cur[1] < '0') || (cur[1] > '9'))			\
	    invalid = 1;					\
	else {							\
	    int val;						\
	    val = (cur[0] - '0') * 10 + (cur[1] - '0');		\
	    if (!func(val))					\
	        invalid = 2;					\
	    else						\
	        num = val;					\
	}							\
	cur += 2;

/**
 * FORMAT_2_DIGITS:
 * @num:  the integer to format
 * @cur: a pointer to an allocated buffer
 *
 * Formats a 2-digits integer. Result is appended to @cur and
 * @cur is updated to point after the integer.
 */
#define FORMAT_2_DIGITS(num, cur)				\
	*cur = '0' + ((num / 10) % 10);				\
	cur++;							\
	*cur = '0' + (num % 10);				\
	cur++;

/**
 * PARSE_FLOAT:
 * @num:  the double to fill in
 * @cur:  an #xmlChar *
 * @invalid: an integer
 *
 * Parses a float and updates @num with the value. @cur is
 * updated to point just after the float. The float must have a
 * 2-digits integer part and may or may not have a decimal part.
 * In case of error, @invalid is set to %TRUE, values of @num and
 * @cur are undefined.
 */
#define PARSE_FLOAT(num, cur, invalid)				\
	PARSE_2_DIGITS(num, cur, VALID_ALWAYS, invalid);	\
	if (!invalid && (*cur == '.')) {			\
	    double mult = 1;				        \
	    cur++;						\
	    if ((*cur < '0') || (*cur > '9'))			\
		invalid = 1;					\
	    while ((*cur >= '0') && (*cur <= '9')) {		\
		mult /= 10;					\
		num += (*cur - '0') * mult;			\
		cur++;						\
	    }							\
	}

/**
 * FORMAT_FLOAT:
 * @num:  the double to format
 * @cur: a pointer to an allocated buffer
 * @pad: a flag for padding to 2 integer digits
 *
 * Formats a float. Result is appended to @cur and @cur is updated to
 * point after the integer. If the @pad flag is non-zero, then the
 * float representation has a minimum 2-digits integer part. The
 * fractional part is formatted if @num has a fractional value.
 */
#define FORMAT_FLOAT(num, cur, pad)				\
	{							\
            xmlChar *sav, *str;                                 \
            if ((pad) && (num < 10.0))                          \
                *cur++ = '0';                                   \
            str = xmlXPathCastNumberToString(num);              \
            sav = str;                                          \
            while (*str != 0)                                   \
                *cur++ = *str++;                                \
            xmlFree(sav);                                       \
	}

/**
 * _exsltDateParseGMonth:
 * @dt:  pointer to a date structure
 * @str: pointer to the string to analyze
 *
 * Parses a xs:gMonth without time zone and fills in the appropriate
 * field of the @dt structure. @str is updated to point just after the
 * xs:gMonth.
 *
 * Returns 0 or the error code
 */
static int
_exsltDateParseGMonth (exsltDateValDatePtr dt, const xmlChar **str)
{
    const xmlChar *cur = *str;
    int ret = 0;

    PARSE_2_DIGITS(dt->mon, cur, VALID_MONTH, ret);
    if (ret != 0)
	return ret;

    *str = cur;

#ifdef DEBUG_EXSLT_DATE
    xsltGenericDebug(xsltGenericDebugContext,
		     "Parsed month %02i\n", dt->mon);
#endif

    return 0;
}

/**
 * FORMAT_GMONTH:
 * @mon:  the month to format
 * @cur: a pointer to an allocated buffer
 *
 * Formats @mon in xsl:gMonth format. Result is appended to @cur and
 * @cur is updated to point after the xsl:gMonth.
 */
#define FORMAT_GMONTH(mon, cur)					\
	FORMAT_2_DIGITS(mon, cur)

/**
 * _exsltDateParseGDay:
 * @dt:  pointer to a date structure
 * @str: pointer to the string to analyze
 *
 * Parses a xs:gDay without time zone and fills in the appropriate
 * field of the @dt structure. @str is updated to point just after the
 * xs:gDay.
 *
 * Returns 0 or the error code
 */
static int
_exsltDateParseGDay (exsltDateValDatePtr dt, const xmlChar **str)
{
    const xmlChar *cur = *str;
    int ret = 0;

    PARSE_2_DIGITS(dt->day, cur, VALID_DAY, ret);
    if (ret != 0)
	return ret;

    *str = cur;

#ifdef DEBUG_EXSLT_DATE
    xsltGenericDebug(xsltGenericDebugContext,
		     "Parsed day %02i\n", dt->day);
#endif

    return 0;
}

/**
 * FORMAT_GDAY:
 * @dt:  the #exsltDateValDate to format
 * @cur: a pointer to an allocated buffer
 *
 * Formats @dt in xsl:gDay format. Result is appended to @cur and
 * @cur is updated to point after the xsl:gDay.
 */
#define FORMAT_GDAY(dt, cur)					\
	FORMAT_2_DIGITS(dt->day, cur)

/**
 * FORMAT_DATE:
 * @dt:  the #exsltDateValDate to format
 * @cur: a pointer to an allocated buffer
 *
 * Formats @dt in xsl:date format. Result is appended to @cur and
 * @cur is updated to point after the xsl:date.
 */
#define FORMAT_DATE(dt, cur)					\
	FORMAT_GYEAR(dt->year, cur);				\
	*cur = '-';						\
	cur++;							\
	FORMAT_GMONTH(dt->mon, cur);				\
	*cur = '-';						\
	cur++;							\
	FORMAT_GDAY(dt, cur);

/**
 * _exsltDateParseTime:
 * @dt:  pointer to a date structure
 * @str: pointer to the string to analyze
 *
 * Parses a xs:time without time zone and fills in the appropriate
 * fields of the @dt structure. @str is updated to point just after the
 * xs:time.
 * In case of error, values of @dt fields are undefined.
 *
 * Returns 0 or the error code
 */
static int
_exsltDateParseTime (exsltDateValDatePtr dt, const xmlChar **str)
{
    const xmlChar *cur = *str;
    unsigned int hour = 0; /* use temp var in case str is not xs:time */
    int ret = 0;

    PARSE_2_DIGITS(hour, cur, VALID_HOUR, ret);
    if (ret != 0)
	return ret;

    if (*cur != ':')
	return 1;
    cur++;

    /* the ':' insures this string is xs:time */
    dt->hour = hour;

    PARSE_2_DIGITS(dt->min, cur, VALID_MIN, ret);
    if (ret != 0)
	return ret;

    if (*cur != ':')
	return 1;
    cur++;

    PARSE_FLOAT(dt->sec, cur, ret);
    if (ret != 0)
	return ret;

    if (!VALID_TIME(dt))
	return 2;

    *str = cur;

#ifdef DEBUG_EXSLT_DATE
    xsltGenericDebug(xsltGenericDebugContext,
		     "Parsed time %02i:%02i:%02.f\n",
		     dt->hour, dt->min, dt->sec);
#endif

    return 0;
}

/**
 * FORMAT_TIME:
 * @dt:  the #exsltDateValDate to format
 * @cur: a pointer to an allocated buffer
 *
 * Formats @dt in xsl:time format. Result is appended to @cur and
 * @cur is updated to point after the xsl:time.
 */
#define FORMAT_TIME(dt, cur)					\
	FORMAT_2_DIGITS(dt->hour, cur);				\
	*cur = ':';						\
	cur++;							\
	FORMAT_2_DIGITS(dt->min, cur);				\
	*cur = ':';						\
	cur++;							\
	FORMAT_FLOAT(dt->sec, cur, 1);

/**
 * _exsltDateParseTimeZone:
 * @dt:  pointer to a date structure
 * @str: pointer to the string to analyze
 *
 * Parses a time zone without time zone and fills in the appropriate
 * field of the @dt structure. @str is updated to point just after the
 * time zone.
 *
 * Returns 0 or the error code
 */
static int
_exsltDateParseTimeZone (exsltDateValDatePtr dt, const xmlChar **str)
{
    const xmlChar *cur;
    int ret = 0;

    if (str == NULL)
	return -1;
    cur = *str;
    switch (*cur) {
    case 0:
	dt->tz_flag = 0;
	dt->tzo = 0;
	break;

    case 'Z':
	dt->tz_flag = 1;
	dt->tzo = 0;
	cur++;
	break;

    case '+':
    case '-': {
	int isneg = 0, tmp = 0;
	isneg = (*cur == '-');

	cur++;

	PARSE_2_DIGITS(tmp, cur, VALID_HOUR, ret);
	if (ret != 0)
	    return ret;

	if (*cur != ':')
	    return 1;
	cur++;

	dt->tzo = tmp * 60;

	PARSE_2_DIGITS(tmp, cur, VALID_MIN, ret);
	if (ret != 0)
	    return ret;

	dt->tzo += tmp;
	if (isneg)
	    dt->tzo = - dt->tzo;

	if (!VALID_TZO(dt->tzo))
	    return 2;

	break;
      }
    default:
	return 1;
    }

    *str = cur;

#ifdef DEBUG_EXSLT_DATE
    xsltGenericDebug(xsltGenericDebugContext,
		     "Parsed time zone offset (%s) %i\n",
		     dt->tz_flag ? "explicit" : "implicit", dt->tzo);
#endif

    return 0;
}

/**
 * FORMAT_TZ:
 * @tzo:  the timezone offset to format
 * @cur: a pointer to an allocated buffer
 *
 * Formats @tzo timezone. Result is appended to @cur and
 * @cur is updated to point after the timezone.
 */
#define FORMAT_TZ(tzo, cur)					\
	if (tzo == 0) {					        \
	    *cur = 'Z';						\
	    cur++;						\
	} else {						\
	    int aTzo = (tzo < 0) ? - tzo : tzo;                 \
	    int tzHh = aTzo / 60, tzMm = aTzo % 60;		\
	    *cur = (tzo < 0) ? '-' : '+' ;			\
	    cur++;						\
	    FORMAT_2_DIGITS(tzHh, cur);				\
	    *cur = ':';						\
	    cur++;						\
	    FORMAT_2_DIGITS(tzMm, cur);				\
	}

/****************************************************************
 *								*
 *	XML Schema Dates/Times Datatypes Handling		*
 *								*
 ****************************************************************/

/**
 * exsltDateCreateDate:
 * @type:       type to create
 *
 * Creates a new #exsltDateVal, uninitialized.
 *
 * Returns the #exsltDateValPtr
 */
static exsltDateValPtr
exsltDateCreateDate (exsltDateType type)
{
    exsltDateValPtr ret;

    ret = (exsltDateValPtr) xmlMalloc(sizeof(exsltDateVal));
    if (ret == NULL) {
	xsltGenericError(xsltGenericErrorContext,
			 "exsltDateCreateDate: out of memory\n");
	return (NULL);
    }
    memset (ret, 0, sizeof(exsltDateVal));

    if (type != EXSLT_UNKNOWN)
        ret->type = type;

    return ret;
}

/**
 * exsltDateFreeDate:
 * @date: an #exsltDateValPtr
 *
 * Frees up the @date
 */
static void
exsltDateFreeDate (exsltDateValPtr date) {
    if (date == NULL)
	return;

    xmlFree(date);
}

/**
 * PARSE_DIGITS:
 * @num:  the integer to fill in
 * @cur:  an #xmlChar *
 * @num_type: an integer flag
 *
 * Parses a digits integer and updates @num with the value. @cur is
 * updated to point just after the integer.
 * In case of error, @num_type is set to -1, values of @num and
 * @cur are undefined.
 */
#define PARSE_DIGITS(num, cur, num_type)	                \
	if ((*cur < '0') || (*cur > '9'))			\
	    num_type = -1;					\
        else                                                    \
	    while ((*cur >= '0') && (*cur <= '9')) {		\
	        num = num * 10 + (*cur - '0');		        \
	        cur++;                                          \
            }

/**
 * PARSE_NUM:
 * @num:  the double to fill in
 * @cur:  an #xmlChar *
 * @num_type: an integer flag
 *
 * Parses a float or integer and updates @num with the value. @cur is
 * updated to point just after the number. If the number is a float,
 * then it must have an integer part and a decimal part; @num_type will
 * be set to 1. If there is no decimal part, @num_type is set to zero.
 * In case of error, @num_type is set to -1, values of @num and
 * @cur are undefined.
 */
#define PARSE_NUM(num, cur, num_type)				\
        num = 0;                                                \
	PARSE_DIGITS(num, cur, num_type);	                \
	if (!num_type && (*cur == '.')) {			\
	    double mult = 1;				        \
	    cur++;						\
	    if ((*cur < '0') || (*cur > '9'))			\
		num_type = -1;					\
            else                                                \
                num_type = 1;                                   \
	    while ((*cur >= '0') && (*cur <= '9')) {		\
		mult /= 10;					\
		num += (*cur - '0') * mult;			\
		cur++;						\
	    }							\
	}

#ifdef WITH_TIME
/**
 * exsltDateCurrent:
 *
 * Returns the current date and time.
 */
static exsltDateValPtr
exsltDateCurrent (void)
{
    struct tm localTm, gmTm;
    time_t secs;
    int local_s, gm_s;
    exsltDateValPtr ret;

    ret = exsltDateCreateDate(XS_DATETIME);
    if (ret == NULL)
        return NULL;

    /* get current time */
    secs    = time(NULL);
#if HAVE_LOCALTIME_R
    localtime_r(&secs, &localTm);
#else
    localTm = *localtime(&secs);
#endif

    /* get real year, not years since 1900 */
    ret->value.date.year = localTm.tm_year + 1900;

    ret->value.date.mon  = localTm.tm_mon + 1;
    ret->value.date.day  = localTm.tm_mday;
    ret->value.date.hour = localTm.tm_hour;
    ret->value.date.min  = localTm.tm_min;

    /* floating point seconds */
    ret->value.date.sec  = (double) localTm.tm_sec;

    /* determine the time zone offset from local to gm time */
#if HAVE_GMTIME_R
    gmtime_r(&secs, &gmTm);
#else
    gmTm = *gmtime(&secs);
#endif
    ret->value.date.tz_flag = 0;
#if 0
    ret->value.date.tzo = (((ret->value.date.day * 1440) +
                            (ret->value.date.hour * 60) +
                             ret->value.date.min) -
                           ((gmTm.tm_mday * 1440) + (gmTm.tm_hour * 60) +
                             gmTm.tm_min));
#endif
    local_s = localTm.tm_hour * SECS_PER_HOUR +
        localTm.tm_min * SECS_PER_MIN +
        localTm.tm_sec;
    
    gm_s = gmTm.tm_hour * SECS_PER_HOUR +
        gmTm.tm_min * SECS_PER_MIN +
        gmTm.tm_sec;
    
    if (localTm.tm_year < gmTm.tm_year) {
 	ret->value.date.tzo = -((SECS_PER_DAY - local_s) + gm_s)/60;
    } else if (localTm.tm_year > gmTm.tm_year) {
 	ret->value.date.tzo = ((SECS_PER_DAY - gm_s) + local_s)/60;
    } else if (localTm.tm_mon < gmTm.tm_mon) {
 	ret->value.date.tzo = -((SECS_PER_DAY - local_s) + gm_s)/60;
    } else if (localTm.tm_mon > gmTm.tm_mon) {
 	ret->value.date.tzo = ((SECS_PER_DAY - gm_s) + local_s)/60;
    } else if (localTm.tm_mday < gmTm.tm_mday) {
 	ret->value.date.tzo = -((SECS_PER_DAY - local_s) + gm_s)/60;
    } else if (localTm.tm_mday > gmTm.tm_mday) {
 	ret->value.date.tzo = ((SECS_PER_DAY - gm_s) + local_s)/60;
    } else  {
 	ret->value.date.tzo = (local_s - gm_s)/60;
    }
 
    return ret;
}
#endif

/**
 * exsltDateParse:
 * @dateTime:  string to analyze
 *
 * Parses a date/time string
 *
 * Returns a newly built #exsltDateValPtr of NULL in case of error
 */
static exsltDateValPtr
exsltDateParse (const xmlChar *dateTime)
{
    exsltDateValPtr dt;
    int ret;
    const xmlChar *cur = dateTime;

#define RETURN_TYPE_IF_VALID(t)					\
    if (IS_TZO_CHAR(*cur)) {					\
	ret = _exsltDateParseTimeZone(&(dt->value.date), &cur);	\
	if (ret == 0) {						\
	    if (*cur != 0)					\
		goto error;					\
	    dt->type = t;					\
	    return dt;						\
	}							\
    }

    if (dateTime == NULL)
	return NULL;

    if ((*cur != '-') && (*cur < '0') && (*cur > '9'))
	return NULL;

    dt = exsltDateCreateDate(EXSLT_UNKNOWN);
    if (dt == NULL)
	return NULL;

    if ((cur[0] == '-') && (cur[1] == '-')) {
	/*
	 * It's an incomplete date (xs:gMonthDay, xs:gMonth or
	 * xs:gDay)
	 */
	cur += 2;

	/* is it an xs:gDay? */
	if (*cur == '-') {
	  ++cur;
	    ret = _exsltDateParseGDay(&(dt->value.date), &cur);
	    if (ret != 0)
		goto error;

	    RETURN_TYPE_IF_VALID(XS_GDAY);

	    goto error;
	}

	/*
	 * it should be an xs:gMonthDay or xs:gMonth
	 */
	ret = _exsltDateParseGMonth(&(dt->value.date), &cur);
	if (ret != 0)
	    goto error;

	if (*cur != '-')
	    goto error;
	cur++;

	/* is it an xs:gMonth? */
	if (*cur == '-') {
	    cur++;
	    RETURN_TYPE_IF_VALID(XS_GMONTH);
	    goto error;
	}

	/* it should be an xs:gMonthDay */
	ret = _exsltDateParseGDay(&(dt->value.date), &cur);
	if (ret != 0)
	    goto error;

	RETURN_TYPE_IF_VALID(XS_GMONTHDAY);

	goto error;
    }

    /*
     * It's a right-truncated date or an xs:time.
     * Try to parse an xs:time then fallback on right-truncated dates.
     */
    if ((*cur >= '0') && (*cur <= '9')) {
	ret = _exsltDateParseTime(&(dt->value.date), &cur);
	if (ret == 0) {
	    /* it's an xs:time */
	    RETURN_TYPE_IF_VALID(XS_TIME);
	}
    }

    /* fallback on date parsing */
    cur = dateTime;

    ret = _exsltDateParseGYear(&(dt->value.date), &cur);
    if (ret != 0)
	goto error;

    /* is it an xs:gYear? */
    RETURN_TYPE_IF_VALID(XS_GYEAR);

    if (*cur != '-')
	goto error;
    cur++;

    ret = _exsltDateParseGMonth(&(dt->value.date), &cur);
    if (ret != 0)
	goto error;

    /* is it an xs:gYearMonth? */
    RETURN_TYPE_IF_VALID(XS_GYEARMONTH);

    if (*cur != '-')
	goto error;
    cur++;

    ret = _exsltDateParseGDay(&(dt->value.date), &cur);
    if ((ret != 0) || !VALID_DATE((&(dt->value.date))))
	goto error;

    /* is it an xs:date? */
    RETURN_TYPE_IF_VALID(XS_DATE);

    if (*cur != 'T')
	goto error;
    cur++;

    /* it should be an xs:dateTime */
    ret = _exsltDateParseTime(&(dt->value.date), &cur);
    if (ret != 0)
	goto error;

    ret = _exsltDateParseTimeZone(&(dt->value.date), &cur);
    if ((ret != 0) || (*cur != 0) || !VALID_DATETIME((&(dt->value.date))))
	goto error;

    dt->type = XS_DATETIME;

    return dt;

error:
    if (dt != NULL)
	exsltDateFreeDate(dt);
    return NULL;
}

/**
 * exsltDateParseDuration:
 * @duration:  string to analyze
 *
 * Parses a duration string
 *
 * Returns a newly built #exsltDateValPtr of NULL in case of error
 */
static exsltDateValPtr
exsltDateParseDuration (const xmlChar *duration)
{
    const xmlChar  *cur = duration;
    exsltDateValPtr dur;
    int isneg = 0;
    unsigned int seq = 0;

    if (duration == NULL)
	return NULL;

    if (*cur == '-') {
        isneg = 1;
        cur++;
    }

    /* duration must start with 'P' (after sign) */
    if (*cur++ != 'P')
	return NULL;

    dur = exsltDateCreateDate(XS_DURATION);
    if (dur == NULL)
	return NULL;

    while (*cur != 0) {
        double         num;
        int            num_type = 0;  /* -1 = invalid, 0 = int, 1 = floating */
        const xmlChar  desig[] = {'Y', 'M', 'D', 'H', 'M', 'S'};
        const double   multi[] = { 0.0, 0.0, 86400.0, 3600.0, 60.0, 1.0, 0.0};

        /* input string should be empty or invalid date/time item */
        if (seq >= sizeof(desig))
            goto error;

        /* T designator must be present for time items */
        if (*cur == 'T') {
            if (seq <= 3) {
                seq = 3;
                cur++;
            } else
                return NULL;
        } else if (seq == 3)
            goto error;

        /* parse the number portion of the item */
        PARSE_NUM(num, cur, num_type);

        if ((num_type == -1) || (*cur == 0))
            goto error;

        /* update duration based on item type */
        while (seq < sizeof(desig)) {
            if (*cur == desig[seq]) {

                /* verify numeric type; only seconds can be float */
                if ((num_type != 0) && (seq < (sizeof(desig)-1)))
                    goto error;

                switch (seq) {
                    case 0:
                        dur->value.dur.mon = (long)num * 12;
                        break;
                    case 1:
                        dur->value.dur.mon += (long)num;
                        break;
                    default:
                        /* convert to seconds using multiplier */
                        dur->value.dur.sec += num * multi[seq];
                        seq++;
                        break;
                }

                break;          /* exit loop */
            }
            /* no date designators found? */
            if (++seq == 3)
                goto error;
        }
        cur++;
    }

    if (isneg) {
        dur->value.dur.mon = -dur->value.dur.mon;
        dur->value.dur.day = -dur->value.dur.day;
        dur->value.dur.sec = -dur->value.dur.sec;
    }

#ifdef DEBUG_EXSLT_DATE
    xsltGenericDebug(xsltGenericDebugContext,
		     "Parsed duration %f\n", dur->value.dur.sec);
#endif

    return dur;

error:
    if (dur != NULL)
	exsltDateFreeDate(dur);
    return NULL;
}

/**
 * FORMAT_ITEM:
 * @num:        number to format
 * @cur:        current location to convert number
 * @limit:      max value
 * @item:       char designator
 *
 */
#define FORMAT_ITEM(num, cur, limit, item)                      \
        if (num != 0) {                                         \
            long comp = (long)num / limit;                      \
            if (comp != 0) {                                    \
                FORMAT_FLOAT((double)comp, cur, 0);             \
                *cur++ = item;                                  \
                num -= (double)(comp * limit);                  \
            }                                                   \
        }

/**
 * exsltDateFormatDuration:
 * @dt: an #exsltDateValDurationPtr
 *
 * Formats @dt in xs:duration format.
 *
 * Returns a newly allocated string, or NULL in case of error
 */
static xmlChar *
exsltDateFormatDuration (const exsltDateValDurationPtr dt)
{
    xmlChar buf[100], *cur = buf;
    double secs, days;
    double years, months;

    if (dt == NULL)
	return NULL;

    /* quick and dirty check */
    if ((dt->sec == 0.0) && (dt->day == 0) && (dt->mon == 0)) 
        return xmlStrdup((xmlChar*)"P0D");
        
    secs   = dt->sec;
    days   = (double)dt->day;
    years  = (double)(dt->mon / 12);
    months = (double)(dt->mon % 12);

    *cur = '\0';
    if (secs < 0.0) {
        secs = -secs;
        *cur = '-';
    } 
    if (days < 0) {
        days = -days;
        *cur = '-';
    } 
    if (years < 0) {
        years = -years;
        *cur = '-';
    } 
    if (months < 0) {
        months = -months;
        *cur = '-';
    }
    if (*cur == '-')
	cur++;

    *cur++ = 'P';

    if (years != 0.0) {
        FORMAT_ITEM(years, cur, 1, 'Y');
    }

    if (months != 0.0) {
        FORMAT_ITEM(months, cur, 1, 'M');
    }

    if (secs >= SECS_PER_DAY) {
        double tmp = floor(secs / SECS_PER_DAY);
        days += tmp;
        secs -= (tmp * SECS_PER_DAY);
    }

    FORMAT_ITEM(days, cur, 1, 'D');
    if (secs > 0.0) {
        *cur++ = 'T';
    }
    FORMAT_ITEM(secs, cur, SECS_PER_HOUR, 'H');
    FORMAT_ITEM(secs, cur, SECS_PER_MIN, 'M');
    if (secs > 0.0) {
        FORMAT_FLOAT(secs, cur, 0);
        *cur++ = 'S';
    }

    *cur = 0;

    return xmlStrdup(buf);
}

/**
 * exsltDateFormatDateTime:
 * @dt: an #exsltDateValDatePtr
 *
 * Formats @dt in xs:dateTime format.
 *
 * Returns a newly allocated string, or NULL in case of error
 */
static xmlChar *
exsltDateFormatDateTime (const exsltDateValDatePtr dt)
{
    xmlChar buf[100], *cur = buf;

    if ((dt == NULL) ||	!VALID_DATETIME(dt))
	return NULL;

    FORMAT_DATE(dt, cur);
    *cur = 'T';
    cur++;
    FORMAT_TIME(dt, cur);
    FORMAT_TZ(dt->tzo, cur);
    *cur = 0;

    return xmlStrdup(buf);
}

/**
 * exsltDateFormatDate:
 * @dt: an #exsltDateValDatePtr
 *
 * Formats @dt in xs:date format.
 *
 * Returns a newly allocated string, or NULL in case of error
 */
static xmlChar *
exsltDateFormatDate (const exsltDateValDatePtr dt)
{
    xmlChar buf[100], *cur = buf;

    if ((dt == NULL) || !VALID_DATETIME(dt))
	return NULL;

    FORMAT_DATE(dt, cur);
    if (dt->tz_flag || (dt->tzo != 0)) {
	FORMAT_TZ(dt->tzo, cur);
    }
    *cur = 0;

    return xmlStrdup(buf);
}

/**
 * exsltDateFormatTime:
 * @dt: an #exsltDateValDatePtr
 *
 * Formats @dt in xs:time format.
 *
 * Returns a newly allocated string, or NULL in case of error
 */
static xmlChar *
exsltDateFormatTime (const exsltDateValDatePtr dt)
{
    xmlChar buf[100], *cur = buf;

    if ((dt == NULL) || !VALID_TIME(dt))
	return NULL;

    FORMAT_TIME(dt, cur);
    if (dt->tz_flag || (dt->tzo != 0)) {
	FORMAT_TZ(dt->tzo, cur);
    }
    *cur = 0;

    return xmlStrdup(buf);
}

/**
 * exsltDateFormat:
 * @dt: an #exsltDateValPtr
 *
 * Formats @dt in the proper format.
 * Note: xs:gmonth and xs:gday are not formatted as there are no
 * routines that output them.
 *
 * Returns a newly allocated string, or NULL in case of error
 */
static xmlChar *
exsltDateFormat (const exsltDateValPtr dt)
{

    if (dt == NULL)
	return NULL;

    switch (dt->type) {
    case XS_DURATION:
        return exsltDateFormatDuration(&(dt->value.dur));
    case XS_DATETIME:
        return exsltDateFormatDateTime(&(dt->value.date));
    case XS_DATE:
        return exsltDateFormatDate(&(dt->value.date));
    case XS_TIME:
        return exsltDateFormatTime(&(dt->value.date));
    default:
        break;
    }

    if (dt->type & XS_GYEAR) {
        xmlChar buf[20], *cur = buf;

        FORMAT_GYEAR(dt->value.date.year, cur);
        if (dt->type == XS_GYEARMONTH) {
	    *cur = '-';
	    cur++;
	    FORMAT_GMONTH(dt->value.date.mon, cur);
        }

        if (dt->value.date.tz_flag || (dt->value.date.tzo != 0)) {
	    FORMAT_TZ(dt->value.date.tzo, cur);
        }
        *cur = 0;
        return xmlStrdup(buf);
    }

    return NULL;
}

/**
 * _exsltDateCastYMToDays:
 * @dt: an #exsltDateValPtr
 *
 * Convert mon and year of @dt to total number of days. Take the 
 * number of years since (or before) 1 AD and add the number of leap
 * years. This is a function  because negative
 * years must be handled a little differently and there is no zero year.
 *
 * Returns number of days.
 */
static long
_exsltDateCastYMToDays (const exsltDateValPtr dt)
{
    long ret;

    if (dt->value.date.year < 0)
        ret = (dt->value.date.year * 365) +
              (((dt->value.date.year+1)/4)-((dt->value.date.year+1)/100)+
               ((dt->value.date.year+1)/400)) +
              DAY_IN_YEAR(0, dt->value.date.mon, dt->value.date.year);
    else
        ret = ((dt->value.date.year-1) * 365) +
              (((dt->value.date.year-1)/4)-((dt->value.date.year-1)/100)+
               ((dt->value.date.year-1)/400)) +
              DAY_IN_YEAR(0, dt->value.date.mon, dt->value.date.year);

    return ret;
}

/**
 * TIME_TO_NUMBER:
 * @dt:  an #exsltDateValPtr
 *
 * Calculates the number of seconds in the time portion of @dt.
 *
 * Returns seconds.
 */
#define TIME_TO_NUMBER(dt)                              \
    ((double)((dt->value.date.hour * SECS_PER_HOUR) +   \
              (dt->value.date.min * SECS_PER_MIN)) + dt->value.date.sec)

/**
 * exsltDateCastDateToNumber:
 * @dt:  an #exsltDateValPtr
 *
 * Calculates the number of seconds from year zero.
 *
 * Returns seconds from zero year.
 */
static double
exsltDateCastDateToNumber (const exsltDateValPtr dt)
{
    double ret = 0.0;

    if (dt == NULL)
        return 0.0;

    if ((dt->type & XS_GYEAR) == XS_GYEAR) {
        ret = (double)_exsltDateCastYMToDays(dt) * SECS_PER_DAY;
    }

    /* add in days */
    if (dt->type == XS_DURATION) {
        ret += (double)dt->value.dur.day * SECS_PER_DAY;
        ret += dt->value.dur.sec;
    } else {
        ret += (double)dt->value.date.day * SECS_PER_DAY;
        /* add in time */
        ret += TIME_TO_NUMBER(dt);
    }


    return ret;
}

/**
 * _exsltDateTruncateDate:
 * @dt: an #exsltDateValPtr
 * @type: dateTime type to set to
 *
 * Set @dt to truncated @type.
 *
 * Returns 0 success, non-zero otherwise.
 */
static int
_exsltDateTruncateDate (exsltDateValPtr dt, exsltDateType type)
{
    if (dt == NULL)
        return 1;

    if ((type & XS_TIME) != XS_TIME) {
        dt->value.date.hour = 0;
        dt->value.date.min  = 0;
        dt->value.date.sec  = 0.0;
    }

    if ((type & XS_GDAY) != XS_GDAY)
        dt->value.date.day = 0;

    if ((type & XS_GMONTH) != XS_GMONTH)
        dt->value.date.mon = 0;

    if ((type & XS_GYEAR) != XS_GYEAR)
        dt->value.date.year = 0;

    dt->type = type;

    return 0;
}

/**
 * _exsltDayInWeek:
 * @yday: year day (1-366)
 * @yr: year
 *
 * Determine the day-in-week from @yday and @yr. 0001-01-01 was
 * a Monday so all other days are calculated from there. Take the 
 * number of years since (or before) add the number of leap years and
 * the day-in-year and mod by 7. This is a function  because negative
 * years must be handled a little differently and there is no zero year.
 *
 * Returns day in week (Sunday = 0).
 */
static long
_exsltDateDayInWeek(long yday, long yr)
{
    long ret;

    if (yr < 0) {
        ret = ((yr + (((yr+1)/4)-((yr+1)/100)+((yr+1)/400)) + yday) % 7);
        if (ret < 0) 
            ret += 7;
    } else
        ret = (((yr-1) + (((yr-1)/4)-((yr-1)/100)+((yr-1)/400)) + yday) % 7);

    return ret;
}

/*
 * macros for adding date/times and durations
 */
#define FQUOTIENT(a,b)                  ((floor(((double)a/(double)b))))
#define MODULO(a,b)                     ((a - FQUOTIENT(a,b) * b))
#define FQUOTIENT_RANGE(a,low,high)     (FQUOTIENT((a-low),(high-low)))
#define MODULO_RANGE(a,low,high)        ((MODULO((a-low),(high-low)))+low)

/**
 * _exsltDateAdd:
 * @dt: an #exsltDateValPtr
 * @dur: an #exsltDateValPtr of type #XS_DURATION
 *
 * Compute a new date/time from @dt and @dur. This function assumes @dt
 * is either #XS_DATETIME, #XS_DATE, #XS_GYEARMONTH, or #XS_GYEAR.
 *
 * Returns date/time pointer or NULL.
 */
static exsltDateValPtr
_exsltDateAdd (exsltDateValPtr dt, exsltDateValPtr dur)
{
    exsltDateValPtr ret;
    long carry, tempdays, temp;
    exsltDateValDatePtr r, d;
    exsltDateValDurationPtr u;

    if ((dt == NULL) || (dur == NULL))
        return NULL;

    ret = exsltDateCreateDate(dt->type);
    if (ret == NULL)
        return NULL;

    r = &(ret->value.date);
    d = &(dt->value.date);
    u = &(dur->value.dur);

    /* normalization */
    if (d->mon == 0)
        d->mon = 1;

    /* normalize for time zone offset */
    u->sec -= (d->tzo * 60);	/* changed from + to - (bug 153000) */
    d->tzo = 0;

    /* normalization */
    if (d->day == 0)
        d->day = 1;

    /* month */
    carry  = d->mon + u->mon;
    r->mon = (unsigned int)MODULO_RANGE(carry, 1, 13);
    carry  = (long)FQUOTIENT_RANGE(carry, 1, 13);

    /* year (may be modified later) */
    r->year = d->year + carry;
    if (r->year == 0) {
        if (d->year > 0)
            r->year--;
        else
            r->year++;
    }

    /* time zone */
    r->tzo     = d->tzo;
    r->tz_flag = d->tz_flag;

    /* seconds */
    r->sec = d->sec + u->sec;
    carry  = (long)FQUOTIENT((long)r->sec, 60);
    if (r->sec != 0.0) {
        r->sec = MODULO(r->sec, 60.0);
    }

    /* minute */
    carry += d->min;
    r->min = (unsigned int)MODULO(carry, 60);
    carry  = (long)FQUOTIENT(carry, 60);

    /* hours */
    carry  += d->hour;
    r->hour = (unsigned int)MODULO(carry, 24);
    carry   = (long)FQUOTIENT(carry, 24);

    /*
     * days
     * Note we use tempdays because the temporary values may need more
     * than 5 bits
     */
    if ((VALID_YEAR(r->year)) && (VALID_MONTH(r->mon)) &&
                  (d->day > MAX_DAYINMONTH(r->year, r->mon)))
        tempdays = MAX_DAYINMONTH(r->year, r->mon);
    else if (d->day < 1)
        tempdays = 1;
    else
        tempdays = d->day;

    tempdays += u->day + carry;

    while (1) {
        if (tempdays < 1) {
            long tmon = (long)MODULO_RANGE((int)r->mon-1, 1, 13);
            long tyr  = r->year + (long)FQUOTIENT_RANGE((int)r->mon-1, 1, 13);
            if (tyr == 0)
                tyr--;
	    /*
	     * Coverity detected an overrun in daysInMonth 
	     * of size 12 at position 12 with index variable "((r)->mon - 1)"
	     */
	    if (tmon < 0)
	        tmon = 0;
	    if (tmon > 12)
	        tmon = 12;
            tempdays += MAX_DAYINMONTH(tyr, tmon);
            carry = -1;
        } else if (tempdays > (long)MAX_DAYINMONTH(r->year, r->mon)) {
            tempdays = tempdays - MAX_DAYINMONTH(r->year, r->mon);
            carry = 1;
        } else
            break;

        temp = r->mon + carry;
        r->mon = (unsigned int)MODULO_RANGE(temp, 1, 13);
        r->year = r->year + (long)FQUOTIENT_RANGE(temp, 1, 13);
        if (r->year == 0) {
            if (temp < 1)
                r->year--;
            else
                r->year++;
	}
    }
    
    r->day = tempdays;

    /*
     * adjust the date/time type to the date values
     */
    if (ret->type != XS_DATETIME) {
        if ((r->hour) || (r->min) || (r->sec))
            ret->type = XS_DATETIME;
        else if (ret->type != XS_DATE) {
            if ((r->mon != 1) && (r->day != 1))
                ret->type = XS_DATE;
            else if ((ret->type != XS_GYEARMONTH) && (r->mon != 1))
                ret->type = XS_GYEARMONTH;
        }
    }

    return ret;
}

/**
 * exsltDateNormalize:
 * @dt: an #exsltDateValPtr
 *
 * Normalize @dt to GMT time.
 *
 */
static void
exsltDateNormalize (exsltDateValPtr dt)
{
    exsltDateValPtr dur, tmp;

    if (dt == NULL)
        return;

    if (((dt->type & XS_TIME) != XS_TIME) || (dt->value.date.tzo == 0))
        return;

    dur = exsltDateCreateDate(XS_DURATION);
    if (dur == NULL)
        return;

    tmp = _exsltDateAdd(dt, dur);
    if (tmp == NULL)
        return;

    memcpy(dt, tmp, sizeof(exsltDateVal));

    exsltDateFreeDate(tmp);
    exsltDateFreeDate(dur);

    dt->value.date.tzo = 0;
}

/**
 * _exsltDateDifference:
 * @x: an #exsltDateValPtr
 * @y: an #exsltDateValPtr
 * @flag: force difference in days
 *
 * Calculate the difference between @x and @y as a duration
 * (i.e. y - x). If the @flag is set then even if the least specific
 * format of @x or @y is xs:gYear or xs:gYearMonth.
 *
 * Returns date/time pointer or NULL.
 */
static exsltDateValPtr
_exsltDateDifference (exsltDateValPtr x, exsltDateValPtr y, int flag)
{
    exsltDateValPtr ret;

    if ((x == NULL) || (y == NULL))
        return NULL;

    if (((x->type < XS_GYEAR) || (x->type > XS_DATETIME)) ||
        ((y->type < XS_GYEAR) || (y->type > XS_DATETIME))) 
        return NULL;

    exsltDateNormalize(x);
    exsltDateNormalize(y);

    /*
     * the operand with the most specific format must be converted to
     * the same type as the operand with the least specific format.
     */
    if (x->type != y->type) {
        if (x->type < y->type) {
            _exsltDateTruncateDate(y, x->type);
        } else {
            _exsltDateTruncateDate(x, y->type);
        }
    }

    ret = exsltDateCreateDate(XS_DURATION);
    if (ret == NULL)
        return NULL;

    if (((x->type == XS_GYEAR) || (x->type == XS_GYEARMONTH)) && (!flag)) {
        /* compute the difference in months */
        ret->value.dur.mon = ((y->value.date.year * 12) + y->value.date.mon) -
                             ((x->value.date.year * 12) + x->value.date.mon);
	/* The above will give a wrong result if x and y are on different sides
	 of the September 1752. Resolution is welcome :-) */
    } else {
        ret->value.dur.day  = _exsltDateCastYMToDays(y) -
                              _exsltDateCastYMToDays(x);
        ret->value.dur.day += y->value.date.day - x->value.date.day;
        ret->value.dur.sec  = TIME_TO_NUMBER(y) - TIME_TO_NUMBER(x);
	if (ret->value.dur.day > 0.0 && ret->value.dur.sec < 0.0) {
	    ret->value.dur.day -= 1;
	    ret->value.dur.sec = ret->value.dur.sec + SECS_PER_DAY;
	} else if (ret->value.dur.day < 0.0 && ret->value.dur.sec > 0.0) {
	    ret->value.dur.day += 1;
	    ret->value.dur.sec = ret->value.dur.sec - SECS_PER_DAY;
	}
    }

    return ret;
}

/**
 * _exsltDateAddDurCalc
 * @ret: an exsltDateValPtr for the return value:
 * @x: an exsltDateValPtr for the first operand
 * @y: an exsltDateValPtr for the second operand
 *
 * Add two durations, catering for possible negative values.
 * The sum is placed in @ret.
 *
 * Returns 1 for success, 0 if error detected.
 */
static int
_exsltDateAddDurCalc (exsltDateValPtr ret, exsltDateValPtr x,
		      exsltDateValPtr y)
{
    long carry;

    /* months */
    ret->value.dur.mon = x->value.dur.mon + y->value.dur.mon;

    /* seconds */
    ret->value.dur.sec = x->value.dur.sec + y->value.dur.sec;
    carry = (long)FQUOTIENT(ret->value.dur.sec, SECS_PER_DAY);
    if (ret->value.dur.sec != 0.0) {
        ret->value.dur.sec = MODULO(ret->value.dur.sec, SECS_PER_DAY);
	/*
	 * Our function MODULO always gives us a positive value, so
	 * if we end up with a "-ve" carry we need to adjust it
	 * appropriately (bug 154021)
	 */
	if ((carry < 0) && (ret->value.dur.sec != 0)) {
	    /* change seconds to equiv negative modulus */
	    ret->value.dur.sec = ret->value.dur.sec - SECS_PER_DAY;
	    carry++;
	}
    }

    /* days */
    ret->value.dur.day = x->value.dur.day + y->value.dur.day + carry;

    /*
     * are the results indeterminate? i.e. how do you subtract days from
     * months or years?
     */
    if ((((ret->value.dur.day > 0) || (ret->value.dur.sec > 0)) &&
         (ret->value.dur.mon < 0)) ||
        (((ret->value.dur.day < 0) || (ret->value.dur.sec < 0)) &&
         (ret->value.dur.mon > 0))) {
        return 0;
    }
    return 1;
}

/**
 * _exsltDateAddDuration:
 * @x: an #exsltDateValPtr of type #XS_DURATION
 * @y: an #exsltDateValPtr of type #XS_DURATION
 *
 * Compute a new duration from @x and @y.
 *
 * Returns date/time pointer or NULL.
 */
static exsltDateValPtr
_exsltDateAddDuration (exsltDateValPtr x, exsltDateValPtr y)
{
    exsltDateValPtr ret;

    if ((x == NULL) || (y == NULL))
        return NULL;

    ret = exsltDateCreateDate(XS_DURATION);
    if (ret == NULL)
        return NULL;

    if (_exsltDateAddDurCalc(ret, x, y))
        return ret;

    exsltDateFreeDate(ret);
    return NULL;
}

/****************************************************************
 *								*
 *		EXSLT - Dates and Times functions		*
 *								*
 ****************************************************************/

/**
 * exsltDateDateTime:
 *
 * Implements the EXSLT - Dates and Times date-time() function:
 *     string date:date-time()
 * 
 * Returns the current date and time as a date/time string.
 */
static xmlChar *
exsltDateDateTime (void)
{
    xmlChar *ret = NULL;
#ifdef WITH_TIME
    exsltDateValPtr cur;

    cur = exsltDateCurrent();
    if (cur != NULL) {
	ret = exsltDateFormatDateTime(&(cur->value.date));
	exsltDateFreeDate(cur);
    }
#endif

    return ret;
}

/**
 * exsltDateDate:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times date() function:
 *     string date:date (string?)
 * 
 * Returns the date specified in the date/time string given as the
 * argument.  If no argument is given, then the current local
 * date/time, as returned by date:date-time is used as a default
 * argument.
 * The date/time string specified as an argument must be a string in
 * the format defined as the lexical representation of either
 * xs:dateTime or xs:date.  If the argument is not in either of these
 * formats, returns NULL.
 */
static xmlChar *
exsltDateDate (const xmlChar *dateTime)
{
    exsltDateValPtr dt = NULL;
    xmlChar *ret = NULL;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return NULL;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return NULL;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_DATE)) {
	    exsltDateFreeDate(dt);
	    return NULL;
	}
    }

    ret = exsltDateFormatDate(&(dt->value.date));
    exsltDateFreeDate(dt);

    return ret;
}

/**
 * exsltDateTime:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times time() function:
 *     string date:time (string?)
 * 
 * Returns the time specified in the date/time string given as the
 * argument.  If no argument is given, then the current local
 * date/time, as returned by date:date-time is used as a default
 * argument.
 * The date/time string specified as an argument must be a string in
 * the format defined as the lexical representation of either
 * xs:dateTime or xs:time.  If the argument is not in either of these
 * formats, returns NULL.
 */
static xmlChar *
exsltDateTime (const xmlChar *dateTime)
{
    exsltDateValPtr dt = NULL;
    xmlChar *ret = NULL;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return NULL;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return NULL;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_TIME)) {
	    exsltDateFreeDate(dt);
	    return NULL;
	}
    }

    ret = exsltDateFormatTime(&(dt->value.date));
    exsltDateFreeDate(dt);

    return ret;
}

/**
 * exsltDateYear:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times year() function
 *    number date:year (string?)
 * Returns the year of a date as a number.  If no argument is given,
 * then the current local date/time, as returned by date:date-time is
 * used as a default argument.
 * The date/time string specified as the first argument must be a
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 *  - xs:gYearMonth (CCYY-MM)
 *  - xs:gYear (CCYY)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateYear (const xmlChar *dateTime)
{
    exsltDateValPtr dt;
    double ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_DATE) &&
	    (dt->type != XS_GYEARMONTH) && (dt->type != XS_GYEAR)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    ret = (double) dt->value.date.year;
    exsltDateFreeDate(dt);

    return ret;
}

/**
 * exsltDateLeapYear:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times leap-year() function:
 *    boolean date:leap-yea (string?)
 * Returns true if the year given in a date is a leap year.  If no
 * argument is given, then the current local date/time, as returned by
 * date:date-time is used as a default argument.
 * The date/time string specified as the first argument must be a
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 *  - xs:gYearMonth (CCYY-MM)
 *  - xs:gYear (CCYY)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static xmlXPathObjectPtr
exsltDateLeapYear (const xmlChar *dateTime)
{
    double year;

    year = exsltDateYear(dateTime);
    if (xmlXPathIsNaN(year))
	return xmlXPathNewFloat(xmlXPathNAN);

    if (IS_LEAP((long)year))
	return xmlXPathNewBoolean(1);

    return xmlXPathNewBoolean(0);
}

/**
 * exsltDateMonthInYear:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times month-in-year() function:
 *    number date:month-in-year (string?)
 * Returns the month of a date as a number.  If no argument is given,
 * then the current local date/time, as returned by date:date-time is
 * used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 *  - xs:gYearMonth (CCYY-MM)
 *  - xs:gMonth (--MM--)
 *  - xs:gMonthDay (--MM-DD)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateMonthInYear (const xmlChar *dateTime)
{
    exsltDateValPtr dt;
    double ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_DATE) &&
	    (dt->type != XS_GYEARMONTH) && (dt->type != XS_GMONTH) &&
	    (dt->type != XS_GMONTHDAY)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    ret = (double) dt->value.date.mon;
    exsltDateFreeDate(dt);

    return ret;
}

/**
 * exsltDateMonthName:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Time month-name() function
 *    string date:month-name (string?)
 * Returns the full name of the month of a date.  If no argument is
 * given, then the current local date/time, as returned by
 * date:date-time is used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 *  - xs:gYearMonth (CCYY-MM)
 *  - xs:gMonth (--MM--)
 * If the date/time string is not in one of these formats, then an
 * empty string ('') is returned.
 * The result is an English month name: one of 'January', 'February',
 * 'March', 'April', 'May', 'June', 'July', 'August', 'September',
 * 'October', 'November' or 'December'.
 */
static const xmlChar *
exsltDateMonthName (const xmlChar *dateTime)
{
    static const xmlChar monthNames[13][10] = {
        { 0 },
	{ 'J', 'a', 'n', 'u', 'a', 'r', 'y', 0 },
	{ 'F', 'e', 'b', 'r', 'u', 'a', 'r', 'y', 0 },
	{ 'M', 'a', 'r', 'c', 'h', 0 },
	{ 'A', 'p', 'r', 'i', 'l', 0 },
	{ 'M', 'a', 'y', 0 },
	{ 'J', 'u', 'n', 'e', 0 },
	{ 'J', 'u', 'l', 'y', 0 },
	{ 'A', 'u', 'g', 'u', 's', 't', 0 },
	{ 'S', 'e', 'p', 't', 'e', 'm', 'b', 'e', 'r', 0 },
	{ 'O', 'c', 't', 'o', 'b', 'e', 'r', 0 },
	{ 'N', 'o', 'v', 'e', 'm', 'b', 'e', 'r', 0 },
	{ 'D', 'e', 'c', 'e', 'm', 'b', 'e', 'r', 0 }
    };
    int month;
    month = (int) exsltDateMonthInYear(dateTime);
    if (!VALID_MONTH(month))
      month = 0;
    return monthNames[month];
}

/**
 * exsltDateMonthAbbreviation:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Time month-abbreviation() function
 *    string date:month-abbreviation (string?)
 * Returns the abbreviation of the month of a date.  If no argument is
 * given, then the current local date/time, as returned by
 * date:date-time is used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 *  - xs:gYearMonth (CCYY-MM)
 *  - xs:gMonth (--MM--)
 * If the date/time string is not in one of these formats, then an
 * empty string ('') is returned.
 * The result is an English month abbreviation: one of 'Jan', 'Feb',
 * 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov' or
 * 'Dec'.
 */
static const xmlChar *
exsltDateMonthAbbreviation (const xmlChar *dateTime)
{
    static const xmlChar monthAbbreviations[13][4] = {
        { 0 },
	{ 'J', 'a', 'n', 0 },
	{ 'F', 'e', 'b', 0 },
	{ 'M', 'a', 'r', 0 },
	{ 'A', 'p', 'r', 0 },
	{ 'M', 'a', 'y', 0 },
	{ 'J', 'u', 'n', 0 },
	{ 'J', 'u', 'l', 0 },
	{ 'A', 'u', 'g', 0 },
	{ 'S', 'e', 'p', 0 },
	{ 'O', 'c', 't', 0 },
	{ 'N', 'o', 'v', 0 },
	{ 'D', 'e', 'c', 0 }
    };
    int month;
    month = (int) exsltDateMonthInYear(dateTime);
    if(!VALID_MONTH(month))
      month = 0;
    return monthAbbreviations[month];
}

/**
 * exsltDateWeekInYear:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times week-in-year() function
 *    number date:week-in-year (string?)
 * Returns the week of the year as a number.  If no argument is given,
 * then the current local date/time, as returned by date:date-time is
 * used as the default argument.  For the purposes of numbering,
 * counting follows ISO 8601: week 1 in a year is the week containing
 * the first Thursday of the year, with new weeks beginning on a
 * Monday.
 * The date/time string specified as the argument is a right-truncated
 * string in the format defined as the lexical representation of
 * xs:dateTime in one of the formats defined in [XML Schema Part 2:
 * Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateWeekInYear (const xmlChar *dateTime)
{
    exsltDateValPtr dt;
    long diy, diw, year, ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_DATE)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    diy = DAY_IN_YEAR(dt->value.date.day, dt->value.date.mon,
                      dt->value.date.year);

    /*
     * Determine day-in-week (0=Sun, 1=Mon, etc.) then adjust so Monday
     * is the first day-in-week
     */
    diw = (_exsltDateDayInWeek(diy, dt->value.date.year) + 6) % 7;

    /* ISO 8601 adjustment, 3 is Thu */
    diy += (3 - diw);
    if(diy < 1) {
	year = dt->value.date.year - 1;
	if(year == 0) year--;
	diy = DAY_IN_YEAR(31, 12, year) + diy;
    } else if (diy > (long)DAY_IN_YEAR(31, 12, dt->value.date.year)) {
	diy -= DAY_IN_YEAR(31, 12, dt->value.date.year);
    }

    ret = ((diy - 1) / 7) + 1;

    exsltDateFreeDate(dt);

    return (double) ret;
}

/**
 * exsltDateWeekInMonth:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times week-in-month() function
 *    number date:week-in-month (string?)
 * The date:week-in-month function returns the week in a month of a
 * date as a number. If no argument is given, then the current local
 * date/time, as returned by date:date-time is used the default
 * argument. For the purposes of numbering, the first day of the month
 * is in week 1 and new weeks begin on a Monday (so the first and last
 * weeks in a month will often have less than 7 days in them).
 * The date/time string specified as the argument is a right-truncated
 * string in the format defined as the lexical representation of
 * xs:dateTime in one of the formats defined in [XML Schema Part 2:
 * Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateWeekInMonth (const xmlChar *dateTime)
{
    exsltDateValPtr dt;
    long fdiy, fdiw, ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_DATE)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    fdiy = DAY_IN_YEAR(1, dt->value.date.mon, dt->value.date.year);
    /*
     * Determine day-in-week (0=Sun, 1=Mon, etc.) then adjust so Monday
     * is the first day-in-week
     */
    fdiw = (_exsltDateDayInWeek(fdiy, dt->value.date.year) + 6) % 7;

    ret = ((dt->value.date.day + fdiw - 1) / 7) + 1;

    exsltDateFreeDate(dt);

    return (double) ret;
}

/**
 * exsltDateDayInYear:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times day-in-year() function
 *    number date:day-in-year (string?)
 * Returns the day of a date in a year as a number.  If no argument is
 * given, then the current local date/time, as returned by
 * date:date-time is used the default argument.
 * The date/time string specified as the argument is a right-truncated
 * string in the format defined as the lexical representation of
 * xs:dateTime in one of the formats defined in [XML Schema Part 2:
 * Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateDayInYear (const xmlChar *dateTime)
{
    exsltDateValPtr dt;
    long ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_DATE)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    ret = DAY_IN_YEAR(dt->value.date.day, dt->value.date.mon,
                      dt->value.date.year);

    exsltDateFreeDate(dt);

    return (double) ret;
}

/**
 * exsltDateDayInMonth:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times day-in-month() function:
 *    number date:day-in-month (string?)
 * Returns the day of a date as a number.  If no argument is given,
 * then the current local date/time, as returned by date:date-time is
 * used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 *  - xs:gMonthDay (--MM-DD)
 *  - xs:gDay (---DD)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateDayInMonth (const xmlChar *dateTime)
{
    exsltDateValPtr dt;
    double ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_DATE) &&
	    (dt->type != XS_GMONTHDAY) && (dt->type != XS_GDAY)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    ret = (double) dt->value.date.day;
    exsltDateFreeDate(dt);

    return ret;
}

/**
 * exsltDateDayOfWeekInMonth:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times day-of-week-in-month() function:
 *    number date:day-of-week-in-month (string?)
 * Returns the day-of-the-week in a month of a date as a number
 * (e.g. 3 for the 3rd Tuesday in May).  If no argument is
 * given, then the current local date/time, as returned by
 * date:date-time is used the default argument.
 * The date/time string specified as the argument is a right-truncated
 * string in the format defined as the lexical representation of
 * xs:dateTime in one of the formats defined in [XML Schema Part 2:
 * Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateDayOfWeekInMonth (const xmlChar *dateTime)
{
    exsltDateValPtr dt;
    long ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_DATE)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    ret = ((dt->value.date.day -1) / 7) + 1;

    exsltDateFreeDate(dt);

    return (double) ret;
}

/**
 * exsltDateDayInWeek:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times day-in-week() function:
 *    number date:day-in-week (string?)
 * Returns the day of the week given in a date as a number.  If no
 * argument is given, then the current local date/time, as returned by
 * date:date-time is used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 * The numbering of days of the week starts at 1 for Sunday, 2 for
 * Monday and so on up to 7 for Saturday.
 */
static double
exsltDateDayInWeek (const xmlChar *dateTime)
{
    exsltDateValPtr dt;
    long diy, ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_DATE)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    diy = DAY_IN_YEAR(dt->value.date.day, dt->value.date.mon,
                      dt->value.date.year);

    ret = _exsltDateDayInWeek(diy, dt->value.date.year) + 1;

    exsltDateFreeDate(dt);

    return (double) ret;
}

/**
 * exsltDateDayName:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Time day-name() function
 *    string date:day-name (string?)
 * Returns the full name of the day of the week of a date.  If no
 * argument is given, then the current local date/time, as returned by
 * date:date-time is used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 * If the date/time string is not in one of these formats, then an
 * empty string ('') is returned.
 * The result is an English day name: one of 'Sunday', 'Monday',
 * 'Tuesday', 'Wednesday', 'Thursday' or 'Friday'.
 */
static const xmlChar *
exsltDateDayName (const xmlChar *dateTime)
{
    static const xmlChar dayNames[8][10] = {
        { 0 },
	{ 'S', 'u', 'n', 'd', 'a', 'y', 0 },
	{ 'M', 'o', 'n', 'd', 'a', 'y', 0 },
	{ 'T', 'u', 'e', 's', 'd', 'a', 'y', 0 },
	{ 'W', 'e', 'd', 'n', 'e', 's', 'd', 'a', 'y', 0 },
	{ 'T', 'h', 'u', 'r', 's', 'd', 'a', 'y', 0 },
	{ 'F', 'r', 'i', 'd', 'a', 'y', 0 },
	{ 'S', 'a', 't', 'u', 'r', 'd', 'a', 'y', 0 }
    };
    int day;
    day = (int) exsltDateDayInWeek(dateTime);
    if((day < 1) || (day > 7))
      day = 0;
    return dayNames[day];
}

/**
 * exsltDateDayAbbreviation:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Time day-abbreviation() function
 *    string date:day-abbreviation (string?)
 * Returns the abbreviation of the day of the week of a date.  If no
 * argument is given, then the current local date/time, as returned by
 * date:date-time is used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 * If the date/time string is not in one of these formats, then an
 * empty string ('') is returned.
 * The result is a three-letter English day abbreviation: one of
 * 'Sun', 'Mon', 'Tue', 'Wed', 'Thu' or 'Fri'.
 */
static const xmlChar *
exsltDateDayAbbreviation (const xmlChar *dateTime)
{
    static const xmlChar dayAbbreviations[8][4] = {
        { 0 },
	{ 'S', 'u', 'n', 0 },
	{ 'M', 'o', 'n', 0 },
	{ 'T', 'u', 'e', 0 },
	{ 'W', 'e', 'd', 0 },
	{ 'T', 'h', 'u', 0 },
	{ 'F', 'r', 'i', 0 },
	{ 'S', 'a', 't', 0 }
    };
    int day;
    day = (int) exsltDateDayInWeek(dateTime);
    if((day < 1) || (day > 7))
      day = 0;
    return dayAbbreviations[day];
}

/**
 * exsltDateHourInDay:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times day-in-month() function:
 *    number date:day-in-month (string?)
 * Returns the hour of the day as a number.  If no argument is given,
 * then the current local date/time, as returned by date:date-time is
 * used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:time (hh:mm:ss)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateHourInDay (const xmlChar *dateTime)
{
    exsltDateValPtr dt;
    double ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_TIME)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    ret = (double) dt->value.date.hour;
    exsltDateFreeDate(dt);

    return ret;
}

/**
 * exsltDateMinuteInHour:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times day-in-month() function:
 *    number date:day-in-month (string?)
 * Returns the minute of the hour as a number.  If no argument is
 * given, then the current local date/time, as returned by
 * date:date-time is used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:time (hh:mm:ss)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateMinuteInHour (const xmlChar *dateTime)
{
    exsltDateValPtr dt;
    double ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_TIME)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    ret = (double) dt->value.date.min;
    exsltDateFreeDate(dt);

    return ret;
}

/**
 * exsltDateSecondInMinute:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times second-in-minute() function:
 *    number date:day-in-month (string?)
 * Returns the second of the minute as a number.  If no argument is
 * given, then the current local date/time, as returned by
 * date:date-time is used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:time (hh:mm:ss)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 * 
 * Returns the second or NaN.
 */
static double
exsltDateSecondInMinute (const xmlChar *dateTime)
{
    exsltDateValPtr dt;
    double ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_TIME)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    ret = dt->value.date.sec;
    exsltDateFreeDate(dt);

    return ret;
}

/**
 * exsltDateAdd:
 * @xstr: date/time string
 * @ystr: date/time string
 *
 * Implements the date:add (string,string) function which returns the
 * date/time * resulting from adding a duration to a date/time. 
 * The first argument (@xstr) must be right-truncated date/time
 * strings in one of the formats defined in [XML Schema Part 2:
 * Datatypes]. The permitted formats are as follows: 
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss) 
 *  - xs:date (CCYY-MM-DD) 
 *  - xs:gYearMonth (CCYY-MM) 
 *  - xs:gYear (CCYY) 
 * The second argument (@ystr) is a string in the format defined for
 * xs:duration in [3.2.6 duration] of [XML Schema Part 2: Datatypes]. 
 * The return value is a right-truncated date/time strings in one of
 * the formats defined in [XML Schema Part 2: Datatypes] and listed
 * above. This value is calculated using the algorithm described in
 * [Appendix E Adding durations to dateTimes] of [XML Schema Part 2:
 * Datatypes]. 

 * Returns date/time string or NULL.
 */
static xmlChar *
exsltDateAdd (const xmlChar *xstr, const xmlChar *ystr)
{
    exsltDateValPtr dt, dur, res;
    xmlChar     *ret;   

    if ((xstr == NULL) || (ystr == NULL))
        return NULL;

    dt = exsltDateParse(xstr);
    if (dt == NULL)
        return NULL;
    else if ((dt->type < XS_GYEAR) || (dt->type > XS_DATETIME)) {
        exsltDateFreeDate(dt);
        return NULL;
    }

    dur = exsltDateParseDuration(ystr);
    if (dur == NULL) {
        exsltDateFreeDate(dt);
        return NULL;
    }

    res = _exsltDateAdd(dt, dur);

    exsltDateFreeDate(dt);
    exsltDateFreeDate(dur);

    if (res == NULL)
        return NULL;

    ret = exsltDateFormat(res);
    exsltDateFreeDate(res);

    return ret;
}

/**
 * exsltDateAddDuration:
 * @xstr:      first duration string
 * @ystr:      second duration string
 *
 * Implements the date:add-duration (string,string) function which returns
 * the duration resulting from adding two durations together. 
 * Both arguments are strings in the format defined for xs:duration
 * in [3.2.6 duration] of [XML Schema Part 2: Datatypes]. If either
 * argument is not in this format, the function returns an empty string
 * (''). 
 * The return value is a string in the format defined for xs:duration
 * in [3.2.6 duration] of [XML Schema Part 2: Datatypes]. 
 * The durations can usually be added by summing the numbers given for
 * each of the components in the durations. However, if the durations
 * are differently signed, then this sometimes results in durations
 * that are impossible to express in this syntax (e.g. 'P1M' + '-P1D').
 * In these cases, the function returns an empty string (''). 
 *
 * Returns duration string or NULL.
 */
static xmlChar *
exsltDateAddDuration (const xmlChar *xstr, const xmlChar *ystr)
{
    exsltDateValPtr x, y, res;
    xmlChar     *ret;   

    if ((xstr == NULL) || (ystr == NULL))
        return NULL;

    x = exsltDateParseDuration(xstr);
    if (x == NULL)
        return NULL;

    y = exsltDateParseDuration(ystr);
    if (y == NULL) {
        exsltDateFreeDate(x);
        return NULL;
    }

    res = _exsltDateAddDuration(x, y);

    exsltDateFreeDate(x);
    exsltDateFreeDate(y);

    if (res == NULL)
        return NULL;

    ret = exsltDateFormatDuration(&(res->value.dur));
    exsltDateFreeDate(res);

    return ret;
}

/**
 * exsltDateSumFunction:
 * @ns:      a node set of duration strings
 *
 * The date:sum function adds a set of durations together. 
 * The string values of the nodes in the node set passed as an argument 
 * are interpreted as durations and added together as if using the 
 * date:add-duration function. (from exslt.org)
 *
 * The return value is a string in the format defined for xs:duration
 * in [3.2.6 duration] of [XML Schema Part 2: Datatypes]. 
 * The durations can usually be added by summing the numbers given for
 * each of the components in the durations. However, if the durations
 * are differently signed, then this sometimes results in durations
 * that are impossible to express in this syntax (e.g. 'P1M' + '-P1D').
 * In these cases, the function returns an empty string (''). 
 *
 * Returns duration string or NULL.
 */
static void
exsltDateSumFunction (xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlNodeSetPtr ns;
    void *user = NULL;
    xmlChar *tmp;
    exsltDateValPtr x, total;
    xmlChar *ret;
    int i;

    if (nargs != 1) {
	xmlXPathSetArityError (ctxt);
	return;
    }

    /* We need to delay the freeing of value->user */
    if ((ctxt->value != NULL) && ctxt->value->boolval != 0) {
	user = ctxt->value->user;
	ctxt->value->boolval = 0;
	ctxt->value->user = NULL;
    }

    ns = xmlXPathPopNodeSet (ctxt);
    if (xmlXPathCheckError (ctxt))
	return;

    if ((ns == NULL) || (ns->nodeNr == 0)) {
	xmlXPathReturnEmptyString (ctxt);
	if (ns != NULL)
	    xmlXPathFreeNodeSet (ns);
	return;
    }

    total = exsltDateCreateDate (XS_DURATION);
    if (total == NULL) {
        xmlXPathFreeNodeSet (ns);
        return;
    }

    for (i = 0; i < ns->nodeNr; i++) {
    	int result;
	tmp = xmlXPathCastNodeToString (ns->nodeTab[i]);
	if (tmp == NULL) {
	    xmlXPathFreeNodeSet (ns);
	    exsltDateFreeDate (total);
	    return;
	}

	x = exsltDateParseDuration (tmp);
	if (x == NULL) {
	    xmlFree (tmp);
	    exsltDateFreeDate (total);
	    xmlXPathFreeNodeSet (ns);
	    xmlXPathReturnEmptyString (ctxt);
	    return;
	}

	result = _exsltDateAddDurCalc(total, total, x);

	exsltDateFreeDate (x);
	xmlFree (tmp);
	if (!result) {
	    exsltDateFreeDate (total);
	    xmlXPathFreeNodeSet (ns);
	    xmlXPathReturnEmptyString (ctxt);
	    return;
	}
    }

    ret = exsltDateFormatDuration (&(total->value.dur));
    exsltDateFreeDate (total);

    xmlXPathFreeNodeSet (ns);
    if (user != NULL)
	xmlFreeNodeList ((xmlNodePtr) user);

    if (ret == NULL)
	xmlXPathReturnEmptyString (ctxt);
    else
	xmlXPathReturnString (ctxt, ret);
}

/**
 * exsltDateSeconds:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times seconds() function:
 *    number date:seconds(string?)
 * The date:seconds function returns the number of seconds specified
 * by the argument string. If no argument is given, then the current
 * local date/time, as returned by exsltDateCurrent() is used as the
 * default argument. If the date/time string is a xs:duration, then the
 * years and months must be zero (or not present). Parsing a duration
 * converts the fields to seconds. If the date/time string is not a 
 * duration (and not null), then the legal formats are:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date     (CCYY-MM-DD)
 *  - xs:gYearMonth (CCYY-MM)
 *  - xs:gYear      (CCYY)
 * In these cases the difference between the @dateTime and 
 * 1970-01-01T00:00:00Z is calculated and converted to seconds.
 *
 * Note that there was some confusion over whether "difference" meant
 * that a dateTime of 1970-01-01T00:00:01Z should be a positive one or
 * a negative one.  After correspondence with exslt.org, it was determined
 * that the intent of the specification was to have it positive.  The
 * coding was modified in July 2003 to reflect this.
 *
 * Returns seconds or Nan.
 */
static double
exsltDateSeconds (const xmlChar *dateTime)
{
    exsltDateValPtr dt;
    double ret = xmlXPathNAN;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
        dt = exsltDateParseDuration(dateTime);
        if (dt == NULL)
            dt = exsltDateParse(dateTime);
    }

    if (dt == NULL)
        return xmlXPathNAN;

    if ((dt->type <= XS_DATETIME) && (dt->type >= XS_GYEAR)) {
        exsltDateValPtr y, dur;

        /*
         * compute the difference between the given (or current) date
         * and epoch date
         */
        y = exsltDateCreateDate(XS_DATETIME);
        if (y != NULL) {
            y->value.date.year = 1970;
            y->value.date.mon  = 1;
            y->value.date.day  = 1;
            y->value.date.tz_flag = 1;

            dur = _exsltDateDifference(y, dt, 1);
            if (dur != NULL) {
                ret = exsltDateCastDateToNumber(dur); 
                exsltDateFreeDate(dur);
            }
            exsltDateFreeDate(y);
        }

    } else if ((dt->type == XS_DURATION) && (dt->value.dur.mon == 0))
        ret = exsltDateCastDateToNumber(dt);

    exsltDateFreeDate(dt);

    return ret;
}

/**
 * exsltDateDifference:
 * @xstr: date/time string
 * @ystr: date/time string
 *
 * Implements the date:difference (string,string) function which returns
 * the duration between the first date and the second date. If the first
 * date occurs before the second date, then the result is a positive
 * duration; if it occurs after the second date, the result is a
 * negative duration.  The two dates must both be right-truncated
 * date/time strings in one of the formats defined in [XML Schema Part
 * 2: Datatypes]. The date/time with the most specific format (i.e. the
 * least truncation) is converted into the same format as the date with
 * the least specific format (i.e. the most truncation). The permitted
 * formats are as follows, from most specific to least specific: 
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss) 
 *  - xs:date (CCYY-MM-DD) 
 *  - xs:gYearMonth (CCYY-MM) 
 *  - xs:gYear (CCYY) 
 * If either of the arguments is not in one of these formats,
 * date:difference returns the empty string (''). 
 * The difference between the date/times is returned as a string in the
 * format defined for xs:duration in [3.2.6 duration] of [XML Schema
 * Part 2: Datatypes]. 
 * If the date/time string with the least specific format is in either
 * xs:gYearMonth or xs:gYear format, then the number of days, hours,
 * minutes and seconds in the duration string must be equal to zero.
 * (The format of the string will be PnYnM.) The number of months
 * specified in the duration must be less than 12. 
 * Otherwise, the number of years and months in the duration string
 * must be equal to zero. (The format of the string will be
 * PnDTnHnMnS.) The number of seconds specified in the duration string
 * must be less than 60; the number of minutes must be less than 60;
 * the number of hours must be less than 24. 
 *
 * Returns duration string or NULL.
 */
static xmlChar *
exsltDateDifference (const xmlChar *xstr, const xmlChar *ystr)
{
    exsltDateValPtr x, y, dur;
    xmlChar        *ret = NULL;   

    if ((xstr == NULL) || (ystr == NULL))
        return NULL;

    x = exsltDateParse(xstr);
    if (x == NULL)
        return NULL;

    y = exsltDateParse(ystr);
    if (y == NULL) {
        exsltDateFreeDate(x);
        return NULL;
    }

    if (((x->type < XS_GYEAR) || (x->type > XS_DATETIME)) ||
        ((y->type < XS_GYEAR) || (y->type > XS_DATETIME)))  {
	exsltDateFreeDate(x);
	exsltDateFreeDate(y);
        return NULL;
    }

    dur = _exsltDateDifference(x, y, 0);

    exsltDateFreeDate(x);
    exsltDateFreeDate(y);

    if (dur == NULL)
        return NULL;

    ret = exsltDateFormatDuration(&(dur->value.dur));
    exsltDateFreeDate(dur);

    return ret;
}

/**
 * exsltDateDuration:
 * @number: a xmlChar string
 *
 * Implements the The date:duration function returns a duration string
 * representing the number of seconds specified by the argument string.
 * If no argument is given, then the result of calling date:seconds
 * without any arguments is used as a default argument. 
 * The duration is returned as a string in the format defined for
 * xs:duration in [3.2.6 duration] of [XML Schema Part 2: Datatypes]. 
 * The number of years and months in the duration string must be equal
 * to zero. (The format of the string will be PnDTnHnMnS.) The number
 * of seconds specified in the duration string must be less than 60;
 * the number of minutes must be less than 60; the number of hours must
 * be less than 24. 
 * If the argument is Infinity, -Infinity or NaN, then date:duration
 * returns an empty string (''). 
 *
 * Returns duration string or NULL.
 */
static xmlChar *
exsltDateDuration (const xmlChar *number)
{
    exsltDateValPtr dur;
    double       secs;
    xmlChar     *ret;

    if (number == NULL)
        secs = exsltDateSeconds(number);
    else
        secs = xmlXPathCastStringToNumber(number);

    if ((xmlXPathIsNaN(secs)) || (xmlXPathIsInf(secs)))
        return NULL;

    dur = exsltDateCreateDate(XS_DURATION);
    if (dur == NULL)
        return NULL;

    dur->value.dur.sec = secs;

    ret = exsltDateFormatDuration(&(dur->value.dur));
    exsltDateFreeDate(dur);

    return ret;
}

/****************************************************************
 *								*
 *		Wrappers for use by the XPath engine		*
 *								*
 ****************************************************************/

#ifdef WITH_TIME
/**
 * exsltDateDateTimeFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateDateTime() for use by the XPath engine.
 */
static void
exsltDateDateTimeFunction (xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlChar *ret;

    if (nargs != 0) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    ret = exsltDateDateTime();
    if (ret == NULL)
        xmlXPathReturnEmptyString(ctxt);
    else
        xmlXPathReturnString(ctxt, ret);
}
#endif

/**
 * exsltDateDateFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateDate() for use by the XPath engine.
 */
static void
exsltDateDateFunction (xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlChar *ret, *dt = NULL;

    if ((nargs < 0) || (nargs > 1)) {
	xmlXPathSetArityError(ctxt);
	return;
    }
    if (nargs == 1) {
	dt = xmlXPathPopString(ctxt);
	if (xmlXPathCheckError(ctxt)) {
	    xmlXPathSetTypeError(ctxt);
	    return;
	}
    }

    ret = exsltDateDate(dt);

    if (ret == NULL) {
	xsltGenericDebug(xsltGenericDebugContext,
			 "{http://exslt.org/dates-and-times}date: "
			 "invalid date or format %s\n", dt);
	xmlXPathReturnEmptyString(ctxt);
    } else {
	xmlXPathReturnString(ctxt, ret);
    }

    if (dt != NULL)
	xmlFree(dt);
}

/**
 * exsltDateTimeFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateTime() for use by the XPath engine.
 */
static void
exsltDateTimeFunction (xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlChar *ret, *dt = NULL;

    if ((nargs < 0) || (nargs > 1)) {
	xmlXPathSetArityError(ctxt);
	return;
    }
    if (nargs == 1) {
	dt = xmlXPathPopString(ctxt);
	if (xmlXPathCheckError(ctxt)) {
	    xmlXPathSetTypeError(ctxt);
	    return;
	}
    }

    ret = exsltDateTime(dt);

    if (ret == NULL) {
	xsltGenericDebug(xsltGenericDebugContext,
			 "{http://exslt.org/dates-and-times}time: "
			 "invalid date or format %s\n", dt);
	xmlXPathReturnEmptyString(ctxt);
    } else {
	xmlXPathReturnString(ctxt, ret);
    }

    if (dt != NULL)
	xmlFree(dt);
}

/**
 * exsltDateYearFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateYear() for use by the XPath engine.
 */
static void
exsltDateYearFunction (xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlChar *dt = NULL;
    double ret;

    if ((nargs < 0) || (nargs > 1)) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    if (nargs == 1) {
	dt = xmlXPathPopString(ctxt);
	if (xmlXPathCheckError(ctxt)) {
	    xmlXPathSetTypeError(ctxt);
	    return;
	}
    }

    ret = exsltDateYear(dt);

    if (dt != NULL)
	xmlFree(dt);

    xmlXPathReturnNumber(ctxt, ret);
}

/**
 * exsltDateLeapYearFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateLeapYear() for use by the XPath engine.
 */
static void
exsltDateLeapYearFunction (xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlChar *dt = NULL;
    xmlXPathObjectPtr ret;

    if ((nargs < 0) || (nargs > 1)) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    if (nargs == 1) {
	dt = xmlXPathPopString(ctxt);
	if (xmlXPathCheckError(ctxt)) {
	    xmlXPathSetTypeError(ctxt);
	    return;
	}
    }

    ret = exsltDateLeapYear(dt);

    if (dt != NULL)
	xmlFree(dt);

    valuePush(ctxt, ret);
}

#define X_IN_Y(x, y)						\
static void							\
exsltDate##x##In##y##Function (xmlXPathParserContextPtr ctxt,	\
			      int nargs) {			\
    xmlChar *dt = NULL;						\
    double ret;							\
								\
    if ((nargs < 0) || (nargs > 1)) {				\
	xmlXPathSetArityError(ctxt);				\
	return;							\
    }								\
								\
    if (nargs == 1) {						\
	dt = xmlXPathPopString(ctxt);				\
	if (xmlXPathCheckError(ctxt)) {				\
	    xmlXPathSetTypeError(ctxt);				\
	    return;						\
	}							\
    }								\
								\
    ret = exsltDate##x##In##y(dt);				\
								\
    if (dt != NULL)						\
	xmlFree(dt);						\
								\
    xmlXPathReturnNumber(ctxt, ret);				\
}

/**
 * exsltDateMonthInYearFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateMonthInYear() for use by the XPath engine.
 */
X_IN_Y(Month,Year)

/**
 * exsltDateMonthNameFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateMonthName() for use by the XPath engine.
 */
static void
exsltDateMonthNameFunction (xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlChar *dt = NULL;
    const xmlChar *ret;

    if ((nargs < 0) || (nargs > 1)) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    if (nargs == 1) {
	dt = xmlXPathPopString(ctxt);
	if (xmlXPathCheckError(ctxt)) {
	    xmlXPathSetTypeError(ctxt);
	    return;
	}
    }

    ret = exsltDateMonthName(dt);

    if (dt != NULL)
	xmlFree(dt);

    if (ret == NULL)
	xmlXPathReturnEmptyString(ctxt);
    else
	xmlXPathReturnString(ctxt, xmlStrdup(ret));
}

/**
 * exsltDateMonthAbbreviationFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateMonthAbbreviation() for use by the XPath engine.
 */
static void
exsltDateMonthAbbreviationFunction (xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlChar *dt = NULL;
    const xmlChar *ret;

    if ((nargs < 0) || (nargs > 1)) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    if (nargs == 1) {
	dt = xmlXPathPopString(ctxt);
	if (xmlXPathCheckError(ctxt)) {
	    xmlXPathSetTypeError(ctxt);
	    return;
	}
    }

    ret = exsltDateMonthAbbreviation(dt);

    if (dt != NULL)
	xmlFree(dt);

    if (ret == NULL)
	xmlXPathReturnEmptyString(ctxt);
    else
	xmlXPathReturnString(ctxt, xmlStrdup(ret));
}

/**
 * exsltDateWeekInYearFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateWeekInYear() for use by the XPath engine.
 */
X_IN_Y(Week,Year)

/**
 * exsltDateWeekInMonthFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateWeekInMonthYear() for use by the XPath engine.
 */
X_IN_Y(Week,Month)

/**
 * exsltDateDayInYearFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateDayInYear() for use by the XPath engine.
 */
X_IN_Y(Day,Year)

/**
 * exsltDateDayInMonthFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateDayInMonth() for use by the XPath engine.
 */
X_IN_Y(Day,Month)

/**
 * exsltDateDayOfWeekInMonthFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDayOfWeekInMonth() for use by the XPath engine.
 */
X_IN_Y(DayOfWeek,Month)

/**
 * exsltDateDayInWeekFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateDayInWeek() for use by the XPath engine.
 */
X_IN_Y(Day,Week)

/**
 * exsltDateDayNameFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateDayName() for use by the XPath engine.
 */
static void
exsltDateDayNameFunction (xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlChar *dt = NULL;
    const xmlChar *ret;

    if ((nargs < 0) || (nargs > 1)) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    if (nargs == 1) {
	dt = xmlXPathPopString(ctxt);
	if (xmlXPathCheckError(ctxt)) {
	    xmlXPathSetTypeError(ctxt);
	    return;
	}
    }

    ret = exsltDateDayName(dt);

    if (dt != NULL)
	xmlFree(dt);

    if (ret == NULL)
	xmlXPathReturnEmptyString(ctxt);
    else
	xmlXPathReturnString(ctxt, xmlStrdup(ret));
}

/**
 * exsltDateMonthDayFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateDayAbbreviation() for use by the XPath engine.
 */
static void
exsltDateDayAbbreviationFunction (xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlChar *dt = NULL;
    const xmlChar *ret;

    if ((nargs < 0) || (nargs > 1)) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    if (nargs == 1) {
	dt = xmlXPathPopString(ctxt);
	if (xmlXPathCheckError(ctxt)) {
	    xmlXPathSetTypeError(ctxt);
	    return;
	}
    }

    ret = exsltDateDayAbbreviation(dt);

    if (dt != NULL)
	xmlFree(dt);

    if (ret == NULL)
	xmlXPathReturnEmptyString(ctxt);
    else
	xmlXPathReturnString(ctxt, xmlStrdup(ret));
}


/**
 * exsltDateHourInDayFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateHourInDay() for use by the XPath engine.
 */
X_IN_Y(Hour,Day)

/**
 * exsltDateMinuteInHourFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateMinuteInHour() for use by the XPath engine.
 */
X_IN_Y(Minute,Hour)

/**
 * exsltDateSecondInMinuteFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateSecondInMinute() for use by the XPath engine.
 */
X_IN_Y(Second,Minute)

/**
 * exsltDateSecondsFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateSeconds() for use by the XPath engine.
 */
static void
exsltDateSecondsFunction (xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlChar *str = NULL;
    double   ret;

    if (nargs > 1) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    if (nargs == 1) {
	str = xmlXPathPopString(ctxt);
	if (xmlXPathCheckError(ctxt)) {
	    xmlXPathSetTypeError(ctxt);
	    return;
	}
    }

    ret = exsltDateSeconds(str);
    if (str != NULL)
	xmlFree(str);

    xmlXPathReturnNumber(ctxt, ret);
}

/**
 * exsltDateAddFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps exsltDateAdd() for use by the XPath processor.
 */
static void
exsltDateAddFunction (xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlChar *ret, *xstr, *ystr;

    if (nargs != 2) {
	xmlXPathSetArityError(ctxt);
	return;
    }
    ystr = xmlXPathPopString(ctxt);
    if (xmlXPathCheckError(ctxt))
	return;

    xstr = xmlXPathPopString(ctxt);
    if (xmlXPathCheckError(ctxt)) {
        xmlFree(ystr);
	return;
    }

    ret = exsltDateAdd(xstr, ystr);

    xmlFree(ystr);
    xmlFree(xstr);

    if (ret == NULL)
        xmlXPathReturnEmptyString(ctxt);
    else
	xmlXPathReturnString(ctxt, ret);
}

/**
 * exsltDateAddDurationFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps exsltDateAddDuration() for use by the XPath processor.
 */
static void
exsltDateAddDurationFunction (xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlChar *ret, *xstr, *ystr;

    if (nargs != 2) {
	xmlXPathSetArityError(ctxt);
	return;
    }
    ystr = xmlXPathPopString(ctxt);
    if (xmlXPathCheckError(ctxt))
	return;

    xstr = xmlXPathPopString(ctxt);
    if (xmlXPathCheckError(ctxt)) {
        xmlFree(ystr);
	return;
    }

    ret = exsltDateAddDuration(xstr, ystr);

    xmlFree(ystr);
    xmlFree(xstr);

    if (ret == NULL)
        xmlXPathReturnEmptyString(ctxt);
    else
	xmlXPathReturnString(ctxt, ret);
}

/**
 * exsltDateDifferenceFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps exsltDateDifference() for use by the XPath processor.
 */
static void
exsltDateDifferenceFunction (xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlChar *ret, *xstr, *ystr;

    if (nargs != 2) {
	xmlXPathSetArityError(ctxt);
	return;
    }
    ystr = xmlXPathPopString(ctxt);
    if (xmlXPathCheckError(ctxt))
	return;

    xstr = xmlXPathPopString(ctxt);
    if (xmlXPathCheckError(ctxt)) {
        xmlFree(ystr);
	return;
    }

    ret = exsltDateDifference(xstr, ystr);

    xmlFree(ystr);
    xmlFree(xstr);

    if (ret == NULL)
        xmlXPathReturnEmptyString(ctxt);
    else
	xmlXPathReturnString(ctxt, ret);
}

/**
 * exsltDateDurationFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps exsltDateDuration() for use by the XPath engine
 */
static void
exsltDateDurationFunction (xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlChar *ret;
    xmlChar *number = NULL;

    if ((nargs < 0) || (nargs > 1)) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    if (nargs == 1) {
	number = xmlXPathPopString(ctxt);
	if (xmlXPathCheckError(ctxt)) {
	    xmlXPathSetTypeError(ctxt);
	    return;
	}
    }

    ret = exsltDateDuration(number);

    if (number != NULL)
	xmlFree(number);

    if (ret == NULL)
	xmlXPathReturnEmptyString(ctxt);
    else
	xmlXPathReturnString(ctxt, ret);
}

/**
 * exsltDateRegister:
 *
 * Registers the EXSLT - Dates and Times module
 */
void
exsltDateRegister (void)
{
    xsltRegisterExtModuleFunction ((const xmlChar *) "add",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateAddFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "add-duration",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateAddDurationFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "date",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateDateFunction);
#ifdef WITH_TIME
    xsltRegisterExtModuleFunction ((const xmlChar *) "date-time",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateDateTimeFunction);
#endif
    xsltRegisterExtModuleFunction ((const xmlChar *) "day-abbreviation",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateDayAbbreviationFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "day-in-month",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateDayInMonthFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "day-in-week",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateDayInWeekFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "day-in-year",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateDayInYearFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "day-name",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateDayNameFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "day-of-week-in-month",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateDayOfWeekInMonthFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "difference",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateDifferenceFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "duration",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateDurationFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "hour-in-day",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateHourInDayFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "leap-year",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateLeapYearFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "minute-in-hour",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateMinuteInHourFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "month-abbreviation",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateMonthAbbreviationFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "month-in-year",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateMonthInYearFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "month-name",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateMonthNameFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "second-in-minute",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateSecondInMinuteFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "seconds",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateSecondsFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "sum",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateSumFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "time",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateTimeFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "week-in-month",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateWeekInMonthFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "week-in-year",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateWeekInYearFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "year",
				   (const xmlChar *) EXSLT_DATE_NAMESPACE,
				   exsltDateYearFunction);
}

/**
 * exsltDateXpathCtxtRegister:
 *
 * Registers the EXSLT - Dates and Times module for use outside XSLT
 */
int
exsltDateXpathCtxtRegister (xmlXPathContextPtr ctxt, const xmlChar *prefix)
{
    if (ctxt
        && prefix
        && !xmlXPathRegisterNs(ctxt,
                               prefix,
                               (const xmlChar *) EXSLT_DATE_NAMESPACE)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "add",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateAddFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "add-duration",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateAddDurationFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "date",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateDateFunction)
#ifdef WITH_TIME
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "date-time",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateDateTimeFunction)
#endif
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "day-abbreviation",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateDayAbbreviationFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "day-in-month",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateDayInMonthFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "day-in-week",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateDayInWeekFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "day-in-year",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateDayInYearFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "day-name",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateDayNameFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "day-of-week-in-month",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateDayOfWeekInMonthFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "difference",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateDifferenceFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "duration",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateDurationFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "hour-in-day",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateHourInDayFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "leap-year",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateLeapYearFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "minute-in-hour",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateMinuteInHourFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "month-abbreviation",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateMonthAbbreviationFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "month-in-year",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateMonthInYearFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "month-name",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateMonthNameFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "second-in-minute",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateSecondInMinuteFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "seconds",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateSecondsFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "sum",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateSumFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "time",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateTimeFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "week-in-month",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateWeekInMonthFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "week-in-year",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateWeekInYearFunction)
        && !xmlXPathRegisterFuncNS(ctxt,
                                   (const xmlChar *) "year",
                                   (const xmlChar *) EXSLT_DATE_NAMESPACE,
                                   exsltDateYearFunction)) {
        return 0;
    }
    return -1;
}

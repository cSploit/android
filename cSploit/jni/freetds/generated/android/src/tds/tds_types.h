/*
 * This file produced from ./types.pl
 */

/**
 * Return the number of bytes needed by specified type.
 */
int
tds_get_size_by_type(int servertype)
{
	switch (servertype) {
	case SYBVOID:
		return 0;
	case SYBBIT:
	case SYBBITN:
	case SYBINT1:
	case SYBSINT1:
	case SYBUINT1:
		return 1;
	case SYBINT2:
	case SYBUINT2:
		return 2;
	case SYBMSDATE:
		return 3;
	case SYBDATE:
	case SYBDATEN:
	case SYBDATETIME4:
	case SYBINT4:
	case SYBMONEY4:
	case SYBREAL:
	case SYBTIME:
	case SYBTIMEN:
	case SYBUINT4:
		return 4;
	case SYB5INT8:
	case SYBDATETIME:
	case SYBFLT8:
	case SYBINT8:
	case SYBINTERVAL:
	case SYBMONEY:
	case SYBUINT8:
		return 8;
	case SYBUNIQUE:
		return 16;
	default:
		return -1;
	}
}

/**
 * tds_get_varint_size() returns the size of a variable length integer
 * returned in a TDS 7.0 result string
 */
int
tds_get_varint_size(TDSCONNECTION * conn, int datatype)
{
	switch (datatype) {
	case SYBBIT:
	case SYBDATETIME:
	case SYBDATETIME4:
	case SYBFLT8:
	case SYBINT1:
	case SYBINT2:
	case SYBINT4:
	case SYBMONEY:
	case SYBMONEY4:
	case SYBREAL:
	case SYBVOID:
		return 0;
	case SYBIMAGE:
	case SYBTEXT:
		return 4;
	}

	if (IS_TDS7_PLUS(conn)) {
		switch (datatype) {
		case SYBINT8:
			return 0;
		case XSYBBINARY:
		case XSYBCHAR:
		case XSYBNCHAR:
		case XSYBNVARCHAR:
		case XSYBVARBINARY:
		case XSYBVARCHAR:
			return 2;
		case SYBNTEXT:
		case SYBVARIANT:
			return 4;
		case SYBMSUDT:
		case SYBMSXML:
			return 8;
		}
	} else if (IS_TDS50(conn)) {
		switch (datatype) {
		case SYB5INT8:
		case SYBDATE:
		case SYBINTERVAL:
		case SYBSINT1:
		case SYBTIME:
		case SYBUINT1:
		case SYBUINT2:
		case SYBUINT4:
		case SYBUINT8:
			return 0;
		case SYBUNITEXT:
		case SYBXML:
			return 4;
		case SYBLONGBINARY:
		case SYBLONGCHAR:
			return 5;
		}
	}
	return 1;
}

/**
 * Return type suitable for conversions (convert all nullable types to fixed type)
 * @param srctype type to convert
 * @param colsize size of type
 * @result type for conversion
 */
int
tds_get_conversion_type(int srctype, int colsize)
{
	switch (srctype) {
	case SYBBITN:
		return SYBBIT;
	case SYBDATEN:
		return SYBDATE;
	case SYBDATETIMN:
		switch (colsize) {
		case 8:
			return SYBDATETIME;
		case 4:
			return SYBDATETIME4;
		}
		break;
	case SYBFLTN:
		switch (colsize) {
		case 8:
			return SYBFLT8;
		case 4:
			return SYBREAL;
		}
		break;
	case SYBINTN:
		switch (colsize) {
		case 8:
			return SYBINT8;
		case 4:
			return SYBINT4;
		case 2:
			return SYBINT2;
		case 1:
			return SYBINT1;
		}
		break;
	case SYBMONEYN:
		switch (colsize) {
		case 8:
			return SYBMONEY;
		case 4:
			return SYBMONEY4;
		}
		break;
	case SYBTIMEN:
		return SYBTIME;
	case SYBUINTN:
		switch (colsize) {
		case 8:
			return SYBUINT8;
		case 4:
			return SYBUINT4;
		case 2:
			return SYBUINT2;
		case 1:
			return SYBUINT1;
		}
		break;
	case SYB5INT8:
		return SYBINT8;
	}
	return srctype;
}


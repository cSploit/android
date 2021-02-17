#ifdef _WIN32
	#ifndef _CRT_SECURE_NO_WARNINGS
	/*	define _CRT_SECURE_NO_WARNINGS */
	#endif
	#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0400
	#endif

	#include <windows.h>
#endif /* _WIN32 */

#include <sqlfront.h>
#include <sqldb.h>

#include <cassert>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

using namespace std;

#if defined(_INC_SQLFRONT)
#define MSGNO_T DBINT
#define MSGNO_LINE DBUSMALLINT
#else
#define MSGNO_T int
#define MSGNO_LINE int
#define LPCSTR char*
#define LPCBYTE unsigned char*
#endif	/* _INC_SQLFRONT */

/*
 * Classes for reading the native bcp data file
 */
struct counted_string : public std::string
{};

istream&
operator>>( istream& is, counted_string& output ) 
{
	unsigned char len;
	is.read( reinterpret_cast<char*>(&len), sizeof(len) );
	streamsize nread = is.gcount();
	assert(is.good() && nread == 1);
	char buf[80];
	if( sizeof(buf) <= len ){
		cerr << "error reading counted_string at offset 0x" << hex << is.tellg() << "; "
			 << "len = 0x" << hex << (unsigned short)len << ", should be <= 30\n";
			 abort();  
	}
	nread = is.readsome( buf, len );
	assert(len == nread);
	buf[len] = '\0';
	std::string& that(output);
	that = buf;
	return is;
}

template <typename T>
struct number
{
	T datum;
	number() : datum(0) {}
};

template <typename T>
istream&
operator>>( istream& is, number<T>& output ) 
{
	static const streamsize len(sizeof(T));
	streamsize nread = is.readsome( reinterpret_cast<char*>(&output.datum), len );
	assert(is.good() && len == nread);
	return is;
}

template <typename T>
ostream&
operator<<( ostream& os, const number<T>& input ) 
{
	return os << input.datum;
}

istream&
operator>>( istream& is, DBDATETIME& dt ) 
{
	streamsize nread = is.readsome( reinterpret_cast<char*>(&dt), sizeof(dt) );
	assert(is.good() && sizeof(dt) == nread);
	
	return is;
}

ostream&
operator<<( ostream& os, DBDATETIME& dt ) 
{
	DBDATEREC date;
	RETCODE erc = dbdatecrack( 0, &date, &dt );
	assert(erc == SUCCEED);

	os << setfill('0') << right 
	   << date.year 
	   << '-' << setw(2) << (1+date.month)
	   << '-' << setw(2) << date.day 
	   << ' ' << setw(2) << date.hour 
	   << ':' << setw(2) << date.minute
	   << setfill(' ');	
	return os;
}

/*
 * Error and Message handlers
 */
int
ErrorHandler(	PDBPROCESS	dbproc 
			  , DBINT		severity
			  , DBINT		dberr 
			  , DBINT		oserr 
			  , LPCSTR		dberrstr
			  , LPCSTR		oserrstr
			  )
{
		cerr  << "DB-Library error " << dberr << " (severity " << severity << "):\n";
		cerr  << "dbproc:\t"		<< dbproc 			<< '\n'
			  << "message:\t"		<< dberrstr 	<< '\n';
		if( oserr ) {
			cerr	<< "OS error "	<< oserr 	<< ": "	<< oserrstr << '\n';
		}
		return INT_CANCEL;
}

int
MessageHandler(	  PDBPROCESS	dbproc
				, MSGNO_T		msgno 
				, DBINT			msgstate 
				, DBINT			severity 
				, LPCSTR		msgtext 
				, LPCSTR		srvname 
				, LPCSTR		procname 
				, MSGNO_LINE	line 
				)
{
	switch(msgno) {
	case 5701:
	case 5703:
		return 0;
	} 
	if( msgno > 0 ) {
		cerr	<< "Error "		<< msgno 
				<< " (severity " << severity << ") " 
				<< "from "		<< srvname;
	}

	if( procname )	/* potentially empty, std::string() */
		cerr << ".." << procname << "(line " << line << ")";
	cerr << '\n';

	cerr	<< "dbproc:\t"		<< dbproc			<< '\n'
		<< "message:\t"		<< msgtext 	<< '\n';

	return 0;
}

/*
 * Data and row buffer types
 */
struct data_t {
	char *name;
	short type;
	double ftype;
	char *date;
} data[] = {
	  {"AddressLine", 39, 3.9, "Feb 27 2009  2:39PM" }
	, {"Date",        58, 5.8, "Feb 27 2009  2:39PM" }
	, {"DateOnly",    61, 6.1, "Feb 27 2009  2:39PM" }
	, {"Handle",      39, 3.9, "Feb 27 2009  2:39PM" }
};
struct buf_t : public data_t
{
	enum{ MAX_NAME = 32 };
	buf_t() {
		name = new char(MAX_NAME);
		date = new char(MAX_NAME);
	}
	buf_t& operator=( const data_t& that ) {
		assert( that.name && MAX_NAME > strlen(that.name) );
		assert( that.date && MAX_NAME > strlen(that.date) );
		strcpy( name, that.name );
		strcpy( date, that.date );
		type = that.type;
		ftype = that.ftype;
		return  *this;
	}
};

static const char drop_table[] = 	"if object_id('xsystypes') is not NULL "
									"	drop table xsystypes";
static const char create_table[] = 	"CREATE TABLE xsystypes "
									"	( name varchar(30)   NOT NULL "
									"	, type int           NOT NULL "
									"	, ftype float        NOT NULL "
									"	, date datetime      NOT NULL "
									"   ) ";

#ifdef _WIN32
const bool fwin32(true);
#else
const bool fwin32(false);
#endif

void
execute( DBPROCESS * dbproc ) 
{
	RETCODE erc = dbsqlexec(dbproc);
	assert( erc == SUCCEED );
	
	while( (erc = dbresults(dbproc)) != NO_MORE_RESULTS ){
		assert( erc == SUCCEED );
		dbprhead(dbproc);
		while( (erc = dbnextrow(dbproc)) != NO_MORE_ROWS ) {
			assert( erc == SUCCEED );
			dbprrow(dbproc);
		}
	}
}

int main( int argc, char *argv[] ) 
{
	cout << "initialized DB-Library: " << dbinit() << endl;

	dberrhandle(ErrorHandler);
	dbmsghandle(MessageHandler);

	LOGINREC * login = dblogin();
	DBSETLSECURE (login);
	BCP_SETL (login, 1);
	
	if( argc < 3 || argc < 5 && !fwin32 ) {
		cerr << "syntax: " << argv[0] << " server database [user] [password]\n";
		return 1;
	}

	const char * server = argv[1];
	const char * dbname = argv[2];
	const char * username = argc > 3? argv[3] : NULL;
	const char * password = argc > 4? argv[4] : NULL;

	if( username ) {
		DBSETLUSER (login, username);
		if( password ) 
			DBSETLPWD (login, password);
	} else {
		DBSETLSECURE (login);
	}

	DBPROCESS *dbproc = dbopen( login, server );
	assert( dbproc );

	RETCODE erc = dbuse(dbproc, dbname);
	assert( erc == SUCCEED );

	/* 
	 * Create the table "xsystypes"
	 */
	erc = dbcmd(dbproc, drop_table );
	assert( erc == SUCCEED );
	execute(dbproc);

	erc = dbcmd(dbproc, "BEGIN TRANSACTION ");
	assert( erc == SUCCEED );
	erc = dbcmd(dbproc, create_table );
	assert( erc == SUCCEED );
	
	execute(dbproc);
	
	/* 
	 * Prepare to write from memory
	 */
	erc = bcp_init( dbproc, "xsystypes", NULL, NULL, DB_IN );
	assert( erc == SUCCEED );

	buf_t buf;

	LPCBYTE null_term = (BYTE*)"";

	/*                      addr              pre vlen term      tlen  type       col */
	erc = bcp_bind( dbproc, (LPCBYTE) buf.name,  0, -1, null_term,  1, SQLVARCHAR, 1 );
	assert( erc == SUCCEED );
	erc = bcp_bind( dbproc, (LPCBYTE)&buf.type,  0, -1,  0,         0, SQLINT2, 2 );
	assert( erc == SUCCEED );
	erc = bcp_bind( dbproc, (LPCBYTE)&buf.ftype, 0, -1,  0,         0, SQLFLT8, 3 );
	assert( erc == SUCCEED );
	erc = bcp_bind( dbproc, (LPCBYTE) buf.date,  0, -1, null_term,  1, SQLVARCHAR, 4 );
	assert( erc == SUCCEED );

	/* 
	 * Send the data.
	 */
	for( int i=0; i < sizeof(data)/sizeof(data[0]); i++ ) {
		buf = data[i];
		erc = bcp_sendrow( dbproc );
		assert( erc == SUCCEED );
	}

	cout << bcp_done(dbproc) << " rows sent" << endl;

	erc = dbcmd(dbproc, "COMMIT TRANSACTION ");
	assert( erc == SUCCEED );
	execute(dbproc);
	
	/* 
	 * Prepare to extract the data to the file "xsystypes.dat".
	 */
	char datafilename[] = "xsystypes.dat";
	erc = bcp_init( dbproc, "xsystypes", datafilename, "xsystypes.err", DB_OUT );
	assert( erc == SUCCEED );
	
	DBINT nrows;
	erc = bcp_exec( dbproc, &nrows );
	assert( erc == SUCCEED );
	
	cout << nrows << " rows extracted to " << datafilename << "\n";

	erc = dbcmd(dbproc, "DROP TABLE xsystypes ");
	assert( erc == SUCCEED );
	execute(dbproc);
	
	/*
	 * Print the data file.
	 */ 
	ifstream datafile(datafilename);
	assert(datafile.is_open());
	for( int i=0; i < sizeof(data)/sizeof(data[0]); i++ ) {
		counted_string name;
		DBDATETIME date;
		number<int> type;
		number<double> ftype;
		
		datafile >> name >> type >> ftype >> date;
		cout << left << setw(30) << name << '\t' << type << '\t' << ftype << '\t' << date << endl;
	}
	
	return 0;
}

	

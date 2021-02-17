/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010 Oracle.  All rights reserved.
 *
 */


/*
** This file defines the sqlite bfile extension for Berkeley DB.
*/

#include <sqlite3.h>
#include <sys/types.h>
 

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


typedef struct sqlite3_bfile sqlite3_bfile;

/*
 * Access BFILE element of the current row in the row set
 */
SQLITE_API int sqlite3_column_bfile(
	sqlite3_stmt *pStmt,
	int iCol,
	sqlite3_bfile **ppBfile
);

/*
 * Open a BFILE
 */
SQLITE_API int sqlite3_bfile_open(
	sqlite3_bfile *pBfile
);

/*
 * Close a opened BFILE
 */
SQLITE_API int sqlite3_bfile_close(
	sqlite3_bfile *pBfile
);

/*
 * Check if file is open using this BFLIE
 */
SQLITE_API int sqlite3_bfile_is_open(
	sqlite3_bfile *pBfile,
	int *open
);

/*
 * Read a portion of a BFILE
 */
SQLITE_API int sqlite3_bfile_read(
	sqlite3_bfile *pBfile,
	void *z,
	int nSize,
	off_t iOffset,
	int *nRead
);

/*
 * Check if a file exists
 */
SQLITE_API int sqlite3_bfile_file_exists(
	sqlite3_bfile *pBfile,
	int *exist
);

/*
 * Get File size of a BFILE
 */
SQLITE_API int sqlite3_bfile_size(
	sqlite3_bfile *pBfile,
	off_t *size
);

/*
 * Finalize a BFILE
 */
SQLITE_API int sqlite3_bfile_final(
	sqlite3_bfile *pBfile
);

#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */

		

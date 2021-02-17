/**
 * @file xpath.c
 * SQLite extension module to select parts of XML documents
 * using libxml2 XPath and SQLite's virtual table mechanism.
 *
 * 2013 March 15
 *
 * The author disclaims copyright to this source code.
 * In place of a legal notice, here is a blessing:
 *
 *    May you do good and not evil.
 *    May you find forgiveness for yourself and forgive others.
 *    May you share freely, never taking more than you give.
 *
 ********************************************************************
 */

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#ifdef WITH_XSLT
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#endif

#ifdef STANDALONE
#include <sqlite3.h>
#else
#include <sqlite3ext.h>
static SQLITE_EXTENSION_INIT1
#endif

/**
 * @typedef XDOC
 * @struct XDOC
 * Structure to cache XML document.
 */

typedef struct XDOC {
    xmlDocPtr doc;		/**< XML document. */
    int refcnt;			/**< Reference counter. */
} XDOC;

/**
 * @typedef XMOD
 * @struct XMOD
 * Structure holding per module/database data.
 */

typedef struct XMOD {
    int refcnt;			/**< Reference counter. */
    sqlite3_mutex *mutex;	/**< DOC table mutex. */
    int sdoc;			/**< Size of docs array. */
    int ndoc;			/**< Number of used entries in docs array. */
    XDOC *docs;			/**< Array of modules's DOCs. */
} XMOD;

static int initialized = 0;
static XMOD *xmod = 0;

/**
 * @typedef XTAB
 * @struct XTAB
 * Structure to describe virtual table.
 */

typedef struct XTAB {
    sqlite3_vtab vtab;  /**< SQLite virtual table. */
    sqlite3 *db;        /**< Open database. */
    XMOD *xm;		/**< Module data. */
    struct XCSR *xc;	/**< Current cursor. */
    int sdoc;		/**< Size of idocs array. */
    int ndoc;		/**< Number of used entries in idocs array. */
    int *idocs;		/**< Indexes in module-wide DOC table. */
} XTAB;

/**
 * @typedef XEXP
 * @struct XEXP
 * Structure to describe XPath expression.
 */

typedef struct XEXP {
    struct XEXP *next;		/**< Next item. */
    struct XEXP *prev;		/**< Previous item. */
    xmlDocPtr doc;		/**< Current XML document. */
    xmlXPathContextPtr pctx;	/**< Current XPath context. */
    xmlXPathObjectPtr pobj;	/**< Current XPath objects. */
    xmlNodePtr parent;		/**< Current parent node or NULL. */
    int pos;			/**< Position within XPath expr. */
    int conv;			/**< Conversion: string/boolean/number. */
    char expr[1];		/**< XPath expression text. */
} XEXP;

/**
 * @typedef XCSR
 * @struct XCSR
 * Structure to describe virtual table cursor.
 */

typedef struct XCSR {
    sqlite3_vtab_cursor cursor;         /**< SQLite virtual table cursor */
    int pos;                            /**< Current index. */
    int nexpr;				/**< Number of XPath expr. */
    XEXP *first;			/**< First XPath expr. */
    XEXP *last;				/**< Last XPath expr. */
} XCSR;

/**
 * Connect to virtual table.
 * @param db SQLite database pointer
 * @param aux user specific pointer
 * @param argc argument count
 * @param argv argument vector
 * @param vtabp pointer receiving virtual table pointer
 * @param errp pointer receiving error messag
 * @result SQLite error code
 *
 * Argument vector contains:
 *
 * argv[0] - module name<br>
 * argv[1] - database name<br>
 * argv[2] - table name (virtual table)<br>
 */

static int
xpath_connect(sqlite3* db, void *aux, int argc, const char * const *argv,
	     sqlite3_vtab **vtabp, char **errp)
{
    int rc = SQLITE_ERROR;
    XTAB *xt;

    xt = sqlite3_malloc(sizeof (XTAB));
    if (!xt) {
nomem:
	*errp = sqlite3_mprintf("out of memory");
	return rc;
    }
    memset(xt, 0, sizeof (XTAB));
    xt->db = db;
    xt->xm = (XMOD *) aux;
    xt->xc = 0;
    xt->sdoc = 128;
    xt->ndoc = 0;
    xt->idocs = sqlite3_malloc(xt->sdoc * sizeof (int));
    if (!xt->idocs) {
	sqlite3_free(xt);
	goto nomem;
    }
    rc = sqlite3_declare_vtab(db,
			      "CREATE TABLE x("
			      " DOCID INTEGER PRIMARY KEY,"
			      " XML HIDDEN BLOB,"
			      " PATH HIDDEN TEXT,"
			      " OPTIONS HIDDEN INTEGER,"
			      " ENCODING HIDDEN TEXT,"
			      " BASEURL HIDDEN TEXT,"
			      " XMLDUMP HIDDEN TEXT"
			      ")");
    if (rc != SQLITE_OK) {
	sqlite3_free(xt->idocs);
	sqlite3_free(xt);
	*errp = sqlite3_mprintf("table definition failed (error %d)", rc);
	return rc;
    }
    *vtabp = &xt->vtab;
    *errp = 0;
    return SQLITE_OK;
}

/**
 * Create virtual table.
 * @param db SQLite database pointer
 * @param aux user specific pointer
 * @param argc argument count
 * @param argv argument vector
 * @param vtabp pointer receiving virtual table pointer
 * @param errp pointer receiving error messag
 * @result SQLite error code
 */

static int
xpath_create(sqlite3* db, void *aux, int argc,
	     const char *const *argv,
	     sqlite3_vtab **vtabp, char **errp)
{
    return xpath_connect(db, aux, argc, argv, vtabp, errp);
}

/**
 * Disconnect virtual table.
 * @param vtab virtual table pointer
 * @result always SQLITE_OK
 */

static int
xpath_disconnect(sqlite3_vtab *vtab)
{
    XTAB *xt = (XTAB *) vtab;
    XMOD *xm = xt->xm;
    int i, n;

    if (xm->mutex) {
	sqlite3_mutex_enter(xm->mutex);
	for (i = 0; xm->docs && (i < xt->ndoc); i++) {
	    n = xt->idocs[i];
	    if ((n >= 0) && (n < xm->sdoc)) {
		xmlDocPtr doc = xm->docs[n].doc;
		if (doc) {
		    xm->docs[n].refcnt -= 1;
		    if (xm->docs[n].refcnt <= 0) {
			xm->docs[n].doc = 0;
			xm->docs[n].refcnt = 0;
			xm->ndoc--;
			xmlFreeDoc(doc);
		    }
		}
	    }
	}
	sqlite3_mutex_leave(xm->mutex);
    }
    sqlite3_free(xt->idocs);
    sqlite3_free(xt);
    return SQLITE_OK;
}

/**
 * Destroy virtual table.
 * @param vtab virtual table pointer
 * @result always SQLITE_OK
 */

static int
xpath_destroy(sqlite3_vtab *vtab)
{
    return xpath_disconnect(vtab);
}

/**
 * Determines information for filter function according to constraints.
 * @param vtab virtual table
 * @param info index/constraint information
 * @result SQLite error code
 */

static int
xpath_bestindex(sqlite3_vtab *vtab, sqlite3_index_info *info)
{
    return SQLITE_OK;
}

/**
 * Open virtual table and return cursor.
 * @param vtab virtual table pointer
 * @param cursorp pointer receiving cursor pointer
 * @result SQLite error code
 */

static int
xpath_open(sqlite3_vtab *vtab, sqlite3_vtab_cursor **cursorp)
{
    XCSR *xc = sqlite3_malloc(sizeof (XCSR));

    if (!xc) {
	return SQLITE_ERROR;
    }
    xc->cursor.pVtab = vtab;
    xc->pos = -1;
    xc->nexpr = 0;
    xc->first = xc->last = 0;
    *cursorp = &xc->cursor;
    return SQLITE_OK;
}

/**
 * Close virtual table cursor.
 * @param cursor cursor pointer
 * @result SQLite error code
 */

static int
xpath_close(sqlite3_vtab_cursor *cursor)
{
    XCSR *xc = (XCSR *) cursor;
    XEXP *xp = xc->first, *next;
    XTAB *xt = (XTAB *) xc->cursor.pVtab;

    while (xp) {
	next = xp->next;
	if (xp->pobj) {
	    xmlXPathFreeObject(xp->pobj);
	}
	if (xp->pctx) {
	    xmlXPathFreeContext(xp->pctx);
	}
	sqlite3_free(xp);
	xp = next;
    }
    if (xt->xc == xc) {
	xt->xc = 0;
    }
    sqlite3_free(xc);
    return SQLITE_OK;
}

/**
 * Retrieve next row from virtual table cursor.
 * @param cursor virtual table cursor
 * @result SQLite error code
 */

static int
xpath_next(sqlite3_vtab_cursor *cursor)
{
    XCSR *xc = (XCSR *) cursor;
    XTAB *xt = (XTAB *) xc->cursor.pVtab;
    XEXP *xp;

    if (xc->pos < xt->ndoc) {
	int ninc = 0;

	if ((xc->pos >= 0) && xc->nexpr) {
	    int newpos;
	    xmlNodePtr node, parent = 0;

	    xp = xc->first;
	    while (xp) {
		if (xp->pobj) {
		    if (xp == xc->first) {
			parent = xp->parent;
		    } else if (parent != xp->parent) {
			break;
		    }
		}
		xp = xp->next;
	    }
	    if (parent && !xp) {
		int pchg = 0;

		xp = xc->first;
		while (xp) {
		    if (xp->pobj && (xp->pobj->type == XPATH_NODESET) &&
			xp->pobj->nodesetval) {
			newpos = xp->pos + 1;
			if (newpos < xp->pobj->nodesetval->nodeNr) {
			    node = xp->pobj->nodesetval->nodeTab[newpos];
			    if (node->parent != xp->parent) {
				pchg++;
			    }
			} else {
			    pchg++;
			}
		    }
		    xp = xp->next;
		}
		if ((pchg != 0) && (pchg != xc->nexpr)) {
		    xp = xc->first;
		    while (xp) {
			if (xp->pobj && (xp->pobj->type == XPATH_NODESET) &&
			    xp->pobj->nodesetval) {
			    newpos = xp->pos + 1;
			    if (newpos < xp->pobj->nodesetval->nodeNr) {
				node = xp->pobj->nodesetval->nodeTab[newpos];
				if (node->parent == xp->parent) {
				    xp->pos = newpos;
				    ninc++;
				}
			    } else {
				xp->pos = xp->pobj->nodesetval->nodeNr;
				ninc++;
			    }
			}
			xp = xp->next;
		    }
		}
	    }
	    if (!ninc) {
		xp = xc->first;
		while (xp) {
		    if (xp->pobj && (xp->pobj->type == XPATH_NODESET) &&
			xp->pobj->nodesetval) {
			newpos = xp->pos + 1;
			if (newpos < xp->pobj->nodesetval->nodeNr) {
			    xp->pos = newpos;
			    ninc++;
			} else {
			    xp->pos = xp->pobj->nodesetval->nodeNr;
			}
		    }
		    xp = xp->next;
		}
	    }
	}
	if (!ninc) {
	    xc->pos++;
	    xp = xc->first;
	    while (xp) {
		xp->pos = -1;
		xp->parent = 0;
		xp = xp->next;
	    }
	}
    }
    return SQLITE_OK;
}

/**
 * Filter function for virtual table.
 * @param cursor virtual table cursor
 * @param idxNum not used
 * @param idxStr nod used
 * @param argc number arguments (not used)
 * @param argv argument (nothing or RHS of filter expression, not used)
 * @result SQLite error code
 */

static int
xpath_filter(sqlite3_vtab_cursor *cursor, int idxNum,
	     const char *idxStr, int argc, sqlite3_value **argv)
{
    XCSR *xc = (XCSR *) cursor;
    XTAB *xt = (XTAB *) xc->cursor.pVtab;

    xc->pos = -1;
    xt->xc = xc;
    return xpath_next(cursor);
}

/**
 * Return end of table state of virtual table cursor.
 * @param cursor virtual table cursor
 * @result true/false
 */

static int
xpath_eof(sqlite3_vtab_cursor *cursor)
{
    XCSR *xc = (XCSR *) cursor;
    XTAB *xt = (XTAB *) xc->cursor.pVtab;

    return xc->pos >= xt->ndoc;
}

/**
 * Return column data of virtual table.
 * @param cursor virtual table cursor
 * @param ctx SQLite function context
 * @param n column index
 * @result SQLite error code
 */ 

static int
xpath_column(sqlite3_vtab_cursor *cursor, sqlite3_context *ctx, int n)
{
    XCSR *xc = (XCSR *) cursor;
    XTAB *xt = (XTAB *) xc->cursor.pVtab;
    XMOD *xm = (XMOD *) xt->xm;

    if ((xc->pos < 0) || (xc->pos >= xt->ndoc)) {
	sqlite3_result_error(ctx, "column out of bounds", -1);
	return SQLITE_ERROR;
    }
    if (n == 0) {
	n = xt->idocs[xc->pos];
	if (xm->docs[n].doc) {
	    sqlite3_result_int(ctx, n + 1);
	    return SQLITE_OK;
	}
    } else if (n == 6) {
	n = xt->idocs[xc->pos];
	if (xm->docs[n].doc) {
	    xmlChar *dump = 0;
	    int dump_len = 0;

	    xmlDocDumpFormatMemoryEnc(xm->docs[n].doc, &dump,
				      &dump_len, "utf-8", 1);
	    if (dump) {
		sqlite3_result_text(ctx, (char *) dump, dump_len,
				    SQLITE_TRANSIENT);
		xmlFree(dump);
		return SQLITE_OK;
	    }
	}
    }
    sqlite3_result_null(ctx);
    return SQLITE_OK;
}

/**
 * Return current rowid of virtual table cursor.
 * @param cursor virtual table cursor
 * @param rowidp value buffer to receive current rowid
 * @result SQLite error code
 */

static int
xpath_rowid(sqlite3_vtab_cursor *cursor, sqlite3_int64 *rowidp)
{
    XCSR *xc = (XCSR *) cursor;
    XTAB *xt = (XTAB *) xc->cursor.pVtab;
    XMOD *xm = (XMOD *) xt->xm;
    int n = xt->idocs[xc->pos];

    if (xm->docs[n].doc) {
	*rowidp = (sqlite3_int64) (n + 1);
	return SQLITE_OK;
    }
    return SQLITE_ERROR;
}

/**
 * Insert/delete row into/from virtual table.
 * @param vtab virtual table pointer
 * @param argc number of arguments
 * @param argv argument vector
 * @param rowidp value buffer to receive rowid
 * @result SQLite error code
 *
 * Examples:
 *
 *   CREATE VIRTUAL TABLE X USING xpath();<br>
 *   INSERT INTO X(XML) VALUES('xml-string ...');<br>
 *   INSERT INTO X(PATH,OPTIONS) VALUES(&lt;url&gt;,0);<br>
 *   DELETE FROM X WHERE DOCID=&lt;docid&gt;;<br>
 *
 * Virtual table columns:
 *
 *   DOCID    - document identifier and ROWID<br>
 *   XML      - XML string<br>
 *   PATH     - pathname or URL<br>
 *   OPTIONS  - parser options, see libxml's XML_PARSE_* defines<br>
 *   ENCODING - optional document encoding, default UTF-8<br>
 *   BASEURL  - optional base URL when XML string given<br>
 *   XMLDUMP  - output column, XML dump of document tree<br>
 *
 * All columns except DOCID are hidden. UPDATE on the virtual table
 * is not supported. Default parser options are XML_PARSE_NOERROR,
 * XML_PARSE_NOWARNING, and XML_PARSE_NONET.
 */

static int
xpath_update(sqlite3_vtab *vtab, int argc, sqlite3_value **argv,
	     sqlite3_int64 *rowidp)
{
    int n = -1, rc = SQLITE_ERROR;
    XTAB *xt = (XTAB *) vtab;
    XMOD *xm = (XMOD *) xt->xm;
    xmlDocPtr doc = 0, docToFree = 0;

    if (argc == 1) {
	/* DELETE */
	int i, k = -1;

	n = sqlite3_value_int(argv[0]);
	for (i = 0; i < xt->ndoc; i++) {
	    if ((n - 1) == xt->idocs[i]) {
		k = xt->idocs[i];
		memmove(xt->idocs + i, xt->idocs + i + 1,
			(xt->ndoc - (i + 1)) * sizeof (int));
		xt->ndoc--;
		break;
	    }
	}
	if ((k >= 0) && xm->mutex) {
	    n = k;
	    doc = xm->docs[n].doc;
	}
	rc = SQLITE_OK;
    } else if ((argc > 1) && (sqlite3_value_type(argv[0]) == SQLITE_NULL)) {
	/* INSERT */
	int i, docid;
	int opts = (XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NONET);
	char *enc = 0;

	if (sqlite3_value_type(argv[1]) != SQLITE_NULL) {
	    if (vtab->zErrMsg) {
		sqlite3_free(vtab->zErrMsg);
	    }
	    vtab->zErrMsg = sqlite3_mprintf("ROWID must be NULL");
	    rc = SQLITE_CONSTRAINT;
	    goto done;
	}
	if (sqlite3_value_type(argv[2]) != SQLITE_NULL) {
	    docid = sqlite3_value_int(argv[2]);
	    if ((sqlite3_value_type(argv[3]) != SQLITE_NULL) ||
		(sqlite3_value_type(argv[4]) != SQLITE_NULL)) {
		if (vtab->zErrMsg) {
		    sqlite3_free(vtab->zErrMsg);
		}
		vtab->zErrMsg = sqlite3_mprintf("XML and PATH must be NULL");
		rc = SQLITE_CONSTRAINT;
		goto done;
	    }
	    sqlite3_mutex_enter(xm->mutex);
	    for (i = 0; xm->docs && (i < xt->ndoc); i++) {
		if ((docid - 1) == xt->idocs[i]) {
		    sqlite3_mutex_leave(xm->mutex);
		    if (vtab->zErrMsg) {
			sqlite3_free(vtab->zErrMsg);
		    }
		    vtab->zErrMsg = sqlite3_mprintf("constraint violation");
		    rc = SQLITE_CONSTRAINT;
		    goto done;
		}
	    }
	    if ((docid > 0) && (docid <= xm->sdoc)) {
		doc = xm->docs[docid - 1].doc;
		if (doc) {
		    xm->docs[docid - 1].refcnt++;
		}
	    }
	    sqlite3_mutex_leave(xm->mutex);
	    if (!doc) {
		if (vtab->zErrMsg) {
		    sqlite3_free(vtab->zErrMsg);
		}
		vtab->zErrMsg = sqlite3_mprintf("invalid DOCID");
		goto done;
	    }
	    goto havedoc;
	}
	if (((sqlite3_value_type(argv[3]) == SQLITE_NULL) &&
	     (sqlite3_value_type(argv[4]) == SQLITE_NULL)) ||
	    ((sqlite3_value_type(argv[3]) != SQLITE_NULL) &&
	     (sqlite3_value_type(argv[4]) != SQLITE_NULL))) {
	    if (vtab->zErrMsg) {
		sqlite3_free(vtab->zErrMsg);
	    }
	    vtab->zErrMsg = sqlite3_mprintf("specify one of XML or PATH");
	    rc = SQLITE_CONSTRAINT;
	    goto done;
	}
	if (sqlite3_value_type(argv[5]) != SQLITE_NULL) {
	    opts = sqlite3_value_int(argv[5]);
	}
	if (sqlite3_value_type(argv[6]) != SQLITE_NULL) {
	    enc = (char *) sqlite3_value_text(argv[6]);
	}
	if (sqlite3_value_type(argv[4]) != SQLITE_NULL) {
	    doc = xmlReadFile((char *) sqlite3_value_text(argv[4]), enc, opts);
	} else {
	    char *url = 0;

	    if (sqlite3_value_type(argv[7]) != SQLITE_NULL) {
		url = (char *) sqlite3_value_text(argv[7]);
	    }
	    doc = xmlReadMemory(sqlite3_value_blob(argv[3]),
				sqlite3_value_bytes(argv[3]),
				url ? url : "", enc, opts);
	}
	if (!doc) {
	    if (vtab->zErrMsg) {
		sqlite3_free(vtab->zErrMsg);
	    }
	    vtab->zErrMsg = sqlite3_mprintf("read error");
	    goto done;
	}
	docToFree = doc;
havedoc:
	if (xt->ndoc >= xt->sdoc) {
	    int *idocs = sqlite3_realloc(xt->idocs, xt->sdoc +
					 128 * sizeof (int));

	    if (!idocs) {
		goto nomem;
	    }
	    xt->idocs = idocs;
	    xt->sdoc += 128;
	}
	if (!xm->mutex) {
	    goto nomem;
	}
	sqlite3_mutex_enter(xm->mutex);
	if (xm->ndoc >= xt->sdoc) {
	    XDOC *docs = sqlite3_realloc(xm->docs, xt->sdoc +
					 128 * sizeof (XDOC));

	    if (!docs) {
		sqlite3_mutex_leave(xm->mutex);
		goto nomem;
	    }
	    xm->docs = docs;
	    docs += xt->sdoc;
	    memset(docs, 0, 128 * sizeof (XDOC));
	    xt->sdoc += 128;
	}
	for (i = 0; i < xm->sdoc; i++) {
	    if (!xm->docs[i].doc) {
		xm->docs[i].doc = doc;
		xm->docs[i].refcnt = 1;
		xm->ndoc++;
		xt->idocs[xt->ndoc++] = i;
		*rowidp = (sqlite3_int64) (i + 1);
		doc = docToFree = 0;
		rc = SQLITE_OK;
		break;
	    }
	}
    } else {
	/* UPDATE */
	if (vtab->zErrMsg) {
	    sqlite3_free(vtab->zErrMsg);
	}
	vtab->zErrMsg = sqlite3_mprintf("UPDATE not supported");
    }
done:
    if (docToFree) {
	xmlFreeDoc(docToFree);
    } else if (doc && (n >= 0)) {
	sqlite3_mutex_enter(xm->mutex);
	xm->docs[n].refcnt -= 1;
	if (xm->docs[n].refcnt <= 0) {
	    xm->docs[n].doc = 0;
	    xm->docs[n].refcnt = 0;
	    xm->ndoc--;
	    xmlFreeDoc(doc);
	}
	sqlite3_mutex_leave(xm->mutex);
    }
    return rc;
nomem:
    if (vtab->zErrMsg) {
	sqlite3_free(vtab->zErrMsg);
    }
    vtab->zErrMsg = sqlite3_mprintf("out of memory");
    rc = SQLITE_NOMEM;
    goto done;
}

/**
 * Common XPath select function for virtual table.
 * @param ctx SQLite function context
 * @param conv conversion (0=string, 1=boolean, 2=number)
 * @param argc number of arguments
 * @param argv argument vector
 *
 * Examples:
 *
 *   CREATE VIRTUAL TABLE X USING xpath();<br>
 *   INSERT INTO X(XML) VALUES('xml-string ...');<br>
 *   SELECT xpath_string(docid, '//book/title') FROM X;<br>
 *   SELECT xpath_number(docid, '//book/price') FROM X;<br>
 *
 * The RHS of the xpath_* functions should be a constant string.
 */

static void 
xpath_vfunc_common(sqlite3_context *ctx, int conv, int argc,
		   sqlite3_value **argv)
{
    XTAB *xt = (XTAB *) sqlite3_user_data(ctx);
    XMOD *xm = xt->xm;
    XCSR *xc = xt->xc;
    XEXP *xp;
    xmlXPathContextPtr pctx = 0;
    xmlXPathObjectPtr pobj = 0;
    int n;
    char *p;

    if ((argc < 2) || !sqlite3_value_text(argv[1])) {
	sqlite3_result_error(ctx, "wrong arguments", -1);
	goto done;
    }
    if (!xc) {
	sqlite3_result_error(ctx, "not in virtual table context", -1);
	goto done;
    }
    if ((xc->pos < 0) || (xc->pos >= xt->ndoc)) {
	sqlite3_result_error(ctx, "cursor out of bounds", -1);
	goto done;
    }
    n = xt->idocs[xc->pos];
    if (!xm->docs[n].doc) {
	sqlite3_result_error(ctx, "no docid", -1);
	goto done;
    }
    p = (char *) sqlite3_value_text(argv[1]);
    if (!p || !p[0]) {
	sqlite3_result_error(ctx, "no or empty XPath expression", -1);
	goto done;
    }
    xp = xc->first;
    while (xp) {
	if (!strcmp(p, xp->expr)) {
	    break;
	}
	xp = xp->next;
    }
    if (!xp) {
	xp = sqlite3_malloc(sizeof (XEXP) + strlen(p));
	if (!xp) {
	    sqlite3_result_error(ctx, "out of memory", -1);
	    goto done;
	}
	xp->next = xp->prev = 0;
	strcpy(xp->expr, p);
	pctx = xmlXPathNewContext(xm->docs[n].doc);
	if (!pctx) {
	    sqlite3_free(xp);
	    sqlite3_result_error(ctx, "out of memory", -1);
	    goto done;
	}
	pobj = xmlXPathEvalExpression((xmlChar *) xp->expr, pctx);
	if (!pobj) {
	    sqlite3_free(xp);
	    sqlite3_result_error(ctx, "bad XPath expression", -1);
	    goto done;
	}
	xp->doc = xm->docs[n].doc;
	xp->pctx = pctx;
	xp->pobj = pobj;
	xp->parent = 0;
	xp->pos = -1;
	xp->conv = conv;
	pctx = 0;
	pobj = 0;
	xc->nexpr++;
	if (xc->first) {
	    xc->last->next = xp;
	    xp->prev = xc->last;
	    xc->last = xp;
	} else {
	    xc->first = xc->last = xp;
	}
    } else if (xm->docs[n].doc != xp->doc) {
	if (xp->pobj) {
	    xmlXPathFreeObject(xp->pobj);
	    xp->pobj = 0;
	}
	if (xp->pctx) {
	    xmlXPathFreeContext(xp->pctx);
	    xp->pctx = 0;
	}
	xp->doc = xm->docs[n].doc;
	xp->parent = 0;
	xp->pos = -1;
	if (xp->doc) {
	    pctx = xmlXPathNewContext(xm->docs[n].doc);
	    if (!pctx) {
		sqlite3_result_error(ctx, "out of memory", -1);
		goto done;
	    }
	    pobj = xmlXPathEvalExpression((xmlChar *) xp->expr, pctx);
	    if (!pobj) {
		sqlite3_result_error(ctx, "bad XPath expression", -1);
		goto done;
	    }
	    xp->pctx = pctx;
	    xp->pobj = pobj;
	    pctx = 0;
	    pobj = 0;
	}
    }
    if (xp->pos < 0) {
	xp->pos = 0;
    }
    if (!xp->pobj) {
	xp->parent = 0;
	sqlite3_result_null(ctx);
	goto done;
    }
    if ((xp->pobj->type == XPATH_NODESET) && xp->pobj->nodesetval) {
	if ((xp->pos < 0) || (xp->pos >= xp->pobj->nodesetval->nodeNr)) {
	    xp->parent = 0;
	    sqlite3_result_null(ctx);
	} else {
	    xmlNodePtr node = xp->pobj->nodesetval->nodeTab[xp->pos];
	    xmlBufferPtr buf = 0;

	    xp->parent = node->parent;
	    if (node) {
		switch (xp->conv) {
		case 1:
		    p = (char *) xmlXPathCastNodeToString(node);
		    n = xmlXPathCastStringToBoolean((xmlChar *) p);
		    sqlite3_result_int(ctx, n);
		    if (p) {
			xmlFree(p);
		    }
		    break;
		case 2:
		    sqlite3_result_double(ctx,
					  xmlXPathCastNodeToNumber(node));
		    break;
		case 3:
		    buf = xmlBufferCreate();
		    if (!buf) {
			sqlite3_result_error(ctx, "out of memory", -1);
			goto done;
		    }
		    xmlNodeDump(buf, xp->doc, node, 0, 0);
		    sqlite3_result_text(ctx, (char *) xmlBufferContent(buf),
					xmlBufferLength(buf),
					SQLITE_TRANSIENT);
		    xmlBufferFree(buf);
		    break;
		default:
		    p = (char *) xmlXPathCastNodeToString(node);
		    sqlite3_result_text(ctx, p, -1, SQLITE_TRANSIENT);
		    if (p) {
			xmlFree(p);
		    }
		    break;
		}
	    } else {
		sqlite3_result_null(ctx);
	    }
	}
    } else {
	xp->parent = 0;
	switch (xp->conv) {
	case 1:
	    sqlite3_result_int(ctx, xmlXPathCastToBoolean(xp->pobj));
	    break;
	case 2:
	    sqlite3_result_double(ctx, xmlXPathCastToNumber(xp->pobj));
	    break;
	default:
	    p = (char *) xmlXPathCastToString(xp->pobj);
	    sqlite3_result_text(ctx, p, -1, SQLITE_TRANSIENT);
	    if (p) {
		xmlFree(p);
	    }
	    break;
	}
    }
done:
    if (pobj) {
	xmlXPathFreeObject(pobj);
    }
    if (pctx) {
	xmlXPathFreeContext(pctx);
    }
}

/**
 * XPath select function returning string value from virtual table.
 * @param ctx SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 */

static void 
xpath_vfunc_string(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
   xpath_vfunc_common(ctx, 0, argc, argv);
}

/**
 * XPath select function returning boolean value from virtual table.
 * @param ctx SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 */

static void 
xpath_vfunc_boolean(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    xpath_vfunc_common(ctx, 1, argc, argv);
}

/**
 * XPath select function returning number from virtual table.
 * @param ctx SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 */

static void 
xpath_vfunc_number(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    xpath_vfunc_common(ctx, 2, argc, argv);
}

/**
 * XPath select function returning XML from virtual table.
 * @param ctx SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 */

static void 
xpath_vfunc_xml(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    xpath_vfunc_common(ctx, 3, argc, argv);
}

/**
 * Find overloaded function on virtual table.
 * @param vtab virtual table
 * @param nargs number arguments
 * @param name function name
 * @param pfunc pointer to function (value return)
 * @param parg pointer to function's argument (value return)
 * @result 0 or 1
 */

static int
xpath_findfunc(sqlite3_vtab *vtab, int nargs, const char *name,
	       void (**pfunc)(sqlite3_context *, int, sqlite3_value **),
	       void **parg)
{
    if (nargs != 2) {
	return 0;
    }
    if (!strcmp(name, "xpath_string")) {
	*pfunc = xpath_vfunc_string;
	*parg = vtab;
	return 1;
    }
    if (!strcmp(name, "xpath_boolean")) {
	*pfunc = xpath_vfunc_boolean;
	*parg = vtab;
	return 1;
    }
    if (!strcmp(name, "xpath_number")) {
	*pfunc = xpath_vfunc_number;
	*parg = vtab;
	return 1;
    }
    if (!strcmp(name, "xpath_xml")) {
	*pfunc = xpath_vfunc_xml;
	*parg = vtab;
	return 1;
    }
    return 0;
}

#if (SQLITE_VERSION_NUMBER > 3004000)

/**
 * Rename virtual table.
 * @param newname new name for table
 * @result SQLite error code
 */

static int
xpath_rename(sqlite3_vtab *vtab, const char *newname)
{
    return SQLITE_OK;
}

#endif

/**
 * SQLite module descriptor.
 */

static sqlite3_module xpath_mod = {
    1,                  /* iVersion */
    xpath_create,       /* xCreate */
    xpath_connect,      /* xConnect */
    xpath_bestindex,    /* xBestIndex */
    xpath_disconnect,   /* xDisconnect */
    xpath_destroy,      /* xDestroy */
    xpath_open,         /* xOpen */
    xpath_close,        /* xClose */
    xpath_filter,       /* xFilter */
    xpath_next,         /* xNext */
    xpath_eof,          /* xEof */
    xpath_column,       /* xColumn */
    xpath_rowid,        /* xRowid */
    xpath_update,       /* xUpdate */
    0,                  /* xBegin */
    0,                  /* xSync */
    0,                  /* xCommit */
    0,                  /* xRollback */
    xpath_findfunc,     /* xFindFunction */
#if (SQLITE_VERSION_NUMBER > 3004000)
    xpath_rename,       /* xRename */
#endif
};

/**
 * Common XPath select function.
 * @param ctx SQLite function context
 * @param conv conversion (0=string, 1=boolean, 2=number)
 * @param argc number of arguments
 * @param argv argument vector
 *
 * Examples:
 *
 *   SELECT xpath_string(&lt;docid&gt;, '//book/title');<br>
 *   SELECT xpath_number(&lt;xml-string&gt;, '//book/price',
 *                       &lt;options;&gt, &lt;encoding&gt;,
 *                       &lt;base-url&gt;);<br>
 *
 * The &lt;docid&gt; argument is the DOCID value of a row
 * in a virtual table. Otherwise a string containing an
 * XML document is expected. The optional arguments are<br>
 * &lt;options&gt;  - parser options, see libxml's XML_PARSE_* defines<br>
 * &lt;encoding&gt; - encoding of the XML document<br>
 * &lt;base-url&gt; - base URL of the XML document<br>
 */

static void 
xpath_func_common(sqlite3_context *ctx, int conv,
		  int argc, sqlite3_value **argv)
{
    xmlDocPtr doc = 0, docToFree = 0;
    xmlXPathContextPtr pctx = 0;
    xmlXPathObjectPtr pobj = 0;
    XMOD *xm = (XMOD *) sqlite3_user_data(ctx);
    int index = 0;
    char *p;

    if (argc < 2) {
	sqlite3_result_null(ctx);
	goto done;
    }
    if (sqlite3_value_type(argv[0]) == SQLITE_INTEGER) {
	index = sqlite3_value_int(argv[0]);
	if (!xm->mutex) {
	    sqlite3_result_error(ctx, "init error", -1);
	    goto done;
	}
	sqlite3_mutex_enter(xm->mutex);
	if ((index <= 0) || (index > xm->sdoc) || !xm->docs[index - 1].doc) {
	    sqlite3_mutex_leave(xm->mutex);
	    sqlite3_result_error(ctx, "invalid DOCID", -1);
	    goto done;
	}
	doc = xm->docs[index - 1].doc;
	xm->docs[index - 1].refcnt += 1;
	sqlite3_mutex_leave(xm->mutex);
    } else {
	int opts = (XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NONET);
	char *enc = 0, *url = 0;

	p = (char *) sqlite3_value_blob(argv[0]);
	if (!p) {
	    sqlite3_result_null(ctx);
	    return;
	}
	if ((argc > 2) && (sqlite3_value_type(argv[2]) != SQLITE_NULL)) {
	    opts = sqlite3_value_int(argv[2]);
	}
	if ((argc > 3) && (sqlite3_value_type(argv[3]) != SQLITE_NULL)) {
	    enc = (char *) sqlite3_value_text(argv[3]);
	}
	if ((argc > 4) && (sqlite3_value_type(argv[4]) != SQLITE_NULL)) {
	    url = (char *) sqlite3_value_text(argv[4]);
	}
	doc = xmlReadMemory(p, sqlite3_value_bytes(argv[0]),
			    url ? url : "", enc, opts);
	docToFree = doc;
	if (!doc) {
	    sqlite3_result_error(ctx, "read error", -1);
	    goto done;
	}
    }
    p = (char *) sqlite3_value_text(argv[1]);
    if (!p) {
	sqlite3_result_null(ctx);
	goto done;
    }
    pctx = xmlXPathNewContext(doc);
    if (!pctx) {
	sqlite3_result_error(ctx, "out of memory", -1);
	goto done;
    }
    pobj = xmlXPathEvalExpression((xmlChar *) p, pctx);
    if (!pobj) {
	sqlite3_result_error(ctx, "bad XPath expression", -1);
	goto done;
    }
    switch (conv) {
    case 1:
	sqlite3_result_int(ctx, xmlXPathCastToBoolean(pobj));
	break;
    case 2:
	sqlite3_result_double(ctx, xmlXPathCastToNumber(pobj));
	break;
    case 3:
	if ((pobj->type == XPATH_NODESET) && pobj->nodesetval &&
	    (pobj->nodesetval->nodeNr)) {
	    xmlNodePtr node = pobj->nodesetval->nodeTab[0];
	    xmlBufferPtr buf = 0;

	    buf = xmlBufferCreate();
	    if (!buf) {
		sqlite3_result_error(ctx, "out of memory", -1);
		goto done;
	    }
	    xmlNodeDump(buf, doc, node, 0, 0);
	    sqlite3_result_text(ctx, (char *) xmlBufferContent(buf),
				xmlBufferLength(buf), SQLITE_TRANSIENT);
	    xmlBufferFree(buf);
	} else {
	    sqlite3_result_null(ctx);
	}
	break;
    default:
	p = (char *) xmlXPathCastToString(pobj);
	sqlite3_result_text(ctx, p, -1, SQLITE_TRANSIENT);
	if (p) {
	    xmlFree(p);
	}
	break;
    }
done:
    if (pobj) {
	xmlXPathFreeObject(pobj);
    }
    if (pctx) {
	xmlXPathFreeContext(pctx);
    }
    if (docToFree) {
	xmlFreeDoc(docToFree);
    } else if (doc) {
	if (xm->mutex) {
	    sqlite3_mutex_enter(xm->mutex);
	    if (xm->docs && index) {
		xm->docs[index - 1].refcnt -= 1;
		if (xm->docs[index - 1].refcnt <= 0) {
		    docToFree = doc;
		    xm->docs[index - 1].refcnt = 0;
		    xm->docs[index - 1].doc = 0;
		}
	    }
	    sqlite3_mutex_leave(xm->mutex);
	    if (docToFree) {
		xmlFreeDoc(docToFree);
	    }
	}
    }
}

/**
 * XPath select function returning string value.
 * @param ctx SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 */

static void 
xpath_func_string(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    xpath_func_common(ctx, 0, argc, argv);
}

/**
 * XPath select function returning boolean value.
 * @param ctx SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 */

static void 
xpath_func_boolean(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    xpath_func_common(ctx, 1, argc, argv);
}

/**
 * XPath select function returning number.
 * @param ctx SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 */

static void 
xpath_func_number(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    xpath_func_common(ctx, 2, argc, argv);
}

/**
 * XPath select function returning XML.
 * @param ctx SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 */

static void 
xpath_func_xml(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    xpath_func_common(ctx, 3, argc, argv);
}

/**
 * Function to dump XML document.
 * @param ctx SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 *
 * Examples:
 *
 *   SELECT xml_dump(&lt;docid&gt;, &lt;encoding&gt; &lt;fmt&gt;)<br>
 *
 * The &lt;docid&gt; argument is the DOCID value of a row
 * in a virtual table. The &lt;encoding&gt; argument is
 * the optional encoding (default UTF-8). The &lt;fmt&gt;
 * argument is the optional formatting flag for the
 * xmlDocDumpFormatMemoryEnc() libxml2 function.
 */

static void 
xpath_func_dump(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    XMOD *xm = (XMOD *) sqlite3_user_data(ctx);
    int index = 0, dump_len = 0, fmt = 1;
    xmlChar *dump = 0;
    char *enc = "utf-8";

    if (argc < 1) {
	sqlite3_result_null(ctx);
	return;
    }
    index = sqlite3_value_int(argv[0]);
    if (argc > 1) {
	enc = (char *) sqlite3_value_text(argv[1]);
	if (!enc) {
	    enc = "utf-8";
	}
    }
    if (argc > 2) {
	fmt = sqlite3_value_int(argv[2]);
    }
    if (!xm->mutex) {
	sqlite3_result_error(ctx, "init error", -1);
	return;
    }
    sqlite3_mutex_enter(xm->mutex);
    if ((index <= 0) || (index > xm->sdoc) || !xm->docs[index - 1].doc) {
	sqlite3_mutex_leave(xm->mutex);
	sqlite3_result_error(ctx, "invalid DOCID", -1);
	return;
    }
    xmlDocDumpFormatMemoryEnc(xm->docs[index - 1].doc, &dump, &dump_len,
			      enc, fmt);
    if (dump) {
	sqlite3_result_text(ctx, (char *) dump, dump_len, SQLITE_TRANSIENT);
	xmlFree(dump);
    }
    sqlite3_mutex_leave(xm->mutex);
}

#ifdef WITH_XSLT
/**
 * Function to transform XML document using XSLT stylesheet.
 * @param ctx SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 *
 * Examples:
 *
 *   SELECT xslt_transform(&lt;docid&gt;, &lt;stylesheet&gt;, ...)<br>
 *   SELECT xslt_transform(&lt;xml-string&gt;, &lt;stylesheet&gt;,
 *                         &lt;options;&gt, &lt;encoding&gt;,
 *                         &lt;base-url&gt;, ...);<br>
 *
 * The &lt;docid&gt; argument is the DOCID value of a row
 * in a virtual table. &lt;stylesheet&gt; is the stylesheet
 * to apply on that document. When the transformation succeeded,
 * the transformed document replaces the original document.<br>
 * In the second form an XML string is transformed with the
 * same parameters as in the xpath_string() SQLite function.<br>
 * The ellipsis optional arguments are the string parameters for
 * the XSLT transformation.
 */

static void 
xpath_func_transform(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    xmlDocPtr doc = 0, docToFree = 0, res = 0;
    xsltStylesheetPtr cur = 0;
    XMOD *xm = (XMOD *) sqlite3_user_data(ctx);
    int index = 0, nparams = 0, param0, i;
    char *p;
    const char **params = 0;

    if (argc < 2) {
	sqlite3_result_null(ctx);
	goto done;
    }
    if (sqlite3_value_type(argv[0]) == SQLITE_INTEGER) {
	index = sqlite3_value_int(argv[0]);
	if (!xm->mutex) {
	    sqlite3_result_error(ctx, "init error", -1);
	    goto done;
	}
	sqlite3_mutex_enter(xm->mutex);
	if ((index <= 0) || (index > xm->sdoc) || !xm->docs[index - 1].doc) {
	    sqlite3_mutex_leave(xm->mutex);
	    sqlite3_result_error(ctx, "invalid DOCID", -1);
	    goto done;
	}
	doc = xm->docs[index - 1].doc;
	xm->docs[index - 1].refcnt += 1;
	sqlite3_mutex_leave(xm->mutex);
	param0 = 2;
	nparams = argc - 2;
    } else {
	int opts = (XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NONET);
	char *enc = 0, *url = 0;

	p = (char *) sqlite3_value_blob(argv[0]);
	if (!p) {
	    sqlite3_result_null(ctx);
	    return;
	}
	if ((argc > 2) && (sqlite3_value_type(argv[2]) != SQLITE_NULL)) {
	    opts = sqlite3_value_int(argv[2]);
	}
	if ((argc > 3) && (sqlite3_value_type(argv[3]) != SQLITE_NULL)) {
	    enc = (char *) sqlite3_value_text(argv[3]);
	}
	if ((argc > 4) && (sqlite3_value_type(argv[4]) != SQLITE_NULL)) {
	    url = (char *) sqlite3_value_text(argv[4]);
	}
	doc = xmlReadMemory(p, sqlite3_value_bytes(argv[0]),
			    url ? url : "", enc, opts);
	docToFree = doc;
	if (!doc) {
	    sqlite3_result_error(ctx, "read error", -1);
	    goto done;
	}
	param0 = 5;
	nparams = argc - 5;
    }
    p = (char *) sqlite3_value_text(argv[1]);
    if (!p) {
	sqlite3_result_null(ctx);
	goto done;
    }
    cur = xsltParseStylesheetFile((xmlChar *) p);
    if (!cur) {
	sqlite3_result_error(ctx, "read error on stylesheet", -1);
	goto done;
    }
    if (nparams <= 0) {
	nparams = 1;
    } else {
	nparams++;
    }
    params = sqlite3_malloc(nparams * sizeof (char *));
    if (!params) {
	sqlite3_result_error(ctx, "out of memory", -1);
	goto done;
    }
    for (i = 0; i < (argc - param0); i++) {
	params[i] = (const char *) sqlite3_value_text(argv[i + param0]);
	if (!params[i]) {
	    params[i] = "";
	}
    }
    params[i] = 0;
    res = xsltApplyStylesheet(cur, doc, params);
    if (!res) {
	sqlite3_result_error(ctx, "transformation failed", -1);
	goto done;
    }
    if (docToFree) {
	xmlChar *str = 0;

	xmlFreeDoc(docToFree);
	docToFree = res;
	i = 0;
	xsltSaveResultToString(&str, &i, res, cur);
	if (str) {
	    sqlite3_result_text(ctx, (char *) str, i, SQLITE_TRANSIENT);
	    xmlFree(str);
	} else {
	    sqlite3_result_null(ctx);
	}
    }
done:
    if (params) {
	sqlite3_free(params);
    }
    if (cur) {
	xsltFreeStylesheet(cur);
    }
    if (docToFree) {
	xmlFreeDoc(docToFree);
    } else if (doc) {
	if (xm->mutex) {
	    sqlite3_mutex_enter(xm->mutex);
	    if (xm->docs && index) {
		docToFree = doc;
		xm->docs[index - 1].doc = 0;
		xmlFreeDoc(docToFree);
		docToFree = 0;
		xm->docs[index - 1].refcnt -= 1;
		xm->docs[index - 1].doc = res;
		if (xm->docs[index - 1].refcnt <= 0) {
		    docToFree = res;
		    xm->docs[index - 1].refcnt = 0;
		    xm->docs[index - 1].doc = 0;
		}
	    }
	    sqlite3_mutex_leave(xm->mutex);
	    if (docToFree) {
		xmlFreeDoc(docToFree);
	    }
	}
    }
}
#endif

/**
 * Module finalizer.
 * @param aux pointer to module data
 * @result SQLite error code
 */

static void
xpath_fini(void *aux)
{
    XMOD *xm = (XMOD *) aux;
    XDOC *docs;
    int i, n, cleanup = 0;
    sqlite3_mutex *mutex = sqlite3_mutex_alloc(SQLITE_MUTEX_STATIC_MASTER);

    if (!mutex) {
	return;
    }
    sqlite3_mutex_enter(mutex);
    if (initialized) {
	xm->refcnt--;
	if (xm->refcnt <= 0) {
	    xmod = 0;
	    initialized = 0;
	    cleanup = 1;
	}
    } else {
	cleanup = 1;
    }
    sqlite3_mutex_leave(mutex);
    if (cleanup) {
	sqlite3_mutex_enter(xm->mutex);
	mutex = xm->mutex;
	xm->mutex = 0;
	docs = xm->docs;
	n = xm->ndoc;
	xm->docs = 0;
	xm->sdoc = xm->ndoc = 0;
	sqlite3_mutex_leave(mutex);
	sqlite3_mutex_free(mutex);
	for (i = 0; i < n; i++) {
	    if (docs->refcnt <= 0) {
		xmlFreeDoc(docs->doc);
		docs->doc = 0;
	    }
	}
	sqlite3_free(docs);
	sqlite3_free(xm);
    }
}

/**
 * Module initializer creating SQLite module and functions.
 * @param db database pointer
 * @result SQLite error code
 */

#ifndef STANDALONE
static
#endif
int
xpath_init(sqlite3 *db)
{
    XMOD *xm;
    int rc;
    sqlite3_mutex *mutex = sqlite3_mutex_alloc(SQLITE_MUTEX_STATIC_MASTER);

    if (!mutex) {
	return SQLITE_NOMEM;
    }
    sqlite3_mutex_enter(mutex);
    if (!initialized) {
	xm = sqlite3_malloc(sizeof (XMOD));
	if (!xm) {
	    sqlite3_mutex_leave(mutex);
	    return SQLITE_NOMEM;
	}
	xm->refcnt = 1;
	xm->mutex = sqlite3_mutex_alloc(SQLITE_MUTEX_FAST);
	if (!xm->mutex) {
	    sqlite3_mutex_leave(mutex);
	    sqlite3_free(xm);
	    return SQLITE_NOMEM;
	}
	xm->sdoc = 128;
	xm->ndoc = 0;
	xm->docs = sqlite3_malloc(xm->sdoc * sizeof (XDOC));
	if (!xm->docs) {
	    sqlite3_mutex_leave(mutex);
	    sqlite3_mutex_free(xm->mutex);
	    sqlite3_free(xm);
	    return SQLITE_NOMEM;
	}
	memset(xm->docs, 0, xm->sdoc * sizeof (XDOC));
	xmod = xm;
	initialized = 1;
    } else {
	xm = xmod;
	xm->refcnt++;
    }
    sqlite3_mutex_leave(mutex);
    sqlite3_create_function(db, "xpath_string", -1, SQLITE_UTF8,
			    (void *) xm, xpath_func_string, 0, 0);
    sqlite3_create_function(db, "xpath_boolean", -1, SQLITE_UTF8,
			    (void *) xm, xpath_func_boolean, 0, 0);
    sqlite3_create_function(db, "xpath_number", -1, SQLITE_UTF8,
			    (void *) xm, xpath_func_number, 0, 0);
    sqlite3_create_function(db, "xpath_xml", -1, SQLITE_UTF8,
			    (void *) xm, xpath_func_xml, 0, 0);
    sqlite3_create_function(db, "xml_dump", -1, SQLITE_UTF8,
			    (void *) xm, xpath_func_dump, 0, 0);
#ifdef WITH_XSLT
    sqlite3_create_function(db, "xslt_transform", -1, SQLITE_UTF8,
			    (void *) xm, xpath_func_transform, 0, 0);
#endif
    rc = sqlite3_create_module_v2(db, "xpath", &xpath_mod,
				  (void *) xm, xpath_fini);
    if (rc != SQLITE_OK) {
	sqlite3_create_function(db, "xpath_string", -1, SQLITE_UTF8,
				(void *) xm, 0, 0, 0);
	sqlite3_create_function(db, "xpath_boolean", -1, SQLITE_UTF8,
				(void *) xm, 0, 0, 0);
	sqlite3_create_function(db, "xpath_number", -1, SQLITE_UTF8,
				(void *) xm, 0, 0, 0);
	sqlite3_create_function(db, "xpath_xml", -1, SQLITE_UTF8,
				(void *) xm, 0, 0, 0);
	sqlite3_create_function(db, "xml_dump", -1, SQLITE_UTF8,
				(void *) xm, 0, 0, 0);
#ifdef WITH_XSLT
	sqlite3_create_function(db, "xslt_transform", -1, SQLITE_UTF8,
				(void *) xm, 0, 0, 0);
#endif
	xpath_fini(xm);
    }
    return rc;
}

#ifndef STANDALONE

/**
 * Initializer for SQLite extension load mechanism.
 * @param db SQLite database pointer
 * @param errmsg pointer receiving error message
 * @param api SQLite API routines
 * @result SQLite error code
 */

int
sqlite3_extension_init(sqlite3 *db, char **errmsg, 
		       const sqlite3_api_routines *api)
{
    SQLITE_EXTENSION_INIT2(api);
    return xpath_init(db);
}

#endif

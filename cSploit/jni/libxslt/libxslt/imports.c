/*
 * imports.c: Implementation of the XSLT imports
 *
 * Reference:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#define IN_LIBXSLT
#include "libxslt.h"

#include <string.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_MATH_H
#include <math.h>
#endif
#ifdef HAVE_FLOAT_H
#include <float.h>
#endif
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#ifdef HAVE_NAN_H
#include <nan.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/hash.h>
#include <libxml/xmlerror.h>
#include <libxml/uri.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "preproc.h"
#include "imports.h"
#include "documents.h"
#include "security.h"
#include "pattern.h"


/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/
/**
 * xsltFixImportedCompSteps:
 * @master: the "master" stylesheet
 * @style: the stylesheet being imported by the master
 *
 * normalize the comp steps for the stylesheet being imported
 * by the master, together with any imports within that. 
 *
 */
static void xsltFixImportedCompSteps(xsltStylesheetPtr master, 
			xsltStylesheetPtr style) {
    xsltStylesheetPtr res;
    xmlHashScan(style->templatesHash,
	            (xmlHashScanner) xsltNormalizeCompSteps, master);
    master->extrasNr += style->extrasNr;
    for (res = style->imports; res != NULL; res = res->next) {
        xsltFixImportedCompSteps(master, res);
    }
}

/**
 * xsltParseStylesheetImport:
 * @style:  the XSLT stylesheet
 * @cur:  the import element
 *
 * parse an XSLT stylesheet import element
 *
 * Returns 0 in case of success -1 in case of failure.
 */

int
xsltParseStylesheetImport(xsltStylesheetPtr style, xmlNodePtr cur) {
    int ret = -1;
    xmlDocPtr import = NULL;
    xmlChar *base = NULL;
    xmlChar *uriRef = NULL;
    xmlChar *URI = NULL;
    xsltStylesheetPtr res;
    xsltSecurityPrefsPtr sec;

    if ((cur == NULL) || (style == NULL))
	return (ret);

    uriRef = xmlGetNsProp(cur, (const xmlChar *)"href", NULL);
    if (uriRef == NULL) {
	xsltTransformError(NULL, style, cur,
	    "xsl:import : missing href attribute\n");
	goto error;
    }

    base = xmlNodeGetBase(style->doc, cur);
    URI = xmlBuildURI(uriRef, base);
    if (URI == NULL) {
	xsltTransformError(NULL, style, cur,
	    "xsl:import : invalid URI reference %s\n", uriRef);
	goto error;
    }

    res = style;
    while (res != NULL) {
        if (res->doc == NULL)
	    break;
	if (xmlStrEqual(res->doc->URL, URI)) {
	    xsltTransformError(NULL, style, cur,
	       "xsl:import : recursion detected on imported URL %s\n", URI);
	    goto error;
	}
	res = res->parent;
    }

    /*
     * Security framework check
     */
    sec = xsltGetDefaultSecurityPrefs();
    if (sec != NULL) {
	int secres;

	secres = xsltCheckRead(sec, NULL, URI);
	if (secres == 0) {
	    xsltTransformError(NULL, NULL, NULL,
		 "xsl:import: read rights for %s denied\n",
			     URI);
	    goto error;
	}
    }

    import = xsltDocDefaultLoader(URI, style->dict, XSLT_PARSE_OPTIONS,
                                  (void *) style, XSLT_LOAD_STYLESHEET);
    if (import == NULL) {
	xsltTransformError(NULL, style, cur,
	    "xsl:import : unable to load %s\n", URI);
	goto error;
    }

    res = xsltParseStylesheetImportedDoc(import, style);
    if (res != NULL) {
	res->next = style->imports;
	style->imports = res;
	if (style->parent == NULL) {
	    xsltFixImportedCompSteps(style, res);
	}
	ret = 0;
    } else {
	xmlFreeDoc(import);
	}

error:
    if (uriRef != NULL)
	xmlFree(uriRef);
    if (base != NULL)
	xmlFree(base);
    if (URI != NULL)
	xmlFree(URI);

    return (ret);
}

/**
 * xsltParseStylesheetInclude:
 * @style:  the XSLT stylesheet
 * @cur:  the include node
 *
 * parse an XSLT stylesheet include element
 *
 * Returns 0 in case of success -1 in case of failure
 */

int
xsltParseStylesheetInclude(xsltStylesheetPtr style, xmlNodePtr cur) {
    int ret = -1;
    xmlDocPtr oldDoc;
    xmlChar *base = NULL;
    xmlChar *uriRef = NULL;
    xmlChar *URI = NULL;
    xsltStylesheetPtr result;
    xsltDocumentPtr include;
    xsltDocumentPtr docptr;
    int oldNopreproc;

    if ((cur == NULL) || (style == NULL))
	return (ret);

    uriRef = xmlGetNsProp(cur, (const xmlChar *)"href", NULL);
    if (uriRef == NULL) {
	xsltTransformError(NULL, style, cur,
	    "xsl:include : missing href attribute\n");
	goto error;
    }

    base = xmlNodeGetBase(style->doc, cur);
    URI = xmlBuildURI(uriRef, base);
    if (URI == NULL) {
	xsltTransformError(NULL, style, cur,
	    "xsl:include : invalid URI reference %s\n", uriRef);
	goto error;
    }

    /*
     * in order to detect recursion, we check all previously included
     * stylesheets.
     */
    docptr = style->includes;
    while (docptr != NULL) {
        if (xmlStrEqual(docptr->doc->URL, URI)) {
	    xsltTransformError(NULL, style, cur,
	        "xsl:include : recursion detected on included URL %s\n", URI);
	    goto error;
	}
	docptr = docptr->includes;
    }

    include = xsltLoadStyleDocument(style, URI);
    if (include == NULL) {
	xsltTransformError(NULL, style, cur,
	    "xsl:include : unable to load %s\n", URI);
	goto error;
    }
#ifdef XSLT_REFACTORED    
    if (IS_XSLT_ELEM_FAST(cur) && (cur->psvi != NULL)) {
	((xsltStyleItemIncludePtr) cur->psvi)->include = include;
    } else {
	xsltTransformError(NULL, style, cur,
	    "Internal error: (xsltParseStylesheetInclude) "
	    "The xsl:include element was not compiled.\n", URI);
	style->errors++;
    }
#endif
    oldDoc = style->doc;
    style->doc = include->doc;
    /* chain to stylesheet for recursion checking */
    include->includes = style->includes;
    style->includes = include;
    oldNopreproc = style->nopreproc;
    style->nopreproc = include->preproc;
    /*
    * TODO: This will change some values of the
    *  including stylesheet with every included module
    *  (e.g. excluded-result-prefixes)
    *  We need to strictly seperate such stylesheet-owned values.
    */
    result = xsltParseStylesheetProcess(style, include->doc);
    style->nopreproc = oldNopreproc;
    include->preproc = 1;
    style->includes = include->includes;
    style->doc = oldDoc;
    if (result == NULL) {
	ret = -1;
	goto error;
    }
    ret = 0;

error:
    if (uriRef != NULL)
	xmlFree(uriRef);
    if (base != NULL)
	xmlFree(base);
    if (URI != NULL)
	xmlFree(URI);

    return (ret);
}

/**
 * xsltNextImport:
 * @cur:  the current XSLT stylesheet
 *
 * Find the next stylesheet in import precedence.
 *
 * Returns the next stylesheet or NULL if it was the last one
 */

xsltStylesheetPtr
xsltNextImport(xsltStylesheetPtr cur) {
    if (cur == NULL)
	return(NULL);
    if (cur->imports != NULL)
	return(cur->imports);
    if (cur->next != NULL)
	return(cur->next) ;
    do {
	cur = cur->parent;
	if (cur == NULL) break;
	if (cur->next != NULL) return(cur->next);
    } while (cur != NULL);
    return(cur);
}

/**
 * xsltNeedElemSpaceHandling:
 * @ctxt:  an XSLT transformation context
 *
 * Checks whether that stylesheet requires white-space stripping
 *
 * Returns 1 if space should be stripped, 0 if not
 */

int
xsltNeedElemSpaceHandling(xsltTransformContextPtr ctxt) {
    xsltStylesheetPtr style;

    if (ctxt == NULL)
	return(0);
    style = ctxt->style;
    while (style != NULL) {
	if (style->stripSpaces != NULL)
	    return(1);
	style = xsltNextImport(style);
    }
    return(0);
}

/**
 * xsltFindElemSpaceHandling:
 * @ctxt:  an XSLT transformation context
 * @node:  an XML node
 *
 * Find strip-space or preserve-space informations for an element
 * respect the import precedence or the wildcards
 *
 * Returns 1 if space should be stripped, 0 if not, and 2 if everything
 *         should be CDTATA wrapped.
 */

int
xsltFindElemSpaceHandling(xsltTransformContextPtr ctxt, xmlNodePtr node) {
    xsltStylesheetPtr style;
    const xmlChar *val;

    if ((ctxt == NULL) || (node == NULL))
	return(0);
    style = ctxt->style;
    while (style != NULL) {
	if (node->ns != NULL) {
	    val = (const xmlChar *)
	      xmlHashLookup2(style->stripSpaces, node->name, node->ns->href);
            if (val == NULL) {
                val = (const xmlChar *)
                    xmlHashLookup2(style->stripSpaces, BAD_CAST "*",
                                   node->ns->href);
            }
	} else {
	    val = (const xmlChar *)
		  xmlHashLookup2(style->stripSpaces, node->name, NULL);
	}
	if (val != NULL) {
	    if (xmlStrEqual(val, (xmlChar *) "strip"))
		return(1);
	    if (xmlStrEqual(val, (xmlChar *) "preserve"))
		return(0);
	}
	if (style->stripAll == 1)
	    return(1);
	if (style->stripAll == -1)
	    return(0);

	style = xsltNextImport(style);
    }
    return(0);
}

/**
 * xsltFindTemplate:
 * @ctxt:  an XSLT transformation context
 * @name: the template name
 * @nameURI: the template name URI
 *
 * Finds the named template, apply import precedence rule.
 * REVISIT TODO: We'll change the nameURI fields of
 *  templates to be in the string dict, so if the
 *  specified @nameURI is in the same dict, then use pointer
 *  comparison. Check if this can be done in a sane way.
 *  Maybe this function is not needed internally at
 *  transformation-time if we hard-wire the called templates
 *  to the caller.
 *
 * Returns the xsltTemplatePtr or NULL if not found
 */
xsltTemplatePtr
xsltFindTemplate(xsltTransformContextPtr ctxt, const xmlChar *name,
	         const xmlChar *nameURI) {
    xsltTemplatePtr cur;
    xsltStylesheetPtr style;

    if ((ctxt == NULL) || (name == NULL))
	return(NULL);
    style = ctxt->style;
    while (style != NULL) {
	cur = style->templates;
	while (cur != NULL) {
	    if (xmlStrEqual(name, cur->name)) {
		if (((nameURI == NULL) && (cur->nameURI == NULL)) ||
		    ((nameURI != NULL) && (cur->nameURI != NULL) &&
		     (xmlStrEqual(nameURI, cur->nameURI)))) {
		    return(cur);
		}
	    }
	    cur = cur->next;
	}

	style = xsltNextImport(style);
    }
    return(NULL);
}


/*
 * extra.c: Implementation of non-standard features
 *
 * Reference:
 *   Michael Kay "XSLT Programmer's Reference" pp 637-643
 *   The node-set() extension function
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#define IN_LIBXSLT
#include "libxslt.h"

#include <string.h>
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/hash.h>
#include <libxml/xmlerror.h>
#include <libxml/parserInternals.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "extensions.h"
#include "variables.h"
#include "transform.h"
#include "extra.h"
#include "preproc.h"

#ifdef WITH_XSLT_DEBUG
#define WITH_XSLT_DEBUG_EXTRA
#endif

/************************************************************************
 * 									*
 * 		Handling of XSLT debugging				*
 * 									*
 ************************************************************************/

/**
 * xsltDebug:
 * @ctxt:  an XSLT processing context
 * @node:  The current node
 * @inst:  the instruction in the stylesheet
 * @comp:  precomputed informations
 *
 * Process an debug node
 */
void
xsltDebug(xsltTransformContextPtr ctxt, xmlNodePtr node ATTRIBUTE_UNUSED,
          xmlNodePtr inst ATTRIBUTE_UNUSED,
          xsltStylePreCompPtr comp ATTRIBUTE_UNUSED)
{
    int i, j;

    xsltGenericError(xsltGenericErrorContext, "Templates:\n");
    for (i = 0, j = ctxt->templNr - 1; ((i < 15) && (j >= 0)); i++, j--) {
        xsltGenericError(xsltGenericErrorContext, "#%d ", i);
        if (ctxt->templTab[j]->name != NULL)
            xsltGenericError(xsltGenericErrorContext, "name %s ",
                             ctxt->templTab[j]->name);
        if (ctxt->templTab[j]->match != NULL)
            xsltGenericError(xsltGenericErrorContext, "name %s ",
                             ctxt->templTab[j]->match);
        if (ctxt->templTab[j]->mode != NULL)
            xsltGenericError(xsltGenericErrorContext, "name %s ",
                             ctxt->templTab[j]->mode);
        xsltGenericError(xsltGenericErrorContext, "\n");
    }
    xsltGenericError(xsltGenericErrorContext, "Variables:\n");
    for (i = 0, j = ctxt->varsNr - 1; ((i < 15) && (j >= 0)); i++, j--) {
        xsltStackElemPtr cur;

        if (ctxt->varsTab[j] == NULL)
            continue;
        xsltGenericError(xsltGenericErrorContext, "#%d\n", i);
        cur = ctxt->varsTab[j];
        while (cur != NULL) {
            if (cur->comp == NULL) {
                xsltGenericError(xsltGenericErrorContext,
                                 "corrupted !!!\n");
            } else if (cur->comp->type == XSLT_FUNC_PARAM) {
                xsltGenericError(xsltGenericErrorContext, "param ");
            } else if (cur->comp->type == XSLT_FUNC_VARIABLE) {
                xsltGenericError(xsltGenericErrorContext, "var ");
            }
            if (cur->name != NULL)
                xsltGenericError(xsltGenericErrorContext, "%s ",
                                 cur->name);
            else
                xsltGenericError(xsltGenericErrorContext, "noname !!!!");
#ifdef LIBXML_DEBUG_ENABLED
            if (cur->value != NULL) {
                xmlXPathDebugDumpObject(stdout, cur->value, 1);
            } else {
                xsltGenericError(xsltGenericErrorContext, "NULL !!!!");
            }
#endif
            xsltGenericError(xsltGenericErrorContext, "\n");
            cur = cur->next;
        }

    }
}

/************************************************************************
 * 									*
 * 		Classic extensions as described by M. Kay		*
 * 									*
 ************************************************************************/

/**
 * xsltFunctionNodeSet:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the node-set() XSLT function
 *   node-set node-set(result-tree)
 *
 * This function is available in libxslt, saxon or xt namespace.
 */
void
xsltFunctionNodeSet(xmlXPathParserContextPtr ctxt, int nargs){
    if (nargs != 1) {
	xsltTransformError(xsltXPathGetTransformContext(ctxt), NULL, NULL,
		"node-set() : expects one result-tree arg\n");
	ctxt->error = XPATH_INVALID_ARITY;
	return;
    }
    if ((ctxt->value == NULL) ||
	((ctxt->value->type != XPATH_XSLT_TREE) &&
	 (ctxt->value->type != XPATH_NODESET))) {
	xsltTransformError(xsltXPathGetTransformContext(ctxt), NULL, NULL,
	    "node-set() invalid arg expecting a result tree\n");
	ctxt->error = XPATH_INVALID_TYPE;
	return;
    }
    if (ctxt->value->type == XPATH_XSLT_TREE) {
	ctxt->value->type = XPATH_NODESET;
    }
}


/*
 * Okay the following really seems unportable and since it's not
 * part of any standard I'm not too ashamed to do this
 */
#if defined(linux) || defined(__sun)
#if defined(HAVE_MKTIME) && defined(HAVE_LOCALTIME) && defined(HAVE_ASCTIME)
#define WITH_LOCALTIME

/**
 * xsltFunctionLocalTime:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the localTime XSLT function used by NORM
 *   string localTime(???)
 *
 * This function is available in Norm's extension namespace
 * Code (and comments) contributed by Norm
 */
static void
xsltFunctionLocalTime(xmlXPathParserContextPtr ctxt, int nargs) {
    xmlXPathObjectPtr obj;
    char *str;
    char digits[5];
    char result[29];
    long int field;
    time_t gmt, lmt;
    struct tm gmt_tm;
    struct tm *local_tm;
 
    if (nargs != 1) {
       xsltTransformError(xsltXPathGetTransformContext(ctxt), NULL, NULL,
                      "localTime() : invalid number of args %d\n", nargs);
       ctxt->error = XPATH_INVALID_ARITY;
       return;
    }
 
    obj = valuePop(ctxt);

    if (obj->type != XPATH_STRING) {
	obj = xmlXPathConvertString(obj);
    }
    if (obj == NULL) {
	valuePush(ctxt, xmlXPathNewString((const xmlChar *)""));
	return;
    }
    
    str = (char *) obj->stringval;

    /* str = "$Date$" */
    memset(digits, 0, sizeof(digits));
    strncpy(digits, str+7, 4);
    field = strtol(digits, NULL, 10);
    gmt_tm.tm_year = field - 1900;

    memset(digits, 0, sizeof(digits));
    strncpy(digits, str+12, 2);
    field = strtol(digits, NULL, 10);
    gmt_tm.tm_mon = field - 1;

    memset(digits, 0, sizeof(digits));
    strncpy(digits, str+15, 2);
    field = strtol(digits, NULL, 10);
    gmt_tm.tm_mday = field;

    memset(digits, 0, sizeof(digits));
    strncpy(digits, str+18, 2);
    field = strtol(digits, NULL, 10);
    gmt_tm.tm_hour = field;

    memset(digits, 0, sizeof(digits));
    strncpy(digits, str+21, 2);
    field = strtol(digits, NULL, 10);
    gmt_tm.tm_min = field;

    memset(digits, 0, sizeof(digits));
    strncpy(digits, str+24, 2);
    field = strtol(digits, NULL, 10);
    gmt_tm.tm_sec = field;

    /* Now turn gmt_tm into a time. */
    gmt = mktime(&gmt_tm);


    /*
     * FIXME: it's been too long since I did manual memory management.
     * (I swore never to do it again.) Does this introduce a memory leak?
     */
    local_tm = localtime(&gmt);

    /*
     * Calling localtime() has the side-effect of setting timezone.
     * After we know the timezone, we can adjust for it
     */
    lmt = gmt - timezone;

    /*
     * FIXME: it's been too long since I did manual memory management.
     * (I swore never to do it again.) Does this introduce a memory leak?
     */
    local_tm = localtime(&lmt);

    /*
     * Now convert local_tm back into a string. This doesn't introduce
     * a memory leak, so says asctime(3).
     */

    str = asctime(local_tm);           /* "Tue Jun 26 05:02:16 2001" */
                                       /*  0123456789 123456789 123 */

    memset(result, 0, sizeof(result)); /* "Thu, 26 Jun 2001" */
                                       /*  0123456789 12345 */

    strncpy(result, str, 20);
    strcpy(result+20, "???");          /* tzname doesn't work, fake it */
    strncpy(result+23, str+19, 5);

    /* Ok, now result contains the string I want to send back. */
    valuePush(ctxt, xmlXPathNewString((xmlChar *)result));
}
#endif
#endif /* linux or sun */


/**
 * xsltRegisterExtras:
 * @ctxt:  a XSLT process context
 *
 * Registers the built-in extensions. This function is deprecated, use
 * xsltRegisterAllExtras instead.
 */
void
xsltRegisterExtras(xsltTransformContextPtr ctxt ATTRIBUTE_UNUSED) {
    xsltRegisterAllExtras();
}

/**
 * xsltRegisterAllExtras:
 *
 * Registers the built-in extensions
 */
void
xsltRegisterAllExtras (void) {
    xsltRegisterExtModuleFunction((const xmlChar *) "node-set",
				  XSLT_LIBXSLT_NAMESPACE,
				  xsltFunctionNodeSet);
    xsltRegisterExtModuleFunction((const xmlChar *) "node-set",
				  XSLT_SAXON_NAMESPACE,
				  xsltFunctionNodeSet);
    xsltRegisterExtModuleFunction((const xmlChar *) "node-set",
				  XSLT_XT_NAMESPACE,
				  xsltFunctionNodeSet);
#ifdef WITH_LOCALTIME
    xsltRegisterExtModuleFunction((const xmlChar *) "localTime",
				  XSLT_NORM_SAXON_NAMESPACE,
				  xsltFunctionLocalTime);
#endif
    xsltRegisterExtModuleElement((const xmlChar *) "debug",
				 XSLT_LIBXSLT_NAMESPACE,
				 NULL,
				 (xsltTransformFunction) xsltDebug);
    xsltRegisterExtModuleElement((const xmlChar *) "output",
				 XSLT_SAXON_NAMESPACE,
				 xsltDocumentComp,
				 (xsltTransformFunction) xsltDocumentElem);
    xsltRegisterExtModuleElement((const xmlChar *) "write",
				 XSLT_XALAN_NAMESPACE,
				 xsltDocumentComp,
				 (xsltTransformFunction) xsltDocumentElem);
    xsltRegisterExtModuleElement((const xmlChar *) "document",
				 XSLT_XT_NAMESPACE,
				 xsltDocumentComp,
				 (xsltTransformFunction) xsltDocumentElem);
    xsltRegisterExtModuleElement((const xmlChar *) "document",
				 XSLT_NAMESPACE,
				 xsltDocumentComp,
				 (xsltTransformFunction) xsltDocumentElem);
}

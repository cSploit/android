/*
********************************************************************************
*
*   Copyright (C) 1996-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
********************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include "unicode/utrace.h"
#include "unicode/uclean.h"
#include "umutex.h"

/* NOTES:
   3/20/1999 srl - strncpy called w/o setting nulls at the end
 */

#define MAXTESTNAME 128
#define MAXTESTS  512
#define MAX_TEST_LOG 4096

struct TestNode
{
  void (*test)(void);
  struct TestNode* sibling;
  struct TestNode* child;
  char name[1]; /* This is dynamically allocated off the end with malloc. */
};


static const struct TestNode* currentTest;

typedef enum { RUNTESTS, SHOWTESTS } TestMode;
#define TEST_SEPARATOR '/'

#ifndef C_TEST_IMPL
#define C_TEST_IMPL
#endif

#include "unicode/ctest.h"

static char ERROR_LOG[MAX_TEST_LOG][MAXTESTNAME];

/* Local prototypes */
static TestNode* addTestNode( TestNode *root, const char *name );

static TestNode *createTestNode(const char* name, int32_t nameLen);

static int strncmp_nullcheck( const char* s1,
                  const char* s2,
                  int n );

static void getNextLevel( const char* name,
              int* nameLen,
              const char** nextName );

static void iterateTestsWithLevel( const TestNode *root, int len,
                   const TestNode** list,
                   TestMode mode);

static void help ( const char *argv0 );

/**
 * Do the work of logging an error. Doesn't increase the error count.
 *
 * @prefix optional prefix prepended to message, or NULL.
 * @param pattern printf style pattern
 * @param ap vprintf style arg list
 */
static void vlog_err(const char *prefix, const char *pattern, va_list ap);
static void vlog_verbose(const char *prefix, const char *pattern, va_list ap);

/* If we need to make the framework multi-thread safe
   we need to pass around the following vars
*/
static int ERRONEOUS_FUNCTION_COUNT = 0;
static int ERROR_COUNT = 0; /* Count of errors from all tests. */
static int DATA_ERROR_COUNT = 0; /* count of data related errors or warnings */
static int INDENT_LEVEL = 0;
int REPEAT_TESTS_INIT = 0; /* Was REPEAT_TESTS initialized? */
int REPEAT_TESTS = 1; /* Number of times to run the test */
int VERBOSITY = 0; /* be No-verbose by default */
int ERR_MSG =1; /* error messages will be displayed by default*/
int QUICK = 1;  /* Skip some of the slower tests? */
int WARN_ON_MISSING_DATA = 0; /* Reduce data errs to warnings? */
UTraceLevel ICU_TRACE = UTRACE_OFF;  /* ICU tracing level */
size_t MINIMUM_MEMORY_SIZE_FAILURE = (size_t)-1; /* Minimum library memory allocation window that will fail. */
size_t MAXIMUM_MEMORY_SIZE_FAILURE = (size_t)-1; /* Maximum library memory allocation window that will fail. */
int32_t ALLOCATION_COUNT = 0;
/*-------------------------------------------*/

/* strncmp that also makes sure there's a \0 at s2[0] */
static int strncmp_nullcheck( const char* s1,
               const char* s2,
               int n )
{
    if (((int)strlen(s2) >= n) && s2[n] != 0) {
        return 3; /* null check fails */
    }
    else {
        return strncmp ( s1, s2, n );
    }
}

static void getNextLevel( const char* name,
           int* nameLen,
           const char** nextName )
{
    /* Get the next component of the name */
    *nextName = strchr(name, TEST_SEPARATOR);

    if( *nextName != 0 )
    {
        char n[255];
        *nameLen = (int)((*nextName) - name);
        (*nextName)++; /* skip '/' */
        strncpy(n, name, *nameLen);
        n[*nameLen] = 0;
        /*printf("->%s-< [%d] -> [%s]\n", name, *nameLen, *nextName);*/
    }
    else {
        *nameLen = (int)strlen(name);
    }
}

static TestNode *createTestNode(const char* name, int32_t nameLen)
{
    TestNode *newNode;

    newNode = (TestNode*)malloc(sizeof(TestNode) + (nameLen + 1));

    newNode->test = NULL;
    newNode->sibling = NULL;
    newNode->child = NULL;

    strncpy( newNode->name, name, nameLen );
    newNode->name[nameLen] = 0;

    return  newNode;
}

void T_CTEST_EXPORT2
cleanUpTestTree(TestNode *tn)
{
    if(tn->child != NULL) {
        cleanUpTestTree(tn->child);
    }
    if(tn->sibling != NULL) {
        cleanUpTestTree(tn->sibling);
    }

    free(tn);
}


void T_CTEST_EXPORT2
addTest(TestNode** root,
        TestFunctionPtr test,
        const char* name )
{
    TestNode *newNode;

    /*if this is the first Test created*/
    if (*root == NULL)
        *root = createTestNode("", 0);

    newNode = addTestNode( *root, name );
    assert(newNode != 0 );
    /*  printf("addTest: nreName = %s\n", newNode->name );*/

    newNode->test = test;
}

/* non recursive insert function */
static TestNode *addTestNode ( TestNode *root, const char *name )
{
    const char* nextName;
    TestNode *nextNode, *curNode;
    int nameLen; /* length of current 'name' */

    /* remove leading slash */
    if ( *name == TEST_SEPARATOR )
        name++;

    curNode = root;

    for(;;)
    {
        /* Start with the next child */
        nextNode = curNode->child;

        getNextLevel ( name, &nameLen, &nextName );

        /*      printf("* %s\n", name );*/

        /* if nextNode is already null, then curNode has no children
        -- add them */
        if( nextNode == NULL )
        {
            /* Add all children of the node */
            do
            {
                /* Get the next component of the name */
                getNextLevel(name, &nameLen, &nextName);

                /* update curName to have the next name segment */
                curNode->child = createTestNode(name, nameLen);
                /* printf("*** added %s\n", curNode->child->name );*/
                curNode = curNode->child;
                name = nextName;
            }
            while( name != NULL );

            return curNode;
        }

        /* Search across for the name */
        while (strncmp_nullcheck ( name, nextNode->name, nameLen) != 0 )
        {
            curNode = nextNode;
            nextNode = nextNode -> sibling;

            if ( nextNode == NULL )
            {
                /* Did not find 'name' on this level. */
                nextNode = createTestNode(name, nameLen);
                curNode->sibling = nextNode;
                break;
            }
        }

        /* nextNode matches 'name' */

        if (nextName == NULL) /* end of the line */
        {
            return nextNode;
        }

        /* Loop again with the next item */
        name = nextName;
        curNode = nextNode;
    }
}

static void iterateTestsWithLevel ( const TestNode* root,
                 int len,
                 const TestNode** list,
                 TestMode mode)
{
    int i;
    int saveIndent;

    char pathToFunction[MAXTESTNAME] = "";
    char separatorString[2] = { TEST_SEPARATOR, '\0'};

    if ( root == NULL )
        return;

    list[len++] = root;

    for ( i=0;i<(len-1);i++ )
    {
        strcat(pathToFunction, list[i]->name);
        strcat(pathToFunction, separatorString);
    }

    strcat(pathToFunction, list[i]->name);

    INDENT_LEVEL = len;
    if ( (mode == RUNTESTS) && (root->test != NULL))
    {
        int myERROR_COUNT = ERROR_COUNT;
        currentTest = root;
        root->test();
        currentTest = NULL;
        if (myERROR_COUNT != ERROR_COUNT)
        {

            log_info("---[%d ERRORS] ", ERROR_COUNT - myERROR_COUNT);
            strcpy(ERROR_LOG[ERRONEOUS_FUNCTION_COUNT++], pathToFunction);
        }
        else
            log_info("---[OK] ");
    }


    /* we want these messages to be at 0 indent. so just push the indent level breifly. */
    saveIndent = INDENT_LEVEL;
    INDENT_LEVEL = 0;
    log_info("%s%s%c\n", (list[i]->test||mode==SHOWTESTS)?"---":"",pathToFunction, list[i]->test?' ':TEST_SEPARATOR );
    INDENT_LEVEL = saveIndent;

    iterateTestsWithLevel ( root->child, len, list, mode );

    len--;

    if ( len != 0 ) /* DO NOT iterate over siblings of the root. */
        iterateTestsWithLevel ( root->sibling, len, list, mode );
}



void T_CTEST_EXPORT2
showTests ( const TestNode *root )
{
    /* make up one for them */
    const TestNode *aList[MAXTESTS];

    if (root == NULL)
        log_err("TEST CAN'T BE FOUND!");

    iterateTestsWithLevel ( root, 0, aList, SHOWTESTS );

}

void T_CTEST_EXPORT2
runTests ( const TestNode *root )
{
    int i;
    const TestNode *aList[MAXTESTS];
    /* make up one for them */


    if (root == NULL)
        log_err("TEST CAN'T BE FOUND!\n");

    ERRONEOUS_FUNCTION_COUNT = ERROR_COUNT = 0;
    iterateTestsWithLevel ( root, 0, aList, RUNTESTS );

    /*print out result summary*/

    if (ERROR_COUNT)
    {
        log_info("\nSUMMARY:\n******* [Total error count:\t%d]\n Errors in\n", ERROR_COUNT);
        for (i=0;i < ERRONEOUS_FUNCTION_COUNT; i++)
            log_info("[%s]\n",ERROR_LOG[i]);
    }
    else
    {
      log_info("\n[All tests passed successfully...]\n");
    }

    if(DATA_ERROR_COUNT) {
      if(WARN_ON_MISSING_DATA==0) {
        log_info("\t*Note* some errors are data-loading related. If the data used is not the \n"
                 "\tstock ICU data (i.e some have been added or removed), consider using\n"
                 "\tthe '-w' option to turn these errors into warnings.\n");
      } else {
        log_info("\t*WARNING* some data-loading errors were ignored by the -w option.\n");
      }
    }
}

const char* T_CTEST_EXPORT2
getTestName(void)
{
  if(currentTest != NULL) {
    return currentTest->name;
  } else {
    return NULL;
  }
}

const TestNode* T_CTEST_EXPORT2
getTest(const TestNode* root, const char* name)
{
    const char* nextName;
    TestNode *nextNode;
    const TestNode* curNode;
    int nameLen; /* length of current 'name' */

    if (root == NULL) {
        log_err("TEST CAN'T BE FOUND!\n");
        return NULL;
    }
    /* remove leading slash */
    if ( *name == TEST_SEPARATOR )
        name++;

    curNode = root;

    for(;;)
    {
        /* Start with the next child */
        nextNode = curNode->child;

        getNextLevel ( name, &nameLen, &nextName );

        /*      printf("* %s\n", name );*/

        /* if nextNode is already null, then curNode has no children
        -- add them */
        if( nextNode == NULL )
        {
            return NULL;
        }

        /* Search across for the name */
        while (strncmp_nullcheck ( name, nextNode->name, nameLen) != 0 )
        {
            curNode = nextNode;
            nextNode = nextNode -> sibling;

            if ( nextNode == NULL )
            {
                /* Did not find 'name' on this level. */
                return NULL;
            }
        }

        /* nextNode matches 'name' */

        if (nextName == NULL) /* end of the line */
        {
            return nextNode;
        }

        /* Loop again with the next item */
        name = nextName;
        curNode = nextNode;
    }
}

static void vlog_err(const char *prefix, const char *pattern, va_list ap)
{
    if( ERR_MSG == FALSE){
        return;
    }
    fprintf(stderr, "%-*s", INDENT_LEVEL," " );
    if(prefix) {
        fputs(prefix, stderr);
    }
    vfprintf(stderr, pattern, ap);
    fflush(stderr);
    va_end(ap);
}

void T_CTEST_EXPORT2
vlog_info(const char *prefix, const char *pattern, va_list ap)
{
    fprintf(stdout, "%-*s", INDENT_LEVEL," " );
    if(prefix) {
        fputs(prefix, stdout);
    }
    vfprintf(stdout, pattern, ap);
    fflush(stdout);
    va_end(ap);
}

static void vlog_verbose(const char *prefix, const char *pattern, va_list ap)
{
    if ( VERBOSITY == FALSE )
        return;

    fprintf(stdout, "%-*s", INDENT_LEVEL," " );
    if(prefix) {
        fputs(prefix, stdout);
    }
    vfprintf(stdout, pattern, ap);
    fflush(stdout);
    va_end(ap);
}

void T_CTEST_EXPORT2
log_err(const char* pattern, ...)
{
    va_list ap;
    if(strchr(pattern, '\n') != NULL) {
        /*
         * Count errors only if there is a line feed in the pattern
         * so that we do not exaggerate our error count.
         */
        ++ERROR_COUNT;
    }
    va_start(ap, pattern);
    vlog_err(NULL, pattern, ap);
}

void T_CTEST_EXPORT2
log_err_status(UErrorCode status, const char* pattern, ...)
{
    va_list ap;
    va_start(ap, pattern);
    
    if ((status == U_FILE_ACCESS_ERROR || status == U_MISSING_RESOURCE_ERROR)) {
        ++DATA_ERROR_COUNT; /* for informational message at the end */
        
        if (WARN_ON_MISSING_DATA == 0) {
            /* Fatal error. */
            if (strchr(pattern, '\n') != NULL) {
                ++ERROR_COUNT;
            }
            vlog_err(NULL, pattern, ap); /* no need for prefix in default case */
        } else {
            vlog_info("[DATA] ", pattern, ap); 
        }
    } else {
        /* Fatal error. */
        if(strchr(pattern, '\n') != NULL) {
            ++ERROR_COUNT;
        }
        vlog_err(NULL, pattern, ap); /* no need for prefix in default case */
    }
}

void T_CTEST_EXPORT2
log_info(const char* pattern, ...)
{
    va_list ap;

    va_start(ap, pattern);
    vlog_info(NULL, pattern, ap);
}

void T_CTEST_EXPORT2
log_verbose(const char* pattern, ...)
{
    va_list ap;

    va_start(ap, pattern);
    vlog_verbose(NULL, pattern, ap);
}


void T_CTEST_EXPORT2
log_data_err(const char* pattern, ...)
{
    va_list ap;
    va_start(ap, pattern);

    ++DATA_ERROR_COUNT; /* for informational message at the end */

    if(WARN_ON_MISSING_DATA == 0) {
        /* Fatal error. */
        if(strchr(pattern, '\n') != NULL) {
            ++ERROR_COUNT;
        }
        vlog_err(NULL, pattern, ap); /* no need for prefix in default case */
    } else {
        vlog_info("[DATA] ", pattern, ap); 
    }
}


/*
 * Tracing functions.
 */
static int traceFnNestingDepth = 0;
U_CDECL_BEGIN
static void U_CALLCONV TraceEntry(const void *context, int32_t fnNumber) {
    char buf[500];
    utrace_format(buf, sizeof(buf), traceFnNestingDepth*3, "%s() enter.\n", utrace_functionName(fnNumber));    buf[sizeof(buf)-1]=0;  
    fputs(buf, stdout);
    traceFnNestingDepth++;
}   
 
static void U_CALLCONV TraceExit(const void *context, int32_t fnNumber, const char *fmt, va_list args) {    char buf[500];
    
    if (traceFnNestingDepth>0) {
        traceFnNestingDepth--; 
    }
    utrace_format(buf, sizeof(buf), traceFnNestingDepth*3, "%s() ", utrace_functionName(fnNumber));    buf[sizeof(buf)-1]=0;
    fputs(buf, stdout);
    utrace_vformat(buf, sizeof(buf), traceFnNestingDepth*3, fmt, args);
    buf[sizeof(buf)-1]=0;
    fputs(buf, stdout);
    putc('\n', stdout);
}

static void U_CALLCONV TraceData(const void *context, int32_t fnNumber,
                          int32_t level, const char *fmt, va_list args) {
    char buf[500];
    utrace_vformat(buf, sizeof(buf), traceFnNestingDepth*3, fmt, args);
    buf[sizeof(buf)-1]=0;
    fputs(buf, stdout);
    putc('\n', stdout);
}

static void *U_CALLCONV ctest_libMalloc(const void *context, size_t size) {
    /*if (VERBOSITY) {
        printf("Allocated %ld\n", (long)size);
    }*/
    if (MINIMUM_MEMORY_SIZE_FAILURE <= size && size <= MAXIMUM_MEMORY_SIZE_FAILURE) {
        return NULL;
    }
    umtx_atomic_inc(&ALLOCATION_COUNT);
    return malloc(size);
}
static void *U_CALLCONV ctest_libRealloc(const void *context, void *mem, size_t size) {
    /*if (VERBOSITY) {
        printf("Reallocated %ld\n", (long)size);
    }*/
    if (MINIMUM_MEMORY_SIZE_FAILURE <= size && size <= MAXIMUM_MEMORY_SIZE_FAILURE) {
        /*free(mem);*/ /* Realloc doesn't free on failure. */
        return NULL;
    }
    if (mem == NULL) {
        /* New allocation. */
        umtx_atomic_inc(&ALLOCATION_COUNT);
    }
    return realloc(mem, size);
}
static void U_CALLCONV ctest_libFree(const void *context, void *mem) {
    if (mem != NULL) {
        umtx_atomic_dec(&ALLOCATION_COUNT);
    }
    free(mem);
}

int T_CTEST_EXPORT2
initArgs( int argc, const char* const argv[], ArgHandlerPtr argHandler, void *context)
{
    int                i;
    int                doList = FALSE;
	int                argSkip = 0;

    VERBOSITY = FALSE;
    ERR_MSG = TRUE;

    for( i=1; i<argc; i++)
    {
        if ( argv[i][0] == '/' )
        {
            /* We don't run the tests here. */
            continue;
        }
        else if ((strcmp( argv[i], "-a") == 0) || (strcmp(argv[i],"-all") == 0))
        {
            /* We don't run the tests here. */
            continue;
        }
        else if (strcmp( argv[i], "-v" )==0 || strcmp( argv[i], "-verbose")==0)
        {
            VERBOSITY = TRUE;
        }
        else if (strcmp( argv[i], "-l" )==0 )
        {
            doList = TRUE;
        }
        else if (strcmp( argv[i], "-e1") == 0)
        {
            QUICK = -1;
        }
        else if (strcmp( argv[i], "-e") ==0)
        {
            QUICK = 0;
        }
        else if (strcmp( argv[i], "-w") ==0)
        {
            WARN_ON_MISSING_DATA = TRUE;
        }
        else if (strcmp( argv[i], "-m") ==0)
        {
            UErrorCode errorCode = U_ZERO_ERROR;
            if (i+1 < argc) {
                char *endPtr = NULL;
                i++;
                MINIMUM_MEMORY_SIZE_FAILURE = (size_t)strtol(argv[i], &endPtr, 10);
                if (endPtr == argv[i]) {
                    printf("Can't parse %s\n", argv[i]);
                    help(argv[0]);
                    return 0;
                }
                if (*endPtr == '-') {
                    char *maxPtr = endPtr+1;
                    endPtr = NULL;
                    MAXIMUM_MEMORY_SIZE_FAILURE = (size_t)strtol(maxPtr, &endPtr, 10);
                    if (endPtr == argv[i]) {
                        printf("Can't parse %s\n", argv[i]);
                        help(argv[0]);
                        return 0;
                    }
                }
            }
            /* Use the default value */
            u_setMemoryFunctions(NULL, ctest_libMalloc, ctest_libRealloc, ctest_libFree, &errorCode);
            if (U_FAILURE(errorCode)) {
                printf("u_setMemoryFunctions returned %s\n", u_errorName(errorCode));
                return 0;
            }
        }
        else if(strcmp( argv[i], "-n") == 0 || strcmp( argv[i], "-no_err_msg") == 0)
        {
            ERR_MSG = FALSE;
        }
        else if (strcmp( argv[i], "-r") == 0)
        {
            if (!REPEAT_TESTS_INIT) {
                REPEAT_TESTS++;
            }
        }
        else if (strcmp( argv[i], "-t_info") == 0) {
            ICU_TRACE = UTRACE_INFO;
        }
        else if (strcmp( argv[i], "-t_error") == 0) {
            ICU_TRACE = UTRACE_ERROR;
        }
        else if (strcmp( argv[i], "-t_warn") == 0) {
            ICU_TRACE = UTRACE_WARNING;
        }
        else if (strcmp( argv[i], "-t_verbose") == 0) {
            ICU_TRACE = UTRACE_VERBOSE;
        }
        else if (strcmp( argv[i], "-t_oc") == 0) {
            ICU_TRACE = UTRACE_OPEN_CLOSE;
        }
        else if (strcmp( argv[i], "-h" )==0 || strcmp( argv[i], "--help" )==0)
        {
            help( argv[0] );
            return 0;
        }
        else if (argHandler != NULL && (argSkip = argHandler(i, argc, argv, context)) > 0)
        {
            i += argSkip - 1;
        }
        else
        {
            printf("* unknown option: %s\n", argv[i]);
            help( argv[0] );
            return 0;
        }
    }
    if (ICU_TRACE != UTRACE_OFF) {
        utrace_setFunctions(NULL, TraceEntry, TraceExit, TraceData);
        utrace_setLevel(ICU_TRACE);
    }

    return 1; /* total error count */
}

int T_CTEST_EXPORT2
runTestRequest(const TestNode* root,
             int argc,
             const char* const argv[])
{
    /**
     * This main will parse the l, v, h, n, and path arguments
     */
    const TestNode*    toRun;
    int                i;
    int                doList = FALSE;
    int                subtreeOptionSeen = FALSE;

    int                errorCount = 0;

    toRun = root;

    for( i=1; i<argc; i++)
    {
        if ( argv[i][0] == '/' )
        {
            printf("Selecting subtree '%s'\n", argv[i]);

            if ( argv[i][1] == 0 )
                toRun = root;
            else
                toRun = getTest(root, argv[i]);

            if ( toRun == NULL )
            {
                printf("* Could not find any matching subtree\n");
                return -1;
            }

            if( doList == TRUE)
                showTests(toRun);
            else
                runTests(toRun);

            errorCount += ERROR_COUNT;

            subtreeOptionSeen = TRUE;
        } else if ((strcmp( argv[i], "-a") == 0) || (strcmp(argv[i],"-all") == 0)) {
            subtreeOptionSeen=FALSE;
        } else if (strcmp( argv[i], "-l") == 0) {
            doList = TRUE;
        }
        /* else option already handled by initArgs */
    }

    if( subtreeOptionSeen == FALSE) /* no other subtree given, run the default */
    {
        if( doList == TRUE)
            showTests(toRun);
        else
            runTests(toRun);

        errorCount += ERROR_COUNT;
    }
    else
    {
        if( ( doList == FALSE ) && ( errorCount > 0 ) )
            printf(" Total errors: %d\n", errorCount );
    }

    REPEAT_TESTS_INIT = 1;

    return errorCount; /* total error count */
}

/**
 * Display program invocation arguments
 */

static void help ( const char *argv0 )
{
    printf("Usage: %s [ -l ] [ -v ] [ -verbose] [-a] [ -all] [-n] [ -no_err_msg]\n"
           "    [ -h ] [-t_info | -t_error | -t_warn | -t_oc | -t_verbose] [-m n[-q] ]\n"
           "    [ /path/to/test ]\n",
            argv0);
    printf("    -l  To get a list of test names\n");
    printf("    -e  to do exhaustive testing\n");
    printf("    -verbose To turn ON verbosity\n");
    printf("    -v  To turn ON verbosity(same as -verbose)\n");
    printf("    -h  To print this message\n");
    printf("    -n  To turn OFF printing error messages\n");
    printf("    -w  Don't fail on data-loading errs, just warn. Useful if\n"
           "        user has reduced/changed the common set of ICU data \n");
    printf("    -t_info | -t_error | -t_warn | -t_oc | -t_verbose  Enable ICU tracing\n");
    printf("    -no_err_msg (same as -n) \n");
    printf("    -m n[-q] Min-Max memory size that will cause an allocation failure.\n");
    printf("        The default is the maximum value of size_t. Max is optional.\n");
    printf("    -r  Repeat tests after calling u_cleanup \n");
    printf("    [/subtest]  To run a subtest \n");
    printf("    eg: to run just the utility tests type: cintltest /tsutil) \n");
}


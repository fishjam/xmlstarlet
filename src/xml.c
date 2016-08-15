/*  $Id: xml.c,v 1.37 2004/11/11 03:39:34 mgrouch Exp $  */

/*

XMLStarlet: Command Line Toolkit to query/edit/check/transform XML documents

Copyright (c) 2002 Mikhail Grushinskiy.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <config.h>
#include <version.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <libxslt/xslt.h>
#include <libxslt/xsltconfig.h>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <shellapi.h>         /* CommandLineToArgvW() */
#endif

#include "xmlstar.h"

gOptions globalOptions;

static const xmlChar* XMLSTAR_NS = BAD_CAST "http://xmlstar.sourceforge.net";
static const xmlChar* XMLSTAR_NS_PREFIX = BAD_CAST "xstar";

extern int edMain(int argc, char **argv);
extern int selMain(int argc, char **argv);
extern int trMain(int argc, char **argv);
extern int valMain(int argc, char **argv);
extern int foMain(int argc, char **argv);
extern int elMain(int argc, char **argv);
extern int c14nMain(int argc, char **argv);
extern int lsMain(int argc, char **argv);
extern int pyxMain(int argc, char **argv);
extern int depyxMain(int argc, char **argv);
extern int escMain(int argc, char **argv, int escape);

const char more_info[] =
"XMLStarlet is a command line toolkit to query/edit/check/transform\n"
"XML documents (for more information see http://xmlstar.sourceforge.net/)\n";

const char libxslt_more_info[] =
"\n"
"Current implementation uses libxslt from GNOME codebase as XSLT processor\n"
"(see http://xmlsoft.org/ for more details)\n";

/**
 *  Display usage syntax
 */
void
usage(int argc, char **argv, exit_status status)
{
    extern void fprint_usage(FILE* o, const char* argv0);
    FILE* o = (status == EXIT_SUCCESS)? stdout : stderr;
    fprint_usage(o, argv[0]);
    fprintf(o, "%s", more_info);
    exit(status);
}  

/**
 *  Initialize global command line options
 */
void
gInitOptions(gOptionsPtr ops)
{
    ops->quiet = 0;
    ops->doc_namespace = 1;
}

/**
 * Error reporting functions
 */
void reportGenericError(void* ctx, const char * msg, ...) {
    /* do nothing */
}

/* by default all errors are reported */
static ErrorInfo errorInfo = { NULL, NULL, VERBOSE, CONTINUE };

void reportError(void *ptr, xmlErrorPtr error)
{
    ErrorInfo *errorInfo = (ErrorInfo*) ptr;
    assert(errorInfo);

    if (errorInfo->verbose)
    {
        int msglen;
        int domain = error->domain;
        const char *filename =
            error->file? error->file :
            errorInfo? errorInfo->filename :
            NULL;
        xmlTextReaderPtr reader = errorInfo->xmlReader;

        int line = (!filename)? 0 :
            (reader)? xmlTextReaderGetParserLineNumber(reader) :
            error->line;
        int column = (!filename)? 0 :
            (reader)? xmlTextReaderGetParserColumnNumber(reader) :
            error->int2;
        if (line)
        {
            fprintf(stderr, "%s:%d.%d: ", filename, line, column);
        }

        msglen = strlen(error->message);
        if (error->message[msglen-1] == '\n')
            error->message[msglen-1] = '\0';
        fprintf(stderr, "%s", error->message);

        /* only print extra info if it's not in message */
        if (error->str1 && strstr(error->message, error->str1) == NULL) {
            fprintf(stderr, ": %s", error->str1);
            if (error->str2 && strstr(error->message, error->str2) == NULL) {
                fprintf(stderr, ", %s", error->str2);
            }
            if (error->str3 && strstr(error->message, error->str3) == NULL) {
                fprintf(stderr, ", %s", error->str3);
            }
        }
        fprintf(stderr, "\n");


        if ((domain == XML_FROM_PARSER) || (domain == XML_FROM_HTML) ||
            (domain == XML_FROM_DTD) || (domain == XML_FROM_NAMESPACE) ||
            (domain == XML_FROM_IO) || (domain == XML_FROM_VALID)) {
            xmlParserCtxtPtr ctxt = error->ctxt;
            if (ctxt) xmlParserPrintFileContext(ctxt->input);
        }
    }
    if (errorInfo->stop == STOP) {
        exit(EXIT_FAILURE);
    }
}

void
suppressErrors(void)
{
    xmlSetGenericErrorFunc(NULL, reportGenericError);
    errorInfo.verbose = QUIET;
}

#define CHECK_MEM(ret) if (!ret) \
        (fprintf(stderr, "out of memory\n"), exit(EXIT_INTERNAL_ERROR))

void*
xmalloc(size_t size)
{
    void *ret = malloc(size);
    CHECK_MEM(ret);
    return ret;
}
void*
xrealloc(void *ptr, size_t size)
{
    void *ret = realloc(ptr, size);
    CHECK_MEM(ret);
    return ret;
}
char*
xstrdup(const char *str)
{
    char *ret = (char*) xmlStrdup(BAD_CAST str);
    CHECK_MEM(ret);
    return ret;
}


#ifdef _WIN32
/* On Windows, it's not really practical to get argv in UTF-8, so we have to do
   this little dance. */
static void
gGetUnicodeOptions(int argc, char **argv)
{
    int nArgs, i;
    LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    assert(nArgs == argc);

    for (i = 0; i < argc; i++) {
        char *utf;
        int utflen = WideCharToMultiByte(CP_UTF8, 0,
            szArglist[i], -1,
            NULL, 0, NULL, NULL);
        if (utflen <= 0) {
            fprintf(stderr, "Error decoding argument %d\n", i);
            exit(EXIT_BAD_ARGS);
        }
        utf = malloc(utflen);
        WideCharToMultiByte(CP_UTF8, 0,
            szArglist[i], -1,
            utf, utflen, NULL, NULL);
        argv[i] = utf;
    }
    LocalFree(szArglist);
}
#else
#  define gGetUnicodeOptions(argc, argv)
#endif

/**
 *  Parse global command line options
 */
void
gParseOptions(gOptionsPtr ops, int *argc, char **argv)
{
    int i, j;
    i = 1;
    while(i < *argc)
    {
        if (!strcmp(argv[i], "--quiet") || !strcmp(argv[i], "-q"))
        {
            ops->quiet = 1;
            i++;
        }
        else if (!strcmp(argv[i], "--no-doc-namespace"))
        {
            ops->doc_namespace = 0;
            i++;
        }
        else if (!strcmp(argv[i], "--doc-namespace"))
        {
            ops->doc_namespace = 1;
            i++;
        }
        else if (!strcmp(argv[i], "--version"))
        {
            fprintf(stdout, "%s\n"
                "compiled against libxml2 %s, linked with %s\n"
                "compiled against libxslt %s, linked with %s\n",
                VERSION,
                LIBXML_DOTTED_VERSION, xmlParserVersion,
                LIBXSLT_DOTTED_VERSION, xsltEngineVersion);
            exit(EXIT_SUCCESS);
        }
        else if (!strcmp(argv[i], "--help"))
        {
            usage(*argc, argv, EXIT_SUCCESS);
        }
        else if (argv[i][0] != '-')
        {
            /* remove parsed arguments */
            i--;
            for (j = 1; j < *argc; j++) {
                if (j < *argc - i)
                    argv[j] = argv[j + i];
                else
                    argv[j] = 0;
            }
            *argc -= i;
            return;
        }
        else
        {
            usage(*argc, argv, EXIT_BAD_ARGS);
        }
    }
}

/**
 *  This is the main function
 */
int
main(int argc, char **argv)
{
    int ret = 0;

    xmlMemSetup(free, xmalloc, xrealloc, xstrdup);

    gGetUnicodeOptions(argc, argv);
    gInitOptions(&globalOptions);
    gParseOptions(&globalOptions, &argc, argv);
    
    xmlSetStructuredErrorFunc(&errorInfo, reportError);
    if (globalOptions.quiet)
        suppressErrors();

    if (argc <= 1)
    {
        usage(argc, argv, EXIT_BAD_ARGS);
    }
    else if (!strcmp(argv[1], "ed") || !strcmp(argv[1], "edit"))
    {
        ret = edMain(argc, argv);
    }
    else if (!strcmp(argv[1], "sel") || !strcmp(argv[1], "select"))
    {
        ret = selMain(argc, argv);
    }
    else if (!strcmp(argv[1], "tr") || !strcmp(argv[1], "transform"))
    {
        ret = trMain(argc, argv);
    }
    else if (!strcmp(argv[1], "fo") || !strcmp(argv[1], "format"))
    {
        ret = foMain(argc, argv);
    }
    else if (!strcmp(argv[1], "val") || !strcmp(argv[1], "validate"))
    {
        ret = valMain(argc, argv);
    }
    else if (!strcmp(argv[1], "el") || !strcmp(argv[1], "elements"))
    {
        ret = elMain(argc, argv);
    }
    else if (!strcmp(argv[1], "c14n") || !strcmp(argv[1], "canonic"))
    {
        ret = c14nMain(argc, argv);
    }
    else if (!strcmp(argv[1], "ls") || !strcmp(argv[1], "list"))
    {
        ret = lsMain(argc, argv);
    }
    else if (!strcmp(argv[1], "pyx") || !strcmp(argv[1], "xmln"))
    {
        ret = pyxMain(argc, argv);
    }
    else if (!strcmp(argv[1], "depyx") || !strcmp(argv[1], "p2x"))
    {
        ret = depyxMain(argc, argv);
    }
    else if (!strcmp(argv[1], "esc") || !strcmp(argv[1], "escape"))
    {
        ret = escMain(argc, argv, 1);
    }
    else if (!strcmp(argv[1], "unesc") || !strcmp(argv[1], "unescape"))
    {
        ret = escMain(argc, argv, 0);
    }
    else
    {
        usage(argc, argv, EXIT_BAD_ARGS);
    }

    exit(ret);
}


void
registerXstarVariable(xmlXPathContextPtr ctxt,
    const char* name, xmlXPathObjectPtr value)
{
    xmlXPathRegisterVariableNS(ctxt, BAD_CAST name, XMLSTAR_NS, value);
}

static xmlXPathObjectPtr varLookupFallbackToXstarNS(void* ctxt_vp,
    const xmlChar* name, const xmlChar* ns_uri)
{
    xmlXPathObjectPtr ret;
    xmlXPathContextPtr ctxt = ctxt_vp;

    ctxt->varLookupFunc = NULL; /* avoid infinite recursion! */

    /* first get the default lookup value */
    ret = xmlXPathVariableLookupNS(ctxt, name, ns_uri);

    if (!ret && !ns_uri) {
        /* if we didn't find anything, and there was no namespace given,
           try looking in XMLStarlet namespace */
        ret = xmlXPathVariableLookupNS(ctxt, name, XMLSTAR_NS);
    }

    ctxt->varLookupFunc = &varLookupFallbackToXstarNS;
    return ret;
}

void
registerXstarNs(xmlXPathContextPtr ctxt)
{
    xmlXPathRegisterVariableLookup(ctxt, &varLookupFallbackToXstarNS, ctxt);
    xmlXPathRegisterNs(ctxt, XMLSTAR_NS_PREFIX, XMLSTAR_NS);
}


static void bad_ns_opt(const char *msg)
{
    fprintf(stderr, "Bad namespace option: %s\n", msg);
    exit(EXIT_BAD_ARGS);
}

const xmlChar *default_ns = NULL;

#define MAX_NS_ARGS    256
xmlChar *ns_arr[2 * MAX_NS_ARGS + 1];

/**
 *  Parse command line for -N <prefix>=<namespace> arguments
 */
int
parseNSArr(xmlChar** ns_arr, int* plen, int argc, char **argv)
{
    int i = 0;
    *plen = 0;
    ns_arr[0] = 0;

    for (i=0; i<argc; i++)
    {
        int prefix_len;
        xmlChar *name, *value;
        const xmlChar *equal_sign;

        /* check for end of arguments */
        if (argv[i] == 0 || argv[i][0] != '-')
            break;
        if (strcmp(argv[i], "-N") != 0)
            continue;

        i++;
        if (i >= argc) bad_ns_opt("-N without argument");

        equal_sign = xmlStrchr((const xmlChar*) argv[i], '=');
        if (!equal_sign)
            bad_ns_opt("namespace should have the form <prefix>=<url>");
        prefix_len = equal_sign - (const xmlChar*) argv[i];

        name = xmlStrndup((const xmlChar*) argv[i], prefix_len);
        value = xmlStrdup((const xmlChar*) argv[i]+prefix_len+1);

        if (*plen >= MAX_NS_ARGS)
        {
            fprintf(stderr, "too many namespaces increase MAX_NS_ARGS\n");
            exit(EXIT_BAD_ARGS);
        }

        ns_arr[*plen] = name;
        (*plen)++;
        ns_arr[*plen] = value;
        (*plen)++;
        ns_arr[*plen] = 0;

    }

    return i;
}

/**
 *  Cleanup memory allocated by namespaces arguments
 */
void
cleanupNSArr(xmlChar **ns_arr)
{
    xmlChar **p = ns_arr;

    while (*p)
    {
        xmlFree(*p);
        p++;
    }
}

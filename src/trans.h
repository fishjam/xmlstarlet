/*  $Id: trans.h,v 1.11 2004/11/21 23:40:40 mgrouch Exp $  */

#ifndef __TRANS_H
#define __TRANS_H

/*

XMLStarlet: Command Line Toolkit to query/edit/check/transform XML documents

Copyright (c) 2002-2004 Mikhail Grushinskiy.  All Rights Reserved.

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

#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/xmlIO.h>
#include <libxml/HTMLtree.h>
#include <libxml/xinclude.h>
#include <libxml/parserInternals.h>
#include <libxml/uri.h>

#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxslt/extensions.h>
#include <libexslt/exslt.h>

#ifdef LIBXML_XINCLUDE_ENABLED
#include <libxml/xinclude.h>
#endif
#ifdef LIBXML_CATALOG_ENABLED
#include <libxml/catalog.h>
#endif

#define MAX_PARAMETERS 256
#define MAX_PATHS 256

typedef struct _xsltOptions {
    int noval;                /* do not validate against DTDs or schemas */
    int nonet;                /* refuse to fetch DTDs or entities over network */
    int show_extensions;      /* display list of extensions */
    int omit_decl;            /* omit xml declaration */
    int noblanks;             /* Remove insignificant spaces from XML tree */
    int embed;                /* Allow applying embedded stylesheet */
#ifdef LIBXML_XINCLUDE_ENABLED
    int xinclude;             /* do XInclude processing on input documents */
#endif
#ifdef LIBXML_HTML_ENABLED
    int html;                 /* inputs are in HTML format */
#endif
#ifdef LIBXML_CATALOG_ENABLED
    int catalogs;             /* use SGML catalogs from $SGML_CATALOG_FILES */
#endif
} xsltOptions;

typedef xsltOptions *xsltOptionsPtr;


extern int errorno;

void xsltInitOptions(xsltOptionsPtr ops);

void xsltInitLibXml(xsltOptionsPtr ops);

void xsltProcess(xsltOptionsPtr ops, xmlDocPtr doc,
                 const char **params, xsltStylesheetPtr cur,
                 const char *filename);

xmlDocPtr xsltTransform(xsltOptionsPtr ops, xmlDocPtr doc,
                 const char **params, xsltStylesheetPtr cur,
                 const char *filename);

int xsltRun(xsltOptionsPtr ops, char* xsl,
            const char **params,
            int count, char **docs);

#endif /* __TRANS_H */

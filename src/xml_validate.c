/*  $Id: xml_validate.c,v 1.36 2005/01/07 01:52:43 mgrouch Exp $  */

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

#include <config.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "xmlstar.h"
#include "trans.h"

#ifdef LIBXML_SCHEMAS_ENABLED
#include <libxml/xmlschemas.h>
#include <libxml/xmlschemastypes.h>
#endif

#ifdef LIBXML_SCHEMAS_ENABLED
#include <libxml/relaxng.h>
#endif

#include <libxml/xmlreader.h>

/*
 *   TODO: Use cases
 *   1. find malfomed XML documents in a given set of XML files 
 *   2. find XML documents which do not match DTD/XSD in a given set of XML files
 *   3. precompile DTD once
 */

typedef struct _valOptions {
    char *dtd;                /* External DTD URL or file name */
    char *schema;             /* External Schema URL or file name */
    char *relaxng;            /* External Relax-NG Schema URL or file name */
    int   err;                /* Allow stderr messages */
    int   stop;               /* Stop on first error */
    int   embed;              /* Validate using embeded DTD */
    int   wellFormed;         /* Check if well formed only */
    int   listGood;           /* >0 list good, <0 list bad */
    int   show_val_res;       /* display file names and valid/invalid message */
    int   nonet;              /* disallow network access */
} valOptions;

typedef valOptions *valOptionsPtr;

/**
 *  display short help message
 */
void
valUsage(int argc, char **argv, exit_status status)
{
    extern void fprint_validate_usage(FILE* o, const char* argv0);
    extern const char more_info[];
    FILE *o = (status == EXIT_SUCCESS)? stdout : stderr;
    fprint_validate_usage(o, argv[0]);
    fprintf(o, "%s", more_info);
    exit(status);
}

/**
 *  Initialize global command line options
 */
void
valInitOptions(valOptionsPtr ops)
{
    ops->wellFormed = 1;
    ops->err = 0;
    ops->stop = 0;
    ops->embed = 0;
    ops->dtd = NULL;
    ops->schema = NULL;
    ops->relaxng = NULL;
    ops->nonet = 1;

    if (globalOptions.quiet) {
        ops->listGood = 0;
        ops->show_val_res = 0;
    } else {
        ops->listGood = -1;
        ops->show_val_res = 1;
    }
}

/**
 *  Parse global command line options
 */
int
valParseOptions(valOptionsPtr ops, int argc, char **argv)
{
    int i;

    i = 2;
    while(i < argc)
    {
        if (!strcmp(argv[i], "--well-formed") || !strcmp(argv[i], "-w"))
        {
            ops->wellFormed = 1;
            i++;
        }
        else if (!strcmp(argv[i], "--err") || !strcmp(argv[i], "-e"))
        {
            ops->err = 1;
            i++;
        }
        else if (!strcmp(argv[i], "--stop") || !strcmp(argv[i], "-S"))
        {
            ops->stop = STOP;
            i++;
        }
        else if (!strcmp(argv[i], "--embed") || !strcmp(argv[i], "-E"))
        {
            ops->embed = 1;
            i++;
        }
        else if (!strcmp(argv[i], "--list-good") || !strcmp(argv[i], "-g"))
        {
            ops->listGood = 1;
            ops->show_val_res = 0;
            i++;
        }
        else if (!strcmp(argv[i], "--list-bad") || !strcmp(argv[i], "-b"))
        {
            ops->listGood = -1;
            ops->show_val_res = 0;
            i++;
        }
        else if (!strcmp(argv[i], "--quiet") || !strcmp(argv[i], "-q"))
        {
            ops->listGood = 0;
            ops->show_val_res = 0;
            i++;
        }
        else if (!strcmp(argv[i], "--dtd") || !strcmp(argv[i], "-d"))
        {
            i++;
            if (i >= argc) valUsage(argc, argv, EXIT_BAD_ARGS);
            ops->dtd = argv[i];
            i++;
        }
        else if (!strcmp(argv[i], "--xsd") || !strcmp(argv[i], "-s"))
        {
            i++;
            if (i >= argc) valUsage(argc, argv, EXIT_BAD_ARGS);
            ops->schema = argv[i];
            i++;
        }
        else if (!strcmp(argv[i], "--relaxng") || !strcmp(argv[i], "-r"))
        {
            i++;
            if (i >= argc) valUsage(argc, argv, EXIT_BAD_ARGS);
            ops->relaxng = argv[i];
            i++;
        }
        else if (!strcmp(argv[i], "--net"))
        {
            ops->nonet = 0;
            i++;
        }
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h"))
        {
            valUsage(argc, argv, EXIT_SUCCESS);
        }
        else if (!strcmp(argv[i], "-"))
        {
            i++;
            break;
        }
        else if (argv[i][0] == '-')
        {
            valUsage(argc, argv, EXIT_BAD_ARGS);
        }
        else
        {
            i++;
            break;
        }
    }

    return i-1;
}

/**
 *  Validate XML document against DTD
 */
int
valAgainstDtd(valOptionsPtr ops, char* dtdvalid, xmlDocPtr doc, char* filename)
{
    int result = 0;

    if (dtdvalid != NULL)
    {
        xmlDtdPtr dtd;

#if !defined(LIBXML_VALID_ENABLED)
	xmlGenericError(xmlGenericErrorContext,
	"libxml2 has no validation support");
	return 2;
#endif
        dtd = xmlParseDTD(NULL, (const xmlChar *)dtdvalid);
        if (dtd == NULL)
        {
            xmlGenericError(xmlGenericErrorContext,
            "Could not parse DTD %s\n", dtdvalid);
            result = 2;
        }
        else
        {
            xmlValidCtxtPtr cvp;

            if ((cvp = xmlNewValidCtxt()) == NULL) 
            {
                xmlGenericError(xmlGenericErrorContext,
                    "Couldn't allocate validation context\n");
                exit(-1);
            }
        
            if (ops->err)
            {
                cvp->userData = (void *) stderr;
                cvp->error    = (xmlValidityErrorFunc) fprintf;
                cvp->warning  = (xmlValidityWarningFunc) fprintf;
            }
            else
            {
                cvp->userData = (void *) NULL;
                cvp->error    = (xmlValidityErrorFunc) NULL;
                cvp->warning  = (xmlValidityWarningFunc) NULL;
            }
                        
            if (!xmlValidateDtd(cvp, doc, dtd))
            {
                if ((ops->listGood < 0) && !ops->show_val_res)
                {
                    fprintf(stdout, "%s\n", filename);
                }
                else if (ops->listGood == 0)
                    xmlGenericError(xmlGenericErrorContext,
                                    "%s: does not match %s\n",
                                    filename, dtdvalid);
                result = 3;
            }
            else
            {
                if ((ops->listGood > 0) && !ops->show_val_res)
                {
                    fprintf(stdout, "%s\n", filename);
                }
            }
            xmlFreeDtd(dtd);
            xmlFreeValidCtxt(cvp);
        }
    }

    return result;
}

/**
 *  This is the main function for 'validate' option
 */
int
valMain(int argc, char **argv)
{
    int start;
    static valOptions ops;
    static ErrorInfo errorInfo;
    int invalidFound = 0;
    int options = XML_PARSE_DTDLOAD | XML_PARSE_DTDATTR;

    if (argc <= 2) valUsage(argc, argv, EXIT_BAD_ARGS);
    valInitOptions(&ops);
    start = valParseOptions(&ops, argc, argv);
    if (ops.nonet) options |= XML_PARSE_NONET;

    errorInfo.verbose = ops.err;
    xmlSetStructuredErrorFunc(&errorInfo, reportError);
    xmlLineNumbersDefault(1);

    if (ops.dtd)
    {
        /* xmlReader doesn't work with external dtd, have to use SAX
         * interface */
        int i;

        /* we have to exit() from the error reporting function to implement
           --stop */
        errorInfo.stop = ops.stop;

        for (i=start; i<argc; i++)
        {
            xmlDocPtr doc;
            int failed;

            failed = 0;
            doc = NULL;

            errorInfo.filename = argv[i];
            doc = xmlReadFile(argv[i], NULL, options);
            if (doc)
            {
                /* TODO: precompile DTD once */                
                failed = valAgainstDtd(&ops, ops.dtd, doc, argv[i]);
                xmlFreeDoc(doc);
            }
            else
            {
                failed = 1; /* Malformed XML or could not open file */
                if ((ops.listGood < 0) && !ops.show_val_res)
                {
                    fprintf(stdout, "%s\n", argv[i]);
                }
            }
            if (failed) invalidFound = 1;

            if (ops.show_val_res)
            {
                if (!failed)
                    fprintf(stdout, "%s - valid\n", argv[i]);
                else
                    fprintf(stdout, "%s - invalid\n", argv[i]);
            }
        }
    }
    else if (ops.schema || ops.relaxng || ops.embed || ops.wellFormed)
    {
        int i;
        xmlTextReaderPtr reader = NULL;

#ifdef LIBXML_SCHEMAS_ENABLED
        xmlSchemaPtr schema = NULL;
        xmlSchemaParserCtxtPtr schemaParserCtxt = NULL;
        xmlSchemaValidCtxtPtr schemaCtxt = NULL;

        xmlRelaxNGPtr relaxng = NULL;
        xmlRelaxNGParserCtxtPtr relaxngParserCtxt = NULL;
        /* there is no xmlTextReaderRelaxNGValidateCtxt() !?  */

        /* TODO: Do not print debug stuff */
        if (ops.schema)
        {
            schemaParserCtxt = xmlSchemaNewParserCtxt(ops.schema);
            if (!schemaParserCtxt)
            {
                invalidFound = 2;
                goto schemaCleanup;
            }
            errorInfo.filename = ops.schema;
            schema = xmlSchemaParse(schemaParserCtxt);
            if (!schema)
            {
                invalidFound = 2;
                goto schemaCleanup;
            }

            xmlSchemaFreeParserCtxt(schemaParserCtxt);
            schemaCtxt = xmlSchemaNewValidCtxt(schema);
            if (!schemaCtxt)
            {
                invalidFound = 2;
                goto schemaCleanup;
            }

        }
        else if (ops.relaxng)
        {
            relaxngParserCtxt = xmlRelaxNGNewParserCtxt(ops.relaxng);
            if (!relaxngParserCtxt)
            {
                invalidFound = 2;
                goto schemaCleanup;
            }

            errorInfo.filename = ops.relaxng;
            relaxng = xmlRelaxNGParse(relaxngParserCtxt);
            if (!relaxng)
            {
                invalidFound = 2;
                goto schemaCleanup;
            }

        }
#endif  /* LIBXML_SCHEMAS_ENABLED */

        for (i=start; i<argc; i++)
        {
            int failed = 0;
            if (ops.embed) options |= XML_PARSE_DTDVALID;

            if (!reader)
            {
                reader = xmlReaderForFile(argv[i], NULL, options);
            }
            else
            {
                failed = xmlReaderNewFile(reader, argv[i], NULL, options);
            }

            errorInfo.xmlReader = reader;
            errorInfo.filename = argv[i];

            /* It makes no sense to continue if we are not reporting errors
             * anyway. Note this doesn't apply to the --dtd case because the we
             * can't stop there without aborting the whole program (and
             * therefore we wouldn't be able to check multiple files).
             */
            if (!ops.err)
                ops.stop = STOP;

            if (reader && !failed)
            {
#ifdef LIBXML_SCHEMAS_ENABLED
                if (schemaCtxt)
                {
                    failed = xmlTextReaderSchemaValidateCtxt(reader,
                        schemaCtxt, 0);
                }
                else if (relaxng)
                {
                    failed = xmlTextReaderRelaxNGSetSchema(reader,
                        relaxng);
                }
#endif  /* LIBXML_SCHEMAS_ENABLED */

                if (failed == 0)
                {
                    int more_nodes;
                    int validating = (schema || relaxng || ops.embed);
                    do
                    {
                        more_nodes = xmlTextReaderRead(reader);
                        failed =
                            (more_nodes == -1)? 1 :
                            (!validating)? 0 :
                            xmlTextReaderIsValid(reader) != 1;
                    } while (more_nodes == 1 && (!failed || !ops.stop));
                }
            }
            else
            {
                if (ops.err)
                    fprintf(stderr, "couldn't read file '%s'\n", errorInfo.filename);
                failed = 1; /* could not open file */
            }
            if (failed) invalidFound = 1;

            if (!ops.show_val_res)
            {
                if ((ops.listGood > 0) && !failed)
                    fprintf(stdout, "%s\n", argv[i]);
                if ((ops.listGood < 0) && failed)
                    fprintf(stdout, "%s\n", argv[i]);
            }
            else
            {
                if (!failed)
                    fprintf(stdout, "%s - valid\n", argv[i]);
                else
                    fprintf(stdout, "%s - invalid\n", argv[i]);
            }
        }
        errorInfo.xmlReader = NULL;
        xmlFreeTextReader(reader);

#ifdef LIBXML_SCHEMAS_ENABLED
    schemaCleanup:
        xmlSchemaFreeValidCtxt(schemaCtxt);
        xmlRelaxNGFree(relaxng);
        xmlSchemaFree(schema);
        xmlRelaxNGCleanupTypes();
        xmlSchemaCleanupTypes();
#endif  /* LIBXML_SCHEMAS_ENABLED */
    }

    xmlCleanupParser();
    return invalidFound;
}

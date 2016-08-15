/*  $Id: xml_elem.c,v 1.23 2004/11/21 23:40:40 mgrouch Exp $  */

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

#include <libxml/xmlstring.h>
#include <libxml/hash.h>
#include <stdlib.h>
#include <string.h>

#include "xmlstar.h"
#include "escape.h"

/* TODO:

   2. Option to display this only for nodes matching
      an XPATH expression

      -p <xpath>

      so it will be able to deal with subtrees as well

*/

typedef struct _elOptions {
    int show_attr;            /* show attributes */
    int show_attr_and_val;    /* show attributes and values */
    int sort_uniq;            /* do sort and uniq on output */
    int check_depth;          /* limit depth */
} elOptions;


static elOptions elOps;
static xmlHashTablePtr uniq = NULL;
static xmlChar *curXPath = NULL;

/**
 *  Display usage syntax
 */
void
elUsage(int argc, char **argv, exit_status status)
{
    extern void fprint_elem_usage(FILE* o, const char* argv0);
    extern const char more_info[];
    FILE *o = (status == EXIT_SUCCESS)? stdout : stderr;
    fprint_elem_usage(o, argv[0]);
    fprintf(o, "%s", more_info);
    exit(status);
}

/**
 *  read file and print element paths
 */
int
parse_xml_file(const char *filename)
{
    int ret, prev_depth = 0;
    xmlTextReaderPtr reader;

    for (reader = xmlReaderForFile(filename, NULL, 0);;)
    {
        int depth;
        const xmlChar *name;
        xmlReaderTypes type;

        if (!reader) {
            fprintf(stderr, "couldn't read file '%s'\n", filename);
            exit(EXIT_BAD_FILE);
        }

        ret = xmlTextReaderRead(reader);
        if (ret <= 0) break;
        type = xmlTextReaderNodeType(reader);
        depth = xmlTextReaderDepth(reader);
        name = xmlTextReaderConstName(reader);

        if (type != XML_READER_TYPE_ELEMENT)
            continue;

        while (curXPath && depth <= prev_depth)
        {
            xmlChar *slash = BAD_CAST strrchr((char*) curXPath, '/');
            if (slash) *slash = '\0';
            prev_depth--;
        }
        prev_depth = depth;

        if (depth > 0) curXPath = xmlStrcat(curXPath, BAD_CAST "/");
        curXPath = xmlStrcat(curXPath, name);

        if (elOps.show_attr)
        {
            int have_attr;

            fprintf(stdout, "%s\n", curXPath);
            for (have_attr = xmlTextReaderMoveToFirstAttribute(reader);
                 have_attr;
                 have_attr = xmlTextReaderMoveToNextAttribute(reader))
            {
                const xmlChar *aname = xmlTextReaderConstName(reader);
                fprintf(stdout, "%s/@%s\n", curXPath, aname);
            }
        }
        else if (elOps.show_attr_and_val)
        {
            fprintf(stdout, "%s", curXPath);
            if (xmlTextReaderHasAttributes(reader))
            {
                int have_attr, first = 1;
                fprintf(stdout, "[");
                for (have_attr = xmlTextReaderMoveToFirstAttribute(reader);
                     have_attr;
                     have_attr = xmlTextReaderMoveToNextAttribute(reader))
                {
                    const xmlChar *aname = xmlTextReaderConstName(reader),
                        *avalue = xmlTextReaderConstValue(reader);
                    char quote;
                    if (!first)
                        fprintf(stdout, " and ");
                    first = 0;

                    quote = xmlStrchr(avalue, '\'')? '"' : '\'';
                    fprintf(stdout, "@%s=%c%s%c", aname, quote, avalue, quote);
                }
                fprintf(stdout, "]");
            }
            fprintf(stdout, "\n");
        }
        else if (elOps.sort_uniq)
        {
            if ((elOps.check_depth == 0) || (elOps.check_depth != 0 && depth < elOps.check_depth))
            {
                xmlHashAddEntry(uniq, curXPath, (void*) 1);
            }
        }
        else fprintf(stdout, "%s\n", curXPath);

    }

    return ret == -1? EXIT_LIB_ERROR : ret;
}

/**
 *  Initialize options values
 */
void
elInitOptions(elOptions *ops)
{
    ops->show_attr = 0;  
    ops->show_attr_and_val = 0;
    ops->sort_uniq = 0;
    ops->check_depth = 0; 
}

typedef struct {
    xmlChar **array;
    int offset;
} ArrayDest;

/**
 * put @name into @data->array[@data->offset]
 */
static void
hash_key_put(void *payload, void *data, xmlChar *name)
{
    ArrayDest *dest = data;
    dest->array[dest->offset++] = name;
}

/**
 * a compare function for qsort
 * takes pointers to 2 xmlChar* and compares them
 */
static int
compare_string_ptr(const void *p1, const void *p2)
{
    typedef xmlChar const *const xmlCChar;
    xmlCChar *str1 = p1, *str2 = p2;
    return xmlStrcmp(*str1, *str2);
}

/**
 *  This is the main function for 'el' option
 */
int
elMain(int argc, char **argv)
{
    int errorno = 0;
    char* inp_file = "-";

    if (argc <= 1) elUsage(argc, argv, EXIT_BAD_ARGS);

    elInitOptions(&elOps);

    if (argc == 2)
        errorno = parse_xml_file("-");  
    else
    {
        if (!strcmp(argv[2], "--help") || !strcmp(argv[2], "-h") ||
            !strcmp(argv[2], "-?") || !strcmp(argv[2], "-Z"))
        {
            elUsage(argc, argv, EXIT_SUCCESS);
        }
        else if (!strcmp(argv[2], "-a"))
        {
            elOps.show_attr = 1;
            if (argc >= 4) inp_file = argv[3];
            errorno = parse_xml_file(inp_file);
        }
        else if (!strcmp(argv[2], "-v"))
        {
            elOps.show_attr_and_val = 1;
            if (argc >= 4) inp_file = argv[3];
            errorno = parse_xml_file(inp_file);
        }
        else if (!strcmp(argv[2], "-u"))
        {
            elOps.sort_uniq = 1;
            if (argc >= 4) inp_file = argv[3];
            uniq = xmlHashCreate(0);
            errorno = parse_xml_file(inp_file);
        }
        else if (!strncmp(argv[2], "-d", 2)) 
        { 
            elOps.check_depth = atoi(argv[2]+2); 
            /* printf("Checking depth (%d)\n", elOps.check_depth); */ 
            elOps.sort_uniq = 1; 
            if (argc >= 4) inp_file = argv[3];
            uniq = xmlHashCreate(0);
            errorno = parse_xml_file(inp_file); 
        }
        else if (argv[2][0] != '-')
        {
            errorno = parse_xml_file(argv[2]);
        }
        else
            elUsage(argc, argv, EXIT_BAD_ARGS);
    }

    if (uniq)
    {
        int i;
        ArrayDest lines;
        lines.array = xmlMalloc(sizeof(xmlChar*) * xmlHashSize(uniq));
        lines.offset = 0;
        xmlHashScan(uniq, hash_key_put, &lines);

        qsort(lines.array, lines.offset, sizeof(xmlChar*), compare_string_ptr);

        for (i = 0; i < lines.offset; i++)
        {
            printf("%s\n", lines.array[i]);
        }

        xmlFree(lines.array);
        xmlHashFree(uniq, NULL);
    }

    return errorno;
}


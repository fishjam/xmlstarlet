/*  $Id: xml_escape.c,v 1.11 2004/11/21 23:40:40 mgrouch Exp $  */

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
#include <ctype.h>

#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/xmlIO.h>
#include <libxml/parserInternals.h>

#include "xmlstar.h"
#include "escape.h"

#define INSZ 4*1024

/**
 *  Print small help for command line options
 */
void
escUsage(int argc, char **argv, int escape, exit_status status)
{
    extern void fprint_escape_usage(FILE* o, const char* argv0);
    extern void fprint_unescape_usage(FILE* o, const char* argv0);
    extern const char more_info[];
    FILE *o = (status == EXIT_SUCCESS)? stdout : stderr;
    if (escape) fprint_escape_usage(o, argv[0]);
    else fprint_unescape_usage(o, argv[0]);
    fprintf(o, "%s", more_info);
    exit(status);
}

/* "apos" or "quot" are biggest, add 1 for leading "&" */
enum { MAX_ENTITY_NAME = 1+4 };

/* return 1 if entity was recognized and value output, 0 otherwise */
static int
put_entity_value(const char* entname, FILE* out)
{
    if (entname[1] == '#') {
        char* endptr;
        const char* entnul = entname + strlen(entname);
        int num = (entname[2] == 'x')?
            strtol(&entname[3], &endptr, 16):
            strtol(&entname[2], &endptr, 10);
        if (endptr == entnul) {
            putc(num, out);
            return 1;
        }
    } else {
        xmlEntityPtr entity = xmlGetPredefinedEntity((xmlChar*) &entname[1]);
        if (entity) {
            fputs((char*) (entity->content), out);
            return 1;
        }
    }
    return 0;
}

/*
 * Macro used to grow the current buffer.
 */
#define grow_BufferReentrant() { 					\
    buffer_size *= 2;   						\
    buffer = (xmlChar *)						\
    		xmlRealloc(buffer, buffer_size * sizeof(xmlChar));	\
    if (buffer == NULL) {       					\
	fprintf(stderr, "growing buffer error");			\
	abort();							\
    }									\
}

/** 
 * xml_C11NNormalizeString:
 * @input: the input string
 * @mode:  the normalization mode (attribute, comment, PI or text)
 *
 * Converts a string to a canonical (normalized) format. The code is stolen
 * from xmlEncodeEntitiesReentrant(). Added normalization of \x09, \x0a, \x0A
 * and the @mode parameter
 *
 * Returns a normalized string (caller is responsible for calling xmlFree())
 * or NULL if an error occurs
 */
xmlChar *
xml_C11NNormalizeString(const xmlChar * input,
                         xml_C14NNormalizationMode mode)
{
    const xmlChar *cur = input;
    xmlChar *buffer = NULL;
    xmlChar *out = NULL;
    int buffer_size = 0;

    if (input == NULL)
        return (NULL);

    /*
     * allocate an translation buffer.
     */
    buffer_size = 1000;
    buffer = (xmlChar *) xmlMallocAtomic(buffer_size * sizeof(xmlChar));
    if (buffer == NULL) {
        fprintf(stderr, "allocating buffer error");
        abort();
    }
    out = buffer;

    while (*cur != '\0') {
        if ((out - buffer) > (buffer_size - 10)) {
            int indx = out - buffer;

            grow_BufferReentrant();
            out = &buffer[indx];
        }

        if ((*cur == '<') && ((mode == XML_C14N_NORMALIZE_ATTR) ||
                              (mode == XML_C14N_NORMALIZE_TEXT))) {
            *out++ = '&';
            *out++ = 'l';
            *out++ = 't';
            *out++ = ';';
        } else if ((*cur == '>') && (mode == XML_C14N_NORMALIZE_TEXT)) {
            *out++ = '&';
            *out++ = 'g';
            *out++ = 't';
            *out++ = ';';
        } else if ((*cur == '&') && ((mode == XML_C14N_NORMALIZE_ATTR) ||
                                     (mode == XML_C14N_NORMALIZE_TEXT))) {
            *out++ = '&';
            *out++ = 'a';
            *out++ = 'm';
            *out++ = 'p';
            *out++ = ';';
        } else if ((*cur == '"') && (mode == XML_C14N_NORMALIZE_ATTR)) {
            *out++ = '&';
            *out++ = 'q';
            *out++ = 'u';
            *out++ = 'o';
            *out++ = 't';
            *out++ = ';';
        } else if ((*cur == '\x09') && (mode == XML_C14N_NORMALIZE_ATTR)) {
            *out++ = '&';
            *out++ = '#';
            *out++ = 'x';
            *out++ = '9';
            *out++ = ';';
        } else if ((*cur == '\x0A') && (mode == XML_C14N_NORMALIZE_ATTR)) {
            *out++ = '&';
            *out++ = '#';
            *out++ = 'x';
            *out++ = 'A';
            *out++ = ';';
        } else if ((*cur == '\x0D') && ((mode == XML_C14N_NORMALIZE_ATTR) ||
                                        (mode == XML_C14N_NORMALIZE_TEXT) ||
                                        (mode == XML_C14N_NORMALIZE_COMMENT) ||
                                        (mode == XML_C14N_NORMALIZE_PI))) {
            *out++ = '&';
            *out++ = '#';
            *out++ = 'x';
            *out++ = 'D';
            *out++ = ';';
        } else {
            /*
             * Works because on UTF-8, all extended sequences cannot
             * result in bytes in the ASCII range.
             */
            *out++ = *cur;
        }
        cur++;
    }
    *out++ = 0;
    return (buffer);
}

/**
 * write unescaped version of @str to @out, if @str appears to end in the middle
 * of entity, return the entity.
 */
static const char *
xml_unescape(const char* str, FILE* out)
{
    static char entity[MAX_ENTITY_NAME+1]; /* +1 for NUL terminator */
    int i;

    for (i = 0; str[i]; i++) {
        if (str[i] == '&') {
            int entity_len;
            int semicolon_off = i+1;
            for (;;) {
                if (str[semicolon_off] == ';' ||
                    str[semicolon_off] == '\0' ||
                    isspace(str[semicolon_off])) break;
                semicolon_off++;
            }
            entity_len = semicolon_off - i;
            if (entity_len < MAX_ENTITY_NAME) {
                memcpy(entity, &str[i], entity_len);
                entity[entity_len] = '\0';
                if (str[semicolon_off] == ';') {
                    if (put_entity_value(entity, out)) {
                        i = semicolon_off;
                        continue;
                    }
                    if (!globalOptions.quiet)
                        fprintf(stderr, "unrecognized entity: %s\n", entity);
                } else if (str[semicolon_off] == '\0') {
                    return entity;
                } else {
                    if (!globalOptions.quiet)
                        fprintf(stderr, "unterminated entity name: %.*s\n", entity_len, &str[i]);
                }
            } else {
                if (!globalOptions.quiet)
                    fprintf(stderr, "entity name too long: %.*s\n", entity_len, &str[i]);
            }
        }
        putc(str[i], out);
    }
    return NULL;
}

/**
 *  This is the main function for 'escape/unescape' options
 */
int
escMain(int argc, char **argv, int escape)
{
    int ret = 0;
    int readStdIn = 0;
        
    char* inp = NULL;
    xmlChar* outBuf = NULL;
    
    if (argc < 2) escUsage(argc, argv, escape, EXIT_BAD_ARGS);

    inp = argv[2];

    if (argc > 2)
    {
        if (!strcmp(argv[2], "--help") || !strcmp(argv[2], "-h") ||
           !strcmp(argv[2], "-?") || !strcmp(argv[2], "-Z"))
            escUsage(argc, argv, escape, EXIT_SUCCESS);
        if (!strcmp(argv[2], "-")) readStdIn = 1;
    }
    else
    {
        readStdIn = 1;        
    }

    if (readStdIn)
    {
       static char line[INSZ];
       int offset = 0;

       while (!feof(stdin))
       {
           if (fgets(line + offset, INSZ - offset, stdin))
           {
               if (escape)
               {
                   outBuf = xmlEncodeEntitiesReentrant(NULL, (xmlChar*) line);
                   if (outBuf)
                   {
                       fprintf(stdout, "%s", outBuf);
                       xmlFree(outBuf);
                   }
               }
               else
               {
                   const char* partial_entity = xml_unescape(line, stdout);
                   if (partial_entity) {
                       offset = strlen(partial_entity);
                       memcpy(line, partial_entity, offset);
                   } else {
                       offset = 0;
                   }
               }
           }
       }
       if (offset) {
           fprintf(stdout, "%.*s", offset, line);
           if (!globalOptions.quiet)
               fprintf(stderr, "partial entity: %.*s\n", offset, line);
       }
       return ret;
    }
    
    if (escape)
    {
        outBuf = xmlEncodeEntitiesReentrant(NULL, (xmlChar*) inp);
        if (outBuf)
        {
            fprintf(stdout, "%s\n", outBuf);
            xmlFree(outBuf);
        }
    }
    else
    {
        const char* partial_entity = xml_unescape(inp, stdout);
        if (partial_entity)
        {
            fprintf(stdout, "%s\n", partial_entity);
            if (!globalOptions.quiet)
                fprintf(stderr, "partial entity: %s\n", partial_entity);
        }
    }
        
    return ret;
}

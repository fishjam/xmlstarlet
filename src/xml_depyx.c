/*  $Id: xml_depyx.c,v 1.8 2005/03/12 03:24:23 mgrouch Exp $  */

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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <libxml/xmlmemory.h>

#include "xmlstar.h"
#include "escape.h"

#define INSZ 4*1024

static void
depyxUsage(int argc, char **argv, exit_status status)
{
    extern void fprint_depyx_usage(FILE* o, const char* argv0);
    extern const char more_info[];
    FILE *o = (status == EXIT_SUCCESS)? stdout : stderr;
    fprint_depyx_usage(o, argv[0]);
    fprintf(o, "%s", more_info);
    exit(status);
}

/**
 *  Decode PYX string
 *
 */
void
pyxDecode(char *str, xml_C14NNormalizationMode mode)
{
   while (*str)
   {
      if ((*str == '\\') && (*(str+1) == 'n'))
      {
         printf("\n");
         str++;
      }
      else if ((*str == '\\') && (*(str+1) == 't'))
      {
         printf("\t");
         str++;
      }
      else if ((*str == '\\') && (*(str+1) == '\\'))
      {
         printf("\\");
         str++;
      }
      else
      {
         if ((*str == '<') && ((mode == XML_C14N_NORMALIZE_ATTR) ||
                              (mode == XML_C14N_NORMALIZE_TEXT))) {
            printf("&lt;");
         } 
         else if ((*str == '>') && (mode == XML_C14N_NORMALIZE_TEXT)) {
            printf("&gt;");
         } 
         else if ((*str == '&') && ((mode == XML_C14N_NORMALIZE_ATTR) ||
                                     (mode == XML_C14N_NORMALIZE_TEXT))) {
            printf("&amp;");
         } 
         else if ((*str == '"') && (mode == XML_C14N_NORMALIZE_ATTR)) {
            printf("&quot;");
         } 
         else {
            printf("%c", *str);
         }
      }
      str++;
   }
}

/**
 *  Decode PYX file
 *
 */
int
pyxDePyx(char *file)
{
   static char line[INSZ];
   FILE *in = stdin;

   if (strcmp(file, "-"))
   {
       in = fopen(file, "r");
       if (in == NULL)
       {
          fprintf(stderr, "error: could not open: %s\n", file);
          exit(EXIT_BAD_FILE);
       }
   }
   
   while (!feof(in))
   {
       if (fgets(line, INSZ - 1, in))
       {
           if(line[strlen(line)-1] == '\n') line[strlen(line)-1] = '\0';

           while (line[0] == '(')
           {
               printf("<%s", line+1);
               if (!feof(in))
               {
                   if (fgets(line, INSZ - 1, in))
                   {
                       if(line[strlen(line)-1] == '\n') line[strlen(line)-1] = '\0';

                       while(line[0] == 'A')  /* attribute */
                       {
                           char *value;

                           printf(" ");
                           value = line+1;
                           while(*value && (*value != ' '))
                           {
                               printf("%c", *value);
                               value++;
                           }
                           if (*value == ' ')
                           {
                               value++;
                               printf("=\"");
                               pyxDecode(value, XML_C14N_NORMALIZE_ATTR);  /* attribute value */
                               printf("\"");
                           }
                           if (!feof(in))
                           {
                               if (fgets(line, INSZ - 1, in))
                               {
                                   if(line[strlen(line)-1] == '\n') line[strlen(line)-1] = '\0';
                               }
                           }
                       }
                       printf(">");
                   }
               }
           }

           if (line[0] == '-')
           {
               /* text */
               pyxDecode(line+1, XML_C14N_NORMALIZE_TEXT);
           }
           else if (line[0] == '?')
           {
               /* processing instruction */
               printf("<?");
               pyxDecode(line+1, XML_C14N_NORMALIZE_TEXT);
               printf("?>");
               printf("\n");  /* is this correct? */
           }
           else if (line[0] == 'D')
           {
               /* processing instruction */
               printf("<!DOCTYPE");
               pyxDecode(line+1, XML_C14N_NORMALIZE_TEXT);
               printf(">");
               printf("\n");  /* is this correct? */
           }
           else if (line[0] == 'C')
           {
               /* comment */
               printf("<!--");
               pyxDecode(line+1, XML_C14N_NORMALIZE_TEXT);
               printf("-->");
               printf("\n");  /* is this correct? */
           }
           else if (line[0] == '[')
           {
               /* CDATA */
               printf("<![CDATA[");
               pyxDecode(line+1, XML_C14N_NORMALIZE_NOTHING);
               printf("]]>");
               printf("\n");  /* is this correct? */
           }
           else if (line[0] == ')')
           {
               printf("</%s>", line+1);
           }
       }
   }

   return EXIT_SUCCESS;
}

/**
 *  Main function for 'de-PYX'
 *
 */
int
depyxMain(int argc, char **argv)
{
   int ret = EXIT_SUCCESS;

   if ((argc >= 3) && (!strcmp(argv[2], "-h") || !strcmp(argv[2], "--help")))
   {
       depyxUsage(argc, argv, EXIT_SUCCESS);
   }
   else if (argc == 3)
   {
       ret = pyxDePyx(argv[2]);
   }
   else if (argc == 2)
   {  
       ret = pyxDePyx("-");
   }
   else
   {
       depyxUsage(argc, argv, EXIT_BAD_ARGS);
   }
   
   printf("\n");

   return ret;
}


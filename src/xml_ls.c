/*  $Id: xml_ls.c,v 1.17 2005/03/19 01:18:02 mgrouch Exp $  */

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

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <libxml/xmlmemory.h>
#include <libxml/c14n.h>

#include "xmlstar.h"
#include "escape.h"

#if !HAVE_LSTAT
# if HAVE_STAT
#  define lstat stat
# else
/* TODO: #ifdef out code that uses stat instead */
#  error "lstat() or stat() required"
# endif
#endif

#ifndef S_ISLNK
# define S_ISLNK(m) 0
#endif

#ifndef S_ISSOCK
# define S_ISSOCK(m) 0
#endif

/**
 *  Print small help for command line options
 */
void
lsUsage(int argc, char **argv, exit_status status)
{
    extern void fprint_ls_usage(FILE* o, const char* argv0);
    extern const char more_info[];
    FILE *o = (status == EXIT_SUCCESS)? stdout : stderr;
    fprint_ls_usage(o, argv[0]);
    fprintf(o, "%s", more_info);
    exit(status);
}


const char *
get_file_type(mode_t mode)
{
    if (S_ISREG(mode)) return "f";       /* regular file */
    else if (S_ISDIR(mode)) return "d";  /* directory */
    else if (S_ISCHR(mode)) return "c";  /* character device */
    else if (S_ISBLK(mode)) return "b";  /* block device */
    else if (S_ISLNK(mode)) return "l";  /* symlink */
    else if (S_ISFIFO(mode)) return "p"; /* fifo */
    else if (S_ISSOCK(mode)) return "s"; /* socket */
    else return "u";                     /* unknown */
}

const char *
get_file_perms(mode_t mode)
{
   int i;
   static char perms[10];

   strcpy(perms, "---------");

   for(i=0; i < sizeof perms - 1; i+=3)
   {
       if(mode &(S_IRUSR>>i))
           perms[i+0] = 'r';

       if(mode &(S_IWUSR>>i))
           perms[i+1] = 'w';

       if(mode &(S_IXUSR>>i))
           perms[i+2] = 'x';
   }

#ifdef S_ISUID
   if((mode & S_ISUID))
       perms[2] = 's';
#endif

#ifdef S_ISGID
   if((mode & S_ISGID))
       perms[5] = 's';
#endif

#ifdef S_ISVTX
   if((mode & S_ISVTX))
       perms[8] = 't';
#endif

   return(perms);
}

int
xml_print_dir(const char* dir)
{
   DIR *dirp;
   struct dirent *d;
   struct stat stats;
   int num_files = 0;

   if((dirp = opendir(dir)) == NULL)
      return(-1);

   chdir(dir);

   while((d = readdir(dirp)) != NULL)
   {
      xmlChar *xml_str;
      char atime[20];
      char mtime[20];
      int size_len;

      if ((d->d_name == NULL) || !strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
          continue;

      if(lstat(d->d_name, &stats) != 0)
      {
        fprintf(stderr, "couldn't stat: %s\n", d->d_name);
      }

#if defined (__MINGW32__)
      /* somehow atime is -1 on Windows XP when the atime is in future */
      if (stats.st_atime < 0) stats.st_atime = 0; 
      /* somehow mtime is -1 on Windows XP when the mtime is in future */
      if (stats.st_mtime < 0) stats.st_mtime = 0; 
#endif

      /* format time as per ISO 8601 */
      strftime(atime, sizeof atime, "%Y%m%dT%H%M%SZ", gmtime(&stats.st_atime));
      strftime(mtime, sizeof mtime, "%Y%m%dT%H%M%SZ", gmtime(&stats.st_mtime));

      xml_str = xml_C11NNormalizeAttr((const xmlChar *) d->d_name);
      printf("<%s p=\"%s\" a=\"%s\" m=\"%s\" s=\"",
          get_file_type(stats.st_mode), get_file_perms(stats.st_mode),
          atime, mtime);
      size_len = printf("%lu", (unsigned long) stats.st_size);
      printf("\"%.*s", 16-size_len, "                ");
      printf(" n=\"%s\"/>\n", xml_str);
      num_files++;
      xmlFree(xml_str);

   } /* end of for loop */

   closedir(dirp);
   return num_files;
}

int
lsMain(int argc, char** argv)
{
    const char *dir = ".";
    int files;

    if (argc == 3) {
        if (strcmp(argv[2], "--help") == 0)
            lsUsage(argc, argv, EXIT_SUCCESS);
        else
            dir = argv[2];
    } else if (argc > 3) {
        lsUsage(argc, argv, EXIT_BAD_ARGS);
    }

    printf("<dir>\n");
    files = xml_print_dir(dir);
    printf("</dir>\n");
    return (files >= 0)? EXIT_SUCCESS : EXIT_FAILURE;
}


/* $Id: xml_pyx.c,v 1.9 2005/03/12 03:24:23 mgrouch Exp $ */

/**
 *  Based on xmln from pyxie project
 *
 *  The PYX format is a line-oriented representation of
 *  XML documents that is derived from the SGML ESIS format.
 *  (see ESIS - ISO 8879 Element Structure Information Set spec,
 *  ISO/IEC JTC1/SC18/WG8 N931 (ESIS))
 *
 *  A non-validating, ESIS generating tool
 *  ESIS Generation by Sean Mc Grath http://www.digitome.com/sean.html
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/parserInternals.h>

#include "xmlstar.h"

/**
 *  Output newline and tab characters as escapes
 *  Required both for attribute values and character data (#PCDATA)
 */
static void
SanitizeData(const xmlChar *s, int len)
{
    while (len--)
    {
        switch (*s)
        {
            case 10:
                printf("\\n");
                break;
            case 13:
                break;
            case 9:
                printf ("\\t");
                break;
            case '\\':
                printf ("\\\\");
                break;
            default:
                putchar (*s);
        }
        s++;
    }
}

static void
print_qname(const xmlChar *prefix, const xmlChar *localname)
{
    if (prefix)
        printf("%s:", prefix);
    printf("%s", localname);
}

int
CompareAttributes(const void *a1,const void *a2)
{
    typedef xmlChar const *const xmlCStr;
    xmlCStr *attr1 = *(xmlCStr* const* const)a1;
    xmlCStr *attr2 = *(xmlCStr* const* const)a2;
    return xmlStrcmp(*attr1, *attr2);
}

void
pyxStartElement (void * ctx,
    const xmlChar * localname,
    const xmlChar * prefix,
    const xmlChar * URI,
    int nb_namespaces,
    const xmlChar ** namespaces,
    int nb_attributes,
    int nb_defaulted,
    const xmlChar ** attributes)
{
    int i;
    /* DON'T modify the attributes array, ever. */
    const xmlChar*** atts = &attributes;

    fprintf(stdout,"(");
    print_qname(prefix, localname);
    fprintf(stdout, "\n");

    if (nb_attributes > 1) {
        atts = calloc(nb_attributes, sizeof(*atts));
        for (i = 0; i < nb_attributes; ++i) {
            atts[i] = &(attributes[i * 5]);
        }
        /* Sort the pairs based on the name part of the pair */
        qsort (atts,
            nb_attributes,
            sizeof(*atts),
            CompareAttributes);
    }

    for (i = 0; i < nb_namespaces; i++) {
        int aidx = i * 2;
        const xmlChar
            *prefix = namespaces[aidx],
            *uri = namespaces[aidx+1];
        /* namespace definitions take the form xmlns:prefix=uri*/
        putchar('A');
        if (xmlStrlen(prefix) > 0)
            print_qname(BAD_CAST "xmlns", prefix);
        else
            fputs("xmlns", stdout);
        putchar(' ');
        SanitizeData(uri, xmlStrlen(uri));
        putchar('\n');
    }

    for (i = 0; i < nb_attributes; i++) {
        const xmlChar *localname = atts[i][0],
            *prefix = atts[i][1],
            /* *nsURI = attributes[atts][2], */
            *valueBegin = atts[i][3],
            *valueEnd = atts[i][4];
        int valueLen = valueEnd - valueBegin;

        /* Attribute Name */
        putchar('A');
        print_qname(prefix, localname);
        putchar(' ');
        /* value - can contain literal "\n" so escape */
        SanitizeData(valueBegin, valueLen);
        putchar('\n');
    }

    /* we did only allocate memory if nb_attributes > 1 */
    if (nb_attributes > 1) free(atts);
}

void
pyxEndElement(void *userData, const xmlChar *localname, const xmlChar *prefix,
    const xmlChar *URI)
{
    fprintf(stdout,")");
    print_qname(prefix, localname);
    putchar('\n');
}

void
pyxCharacterData(void *userData, const xmlChar *s, int len)
{
    fprintf(stdout, "-");
    SanitizeData(s, len);
    putchar('\n');
}

void
pyxProcessingInstruction(void *userData, 
                         const xmlChar *target, 
                         const xmlChar *data)
{
    fprintf(stdout,"?%s ",target);
    SanitizeData(data, xmlStrlen(data));
    fprintf(stdout,"\n");
}

void
pyxUnparsedEntityDeclHandler(void *userData,
                             const xmlChar *entityName,
                             const xmlChar *publicId,
                             const xmlChar *systemId,
                             const xmlChar *notationName)
{
    fprintf(stdout, "U%s %s %s%s%s\n", 
           (char *)entityName, (char *)notationName, (char *)systemId,
           (publicId == NULL? "": " "), 
           (publicId == NULL? "": (char *) publicId));
}

void
pyxNotationDeclHandler(void *userData,
                       const xmlChar *notationName,
                       const xmlChar *publicId,
                       const xmlChar *systemId)
{
    fprintf(stdout, "N%s %s%s%s\n", (char*) notationName, (char*) systemId,
           (publicId == NULL? "": " "), 
           (publicId == NULL? "": (const char*) publicId));
}

void
pyxExternalEntityReferenceHandler(void* userData,
                                  const xmlChar *name)
{
    const xmlChar *p = name;
    fprintf (stdout, "&");
    /* Up to space is the name of the referenced entity */
    while (*p && (*p != ' ')) {
        putchar (*p);
        p++;
    }
}

static void
pyxExternalSubsetHandler(void *ctx ATTRIBUTE_UNUSED, const xmlChar *name,
                         const xmlChar *ExternalID, const xmlChar *SystemID)
{
    fprintf(stdout, "D %s PUBLIC", name); /* TODO: re-check */
    if (ExternalID == NULL)
        fprintf(stdout, " ");
    else
        fprintf(stdout, " \"%s\"", ExternalID);
    if (SystemID == NULL)
        fprintf(stdout, "\n");
    else
        fprintf(stdout, " \"%s\"\n", SystemID);
}

static void
pyxCommentHandler(void *ctx ATTRIBUTE_UNUSED, const xmlChar *value)
{
    fprintf(stdout,"C");
    SanitizeData(value, xmlStrlen(value));
    fprintf(stdout,"\n");
}

static void
pyxCdataBlockHandler(void *ctx ATTRIBUTE_UNUSED, const xmlChar *value, int len)
{
    fprintf(stdout,"[");
    SanitizeData(value, len);
    fprintf(stdout,"\n");
}

static void
pyxUsage(const char *argv0, exit_status status)
{
    extern void fprint_pyx_usage(FILE* o, const char* argv0);
    extern const char more_info[];
    FILE *o = (status == EXIT_SUCCESS)? stdout : stderr;
    fprint_pyx_usage(o, argv0);
    fprintf(o, "%s", more_info);
    exit(status);
}

static xmlSAXHandler pyxSAX;

static int
pyx_process_file(const char *filename)
{
    int ret;
    xmlParserCtxtPtr ctxt;

    ctxt = xmlCreateFileParserCtxt(filename);
    if (!ctxt) /* assume it failed because of filename */
        return EXIT_BAD_FILE;

    ctxt->sax = &pyxSAX;
    ret = xmlParseDocument(ctxt);

    ctxt->sax = NULL; /* don't try to free pyxSAX */
    xmlFreeParserCtxt(ctxt);

    return (ret == 0)? 0 : EXIT_LIB_ERROR;
}

int
pyxMain(int argc,const char *argv[])
{
    int status = 0;

    if ((argc > 2) &&
        (
           (strcmp(argv[2],"-h") == 0) ||
           (strcmp(argv[2],"-H") == 0) ||
           (strcmp(argv[2],"-Z") == 0) ||
           (strcmp(argv[2],"-?") == 0) ||
           (strcmp(argv[2],"--help") == 0)
       ))
    {
        pyxUsage(argv[0], EXIT_SUCCESS);
    }

    xmlInitParser();
    /* Establish Event Handlers */
    pyxSAX.startElementNs = pyxStartElement;
    pyxSAX.endElementNs = pyxEndElement;
    pyxSAX.processingInstruction = pyxProcessingInstruction;
    pyxSAX.characters = pyxCharacterData;
    pyxSAX.notationDecl = pyxNotationDeclHandler;
    pyxSAX.reference = pyxExternalEntityReferenceHandler;
    pyxSAX.unparsedEntityDecl = pyxUnparsedEntityDeclHandler;
    pyxSAX.externalSubset = pyxExternalSubsetHandler;
    pyxSAX.comment = pyxCommentHandler;
    pyxSAX.cdataBlock = pyxCdataBlockHandler;
    pyxSAX.initialized = XML_SAX2_MAGIC;

    if (argc == 2) {
        status = pyx_process_file("-");
    }
    else {
        argv++;
        argc--;
        for (++argv; argc>1; argc--,argv++) {
            int ret = pyx_process_file(*argv);
            if (ret != 0) status = ret;
        }
    }
    xmlCleanupParser();
    return status;
}

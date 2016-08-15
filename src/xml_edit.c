/*  $Id: xml_edit.c,v 1.45 2005/01/08 00:07:03 mgrouch Exp $  */

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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/xmlsave.h>
#include <libxml/HTMLtree.h>
#include <libxml/xinclude.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xpointer.h>
#include <libxml/parserInternals.h>
#include <libxml/uri.h>
#include <libexslt/exslt.h>

#include "xmlstar.h"

/*
   TODO:
          1. Should this be allowed ?
             ./xml ed -m /xml /xml/table/rec/object ../examples/xml/tab-obj.xml 
*/

typedef struct _edOptions {   /* Global 'edit' options */
    int noblanks;             /* Remove insignificant spaces from XML tree */
    int preserveFormat;       /* Preserve original XML formatting */
    int omit_decl;            /* Omit XML declaration line <?xml version="1.0"?> */
    int inplace;              /* Edit file inplace (no output on stdout) */
    int nonet;                /* Disallow network access */
} edOptions;

typedef edOptions *edOptionsPtr;

typedef enum _XmlEdOp {
   XML_ED_DELETE,
   XML_ED_VAR,
   XML_ED_INSERT,
   XML_ED_APPEND,
   XML_ED_UPDATE,
   XML_ED_RENAME,
   XML_ED_MOVE,
   XML_ED_SUBNODE
} XmlEdOp;

/* TODO ??? */
typedef enum _XmlNodeType {
   XML_UNDEFINED,
   XML_ATTR,
   XML_ELEM,
   XML_TEXT,
   XML_COMT,
   XML_CDATA,
   XML_EXPR
} XmlNodeType;

typedef struct {
    char shortOpt;
    const char* longOpt;        /* include "--" */
    XmlNodeType type;
} OptionSpec;

static const OptionSpec
    OPT_VAL_OR_EXP[] = {
        {'x', "--expr", XML_EXPR},
        {'v', "--value", XML_TEXT}
    },
    OPT_JUST_VAL[] = {
        {'v', "--value", XML_TEXT}
    },
    OPT_JUST_TYPE[] = {
        {'t', "--type"}
    },
    OPT_NODE_TYPE[] = {
        {0, "elem", XML_ELEM},
        {0, "attr", XML_ATTR},
        {0, "text", XML_TEXT}
    },
    OPT_JUST_NAME[] = {
        {'n', "--name"}
    };


typedef const char* XmlEdArg;

typedef struct _XmlEdAction {
  XmlEdOp       op;
  XmlEdArg      arg1;
  XmlEdArg      arg2;
  XmlEdArg      arg3;
  XmlNodeType   type;
} XmlEdAction;

/**
 *  display short help message
 */
static void
edUsage(const char *argv0, exit_status status)
{
    extern void fprint_edit_usage(FILE* o, const char* argv0);
    extern const char more_info[];
    FILE *o = (status == EXIT_SUCCESS)? stdout : stderr;
    fprint_edit_usage(o, argv0);
    fprintf(o, "%s", more_info);
    exit(status);
}

/**
 *  Initialize global command line options
 */
static void
edInitOptions(edOptionsPtr ops)
{
    ops->noblanks = 1;
    ops->omit_decl = 0;
    ops->preserveFormat = 0;
    ops->inplace = 0;
    ops->nonet = 1;
}

/**
 *  Parse global command line options
 */
static int
edParseOptions(edOptionsPtr ops, int argc, char **argv)
{
    int i;

    i = 2;
    while((i < argc) && (argv[i][0] == '-'))
    {
        if (!strcmp(argv[i], "-S") || !strcmp(argv[i], "--ps"))
        {
            ops->noblanks = 0;            /* preserve spaces */
        }
        else if (!strcmp(argv[i], "-P") || !strcmp(argv[i], "--pf"))
        {
            ops->preserveFormat = 1;      /* preserve format */
        }
        else if (!strcmp(argv[i], "-O") || !strcmp(argv[i], "--omit-decl"))
        {
            ops->omit_decl = 1;
        }
        else if (!strcmp(argv[i], "-L") || !strcmp(argv[i], "--inplace"))
        {
            ops->inplace = 1;
        }
        else if (!strcmp(argv[i], "--net"))
        {
            ops->nonet = 0;
        }
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h") ||
                 !strcmp(argv[i], "-?") || !strcmp(argv[i], "-Z"))
        {
            edUsage(argv[0], EXIT_SUCCESS);
        }
        else
        {
            break;
        }
        i++;
    }

    return i;
}

/**
 *  register the namespace from @ns_arr to @ctxt
 */
static void
nsarr_xpath_register(xmlXPathContextPtr ctxt)
{
    int ns;
    for (ns = 0; ns_arr[ns]; ns += 2) {
        xmlXPathRegisterNs(ctxt, ns_arr[ns], ns_arr[ns+1]);
    }
}

/**
 * register top-level namespace definitions from @doc to @ctxt
 */
static void
extract_ns_defs(xmlDocPtr doc, xmlXPathContextPtr ctxt)
{
    xmlNsPtr nsDef;
    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) return;

    for (nsDef = root->nsDef; nsDef; nsDef = nsDef->next) {
        if (nsDef->prefix != NULL) { /* can only register ns with prefix */
            xmlXPathRegisterNs(ctxt, nsDef->prefix, nsDef->href);
        } else {
            default_ns = nsDef->href;
        }
    }

    if (default_ns) {
        xmlXPathRegisterNs(ctxt, BAD_CAST "_", default_ns);
        xmlXPathRegisterNs(ctxt, BAD_CAST "DEFAULT", default_ns);
    }
}

static void
update_string(xmlDocPtr doc, xmlNodePtr dest, const xmlChar* newstr)
{
    /* TODO: do we need xmlEncodeEntitiesReentrant() too/instead? */
    xmlChar* string = xmlEncodeSpecialChars(doc, newstr);
    xmlNodeSetContent(dest, string);
    xmlFree(string);
}

/**
 *  'update' operation
 */
static void
edUpdate(xmlDocPtr doc, xmlNodeSetPtr nodes, const char *val,
    XmlNodeType type, xmlXPathContextPtr ctxt)
{
    int i;
    xmlXPathCompExprPtr xpath = NULL;

    if (type == XML_EXPR) {
        xpath = xmlXPathCompile((const xmlChar*) val);
        if (!xpath) return;
    }

    for (i = 0; i < nodes->nodeNr; i++)
    {
        /* update node */
        if (type == XML_EXPR) {
            xmlXPathObjectPtr res;

            ctxt->node = nodes->nodeTab[i];
            res = xmlXPathCompiledEval(xpath, ctxt);
            if (res->type == XPATH_NODESET || res->type == XPATH_XSLT_TREE) {
                int j;
                xmlNodePtr oldChild;
                xmlNodeSetPtr oldChildren = xmlXPathNodeSetCreate(NULL);
                /* NOTE: newChildren can be NULL for empty result set */
                xmlNodeSetPtr newChildren = res->nodesetval;

                /* NOTE: nodes can be both oldChildren and newChildren */

                /* unlink the old children */
                for (oldChild = nodes->nodeTab[i]->children; oldChild; oldChild = oldChild->next) {
                    xmlUnlinkNode(oldChild);
                    /* we can't free it now because an oldChild can also be
                       newChild! just put it in the list */
                    xmlXPathNodeSetAdd(oldChildren, oldChild);
                }

                /* add the new children */
                for (j = 0; newChildren && j < newChildren->nodeNr; j++) {
                    xmlNodePtr node = newChildren->nodeTab[j];
                    xmlAddChild(nodes->nodeTab[i],
                        /* if node is linked to this doc we need to copy */
                        (node->doc == doc)? xmlDocCopyNode(node, doc, 1) : node);
                    newChildren->nodeTab[j] = NULL;
                }
                newChildren->nodeNr = 0;

                /* NOTE: if any oldChildren were newChildren, they've been
                   copied so we can free them all now */
                for (j = 0; j < oldChildren->nodeNr; j++) {
                    xmlFreeNode(oldChildren->nodeTab[j]);
                    oldChildren->nodeTab[j] = NULL;
                }
                oldChildren->nodeNr = 0;
                xmlXPathFreeNodeSet(oldChildren);
            } else {
                res = xmlXPathConvertString(res);
                update_string(doc, nodes->nodeTab[i], res->stringval);
            }
            xmlXPathFreeObject(res);
        } else {
            update_string(doc, nodes->nodeTab[i], (const xmlChar*) val);
        }
    }

    xmlXPathFreeCompExpr(xpath);
}

/* holds the node that was last inserted */
static xmlNodeSetPtr previous_insertion;

/**
 * We must not keep free'd nodes in @previous_insertion.
 * This is a callback from xmlFreeNode()
 */
static void
removeNodeFromPrev(xmlNodePtr node)
{
    xmlXPathNodeSetDel(previous_insertion, node);
}

/**
 *  'insert' operation
 */
static void
edInsert(xmlDocPtr doc, xmlNodeSetPtr nodes, const char *val, const char *name,
         XmlNodeType type, int mode)
{
    int i;

    xmlXPathEmptyNodeSet(previous_insertion);

    for (i = 0; i < nodes->nodeNr; i++)
    {
        xmlNodePtr node;

        if (nodes->nodeTab[i] == (void*) doc && mode != 0) {
            fprintf(stderr, "The document node cannot have siblings.\n");
            exit(EXIT_INTERNAL_ERROR);
        }

        /* update node */
        if (type == XML_ATTR)
        {
            node = (xmlNodePtr) xmlNewProp(nodes->nodeTab[i], BAD_CAST name, BAD_CAST val);
        }
        else if (type == XML_ELEM)
        {
            node = xmlNewDocNode(doc, NULL /* TODO: NS */, BAD_CAST name, BAD_CAST val);
            if (mode > 0)
                xmlAddNextSibling(nodes->nodeTab[i], node);
            else if (mode < 0)
                xmlAddPrevSibling(nodes->nodeTab[i], node);
            else
                xmlAddChild(nodes->nodeTab[i], node);
        }
        else if (type == XML_TEXT)
        {
            node = xmlNewDocText(doc, BAD_CAST val);
            if (mode > 0)
                xmlAddNextSibling(nodes->nodeTab[i], node);
            else if (mode < 0)
                xmlAddPrevSibling(nodes->nodeTab[i], node);
            else
                xmlAddChild(nodes->nodeTab[i], node);
        }
        xmlXPathNodeSetAdd(previous_insertion, node);
    }
}

/**
 *  'rename' operation
 */
static void
edRename(xmlDocPtr doc, xmlNodeSetPtr nodes, const char *val, XmlNodeType type)
{
    int i;
    for (i = 0; i < nodes->nodeNr; i++)
    {
        if (nodes->nodeTab[i] == (void*) doc) {
            fprintf(stderr, "The document node cannot be renamed.\n");
            exit(EXIT_INTERNAL_ERROR);
        }
        xmlNodeSetName(nodes->nodeTab[i], BAD_CAST val);
    }
}

/**
 *  'delete' operation
 */
static void
edDelete(xmlDocPtr doc, xmlNodeSetPtr nodes)
{
    int i;
    for (i = nodes->nodeNr - 1; i >= 0; i--)
    {
        if (nodes->nodeTab[i] == (void*) doc) {
            fprintf(stderr, "The document node cannot be deleted.\n");
            exit(EXIT_INTERNAL_ERROR);
        }

        if (nodes->nodeTab[i]->type == XML_NAMESPACE_DECL) {
            fprintf(stderr, "FIXME: can't delete namespace nodes\n");
            exit(EXIT_INTERNAL_ERROR);
        }
        /* delete node */
        xmlUnlinkNode(nodes->nodeTab[i]);

        /* Free node and children */
        xmlFreeNode(nodes->nodeTab[i]);
        nodes->nodeTab[i] = NULL;
    }
}

/**
 *  'move' operation
 */
static void
edMove(xmlDocPtr doc, xmlNodeSetPtr nodes, xmlNodePtr to)
{
    int i;
    for (i = 0; i < nodes->nodeNr; i++)
    {
        if (nodes->nodeTab[i] == (void*) doc) {
            fprintf(stderr, "The document node cannot be moved.\n");
            exit(EXIT_INTERNAL_ERROR);
        }

        if (nodes->nodeTab[i]->type == XML_NAMESPACE_DECL) {
            fprintf(stderr, "FIXME: can't move namespace nodes\n");
            exit(EXIT_INTERNAL_ERROR);
        }
        /* move node */
        xmlUnlinkNode(nodes->nodeTab[i]);
        xmlAddChild(to, nodes->nodeTab[i]);
    }
}

/**
 *  Loop through array of operations and perform them
 */
static void
edProcess(xmlDocPtr doc, const XmlEdAction* ops, int ops_count)
{
    int k;
    xmlXPathContextPtr ctxt = xmlXPathNewContext(doc);
    /* NOTE: later registrations override earlier ones */
    registerXstarNs(ctxt);

    /* variables */
    previous_insertion = xmlXPathNodeSetCreate(NULL);
    registerXstarVariable(ctxt, "prev",
        xmlXPathWrapNodeSet(previous_insertion));
    xmlDeregisterNodeDefault(&removeNodeFromPrev);

#if HAVE_EXSLT_XPATH_REGISTER
    /* register extension functions */
    exsltDateXpathCtxtRegister(ctxt, BAD_CAST "date");
    exsltMathXpathCtxtRegister(ctxt, BAD_CAST "math");
    exsltSetsXpathCtxtRegister(ctxt, BAD_CAST "set");
    exsltStrXpathCtxtRegister(ctxt, BAD_CAST "str");
#endif
    /* namespaces from doc */
    if (globalOptions.doc_namespace)
        extract_ns_defs(doc, ctxt);
    /* namespaces from command line */
    nsarr_xpath_register(ctxt);

    for (k = 0; k < ops_count; k++)
    {
        xmlXPathObjectPtr res;
        xmlNodeSetPtr nodes;

        /* NOTE: to make relative paths match as if from "/", set context to
           document; setting to root would match as if from "/node()/" */
        ctxt->node = (xmlNodePtr) doc;

        if (ops[k].op == XML_ED_VAR) {
            res = xmlXPathEvalExpression(BAD_CAST ops[k].arg2, ctxt);
            xmlXPathRegisterVariable(ctxt, BAD_CAST ops[k].arg1, res);
            continue;
        }

        res = xmlXPathEvalExpression(BAD_CAST ops[k].arg1, ctxt);
        if (!res || res->type != XPATH_NODESET || !res->nodesetval) continue;
        nodes = res->nodesetval;

        switch (ops[k].op)
        {
            case XML_ED_DELETE:
                edDelete(doc, nodes);
                break;
            case XML_ED_MOVE: {
                xmlXPathObjectPtr res_to;
                ctxt->node = (xmlNodePtr) doc;
                res_to = xmlXPathEvalExpression(BAD_CAST ops[k].arg2, ctxt);
                if (!res_to
                    || res_to->type != XPATH_NODESET
                    || res_to->nodesetval->nodeNr != 1) {
                    fprintf(stderr, "move destination is not a single node\n");
                    continue;
                }
                edMove(doc, nodes, res_to->nodesetval->nodeTab[0]);
                xmlXPathFreeObject(res_to);
                break;
            }
            case XML_ED_UPDATE:
                edUpdate(doc, nodes, ops[k].arg2, ops[k].type, ctxt);
                break;
            case XML_ED_RENAME:
                edRename(doc, nodes, ops[k].arg2, ops[k].type);
                break;
            case XML_ED_INSERT:
                edInsert(doc, nodes, ops[k].arg2, ops[k].arg3, ops[k].type, -1);
                break;
            case XML_ED_APPEND:
                edInsert(doc, nodes, ops[k].arg2, ops[k].arg3, ops[k].type, 1);
                break;
            case XML_ED_SUBNODE:
                edInsert(doc, nodes, ops[k].arg2, ops[k].arg3, ops[k].type, 0);
                break;
            default:
                break;
        }
        xmlXPathFreeObject(res);
    }
    /* NOTE: free()ing ctxt also free()s previous_insertion */
    previous_insertion = NULL;
    xmlDeregisterNodeDefault(NULL);

    xmlXPathFreeContext(ctxt);
}

/**
 *  Output document
 */
static void
edOutput(const char* filename, const XmlEdAction* ops, int ops_count,
    const edOptions* g_ops)
{
    xmlDocPtr doc;
    int save_options =
#if LIBXML_VERSION >= 20708
        (g_ops->noblanks? 0 : XML_SAVE_WSNONSIG) |
#endif
        (g_ops->preserveFormat? 0 : XML_SAVE_FORMAT) |
        (g_ops->omit_decl? XML_SAVE_NO_DECL : 0);
    int read_options =
        (g_ops->nonet? XML_PARSE_NONET : 0);
    xmlSaveCtxtPtr save;

    doc = xmlReadFile(filename, NULL, read_options);
    if (!doc)
    {
        cleanupNSArr(ns_arr);
        xmlCleanupParser();
        xmlCleanupGlobals();
        exit(EXIT_BAD_FILE);
    }

    edProcess(doc, ops, ops_count);

    /* avoid getting ASCII CRs in UTF-16/UCS-(2,4) text */
    if ((xmlStrcasestr(doc->encoding, BAD_CAST "UTF") == 0
            && xmlStrcasestr(doc->encoding, BAD_CAST "16") == 0)
        ||
        (xmlStrcasestr(doc->encoding, BAD_CAST "UCS") == 0
            && (xmlStrcasestr(doc->encoding, BAD_CAST "2") == 0
                ||
                xmlStrcasestr(doc->encoding, BAD_CAST "4") == 0)))
    {
        set_stdout_binary();
    }

    save = xmlSaveToFilename(g_ops->inplace? filename : "-", NULL, save_options);
    xmlSaveDoc(save, doc);
    xmlSaveClose(save);
    xmlFreeDoc(doc);
}

/**
 * get next command line arg, or print error exit and exit if there isn't one
 * @returns pointer to the arg
 * @argi is incremented
 */
static const char*
nextArg(char *const*const argv, int *argi)
{
    const char *arg = argv[*argi];
    if (arg == NULL)
    {
        edUsage(argv[0], EXIT_BAD_ARGS);
    }
    *argi += 1;
    return arg;
}

/**
 * like nextArg(), but additionally look for next arg in @choices
 */
static XmlNodeType
parseNextArg(char *const*const argv, int *argi,
    const OptionSpec choices[], int choices_count)
{
    const char* arg = nextArg(argv, argi);
    int i;
    for (i = 0; i < choices_count; i++) {
        if ((arg[0] == '-' && arg[1] == choices[i].shortOpt) ||
            (strcmp(arg, choices[i].longOpt) == 0))
            return choices[i].type;
    }
    edUsage(argv[0], EXIT_BAD_ARGS);
    return 0;                   /* never reach here */
}
#define parseNextArg(argv, argi, choices) \
    parseNextArg(argv, argi, choices, COUNT_OF(choices))


/** --insert, --append, and --subnode all take the same arguments */
static void
parseInsertionArgs(XmlEdOp op_type, XmlEdAction* op,
    char *const*const argv, int *argi)
{
    XmlEdArg arg;
    op->op = op_type;
    op->arg1 = nextArg(argv, argi);
    parseNextArg(argv, argi, OPT_JUST_TYPE);
    op->type = parseNextArg(argv, argi, OPT_NODE_TYPE);
    parseNextArg(argv, argi, OPT_JUST_NAME);
    op->arg3 = nextArg(argv, argi);
    /* test if value is given */
    op->arg2 = 0;
    arg = argv[*argi];
    if (!arg) return;
    if (!strcmp(arg, "-v") || !strcmp(arg, "--value")) {
        parseNextArg(argv, argi, OPT_JUST_VAL);
        op->arg2 = nextArg(argv, argi);
    }
}

/**
 *  This is the main function for 'edit' option
 */
int
edMain(int argc, char **argv)
{
    int i, ops_count, max_ops_count = 8, n, start = 0;
    XmlEdAction* ops = xmlMalloc(sizeof(XmlEdAction) * max_ops_count);
    static edOptions g_ops;
    int nCount = 0;

    if (argc < 3) edUsage(argv[0], EXIT_BAD_ARGS);

    edInitOptions(&g_ops);
    start = edParseOptions(&g_ops, argc, argv);

    parseNSArr(ns_arr, &nCount, argc-start, argv+start);
        
    /*
     *  Parse command line and fill array of operations
     */
    ops_count = 0;
    i = start + nCount;

    while (i < argc)
    {
        const char *arg = nextArg(argv, &i);
        if (arg[0] == '-')
        {
            if (ops_count >= max_ops_count)
            {
                max_ops_count *= 2;
                ops = xmlRealloc(ops, sizeof(XmlEdAction) * max_ops_count);
            }
            ops[ops_count].type = XML_UNDEFINED;

            if (!strcmp(arg, "-d") || !strcmp(arg, "--delete"))
            {
                ops[ops_count].op = XML_ED_DELETE;
                ops[ops_count].arg1 = nextArg(argv, &i);
                ops[ops_count].arg2 = 0;
            }
            else if (!strcmp(arg, "--var"))
            {
                ops[ops_count].op = XML_ED_VAR;
                ops[ops_count].arg1 = nextArg(argv, &i);
                ops[ops_count].arg2 = nextArg(argv, &i);
            }
            else if (!strcmp(arg, "-m") || !strcmp(arg, "--move"))
            {
                ops[ops_count].op = XML_ED_MOVE;
                ops[ops_count].arg1 = nextArg(argv, &i);
                ops[ops_count].arg2 = nextArg(argv, &i);
            }
            else if (!strcmp(arg, "-u") || !strcmp(arg, "--update"))
            {
                ops[ops_count].op = XML_ED_UPDATE;
                ops[ops_count].arg1 = nextArg(argv, &i);
                ops[ops_count].type = parseNextArg(argv, &i, OPT_VAL_OR_EXP);
                ops[ops_count].arg2 = nextArg(argv, &i);
            }
            else if (!strcmp(arg, "-r") || !strcmp(arg, "--rename"))
            {
                ops[ops_count].op = XML_ED_RENAME;
                ops[ops_count].arg1 = nextArg(argv, &i);
                ops[ops_count].type = parseNextArg(argv, &i, OPT_JUST_VAL);
                ops[ops_count].arg2 = nextArg(argv, &i);
            }
            else if (!strcmp(arg, "-i") || !strcmp(arg, "--insert"))
            {
                parseInsertionArgs(XML_ED_INSERT, &ops[ops_count], argv, &i);
            }
            else if (!strcmp(arg, "-a") || !strcmp(arg, "--append"))
            {
                parseInsertionArgs(XML_ED_APPEND, &ops[ops_count], argv, &i);
            }
            else if (!strcmp(arg, "-s") || !strcmp(arg, "--subnode"))
            {
                parseInsertionArgs(XML_ED_SUBNODE, &ops[ops_count], argv, &i);
            }
            else
            {
                fprintf(stderr, "Warning: unrecognized option '%s'\n", arg);
            }
            ops_count++;
        }
        else
        {
            i--;                /* it was a filename, we didn't use it */
            break;
        }
    }

    xmlKeepBlanksDefault(0);

    if ((!g_ops.noblanks) || g_ops.preserveFormat) xmlKeepBlanksDefault(1);

    if (i >= argc)
    {
        edOutput("-", ops, ops_count, &g_ops);
    }
    
    for (n=i; n<argc; n++)
    {
        edOutput(argv[n], ops, ops_count, &g_ops);
    }

    xmlFree(ops);
    cleanupNSArr(ns_arr);
    xmlCleanupParser();
    xmlCleanupGlobals();
    return 0;
}

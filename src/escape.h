/*  $Id: escape.h,v 1.2 2005/03/12 03:24:23 mgrouch Exp $  */

#ifndef __ESCAPE_H
#define __ESCAPE_H

/*

XMLStarlet: Command Line Toolkit to query/edit/check/transform XML documents

Copyright (c) 2004 Mikhail Grushinskiy.  All Rights Reserved.

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

typedef enum {
    XML_C14N_NORMALIZE_ATTR = 0,
    XML_C14N_NORMALIZE_COMMENT = 1,
    XML_C14N_NORMALIZE_PI = 2,
    XML_C14N_NORMALIZE_TEXT = 3,
    XML_C14N_NORMALIZE_NOTHING = 4
} xml_C14NNormalizationMode;

extern xmlChar *xml_C11NNormalizeString(const xmlChar * input,
                                 xml_C14NNormalizationMode mode);
                                       
#define  xml_C11NNormalizeAttr( a ) \
    xml_C11NNormalizeString((a), XML_C14N_NORMALIZE_ATTR)
#define  xml_C11NNormalizeComment( a ) \
    xml_C11NNormalizeString((a), XML_C14N_NORMALIZE_COMMENT)
#define  xml_C11NNormalizePI( a )	\
    xml_C11NNormalizeString((a), XML_C14N_NORMALIZE_PI)
#define  xml_C11NNormalizeText( a ) \
    xml_C11NNormalizeString((a), XML_C14N_NORMALIZE_TEXT)

#endif /* __ESCAPE_H */

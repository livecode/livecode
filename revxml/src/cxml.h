/* Copyright (C) 2003-2015 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/HTMLparser.h>
// MDW-2013-07-09: [[ RevXmlXPath ]]
#include <libxml/xpath.h>
#include <libxslt/documents.h>

#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


#if  defined MACOS || defined X11
#define _vsnprintf vsnprintf
#define _snprintf snprintf
#endif

#ifndef Bool
#define Bool int
#define True 1
#define False 0
#endif

#define BUFSIZEINC 1024

static char XMLNULLSTRING[] = "";

class CXMLElement;
class CXMLAttribute;

//xmldoc.cpp
int util_strnicmp(const char *s1, const char *s2, int n);
int util_strncmp(const char *s1, const char *s2, int n);
void util_concatstring(const char *s, int slen, char *&d, int &dlen, int &dalloc);
const char *util_strchr(const char *sptr, char target, int l);

extern void CB_startDocument();
extern void CB_endDocument();
extern void CB_startElement(const char *name, const char **atts);
extern void CB_endElement(const char *name);
extern void CB_elementData(const char *data, int length);

class CXMLDocument
{
public:
CXMLDocument() {Init();}
// MDW-2013-09-03: [[ RevXmlXslt ]] new constructors
CXMLDocument(xmlXPathContextPtr p_id) {Init(); xpathContext = p_id;}
CXMLDocument(xsltStylesheetPtr p_id) {Init(); xsltID = p_id;}
~CXMLDocument() {Free();}
inline Bool isinited() {return doc != NULL;}
Bool Read(char *data, unsigned long tlength, Bool wellformed);
Bool ReadFile(char *filename,  Bool wellformed);
Bool Validate();
void CopyDocument(CXMLDocument *tdocument,Bool truecopy = False);
void New();
unsigned int GetID() {return id;}
void Write(char **data,  int *length,Bool isformatted);
Bool GetElementByPath(CXMLElement *telement, char *tpath);
Bool GetRootElement(CXMLElement *telement);
xmlDocPtr GetDocPtr() {return doc;}
// MDW-2013-07-09: [[ RevXmlXPath ]]
xmlXPathContextPtr GetXPathContext() {return xpathContext;}
// MDW-2013-09-03: [[ RevXmlXslt ]]
xsltStylesheetPtr GetXsltContext() {return xsltID;}
Bool AddDTD(char *data, unsigned long tlength);
Bool ValidateDTD(char *data, unsigned long tlength);
char *GetError() {return errorbuf;}
static Bool buildtree;
static Bool allowcallbacks;
protected:
void Free();
static void warningCallback(void *ctx, const char *msg, ...);
static void errorCallback(void *ctx, const char *msg, ...);
static void fatalCallback(void *ctx, const char *msg, ...);
static void startDocumentCallback(void *ctx);
static void endDocumentCallback(void *ctx);
static void startElementCallback(void *ctx,const xmlChar *fullname,const xmlChar **atts);
static void endElementCallback(void *ctx,const xmlChar *name);
static void elementDataCallback(void *ctx,const xmlChar *ch,int len);
static void elementCDataCallback(void *ctx,const xmlChar *ch,int len);
static xmlSAXHandler SAXHandlerTable;
static unsigned int idcounter;
unsigned int id;
static char errorbuf[256];
xmlDocPtr doc;
private:
void Init() {id = ++idcounter; doc = NULL; xpathContext = NULL; xsltID = NULL;}
// MDW-2013-07-09: [[ RevXmlXPath ]]
xmlXPathContextPtr xpathContext;
// MDW-2013-09-03: [[ RevXmlXslt ]]
xsltStylesheetPtr xsltID;
};

class CXMLElement
{
public:
CXMLElement() {element = NULL;}
~CXMLElement() {}

inline Bool isinited() {return element != NULL;}
xmlNodePtr GetNodePtr() {return element;}
bool IsTextNode() {return element -> type == XML_TEXT_NODE || element -> type == XML_CDATA_SECTION_NODE;}
void SetNodePtr(xmlNodePtr tnode) {element = tnode;}
char *GetAttributeValue(char *attname, Bool isbuffered = True);
Bool SetAttribute(char *attname,char *tdata);
Bool RemoveAttribute(char *attname);
void Remove();
void Write(char **data,int *tlength, Bool isformatted);
Bool AddChild(char *tname, char *tcontent,CXMLElement *telement, bool before);
Bool AddSibling(char *tname, char *tcontent, CXMLElement *telement, bool before);
char *GetName();
int GetIndex();
char *GetPath();
Bool GetFirstAttribute(CXMLAttribute *tattribute);
Bool GetNamespace(xmlNs *&r_namespace);
Bool GoChildByPath(char *tpath);
int AttributeCount(char *attributename);
int ChildCount(char *childname, int maxdepth);
void CopyElement(CXMLElement *telement,Bool truecopy = False);
void AddElement(CXMLElement *telement);
bool MoveElement(CXMLElement *tsrcelement, bool p_sibling, bool p_before);
// MW-2014-06-12: [[ Bug 12628 ]] Uses appropriate libxml calls to move srclement from another document
bool MoveRemoteElement(CXMLElement *tsrcelement, bool p_sibling, bool p_before);
char *GetContent(Bool isbuffered = True);
void SetContent(char *tdata);
Bool GoChild(char *ename, bool inc_text = false);
Bool GoNext(char *ename, bool inc_text = false);
Bool GoPrev(char *ename, bool inc_text = false);
Bool GoParent();
protected:
	xmlNodePtr element;
};

class CXMLAttribute
{
public:
	CXMLAttribute() {attribute = NULL;}
	~CXMLAttribute() {}
	inline Bool isinited() {return attribute != NULL;}
	char *GetName();
	void SetAttrPtr(xmlAttrPtr tattribute) {attribute = tattribute;} 
	char *GetContent();
	Bool GoNext();
	Bool GoPrev();
protected:
	xmlAttrPtr attribute;
};




class CXMLElementEnumerator
{
public:
	CXMLElementEnumerator(CXMLElement *startat,int tmaxdepth);
	CXMLElement *GetElement() {return &element;}
	Bool Next(char *childname, bool inc_text = false);
	int GetDepth() {return depth;}
protected:
	CXMLElement element;
	int maxdepth;
	int depth; 
};

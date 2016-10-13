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

#include "cxml.h"
#if defined _MACOSX || defined ( _LINUX ) || defined (TARGET_SUBPLATFORM_IPHONE) || defined (TARGET_SUBPLATFORM_ANDROID)
#define _vsnprintf vsnprintf
#endif

//---------------------------------------GENERAL UTILITY FUNCTIONS------------------------------------


#define WHOLEMATCHES 1

int util_strnicmp(const char *one, const char *two, int n)
{
  const char *optr = (const char *)one;
  const char *tptr = (const char *)two;
  while (n--) {
    if (*optr != *tptr) {
      char o = tolower(*optr);
      char t = tolower(*tptr);
      if (o != t || o == '\0' || t == '\0')
	return o - t;
    }
    optr++;
    tptr++;
  }
  if (!WHOLEMATCHES || !*optr)
  return 0;
  return 1;
}

int util_strncmp(const char *one, const char *two, int n)
{
  const char *optr = (const char *)one;
  const char *tptr = (const char *)two;
  while (n--) {
    if (*optr != *tptr) {
      char o = *optr;
      char t = *tptr;
      if (o != t || o == '\0' || t == '\0')
	return o - t;
    }
    optr++;
    tptr++;
  }
  if (!WHOLEMATCHES || !*optr)
  return 0;
  return 1;
}

const char *util_strchr(const char *sptr, char target, int l)
{
  if (!l) l = strlen(sptr);
  const char *eptr = sptr + l;
  while (sptr < eptr) {
    if (*sptr == target) 
      return sptr;
    sptr++;
  }
  return NULL;
}

//utility function used to concat two strings..and reallocate string if neccessary
void util_concatstring(const char *s, int slen, char *&d, int &dlen, int &dalloc)
{
	int newbufsize = dalloc;
	while (dlen + slen + 1 > newbufsize)
		newbufsize += BUFSIZEINC;
	if (newbufsize != dalloc){
		dalloc = newbufsize;
		d = (char *)realloc(d, dalloc);
	}
	memcpy(d + dlen, s, slen);
	dlen += slen;


}


//----------------------------------CXMLDOCUMENT STATIC MEMBERS AND METHODS-----------------------------
unsigned int CXMLDocument::idcounter = 0;
Bool CXMLDocument::buildtree = True;
Bool CXMLDocument::allowcallbacks = False;
char CXMLDocument::errorbuf[256] = "";

//lists of callbacks - used to establish custom callbacks for parse errors, other callbacks in future
xmlSAXHandler CXMLDocument::SAXHandlerTable = {
    internalSubset,
    isStandalone,
    hasInternalSubset,
    hasExternalSubset,
    resolveEntity,
    getEntity,
    entityDecl,
    notationDecl,
    attributeDecl,
    elementDecl,
    unparsedEntityDecl,
    setDocumentLocator,
    startDocumentCallback,//startDocument
    endDocumentCallback,//endDocument
    startElementCallback,//startElement
    endElementCallback,//endElement
    reference,
	elementDataCallback, //characters,
    ignorableWhitespace,
    processingInstruction,
    comment,
    warningCallback,
    errorCallback,
    fatalCallback,
    getParameterEntity,
    elementCDataCallback,//cdataBlock,
    externalSubset,
	1,
    NULL,
    NULL,
    NULL,
    NULL,
};

void CXMLDocument::startDocumentCallback(void *ctx)
{
	if (allowcallbacks)
		CB_startDocument();
	startDocument(ctx);
}

void CXMLDocument::endDocumentCallback(void *ctx)
{
	if (allowcallbacks)
		CB_endDocument();
	endDocument(ctx);
}

void CXMLDocument::startElementCallback(void *ctx,
										const xmlChar *fullname,
										const xmlChar **atts)
{
    extern Bool XML_ProcessNameSpaces;
    if (allowcallbacks)
		CB_startElement((const char *)fullname,(const char **)atts);
	if (buildtree)
    {
        //HS-2010-10-11: [[ Bug 7586 ]] Reinstate libxml2 to create name spaces. Implement new liveCode commands to suppress name space creation.
        if (XML_ProcessNameSpaces)
        {
		    startElement(ctx,fullname,atts);
        }
        else
        {
            xmlSAX2StartElementNoNS(ctx,fullname,atts);
        }
    }
}

void CXMLDocument::endElementCallback(void *ctx,
									  const xmlChar *name)
{
	if (allowcallbacks)
		CB_endElement((const char *)name);
	if (buildtree)
		endElement(ctx,name);
}


void CXMLDocument::elementCDataCallback(void *ctx,const xmlChar *ch,int len)
{
	if (allowcallbacks)
		CB_elementData((const char *)ch,len);
	if (buildtree)
		cdataBlock(ctx,ch,len);
}


void CXMLDocument::elementDataCallback(void *ctx,const xmlChar *ch,int len)
{
	if (allowcallbacks)
		CB_elementData((const char *)ch,len);
	if (buildtree)
		characters(ctx,ch,len);
}
 

void CXMLDocument::warningCallback(void *ctx, const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    _vsnprintf(errorbuf, 255, msg, args);
    va_end(args);
}

void CXMLDocument::errorCallback(void *ctx, const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    _vsnprintf(errorbuf, 255, msg, args);
    va_end(args);
}

void CXMLDocument::fatalCallback(void *ctx, const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
	_vsnprintf(errorbuf, 255, msg, args);
    va_end(args);
}


//--------------------------------CXMLDOCUMENT MEMBER FUNCTIONS--------------------------------

/* New - creates new xml document. */
void CXMLDocument::New()
{
	Free(); //free document
	doc = xmlNewDoc((xmlChar *) "1.0");
}

/*Read - parse xml data and create xml tree
data - points to xml data to parse
tlength - length of xml data to parse
returns false on parse error
*/
Bool CXMLDocument::Read(char *data, unsigned long tlength, Bool wellformed)
{
	xmlKeepBlanksDefault(0);
	Free(); //free document	
	
	// MW-2014-03-10: [[ Bug 11903 ]] Use modified libXML functions that allow us to
	//   pass through XML_PARSE_HUGE (so the new 2.9 limits don't apply!).
	int options;
	options = XML_PARSE_HUGE;
	if (!wellformed)
		options |= XML_PARSE_RECOVER;
	
	doc =  xmlSAXParseMemoryWithDataAndOptions(&SAXHandlerTable, data, tlength, options, NULL);

	// OK-2007-12-17 : Bug 5632. If Validate() is called in this context it crashes
	// For now we just remove the validation step from here as it proved difficult to
	// find a way to make it work.
	//if (doc && wellformed && !Validate())
	//if (doc && wellformed)
	//	Free();

	return doc != NULL;
}

Bool CXMLDocument::ReadFile(char *filename,  Bool wellformed)
{
	xmlKeepBlanksDefault(0);
	Free(); //free document	
	
	// MW-2014-03-10: [[ Bug 11903 ]] Use modified libXML functions that allow us to
	//   pass through XML_PARSE_HUGE (so the new 2.9 limits don't apply!).
	int options;
	options = XML_PARSE_HUGE;
	if (!wellformed)
		options |= XML_PARSE_RECOVER;
		
	doc =  xmlSAXParseFileWithDataAndOptions(&SAXHandlerTable, filename, options, NULL);

	// OK-2007-12-17 : Bug 5632. If Validate() is called in this context it crashes
	// For now we just remove the validation step from here as it proved difficult to
	// find a way to make it work.
	//if (doc && wellformed && !Validate())
	//	Free();

	return doc != NULL;
}

Bool CXMLDocument::Validate()
{
if (!isinited()) return False;
		xmlValidCtxt ctxt;
		ctxt.doc = doc;
		ctxt.userData = NULL;
		//point to dtd error handling function
		ctxt.error = errorCallback; 
		ctxt.warning = warningCallback;
		if ((doc->intSubset == NULL) && (doc->extSubset == NULL))
			return True;
		return xmlValidateDocument(&ctxt, doc) == 1;
}

Bool CXMLDocument::AddDTD(char *data, unsigned long tlength)
{
	if (!isinited()) return False;
	xmlParserInputBufferPtr dtdInputBufferPtr; 
	xmlDtdPtr dtd; 
	dtdInputBufferPtr = xmlParserInputBufferCreateMem(data, tlength, XML_CHAR_ENCODING_UTF8); 
	dtd = xmlIOParseDTD(NULL, dtdInputBufferPtr, XML_CHAR_ENCODING_UTF8); 
	if (!dtd) return False;
	if (dtd->name != NULL)
		xmlFree((char*)dtd->name);
	CXMLElement telement;
	GetRootElement(&telement);
	dtd->name = xmlStrdup((xmlChar *)telement.GetName());
	doc->intSubset = dtd;
    if (dtd->ExternalID != NULL) { 
           xmlFree((xmlChar *) dtd->ExternalID); 
           dtd->ExternalID = NULL; 
       } 
       if (dtd->SystemID != NULL) { 
           xmlFree((xmlChar *) dtd->SystemID); 
           dtd->SystemID = NULL; 
       } 
	dtd->doc = doc;
	dtd->parent = doc;
	if (doc->children == NULL) xmlAddChild((xmlNodePtr)doc, (xmlNodePtr)dtd);
	else xmlAddPrevSibling(doc->children, (xmlNodePtr)dtd);
	return Validate();
}

Bool CXMLDocument::ValidateDTD(char *data, unsigned long tlength)
{
	if (!isinited()) return False;
	xmlParserInputBufferPtr dtdInputBufferPtr; 
	xmlDtdPtr dtd; 
	dtdInputBufferPtr = xmlParserInputBufferCreateMem(data, tlength, XML_CHAR_ENCODING_UTF8); 
	dtd = xmlIOParseDTD(NULL, dtdInputBufferPtr, XML_CHAR_ENCODING_UTF8); 
	if (!dtd) return False;
	xmlValidCtxt ctxt;
	ctxt.doc = doc;
	ctxt.userData = NULL;
	//point to dtd error handling function
	ctxt.error = errorCallback; 
	ctxt.warning = warningCallback;
	Bool isvalid = xmlValidateDtd(&ctxt,doc,dtd);
	xmlFreeDtd(dtd);
	return isvalid;
}
	
/*Write - writes xml tree to buffer
output: 
data - is set to point to buffer containing xml data
tlength - is set to length of xml data
*/
void CXMLDocument::Write(char **data,int *tlength,Bool isformatted)
{
	if (!isinited()) return;
	xmlDocDumpFormatMemory(doc,(unsigned char**)data,tlength,isformatted);
}

/*Free - frees xml document*/
void CXMLDocument::Free()
{
	if (!isinited()) return;
	xmlFreeDoc(doc);
	doc = NULL;
	if (NULL != xpathContext)
		xmlXPathFreeContext(xpathContext);
	xpathContext = NULL;
	// MDW-2013-09-04: [[ RevXmlXslt ]]
	if (NULL != xsltID)
		xsltFreeStylesheet(xsltID);
	xsltID = NULL;
}

/*CopyDocument - copies xml tree of other CXMLDocument
truecopy - if true this is set to point to xmltree of tdocument, 
otherwise this is set to point to a copy of xmltree in tdocument
output: tdocument - xml document to copy from
*/
void CXMLDocument::CopyDocument(CXMLDocument *tdocument,Bool truecopy)
{
	if (!tdocument->isinited()) return;
	Free();
	if (truecopy)
		doc = xmlCopyDoc(tdocument->GetDocPtr(),0);
	else
		doc = tdocument->GetDocPtr();
}

/*GetRootElement
telement - is set to point to root element, returns false on error
returns False on error
*/
Bool CXMLDocument::GetRootElement(CXMLElement *telement)
{
	if (!isinited()) return False;
	xmlNodePtr rootelement = xmlDocGetRootElement(doc);
	if (rootelement)
		telement->SetNodePtr(rootelement);
	return rootelement != NULL;
}

/*GetElementByPath
tpath - path of element in xml tree (ie. /rootelement/parentelement[1]/childelement)
telement - on success, output is set to point to element specified by tpath
return True if element found
*/
Bool CXMLDocument::GetElementByPath(CXMLElement *telement, char *tpath)
{
	if (!isinited()) return False;
	char *sptr = tpath;
	Bool isroot = False;

	if (!GetRootElement(telement))
		return False;

	if (*sptr == '/') sptr++;
	const char *nameend;
	char *nextname = strchr(sptr, '/');
	if (!nextname) 
	{
		if (!*sptr)
			return True;
		nextname = sptr+strlen(sptr);
		isroot = True;
		
	}
	const char *numpointer = util_strchr(sptr,'[',nextname-sptr);
	if (numpointer)
			nameend = numpointer;
	else
			nameend = nextname;
	if (!util_strnicmp(telement->GetName(),sptr,nameend-sptr))
    {
		if (isroot)
			return True;
		else
			return telement->GoChildByPath(nextname+1);
    }
	return False;
}

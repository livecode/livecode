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
#include <revolution/support.h>

#if defined _MACOSX || defined ( _LINUX ) || defined (TARGET_SUBPLATFORM_IPHONE) || defined (TARGET_SUBPLATFORM_ANDROID)
#define _snprintf snprintf
#endif
/*GetName
returns name of element
*/
char *CXMLElement::GetName()
{
	if (!isinited()) return NULL;
	return (char *)element->name;
}



/*Modified libxml code to avoid truncating text of child elements*/
static xmlChar *
MyxmlNodeGetContent(xmlNodePtr cur) {
    if (cur == NULL) return(NULL);
    switch (cur->type) {
	case XML_DOCUMENT_FRAG_NODE:
	case XML_ELEMENT_NODE: {
		xmlNodePtr tmp = cur;
		xmlBufferPtr buffer;
		xmlChar *ret;
		
		buffer = xmlBufferCreate();
		if (buffer == NULL)
			return(NULL);
		while (tmp != NULL) {
			switch (tmp->type) {
			case XML_CDATA_SECTION_NODE:
			case XML_TEXT_NODE:
				if (tmp->content != NULL)
#ifndef XML_USE_BUFFER_CONTENT
					xmlBufferCat(buffer, tmp->content);
#else
				xmlBufferCat(buffer,
					xmlBufferContent(tmp->content));
#endif
				break;
			case XML_ENTITY_REF_NODE: {
				xmlEntityPtr ent;
				
				ent = xmlGetDocEntity(cur->doc, tmp->name);
				if (ent != NULL)
					xmlBufferCat(buffer, ent->content);
									  }
			default:
				break;
			}
			/*
			* Skip to next node
			*/
			if (tmp->children != NULL) {
				if (tmp->children->type != XML_ENTITY_DECL && tmp->children->type != XML_ELEMENT_NODE) {
					tmp = tmp->children;
					continue;
				}
			}
			
			if (tmp == cur)
				break;
			
			if (tmp->next != NULL) {
				tmp = tmp->next;
				continue;
			}
			
			do {
				tmp = tmp->parent;
				if (tmp == NULL)
					break;
				if (tmp == cur) {
					tmp = NULL;
					break;
				}
				if (tmp->next != NULL) {
					tmp = tmp->next;
					break;
				}
			} while (tmp != NULL);
		}
		ret = buffer->content;
		buffer->content = NULL;
		xmlBufferFree(buffer);
		return(ret);
						   }
	case XML_ATTRIBUTE_NODE: {
		xmlAttrPtr attr = (xmlAttrPtr) cur;
		if (attr->parent != NULL)
			return(xmlNodeListGetString(attr->parent->doc, attr->children, 1));
		else
			return(xmlNodeListGetString(NULL, attr->children, 1));
		break;
							 }
	case XML_COMMENT_NODE:
	case XML_PI_NODE:
		if (cur->content != NULL)
#ifndef XML_USE_BUFFER_CONTENT
			return(xmlStrdup(cur->content));
#else
		return(xmlStrdup(xmlBufferContent(cur->content)));
#endif
		return(NULL);
	case XML_ENTITY_REF_NODE:
	/*
	* Locate the entity, and get it's content
	* @@@
		*/
		return(NULL);
	case XML_ENTITY_NODE:
	case XML_DOCUMENT_NODE:
	case XML_HTML_DOCUMENT_NODE:
	case XML_DOCUMENT_TYPE_NODE:
	case XML_NOTATION_NODE:
	case XML_DTD_NODE:
	case XML_XINCLUDE_START:
	case XML_XINCLUDE_END:
#ifdef LIBXML_DOCB_ENABLED
	case XML_DOCB_DOCUMENT_NODE:
#endif
		return(NULL);
	case XML_NAMESPACE_DECL:
		return(xmlStrdup(((xmlNsPtr)cur)->href));
	case XML_ELEMENT_DECL:
		/* UNIMPLEMENTED */
		return(NULL);
	case XML_ATTRIBUTE_DECL:
		/* UNIMPLEMENTED */
		return(NULL);
	case XML_ENTITY_DECL:
		/* UNIMPLEMENTED */
		return(NULL);
	case XML_CDATA_SECTION_NODE:
	case XML_TEXT_NODE:
		if (cur->content != NULL)
#ifndef XML_USE_BUFFER_CONTENT
			return(xmlStrdup(cur->content));
#else
		return(xmlStrdup(xmlBufferContent(cur->content)));
#endif
		return(NULL);
    }
    return(NULL);
}

/*Modified libxml code to avoid outputting paths with namespaces..at least for now*/
static xmlChar *MyxmlGetNodePath(xmlNodePtr node)
{
    xmlNodePtr cur, tmp, next;
    xmlChar *buffer = NULL, *temp;
    size_t buf_len;
    xmlChar *buf;
    char sep;
    const char *name;
    char nametemp[100];
    int occur = 0;

    if (node == NULL)
        return (NULL);

    buf_len = 500;
    buffer = (xmlChar *) xmlMalloc(buf_len * sizeof(xmlChar));
    if (buffer == NULL)
        return (NULL);
    buf = (xmlChar *) xmlMalloc(buf_len * sizeof(xmlChar));
    if (buf == NULL) {
        xmlFree(buffer);
        return (NULL);
    }

    buffer[0] = 0;
    cur = node;
    do {
        name = "";
        sep = '?';
        occur = 0;
        if ((cur->type == XML_DOCUMENT_NODE) ||
            (cur->type == XML_HTML_DOCUMENT_NODE)) {
            if (buffer[0] == '/')
                break;
            sep = '/';
            next = NULL;
        } else if (cur->type == XML_ELEMENT_NODE) {
            sep = '/';
            name = (const char *) cur->name;
            next = cur->parent;

            /*
             * Thumbler index computation
             */
            tmp = cur->prev;
            while (tmp != NULL) {
                if (xmlStrEqual(cur->name, tmp->name))
                    occur++;
                tmp = tmp->prev;
            }
            if (occur == 0) {
                tmp = cur->next;
                while (tmp != NULL) {
                    if (xmlStrEqual(cur->name, tmp->name))
					{
                        occur++;
						break;
					}
                    tmp = tmp->next;
                }
                if (occur != 0)
                    occur = 1;
            } else
                occur++;
        } else if (cur->type == XML_ATTRIBUTE_NODE) {
            sep = '@';
            name = (const char *) (((xmlAttrPtr) cur)->name);
            next = ((xmlAttrPtr) cur)->parent;
        } else if (cur->type == XML_TEXT_NODE || cur -> type == XML_CDATA_SECTION_NODE) {
			sep = '/';
			name = "";
			next = cur->parent;
            tmp = cur->prev;
            while (tmp != NULL) {
                if (tmp->type == XML_TEXT_NODE || tmp -> type == XML_CDATA_SECTION_NODE)
                    occur++;
                tmp = tmp->prev;
            }
            if (occur == 0) {
                tmp = cur->next;
                while (tmp != NULL) {
                    if (tmp->type == XML_TEXT_NODE || tmp -> type == XML_CDATA_SECTION_NODE)
					{
                        occur++;
						break;
					}
                    tmp = tmp->next;
                }
                if (occur != 0)
                    occur = 1;
            } else
                occur++;
			if (occur == 0)
			{
				cur = next;
				continue;
			}
		} else {
            next = cur->parent;
        }

        /*
         * Make sure there is enough room
         */
        if (xmlStrlen(buffer) + sizeof(nametemp) + 20 > buf_len) {
            buf_len =
                2 * buf_len + xmlStrlen(buffer) + sizeof(nametemp) + 20;
            temp = (xmlChar *) xmlRealloc(buffer, buf_len);
            if (temp == NULL) {
                xmlFree(buf);
                xmlFree(buffer);
                return (NULL);
            }
            buffer = temp;
            temp = (xmlChar *) xmlRealloc(buf, buf_len);
            if (temp == NULL) {
                xmlFree(buf);
                xmlFree(buffer);
                return (NULL);
            }
            buf = temp;
        }
        if (occur == 0)
            _snprintf((char *) buf, buf_len, "%c%s%s",
                     sep, name, (char *) buffer);
        else
            _snprintf((char *) buf, buf_len, "%c%s[%d]%s",
                     sep, name, occur, (char *) buffer);
        _snprintf((char *) buffer, buf_len, "%s", buf);
        cur = next;
    } while (cur != NULL);
    xmlFree(buf);
    return (buffer);
}

/*GetPath
returns full path of element. Caller must free return buffer.
*/
char *CXMLElement::GetPath() 
{
	if (!isinited()) return NULL;
	return (char *)MyxmlGetNodePath(element);
}



/*GetIndex
return the index of element 
(ie. number of elements previous to this one with same tag)
*/
int CXMLElement::GetIndex()
{
	CXMLElement telement;
	telement.CopyElement(this);
	int occur = 1;
	if (!IsTextNode())
	{
		while (telement.GoPrev((char *)element->name))
			occur++;
	}
	else
	{
		while(telement.GoPrev(NULL, true))
			if (telement.IsTextNode())
				occur++;
	}
	return occur;
}

/*AddChild - creates new child element
tname - name of new child element
tcontent - value of new child element
telement - on success, points to new child element
returns False on error.
*/
Bool CXMLElement::AddChild(char *tname, char *tcontent,CXMLElement *telement, bool tbefore)
{
		if (!isinited()) return False;
	   xmlNodePtr newchild = xmlNewTextChild(element, NULL, (xmlChar *)tname, 
		   (xmlChar *)tcontent);
	   if (newchild)
		 {
			 if (tbefore)
			   xmlAddPrevSibling(element -> children, newchild);
				 
		   if (telement) 
			   telement->SetNodePtr(newchild);
		 }
	   return newchild != NULL;
}

/*AddSibling - creates a new element and adds as
  the previous sibling of this node
	tname - name of new sibling element
	tcontent - value of new sibling element
	telement - on success, points to the new element
	returns False on error
*/
Bool CXMLElement::AddSibling(char *tname, char *tcontent, CXMLElement *telement, bool tbefore)
{
	if (!isinited())
		return False;
		
	xmlNodePtr newelement;
	newelement = xmlNewTextChild(element -> parent, NULL, (xmlChar *)tname, (xmlChar *)tcontent);
	if (newelement != NULL)
	{
		if (tbefore)
			xmlAddPrevSibling(element, newelement);
		else
			xmlAddNextSibling(element, newelement);
			
		if (telement)
			telement -> SetNodePtr(newelement);
	}
	
	return newelement != NULL;
}

/*Remove - removes element from xml tree*/
void CXMLElement::Remove()
{
	if (!isinited()) return;
	 xmlUnlinkNode(element);
	 xmlFreeNode(element);
	 element = NULL;
}


/*Write - writes xml tree of element to buffer
output: 
data - is set to point to buffer containing xml data
tlength - is set to length of xml data
*/
void CXMLElement::Write(char **data,int *tlength,Bool isformatted)
{
	if (!isinited()) return;
	if (element->doc){
		xmlBufferPtr buf;
		buf = xmlBufferCreate();
		xmlNodeDump(buf, element->doc, element, 0, isformatted);
		*data = (char *)xmlStrdup(xmlBufferContent(buf));
		*tlength = xmlBufferLength(buf);
		xmlBufferFree(buf);
	}
}

/*CopyElement - copies element of other CXMLElement
truecopy - if true this is set to point to element of telement, 
otherwise this is set to point to a copy of element in telement
output: telement - xml element to copy from
*/
void CXMLElement::CopyElement(CXMLElement *telement,Bool truecopy)
{
	if (truecopy)
		element = xmlCopyNode(telement->GetNodePtr(), 1);
	else
		element = telement->GetNodePtr();
}


/*AddElement - adds element (recursive) of other CXMLElement 
as child element of this element
*/
void CXMLElement::AddElement(CXMLElement *telement)
{
	xmlUnlinkNode(telement->GetNodePtr());
	xmlAddChild(element,telement->GetNodePtr());
}

bool CXMLElement::MoveElement(CXMLElement* p_element, bool p_sibling, bool p_before)
{
	xmlNodePtr t_parent;
	for(t_parent = element; t_parent != NULL; t_parent = t_parent -> parent)
		if (t_parent == p_element -> GetNodePtr())
			return false;

	xmlUnlinkNode(p_element -> GetNodePtr());
	if (p_sibling)
	{
		if (p_before)
			xmlAddPrevSibling(element, p_element -> GetNodePtr());
		else
			xmlAddNextSibling(element, p_element -> GetNodePtr());
	}
	else
	{
		if (p_before && element -> children != NULL)
			xmlAddPrevSibling(element -> children, p_element -> GetNodePtr());
		else
			xmlAddChild(element, p_element -> GetNodePtr());
	}

	return true;
}

// MW-2014-06-12: [[ Bug 12628 ]] Use Remote operation to copy the root out of the new document.
bool CXMLElement::MoveRemoteElement(CXMLElement *p_element, bool p_sibling, bool p_before)
{
    // DOMWrapAdoptNode only allows you to move a node to under a parent, so first
    // we compute the parent node. In non-sibling mode, this is the parent node. In sibling
    // mode the parent of this is the parent node.
    xmlNodePtr t_parent;
    t_parent = element;
    if (p_sibling)
        t_parent = t_parent -> parent;
    
    // If this has no parent then badness.
    if (t_parent == NULL)
        return false;
    
    // Now make our doc tree adopt the source node.
    if (xmlDOMWrapAdoptNode(NULL, p_element -> element -> doc, p_element -> element, element -> doc, t_parent, 0) != 0)
        return false;
    
    // Now that p_element is in this's xmlTree we can use MoveElement to place it appropriately.
    return MoveElement(p_element, p_sibling, p_before);
}

/*GetContent - return value of element
isbuffered - if true returns copy of element data 
- must be disposed by caller.
returns pointer to value of element, null if no content is found.
*/
char *CXMLElement::GetContent(Bool isbuffered)
{
	if (!isinited()) return NULL;
//	return (char *)MyxmlNodeGetContent(element);
	if (isbuffered)
	{
		xmlNodePtr telement = element;
		if (telement -> type != XML_TEXT_NODE && telement -> type != XML_CDATA_SECTION_NODE)
		{
			telement = telement->children;
			while(telement && telement->type != XML_TEXT_NODE && telement->type != XML_CDATA_SECTION_NODE)
				telement = telement->next;
		}
		if (!telement)
			return NULL;
		return telement->content != NULL? (char *)telement->content: NULL;
	}
	else 
		return (char *)MyxmlNodeGetContent(element);
}

/*SetContent - sets value of element
tdata - new value
*/

void CXMLElement::SetContent(char *tdata)
{
	if (!isinited()) return;
	xmlNodeSetContent(element,(xmlChar *)tdata);
}

/*GoChild - Navigate to child element.
ename - if non-null filters child elements by name, otherwise navigates to first child element
returns True if child element found
*/
Bool CXMLElement::GoChild(char *ename, bool p_inc_text) //go to next child
{
	if (!isinited()) return False;
	xmlNodePtr telement = element;
	if (telement->children) {
			telement = telement->children;
			while (telement != NULL)
			{
				if (telement->type == XML_ELEMENT_NODE)
					break;
				if (p_inc_text && ename == NULL && (telement->type == XML_TEXT_NODE || telement->type == XML_CDATA_SECTION_NODE))
					break;
				telement = telement->next;
			}
			if (!telement)
				return False;
			element = telement;
			if (ename && util_strnicmp((char *)element->name,ename, strlen(ename)) != 0)
					return GoNext(ename);
			return True;
	}
	return False;
}

/*GoParent - Navigate to parent element
 * Return True if succeeded, False on error.
 */
Bool CXMLElement::GoParent()
{
	if (!isinited()) return False;
	if (element->parent){
		element = element->parent;
		return True;
	}
	return False;
}


/*GoNext - Navigate to next sibling element.
ename - if non-null filters  elements by name, otherwise navigates to next element
returns True if element found
*/
Bool CXMLElement::GoNext(char *ename, bool p_inc_text) //go to next child
{
	if (!isinited()) return False;
	Bool match = False;
	xmlNodePtr telement = element;
	while (!match && telement->next)
	{
		if (telement->next->type == XML_ELEMENT_NODE 
			&& (!ename || util_strnicmp((char *)telement->next->name,ename,strlen(ename)) == 0))
			match = True;
		else if (p_inc_text && ename == NULL &&(telement->next->type == XML_TEXT_NODE || telement->next->type == XML_CDATA_SECTION_NODE))
			match = True;
		telement = telement->next;
	}
	if (match)
		element = telement;
	return match;
}

/*GoPrev - Navigate to previous sibling element.
ename - if non-null filters  elements by name, otherwise navigates to prev element
returns True if element found
*/
Bool CXMLElement::GoPrev(char *ename, bool p_inc_text)
{
	if (!isinited()) return False;
	Bool match = False;
	xmlNodePtr telement = element;
	while (!match && telement->prev)
	{
		if (telement->prev->type == XML_ELEMENT_NODE 
			&& (!ename || util_strnicmp((char *)telement->prev->name,ename,strlen(ename)) == 0))
			match = True;
		else if (p_inc_text && ename == NULL &&(telement->prev->type == XML_TEXT_NODE || telement->prev->type == XML_CDATA_SECTION_NODE))
			match = True;
		telement = telement->prev;
	}
	if (match)
		element = telement;
	return match;
}

/*GoChildByPath
tpath - path of child element (ie. /parentelement[1]/childelement)
telement - on success, navigates to element specified by tpath
return True if element found
*/
Bool CXMLElement::GoChildByPath(char *tpath)
{
	if (!isinited()) return False;
	const char *sptr = tpath;
	CXMLElement telement;
	telement.CopyElement(this);
	if (*sptr == '/') sptr++; //skip first slash
	const char *endptr = sptr + strlen(sptr);
	while (sptr < endptr)
	{
		const char *namestart,*nameend,*nextname,*numpointer;
		nextname = strchr(sptr, '/' );
		if (!nextname) nextname = endptr;
		namestart = sptr;
		nameend = nextname;

		bool t_is_text;
		t_is_text = (nameend == namestart) || (namestart[0] == '[');

		if (!telement.GoChild(NULL, t_is_text))
			return False;
		
		int whichchild = 1;
		numpointer = util_strchr(sptr,'[',nameend-sptr);
		if (numpointer)
		{
			whichchild = strtol((const char *)++numpointer, NULL, 10);
			nameend = numpointer-1;
		}
		
		Bool foundmatch = False;
		int childcounter = 0;
		do
		{
			if ((!t_is_text && util_strnicmp(telement.GetName(),namestart,nameend-namestart)==0) ||
				(t_is_text && telement.IsTextNode()))
			{
				childcounter++;
				if (childcounter == whichchild)
				{
					sptr = nextname+1;
					foundmatch = True;
				}
			}
		}
		while (!foundmatch && telement.GoNext(NULL, t_is_text));
		if (!foundmatch)
			return False;
	}
	CopyElement(&telement);
	return True;

}


/*ChildCount - returns total count of children.
childname - filter by childname. NULL to count any child.
maxdepth - how many levels to go down in child tree
*/
int CXMLElement::ChildCount(char *childname, int maxdepth)
{
	if (!isinited()) return 0;
	CXMLElement telement;
	CXMLElementEnumerator tenum(this,maxdepth);
	int childcount = 0;
	while (tenum.Next(childname))
				childcount++;
	return childcount;
}

/*AttributeCount - returns total count of attributes.
attributename - filter by attributename. NULL to count any attribute.
*/
// MDW-2013-07-09: [[ RevXmlXPath ]]
int CXMLElement::AttributeCount(char *attributename)
{
	if (!isinited()) return 0;
	CXMLAttribute tattribute;
	int attributecount = 0;
	if (GetFirstAttribute(&tattribute)){
		do {
			if (!attributename || 
				util_strnicmp(tattribute.GetName(),attributename,strlen(attributename)) == 0)
				attributecount++;
		} while (tattribute.GoNext());
	}
	return attributecount;
}


/*SetAttribute - set attribute value in element
attname-attribute name
tdata - new value
returns False on error
*/
Bool CXMLElement::SetAttribute(char *attname,char *tdata)
{
	if (!isinited())
		return False;

	// OK-2009-01-06: [[Bug 7586]] - When setting attributes, check to see if the user is trying to set the namespace.
	// If they are, then either create a namespace, or modify the existing one. Note that this doesn't support multiple
	// namespaces, not sure if this is a problem or not.
	xmlAttrPtr  tatt;
	if (strcasecmp(attname, "xmlns") == 0)
	{
		xmlNs *t_namespace;
		t_namespace = NULL;
		GetNamespace(t_namespace);
		if (t_namespace != NULL && t_namespace -> href != NULL)
		{
			// Free the old href buffer, and replace it with a new one
			xmlFree((xmlChar* )t_namespace -> href);
			t_namespace -> href = xmlStrdup((xmlChar *)tdata);

			// Replace the existing namespace with the modified one
			xmlSetNs(element, t_namespace);
		}
		else
		{
			// No namespace exists, so we just create one.
			t_namespace = xmlNewNs(element, (xmlChar *)tdata, NULL);
		}

		return True;
	}
	else
	{
		tatt = xmlSetProp(element, (xmlChar *)attname, (xmlChar *)tdata);
		return tatt != NULL;
	}
}

/*RemoveAttribute - remove attribute from element
attname - name of attribute to remove
returns False on error
*/
Bool CXMLElement::RemoveAttribute(char *attname)
{
	if (!isinited())
		return False;

	// OK-2009-01-06: [[Bug 7586]] - Remove the namespace if that is the name of the attribute.
	// Not sure how reliable this is, it seems to work ok. The revXMLRemoveAttribute is not currently
	// documented.
	if (strcasecmp(attname, "xmlns") == 0)
	{
		xmlNsPtr t_namespace;
		t_namespace = NULL;
		GetNamespace(t_namespace);
		if (t_namespace != NULL)
			xmlFreeNs(t_namespace);

		element -> ns = NULL;
		element -> nsDef = NULL;
		return True;
	}
	else
	{
		int res = xmlUnsetProp (element, (xmlChar *)attname);
		return res == 0;
	}
}


/*GetFirstAttribute
tattribute - on success, point to first attribute
returns False if no attributes exists for this element
*/
Bool CXMLElement::GetFirstAttribute(CXMLAttribute *tattribute)
{
	if (!isinited()) return False;
	if (element->properties)
	tattribute->SetAttrPtr(element->properties);
	return element->properties != NULL;
}

Bool CXMLElement::GetNamespace(xmlNs *&r_namespace)
{
	r_namespace = element -> nsDef;
	return True;
}

/*GetAttribute - returns value of specified attribute
attname - name of attribute to look for
isbuffered - if true returns copy of attribute value (must be freed by caller).
returns NULL if specified attribute not found
*/
char *CXMLElement::GetAttributeValue(char *attname, Bool isbuffered)
{
	if (!isinited()) 
		return NULL;

	// OK-2009-01-05: [[Bug 7586]] - Some elements may have a "namespace". This is not regarded by libXML
	// as one of the elements' attributes, but is stored separately. If the required attribute is called 
	// "xmlns", then we retreive the namespace instead of using GetAttributeValue.
	if (strcasecmp(attname, "xmlns") == 0)
	{
		xmlNs *t_namespace;
		t_namespace = NULL;
		GetNamespace(t_namespace);

		if (t_namespace != NULL && t_namespace -> href != NULL)
		{
			return strdup((char *)t_namespace -> href);
		}
	}

	if (isbuffered)
	{
		CXMLAttribute tattribute;
		if (GetFirstAttribute(&tattribute))
		{
			do 
			{
				if (!attname || util_strnicmp(tattribute.GetName(), attname, strlen(attname)) == 0)
                // AL-2015-09-02: [[ Bug 15848 ]] if isbuffered is true, returned string is freed by the caller
					return strdup(tattribute.GetContent());
			} while (tattribute.GoNext());
		}
	}
	else
		return (char *)xmlGetProp(element, (xmlChar *)attname);
	return NULL;
}

//---------------------CXMLElementEnumerator---------------------------
CXMLElementEnumerator::CXMLElementEnumerator(CXMLElement *startat,int tmaxdepth)
{
	element.CopyElement(startat);
	maxdepth = tmaxdepth;
	depth = 0;
}

Bool CXMLElementEnumerator::Next(char *childname, bool p_inc_text)
{
	Bool res;
	
	if (childname != NULL && p_inc_text)
		p_inc_text = false;

	do {
		res = False;
		if (!res && (maxdepth == - 1 || depth <= maxdepth)){
			res = element.GoChild(NULL, p_inc_text);
			if (res) depth++;
		}
		if (!res && depth) res = element.GoNext(NULL, p_inc_text);
		while (!res && depth){
			res = element.GoParent();
			if (res) depth--;
			if (!depth)
				res = False;
			else
				res = element.GoNext(NULL, p_inc_text);
		}
	} while (childname && 
		res && util_strnicmp(element.GetName(), childname, strlen(childname)) != 0);  
	return res;
}

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

char *CXMLAttribute::GetName()
{
	if (!isinited()) 	return NULL;
	return (char *)attribute->name;

}

Bool CXMLAttribute::GoNext()
{
	if (!isinited()) return False;
	Bool retval = attribute->next != NULL;
	if (attribute->next) attribute = attribute->next;
	return retval;
}

Bool CXMLAttribute::GoPrev()
{
	if (!isinited()) return False;
		// MDW-2013-07-09: [[ RevXmlXPath ]]
		Bool retval = attribute->prev != NULL;
		if (attribute->prev) attribute = attribute->prev;
		return retval;
}

char *CXMLAttribute::GetContent()
{
	if (!isinited()) return NULL;
	if (attribute->children && attribute->children->content)
			return (char *)attribute->children->content;
	return XMLNULLSTRING;
}

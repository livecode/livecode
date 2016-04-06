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

void REVXML_Version(char *args[], int nargs, char **retstring,
		   Bool *pass, Bool *error);
void XML_Init(char *args[], int nargs, char **retstring,
							Bool *pass, Bool *error); 
void XML_NewDocument(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error); 
//HS-2010-10-11: [[ Bug 7586 ]] Reinstate libxml2 to create name spaces. Implement new liveCode commands to suppress name space creation.
void XML_NewDocumentNS(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error); 
void XML_NewDocumentNNS(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error); 
void XML_NewDocumentFromFile(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error); 
//HS-2010-10-11: [[ Bug 7586 ]] Reinstate libxml2 to create name spaces. Implement new liveCode commands to suppress name space creation.
void XML_NewDocumentFromFileNS(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error); 
void XML_NewDocumentFromFileNNS(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error); 
void XML_FreeDocument(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error); 
void XML_GetAttributeValue(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error); 
void XML_GetElementContents(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error);
void XML_GetDocText(char *args[], int nargs, char **retstring,
							Bool *pass, Bool *error); 
void XML_AllowCallbacks(char *args[], int nargs, char **retstring,
					  Bool *pass, Bool *error);
void XML_BuildTree(char *args[], int nargs, char **retstring,
					  Bool *pass, Bool *error);
void XML_Documents(char *args[], int nargs, char **retstring,
							Bool *pass, Bool *error); 
void XML_AddDTD(char *args[], int nargs, char **retstring,
							Bool *pass, Bool *error); 
void XML_ValidateDTD(char *args[], int nargs, char **retstring,
							Bool *pass, Bool *error); 
void XML_AddXML(char *args[], int nargs, char **retstring,
							Bool *pass, Bool *error); 
void XML_AddElement(char *args[], int nargs, char **retstring,
							Bool *pass, Bool *error); 
void XML_RemoveElement(char *args[], int nargs, char **retstring,
							Bool *pass, Bool *error); 
void XML_SetAttributeValue(char *args[], int nargs, char **retstring,
							Bool *pass, Bool *error); 
void XML_RemoveAttribute(char *args[], int nargs, char **retstring,
							Bool *pass, Bool *error); 
void XML_SetElementContents(char *args[], int nargs, char **retstring,
							Bool *pass, Bool *error); 
void XML_FreeAll(char *args[], int nargs, char **retstring,
							Bool *pass, Bool *error); 
void XML_ListOfAttributes(char *args[], int nargs, char **retstring,
						  Bool *pass, Bool *error);
void XML_ListOfChildText(char *args[], int nargs, char **retstring,
						  Bool *pass, Bool *error);
void XML_RootName(char *args[], int nargs, char **retstring,
						  Bool *pass, Bool *error);
void XML_GetChildPath(char *args[], int nargs, char **retstring,
						  Bool *pass, Bool *error);
void XML_GetNextSiblingPath(char *args[], int nargs, char **retstring,
						  Bool *pass, Bool *error);
void XML_GetPrevSiblingPath(char *args[], int nargs, char **retstring,
						  Bool *pass, Bool *error);
void XML_GetParentPath(char *args[], int nargs, char **retstring,
						  Bool *pass, Bool *error);
void XML_ChildCount(char *args[], int nargs, char **retstring,
						  Bool *pass, Bool *error);
void XML_Children(char *args[], int nargs, char **retstring, 
				  Bool *pass, Bool *error);
void XML_Tree(char *args[], int nargs, char **retstring,
				  Bool *pass, Bool *error);
void XML_ListByAttributeValue(char *args[], int nargs, char **retstring,
				  Bool *pass, Bool *error);
void XML_FindElementByAttributeValue(char *args[], int nargs, char **retstring,
				  Bool *pass, Bool *error);
void REVXML_INIT();
void REVXML_QUIT();

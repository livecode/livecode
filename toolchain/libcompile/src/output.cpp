/* Copyright (C) 2016 LiveCode Ltd.
 
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

//#include "report.h"
//#include "literal.h"
#include <output.h>
#include <report.h>

#include <foundation.h>
#include <foundation-auto.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#ifndef WIN32
#include <unistd.h>
#endif

//////////

static FILE *s_output = NULL;

void OutputFileBegin(FILE *p_file)
{
    s_output = p_file;
}

void OutputWrite(const char *p_string)
{
    if (s_output == NULL)
        return;
    
    fprintf(s_output, "%s", p_string);
}

void OutputWriteS(const char *p_left, const char *p_string, const char *p_right)
{
    if (s_output == NULL)
        return;
    
    fprintf(s_output, "%s%s%s", p_left, p_string, p_right);
}

void OutputWriteI(const char *p_left, NameRef p_name, const char *p_right)
{
    if (s_output == NULL)
        return;
    
    const char *t_name_string;
    GetStringOfNameLiteral(p_name, &t_name_string);
    OutputWriteS(p_left, t_name_string, p_right);
}

void OutputWriteD(const char *p_left, double* p_number, const char *p_right)
{
    if (s_output == NULL)
        return;
    
    fprintf(s_output, "%s%f%s", p_left, *p_number, p_right);
}

void OutputWriteN(const char *p_left, intptr_t p_number, const char *p_right)
{
    if (s_output == NULL)
        return;
    
    fprintf(s_output, "%s%ld%s", p_left, p_number, p_right);
}

void OutputEnd(void)
{
    if (s_output == NULL)
        return;
    
    fclose(s_output);
}

/* This is the same as OutputWriteS, but escapes special XML
 * characters (&, ", ', <, >) found in p_string */
void OutputWriteXmlS(const char *p_left, const char *p_string, const char *p_right)
{
	struct xml_replacement_t {
		const char *from, *to;
	};
	
	static const struct xml_replacement_t k_replacements[] = {
		{"&", "&amp;"},
		{"<", "&lt;"},
		{">", "&gt;"},
		{"\"", "&quot;"},
		{"\'", "&apos;"},
		{NULL, NULL},
	};
	
	if (s_output == NULL)
	{
		return;
	}
	
	bool t_success = true;
	MCAutoStringRef t_string;
	if (t_success)
	{
		t_success =
		MCStringCreateWithBytes(reinterpret_cast<const byte_t *>(p_string),
								(uindex_t)strlen(p_string), kMCStringEncodingUTF8,
								false, &t_string);
	}
	
	MCAutoStringRef t_xml_string;
	if (t_success)
	{
		t_success = MCStringMutableCopy(*t_string, &t_xml_string);
	}
	for (int i = 0; t_success && k_replacements[i].from != nil; ++i)
	{
		t_success = MCStringFindAndReplace(*t_xml_string,
										   MCSTR(k_replacements[i].from),
										   MCSTR(k_replacements[i].to),
										   kMCStringOptionCompareExact);
	}
	
	MCAutoStringRefAsUTF8String t_xml_utf8string;
	if (t_success)
	{
		t_success = t_xml_utf8string.Lock(*t_xml_string);
	}
	
	if (t_success)
	{
		OutputWriteS(p_left, *t_xml_utf8string, p_right);
	}
	else
	{
		/* UNCHECKED */ abort();
	}
}

//////////

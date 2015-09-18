/* Copyright (C) 2013-2015 LiveCode Ltd.
 
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

/*
 *  Coder.cpp
 *  lcidlc
 *
 *  Created by Mark Waddingham on 27/07/2013.
 *
 */

#include <stdio.h>
#include "foundation.h"
#include "Coder.h"

struct Coder
{
	FILE *stream;
	uint32_t depth;
	bool padded;
};

static void CoderIndent(CoderRef self)
{
	const char *s_tabs = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	fprintf(self -> stream, "%.*s", self -> depth > 32 ? 32 : self -> depth, s_tabs);
}

bool CoderStart(const char *p_filename, CoderRef& r_coder)
{
	bool t_success;
	t_success = true;
	
	CoderRef self;
	self = nil;
	if (t_success)
		t_success = MCMemoryNew(self);
		
	if (t_success)
	{
		self -> stream = fopen(p_filename, "w");
		if (self -> stream == nil)
			t_success = false;
	}
	
	if (t_success)
	{
		self -> depth = 0;
		r_coder = self;
	}
	else
		MCMemoryDelete(self);
		
	return t_success;
}

bool CoderFinish(CoderRef self)
{
	fclose(self -> stream);
	MCMemoryDelete(self);
	return true;
}

void CoderCancel(CoderRef self)
{
	CoderFinish(self);
}

void CoderWriteLine(CoderRef self, const char *p_format, ...)
{
	va_list t_args;
	va_start(t_args, p_format);
	vfprintf(self -> stream, p_format, t_args);
	va_end(t_args);
	fprintf(self -> stream, "\n");
	
	self -> padded = false;
}

void CoderWrite(CoderRef self, const char *p_format, ...)
{
	va_list t_args;
	va_start(t_args, p_format);
	vfprintf(self -> stream, p_format, t_args);
	va_end(t_args);
}

void CoderWriteStatement(CoderRef self, const char *p_format, ...)
{
	CoderBeginStatement(self);

	va_list t_args;
	va_start(t_args, p_format);
	vfprintf(self -> stream, p_format, t_args);
	va_end(t_args);
	
	CoderEndStatement(self);
}

void CoderBeginStatement(CoderRef self)
{
	CoderIndent(self);
}

void CoderEndStatement(CoderRef self)
{
	fprintf(self -> stream, ";\n");
	
	self -> padded = false;
}

void CoderBeginPreprocessor(CoderRef self, const char *p_format, ...)
{
	va_list t_args;
	va_start(t_args, p_format);
	vfprintf(self -> stream, p_format, t_args);
	va_end(t_args);
	fprintf(self -> stream, "\n");

	self -> padded = false;
}

void CoderEndPreprocessor(CoderRef self, const char *p_format, ...)
{
	va_list t_args;
	va_start(t_args, p_format);
	vfprintf(self -> stream, p_format, t_args);
	va_end(t_args);
	fprintf(self -> stream, "\n");
	
	self -> padded = false;
}

void CoderBegin(CoderRef self, const char *p_format, ...)
{
	if (*p_format != '\0')
	{
		CoderIndent(self);
		va_list t_args;
		va_start(t_args, p_format);
		vfprintf(self -> stream, p_format, t_args);
		va_end(t_args);
		fprintf(self -> stream, "\n");
	}
	CoderIndent(self);
	fprintf(self -> stream, "{\n");
	
	self -> depth += 1;
	self -> padded = false;
}

void CoderEndBegin(CoderRef self, const char *p_format, ...)
{
	CoderEnd(self, "");
	CoderIndent(self);
	va_list t_args;
	va_start(t_args, p_format);
	vfprintf(self -> stream, p_format, t_args);
	va_end(t_args);
	fprintf(self -> stream, "\n");
	CoderBegin(self, "");
	
	self -> padded = false;
}

void CoderEnd(CoderRef self, const char *p_format, ...)
{
	self -> depth -= 1;
	CoderIndent(self);
	fprintf(self -> stream, "}\n");
	if (*p_format != '\0')
	{
		CoderIndent(self);
		va_list t_args;
		va_start(t_args, p_format);
		vfprintf(self -> stream, p_format, t_args);
		va_end(t_args);
		fprintf(self -> stream, "\n");
	}
	
	self -> padded = false;
}

void CoderBeginIf(CoderRef self, const char *p_format, ...)
{
	CoderIndent(self);
	fprintf(self -> stream, "if (");
	va_list t_args;
	va_start(t_args, p_format);
	vfprintf(self -> stream, p_format, t_args);
	va_end(t_args);
	fprintf(self -> stream, ")\n");
	CoderBegin(self, "");
	
	self -> padded = false;
}

void CoderElse(CoderRef self)
{
	CoderEndBegin(self, "else");
}

void CoderEndIf(CoderRef self)
{
	CoderEnd(self, "");
}

void CoderPad(CoderRef self)
{
	if (self -> padded)
		return;
		
	CoderWriteLine(self, "");
	self -> padded = true;
}

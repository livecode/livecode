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

#include <stdio.h>
#include "foundation.h"
#include "Error.h"
#include "Scanner.h"

////////////////////////////////////////////////////////////////////////////////

struct Scanner
{
	char *input_buffer;
	uint32_t input_frontier;
	uint32_t input_limit;
	
	uint32_t input_row;
	uint32_t input_column;

	Token *token_buffer;
	uint32_t token_pointer;
	uint32_t token_mark;
	uint32_t token_frontier;
	uint32_t token_capacity;
};

////////////////////////////////////////////////////////////////////////////////

static bool ScannerIsEndPrefix(ScannerRef self)
{
	return self -> input_frontier == self -> input_limit;
}

static bool ScannerIsWhitespacePrefix(ScannerRef self)
{
	return self -> input_buffer[self -> input_frontier] == ' ' ||
			self -> input_buffer[self -> input_frontier] == '\t';
}

static bool ScannerIsNewlinePrefix(ScannerRef self)
{
	return self -> input_buffer[self -> input_frontier] == 10 ||
			self -> input_buffer[self -> input_frontier] == 13;
}

static bool ScannerIsCommentPrefix(ScannerRef self)
{
	if (self -> input_buffer[self -> input_frontier] == '#')
		return true;

	if (self -> input_limit - self -> input_frontier <= 1)
		return false;
		
	if (self -> input_buffer[self -> input_frontier] == '/' &&
		self -> input_buffer[self -> input_frontier + 1] == '/')
		return true;
		
	if (self -> input_buffer[self -> input_frontier] == '-' &&
		self -> input_buffer[self -> input_frontier + 1] == '-')
		return true;
		
	return false;
}

static bool ScannerIsMultilineCommentPrefix(ScannerRef self)
{
	if (self -> input_buffer[self -> input_frontier] == '/' &&
		self -> input_buffer[self -> input_frontier + 1] == '*')
		return true;
	
	return false;
}

// MW-2013-06-14: [[ ExternalsApiV5 ]] Add support for /* ... */ style multi-line comments.
static bool ScannerIsMultilineCommentSuffix(ScannerRef self)
{
	if (self -> input_buffer[self -> input_frontier] == '*' &&
		self -> input_buffer[self -> input_frontier + 1] == '/')
		return true;
	
	return false;
}

static bool ScannerIsIdentifierPrefix(ScannerRef self)
{
	char t_lookahead;
	t_lookahead = self -> input_buffer[self -> input_frontier];
	if ((t_lookahead >= 'a' && t_lookahead <= 'z') ||
		(t_lookahead >= 'A' && t_lookahead <= 'Z') ||
		t_lookahead == '_')
		return true;
		
	return false;
}

static bool ScannerIsIdentifierSuffix(ScannerRef self)
{
	char t_lookahead;
	t_lookahead = self -> input_buffer[self -> input_frontier];
	if ((t_lookahead >= 'a' && t_lookahead <= 'z') ||
		(t_lookahead >= 'A' && t_lookahead <= 'Z') ||
		(t_lookahead >= '0' && t_lookahead <= '9') ||
		t_lookahead == '_' ||
		t_lookahead == '-' || t_lookahead == '+' ||
		t_lookahead == '.')
		return true;
		
	return false;
}

static bool ScannerIsNumberPrefix(ScannerRef self)
{
	char t_lookahead;
	t_lookahead = self -> input_buffer[self -> input_frontier];
	// MERG-2013-06-14: [[ ExternalsApiV5 ]] Allow negative integer constants.
	if ((t_lookahead >= '0' && t_lookahead <= '9') ||
        t_lookahead == '-')
		return true;

	return false;
}

static bool ScannerIsIntegerSuffix(ScannerRef self)
{
	char t_lookahead;
	t_lookahead = self -> input_buffer[self -> input_frontier];
	if ((t_lookahead >= '0' && t_lookahead <= '9'))
		return true;
	return false;
}

static bool ScannerIsDecimalPrefix(ScannerRef self)
{
	if (self -> input_buffer[self -> input_frontier] == '.')
		return true;
	return false;
}

static bool ScannerIsStringPrefix(ScannerRef self)
{
	if (self -> input_buffer[self -> input_frontier] == '"')
		return true;
	return false;
}

static bool ScannerIsComma(ScannerRef self)
{
	return self -> input_buffer[self -> input_frontier] == ',';
}

static void ScannerSkipNewline(ScannerRef self)
{
	char t_lookahead;
	t_lookahead = self -> input_buffer[self -> input_frontier];
	if (t_lookahead == 10)
	{
		self -> input_column = 0;
		self -> input_row += 1;
		self -> input_frontier += 1;
	}
	else if (t_lookahead == 13)
	{
		if (self -> input_limit - self -> input_frontier > 1 &&
			self -> input_buffer[self -> input_frontier + 1] == 10)
			self -> input_frontier += 1;
		self -> input_column = 0;
		self -> input_row += 1;
		self -> input_frontier += 1;
	}
}

static void ScannerSkipComment(ScannerRef self)
{
	while(!ScannerIsEndPrefix(self))
	{
		if (ScannerIsNewlinePrefix(self))
		{
			ScannerSkipNewline(self);
			break;
		}
		
		self -> input_frontier += 1;
		self -> input_column += 1;
	}
}

// MW-2013-06-14: [[ ExternalsApiV5 ]] Add support for /* ... */ style multi-line comments.
static void ScannerSkipMultilineComment(ScannerRef self)
{
	while(!ScannerIsEndPrefix(self))
	{
		if (ScannerIsNewlinePrefix(self))
		{
			ScannerSkipNewline(self);
		}
		else if (ScannerIsMultilineCommentSuffix(self))
		{
			self -> input_frontier += 2;
			self -> input_column += 2;
			break;
		}
		else
		{
			self->input_frontier += 1;
			self->input_column += 1;
		}
	}
}

// MW-2013-06-14: [[ ExternalsApiV5 ]] Add support for /* ... */ style multi-line comments.
static void ScannerSkipWhitespace(ScannerRef self)
{
	while(!ScannerIsEndPrefix(self))
	{
		if (ScannerIsWhitespacePrefix(self))
		{
			self -> input_column += 1;
			self -> input_frontier += 1;
		}	
		else if (ScannerIsNewlinePrefix(self))
			ScannerSkipNewline(self);
		else if (ScannerIsCommentPrefix(self))
			ScannerSkipComment(self);
		else if (ScannerIsMultilineCommentPrefix(self))
			ScannerSkipMultilineComment(self);
		else
			break;
	}
}

static bool ScannerEnsureToken(ScannerRef self)
{
	if (self -> token_frontier == self -> token_capacity)
	{
		if (self -> token_mark != 0)
		{
			for(uint32_t i = 0; i < self -> token_mark; i++)
				ValueRelease(self -> token_buffer[i] . value);
			
			for(uint32_t i = self -> token_mark; i < self -> token_frontier; i++)
				self -> token_buffer[i - self -> token_mark] = self -> token_buffer[i];
			
			self -> token_pointer -= self -> token_mark;
			self -> token_frontier -= self -> token_mark;
			self -> token_mark = 0;
		}
		else
		{
			if (!MCMemoryResizeArray(self -> token_capacity == 0 ? 16 : self -> token_capacity * 2, self -> token_buffer, self -> token_capacity))
				return false;
		}
	}
	
	return true;
}

static bool ScannerConsume(ScannerRef self)
{
	TokenType t_type;
	uint32_t t_row;
	uint32_t t_first_column;
	uint32_t t_start;
	ValueRef t_value;
	
	ScannerSkipWhitespace(self);
	
	t_type = kTokenTypeNone;
	t_row = self -> input_row;
	t_first_column = self -> input_column;
	t_start = self -> input_frontier;
	t_value = nil;
	
	bool t_success;
	t_success = true;

	if (ScannerIsEndPrefix(self))
		t_type = kTokenTypeEnd;
	else if (ScannerIsIdentifierPrefix(self))
	{
		t_type = kTokenTypeIdentifier;

		while(!ScannerIsEndPrefix(self))
		{
			if (!ScannerIsIdentifierSuffix(self))
				break;

			self -> input_frontier += 1;
			self -> input_column += 1;
		}
		
		t_success = NameCreateWithNativeChars(self -> input_buffer + t_start, self -> input_frontier - t_start, t_value);
	}
	else if (ScannerIsNumberPrefix(self))
	{
		t_type = kTokenTypeNumber;
		
		// MW-2013-06-17: [[ ExternalsApiV5 ]] Since number-prefix is not a subset of
		//   number-suffix (due to '-'), make sure we advance past the prefix before
		//   continuing the scan.
		self -> input_frontier += 1;
		self -> input_column += 1;
		
		while(!ScannerIsEndPrefix(self) &&
				ScannerIsIntegerSuffix(self))
		{
			self -> input_frontier += 1;
			self -> input_column += 1;
		}
		
		if (!ScannerIsEndPrefix(self) &&
			ScannerIsDecimalPrefix(self))
		{
			self -> input_frontier += 1;
			self -> input_column += 1;			
			while(!ScannerIsEndPrefix(self) &&
					ScannerIsIntegerSuffix(self))
			{
				self -> input_frontier += 1;
				self -> input_column += 1;
			}
		}
		
		t_success = NumberCreateWithNativeChars(self -> input_buffer + t_start, self -> input_frontier - t_start, t_value);
	}
	else if (ScannerIsStringPrefix(self))
	{
		self -> input_frontier += 1;
		self -> input_column += 1;
			
		do
		{
			if (ScannerIsEndPrefix(self))
			{
				t_type = kTokenTypeUnterminatedStringError;
				break;
			}
			
			if (ScannerIsNewlinePrefix(self))
			{
				t_type = kTokenTypeNewlineInStringError;
				break;
			}
			
			if (ScannerIsStringPrefix(self))
				t_type = kTokenTypeString;
			
			self -> input_frontier += 1;
			self -> input_column += 1;
		}
		while(t_type == kTokenTypeNone);
		
		t_success = StringCreateWithNativeChars(self -> input_buffer + t_start + 1, self -> input_frontier - t_start - 2, t_value);
	}
	else if (ScannerIsComma(self))
	{
		self -> input_frontier += 1;
		self -> input_column += 1;
		
		t_type = kTokenTypeComma;
	}
	else
	{
		t_type = kTokenTypeInvalidCharError;
		
		self -> input_frontier += 1;
		self -> input_column += 1;
	}
	
	if (t_success)
		t_success = ScannerEnsureToken(self);
		
	if (t_success)
	{
		self -> token_buffer[self -> token_frontier] . type = t_type;
		self -> token_buffer[self -> token_frontier] . start = PositionMake(t_row, t_first_column);
		self -> token_buffer[self -> token_frontier] . finish = PositionMake(t_row, t_first_column + (self -> input_frontier - t_start));
		self -> token_buffer[self -> token_frontier] . value = t_value;
		self -> token_frontier += 1;
	}
	else
		ValueRelease(t_value);
		
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static bool TextFileFetch(const char *p_filename, char*& r_data, uint32_t& r_data_size)
{
	bool t_success;
	t_success = true;
	
	FILE *t_stream;
	t_stream = nil;
	if (t_success)
	{
		t_stream = fopen(p_filename, "rb");
		if (t_stream == nil)
			t_success = Throw(kErrorCouldNotOpenFile);
	}
	
	uint32_t t_file_size;
	t_file_size = 0;
    
	if (t_success)
        t_success = fseek(t_stream, 0, SEEK_END) == 0;
    
    if (t_success)
    {
        long t_size;
        t_size = ftell(t_stream);
        t_file_size = (uint32_t) t_size;
        t_success = (t_size >= 0);
    }
        
    if (t_success)
        t_success = fseek(t_stream, 0, SEEK_SET) == 0;
    
	char *t_file_data;
	t_file_data = nil;
	if (t_success)
		t_success = MCMemoryNewArray(t_file_size, t_file_data);
		
	if (t_success)
	{
		if (fread(t_file_data, t_file_size, 1, t_stream) != 1)
			t_success = Throw(kErrorCouldNotReadFile);
	}
	
	if (t_success)
	{
		r_data = t_file_data;
		r_data_size = t_file_size;
	}
	else
		MCMemoryDeleteArray(t_file_data);
	
	if (t_stream != nil)
		fclose(t_stream);
		
	return t_success;
}

bool ScannerCreate(const char *p_filename, ScannerRef& r_scanner)
{
	bool t_success;
	t_success = true;
	
	ScannerRef self;
	self = nil;
	if (t_success)
		t_success = MCMemoryNew(self);
	
	if (t_success)
		t_success = TextFileFetch(p_filename, self -> input_buffer, self -> input_limit);
	
	if (t_success)
		t_success = ScannerConsume(self);
	
	if (t_success)
		r_scanner = self;
	else
		ScannerDestroy(self);
		
	return t_success;
}

void ScannerDestroy(ScannerRef self)
{
	for(uint32_t i = 0; i < self -> token_frontier; i++)
		ValueRelease(self -> token_buffer[i] . value);
	MCMemoryDeleteArray(self -> token_buffer);
	
	MCMemoryDeleteArray(self -> input_buffer);
	
	MCMemoryDelete(self);
}

////////////////////////////////////////////////////////////////////////////////

bool ScannerAdvance(ScannerRef self)
{
	bool t_success;
	t_success = true;
	
	if (t_success &&
		self -> token_pointer + 1 == self -> token_frontier)
		t_success = ScannerConsume(self);
	
	if (t_success &&
		self -> token_pointer == self -> token_frontier)
		t_success = Throw(kErrorCantAdvancePastEnd);
		
	if (t_success)
		self -> token_pointer += 1;
	
	return t_success;
}

bool ScannerRetreat(ScannerRef self)
{
	bool t_success;
	t_success = true;
	
	if (t_success &&
		self -> token_pointer == self -> token_mark)
		t_success = Throw(kErrorCantRetreatPastMark);
		
	if (t_success)
		self -> token_pointer -= 1;
		
	return t_success;
}

void ScannerMark(ScannerRef self)
{
	self -> token_mark = self -> token_pointer;
}

bool ScannerRetrieve(ScannerRef self, const Token*& r_token)
{
	bool t_success;
	t_success = true;
	
	if (t_success &&
		self -> token_pointer == self -> token_frontier)
		t_success = Throw(kErrorNoCurrentToken);
	
	if (t_success)
		r_token = &self -> token_buffer[self -> token_pointer];
		
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

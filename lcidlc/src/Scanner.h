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

#ifndef __SCANNER__
#define __SCANNER__

#ifndef __VALUE__
#include "Value.h"
#endif

#ifndef __POSITION__
#include "Position.h"
#endif

////////////////////////////////////////////////////////////////////////////////

enum TokenType
{
	kTokenTypeNone,
	kTokenTypeEnd,
	kTokenTypeIdentifier,
	kTokenTypeNumber,
	kTokenTypeString,
	kTokenTypeComma,
	
	kTokenTypeError,
	kTokenTypeInvalidCharError,
	kTokenTypeUnterminatedStringError,
	kTokenTypeNewlineInStringError,
};

struct Token
{
	TokenType type;
	Position start, finish;
	ValueRef value;
};

////////////////////////////////////////////////////////////////////////////////

typedef struct Scanner *ScannerRef;

bool ScannerCreate(const char *p_filename, ScannerRef& r_scanner);
void ScannerDestroy(ScannerRef scanner);

bool ScannerAdvance(ScannerRef scanner);
bool ScannerRetreat(ScannerRef scanner);

void ScannerMark(ScannerRef scanner);

bool ScannerRetrieve(ScannerRef scanner, const Token*& r_token); 

////////////////////////////////////////////////////////////////////////////////

#endif

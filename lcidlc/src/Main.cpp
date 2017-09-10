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

#include "Scanner.h"
#include "Parser.h"
#include "Interface.h"
#include "foundation-locale.h"

#define canvas_idl "/Users/mark/Workspace/revolution/externals/rrecanvas/rrecanvas.lcidl.txt"
#define error_idl "/Users/mark/Workspace/revolution/trunk/lcidlc/interface_error_test.txt"
#define test_idl "/Users/mark/Workspace/revolution/trunk/lcidlc/coverage_test.lcidl"

////////////////////////////////////////////////////////////////////////////////

// Improve XCode error output.
const char *g_input_filename = nil;

// SN-20140-07-02: [[ ExternalsApiV6 ]] Added needed extern variable in libfoundation...
MCLocaleRef kMCBasicLocale;

int main(int argc, char *argv[])
{
    MCInitialize();
    
	bool t_success;
	t_success = true;

	if (argc != 3)
	{
		fprintf(stderr, "Syntax: lcidlc <input file> <output file/folder>\n");
		return 1;
	}
	
	// Improve XCode error output.
	g_input_filename = argv[1];
    
	ScannerRef t_scanner;
	t_scanner = nil;
	if (t_success)
		t_success = ScannerCreate(argv[1], t_scanner);

	InterfaceRef t_interface;
	t_interface = nil;
	if (t_success)
		t_success = ParserRun(t_scanner, t_interface);
	
	if (t_success && t_interface != nil)
		t_success = InterfaceGenerate(t_interface, argv[2]);

#if DUMP_TOKENS
	while(t_success)
	{
		const Token *t_token;
		if (t_success)
			t_success = ScannerRetrieve(t_scanner, t_token);
			
		if (t_success)
		{
			ScannerMark(t_scanner);
			
			const char *t_string;
			if (t_token -> type == kTokenTypeString)
				t_string = StringGetCStringPtr(t_token -> value);
			else if (t_token -> type == kTokenTypeIdentifier)
				t_string = StringGetCStringPtr(NameGetString(t_token -> value));
			else
				t_string = "";
			
			fprintf(stderr, "%d (%d,%d)-(%d,%d) - %s\n",
						t_token -> type,
						PositionGetRow(t_token -> start), PositionGetColumn(t_token -> start),
						PositionGetRow(t_token -> finish), PositionGetColumn(t_token -> finish),
						t_string);
						
			if (t_token -> type == kTokenTypeEnd)
				break;
		}
		
		if (t_success)
			t_success = ScannerAdvance(t_scanner);
	}
#endif
	
	return t_success ? 0 : 1;
}

////////////////////////////////////////////////////////////////////////////////

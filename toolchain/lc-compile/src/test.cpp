/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
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

#include "foundation.h"
#include "foundation-auto.h"
#include "foundation-system.h"
#include "script.h"

extern "C" void MCStringExecPutStringAfter(void);
extern "C" void MCArithmeticExecAddIntegerToInteger(void);
extern "C" void MCCharFetchCharOf(void);
void dummy(void)
{
    MCStringExecPutStringAfter();
    MCArithmeticExecAddIntegerToInteger();
    MCCharFetchCharOf();
}

static MCScriptModuleRef load_module(const char *p_filename)
{
    FILE *t_file;
    t_file = fopen(p_filename, "rb");
    if (t_file == NULL)
        return NULL;
    
    fseek(t_file, 0, SEEK_END);
    long t_length;
    t_length = ftell(t_file);
    fseek(t_file, 0, SEEK_SET);
    
    void *t_mem;
    t_mem = malloc(t_length);
    fread(t_mem, t_length, 1, t_file);
    fclose(t_file);
    
    MCStreamRef t_stream;
    MCScriptModuleRef t_module;
    MCMemoryInputStreamCreate(t_mem, t_length, t_stream);
    if (!MCScriptCreateModuleFromStream(t_stream, t_module))
        t_module = NULL;
    MCValueRelease(t_stream);
    
    free(t_mem);
    
    return t_module;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "invalid arguments\n");
        exit(1);
    }
    
    MCInitialize();
    MCSInitialize();
    MCScriptInitialize();
    
    // Skip command arg.
    argc -= 1;
    argv += 1;
    
    MCScriptModuleRef t_module;
    for(int i = 0; i < argc; i++)
    {
        t_module = load_module(argv[i]);
        
        if (t_module == NULL ||
            !MCScriptEnsureModuleIsUsable(t_module))
        {
            fprintf(stderr, "'%s' failed to initialize\n", argv[i]);
            exit(1);
        }
    }
    
    // The last module we create an instance of and attempt to execute
    // a handler.
    bool t_success;
    t_success = true;
    
    MCScriptInstanceRef t_instance;
    if (t_success)
        t_success = MCScriptCreateInstanceOfModule(t_module, t_instance);
    
    MCValueRef t_result;
    if (t_success)
        t_success = MCScriptCallHandlerOfInstance(t_instance, MCNAME("test"), nil, 0, t_result);
    
	MCAutoStringRef t_message;
	MCErrorRef t_error;
	if (t_success)
	{
		/* UNCHECKED */ MCStringFormat (&t_message,
		                                "Executed test with result %@\n",
		                                t_result);
	}
	else if (MCErrorCatch (t_error))
	{
		/* UNCHECKED */ MCStringFormat (&t_message,
		                                "Failed to execute test: %@\n",
		                                MCErrorGetMessage (t_error));
	}
	else
	{
		/* UNCHECKED */ MCStringCopy(MCSTR("Failed to execute test: unknown error\n"),
		                             &t_message);
	}
	MCAutoStringRefAsCString t_sys;
	t_sys.Lock(*t_message);
	printf("%s", *t_sys);

    MCFinalize();
    
	return t_success ? 0 : 1;
}

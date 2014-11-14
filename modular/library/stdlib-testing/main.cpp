//
//  main.cpp
//  stdlib-testing
//
//  Created by Ali Lloyd on 22/10/2014.
//  Copyright (c) 2014 RunRev. All rights reserved.
//

#include <foundation.h>
#include <foundation-locale.h>

extern void MCListRunTests();
extern void MCMapRunTests();
extern void MCTypeRunTests();
extern void MCEncodingRunTests();
extern void MCSortRunTests();
extern void MCByteRunTests();

void log(const char *module, const char *testname, bool t_success)
{
    MCLog("%s:%s \n %s", module, testname, t_success ? "passed" : "                                                     failed");
}

int main(int argc, const char * argv[])
{
   	if (!MCInitialize())
		exit(-1);
    
    MCListRunTests();
    MCMapRunTests();
    MCTypeRunTests();
    MCEncodingRunTests();
    MCSortRunTests();
    MCByteRunTests();
    
    MCFinalize();
}


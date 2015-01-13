//
//  test.cpp
//  stdmlc
//
//  Created by Ali Lloyd on 22/10/2014.
//  Copyright (c) 2014 RunRev. All rights reserved.
//

#include "foundation.h"

int main(int argc, char **argv) {

   	if (!MCInitialize())
		exit(-1);
    
    MCLog("running", nil);
    
    MCFinalize();
}
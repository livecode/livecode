/* Copyright (C) 2003-2016 LiveCode Ltd.
 
 This file is part of LiveCode.
 
 All rights reserved.  */

#include "prefix.h"

#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "dispatch.h"

////////////////////////////////////////////////////////////////////////////////

bool MCStackSecurityCreateDispatch(MCDispatch*& r_dispatch)
{
    r_dispatch = new MCDispatch;
    return r_dispatch != nil;
}

////////////////////////////////////////////////////////////////////////////////

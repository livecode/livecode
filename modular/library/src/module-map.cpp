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

#include <foundation.h>

void MCMapEvalKeysOf(MCArrayRef p_target, MCProperListRef& r_output)
{
    
}

void MCMapEvalElementsOf(MCArrayRef p_target, MCProperListRef& r_output)
{
    
}

void MCMapEvalNumberOfElementsIn(MCArrayRef p_target, uindex_t& r_output)
{
    r_output = MCArrayGetCount(p_target);
}

void MCMapEvalIsAmongTheElementsOf(MCValueRef p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output)
{
    
}

void MCMapEvalIsAmongTheKeysOfCaseless(MCStringRef p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output)
{
    
}

void MCMapEvalIsAmongTheKeysOfNumeric(integer_t p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output)
{
    
}

void MCMapEvalIsAmongTheKeysOfMatrix(MCProperListRef p_needle, bool p_is_not, MCArrayRef p_target, bool& r_output)
{
    
}

void MCMapFetchElementOfCaseless(MCArrayRef p_target, MCStringRef p_key, MCValueRef& r_output)
{
    
}

void MCMapStoreElementOfCaseless(MCValueRef p_value, MCArrayRef& x_target, MCStringRef p_key)
{
    
}

void MCMapFetchElementOfNumeric(MCArrayRef p_target, integer_t p_key, MCValueRef& r_output)
{
    
}

void MCMapStoreElementOfNumeric(MCValueRef p_value, MCArrayRef& x_target, integer_t p_key)
{
    
}

void MCMapFetchElementOfMatrix(MCArrayRef p_target, MCProperListRef p_key, MCValueRef& r_output)
{
    
}

void MCMapStoreElementOfMatrix(MCValueRef p_value, MCArrayRef& x_target, MCProperListRef p_key)
{
    
}
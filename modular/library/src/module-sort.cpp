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

void MCSortExecSortListAscendingText(MCProperListRef& x_target)
{
    MCProperListSort(x_target, true, kMCProperListSortTypeText);
}

void MCSortExecSortListDescendingText(MCProperListRef& x_target)
{
    MCProperListSort(x_target, false, kMCProperListSortTypeText);
}

void MCSortExecSortListAscendingBinary(MCProperListRef& x_target)
{
    MCProperListSort(x_target, true, kMCProperListSortTypeBinary);
}

void MCSortExecSortListDescendingBinary(MCProperListRef& x_target)
{
    MCProperListSort(x_target, false, kMCProperListSortTypeBinary);
}

void MCSortExecSortListAscendingNumeric(MCProperListRef& x_target)
{
    MCProperListSort(x_target, true, kMCProperListSortTypeNumeric);
}

void MCSortExecSortListDescendingNumeric(MCProperListRef& x_target)
{
    MCProperListSort(x_target, false, kMCProperListSortTypeNumeric);
}

void MCSortExecSortListAscendingDateTime(MCProperListRef& x_target)
{
    MCProperListSort(x_target, true, kMCProperListSortTypeDateTime);
}

void MCSortExecSortListDescendingDateTime(MCProperListRef& x_target)
{
    MCProperListSort(x_target, false, kMCProperListSortTypeDateTime);
}

/*

void MCSortExecSortStringListAscendingNumeric(MCProperListRef<MCStringRef>& x_target)
{
    MCProperListSort(x_target, true, kMCProperListSortTypeNumeric);
}
 
void MCSortExecSortStringListDescendingNumeric(MCProperListRef<MCStringRef>& x_target)
{
    MCProperListSort(x_target, false, kMCProperListSortTypeNumeric);
}
 
*/
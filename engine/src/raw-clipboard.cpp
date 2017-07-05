/* Copyright (C) 2015 LiveCode Ltd.
 
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


#include "raw-clipboard.h"
#include "globdefs.h"

#include "foundation-auto.h"

MCRawClipboard::~MCRawClipboard()
{
    ;
}


MCRawClipboardItem::~MCRawClipboardItem()
{
    ;
}

bool MCRawClipboardItem::HasRepresentation(MCStringRef p_type) const
{
#if !defined(_MAC_DESKTOP) && !defined(_MAC_SERVER)
    // Check that the representation string is valid
    if (p_type == NULL)
        return false;
    
    // Loop over the representations of this item and test each one
    for (uindex_t i = 0; i < GetRepresentationCount(); i++)
    {
        MCAutoStringRef t_type(FetchRepresentationAtIndex(i)->CopyTypeString());
        if (MCStringIsEqualTo(p_type, *t_type, kMCStringOptionCompareExact))
            return true;
    }
#endif
    return false;
}

const MCRawClipboardItemRep* MCRawClipboardItem::FetchRepresentationByType(MCStringRef p_type) const
{
#if !defined(_MAC_DESKTOP) && !defined(_MAC_SERVER)
    // Check that the representation string is valid
    if (p_type == NULL)
        return NULL;
    
    // Loop over the representations of this item and test each one
    for (uindex_t i = 0; i < GetRepresentationCount(); i++)
    {
        MCAutoStringRef t_type(FetchRepresentationAtIndex(i)->CopyTypeString());
        if (MCStringIsEqualTo(p_type, *t_type, kMCStringOptionCompareExact))
            return FetchRepresentationAtIndex(i);
    }
#endif
    
    return NULL;
}

MCRawClipboardItemRep::~MCRawClipboardItemRep()
{
    ;
}

MCDataRef MCRawClipboard::EncodeBMPForTransfer(MCDataRef p_bmp) const
{
#if !defined(_MAC_DESKTOP) && !defined(_MAC_SERVER)
	// Most platforms transfer BMP files without any further transformations
	return MCValueRetain(p_bmp);
#endif
    
    return NULL;
}

MCDataRef MCRawClipboard::DecodeTransferredBMP(MCDataRef p_bmp) const
{
#if !defined(_MAC_DESKTOP) && !defined(_MAC_SERVER)
	// Most platforms transfer BMP files without any further transformations
	return MCValueRetain(p_bmp);
#endif
    
    return NULL;
}



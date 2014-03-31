/* Copyright (C) 2014 Runtime Revolution Ltd.
 
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


#include "foundation-text.h"

#include "foundation-auto.h"


void MCTextFilter::AcceptText()
{
    // All filters other than the first just pass on the message
    m_Prev->AcceptText();
}

uindex_t MCTextFilter::GetAcceptedIndex() const
{
    // All filters other than the first just pass on the message
    return m_Prev->GetAcceptedIndex();
}

MCTextFilter::~MCTextFilter()
{
    if (m_Next)
    {
        m_Next->m_Prev = nil;
        delete m_Next;
    }
    if (m_Prev)
    {
        m_Prev->m_Next = nil;
        delete m_Prev;
    }
}

bool MCTextFilter::PlaceBefore(MCTextFilter* p_filter)
{
    p_filter->m_Prev = this;
    m_Next = p_filter;
    return true;
}

bool MCTextFilter::PlaceAfter(MCTextFilter* p_filter)
{
    p_filter->m_Next = this;
    m_Prev = p_filter;
    return true;
}

MCTextFilter* MCTextFilter::NextFilter() const
{
    return m_Next;
}

MCTextFilter* MCTextFilter::PrevFilter() const
{
    return m_Prev;
}

MCTextFilter::MCTextFilter()
  : m_Next(nil), m_Prev(nil)
{
    ;
}

bool MCTextFilter_Encoder::PlaceBefore(MCTextFilter*)
{
    return false;
}

bool MCTextFilter_Decoder::PlaceAfter(MCTextFilter*)
{
    return false;
}


codepoint_t MCTextFilter_DecodeUTF16::GetNextCodepoint()
{
    // Read the first codeunit and check whether it is a leading surrogate half
    unichar_t t_lead = (m_ReadIndex < m_DataLength) ? m_Data[m_ReadIndex] : -1;
    if (t_lead < 0xD800 || t_lead >= 0xDC00)
        return t_lead;
    
    // Read another codeunit to check whether it is an trailing surrogate half
    unichar_t t_trail = (m_ReadIndex + 1 < m_DataLength) ? m_Data[m_ReadIndex+1] : -1;
    if (t_trail < 0xDC00 || t_trail >= 0xE000)
        return t_lead;
    
    // Valid surrogate pair
    m_surrogate = true;
    return MCUnicodeSurrogatesToCodepoint(t_lead, t_trail);
}

bool MCTextFilter_DecodeUTF16::AdvanceCursor()
{
    if (m_surrogate)
        m_ReadIndex += 2;
    else
        m_ReadIndex += 1;
    m_surrogate = false;
    
    return m_ReadIndex < m_DataLength;
}

bool MCTextFilter_DecodeUTF16::HasData() const
{
    return m_ReadIndex < m_DataLength;
}

void MCTextFilter_DecodeUTF16::AcceptText()
{
    m_AcceptedIndex = m_ReadIndex;
}

uindex_t MCTextFilter_DecodeUTF16::GetAcceptedIndex() const
{
    return m_AcceptedIndex;
}

MCTextFilter_DecodeUTF16::MCTextFilter_DecodeUTF16(const unichar_t *p_text, uindex_t p_length)
  : m_surrogate(false), m_AcceptedIndex(0), m_ReadIndex(0), m_Data(p_text), m_DataLength(p_length)
{
    ;
}

MCTextFilter_DecodeUTF16::~MCTextFilter_DecodeUTF16()
{
    ;
}


codepoint_t MCTextFilter_SimpleCaseFold::GetNextCodepoint()
{
    // Get a codepoint from the preceding filter
    codepoint_t t_raw;
    t_raw = PrevFilter()->GetNextCodepoint();
    
    // Case fold the codepoint
    codepoint_t t_folded;
    t_folded = MCUnicodeGetCharacterProperty(t_raw, kMCUnicodePropertySimpleCaseFolding);
    
    return t_folded;
}

bool MCTextFilter_SimpleCaseFold::AdvanceCursor()
{
    // If toplevel, accept all preceding text
    if (NextFilter() == nil)
        AcceptText();
    
    return PrevFilter()->AdvanceCursor();
}

bool MCTextFilter_SimpleCaseFold::HasData() const
{
    return PrevFilter()->HasData();
}

MCTextFilter_SimpleCaseFold::MCTextFilter_SimpleCaseFold()
{
    ;
}

MCTextFilter_SimpleCaseFold::~MCTextFilter_SimpleCaseFold()
{
    ;
}


codepoint_t MCTextFilter_NormalizeNFC::GetNextCodepoint()
{
    // If possible, return a codepoint from the cached state
    if (m_ReadIndex < m_StateLength)
        return m_State[m_ReadIndex];
    
    // Need to refresh therefore all text has been accepted
    if (NextFilter() == nil)
        AcceptText();
    
    // Otherwise, the state needs to be refreshed. Loop until we get to a
    // normalisation boundary (i.e a base character)
    codepoint_t t_cp;
    m_StateLength = 0;
    while (PrevFilter()->HasData())
    {
        t_cp = PrevFilter()->GetNextCodepoint();
        
        // The first character is always added to the state
        if (m_StateLength == 0)
        {
            m_State[m_StateLength++] = t_cp;
            PrevFilter()->AdvanceCursor();
        }
        // Non-first grapheme base characters terminate the run
        else if (MCUnicodeGetBinaryProperty(t_cp, kMCUnicodePropertyGraphemeBase))
        {
            break;
        }
        // All other characters are appended to the state
        else
        {
            m_State[m_StateLength++] = t_cp;
            PrevFilter()->AdvanceCursor();
        }
        
        // Abort if our arbitrary limit has been reached
        if (m_StateLength == kMCTextFilterMaxNormLength)
            break;
    }
    
    // Normalise the state
    unichar_t *t_norm;
    uindex_t t_norm_length;
    MCUnicodeNormaliseNFC(m_State, m_StateLength, t_norm, t_norm_length);
    
    // Copy the normalised state to the internal state (note: we assume that
    // composing will never create a longer string)
    memcpy(m_State, t_norm, t_norm_length);
    m_StateLength = t_norm_length;
    m_ReadIndex = 0;
    delete[] t_norm;
    
    // All done
    return m_State[0];
}

bool MCTextFilter_NormalizeNFC::AdvanceCursor()
{
    m_ReadIndex++;
}

bool MCTextFilter_NormalizeNFC::HasData() const
{
    return m_ReadIndex < m_StateLength || PrevFilter()->HasData();
}

MCTextFilter_NormalizeNFC::MCTextFilter_NormalizeNFC()
  : m_StateLength(0), m_ReadIndex(0)
{
    ;
}

MCTextFilter_NormalizeNFC::~MCTextFilter_NormalizeNFC()
{
    ;
}

MCTextFilter* MCTextFilterCreate(MCStringRef p_string, MCStringOptions p_options)
{
    return MCTextFilterCreate(MCStringGetCharPtr(p_string), MCStringGetLength(p_string), kMCStringEncodingUTF16, p_options);
}

MCTextFilter* MCTextFilterCreate(MCDataRef p_data, MCStringEncoding p_encoding, MCStringOptions p_options)
{
    return MCTextFilterCreate(MCDataGetBytePtr(p_data), MCDataGetLength(p_data), p_encoding, p_options);
}

MCTextFilter* MCTextFilterCreate(const void *p_data, uindex_t p_length, MCStringEncoding p_encoding, MCStringOptions p_options)
{
    MCTextFilter *t_chain = nil;
    
    // Choose the decoder based on the encoding
    if (p_encoding == kMCStringEncodingUTF16)
        t_chain = new MCTextFilter_DecodeUTF16(reinterpret_cast<const unichar_t*>(p_data), p_length);
    else
        return nil;
    
    // Add filters based on the options given
    if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
    {
        MCTextFilter *t_filter;
        t_filter = new MCTextFilter_SimpleCaseFold();
        t_chain->PlaceBefore(t_filter);
        t_chain = t_filter;
    }
    
    if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareNonliteral)
    {
        MCTextFilter *t_filter;
        t_filter = new MCTextFilter_NormalizeNFC();
        t_chain->PlaceBefore(t_filter);
        t_chain = t_filter;
    }
    
    return t_chain;
}

void MCTextFilterRelease(MCTextFilter *t_chain)
{
    delete t_chain;
}

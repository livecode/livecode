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


#include "foundation-text.h"

#include "foundation-auto.h"


void MCTextFilter::MarkText()
{
    // All filters other than the first just pass on the message
    m_Prev->MarkText();
}

uindex_t MCTextFilter::GetMarkedLength() const
{
    // All filters other than the first just pass on the message
    return m_Prev->GetMarkedLength();
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
    if (!m_Reverse)
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
    
    // Read the first codeunit from end and check whether it is a trailing surrogate half
    unichar_t t_trail = (m_ReadIndex < m_DataLength) ? m_Data[m_DataLength - m_ReadIndex - 1] : -1;
    if (t_trail < 0xDC00 || t_trail >= 0xE000)
        return t_trail;
    
    // Read another codeunit to check whether it is an trailing surrogate half
    unichar_t t_lead = (m_ReadIndex + 1 < m_DataLength) ? m_Data[m_DataLength - m_ReadIndex - 2] : -1;
    if (t_lead < 0xD800 || t_lead >= 0xDC00)
        return t_trail;
    
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

void MCTextFilter_DecodeUTF16::MarkText()
{
    m_AcceptedIndex = m_ReadIndex;
}

uindex_t MCTextFilter_DecodeUTF16::GetMarkedLength() const
{
    return m_AcceptedIndex + 1;
}

MCTextFilter_DecodeUTF16::MCTextFilter_DecodeUTF16(const unichar_t *p_text, uindex_t p_length, bool p_from_end)
  : m_surrogate(false), m_AcceptedIndex(-1), m_ReadIndex(0), m_Data(p_text), m_DataLength(p_length), m_Reverse(p_from_end)
{
    ;
}

MCTextFilter_DecodeUTF16::~MCTextFilter_DecodeUTF16()
{
    ;
}

codepoint_t MCTextFilter_DecodeNative::GetNextCodepoint()
{
    // Don't read beyond the end of the input if no data remains
    if (m_ReadIndex >= m_DataLength)
        return 0;
    
    if (m_Reverse)
        return MCUnicodeMapFromNative(m_Data[m_DataLength - m_ReadIndex - 1]);
    
    return MCUnicodeMapFromNative(m_Data[m_ReadIndex]);
}

bool MCTextFilter_DecodeNative::AdvanceCursor()
{
    return ++m_ReadIndex < m_DataLength;
}

bool MCTextFilter_DecodeNative::HasData() const
{
    return m_ReadIndex < m_DataLength;
}

void MCTextFilter_DecodeNative::MarkText()
{
    m_AcceptedIndex = m_ReadIndex;
}

uindex_t MCTextFilter_DecodeNative::GetMarkedLength() const
{
    return m_AcceptedIndex + 1;
}

MCTextFilter_DecodeNative::MCTextFilter_DecodeNative(const char_t *p_text, uindex_t p_length, bool p_from_end)
: m_Data(p_text), m_DataLength(p_length), m_AcceptedIndex(-1), m_ReadIndex(0), m_Reverse(p_from_end)
{
    ;
}

MCTextFilter_DecodeNative::~MCTextFilter_DecodeNative()
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
    if (m_Reverse)
        return GetNextCodepointReverse();
    
    // If possible, return a codepoint from the cached state
    if (m_ReadIndex < m_StateLength)
    {
        // AL-2014-10-23: [[ Bug 13762 ]] Update the mark point when fetching codepoints from the state
        m_MarkPoint++;
        // Check whether we have a surrogate pair
        // We are sure to have the following trail surrogate if we find a lead surrogate
        if (m_State[m_ReadIndex] > 0xD800 && m_State[m_ReadIndex] < 0xDBFF)
        {
            m_MarkPoint++;
            m_surrogate = true;
            return MCUnicodeSurrogatesToCodepoint(m_State[m_ReadIndex], m_State[m_ReadIndex + 1]);
        }
        return m_State[m_ReadIndex];
    }
    
    PrevFilter()->MarkText();
    m_MarkPoint = PrevFilter()->GetMarkedLength();
    
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
            // Check whether the codepoint we got is a surrogate pair
            if (MCUnicodeCodepointToSurrogates(t_cp, m_State[m_StateLength],
                                               m_State[m_StateLength + 1]))
                m_StateLength++;
            
            m_StateLength++;
            
            PrevFilter()->AdvanceCursor();
        }
        // Non-first grapheme base characters terminate the run
        else if (MCUnicodeGetBinaryProperty(t_cp, kMCUnicodePropertyGraphemeBase) || MCUnicodeGetBinaryProperty(t_cp, kMCUnicodePropertyWhiteSpace))
        {
            break;
        }
        // All other characters are appended to the state
        else
        {
            // Check whether the codepoint we got is a surrogate pair
            if (MCUnicodeCodepointToSurrogates(t_cp, m_State[m_StateLength],
                                               m_State[m_StateLength+1]))
                m_StateLength++;
            
            m_StateLength++;
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
    MCMemoryCopy(m_State, t_norm, t_norm_length * sizeof(unichar_t));
    m_StateLength = t_norm_length;
    m_ReadIndex = 0;
	free (t_norm);
    
    // All done
    if (m_State[0] > 0xD800 && m_State[0] < 0xDBFF)
    {
        m_surrogate = true;
        return MCUnicodeSurrogatesToCodepoint(m_State[0], m_State[1]);
    }
    else
        return m_State[0];
}

codepoint_t MCTextFilter_NormalizeNFC::GetNextCodepointReverse()
{
    // If possible, return a codepoint from the cached state
    if (m_ReadIndex < m_StateLength)
    {
        // AL-2014-10-23: [[ Bug 13762 ]] Update the mark point when fetching codepoints from the state
        m_MarkPoint++;
        // We are sure to have the preceding lead surrogate if we find a trail surrogate
        if (m_State[kMCTextFilterMaxNormLength - m_ReadIndex - 1] >= 0xDC00 && m_State[kMCTextFilterMaxNormLength - m_ReadIndex - 1] < 0xE000)
        {
            m_MarkPoint++;
            m_surrogate = true;
            return MCUnicodeSurrogatesToCodepoint(m_State[kMCTextFilterMaxNormLength - m_ReadIndex - 2], m_State[kMCTextFilterMaxNormLength - m_ReadIndex - 1]);
        }
        return m_State[kMCTextFilterMaxNormLength - m_ReadIndex - 1];
    }
    
    PrevFilter()->MarkText();
    m_MarkPoint = PrevFilter()->GetMarkedLength();
    
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
            // Check whether the codepoint we got is a surrogate pair
            if (MCUnicodeCodepointToSurrogates(t_cp,
                                               m_State[kMCTextFilterMaxNormLength - 2],
                                               m_State[kMCTextFilterMaxNormLength - 1]))
                m_StateLength++;
            else
                m_State[kMCTextFilterMaxNormLength - 1] = m_State[kMCTextFilterMaxNormLength - 2];
            
            m_StateLength++;
            
            PrevFilter()->AdvanceCursor();
        }
        // All other characters are appended to the state
        else
        {
            // Check whether the codepoint we got is a surrogate pair
            if (MCUnicodeCodepointToSurrogates(t_cp,
                                               m_State[kMCTextFilterMaxNormLength - m_StateLength - 2],
                                               m_State[kMCTextFilterMaxNormLength - m_StateLength - 1]))
                
                m_StateLength++;
            else
                m_State[kMCTextFilterMaxNormLength - m_StateLength - 1] = m_State[kMCTextFilterMaxNormLength - m_StateLength - 2];
            
            m_StateLength++;
            PrevFilter()->AdvanceCursor();
        }
    
        // The backwards run is terminated after a grapheme base.
        if (MCUnicodeGetBinaryProperty(t_cp, kMCUnicodePropertyGraphemeBase) || MCUnicodeGetBinaryProperty(t_cp, kMCUnicodePropertyWhiteSpace))
        {
            break;
        }
        // Abort if our arbitrary limit has been reached
        if (m_StateLength == kMCTextFilterMaxNormLength)
            break;
    }
    
    // Normalise the state
    unichar_t *t_norm;
    uindex_t t_norm_length;
    MCUnicodeNormaliseNFC(m_State + kMCTextFilterMaxNormLength - m_StateLength, m_StateLength, t_norm, t_norm_length);
    
    // Copy the normalised state to the internal state (note: we assume that
    // composing will never create a longer string)
    uindex_t i = 0;
    while (t_norm_length)
        m_State[kMCTextFilterMaxNormLength - t_norm_length--] = t_norm[i++];
    m_StateLength = i;
    m_ReadIndex = 0;
	free (t_norm);
    
    // All done
    if (m_StateLength > 1 && m_State[kMCTextFilterMaxNormLength - 2] > 0xD800 && m_State[kMCTextFilterMaxNormLength - 2] < 0xDBFF)
    {
        m_surrogate = true;
        return MCUnicodeSurrogatesToCodepoint(m_State[kMCTextFilterMaxNormLength - 2], m_State[kMCTextFilterMaxNormLength - 1]);
    }
    else
        return m_State[kMCTextFilterMaxNormLength - 1];
}

bool MCTextFilter_NormalizeNFC::AdvanceCursor()
{
    m_ReadIndex++;
    if (m_surrogate)
    {
        m_ReadIndex++;
        m_surrogate = false;
    }
    
	return HasData();
}

bool MCTextFilter_NormalizeNFC::HasData() const
{
    return m_ReadIndex < m_StateLength || PrevFilter()->HasData();
}

void MCTextFilter_NormalizeNFC::MarkText()
{
    // Only mark on run boundaries
    m_MarkedLength = m_MarkPoint;
}

uindex_t MCTextFilter_NormalizeNFC::GetMarkedLength() const
{
    return m_MarkedLength;
}

MCTextFilter_NormalizeNFC::MCTextFilter_NormalizeNFC(bool p_from_end)
  : m_StateLength(0), m_ReadIndex(0), m_MarkedLength(0), m_MarkPoint(0), m_surrogate(false), m_Reverse(p_from_end)
{
    ;
}

MCTextFilter_NormalizeNFC::~MCTextFilter_NormalizeNFC()
{
    ;
}

MCTextFilter* MCTextFilterCreate(MCStringRef p_string, MCStringOptions p_options)
{
    if (MCStringIsNative(p_string))
        return MCTextFilterCreate(MCStringGetNativeCharPtr(p_string), MCStringGetLength(p_string), kMCStringEncodingNative, p_options, false);
    
    return MCTextFilterCreate(MCStringGetCharPtr(p_string), MCStringGetLength(p_string), kMCStringEncodingUTF16, p_options, false);
}

MCTextFilter* MCTextFilterCreate(MCDataRef p_data, MCStringEncoding p_encoding, MCStringOptions p_options)
{
    return MCTextFilterCreate(MCDataGetBytePtr(p_data), MCDataGetLength(p_data), p_encoding, p_options, false);
}

MCTextFilter* MCTextFilterCreate(const void *p_data, uindex_t p_length, MCStringEncoding p_encoding, MCStringOptions p_options, bool p_from_end)
{
    MCTextFilter *t_chain = nil;
    
    // Choose the decoder based on the encoding
    if (p_encoding == kMCStringEncodingUTF16)
        t_chain = new (nothrow) MCTextFilter_DecodeUTF16(reinterpret_cast<const unichar_t*>(p_data), p_length, p_from_end);
    else
        t_chain = new (nothrow) MCTextFilter_DecodeNative(reinterpret_cast<const char_t*>(p_data), p_length, p_from_end);
    
    // Add filters based on the options given
    if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
    {
        MCTextFilter *t_filter;
        t_filter = new (nothrow) MCTextFilter_SimpleCaseFold();
        t_chain->PlaceBefore(t_filter);
        t_chain = t_filter;
    }
    
    if (p_encoding == kMCStringEncodingUTF16 && (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareNonliteral))
    {
        MCTextFilter *t_filter;
        t_filter = new (nothrow) MCTextFilter_NormalizeNFC(p_from_end);
        t_chain->PlaceBefore(t_filter);
        t_chain = t_filter;
    }
    
    return t_chain;
}


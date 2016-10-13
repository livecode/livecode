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

#ifndef __MC_FOUNDATION_TEXT_
#define __MC_FOUNDATION_TEXT_


#include "foundation.h"
#include "foundation-unicode.h"
#include "foundation-locale.h"


class MCTextFilter
{
public:
    
    // Returns the next codepoint
    virtual codepoint_t GetNextCodepoint() = 0;
    
    // Advances the read cursor or returns false if no more data remains
    virtual bool AdvanceCursor() = 0;
    
    // Returns true if there is still data to be read
    virtual bool HasData() const = 0;
    
    // Marks the codepoints read up to this point as being accepted (this is
    // used by comparisons to find the range of indices that match). Filters
    // that have multiple codepoints of state should only mark on boundaries.
    virtual void MarkText();
    
    // Returns the index into the underlying data that has been accepted
    virtual uindex_t GetMarkedLength() const;
    
    // Destructor also destroys all connected filters (i.e the entire chain is
    // destroyed at one) - this is to simplify filter chain management.
    virtual ~MCTextFilter();
    
    // Filter chaining. Not all filters stack fully: encoding filters can't
    // have anything added after them and decoding filters can't have anything
    // added before them.
    virtual bool PlaceBefore(MCTextFilter* p_filter);
    virtual bool PlaceAfter(MCTextFilter* p_filter);
    
    // Chain information
    MCTextFilter *NextFilter() const;
    MCTextFilter *PrevFilter() const;

protected:
    
    MCTextFilter();
    
private:
    
    // Filter chain
    MCTextFilter *m_Next, *m_Prev;
};


class MCTextFilter_Encoder : public MCTextFilter
{
public:

    // Must always come last so always fails
    virtual bool PlaceBefore(MCTextFilter* p_filter);
};


class MCTextFilter_Decoder : public MCTextFilter
{
public:
    
    // Must always come first so always fails
    virtual bool PlaceAfter(MCTextFilter* p_filter);
};


class MCTextFilter_Breaker : public MCTextFilter
{
private:
    
    // Break iterator being used
    MCBreakIteratorRef m_BreakIterator;
};


class MCTextFilter_DecodeUTF16 : public MCTextFilter_Decoder
{
public:
    
    // Inherited from MCTextFilter
    virtual codepoint_t GetNextCodepoint();
    virtual bool AdvanceCursor();
    virtual bool HasData() const;
    virtual void MarkText();
    virtual uindex_t GetMarkedLength() const;
    
    MCTextFilter_DecodeUTF16(const unichar_t*, uindex_t, bool);
    ~MCTextFilter_DecodeUTF16();
    
private:
    
    // Flag to indicate whether we need to advance 2 codeunits
    bool m_surrogate;
    
    // Accepted and reading indices into the code units
    uindex_t m_AcceptedIndex, m_ReadIndex;
    
    // Text storage
    const unichar_t *m_Data;
    uindex_t m_DataLength;
    
    // Going backwards, for things like shared suffix
    bool m_Reverse;
};

class MCTextFilter_DecodeNative : public MCTextFilter_Decoder
{
public:
    
    // Inherited from MCTextFilter
    virtual codepoint_t GetNextCodepoint();
    virtual bool AdvanceCursor();
    virtual bool HasData() const;
    virtual void MarkText();
    virtual uindex_t GetMarkedLength() const;
    
    MCTextFilter_DecodeNative(const char_t*, uindex_t, bool);
    ~MCTextFilter_DecodeNative();
    
private:

    // Text storage
    const char_t *m_Data;
    uindex_t m_DataLength;
    
    // Accepted and reading indices into the code units
    uindex_t m_AcceptedIndex, m_ReadIndex;
    
    // Going backwards, for things like shared suffix
    bool m_Reverse;
};

class MCTextFilter_EncodeUTF16 : public MCTextFilter_Encoder
{
private:
    
    // Non-zero value is trailing surrogate to emit next
    unichar_t m_TrailSurrogate;
};


class MCTextFilter_SimpleCaseFold : public MCTextFilter
{
public:
    
    // Inherited from MCTextFilter
    virtual codepoint_t GetNextCodepoint();
    virtual bool AdvanceCursor();
    virtual bool HasData() const;
    
    MCTextFilter_SimpleCaseFold();
    ~MCTextFilter_SimpleCaseFold();
    
private:
    
    // This class uses the "simple" case folding rules where each character
    // folds to exactly one character (so no sharp-s -> SS -> ss). As such, it
    // doesn't need to maintain any state.
};


class MCTextFilter_NormalizeNFC : public MCTextFilter
{
public:
    
    // Inherited from MCTextFilter
    virtual codepoint_t GetNextCodepoint();
    virtual bool AdvanceCursor();
    virtual bool HasData() const;
    virtual void MarkText();
    virtual uindex_t GetMarkedLength() const;
    
    MCTextFilter_NormalizeNFC(bool);
    ~MCTextFilter_NormalizeNFC();
    
private:
    
    // The amount of context neaded for normalisation is potentially unbounded.
    // To avoid problems with this, we implement the same fudge as libICU:
    // arbitrarily limit the length of normalisable sequences to 256 codepoints.
    enum { kMCTextFilterMaxNormLength = 256 };
    unichar_t m_State[kMCTextFilterMaxNormLength];
    
    // The length of state currently stored
    uindex_t m_StateLength;
    
    // Cursor
    uindex_t m_ReadIndex;
    
    // Marked length
    uindex_t m_MarkedLength;
    uindex_t m_MarkPoint;
    
    bool m_surrogate;
    
    // Going backwards, for things like shared suffix
    bool m_Reverse;
    codepoint_t GetNextCodepointReverse();
};


// Utility functions for creating filter chains - the returned objects should
// be released by calling 'delete'.
MCTextFilter *MCTextFilterCreate(MCStringRef, MCStringOptions);
MCTextFilter *MCTextFilterCreate(MCDataRef, MCStringEncoding, MCStringOptions);
MCTextFilter *MCTextFilterCreate(const void *, uindex_t, MCStringEncoding, MCStringOptions, bool from_end = false);

#endif

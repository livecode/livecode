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

#include "foundation-bidi.h"
#include "foundation-unicode.h"
#include "foundation-auto.h"

enum bidi_directional_override_status
{
    kMCBidiOverrideNeutral = 0,
    kMCBidiOverrideLTR,
    kMCBidiOverrideRTL,
};

struct bidi_stack_entry
{
    uint8_t level;
    bidi_directional_override_status override;
    bool isolate;
    
    void clear()
    {
        level = 0;
        override = kMCBidiOverrideNeutral;
        isolate = false;
    }
};

struct level_run
{
    uindex_t start, length;
    uindex_t irs;
    level_run *next_run, *prev_run;
};

struct isolating_run_sequence
{
    uint8_t sos, eos;
    level_run *first_run, *last_run;
};

static bool bidiIncrementISRIndex(uint8_t *classes, level_run*& x_run, uindex_t &x_index)
{
    if (x_run == nil)
        return false;
    
    if (++x_index < x_run->start + x_run->length)
    {
        // Ignore BNs
        if (classes[x_index] == kMCUnicodeDirectionBoundaryNeutral)
            return bidiIncrementISRIndex(classes, x_run, x_index);
        return true;
    }
    
    x_run = x_run -> next_run;
    if (x_run == nil)
        return false;
    
    x_index = x_run -> start - 1;
    return bidiIncrementISRIndex(classes, x_run, x_index);
}

static uint8_t bidiPeekNextISRCharClass(uint8_t *classes, uint8_t alternative, level_run* p_run, uindex_t p_index)
{
    if (!bidiIncrementISRIndex(classes, p_run, p_index))
        return alternative;
    return classes[p_index];
}

static bool bidiDecrementISRIndex(uint8_t *classes, level_run*& x_run, uindex_t& x_index)
{
    if (x_run == nil || x_index == 0)
        return false;
    
    // AL-2014-06-24: [[ Bug 12343 ]] Runs are 0-indexed.
    if (--x_index < x_run -> start)
    {
        x_run = x_run -> prev_run;
        if (x_run != nil)
            x_index = x_run -> start + x_run -> length - 1;
    }
    
    if (x_run == nil)
        return false;
    
    // Ignore BNs
    if (classes[x_index] == kMCUnicodeDirectionBoundaryNeutral)
        return bidiDecrementISRIndex(classes, x_run, x_index);
    
    return true;
}

static uint8_t bidiPeekPrevISRCharClass(uint8_t *classes, uint8_t alternative, level_run* p_run, uindex_t p_index)
{
    if (!bidiDecrementISRIndex(classes, p_run, p_index))
        return alternative;
    return classes[p_index];
}

static void bidiApplyRuleW1(isolating_run_sequence& irs, uint8_t *classes)
{
    // ---- RULE W1 -----
    // Non-spacing marks
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    do
    {
        t_class = classes[t_index];
        if (t_class == kMCUnicodeDirectionNonSpacingMark)
        {
            uint8_t t_before = bidiPeekPrevISRCharClass(classes, irs.sos, t_run, t_index);
            if (t_before == kMCUnicodeDirectionRightToLeftIsolate
                || t_before == kMCUnicodeDirectionLeftToRightIsolate
                || t_before == kMCUnicodeDirectionFirstStrongIsolate
                || t_before == kMCUnicodeDirectionPopDirectionalIsolate)
            {
                classes[t_index] = kMCUnicodeDirectionOtherNeutral;
            }
            else
            {
                classes[t_index] = t_before;
            }
        }
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static void bidiApplyRuleW2(isolating_run_sequence& irs, uint8_t *classes)
{
    // ----- RULE W2 -----
    // Search backwards from European numbers for strong types
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    do
    {
        t_class = classes[t_index];
        if (t_class == kMCUnicodeDirectionEuropeanNumber)
        {
            uint8_t t_strong;
            level_run *t_run_b = t_run;
            uindex_t t_index_b = t_index;
            bool t_valid;
            do
            {
                t_valid = bidiDecrementISRIndex(classes, t_run_b, t_index_b);
                if (t_valid)
                    t_strong = classes[t_index_b];
            }
            while (t_valid
                   && t_strong != kMCUnicodeDirectionRightToLeft
                   && t_strong != kMCUnicodeDirectionLeftToRight
                   && t_strong != kMCUnicodeDirectionRightToLeftArabic);
            
            if (!t_valid)
                t_strong = irs.sos;
            
            if (t_strong == kMCUnicodeDirectionRightToLeftArabic)
                classes[t_index] = kMCUnicodeDirectionArabicNumber;
        }
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static void bidiApplyRuleW3(isolating_run_sequence& irs, uint8_t *classes)
{
    // ----- RULE W3 -----
    // Change all Arabic Letters to RTL
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    do
    {
        t_class = classes[t_index];
        if (t_class == kMCUnicodeDirectionRightToLeftArabic)
            classes[t_index] = kMCUnicodeDirectionRightToLeft;
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static void bidiApplyRuleW4(isolating_run_sequence& irs, uint8_t *classes)
{
    // ----- RULE W4 -----
    // EN ES EN -> EN EN EN
    // EN CS EN -> EN EN EN
    // AN CS EN -> AN AN AN
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    uint8_t t_prev_class = irs.sos;
    do
    {
        uint8_t t_peek;
        t_class = classes[t_index];
        t_peek = bidiPeekNextISRCharClass(classes, irs.eos, t_run, t_index);
        
        if (t_class == kMCUnicodeDirectionEuropeanNumberSeparator)
        {
            if (t_prev_class == kMCUnicodeDirectionEuropeanNumber
                && t_peek == kMCUnicodeDirectionEuropeanNumber)
            {
                classes[t_index] = kMCUnicodeDirectionEuropeanNumber;
            }
        }
        
        else if (t_class == kMCUnicodeDirectionCommonNumberSeparator)
        {
            if (t_prev_class == kMCUnicodeDirectionEuropeanNumber
                && t_peek == kMCUnicodeDirectionEuropeanNumber)
            {
                classes[t_index] = kMCUnicodeDirectionEuropeanNumber;
            }
            
            else if (t_prev_class == kMCUnicodeDirectionArabicNumber
                     && t_peek == kMCUnicodeDirectionArabicNumber)
            {
                classes[t_index] = kMCUnicodeDirectionArabicNumber;
            }
        }
        
        t_prev_class = t_class;
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static void bidiApplyRuleW5(isolating_run_sequence& irs, uint8_t *classes)
{
    // ----- RULE W5 -----
    // Sequences of ET adjacent to EN turn into EN
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    bool t_in_en = false;
    do
    {
        t_class = classes[t_index];
        
        // Are we already expanding an EN?
        if (t_in_en)
        {
            if (t_class == kMCUnicodeDirectionEuropeanNumber)
                ;
            else if (t_class == kMCUnicodeDirectionEuropeanNumberTerminator)
                classes[t_index] = kMCUnicodeDirectionEuropeanNumber;
            else
                t_in_en = false;
            
        }
        else if (t_class == kMCUnicodeDirectionEuropeanNumberTerminator)
        {
            // Scan along a run of European number terminators
            uindex_t t_start = t_index;
            level_run *t_start_run = t_run;
            bool t_valid = true;
            while (t_class == kMCUnicodeDirectionEuropeanNumberTerminator
                   && (t_valid = bidiIncrementISRIndex(classes, t_run, t_index)))
                
                
                // Did we find a European number?
                if (t_valid && classes[t_index] == kMCUnicodeDirectionEuropeanNumber)
                {
                    // Set all of the terminators to EN
                    while (t_start != t_index)
                    {
                        classes[t_start] = kMCUnicodeDirectionEuropeanNumber;
                        bidiIncrementISRIndex(classes, t_start_run, t_start);
                    }
                }
        }
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static void bidiApplyRuleW6(isolating_run_sequence& irs, uint8_t *classes)
{
    // ----- RULE W6 -----
    // Separators and terminators become neutral
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    do
    {
        t_class = classes[t_index];
        switch (t_class)
        {
            case kMCUnicodeDirectionEuropeanNumberTerminator:
            case kMCUnicodeDirectionEuropeanNumberSeparator:
            case kMCUnicodeDirectionCommonNumberSeparator:
                classes[t_index] = kMCUnicodeDirectionOtherNeutral;
                break;
        }
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static void bidiApplyRuleW7(isolating_run_sequence& irs, uint8_t *classes)
{
    // ----- RULE W7 -----
    // Search backwards from EN for first strong type and make them L if L found
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    do
    {
        t_class = classes[t_index];
        if (t_class == kMCUnicodeDirectionEuropeanNumber)
        {
            // Search backwards for the first strong type
            uint8_t t_strong;
            level_run *t_run_b = t_run;
            uindex_t t_index_b = t_index;
            bool t_valid;
            do
            {
                t_valid = bidiDecrementISRIndex(classes, t_run_b, t_index_b);
                if (t_valid)
                    t_strong = classes[t_index_b];
            }
            while (t_valid
                   && t_strong != kMCUnicodeDirectionRightToLeft
                   && t_strong != kMCUnicodeDirectionLeftToRight);
            
            if (!t_valid)
                t_strong = irs.sos;
            
            if (t_strong == kMCUnicodeDirectionLeftToRight)
                classes[t_index] = kMCUnicodeDirectionLeftToRight;
        }
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static bool bidiIsNI(uint8_t p_class)
{
    switch (p_class)
    {
        case kMCUnicodeDirectionBlockSeparator:
        case kMCUnicodeDirectionSegmentSeparator:
        case kMCUnicodeDirectionWhiteSpaceNeutral:
        case kMCUnicodeDirectionOtherNeutral:
        case kMCUnicodeDirectionFirstStrongIsolate:
        case kMCUnicodeDirectionLeftToRightIsolate:
        case kMCUnicodeDirectionRightToLeftIsolate:
        case kMCUnicodeDirectionPopDirectionalIsolate:
            return true;
    }
    
    return false;
}

static bool bidiIsRForNIRun(uint8_t p_class)
{
    switch (p_class)
    {
        case kMCUnicodeDirectionRightToLeft:
        case kMCUnicodeDirectionEuropeanNumber:
        case kMCUnicodeDirectionArabicNumber:
            return true;
    }
    
    return false;
}

static void bidiApplyRuleN0(isolating_run_sequence& irs, uint8_t *classes)
{
    // TODO
}

static void bidiApplyRuleN1(isolating_run_sequence& irs, uint8_t *classes, uint8_t *levels)
{
    // ----- RULE N1 -----
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    do
    {
        t_class = classes[t_index];
        if (bidiIsNI(t_class))
        {
            uindex_t t_before_index, t_after_index;
            level_run *t_before_run;
            
            // Scan backwards for a strong direction
            uint8_t t_strong_before;
            uindex_t t_index_temp = t_index;
            level_run *t_run_temp = t_run;
            bool t_valid;
            do
            {
                t_valid = bidiDecrementISRIndex(classes, t_run_temp, t_index_temp);
                if (t_valid)
                    t_strong_before = classes[t_index_temp];
            }
            while (t_valid && bidiIsNI(t_strong_before));
            
            t_before_index = t_index_temp + 1;
            t_before_run = t_run_temp;
            
            if (!t_valid)
            {
                t_strong_before = irs.sos;
                t_before_run = irs.first_run;
            }
            
            // Scan forwards for a strong direction
            uint8_t t_strong_after;
            t_index_temp = t_index;
            t_run_temp = t_run;
            do
            {
                t_valid = bidiIncrementISRIndex(classes, t_run_temp, t_index_temp);
                if (t_valid)
                    t_strong_after = classes[t_index_temp];
            }
            while (t_valid && bidiIsNI(t_strong_after));
            
            t_after_index = t_index_temp;
            
            if (!t_valid)
                t_strong_after = irs.eos;
            
            // Do both have the same direction?
            if (bidiIsRForNIRun(t_strong_before) == bidiIsRForNIRun(t_strong_after))
            {
                uint8_t t_new_class;
                t_new_class = bidiIsRForNIRun(t_strong_after)
                ? kMCUnicodeDirectionRightToLeft
                : kMCUnicodeDirectionLeftToRight;
                
                // This run needs to take on the direction
                while (t_before_index < t_after_index)
                {
                    classes[t_before_index] = t_new_class;
                    bidiIncrementISRIndex(classes, t_before_run, t_before_index);
                }
            }
            // AL-2014-08-14: [[ Bug 13077 ]] Set the index to the index of the strong text found.
            //  The index is then incremented to the next point of interest by the loop.
            t_index = t_after_index;
        }
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static void bidiApplyRuleN2(isolating_run_sequence& irs, uint8_t *classes, uint8_t *levels)
{
    // ----- RULE N2 -----
    // Remaining NIs take the embedding level
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    do
    {
        t_class = classes[t_index];
        if (bidiIsNI(t_class))
        {
            if (levels[t_index] & 1)
                classes[t_index] = kMCUnicodeDirectionRightToLeft;
            else
                classes[t_index] = kMCUnicodeDirectionLeftToRight;
        }
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static void bidiApplyRuleI1(isolating_run_sequence& irs, uint8_t *classes, uint8_t *levels)
{
    // ----- RULE I1 -----
    // Characters with an even embedding level
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    do
    {
        t_class = classes[t_index];
        if ((levels[t_index] & 1) == 0)
        {
            if (t_class == kMCUnicodeDirectionRightToLeft)
                levels[t_index] += 1;
            else if (t_class == kMCUnicodeDirectionArabicNumber
                     || t_class == kMCUnicodeDirectionEuropeanNumber)
                levels[t_index] += 2;
        }
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static void bidiApplyRuleI2(isolating_run_sequence& irs, uint8_t *classes, uint8_t *levels)
{
    // ----- RULE I2 -----
    // Characters with an odd embedding level
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    do
    {
        t_class = classes[t_index];
        if ((levels[t_index] & 1))
        {
            if (t_class == kMCUnicodeDirectionLeftToRight
                || t_class == kMCUnicodeDirectionEuropeanNumber
                || t_class == kMCUnicodeDirectionArabicNumber)
                levels[t_index] += 1;
        }
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

bool MCBidiResolveTextDirection(MCStringRef p_string, intenum_t p_base_level, uint8_t *&r_levels, uindex_t& r_level_size)
{
    uint8_t t_base_level;
    if (p_base_level == kMCTextDirectionAuto)
        t_base_level = MCBidiFirstStrongIsolate(p_string, 0);
    else
        t_base_level = uint8_t(p_base_level);
    
    uindex_t t_length;
    t_length = MCStringGetLength(p_string);
    
    // Map every codepoint in the string to its bidi class
    MCAutoArray<uint8_t> t_classes;
	if (!t_classes.New(t_length))
	{
		return false;
	}

    MCUnicodeGetProperty(MCStringGetCharPtr(p_string), t_length, kMCUnicodePropertyBidiClass, kMCUnicodePropertyTypeUint8, t_classes.Ptr());
    
    // Create an array to store the BiDi level of each character
    uint8_t *t_levels;
	if (!MCMemoryAllocate(t_length, t_levels))
	{
		return false;
	}
    
    // AL-2014-11-13: [[ Bug 13948 ]] Set the array of levels to zero, since there are some 'default' characters
    //  whose levels are not set by any of the explicit direction rules of the BiDi algorithm
    MCMemoryClear(t_levels, t_length);
    
    // Directional status stack
    MCAutoArray<bidi_stack_entry> t_stack;
    /* UNCHECKED */ t_stack.New(256);
    uindex_t t_depth = 0;
    t_stack[t_depth].clear();
    
    // Counters
    const uindex_t MAX_DEPTH = 125;
    uint8_t t_current_level = t_base_level;
    uindex_t t_overflow_isolates = 0;
    uindex_t t_overflow_embedding = 0;
    uindex_t t_valid_isolates = 0;
    
    // Isolating run sequences
    MCAutoArray<level_run> t_runs;
    MCAutoArray<isolating_run_sequence> t_irs;
    
    // UNICODE BIDIRECTIONAL ALGORITHM BEGIN
    
    // ----- RULE X1 -----
    //  Process each character iteratively, applying rules X2-X8
    for (uindex_t i = 0; i < t_length; i++)
    {
        uint8_t t_class = t_classes[i];
        bool t_formatting = false;
        bool t_to_remove = false;
        
        // ----- RULE X2 -----
        // Handle RLEs
        if (t_class == kMCUnicodeDirectionRightToLeftEmbedding && t_overflow_isolates == 0 && t_overflow_embedding == 0)
        {
            t_formatting = true;
            t_to_remove = true;
            if (t_depth < MAX_DEPTH)
            {
                // Level is the next odd level > current level
                t_stack[++t_depth].clear();
                t_stack[t_depth].level = t_current_level = t_current_level + 1 + (t_current_level & 1);
            }
            else if (t_overflow_isolates == 0)
            {
                t_overflow_embedding++;
            }
        }
        
        // ----- RULE X3 -----
        // Handle LREs
        if (t_class == kMCUnicodeDirectionLeftToRightEmbedding)
        {
            t_formatting = true;
            t_to_remove = true;
            if (t_depth < MAX_DEPTH && t_overflow_isolates == 0 && t_overflow_embedding == 0)
            {
                // Level is the next even level > current level
                t_stack[++t_depth].clear();
                t_stack[t_depth].level = t_current_level = t_current_level + 2 - (t_current_level & 1);
            }
            else if (t_overflow_isolates == 0)
            {
                t_overflow_embedding++;
            }
        }
        
        // ----- RULE X4 -----
        // Handle RLOs
        if (t_class == kMCUnicodeDirectionRightToLeftOverride)
        {
            t_formatting = true;
            t_to_remove = true;
            if (t_depth < MAX_DEPTH && t_overflow_isolates == 0 && t_overflow_embedding == 0)
            {
                // Level is the next odd level > current level
                t_stack[++t_depth].clear();
                t_stack[t_depth].level = t_current_level = t_current_level + 1 + (t_current_level & 1);
                t_stack[t_depth].override = kMCBidiOverrideRTL;
            }
            else if (t_overflow_isolates == 0)
            {
                t_overflow_embedding++;
            }
        }
        
        // ----- RULE X5 -----
        // Handle LROs
        if (t_class == kMCUnicodeDirectionLeftToRightOverride)
        {
            t_formatting = true;
            t_to_remove = true;
            if (t_depth < MAX_DEPTH && t_overflow_isolates == 0 && t_overflow_embedding == 0)
            {
                // Level is the next even level > current level
                t_stack[++t_depth].clear();
                t_stack[t_depth].level = t_current_level = t_current_level + 2 - (t_current_level & 1);
                t_stack[t_depth].override = kMCBidiOverrideLTR;
            }
            else if (t_overflow_isolates == 0)
            {
                t_overflow_embedding++;
            }
        }
        
        // ----- RULE X5c -----
        // Handle FSIs
        if (t_class == kMCUnicodeDirectionFirstStrongIsolate)
        {
            // Calculate the first strong isolate and handle as an RLI or LRI
            uint8_t t_fsi_level = MCBidiFirstStrongIsolate(p_string, i);
            if (t_fsi_level == kMCUnicodeDirectionLeftToRight)
                t_class = kMCUnicodeDirectionLeftToRightIsolate;
            else // t_fsi_level == kMCUnicodeDirectionRightToLeft
                t_class = kMCUnicodeDirectionRightToLeftIsolate;
        }
        
        // ----- RULE X5a -----
        // Handle RLIs
        if (t_class == kMCUnicodeDirectionRightToLeftIsolate)
        {
            t_formatting = true;
            t_levels[i] = t_current_level;
            if (t_depth < MAX_DEPTH && t_overflow_isolates == 0 && t_overflow_embedding == 0)
            {
                // Level is the next odd level > current level
                t_stack[++t_depth].clear();
                t_stack[t_depth].level = t_current_level = t_current_level + 1 + (t_current_level & 1);
                t_stack[t_depth].isolate = true;
                t_valid_isolates++;
            }
            else
            {
                t_overflow_isolates++;
            }
        }
        
        // ----- RULE X5b -----
        // Handle LRIs
        if (t_class == kMCUnicodeDirectionLeftToRightIsolate)
        {
            t_formatting = true;
            t_levels[i] = t_current_level;
            if (t_depth < MAX_DEPTH && t_overflow_isolates == 0 && t_overflow_embedding == 0)
            {
                // Level is the next even level > current level
                t_stack[++t_depth].clear();
                t_stack[t_depth].level = t_current_level = t_current_level + 2 - (t_current_level & 1);
                t_stack[t_depth].isolate = true;
                t_valid_isolates++;
            }
            else
            {
                t_overflow_isolates++;
            }
        }
        
        // ----- RULE X6a -----
        // Handle PDIs
        if (t_class == kMCUnicodeDirectionPopDirectionalIsolate)
        {
            if (t_overflow_isolates > 0)
                t_overflow_isolates--;
            else if (t_valid_isolates != 0)
            {
                t_overflow_embedding = 0;
                while (t_stack[t_depth].isolate == false)
                    t_current_level = t_stack[--t_depth].level;
                t_current_level = t_stack[--t_depth].level;
            }
            
            t_levels[i] = t_current_level;
            t_formatting = true;
        }
        
        // ----- RULE X7 -----
        // Handle PDFs
        if (t_class == kMCUnicodeDirectionPopDirectionalFormat)
        {
            if (t_overflow_isolates > 0)
                ;
            else if (t_overflow_embedding > 0)
                t_overflow_embedding--;
            else if (t_stack[t_depth].isolate == false && t_depth > 0)
                t_current_level = t_stack[--t_depth].level;
            
            t_formatting = true;
            t_to_remove = true;
        }
        
        // ----- RULE X6 -----
        // Handle non-formatting characters
        if (!t_formatting
            && t_class != kMCUnicodeDirectionBlockSeparator
            && t_class != kMCUnicodeDirectionBoundaryNeutral)
        {
            t_levels[i] = t_current_level;
            if (t_stack[t_depth].override == kMCBidiOverrideLTR)
                t_classes[i] = kMCUnicodeDirectionLeftToRight;
            else if (t_stack[t_depth].override == kMCBidiOverrideRTL)
                t_classes[i] = kMCUnicodeDirectionRightToLeft;
        }
        
        // ----- RULE X9 -----
        // Remove embedding/override formatting characters
        if (t_to_remove || t_class == kMCUnicodeDirectionBoundaryNeutral)
        {
            t_classes[i] = kMCUnicodeDirectionBoundaryNeutral;
            t_levels[i] = t_current_level;
        }
    }
    
    // ----- RULE X10 -----
    // Compute the isolating run sequences and apply rules W1-W7, N0-N2 and I1-I2
    
    // X10: compute the set of level runs
    uindex_t t_run_cursor = 0;
    while (t_run_cursor < t_length)
    {
        uindex_t t_run_start = t_run_cursor;
        uint8_t t_this_level = t_levels[t_run_cursor];
        while (t_run_cursor < t_length && t_levels[t_run_cursor] == t_this_level)
            t_run_cursor++;
        
        uindex_t t_run_number = t_runs.Size();
        t_runs.Extend(t_run_number + 1);
        t_runs[t_run_number].start = t_run_start;
        t_runs[t_run_number].length = t_run_cursor - t_run_start;
    }
    
    // X10: compute the set of isolating run sequences
    for (uindex_t i = 0; i < t_runs.Size(); i++)
    {
        // TODO: figure out WTH TR9 is going on about here. Somewhat unclear...
        t_runs[i].irs = i;
        t_runs[i].next_run = nil;
        t_runs[i].prev_run = nil;
        
        uindex_t t_irs_number = t_irs.Size();
        t_irs.Extend(t_irs_number + 1);
        t_irs[t_irs_number].first_run = &t_runs[i];
        t_irs[t_irs_number].last_run = &t_runs[i];
    }
    
    // X10: compute the start-of-sequence and end-of-sequence types
    for (uindex_t i = 0; i < t_irs.Size(); i++)
    {
        // SOS calculation
        uindex_t t_preceding = t_irs[i].first_run->start;
        uint8_t t_sos_level = t_levels[t_preceding];
        uint8_t t_preceding_level = t_base_level;
        while (t_preceding > 0 && t_classes[--t_preceding] == kMCUnicodeDirectionBoundaryNeutral)
            ;
        
        // EOS calculation
        // NOTE: this might be wrong if the last char in the sequence is FSI/LRI/RLI without matching PDI
        uindex_t t_succeeding = t_irs[i].last_run->start + t_irs[i].last_run->length - 1;
        uint8_t t_eos_level = t_levels[t_succeeding];
        uint8_t t_succeeding_level = t_base_level;
        while (t_succeeding < (t_length-1) && t_classes[++t_succeeding] == kMCUnicodeDirectionBoundaryNeutral)
            ;
        
        // Direction is R if the higher level is odd, L otherwise
        t_sos_level = t_sos_level < t_preceding_level ? t_preceding_level : t_sos_level;
        t_eos_level = t_eos_level < t_succeeding_level ? t_succeeding_level : t_eos_level;
        if (t_sos_level & 1)
            t_irs[i].sos = kMCUnicodeDirectionRightToLeft;
        else
            t_irs[i].sos = kMCUnicodeDirectionLeftToRight;
        if (t_eos_level & 1)
            t_irs[i].eos = kMCUnicodeDirectionRightToLeft;
        else
            t_irs[i].eos = kMCUnicodeDirectionLeftToRight;
    }
    
    // X10: for each isolating run sequence...
    for (uindex_t i = 0; i < t_irs.Size(); i++)
    {
        isolating_run_sequence &irs = t_irs[i];
        uint8_t *classes = t_classes.Ptr();
        uint8_t *levels = t_levels;
        bidiApplyRuleW1(irs, classes);
        bidiApplyRuleW2(irs, classes);
        bidiApplyRuleW3(irs, classes);
        bidiApplyRuleW4(irs, classes);
        bidiApplyRuleW5(irs, classes);
        bidiApplyRuleW6(irs, classes);
        bidiApplyRuleW7(irs, classes);
        bidiApplyRuleN0(irs, classes);
        bidiApplyRuleN1(irs, classes, levels);
        bidiApplyRuleN2(irs, classes, levels);
        bidiApplyRuleI1(irs, classes, levels);
        bidiApplyRuleI2(irs, classes, levels);
    }
    
    r_level_size = t_length;
    r_levels = t_levels;

	return true;
}

uint8_t MCBidiFirstStrongIsolate(MCStringRef p_string, uindex_t p_offset)
{
    // From TR9:
    //  P1. Split the text into separate paragraphs. A paragraph separator is
    //      kept with the previous paragraph. Within each paragraph, apply all
    //      the other rules of this algorithm. (Already done by this stage)
    //
    //  P2. In each paragraph, find the first character of type L, AL, or R
    //      while skipping over any characters between an isolate initiator and
    //      its matching PDI or, if it has no matching PDI, the end of the
    //      paragraph.
    //
    //  P3. If a character is found in P2 and it is of type AL or R, then set
    //      the paragraph embedding level to one; otherwise, set it to zero
    
    bool t_found = false;
    uindex_t t_depth = 0;
    uint8_t t_level = 0;
    while (!t_found && p_offset < MCStringGetLength(p_string))
    {
        codepoint_t t_char;
        t_char = MCStringGetCharAtIndex(p_string, p_offset);
        
        // Get the surrogate pair, if required
        uindex_t t_increment = 1;
        codepoint_t t_low;
        if (MCUnicodeCodepointIsHighSurrogate(t_char) &&
            MCUnicodeCodepointIsLowSurrogate(t_low = MCStringGetCharAtIndex(p_string, p_offset + 1)))
        {
            t_char = MCUnicodeSurrogatesToCodepoint(uint16_t(t_char), uint16_t(t_low));
            t_increment = 2;
        }
        
        // Get the directional category for this codepoint
        int32_t t_dir;
        t_dir = MCUnicodeGetIntegerProperty(t_char, kMCUnicodePropertyBidiClass);
        
        // Is this an isolate initiator?
        if (t_dir == kMCUnicodeDirectionLeftToRightIsolate
            || t_dir == kMCUnicodeDirectionRightToLeftIsolate)
        {
            t_depth++;
        }
        
        // Is this an isolate terminator?
        if (t_dir == kMCUnicodeDirectionPopDirectionalIsolate && t_depth > 0)
        {
            t_depth--;
        }
        
        // Is this a codepoint with a strong direction?
        if (t_depth == 0 && t_dir == kMCUnicodeDirectionLeftToRight)
        {
            t_level = 0;
            t_found = true;
        }
        else if (t_depth == 0 && (t_dir == kMCUnicodeDirectionRightToLeft
                                  || t_dir == kMCUnicodeDirectionRightToLeftArabic))
        {
            t_level = 1;
            t_found = true;
        }
        
        p_offset += t_increment;
    }
    
    return t_level;
}

/* Copyright (C) 2003-2015 LiveCode Ltd.

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

#include "foundation-private.h"

#include "foundation-string-hash.h"

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCNameRef kMCEmptyName;
MC_DLLEXPORT_DEF MCNameRef kMCTrueName;
MC_DLLEXPORT_DEF MCNameRef kMCFalseName;

////////////////////////////////////////////////////////////////////////////////

static MCNameRef *s_name_table;
static uindex_t s_name_table_occupancy;
static uindex_t s_name_table_capacity;

////////////////////////////////////////////////////////////////////////////////

static void __MCNameGrowTable(void);
static void __MCNameShrinkTable(void);

////////////////////////////////////////////////////////////////////////////////

/* HASH VALUE STORAGE
 *
 * On 32-bit systems, the hash value is stored in a separate field in the
 * __MCName structure.
 *
 * On 64-bit systems, in order to minimize the size of the name struct, it is
 * stored partially in the flags word (28-bits) and then the remaining 4-bits
 * split equally between the bottom two bits of the next and key ptr fields.
 *
 * The following 'accessor' functions hide the bit-twiddling details of this.
 */

static inline hash_t __MCNameReduceHash(hash_t p_hash)
{
    return p_hash;
}

static inline hash_t __MCNameGetHash(__MCName* p_name)
{
#ifdef __32_BIT__
    return p_name->hash;
#else
    return (p_name->flags & kMCValueFlagsNameHashMask) |
            ((p_name->next & 0x3) << kMCValueFlagsNameHashBits) |
            ((p_name->key & 0x3) << (kMCValueFlagsNameHashBits + 2));
#endif
}

static inline void __MCNameSetHash(__MCName* p_name, hash_t p_hash)
{
#ifdef __32_BIT__
    p_name->hash = p_hash;
#else
    p_name->flags = (p_name->flags & kMCValueFlagsTypeCodeMask) | (p_hash & kMCValueFlagsNameHashMask);
    p_name->next = (p_name->next & ~0x3) | ((p_hash >> kMCValueFlagsNameHashBits) & 0x3);
    p_name->key = (p_name->key & ~0x3) | ((p_hash >> (kMCValueFlagsNameHashBits + 2)));
#endif
}

static inline __MCName *__MCNameGetKey(__MCName* p_name)
{
#ifdef __32_BIT__
    return p_name->key;
#else
    return (__MCName *)(p_name->key & ~0x3);
#endif
}

static inline void __MCNameSetKey(__MCName* p_name, __MCName* p_key)
{
#ifdef __32_BIT__
    p_name->key = p_key;
#else
    p_name->key = ((uintptr_t)p_key) | (p_name->key & 0x3);
#endif
}

static inline __MCName *__MCNameGetNext(__MCName* p_name)
{
#ifdef __32_BIT__
    return p_name->next;
#else
    return (__MCName *)(p_name->next & ~0x3);
#endif
}

static inline void __MCNameSetNext(__MCName* p_name, __MCName* p_next)
{
#ifdef __32_BIT__
    p_name->next = p_next;
#else
    p_name->next = ((uintptr_t)p_next) | (p_name->next & 0x3);
#endif
}

////////////////////////////////////////////////////////////////////////////////

static void __MCNameIndexToNativeChars(index_t p_value, char_t r_chars[16], uindex_t& r_char_count)
{
    static const char kDigits[201] =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";
    
    r_char_count = 0;
    
    uindex_t t_value;
    if (p_value < 0)
    {
        r_chars[r_char_count++] = '-';
        p_value = -p_value;
    }
    
    t_value = p_value;
    
    uint32_t t_length = 1;
    for (;;) {
        if (t_value < 10) break;
        if (t_value < 100) { t_length += 1; break; }
        if (t_value < 1000) { t_length += 2; break; }
        if (t_value < 10000) { t_length += 3; break; }
        t_value /= 10000U;
        t_length += 4;
    }
    
    t_value = p_value;
    
    r_char_count += t_length;
    
    uindex_t t_next =
        r_char_count - 1;
    
    while(t_value >= 100)
    {
        index_t t_offset =
                    (t_value % 100) * 2;
        
        t_value /= 100;
        
        r_chars[t_next] = kDigits[t_offset + 1];
        r_chars[t_next - 1] = kDigits[t_offset];
        
        t_next -= 2;
    }
    
    if (t_value < 10)
    {
        r_chars[t_next] = '0' + t_value;
    }
    else
    {
        index_t t_offset =
                    t_value * 2;
        
        r_chars[t_next] = kDigits[t_offset + 1];
        r_chars[t_next - 1] = kDigits[t_offset];
    }
}

static hash_t __MCNameIndexHash(const char_t *p_chars, uindex_t p_char_count)
{
    MCHashCharsContext t_hash;
    switch(p_char_count)
    {
    case 11:
        t_hash.consume(*p_chars++);
    case 10:
        t_hash.consume(*p_chars++);
    case 9:
        t_hash.consume(*p_chars++);
    case 8:
        t_hash.consume(*p_chars++);
    case 7:
        t_hash.consume(*p_chars++);
    case 6:
        t_hash.consume(*p_chars++);
    case 5:
        t_hash.consume(*p_chars++);
    case 4:
        t_hash.consume(*p_chars++);
    case 3:
        t_hash.consume(*p_chars++);
    case 2:
        t_hash.consume(*p_chars++);
    case 1:
        t_hash.consume(*p_chars++);
        break;
    }
    return t_hash;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
MCNameRef MCNAME(const char *p_string)
{
	MCStringRef t_string;
	t_string = MCSTR(p_string);
	
	MCNameRef t_name;
	/* UNCHECKED */ MCNameCreate(t_string, t_name);

	return t_name;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCNameCreate(MCStringRef p_string, MCNameRef& r_name)
{
	__MCAssertIsString(p_string);

	if (p_string -> char_count == 0 && kMCEmptyName != nil)
	{
		MCValueRetain(kMCEmptyName);
		r_name = kMCEmptyName;
		return true;
	}

	// Compute the has of the characters, up to case.
	hash_t t_hash;
	t_hash = MCStringHash(p_string, kMCStringOptionCompareCaseless);
    
    // Reduce the hash to the size we store
    t_hash = __MCNameReduceHash(t_hash);

	// Calculate the index of the chain in the name table where this might be
	// found. The capacity is always a power-of-two, so its just a mask op.
	uindex_t t_index;
	t_index = t_hash & (s_name_table_capacity - 1);

	// Search for the first representation of the would-be name's equivalence
	// class.
	__MCName *t_key_name;
	t_key_name = s_name_table[t_index];
	while(t_key_name != nil)
	{
		// If the string matches, then we are done - notice we compare the
		// full hash first.
		if (t_hash == __MCNameGetHash(t_key_name) &&
			MCStringIsEqualTo(p_string, t_key_name -> string, kMCStringOptionCompareCaseless))
			break;

		// Otherwise skip all other members of the same equivalence class.
		while(__MCNameGetNext(t_key_name) != nil &&
				__MCNameGetKey(t_key_name) == __MCNameGetKey(__MCNameGetNext(t_key_name)))
            t_key_name = __MCNameGetNext(t_key_name);

        // Next name must be the next one.
        t_key_name = __MCNameGetNext(t_key_name);
	}

	// Now search within the equivalence class for one with the same string and
	// return immediately if we find a match.
	__MCName *t_name;
	for(t_name = t_key_name; t_name != nil && __MCNameGetKey(t_name) == t_key_name; t_name = __MCNameGetNext(t_name))
		if (MCStringIsEqualTo(p_string, t_name -> string, kMCStringOptionCompareExact))
		{
			t_name -> references += 1;
			r_name = t_name;
			return true;
		}

	// We haven't found an exact match, so we create a new name...
	bool t_success;
	t_success = true;

	// Allocate a name record.
	if (t_success)
		t_success = __MCValueCreate(kMCValueTypeCodeName, t_name);

	// Copy the string (as immutable).
	if (t_success)
		t_success = MCStringCopy(p_string, t_name -> string);

	// Now add the name to the table and fill in the rest of the fields.
	if (t_success)
	{
		// If there is no existing equivalence class, we chain at the start,
		// otherwise we insert the name after the representative.
		if (t_key_name == nil)
		{
			// To keep hashin efficient, we (try to) double the size of the
			// table each time occupancy reaches capacity.
			if (s_name_table_occupancy == s_name_table_capacity)
			{
				__MCNameGrowTable();
				t_index = t_hash & (s_name_table_capacity - 1);
			}

			// Increase occupancy.
			s_name_table_occupancy += 1;

            __MCNameSetNext(t_name, s_name_table[t_index]);
            __MCNameSetKey(t_name, t_name);
			s_name_table[t_index] = t_name;
		}
		else
		{
            __MCNameSetNext(t_name, __MCNameGetNext(t_key_name));
            __MCNameSetKey(t_name, t_key_name);
            __MCNameSetNext(t_key_name, t_name);

			// Increment the reference count of the representative as we need
			// it to 'hang around' for the entire lifetime of all others in the
			// equivalence class to give a search handle.
			t_key_name -> references += 1;
		}

		// Record the hash (speeds up searching and such).
        __MCNameSetHash(t_name, t_hash);

		// Return the new name.
		r_name = t_name;
	}
	else
	{
		MCValueRelease(t_name -> string);
		MCMemoryDelete(t_name);
	}

	return t_success;
}

MC_DLLEXPORT_DEF
bool MCNameCreateWithIndex(index_t p_index, MCNameRef& r_name)
{
    char_t t_chars[11];
    uindex_t t_char_count;
    __MCNameIndexToNativeChars(p_index, t_chars, t_char_count);
    
    // Compute the hash of the characters, up to case.
    hash_t t_hash;
    t_hash = __MCNameIndexHash(t_chars, t_char_count);
    
    // Reduce the hash to the size we store
    t_hash = __MCNameReduceHash(t_hash);
    
    // Calculate the index of the chain in the name table where this might be
    // found. The capacity is always a power-of-two, so its just a mask op.
    uindex_t t_index;
    t_index = t_hash & (s_name_table_capacity - 1);
    
    // Search for the first representation of the would-be name's equivalence
    // class.
    __MCName *t_key_name;
    t_key_name = s_name_table[t_index];
    while(t_key_name != nil)
    {
        // If the string matches, then we are done - notice we compare the
        // full hash first.
        if (t_hash == __MCNameGetHash(t_key_name) &&
            __MCNameGetKey(t_key_name) == t_key_name &&
            MCStringIsEqualToNativeChars(t_key_name -> string, t_chars, t_char_count, kMCStringOptionCompareExact))
        {
            t_key_name -> references += 1;
            r_name = t_key_name;
            return true;
        }
        
        // Next name must be the next one.
        t_key_name = __MCNameGetNext(t_key_name);
    }
    
    // We haven't found an exact match, so we create a new name...
    bool t_success;
    t_success = true;
    
    // Allocate a name record.
    __MCName *t_name;
    if (t_success)
        t_success = __MCValueCreate(kMCValueTypeCodeName, t_name);
    
    // Copy the string (as immutable).
    if (t_success)
        t_success = MCStringCreateWithNativeChars(t_chars, t_char_count, t_name -> string);
    
    // Now add the name to the table and fill in the rest of the fields.
    if (t_success)
    {
        // To keep hashin efficient, we (try to) double the size of the
        // table each time occupancy reaches capacity.
        if (s_name_table_occupancy == s_name_table_capacity)
        {
            __MCNameGrowTable();
            t_index = t_hash & (s_name_table_capacity - 1);
        }
        
        // Increase occupancy.
        s_name_table_occupancy += 1;
        
        __MCNameSetNext(t_name, s_name_table[t_index]);
        __MCNameSetKey(t_name, t_name);
        s_name_table[t_index] = t_name;

        // Record the hash (speeds up searching and such).
        __MCNameSetHash(t_name, t_hash);
        
        // Return the new name.
        r_name = t_name;
    }
    else
    {
        MCValueRelease(t_name -> string);
        MCMemoryDelete(t_name);
    }
    
    return t_success;
}

MC_DLLEXPORT_DEF
bool MCNameCreateWithNativeChars(const char_t *p_chars, uindex_t p_count, MCNameRef& r_name)
{
	MCStringRef t_string;
	if (!MCStringCreateWithNativeChars(p_chars, p_count, t_string))
		return false;
	if (!MCNameCreateAndRelease(t_string, r_name))
	{
		MCValueRelease(t_string);
		return false;
	}
	return true;
}

MC_DLLEXPORT_DEF
bool MCNameCreateWithChars(const unichar_t *p_chars, uindex_t p_count, MCNameRef& r_name)
{
	MCStringRef t_string;
	if (!MCStringCreateWithChars(p_chars, p_count, t_string))
		return false;
	return MCNameCreateAndRelease(t_string, r_name);
}

MC_DLLEXPORT_DEF
bool MCNameCreateAndRelease(MCStringRef p_string, MCNameRef& r_name)
{
	if (MCNameCreate(p_string, r_name))
	{
		MCValueRelease(p_string);
		return true;
	}

	return false;
}

MC_DLLEXPORT_DEF
MCNameRef MCNameLookupCaseless(MCStringRef p_string)
{
	// Compute the hash of the characters, up to case.
	hash_t t_hash;
	t_hash = MCStringHash(p_string, kMCStringOptionCompareCaseless);
    
    // Reduce the hash to the size we store
    t_hash = __MCNameReduceHash(t_hash);
    
	// Calculate the index of the chain in the name table where this name might
	// be found. The capacity is always a power-of-two, so its just a mask op.
	uindex_t t_index;
	t_index = t_hash & (s_name_table_capacity - 1);

	// Search for the first representative of the would-be name's equivalence class.
	__MCName *t_key_name;
	t_key_name = s_name_table[t_index];
	while(t_key_name != nil)
	{
		// If the string matches, then we are done - notice we compare the full
		// hash first.
		if (t_hash == __MCNameGetHash(t_key_name) &&
			MCStringIsEqualTo(p_string, t_key_name -> string, kMCStringOptionCompareCaseless))
			break;

        // Otherwise skip all other members of the same equivalence class.
        while(__MCNameGetNext(t_key_name) != nil &&
              __MCNameGetKey(t_key_name) == __MCNameGetKey(__MCNameGetNext(t_key_name)))
            t_key_name = __MCNameGetNext(t_key_name);

        // Next name must be the next one
        t_key_name = __MCNameGetNext(t_key_name);
	}

	return t_key_name;
}

MCNameRef MCNameLookupIndex(index_t p_index)
{
    char_t t_chars[11];
    uindex_t t_char_count;
    __MCNameIndexToNativeChars(p_index, t_chars, t_char_count);
    
    // Compute the hash of the characters, up to case.
    hash_t t_hash;
    t_hash = __MCNameIndexHash(t_chars, t_char_count);
    
    // Reduce the hash to the size we store
    t_hash = __MCNameReduceHash(t_hash);
    
    // Calculate the index of the chain in the name table where this name might
    // be found. The capacity is always a power-of-two, so its just a mask op.
    uindex_t t_index;
    t_index = t_hash & (s_name_table_capacity - 1);
    
    // Search for the first representative of the would-be name's equivalence class.
    __MCName *t_key_name;
    t_key_name = s_name_table[t_index];
    while(t_key_name != nil)
    {
        // If the string matches, then we are done - notice we compare the full
        // hash first. As an index as a string consists of folded chars, we can
        // use exact comparison.
        if (t_hash == __MCNameGetHash(t_key_name) &&
            __MCNameGetKey(t_key_name) == t_key_name &&
            MCStringIsEqualToNativeChars(t_key_name -> string, t_chars, t_char_count, kMCStringOptionCompareExact))
            break;
        
        // Next name must be the next one
        t_key_name = __MCNameGetNext(t_key_name);
    }
    
    return t_key_name;
}

MC_DLLEXPORT_DEF
uintptr_t MCNameGetCaselessSearchKey(MCNameRef self)
{
	__MCAssertIsName(self);
	return (uintptr_t)__MCNameGetKey(self);
}

MC_DLLEXPORT_DEF
MCStringRef MCNameGetString(MCNameRef self)
{
	__MCAssertIsName(self);
	return self -> string;
}

MC_DLLEXPORT_DEF
bool MCNameIsEmpty(MCNameRef self)
{
	return self == kMCEmptyName;
}

MC_DLLEXPORT_DEF
bool MCNameIsEqualToCaseless(MCNameRef self, MCNameRef p_other_name)
{
	__MCAssertIsName(self);
	__MCAssertIsName(p_other_name);

	return self == p_other_name ||
			__MCNameGetKey(self) == __MCNameGetKey(p_other_name);
}

MC_DLLEXPORT_DEF
bool MCNameIsEqualTo(MCNameRef self, MCNameRef p_other_name, MCStringOptions p_options)
{
	__MCAssertIsName(self);
	__MCAssertIsName(p_other_name);

    if (self == p_other_name)
        return true;

    if (p_options == kMCStringOptionCompareExact)
        return false;
    
    if (p_options == kMCStringOptionCompareCaseless)
        return __MCNameGetKey(self) == __MCNameGetKey(p_other_name);
    
    return MCStringIsEqualTo(self -> string, p_other_name -> string, p_options);
}

////////////////////////////////////////////////////////////////////////////////

void __MCNameDestroy(__MCName *self)
{
	// Compute the index in the table
	uindex_t t_index;
	t_index = __MCNameGetHash(self) & (s_name_table_capacity - 1);

	// Find the previous link in the chain
	__MCName *t_previous;
	t_previous = nil;
	for(__MCName *t_name = s_name_table[t_index]; t_name != self; t_name = __MCNameGetNext(t_name))
		t_previous = t_name;

	// Update the previous name's next field
	if (t_previous == nil)
		s_name_table[t_index] = __MCNameGetNext(self);
	else
        __MCNameSetNext(t_previous, __MCNameGetNext(self));

	// If this name is not the key then remove our reference to it. Otherwise
	// adjust occupancy appropriately.
	if (__MCNameGetKey(self) != self)
		MCValueRelease(__MCNameGetKey(self));
	else
	{
		// Reduce occupancy of the table
		s_name_table_occupancy -= 1;

		// If the table is too sparse, reduce its size (current heuristic has the
		// threshold at 33%).
		if (s_name_table_capacity > 1024 && s_name_table_occupancy * 16 / s_name_table_capacity < 5)
			__MCNameShrinkTable();
	}

	// Delete the resources
	MCValueRelease(self -> string);
}

bool __MCNameCopyDescription(__MCName *self, MCStringRef& r_string)
{
	return MCStringCopy(self -> string, r_string);
}

hash_t __MCNameHash(__MCName *self)
{
	return __MCNameGetHash(self);
}

bool __MCNameIsEqualTo(__MCName *self, __MCName *p_other_self)
{
	return self == p_other_self;
}

////////////////////////////////////////////////////////////////////////////////

static void __MCNameRelocateTableEntries(uindex_t p_start, uindex_t p_finish, uindex_t p_new_capacity)
{
	// Loop through the entries, moving them as necessary. As the hash-table
	// grows by powers of two, an entry either stays put or moves to outside
	// the range of consideration.
	for(uindex_t t_old_index = p_start; t_old_index < p_finish; t_old_index++)
	{
		// The last name in the chain we previously processed which might
		// get its 'next' field updated.
		MCNameRef t_previous;
		t_previous = nil;

		// The first name in the current chain being processed.
		MCNameRef t_first;
		t_first = s_name_table[t_old_index];
		while(t_first != nil)
		{
			// Compute the new index.
			uindex_t t_new_index;
			t_new_index = __MCNameGetHash(t_first) & (p_new_capacity - 1);

			// Search for the last name to relocate.
			MCNameRef t_last;
			for(t_last = t_first; __MCNameGetNext(t_last) != nil; t_last = __MCNameGetNext(t_last))
				if ((__MCNameGetHash(__MCNameGetNext(t_last)) & (p_new_capacity - 1)) != t_new_index)
					break;

			// Record the next name to process after this chain.
			MCNameRef t_next;
			t_next = __MCNameGetNext(t_last);

			// If the new index is different from the old, then we need to move
			// the names from first->last.
			if (t_new_index != t_old_index)
			{
				// Remove the link from the previous name, or the table at the
				// old index.
				if (t_previous != nil)
                    __MCNameSetNext(t_previous, __MCNameGetNext(t_last));
				else
					s_name_table[t_old_index] = __MCNameGetNext(t_last);

				// Push the chain of names onto the front of the table at the
				// new index.
				__MCNameSetNext(t_last, s_name_table[t_new_index]);
				s_name_table[t_new_index] = t_first;
			}
			else
				t_previous = t_last;

			// Move to the next chain.
			t_first = t_next;
		}
	}
}

static void __MCNameGrowTable(void)
{
	// First attempt to double the table size.
	if (!MCMemoryResizeArray(s_name_table_capacity * 2, s_name_table, s_name_table_capacity))
		return;

	// Now relocate any entries from the used half that need to be moved to the
	// upper half (another bit has become available to determine spread).
	__MCNameRelocateTableEntries(0, s_name_table_capacity / 2, s_name_table_capacity);
}

static void __MCNameShrinkTable(void)
{
	// First relocate any entries from the upper half of the table to the lower
	// half (a bit has been removed from that which determines spread).
	__MCNameRelocateTableEntries(s_name_table_capacity / 2, s_name_table_capacity, s_name_table_capacity / 2);

	// Now resize the array.
	MCMemoryResizeArray(s_name_table_capacity / 2, s_name_table, s_name_table_capacity);
}

////////////////////////////////////////////////////////////////////////////////

bool __MCNameInitialize(void)
{
	if (!MCMemoryNewArray(1024, s_name_table, s_name_table_capacity))
		return false;

	if (!MCNameCreate(kMCEmptyString, kMCEmptyName))
		return false;

	if (!MCNameCreate(kMCTrueString, kMCTrueName))
		return false;
		
	if (!MCNameCreate(kMCFalseString, kMCFalseName))
		return false;

	s_name_table_occupancy = 0;

	return true;
}

void __MCNameFinalize(void)
{
	MCValueRelease(kMCEmptyName);
	kMCEmptyName = nil;

    MCValueRelease(kMCTrueName);
    kMCTrueName = nil;

    MCValueRelease(kMCFalseName);
    kMCFalseName = nil;

	MCMemoryDeleteArray(s_name_table);
	s_name_table = nil;
	s_name_table_capacity = 0;
	s_name_table_occupancy = 0;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef __DUMP
void MCNameDumpTable(void)
{
	FILE *t_output;
	t_output = fopen("C:\\Users\\Mark\\Desktop\\mcnamedump.txt", "w");
	if (t_output == nil)
		return;

	uint32_t t_singleton_count, t_variants_count;
	t_singleton_count = 0;
	t_variants_count = 0;

	for(uint32_t i = 0; i < s_name_table_capacity; i++)
	{
		if (s_name_table[i] == nil)
			continue;

		MCNameRef t_name;
		t_name = s_name_table[i];
		while(t_name != nil)
		{
			if (t_name -> references == 1)
				t_singleton_count++;

			MCNameRef t_other_name;
			fprintf(t_output, "'%s' (%u)", t_name -> string -> chars, t_name -> references);
			for(t_other_name = t_name -> next; t_other_name != nil && t_other_name -> key == t_name; t_other_name = t_other_name -> next)
			{
				fprintf(t_output, " : '%s' (%u)", t_other_name -> string -> chars, t_other_name -> references);
				t_variants_count++;
			}
			fprintf(t_output, "\n");

			t_name = t_other_name;

		}
	}

	fprintf(t_output, "\nSingleton Count = %u\nVariants Count = %u\n", t_singleton_count, t_variants_count);

	fclose(t_output);
}
#endif


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
#include "foundation-span.h"
#include "foundation-hash.h"

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

MC_DLLEXPORT_DEF
MCNameRef MCNAME(const char *p_string)
{
	MCStringRef t_string;
	t_string = MCSTR(p_string);
	
	MCNameRef t_name;
	/* UNCHECKED */ MCNameCreate(t_string, t_name);
	
	MCValueRef t_name_unique;
	/* UNCHECKED */ MCValueInter(t_name, t_name_unique);
	
	MCValueRelease(t_name);
	
	return (MCNameRef)t_name_unique;
}

////////////////////////////////////////////////////////////////////////////////

// This class is a generic implementation of the algorithm used to search the
// name table for a given key, and then create it if required.
//
// It is parameterized by an Input class which must have the following methods:
//
//   1) IsEmpty() - should return true if the input is the empty string
//   2) IsEquivalentTo(other) - should return true if the input lies in the same
//      (caseless) equivalence class as other.
//   3) IsEqualTo(other) - should return true if the input is identical to
//      other.
//   4) Copy(&string) - return a copy of the input as a string ref for use in
//      constructing the new name.
//   5) Hash() - return the hash of the input.
//
// The LookupCaseless method can be used to check to see if a name is already in
// the table. This method returns the representative up to caseless equivalence
// for the given input if it is present in the table, or nil otherwise.
//
// The Create method can be used to ensure the given input exists as an exact
// name. This method returns true if construction is successful, and returns a
// copy of the new name as an out parameter.
//
template<typename Input>
class __MCNameTableSearcher
{
public:
    __MCNameTableSearcher(const Input& p_input)
        : m_input(p_input),
          m_hash(p_input.Hash())
    {
    }
    
    MCNameRef LookupCaseless(void) const
    {
        // If the input is the empty string, then make sure we return the existing
        // empty name value.
        if (m_input.IsEmpty() &&
            kMCEmptyName != nil)
        {
            return kMCEmptyName;
        }
        
        // Calculate the index of the chain in the name table where this might be
        // found. The capacity is always a power-of-two, so its just a mask op.
        uindex_t t_index =
                m_hash & (s_name_table_capacity - 1);
        
        // Search for the first representation of the would-be name's equivalence
        // class.
        __MCName *t_key_name =
                s_name_table[t_index];
        while(t_key_name != nil)
        {
            // If the hash is the same, see if the input is equivalent to the
            // key name in the current equivalence class.
            if (m_hash == t_key_name->hash &&
                m_input.IsEquivalentTo(t_key_name->string))
                break;
            
            // Skip to the next equivalence class.
            while(t_key_name->next != nil &&
                  t_key_name->key == t_key_name->next->key)
                t_key_name = t_key_name -> next;
            
            // Next name must be the next one.
            t_key_name = t_key_name->next;
        }

        return t_key_name;
    }
    
    bool Create(MCNameRef& r_name)
    {
        MCNameRef t_key_name =
                LookupCaseless();
        
        // Search for the exact representative, if present and return it if found.
        __MCName *t_name = nil;
        for(t_name = t_key_name; t_name != nil && t_name -> key == t_key_name; t_name = t_name -> next)
            if (m_input.IsEqualTo(t_name->string))
            {
                r_name = MCValueRetain(t_name);
                return true;
            }
        
        // An exact match was not found, so we must now create a new name.
        
        if (!__MCValueCreate(kMCValueTypeCodeName,
                             t_name))
        {
            return false;
        }
        
        if (!m_input.Copy(t_name->string))
        {
            MCMemoryDelete(t_name);
            return false;
        }
        
        // If there is no existing equivalence class, we chain at the start,
        // otherwise we insert the name after the representative.
        if (t_key_name == nil)
        {
            // To keep hashin efficient, we (try to) double the size of the
            // table each time occupancy reaches capacity.
            uindex_t t_index;
            if (s_name_table_occupancy == s_name_table_capacity)
            {
                __MCNameGrowTable();
            }
            t_index = m_hash & (s_name_table_capacity - 1);
            
            // Increase occupancy.
            s_name_table_occupancy += 1;
            
            t_name->next = s_name_table[t_index];
            t_name->key = t_name;
            s_name_table[t_index] = t_name;
        }
        else
        {
            t_name -> next = t_key_name -> next;
            t_name -> key = t_key_name;
            t_key_name -> next = t_name;
            
            // Increment the reference count of the representative as we need
            // it to 'hang around' for the entire lifetime of all others in the
            // equivalence class to give a search handle.
            t_key_name -> references += 1;
        }
        
        // Record the hash (speeds up searching and such).
        t_name->hash = m_hash;
        
        // Return the new name.
        r_name = t_name;
        
        return true;
    }
    
private:
    const Input& m_input;
    const hash_t m_hash;
};

//////////

// This comparator is used to search for a stringref in the name table. The
// provided StringRef can be mutable or immutable, as it is Copied if the
// name needs to be created.
class __MCNameStringInput
{
public:
    __MCNameStringInput(MCStringRef p_string)
        : m_string(p_string)
    {
    }

    bool IsEmpty(void) const
    {
        return m_string->char_count == 0;
    }
    
    hash_t Hash(void) const
    {
        return MCStringHash(m_string,
                            kMCStringOptionCompareCaseless);
    }
    
    bool IsEquivalentTo(MCStringRef p_other_string) const
    {
        return MCStringIsEqualTo(m_string,
                                 p_other_string,
                                 kMCStringOptionCompareCaseless);
    }
    
    bool IsEqualTo(MCStringRef p_other_string) const
    {
        return MCStringIsEqualTo(m_string,
                                 p_other_string,
                                 kMCStringOptionCompareExact);
    }
    
    bool Copy(MCStringRef& r_copied_string) const
    {
        return MCStringCopy(m_string,
                            r_copied_string);
    }
    
private:
    MCStringRef m_string;
};

MC_DLLEXPORT_DEF
bool MCNameCreate(MCStringRef p_string, MCNameRef& r_name)
{
	__MCAssertIsString(p_string);
    
    return __MCNameTableSearcher<__MCNameStringInput>(p_string).Create(r_name);
}

//////////

// This comparator is used to search for native string in the name table.
class __MCNameNativeCharsInput
{
public:
    __MCNameNativeCharsInput(MCSpan<const char_t> p_native_chars)
        : m_native_chars(p_native_chars)
    {
    }
    
    bool IsEmpty(void) const
    {
        return m_native_chars.length() == 0;
    }
    
    hash_t Hash(void) const
    {
        return MCHashNativeChars(m_native_chars.data(),
                                 m_native_chars.length());
    }
    
    bool IsEquivalentTo(MCStringRef p_other_string) const
    {
        return MCStringIsEqualToNativeChars(p_other_string,
                                            m_native_chars.data(),
                                            m_native_chars.length(),
                                            kMCStringOptionCompareCaseless);
    }
    
    bool IsEqualTo(MCStringRef p_other_string) const
    {
        return MCStringIsEqualToNativeChars(p_other_string,
                                            m_native_chars.data(),
                                            m_native_chars.length(),
                                            kMCStringOptionCompareExact);
    }
    
    bool Copy(MCStringRef& r_copied_string) const
    {
        return MCStringCreateWithNativeChars(m_native_chars.data(),
                                             m_native_chars.length(),
                                             r_copied_string);
    }
    
private:
    MCSpan<const char_t> m_native_chars;
};

MC_DLLEXPORT_DEF
bool MCNameCreateWithNativeChars(const char_t *p_chars, uindex_t p_count, MCNameRef& r_name)
{
    return __MCNameTableSearcher<__MCNameNativeCharsInput>(MCMakeSpan(p_chars,
                                                                      p_count)).Create(r_name);
}

//////////

// This comparator is used to search for native string in the name table.
class __MCNameCharsInput
{
public:
    __MCNameCharsInput(MCSpan<const unichar_t> p_chars)
        : m_chars(p_chars)
    {
    }
    
    bool IsEmpty(void) const
    {
        return m_chars.length() == 0;
    }
    
    hash_t Hash(void) const
    {
        return MCHashChars(m_chars.data(),
                           m_chars.length());
    }
    
    bool IsEquivalentTo(MCStringRef p_other_string) const
    {
        return MCStringIsEqualToChars(p_other_string,
                                      m_chars.data(),
                                      m_chars.length(),
                                      kMCStringOptionCompareCaseless);
    }
    
    bool IsEqualTo(MCStringRef p_other_string) const
    {
        return MCStringIsEqualToChars(p_other_string,
                                      m_chars.data(),
                                      m_chars.length(),
                                      kMCStringOptionCompareExact);
    }
    
    bool Copy(MCStringRef& r_copied_string) const
    {
        return MCStringCreateWithChars(m_chars.data(),
                                       m_chars.length(),
                                       r_copied_string);
    }
    
private:
    MCSpan<const unichar_t> m_chars;
};

MC_DLLEXPORT_DEF
bool MCNameCreateWithChars(const unichar_t *p_chars, uindex_t p_count, MCNameRef& r_name)
{
    return __MCNameTableSearcher<__MCNameCharsInput>(MCMakeSpan(p_chars,
                                                                p_count)).Create(r_name);
}

//////////

class __MCNameIndexInput
{
public:
    __MCNameIndexInput(index_t p_index)
        : m_char_count(0)
    {
        IndexToString(p_index);
    }
    
    bool IsEmpty(void) const
    {
        return false;
    }
    
    hash_t Hash(void) const
    {
        __MCHashCharContext t_context;
        for(uindex_t i = 0; i < m_char_count; i++)
            t_context.ConsumeHalfChar(m_chars[i]);
        return t_context.Current();
    }
    
    bool IsEquivalentTo(MCStringRef p_other_string) const
    {
        return MCStringIsEqualToNativeChars(p_other_string,
                                            m_chars,
                                            m_char_count,
                                            kMCStringOptionCompareCaseless);
    }
    
    // As equivalence => equality for index strings, this is always true.
    bool IsEqualTo(MCStringRef p_other_string) const
    {
        return true;
    }
    
    bool Copy(MCStringRef& r_copied_string) const
    {
        return MCStringCreateWithNativeChars(m_chars,
                                             m_char_count,
                                             r_copied_string);
    }
    
private:
    char_t m_chars[16];
    uindex_t m_char_count;
    
    static uindex_t CountDigits(index_t p_index)
    {
        uint32_t t_count = 1;
        for (;;) {
            if (p_index < 10) return t_count;
            if (p_index < 100) return t_count + 1;
            if (p_index < 1000) return t_count + 2;
            if (p_index < 10000) return t_count + 3;
            p_index /= 10000U;
            t_count += 4;
        }
        return t_count;
    }
    
    void IndexToString(index_t p_value)
    {
        static const char kDigits[201] =
            "0001020304050607080910111213141516171819"
            "2021222324252627282930313233343536373839"
            "4041424344454647484950515253545556575859"
            "6061626364656667686970717273747576777879"
            "8081828384858687888990919293949596979899";
        
        if (p_value < 0)
        {
            m_chars[m_char_count++] = '-';
            p_value = -p_value;
        }
        
        uindex_t t_length =
                CountDigits(p_value);
        
        m_char_count += t_length;
        
        uindex_t t_next =
                m_char_count - 1;
        
        while(p_value >= 100)
        {
            index_t t_offset =
                (p_value % 100) * 2;
            p_value /= 100;
            
            m_chars[t_next] = kDigits[t_offset + 1];
            m_chars[t_next - 1] = kDigits[t_offset];
            
            t_next -= 2;
        }
        
        if (p_value < 10)
        {
            m_chars[t_next] = '0' + p_value;
        }
        else
        {
            index_t t_offset =
                    p_value * 2;
            
            m_chars[t_next] = kDigits[t_offset + 1];
            m_chars[t_next - 1] = kDigits[t_offset];
        }
    }
};

MC_DLLEXPORT_DEF
bool MCNameCreateWithIndex(index_t p_index,
                           MCNameRef& r_name)
{
    return __MCNameTableSearcher<__MCNameIndexInput>(p_index).Create(r_name);
}

//////////

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

//////////

MC_DLLEXPORT_DEF
MCNameRef MCNameLookupCaseless(MCStringRef p_string)
{
    __MCAssertIsString(p_string);
    
    return __MCNameTableSearcher<__MCNameStringInput>(p_string).LookupCaseless();
}

MC_DLLEXPORT_DEF
MCNameRef MCNameLookupIndex(index_t p_index)
{
    return __MCNameTableSearcher<__MCNameIndexInput>(p_index).LookupCaseless();
}

//////////

MC_DLLEXPORT_DEF
uintptr_t MCNameGetCaselessSearchKey(MCNameRef self)
{
	__MCAssertIsName(self);
	return (uintptr_t)self -> key;
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

bool MCNameIsEqualTo(MCNameRef self, MCNameRef p_other_name)
{
	__MCAssertIsName(self);
	__MCAssertIsName(p_other_name);

	return self == p_other_name ||
			self -> key == p_other_name -> key;
}

bool MCNameIsEqualTo(MCNameRef self, MCNameRef p_other_name, bool p_case_sensitive, bool p_form_sensitive)
{
	__MCAssertIsName(self);
	__MCAssertIsName(p_other_name);

    if (self == p_other_name)
        return true;

    if (p_case_sensitive && p_form_sensitive)
        return false;
    else if (!p_case_sensitive && !p_form_sensitive)
        return self -> key == p_other_name -> key;
    else if (p_case_sensitive)
        return MCStringIsEqualTo(self -> string, p_other_name -> string, kMCStringOptionCompareNonliteral);
    else
        return MCStringIsEqualTo(self -> string, p_other_name -> string, kMCStringOptionCompareFolded);
}

////////////////////////////////////////////////////////////////////////////////

void __MCNameDestroy(__MCName *self)
{
	// Compute the index in the table
	uindex_t t_index;
	t_index = self -> hash & (s_name_table_capacity - 1);

	// Find the previous link in the chain
	__MCName *t_previous;
	t_previous = nil;
	for(__MCName *t_name = s_name_table[t_index]; t_name != self; t_name = t_name -> next)
		t_previous = t_name;

	// Update the previous name's next field
	if (t_previous == nil)
		s_name_table[t_index] = self -> next;
	else
		t_previous -> next = self -> next;

	// If this name is not the key then remove our reference to it. Otherwise
	// adjust occupancy appropriately.
	if (self -> key != self)
		MCValueRelease(self -> key);
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
	return self -> hash;
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
			t_new_index = t_first -> hash & (p_new_capacity - 1);

			// Search for the last name to relocate.
			MCNameRef t_last;
			for(t_last = t_first; t_last -> next != nil; t_last = t_last -> next)
				if ((t_last -> next -> hash & (p_new_capacity - 1)) != t_new_index)
					break;

			// Record the next name to process after this chain.
			MCNameRef t_next;
			t_next = t_last -> next;

			// If the new index is different from the old, then we need to move
			// the names from first->last.
			if (t_new_index != t_old_index)
			{
				// Remove the link from the previous name, or the table at the
				// old index.
				if (t_previous != nil)
					t_previous -> next = t_last -> next;
				else
					s_name_table[t_old_index] = t_last -> next;

				// Push the chain of names onto the front of the table at the
				// new index.
				t_last -> next = s_name_table[t_new_index];
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


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

#include "graphics.h"
#include "graphics-internal.h"

////////////////////////////////////////////////////////////////////////////////

#ifndef UINDEX_MAX
#define UINDEX_MAX 4294967295U
#endif

static hash_t MCGHashBytes(const void *p_bytes, size_t length)
{
    // 32-bit FNV-1a hash algorithm
    const uint32_t t_prime = 16777619;
    const uint32_t t_bias = 2166136261U;
    
    const uint8_t *t_data = (const uint8_t*)p_bytes;
    uint32_t t_hash = t_bias;
    for (size_t i = 0; i < length; i++)
    {
        t_hash ^= t_data[i];
        t_hash *= t_prime;
    }
    
    return t_hash;
}

////////////////////////////////////////////////////////////////////////////////

struct __MCGCacheTableEntry
{
	hash_t		hash;
	uint32_t	key_length;
	void		*key;
	uintptr_t	*value;
};

struct __MCGCacheTable
{
	uindex_t				total_buckets;
	uint32_t				used_buckets;
	uindex_t				max_occupancy;
	uindex_t				max_bytes;
	uint32_t				bytes_used;
	__MCGCacheTableEntry	*pairs;
};

////////////////////////////////////////////////////////////////////////////////

static uindex_t MCGCacheTableLookup(MCGCacheTableRef self, void *p_key, uint32_t p_key_length, hash_t p_hash)
{
	uindex_t t_probe;
	t_probe = p_hash % self -> total_buckets;	
	
	if (self -> used_buckets != self -> total_buckets)
	{
		for (uindex_t i = 0; i < self -> total_buckets; i++)
		{
			__MCGCacheTableEntry *t_pair;
			t_pair = &self -> pairs[t_probe];
			
			if (t_pair -> value == NULL || (t_pair -> hash == p_hash && t_pair -> key_length == p_key_length && MCMemoryEqual(p_key, t_pair -> key, p_key_length)))
				return t_probe;
			
			t_probe++;
			if (t_probe >= self -> total_buckets)
				t_probe -= self -> total_buckets;
		}
	}
	
	return UINDEX_MAX;
}

static inline void MCGCacheTableDiscardPair(MCGCacheTableRef self, uindex_t p_pair)
{
	self -> used_buckets--;
	self -> bytes_used -= self -> pairs[p_pair] . key_length;
    
	MCMemoryDelete(self -> pairs[p_pair] . key);
	
	self -> pairs[p_pair] . hash = 0;
	self -> pairs[p_pair] . key_length = 0;
	self -> pairs[p_pair] . key = NULL;
	self -> pairs[p_pair] . value = NULL;
}

////////////////////////////////////////////////////////////////////////////////

bool MCGCacheTableCreate(uindex_t p_size, uindex_t p_max_occupancy, uindex_t p_max_bytes, MCGCacheTableRef &r_cache_table)
{
	bool t_success;
	t_success = true;
	
	__MCGCacheTable *t_cache_table;
	t_cache_table = NULL;
	if (t_success)
		t_success = MCMemoryNew(t_cache_table);
	
	__MCGCacheTableEntry *t_pairs;
	t_pairs = NULL;
	if (t_success)
		t_success = MCMemoryNewArray(p_size, t_pairs);
	
	if (t_success)
	{
		t_cache_table -> total_buckets = p_size;
		t_cache_table -> used_buckets = 0;
		t_cache_table -> max_occupancy = p_max_occupancy;
		t_cache_table -> max_bytes = p_max_bytes;
		t_cache_table -> bytes_used = sizeof(__MCGCacheTable) + sizeof(__MCGCacheTableEntry) * p_size;
		t_cache_table -> pairs = t_pairs;
		
		t_success =  p_max_bytes > t_cache_table -> bytes_used;
	}
	
	if (t_success)
		r_cache_table = t_cache_table;
	else
	{
		MCMemoryDeleteArray(t_pairs);
		MCMemoryDelete(t_cache_table);
	}
	
	return t_success;
}

void MCGCacheTableDestroy(MCGCacheTableRef self)
{
	if (self == NULL)
		return;
	
	if (self -> pairs != NULL)
	{
		for (uindex_t i = 0; i < self -> total_buckets; i++)
			MCMemoryDelete(self -> pairs[i] . key);		
		MCMemoryDeleteArray(self -> pairs);
	}
	
	MCMemoryDelete(self);
}

void MCGCacheTableCompact(MCGCacheTableRef self)
{
	if (self == NULL)
		return;
	
	for (uindex_t i = 0; i < self -> total_buckets; i++)
		MCGCacheTableDiscardPair(self, i);	
}

void MCGCacheTableSet(MCGCacheTableRef self, void *p_key, uint32_t p_key_length, void *p_value, uint32_t p_value_length)
{
	if (self == NULL)
		return;
	
	MCAssert(sizeof(uintptr_t) >= p_value_length);
	
	if (self -> bytes_used >= self -> max_bytes)
	{
		uindex_t t_discard_bucket;
		t_discard_bucket = rand() % self -> total_buckets;
		while (self -> bytes_used >= self -> max_bytes)
		{			
			while (self -> pairs[t_discard_bucket] . key == NULL)
			{
				t_discard_bucket++;
				if (t_discard_bucket >= self -> total_buckets)
					t_discard_bucket -= self -> total_buckets;
			}
			
			MCGCacheTableDiscardPair(self, t_discard_bucket);
			
			//MCLog("MCGCacheTableSet: Max bytes reached. Hash %d discarded.", t_discard_bucket);
		}
	}
	
	if (self -> used_buckets >= self -> max_occupancy)
	{
		uindex_t t_discard_bucket;
		t_discard_bucket = rand() % self -> total_buckets;			
		while (self -> pairs[t_discard_bucket] . key == NULL)
		{
			t_discard_bucket++;
			if (t_discard_bucket >= self -> total_buckets)
				t_discard_bucket -= self -> total_buckets;
		}
		
		MCGCacheTableDiscardPair(self, t_discard_bucket);
		
		//MCLog("MCGCacheTableSet: Max occupancy reached. Hash %d discarded.", t_discard_bucket);
	}
		
	hash_t t_hash;
	t_hash = MCGHashBytes(p_key, p_key_length);
	
	uindex_t t_target_bucket;
	t_target_bucket = MCGCacheTableLookup(self, p_key, p_key_length, t_hash);
	
	if (t_target_bucket == UINDEX_MAX)
	{
		t_target_bucket = t_hash % self -> total_buckets;
		MCMemoryDelete(self -> pairs[t_target_bucket] . key);
        
		self -> bytes_used -= self -> pairs[t_target_bucket] . key_length;
		
		self -> pairs[t_target_bucket] . hash = t_hash;
		self -> pairs[t_target_bucket] . key = p_key;
		self -> pairs[t_target_bucket] . key_length = p_key_length;
		
		self -> pairs[t_target_bucket] . value = NULL;
		MCMemoryCopy(&self -> pairs[t_target_bucket] . value, p_value, p_value_length);
		
		self -> bytes_used += p_key_length;
		
		//MCLog("MCGCacheTableSet: Cache table overflow. Hash %d thrown out.", t_target_bucket);
	}
	else if (self -> pairs[t_target_bucket] . key != NULL)
	{
		MCMemoryDelete(p_key);
        
		self -> pairs[t_target_bucket] . value = NULL;
		MCMemoryCopy(&self -> pairs[t_target_bucket] . value, p_value, p_value_length);
		
		//MCLog("MCGCacheTableSet: Cache table overwrite. Hash %d rewritten.", t_target_bucket);
	}
	else
	{
		self -> pairs[t_target_bucket] . hash = t_hash;
		self -> pairs[t_target_bucket] . key = p_key;
		self -> pairs[t_target_bucket] . key_length = p_key_length;
		MCMemoryCopy(&self -> pairs[t_target_bucket] . value, p_value, p_value_length);
		
		self -> bytes_used += p_key_length;
		self -> used_buckets++;
		
		//MCLog("MCGCacheTableSet: Cache table write. Hash %d written.", t_target_bucket);
	}
}

void *MCGCacheTableGet(MCGCacheTableRef self, void *p_key, uint32_t p_key_length)
{
	if (self == NULL)
		return NULL;	
	
	hash_t t_hash;
	t_hash = MCGHashBytes(p_key, p_key_length);
	
	uindex_t t_target_bucket;
	t_target_bucket = MCGCacheTableLookup(self, p_key, p_key_length, t_hash);
	
	if (t_target_bucket == UINDEX_MAX)
	{
		//MCLog("MCGCacheTableGet: No value found for key. Table full.", NULL);
		return NULL;
	}
	else
	{
		if (self -> pairs[t_target_bucket] . value == NULL)
		{
			//MCLog("MCGCacheTableGet: No value found for key. Value empty", NULL);
			return NULL;
		}
		else
		{
			//MCLog("MCGCacheTableGet: Value found for key in bucket %d.", t_target_bucket);
			return &self -> pairs[t_target_bucket] . value;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

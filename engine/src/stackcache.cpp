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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "stack.h"

////////////////////////////////////////////////////////////////////////////////

#define UINDEX_MAX UINT32_MAX
#define UINDEX_MIN UINT32_MIN
#define UINTPTR_MAX UINT32_MAX
#define UINTPTR_MIN UINT32_MIN

class MCStackIdCache
{
public:
	MCStackIdCache(void);
	~MCStackIdCache(void);
	
	void CacheObject(MCObject *object);
	void UncacheObject(MCObject *object);
	MCObject *FindObject(uint32_t id);
	
	bool RehashBuckets(uindex_t p_new_count);

private:
	static hash_t HashId(uint32_t id);

	uindex_t FindBucket(uint32_t id, hash_t hash);
	uindex_t FindBucketIfExists(uint32_t id, hash_t hash);
	uindex_t FindBucketAfterRehash(uint32_t id, hash_t hash);
	
	uindex_t m_capacity_idx;
	uindex_t m_count;
	uintptr_t *m_buckets;
};

// Prime numbers. Values above 100 have been adjusted up so that the
// malloced block size will be just below a multiple of 512; values
// above 1200 have been adjusted up to just below a multiple of 4096.
 const uindex_t __kMCValueHashTableSizes[] = {
    0, 3, 7, 13, 23, 41, 71, 127, 191, 251, 383, 631, 1087, 1723,
    2803, 4523, 7351, 11959, 19447, 31231, 50683, 81919, 132607,
    214519, 346607, 561109, 907759, 1468927, 2376191, 3845119,
    6221311, 10066421, 16287743, 26354171, 42641881, 68996069,
    111638519, 180634607, 292272623, 472907251,
#ifdef __HUGE__
    765180413UL, 1238087663UL, 2003267557UL, 3241355263UL, 5244622819UL,
#if 0
    8485977589UL, 13730600407UL, 22216578047UL, 35947178479UL,
    58163756537UL, 94110934997UL, 152274691561UL, 246385626107UL,
    398660317687UL, 645045943807UL, 1043706260983UL, 1688752204787UL,
    2732458465769UL, 4421210670577UL, 7153669136377UL,
    11574879807461UL, 18728548943849UL, 30303428750843UL
#endif
#endif
};

const uindex_t __kMCValueHashTableCapacities[] = {
    0, 3, 6, 11, 19, 32, 52, 85, 118, 155, 237, 390, 672, 1065,
    1732, 2795, 4543, 7391, 12019, 19302, 31324, 50629, 81956,
    132580, 214215, 346784, 561026, 907847, 1468567, 2376414,
    3844982, 6221390, 10066379, 16287773, 26354132, 42641916,
    68996399, 111638327, 180634415, 292272755,
#ifdef __HUGE__
    472907503UL, 765180257UL, 1238087439UL, 2003267722UL, 3241355160UL,
#if 0
    5244622578UL, 8485977737UL, 13730600347UL, 22216578100UL,
    35947178453UL, 58163756541UL, 94110935011UL, 152274691274UL,
    246385626296UL, 398660317578UL, 645045943559UL, 1043706261135UL,
    1688752204693UL, 2732458465840UL, 4421210670552UL,
    7153669136706UL, 11574879807265UL, 18728548943682UL
#endif
#endif
};

////////////////////////////////////////////////////////////////////////////////

MCStackIdCache::MCStackIdCache(void)
{
	m_capacity_idx = 0;
	m_count = 0;
	m_buckets = nil;
}

MCStackIdCache::~MCStackIdCache(void)
{
	MCMemoryDeleteArray(m_buckets);
}

void MCStackIdCache::CacheObject(MCObject *p_object)
{
	if (p_object -> getinidcache())
		return;
	
	uint32_t t_id;
	t_id = p_object -> getid();
	
	hash_t t_hash;
	t_hash = HashId(t_id);
	
	uindex_t t_target_slot;
	t_target_slot = FindBucket(t_id, t_hash);
	
	if (t_target_slot == UINDEX_MAX)
	{
		if (!RehashBuckets(1))
			return;
		
		t_target_slot = FindBucketAfterRehash(t_id, t_hash);
	}
	
	if (t_target_slot == UINDEX_MAX)
		return;

	p_object -> setinidcache(true);
	m_buckets[t_target_slot] = (uintptr_t)p_object;
	m_count += 1;
}

void MCStackIdCache::UncacheObject(MCObject *p_object)
{
	if (!p_object -> getinidcache())
		return;
	
	uint32_t t_id;
	t_id = p_object -> getid();
	
	hash_t t_hash;
	t_hash = HashId(t_id);
	
	uindex_t t_target_slot;
	t_target_slot = FindBucketIfExists(t_id, t_hash);
	
	if (t_target_slot != UINDEX_MAX)
	{
		p_object -> setinidcache(false);
		m_buckets[t_target_slot] = UINTPTR_MAX;
		m_count -= 1;
	}
}

MCObject *MCStackIdCache::FindObject(uint32_t p_id)
{
	hash_t t_hash;
	t_hash = HashId(p_id);
	
	uindex_t t_target_slot;
	t_target_slot = FindBucketIfExists(p_id, t_hash);
	
	if (t_target_slot != UINDEX_MAX)
		return (MCObject *)m_buckets[t_target_slot];
	
	return nil;
}

////////////////////////////////////////////////////////////////////////////////

hash_t MCStackIdCache::HashId(uint32_t h)
{
	h ^= (h >> 20) ^ (h >> 12);
    return h ^ (h >> 7) ^ (h >> 4);
}

uindex_t MCStackIdCache::FindBucket(uint32_t p_id, hash_t p_hash)
{
	uindex_t t_capacity;
	t_capacity = __kMCValueHashTableSizes[m_capacity_idx];
	
	uindex_t t_h1;
#if defined(__ARM__) && 0 // TODO
	t_h1 = __MCHashFold(p_hash, m_capacity_idx);
#else
	t_h1 = p_hash % t_capacity;
#endif
	
	uindex_t t_probe;
	t_probe = t_h1;

	uindex_t t_target_slot;
	t_target_slot = UINDEX_MAX;

	for(uindex_t i = 0; i < t_capacity; i++)
	{
		uintptr_t t_bucket;
		t_bucket = m_buckets[t_probe];
		if (t_bucket == UINTPTR_MIN)
		{
			if (t_target_slot == UINDEX_MAX)
				t_target_slot = t_probe;
			break;
		}
		else if (t_bucket == UINTPTR_MAX)
		{
			if (t_target_slot == UINDEX_MAX)
				t_target_slot = t_probe;
		}
		else
		{
			if (p_id == ((MCObject *)t_bucket) -> getid())
				return t_probe;
		}

		t_probe += 1;

		if (t_capacity <= t_probe)
			t_probe -= t_capacity;
	}

	return t_target_slot;
}

uindex_t MCStackIdCache::FindBucketIfExists(uint32_t p_id, hash_t p_hash)
{
	uindex_t t_capacity;
	t_capacity = __kMCValueHashTableSizes[m_capacity_idx];

	uindex_t t_h1;
#if defined(__ARM__) && 0 // TODO
	t_h1 = __MCHashFold(p_hash, m_capacity_idx);
#else
	t_h1 = p_hash % t_capacity;
#endif

	uindex_t t_probe;
	t_probe = t_h1;

	for(uindex_t i = 0; i < t_capacity; i++)
	{
		uintptr_t t_bucket;
		t_bucket = m_buckets[t_probe];

		if (t_bucket == UINTPTR_MIN)
			return UINDEX_MAX;

		if (t_bucket != UINTPTR_MAX &&
			((MCObject *)t_bucket) -> getid() == p_id)
			return t_probe;

		t_probe += 1;

		if (t_capacity <= t_probe)
			t_probe -= t_capacity;
	}

	return UINDEX_MAX;
}

uindex_t MCStackIdCache::FindBucketAfterRehash(uint32_t p_id, hash_t p_hash)
{
	uindex_t t_capacity;
	t_capacity = __kMCValueHashTableSizes[m_capacity_idx];

	uindex_t t_h1;
#if defined(__ARM__) && 0 // TODO
	t_h1 = __MCHashFold(p_hash, m_capacity_idx);
#else
	t_h1 = p_hash % t_capacity;
#endif

	uindex_t t_probe;
	t_probe = t_h1;

	for(uindex_t i = 0; i < t_capacity; i++)
	{
		uintptr_t t_bucket;
		t_bucket = m_buckets[t_probe];

		if (t_bucket == UINTPTR_MIN)
			return t_probe;

		t_probe += 1;

		if (t_capacity <= t_probe)
			t_probe -= t_capacity;
	}

	return UINDEX_MAX;
}

bool MCStackIdCache::RehashBuckets(uindex_t p_new_item_count)
{
	uindex_t t_new_capacity_idx;
	t_new_capacity_idx = m_capacity_idx;
	if (p_new_item_count != 0)
	{
		// If we are shrinking we just shrink down to the level needed by the currently
		// used buckets.
		if (p_new_item_count < 0)
			p_new_item_count = 0;

		// Work out the smallest possible capacity greater than the requested capacity.
		uindex_t t_new_capacity_req;
		t_new_capacity_req = m_count + p_new_item_count;
		for(t_new_capacity_idx = 0; t_new_capacity_idx < 64; t_new_capacity_idx++)
			if (t_new_capacity_req <= __kMCValueHashTableCapacities[t_new_capacity_idx])
				break;
	}

	// Fetch the old capacity and table.
	uindex_t t_old_capacity;
	uintptr_t *t_old_buckets;
	t_old_capacity = __kMCValueHashTableSizes[m_capacity_idx];
	t_old_buckets = m_buckets;

	// Create the new table.
	uindex_t t_new_capacity;
	uintptr_t *t_new_buckets;
	t_new_capacity = __kMCValueHashTableSizes[t_new_capacity_idx];
	if (!MCMemoryNewArray(t_new_capacity, t_new_buckets))
		return false;

	// Update the vars.
	m_capacity_idx = t_new_capacity_idx;
	m_buckets = t_new_buckets;

	// Now rehash the values from the old table.
	for(uindex_t i = 0; i < t_old_capacity; i++)
	{
		if (t_old_buckets[i] != UINTPTR_MIN && t_old_buckets[i] != UINTPTR_MAX)
		{
			uint32_t t_id;
			t_id = ((MCObject *)t_old_buckets[i]) -> getid();
			
			hash_t t_hash;
			t_hash = HashId(t_id);
			
			uindex_t t_target_slot;
			t_target_slot = FindBucketAfterRehash(t_id, t_hash);

			// This assertion should never trigger - something is very wrong if it does!
			MCAssert(t_target_slot != UINDEX_MAX);

			m_buckets[t_target_slot] = t_old_buckets[i];
		}
	}

	// Delete the old table.
	MCMemoryDeleteArray(t_old_buckets);

	// We are done!
	return true;
}

////////////////////////////////////////////////////////////////////////////////
// MM-2014-07-31: [[ ThreadedRendering ]] Updated to ensure only a single thread mutates the ID cache at a time.

void MCStack::cacheobjectbyid(MCObject *p_object)
{
	if (m_id_cache == nil)
	{
		m_id_cache = new MCStackIdCache;
		if (!m_id_cache -> RehashBuckets(1))
		{
			delete m_id_cache;
			m_id_cache = nil;
		}
	}
		
	if (m_id_cache != nil)
		m_id_cache -> CacheObject(p_object);
}

void MCStack::uncacheobjectbyid(MCObject *p_object)
{
	if (m_id_cache == nil)
		return;
		
	m_id_cache -> UncacheObject(p_object);
}

MCObject *MCStack::findobjectbyid(uint32_t p_id)
{
	if (m_id_cache == nil)
		return nil;
		
    MCObject *t_object;
    t_object = m_id_cache -> FindObject(p_id);
    
    return t_object;
}

void MCStack::freeobjectidcache(void)
{
	delete m_id_cache;
}

////////////////////////////////////////////////////////////////////////////////


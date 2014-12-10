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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "stack.h"

#include "systhreads.h"

////////////////////////////////////////////////////////////////////////////////

template<typename K>
class MCStackCache
{
public:
	MCStackCache (void);
	virtual ~MCStackCache (void);

	virtual bool CacheObject (MCObject *object);
	virtual void UncacheObject (MCObject *object);
	MCObject *FindObject (const K & key) const;

	bool RehashBuckets (uindex_t p_new_count);

protected:
	virtual hash_t KeyHash (const K & key) const = 0;
	virtual compare_t KeyCompare (const K &, const K &) const = 0;
	virtual void ObjectGetKey (MCObject *object, K & key) const = 0;

private:
	uindex_t FindBucket (const K & key, hash_t hash) const;
	uindex_t FindBucketIfExists (const K & key, hash_t hash) const;
	uindex_t FindBucketAfterRehash (const K & key, hash_t hash) const;

	uindex_t m_capacity_idx;
	uindex_t m_count;
	uintptr_t *m_buckets;

	static const uindex_t kNOTFOUND = UINDEX_MAX;
	static const uintptr_t kUNUSED = UINTPTR_MIN;
	static const uintptr_t kDELETED = UINTPTR_MAX;
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

template <typename K>
MCStackCache<K>::MCStackCache (void)
	: m_capacity_idx (0), m_count (0), m_buckets (NULL)
{
}

template <typename K>
MCStackCache<K>::~MCStackCache (void)
{
	MCMemoryDeleteArray (m_buckets);
}

template <typename K>
bool
MCStackCache<K>::CacheObject (MCObject *p_object)
{
	MCAssert (p_object != NULL);

	K t_key;
	hash_t t_hash;
	uindex_t t_target_slot;

	ObjectGetKey (p_object, t_key);
	t_hash = KeyHash (t_key);
	t_target_slot = FindBucket (t_key, t_hash);

	if (t_target_slot == kNOTFOUND) /* Grow hashtable */
	{
		if (!RehashBuckets (1)) return false;
		t_target_slot = FindBucketAfterRehash (t_key, t_hash);
	}

	if (t_target_slot == kNOTFOUND) /* No room in hashtable */
		return false;

	m_buckets[t_target_slot] = reinterpret_cast<uintptr_t>(p_object);
	++m_count;

	return true;
}

template <typename K>
void
MCStackCache<K>::UncacheObject (MCObject *p_object)
{
	MCAssert (p_object != NULL);

	K t_key;
	hash_t t_hash;
	uindex_t t_target_slot;

	ObjectGetKey (p_object, t_key);
	t_hash = KeyHash (t_key);
	t_target_slot = FindBucketIfExists (t_key, t_hash);

	if (t_target_slot == kNOTFOUND) /* Not found in hashtable */
		return;

	m_buckets[t_target_slot] = kDELETED;
}

template <typename K>
MCObject *
MCStackCache<K>::FindObject (const K & t_key) const
{
	hash_t t_hash;
	uindex_t t_target_slot;

	t_hash = KeyHash (t_key);
	t_target_slot = FindBucketIfExists (t_key, t_hash);

	if (t_target_slot == kNOTFOUND) /* Not found in hashtable */
		return NULL;

	return reinterpret_cast<MCObject *>(m_buckets[t_target_slot]);
}

template <typename K>
uindex_t
MCStackCache<K>::FindBucket (const K & p_key,
                               hash_t p_hash) const
{
	uindex_t t_capacity, t_h1, t_target_slot;

	t_capacity = __kMCValueHashTableSizes[m_capacity_idx];
	if (t_capacity == 0) return kNOTFOUND;

	/* FIXME do something sensible on ARM ?? */
	t_h1 = p_hash % t_capacity;

	t_target_slot = kNOTFOUND;

	for (uindex_t i = 0; i < t_capacity; ++i)
	{
		/* Compute the trial insertion position, wrapping around */
		uindex_t t_probe;
		t_probe = (t_h1 + i) % t_capacity;

		uintptr_t t_bucket;
		t_bucket = m_buckets[t_probe];

		if (t_bucket == kDELETED)
		{ /* Previously deleted entry. Still need to check that key not present */
			if (t_target_slot == kNOTFOUND)
				t_target_slot = t_probe;
		}
		else if (t_bucket == kUNUSED)
		{ /* Bucket has not yet been filled. */
			if (t_target_slot == kNOTFOUND)
				t_target_slot = t_probe;
			break;
		}
		else
		{ /* Bucket is full. Check the key */
			K t_key;
			ObjectGetKey (reinterpret_cast<MCObject *>(t_bucket), t_key);
			if (0 == KeyCompare (t_key, p_key))
				return t_probe;
		}
	}

	return t_target_slot;
}

template <typename K>
uindex_t
MCStackCache<K>::FindBucketIfExists (const K & p_key,
                                       hash_t p_hash) const
{
	uindex_t t_bucket = FindBucket (p_key, p_hash);

	if (t_bucket == kNOTFOUND) return kNOTFOUND;

	/* Requested key is not currently in the hash table */
	if (m_buckets[t_bucket] == kUNUSED ||
	    m_buckets[t_bucket] == kDELETED)
		return kNOTFOUND;

	return t_bucket;
}

template <typename K>
uindex_t
MCStackCache<K>::FindBucketAfterRehash (const K & p_key,
                                          hash_t p_hash) const
{
	return FindBucket (p_key, p_hash);
}

template <typename K>
bool
MCStackCache<K>::RehashBuckets (uindex_t p_new_item_count)
{
	/* Keep the old capacity and table */
	uindex_t t_old_capacity;
	uintptr_t *t_old_buckets;
	t_old_capacity = __kMCValueHashTableSizes[m_capacity_idx];
	t_old_buckets = m_buckets;

	/* Compute the new capacity and create the new table */
	uindex_t t_new_capacity_idx;
	uindex_t t_new_capacity;
	uintptr_t *t_new_buckets;

	t_new_capacity_idx = m_capacity_idx;
	if (p_new_item_count != 0)
	{
		uindex_t t_required_capacity;

		/* If shrinking, always shrink to the level needed by the
		 * current contents. */
		if (p_new_item_count < 0)
			t_required_capacity = m_count;
		else
			t_required_capacity = m_count + p_new_item_count;

		/* Work out the smallest possible capacity greater than the
		 * requested capacity */
		for (t_new_capacity_idx = 0;
		     (t_new_capacity_idx < 64 &&
		      t_required_capacity <= __kMCValueHashTableCapacities[t_new_capacity_idx]);
		     ++t_new_capacity_idx);
	}

	t_new_capacity = __kMCValueHashTableSizes[t_new_capacity_idx];
	if (!MCMemoryNewArray (t_new_capacity, t_new_buckets))
		return false;

	/* Update the variables */
	m_capacity_idx = t_new_capacity_idx;
	m_buckets = t_new_buckets;

	/* Rehash values from old table */
	for (uindex_t i = 0; i < t_old_capacity; ++i)
	{
		/* Skip empty or deleted buckets */
		if (t_old_buckets[i] == kUNUSED || t_old_buckets[i] == kDELETED)
			continue;

		K t_key;
		hash_t t_hash;
		uindex_t t_target_slot;

		ObjectGetKey (reinterpret_cast<MCObject *>(t_old_buckets[i]), t_key);
		t_hash = KeyHash (t_key);
		t_target_slot = FindBucketAfterRehash (t_key, t_hash);

		/* There should always be enough space for a new entry at this point! */
		MCAssert (t_target_slot != kNOTFOUND);

		m_buckets[t_target_slot] = t_old_buckets[i];
	}

	/* Delete the old table */
	MCMemoryDeleteArray (t_old_buckets);

	return true;
}

////////////////////////////////////////////////////////////////////////////////

class MCStackIdCache : public MCStackCache<uint32_t>
{
protected:
	virtual hash_t KeyHash (const uint32_t & key) const;
	virtual compare_t KeyCompare (const uint32_t &, const uint32_t &) const;
	virtual void ObjectGetKey (MCObject *object, uint32_t & key) const;
};

hash_t
MCStackIdCache::KeyHash (const uint32_t & key) const
{
	uint32_t h;
	h = key;
	h ^= (h >> 20) ^ (h >> 12);
	h ^= (h >> 7) ^ (h >> 4);
	return h;
}

compare_t
MCStackIdCache::KeyCompare (const uint32_t & a,
                            const uint32_t & b) const
{
	return (a - b);
}

void
MCStackIdCache::ObjectGetKey (MCObject *object,
                              uint32_t & key) const
{
	key = object->getid ();
}

////////////////////////////////////////////////////////////////////////////////
// MM-2014-07-31: [[ ThreadedRendering ]] Updated to ensure only a single thread mutates the ID cache at a time.

void MCStack::cacheobjectbyid(MCObject *p_object)
{
	if (p_object->getinidcache ())
		return;

    MCThreadMutexLock(m_id_cache_lock);
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
		p_object->setinidcache (m_id_cache -> CacheObject(p_object));
    MCThreadMutexUnlock(m_id_cache_lock);
}

void MCStack::uncacheobjectbyid(MCObject *p_object)
{
	if (m_id_cache == nil)
		return;
	if (!p_object->getinidcache ())
		return;
		
    MCThreadMutexLock(m_id_cache_lock);
	m_id_cache -> UncacheObject(p_object);
	p_object->setinidcache (false);
    MCThreadMutexUnlock(m_id_cache_lock);
}

MCObject *MCStack::findobjectbyid(uint32_t p_id)
{
	if (m_id_cache == nil)
		return nil;
		
    MCThreadMutexLock(m_id_cache_lock);
    MCObject *t_object;
    t_object = m_id_cache -> FindObject(p_id);
    MCThreadMutexUnlock(m_id_cache_lock);
    
    return t_object;
}

void MCStack::freeobjectidcache(void)
{
    MCThreadMutexLock(m_id_cache_lock);
	delete m_id_cache;
    MCThreadMutexUnlock(m_id_cache_lock);
}

////////////////////////////////////////////////////////////////////////////////


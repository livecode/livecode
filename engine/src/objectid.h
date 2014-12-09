/*                                                                   -*- c++ -*-
Copyright (C) 2003-2014 Runtime Revolution Ltd.

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

#ifndef _MC_OBJECTID_H_
#define _MC_OBJECTID_H_

#include "uuid.h"

/* ----------------------------------------------------------------
 * MCObjectId (abstract)
 * ---------------------------------------------------------------- */

/* An MCObjectId contains the information about how to reference an
 * MCObject instance.  Each MCObject encapsulates exactly one
 * MCObjectId that references it and contains all of its identifying
 * information. */

class MCObject;

/* Abstract base class */
class MCObjectId
{
public:
	typedef uint32_t id_t;

	/* Return the object instance referenced by this object id, or
	 * NULL if the referenced object cannot be found. */
	virtual MCObject *Get (void) const = 0;

	virtual void SetUuid (const MCUuid &) = 0;
	virtual bool HasUuid (void) const = 0;
	virtual bool GetUuid (MCUuid & r_uuid) const = 0;

	virtual void SetId (id_t) = 0;
	virtual bool HasId (void) const = 0;
	virtual id_t GetId (void) const = 0;

	virtual void SetName (MCNameRef) = 0;
	virtual bool HasName (void) const = 0;
	virtual MCNameRef GetName (void) const = 0;
};

/* ----------------------------------------------------------------
 * MCDelayedObjectId
 * ---------------------------------------------------------------- */

/* MCDelayedObjectId is an MCObjectId that encapsulates identifying
 * information without being tied to a specific MCObject instance.  At
 * any time, its MCDelayedObjectId::Resolve() method may be used to
 * attempt to obtain an MCConcreteObjectId. */
class MCDelayedObjectId : public MCObjectId
{
public:
	/* There are several different combinations of information that can be
	 * used to create a delayed object ID. */
	MCDelayedObjectId ();

	/* Create a delayed object ID with the same identifying information as
	 * in another object ID */
	MCDelayedObjectId (const MCObjectId &);
	MCDelayedObjectId (const MCDelayedObjectId &);

	/* An object can only be uniquely identified by id or name if
	 * these are resolved relative to a stack.  A context must be
	 * provided in the form of an object instance or object ID, and
	 * must be a stack or an object that is part of a stack. */
	MCDelayedObjectId (id_t, const MCObject *);
	MCDelayedObjectId (MCNameRef, const MCObject *);
	MCDelayedObjectId (id_t, const MCObjectId *);
	MCDelayedObjectId (MCNameRef, const MCObjectId *);

	/* UUIDs may be resolved relative to a stack. */
	MCDelayedObjectId (MCUuid p_uuid, const MCObject *p_context);
	MCDelayedObjectId (MCUuid p_uuid, const MCObjectId *p_context=NULL);

	/* Attempt to obtain a concrete object ID for the object instance
	 * referenced by this delayed object ID.  Analogous to "ending the
	 * delay".  Returns NULL if the object ID can't be resolved
	 * yet. */
	virtual const MCObjectId *Resolve (void) const;

	/* Resolves this object ID and obtains the underlying object from
	 * the resulting concrete object ID.  Returns NULL if the object
	 * ID can't be resolved yet. */
	virtual MCObject *Get (void) const;

	virtual void SetUuid (const MCUuid &);
	virtual bool HasUuid (void) const { return m_have_uuid; }
	/* If no UUID is set, returns false */
	virtual bool GetUuid (MCUuid &) const;

	virtual void SetId (id_t p_id);
	virtual bool HasId (void) const { return m_have_id; }
	/* If no ID is set, returns 0 */
	virtual id_t GetId (void) const { return m_have_id ? m_id : 0; }

	virtual void SetName (MCNameRef p_name);
	virtual bool HasName (void) const { return (m_name != NULL); }
	virtual MCNameRef GetName (void) const { return m_name; }

	virtual void SetContext (const MCObjectId *);
	virtual void SetContext (const MCObject *);
	virtual bool HasContext (void) const { return (m_context != NULL); }
	virtual const MCObjectId *GetContext (void) const {return m_context; }

	~MCDelayedObjectId (void);

protected:
	const MCObjectId *m_context;
	MCUuid m_uuid;
	bool m_have_uuid;
	id_t m_id;
	bool m_have_id;
	MCNameRef m_name;
};

#endif /* !_MC_OBJECTID_H_ */

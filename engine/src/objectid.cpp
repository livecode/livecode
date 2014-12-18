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

#include "prefix.h"

#include "objdefs.h"
#include "parsedef.h"

#include "object.h"
#include "dispatch.h"
#include "stack.h"
#include "exec.h"

#include "objectid.h"

/* ================================================================
 * MCDelayedObjectId
 * ================================================================ */

/* The delayed object ID is a reference to an MCObject instance that
 * may not exist yet. */

/* ----------------------------------------------------------------
 * Construction
 * ---------------------------------------------------------------- */

MCDelayedObjectId::MCDelayedObjectId ()
	: m_context (NULL),
	  m_have_uuid (false),
	  m_id (0),
	  m_have_id (false),
	  m_name (NULL)
{}

MCDelayedObjectId::MCDelayedObjectId (const MCObjectId * id)
	: m_context (NULL)
{
	m_have_uuid = id->HasUuid();
	if (m_have_uuid)
		id->GetUuid (m_uuid);

	m_have_id = id->HasId();
	if (m_have_id)
		m_id = id->GetId();

	m_name = id->HasName() ? MCValueRetain (id->GetName()) : NULL;
}

MCDelayedObjectId::MCDelayedObjectId (const MCDelayedObjectId * id)
	: m_context (id->m_context),
	  m_uuid (id->m_uuid),
	  m_have_uuid (id->m_have_uuid),
	  m_id (id->m_id),
	  m_have_id (id->m_have_id),
	  m_name (MCValueRetain (id->m_name))
{
}

MCDelayedObjectId::MCDelayedObjectId (MCUuid p_uuid,
                                      const MCObject *p_context)
	: m_id (0),
	  m_have_id (false),
	  m_name (NULL)
{
	SetUuid (p_uuid);
	SetContext (p_context);
}

MCDelayedObjectId::MCDelayedObjectId (MCUuid p_uuid,
                                      const MCObjectId *p_context)
	: m_id (0),
	  m_have_id (false),
	  m_name (NULL)
{
	SetUuid (p_uuid);
	SetContext (p_context);
}

MCDelayedObjectId::MCDelayedObjectId (MCObjectId::id_t p_id,
                                      const MCObject *p_context)
	: m_have_uuid (false),
	  m_name (NULL)
{
	SetId (p_id);
	SetContext (p_context);
}

MCDelayedObjectId::MCDelayedObjectId (MCObjectId::id_t p_id,
                                      const MCObjectId *p_context)
	: m_have_uuid (false),
	  m_name (NULL)
{
	SetId (p_id);
	SetContext (p_context);
}

MCDelayedObjectId::MCDelayedObjectId (MCNameRef p_name,
                                      const MCObject *p_context)
	: m_have_uuid (false),
	  m_id (0),
	  m_have_id (false),
	  m_name (NULL)
{
	SetName (p_name);
	SetContext (p_context);
}

MCDelayedObjectId::MCDelayedObjectId (MCNameRef p_name,
                                      const MCObjectId *p_context)
	: m_have_uuid (false),
	  m_id (0),
	  m_have_id (false),
	  m_name (NULL)
{
	SetName (p_name);
	SetContext (p_context);
}

/* ----------------------------------------------------------------
 * Destruction
 * ---------------------------------------------------------------- */

MCDelayedObjectId::~MCDelayedObjectId (void)
{
	MCValueRelease (m_name);
}

/* ----------------------------------------------------------------
 * UUIDs
 * ---------------------------------------------------------------- */

void
MCDelayedObjectId::SetUuid (const MCUuid & p_uuid)
{
	m_uuid = p_uuid;
	m_have_uuid = true;
}

bool
MCDelayedObjectId::GetUuid (MCUuid & r_uuid) const
{
	if (!m_have_uuid) return false;
	r_uuid = m_uuid;
	return true;
}

/* ----------------------------------------------------------------
 * ID & name resolution contexts
 * ---------------------------------------------------------------- */

void
MCDelayedObjectId::SetContext (const MCObject *p_context)
{
	MCAssert (p_context);
	SetContext(&p_context->GetObjectId ());
}

void
MCDelayedObjectId::SetContext (const MCObjectId *p_context)
{
	m_context = p_context;
}

/* ----------------------------------------------------------------
 * IDs
 * ---------------------------------------------------------------- */

void
MCDelayedObjectId::SetId (MCObjectId::id_t p_id)
{
	m_id = p_id;
	m_have_id = true;
}

/* ----------------------------------------------------------------
 * Names
 * ---------------------------------------------------------------- */

void
MCDelayedObjectId::SetName (MCNameRef p_name)
{
	MCAssert (p_name);
	MCValueRelease (m_name);
	m_name = MCValueRetain (p_name);
}

/* ----------------------------------------------------------------
 * Resolution
 * ---------------------------------------------------------------- */

const MCObjectId *
MCDelayedObjectId::Resolve (void) const
{
	MCObject *t_obj = Get();

	if (!t_obj)
		return NULL;

	return &(Get()->GetObjectId());
}

MCObject *
MCDelayedObjectId::Get (void) const
{
	MCObject *t_obj = NULL;

	/* Resolve context object and acquire context stack */
	MCStack *t_context_stack;
	if (HasContext ())
	{
		MCObject *t_context_obj;
		t_context_obj = GetContext () -> Get ();
		t_context_stack = t_context_obj ? t_context_obj->getstack () : NULL;
	}
	else
		t_context_stack = NULL;

	if (HasUuid ())
	{
		MCUuid t_uuid;
		GetUuid (t_uuid);
		if (!t_obj && t_context_stack)
			t_obj = t_context_stack->GetObjectByUuid (t_uuid);
		else if (!t_obj)
			t_obj = MCdispatcher->GetObjectByUuid (t_uuid);
	}

	if (HasId ())
	{
		if (t_context_stack)
		{
			if (!t_obj) /* Stacks (& substacks) */
				t_obj = t_context_stack->findstackid (GetId ());
			if (!t_obj) /* Controls */
				t_obj = t_context_stack->getobjid (CT_ANY, GetId ());
			if (!t_obj) /* Audio */
				t_obj = t_context_stack->getobjid (CT_AUDIO_CLIP, GetId ());
			if (!t_obj) /* Video */
				t_obj = t_context_stack->getobjid (CT_VIDEO_CLIP, GetId ());
		}
		else
		{
			if (!t_obj) /* Stacks (& substacks) */
				t_obj = MCdispatcher->findstackid (GetId ());
			if (!t_obj) /* Controls */
				t_obj = MCdispatcher->getobjid (CT_ANY, GetId ());
			if (!t_obj) /* Audio */
				t_obj = MCdispatcher->getobjid (CT_AUDIO_CLIP, GetId ());
			if (!t_obj) /* Video */
				t_obj = MCdispatcher->getobjid (CT_VIDEO_CLIP, GetId ());
		}
	}

	if (HasName ())
	{
		t_obj = NULL;
		if (t_context_stack)
		{
			if (!t_obj) /* Stacks (& substacks) */
				t_obj = t_context_stack->findstackname (GetName ());
			if (!t_obj) /* Controls */
				t_obj = t_context_stack->getobjname (CT_ANY, GetName ());
			if (!t_obj) /* Audio */
				t_obj = t_context_stack->getobjname (CT_AUDIO_CLIP, GetName ());
			if (!t_obj) /* Video */
				t_obj = t_context_stack->getobjname (CT_VIDEO_CLIP, GetName ());
		}
		else
		{
			if (!t_obj) /* Stacks (& substacks) */
				t_obj = MCdispatcher->findstackname (GetName ());
			if (!t_obj) /* Controls */
				t_obj = MCdispatcher->getobjname (CT_ANY, GetName ());
			if (!t_obj) /* Audio */
				t_obj = MCdispatcher->getobjname (CT_AUDIO_CLIP, GetName ());
			if (!t_obj) /* Video */
				t_obj = MCdispatcher->getobjname (CT_VIDEO_CLIP, GetName ());
		}
	}

	return t_obj;
}

/* ================================================================
 * MCObject::MCConcreteObjectId
 * ================================================================ */

void
MCObject::MCConcreteObjectId::SetId (MCObjectId::id_t p_id)
{
	/* It's better to use the exec function because it ensures that
	 * everything that depends on the ID is correctly updated (e.g. ID
	 * lookup caches) */
	MCExecContext t_ctxt;
	Get()->SetId (t_ctxt, p_id);
}

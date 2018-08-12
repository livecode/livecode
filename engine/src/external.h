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

#ifndef __MC_EXTERNAL__
#define __MC_EXTERNAL__

////////////////////////////////////////////////////////////////////////////////

typedef bool (*MCExternalListHandlersCallback)(void *state, Handler_type type, const char *name, uint32_t index);

class MCExternal
{
public:
	MCExternal(void);
	virtual ~MCExternal(void);

	virtual const char *GetName(void) const = 0;
	virtual Handler_type GetHandlerType(uint32_t index) const = 0;
	virtual bool ListHandlers(MCExternalListHandlersCallback callback, void *state) = 0;
	virtual Exec_stat Handle(MCObject *p_context, Handler_type p_type, uint32_t p_index, MCParameter *p_parameters) = 0;
	
	static MCExternal *Load(MCStringRef p_filename);
	static void Unload(MCExternal *p_external);

	// Called on exit to cleanup all external instances.
	static void Cleanup(void);

protected:
	virtual bool Prepare(void) = 0;
	virtual bool Initialize(void) = 0;
	virtual void Finalize(void) = 0;

	MCExternal *m_next;

	uint32_t m_references;
	MCSAutoLibraryRef m_module;
	const char *m_name;

	static MCExternal *s_externals;
};

////////////////////////////////////////////////////////////////////////////////

// An external handler maps a handler name to external and handler index. The
// external index is the position in the MCExternalHandlerList's extenals
// array, while the handler index is private to the external and uniquely
// identifies the corresponding handler.
struct MCExternalHandlerListEntry
{
	MCNameRef name;
	uint16_t external;
	uint16_t handler;
};

// An external handler list manages a collection of handlers whose
// implementation is in externals. The object merges the handler tables from
// all its externals into a single, flat binary-searchable list. Both MCStack
// and MCDispatch use this object to manage their external lists.
class MCExternalHandlerList
{
public:
	MCExternalHandlerList(void);
	~MCExternalHandlerList(void);

	// Returns true if the handler list is empty.
	bool IsEmpty(void);

	// Create a return-delimited list of external names.
	bool ListExternals(MCStringRef& r_list);

	// Create a return-delimited list of external handlers of the given type.
	bool ListHandlers(Handler_type p_type, MCStringRef& r_list);

	// Looks to see if there is a handler of the given type.
	bool HasHandler(MCNameRef handler, Handler_type type);
	
	// Attempt to load the external with the given path.
	bool Load(MCStringRef p_external);

	// Attempt to handle the given message.
	Exec_stat Handle(MCObject *p_object, Handler_type type, MCNameRef p_message, MCParameter *p_parameters);

private:
	// Lookup name in the handler list, returning true if it is found, false
	// if not. In either case 'r_index' is set to the location the entry
	// would be at.
	bool Lookup(MCNameRef name, uindex_t& r_index);

	// This is the callback to MCExternal::ListHandlers, used to build up the
	// handler list.
	static bool AddHandler(void *state, Handler_type type, const char *name, uint32_t index);

	// The list of externals the list managers.
	MCRawArray<MCExternal *> m_externals;

	// The aggregated list of handlers from all the managed externals.
	MCRawArray<MCExternalHandlerListEntry> m_handlers;
};

////////////////////////////////////////////////////////////////////////////////

#endif

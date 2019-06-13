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
#include "mcio.h"


#include "scriptpt.h"
#include "dispatch.h"
#include "stack.h"
#include "tooltip.h"
#include "card.h"
#include "field.h"
#include "button.h"
#include "image.h"
#include "aclip.h"
#include "vclip.h"
#include "stacklst.h"
#include "mcerror.h"
#include "hc.h"
#include "util.h"
#include "param.h"
#include "debug.h"
#include "statemnt.h"
#include "funcs.h"
#include "magnify.h"
#include "sellst.h"
#include "undolst.h"
#include "styledtext.h"
#include "property.h"
#include "internal.h"
#include "globals.h"
#include "license.h"
#include "mode.h"
#include "revbuild.h"
#include "deploy.h"
#include "capsule.h"
#include "player.h"
#include "minizip.h"
#include "bsdiff.h"
#include "osspec.h"

#if defined(_MAC_DESKTOP)
#include "osxprefix.h"
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <syslog.h>
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  Globals specific to INSTALLER mode
//

// The MCCapsule info structure records a block of data that has been inserted
// into the executable by the deploy command. The blocks of data are inserted
// as distinct 'sections', and thus preserve the integrity of the executable.

// Something that is not entirely obvious, but follows directly from the static
// nature of executables is that (at least on Windows) the compiler assumes it
// can fix the address of sections at the point of compilation. This means that
// we cannot use the variable addresses directly, and must seach for the
// section's address dynamically at runtime.

struct MCCapsuleInfo
{
	uint32_t size;
	uint32_t data[3];
};

// Define the needed sections on a platform-by-platform basis.
#if defined(_WINDOWS)

#define PAYLOAD_SECTION_NAME ".payload"
#define PROJECT_SECTION_NAME ".project"

#pragma section(".payload", read, discard)
__declspec(allocate(".payload")) volatile MCCapsuleInfo MCpayload = {};
#pragma section(".project", read, discard)
__declspec(allocate(".project")) volatile MCCapsuleInfo MCcapsule = {};

#elif defined(_LINUX)

#define PAYLOAD_SECTION_NAME ".payload"
#define PROJECT_SECTION_NAME ".project"

__attribute__((section(".payload"))) volatile MCCapsuleInfo MCpayload = {};
__attribute__((section(".project"))) volatile MCCapsuleInfo MCcapsule = {};

#elif defined(_MACOSX)

#define PAYLOAD_SECTION_NAME "__PAYLOAD"
#define PROJECT_SECTION_NAME "__PROJECT"

__attribute__((section("__PROJECT,__project"),__used__)) volatile MCCapsuleInfo MCcapsule = {};

#endif

// The default license parameters to use for the installer engine.
MCLicenseParameters MClicenseparameters =
{
	NULL, NULL, NULL, kMCLicenseClassNone, 0,
	10, 10, 50, 10,
	0,
	NULL,
};

// This method is implemented per-platform and locates a named section in the 
// current exe.
static void *MCExecutableFindSection(const char *p_name);

// We don't include error string in this mode
const char *MCparsingerrors = "";
const char *MCexecutionerrors = "";

////////////////////////////////////////////////////////////////////////////////
//
//  Property tables specific to INSTALLER mode
//

MCObjectPropertyTable MCObject::kModePropertyTable =
{
	nil,
	0,
	nil,
};

MCObjectPropertyTable MCStack::kModePropertyTable =
{
	nil,
	0,
	nil,
};

MCPropertyTable MCProperty::kModePropertyTable =
{
	0,
	nil,
};

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of internal payload commands
//

static MCMiniZipRef s_payload_minizip = nil;

#ifdef _MACOSX
static void *s_payload_loaded_data = nil;
static void *s_payload_mapped_data = nil;
static uint32_t s_payload_mapped_size = 0;
#else
#ifdef _DEBUG
static void *s_payload_data = nil;
#endif
#endif

#ifdef _WINDOWS
// MM-2011-03-23: If the payload is specified via file, we need to preserve (and clean up) 
// a handle to the file, a mapping object and a pointer to where we map the file contents.
HANDLE s_payload_file_handle = nil;
HANDLE s_payload_file_map = nil;
static void *s_payload_mapped_data = nil;
#endif

class MCInternalPayloadOpen: public MCStatement
{
public:
	MCInternalPayloadOpen(void)
	{
		m_filename = nil;
	}

	~MCInternalPayloadOpen(void)
	{
		delete m_filename;
	}

	Parse_stat parse(MCScriptPoint& sp)
	{
		initpoint(sp);
		sp . parseexp(False, True, &m_filename);
		return PS_NORMAL;
	}

	void exec_ctxt(MCExecContext& ctxt)
	{
		// Do nothing if its already open
		if (s_payload_minizip != nil)
			return;
		
		// MM-2011-03-23: Added optional paramater, allowing the payload to be specified by file path.

		const void *t_payload_data;
		t_payload_data = nil;
		uint32_t t_payload_size;

		// MM-2011-03-23: If a file is specified, fetch the payload from the file.
		if (m_filename != nullptr)
		{
            MCAutoStringRef t_string;
            if (!ctxt.EvalExprAsStringRef(m_filename, EE_UNDEFINED, &t_string))
                return;

            MCAutoStringRefAsCString t_filename;
            /* UNCHECKED */ t_filename.Lock(*t_string);

			mmap_payload_from_file(*t_filename, t_payload_data, t_payload_size);
			if (t_payload_data == nil)
			{
				ctxt . SetTheResultToCString("could not load paylod from file");
				return;
			}
		}

		// MM-2011-03-23: If no payload file specified, then extract payload as before, from installer.
		if (t_payload_data == nil)
		{
#ifdef _MACOSX		
            // Force a reference to the project section to prevent extra-clever
            // optimising linkers from discarding the section.
            (void)MCcapsule.size;
            
            // On Mac OS X, the payload is in a separate file.
			// MM-2011-03-23: Refactored code to use method call.
			MCAutoStringRef t_payload_file;
            uindex_t t_last_slash;
            if (!MCStringLastIndexOfChar(MCcmd, '/', UINDEX_MAX, kMCCompareExact, t_last_slash))
                t_last_slash = UINDEX_MAX;
            MCRange t_range = MCRangeMake(0, t_last_slash);
            /* FRAGILE */ if (MCStringFormat(&t_payload_file, "%*@/payload", &t_range, MCcmd))
			{
                MCAutoStringRefAsUTF8String t_utf8_payload_file;
                /* UNCHECKED */ t_utf8_payload_file . Lock(*t_payload_file);
                mmap_payload_from_file(*t_utf8_payload_file, t_payload_data, t_payload_size);
				if(t_payload_data == nil)
				{
					ctxt . SetTheResultToCString("could not find payload");
					return;
				}			
			}			
#else
            // Force references to the payload and project sections to prevent
            // extra-clever optimising linkers from discarding the sections.
            (void)MCpayload.size;
            (void)MCcapsule.size;
            
			// Search for the payload section - first see if there is a payload
			// section of suitable size; then if in debug mode, try to load a stack
			// via env var.
			MCCapsuleInfo *t_info;
			t_info = (MCCapsuleInfo *)MCExecutableFindSection(PAYLOAD_SECTION_NAME);
			if (t_info != nil && t_info -> size > sizeof(MCCapsuleInfo))
			{
				t_payload_data = t_info -> data;
				t_payload_size = t_info -> size;
			}
			else
			{
#ifdef _DEBUG
				MCAutoStringRef t_payload_file;
				MCAutoDataRef t_payload_dataref;
				/* UNCHECKED */ MCStringCreateWithCString(getenv("TEST_PAYLOAD"), &t_payload_file);
				/* UNCHECKED */ MCS_loadbinaryfile(*t_payload_file, &t_payload_dataref);
				if (MCDataGetLength(*t_payload_dataref) == 0)
				{
					MCresult -> sets("could not load payload");
					return;
				}

				if (!MCMemoryAllocate(MCDataGetLength(*t_payload_dataref), s_payload_data))
				{
					MCresult -> sets("out of memory while loading payload");
					return;
				}

				MCMemoryCopy(s_payload_data, (void*)MCDataGetBytePtr(*t_payload_dataref), MCDataGetLength(*t_payload_dataref));

				t_payload_data = s_payload_data;
				t_payload_size = MCDataGetLength(*t_payload_dataref);
#else
				MCresult -> sets("could not find payload");
				return;
#endif
			}
#endif

		}

		// Open the payload as a minizip
		if (!MCMiniZipOpen(t_payload_data, t_payload_size, s_payload_minizip))
		{
			ctxt . SetTheResultToCString("could not open payload");
			return;
		}

		// Empty result means success
        ctxt . SetTheResultToEmpty();

		return;
	}

private:
	MCExpression *m_filename;

	// MM-2011-03-23: Takes a file path and memory maps its contents to r_payload_data, also returning the file size.
	static bool mmap_payload_from_file(const char *p_file_name, const void *&r_payload_data, uint32_t &r_payload_size)
	{
        bool t_success;
#if defined(_MACOSX)
		// The OS X code is just refactored from the method funciton.
		s_payload_mapped_data = nil;
		s_payload_mapped_size = 0;

		int32_t t_fd;
		t_fd = open(p_file_name, O_RDONLY, 0);
		if (t_fd >= 0)
		{
			struct stat t_info;
			if (fstat(t_fd, &t_info) == 0)
			{
				s_payload_mapped_data = mmap(nil, t_info . st_size, PROT_READ, MAP_FILE | MAP_PRIVATE, t_fd, 0);
				s_payload_mapped_size = t_info . st_size;
				
				if (s_payload_mapped_data == nil)
				{
					s_payload_loaded_data = malloc(t_info . st_size);
					if (s_payload_loaded_data != nil)
					{
						if (read(t_fd, s_payload_loaded_data, t_info . st_size) != t_info . st_size)
						{
							free(s_payload_loaded_data);
							s_payload_loaded_data = nil;
						}
					}
				}
							
			}
			close(t_fd);
		}

		if (s_payload_mapped_data == nil && s_payload_loaded_data == nil)
            t_success = false;
        else
        {
            r_payload_data = s_payload_mapped_data != nil ? s_payload_mapped_data : s_payload_loaded_data;
            r_payload_size = s_payload_mapped_size;
            t_success = true;
        }
#elif defined(_WINDOWS)
		s_payload_file_handle = nil;
		s_payload_file_map = nil;
		s_payload_mapped_data = nil;

        // Fetch a handle to the file and map the contents to memory.
		t_success = true;
		if (t_success)
		{
			s_payload_file_handle = CreateFileA(p_file_name, GENERIC_READ, FILE_SHARE_READ, nil, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, nil);
			if (s_payload_file_handle == INVALID_HANDLE_VALUE)
				t_success = false;
		}
		if (t_success)
		{
			s_payload_file_map = CreateFileMappingA(s_payload_file_handle, nil, PAGE_READONLY, 0, 0, nil);
			if (s_payload_file_map == nil)
				t_success = false;
		}
		if (t_success)
		{
			s_payload_mapped_data = MapViewOfFile(s_payload_file_map, FILE_MAP_READ, 0, 0, 0);
			if (s_payload_mapped_data == nil)
				t_success = false;
		}
		
		if (t_success)
		{
			r_payload_data = s_payload_mapped_data;
			r_payload_size = GetFileSize(s_payload_file_handle, nil);
		}
		else
		{
			if (s_payload_mapped_data != nil)
				UnmapViewOfFile(s_payload_mapped_data);
			if (s_payload_file_map != nil)
				CloseHandle(s_payload_file_map);
			if (s_payload_file_handle != INVALID_HANDLE_VALUE)
				CloseHandle(s_payload_file_handle);
        }
#else
        t_success = false;
#endif

        return t_success;
	}
};

class MCInternalPayloadClose: public MCStatement
{
public:
	MCInternalPayloadClose(void) {}
	~MCInternalPayloadClose(void) {}

	void exec_ctxt(MCExecContext& ctxt)
	{
		// Don't do anything if the payload isn't open
		if (s_payload_minizip == nil)
			return;

		// Close the payload
		MCMiniZipClose(s_payload_minizip);
		s_payload_minizip = nil;

#ifdef _MACOSX
		if (s_payload_mapped_data != nil)
			munmap(s_payload_mapped_data, s_payload_mapped_size);
		if (s_payload_loaded_data != nil)
			free(s_payload_loaded_data);
#else
		
#ifdef _WINDOWS
		// MM-2011-03-23: Clean up any handles we have opened (if payload has been specified by file).
		if (s_payload_mapped_data != nil)
			UnmapViewOfFile(s_payload_mapped_data);
		if (s_payload_file_map != nil)
			CloseHandle(s_payload_file_map);
		if (s_payload_file_handle != INVALID_HANDLE_VALUE)
			CloseHandle(s_payload_file_handle);
#endif

#ifdef _DEBUG
		MCMemoryDeallocate(s_payload_data);
#endif
		
#endif

		return;
	}
};

class MCInternalPayloadList: public MCStatement
{
public:
	MCInternalPayloadList(void)
	{
	}
	
	~MCInternalPayloadList(void)
	{
	}

	Parse_stat parse(MCScriptPoint& sp)
	{
		initpoint(sp);

		return PS_NORMAL;
	}

	void exec_ctxt(MCExecContext& ctxt)
	{
		// Don't do anything if the payload isn't open
		if (s_payload_minizip == nil)
		{
			ctxt . SetTheResultToCString("payload not open");
			return;
		}

        MCListRef t_list;
        /* UNCHECKED */ MCListCreateMutable(EC_RETURN, t_list);
		MCMiniZipListItems(s_payload_minizip, list_items, t_list);
        MCAutoStringRef t_string;
        /* UNCHECKED */ MCListCopyAsStringAndRelease(t_list, &t_string);
        ctxt . SetItToValue(*t_string);

		return;
	}

private:
	static bool list_items(void *r_list, MCStringRef p_item)
	{
        return MCListAppend((MCListRef&)r_list, p_item);
	}
};

class MCInternalPayloadDescribe: public MCStatement
{
public:
	MCInternalPayloadDescribe(void)
	{
		m_item_expr = nil;
	}

	~MCInternalPayloadDescribe(void)
	{
		delete m_item_expr;
	}

	Parse_stat parse(MCScriptPoint& sp)
	{
		initpoint(sp);

		if (sp . parseexp(False, True, &m_item_expr) != PS_NORMAL)
		{
			MCperror -> add(PE_PUT_BADEXP, sp);
			return PS_ERROR;
		}

		return PS_NORMAL;
	}

	void exec_ctxt(MCExecContext& ctxt)
	{
        MCAutoStringRef t_name;

        if (!ctxt . EvalExprAsStringRef(m_item_expr, EE_PUT_BADEXP, &t_name))
        {
            return;
        }
        
		if (s_payload_minizip != nil)
		{
			MCMiniZipItemInfo t_info;
			if (MCMiniZipDescribeItem(s_payload_minizip, *t_name, t_info))
			{
                MCAutoStringRef t_string;
				MCStringFormat(&t_string, ",%u,%u,,%u,", t_info . checksum, t_info . uncompressed_size, t_info . compressed_size);
                
                ctxt.SetTheResultToEmpty();
                ctxt . SetItToValue(*t_string);
			}
			else
            {
                ctxt . SetTheResultToCString("describe failed");
            }
		}
		else
        {
            ctxt . SetTheResultToCString("payload not open");
        }

		return;
	}

private:
	MCExpression *m_item_expr;
};

class MCInternalPayloadExtract: public MCStatement
{
public:
	MCInternalPayloadExtract(void)
	{
		m_item_expr = nil;
		m_file_expr = nil;
	}

	~MCInternalPayloadExtract(void)
	{
		delete m_item_expr;
		delete m_file_expr;
	}

	Parse_stat parse(MCScriptPoint& sp)
	{
		initpoint(sp);

		if (sp . parseexp(False, True, &m_item_expr) != PS_NORMAL)
		{
			MCperror -> add(PE_PUT_BADEXP, sp);
			return PS_ERROR;
		}

		if (sp . skip_token(SP_FACTOR, TT_TO, PT_TO) == PS_NORMAL &&
			sp . parseexp(False, True, &m_file_expr) != PS_NORMAL)
		{
			MCperror -> add(PE_PUT_BADEXP, sp);
			return PS_ERROR;
		}

		return PS_NORMAL;
	}

	void exec_ctxt(MCExecContext& ctxt)
	{
		MCAutoStringRef t_item;
        if (!ctxt . EvalExprAsStringRef(m_item_expr, EE_PUT_BADEXP, &t_item))
            return;
				
		MCAutoStringRef t_file;
        if (!ctxt . EvalOptionalExprAsStringRef(m_file_expr, kMCEmptyString, EE_PUT_BADEXP, &t_file))
            return;
		
		if (s_payload_minizip != nil)
		{
			ExtractContext t_context;
			t_context . target = MCtargetptr;
			t_context . name = *t_item;
            if (!ctxt.GetIt()->evalcontainer(ctxt, t_context.var))
                return;
			t_context . stream = nil;

			if (!MCStringIsEmpty(*t_file))
				t_context . stream = MCS_open(*t_file, kMCOpenFileModeWrite, False, False, 0);

			t_context.var.clear();
			if (MCStringIsEmpty(*t_file) || t_context . stream != nil)
			{
				if (MCMiniZipExtractItem(s_payload_minizip, t_context . name, extract_item, &t_context))
					ctxt . SetTheResultToEmpty();
				else
					ctxt . SetTheResultToCString("extract failed");
			}
			else
				ctxt . SetTheResultToCString("could not open file");
			
			if (t_context . stream != nil)
				MCS_close(t_context . stream);
		}
		else
			ctxt . SetTheResultToCString("payload not open");
	}

private:
	struct ExtractContext
	{
		MCObjectHandle target;
		MCStringRef name;
		IO_handle stream;
		MCContainer var;
	};

	static bool extract_item(void *p_context, const void *p_data, uint32_t p_data_length, uint32_t p_data_offset, uint32_t p_data_total)
	{
		ExtractContext *context;
		context = (ExtractContext *)p_context;

		if (context -> stream != nil)
		{
			if (MCS_write(p_data, p_data_length, 1, context -> stream) != IO_NORMAL)
				return false;
		}
		else
		{
			MCExecContext ctxt;
			MCAutoStringRef t_data;
			/* UNCHECKED */ MCStringCreateWithBytes((const byte_t *)p_data, p_data_length, kMCStringEncodingNative, false, &t_data);
            context->var.set(ctxt, *t_data, kMCVariableSetAfter);
		}
        
		if (context->target.IsValid())
		{
			MCParameter p1, p2, p3;
			p1 . setnext(&p2);
			p2 . setnext(&p3);
			p1 . setvalueref_argument(context -> name);
			p2 . setn_argument(p_data_offset + p_data_length);
			p3 . setn_argument(p_data_total);

			context->target->message(MCNAME("payloadProgress"), &p1);
		}

		return true;
	}

	MCExpression *m_item_expr;
	MCExpression *m_file_expr;
};

class MCInternalPayloadPatch: public MCStatement
{
public:
	MCInternalPayloadPatch(void)
	{
		m_patch_item_expr = nil;
		m_base_item_expr = nil;
		m_output_file_expr = nil;
	}

	~MCInternalPayloadPatch(void)
	{
		delete m_patch_item_expr;
		delete m_base_item_expr;
		delete m_output_file_expr;
	}

	Parse_stat parse(MCScriptPoint& sp)
	{
		initpoint(sp);

		if (sp . parseexp(False, True, &m_patch_item_expr) != PS_NORMAL)
			return PS_ERROR;
		if (sp . skip_token(SP_FACTOR, TT_TO, PT_TO) != PS_NORMAL)
			return PS_ERROR;
		if (sp . parseexp(False, True, &m_base_item_expr) != PS_NORMAL)
			return PS_ERROR;
		if (sp . skip_token(SP_FACTOR, TT_PREP, PT_INTO) != PS_NORMAL)
			return PS_ERROR;
		if (sp . parseexp(False, True, &m_output_file_expr) != PS_NORMAL)
			return PS_ERROR;

		return PS_NORMAL;
	}

	void exec_ctxt(MCExecContext& ctxt)
	{
		bool t_success;
		t_success = true;

		MCAutoStringRef t_patch_item;
        if (t_success && ctxt . EvalExprAsStringRef(m_patch_item_expr, EE_INTERNAL_PATCH_BADITEM, &t_patch_item))
			t_success = true;
		else
			t_success = false;

		MCAutoStringRef t_base_item;

		if (t_success && ctxt . EvalExprAsStringRef(m_base_item_expr, EE_INTERNAL_BASE_BADITEM, &t_base_item))
			t_success = true;
		else
			t_success = false;

		MCAutoStringRef t_output_filename;
		if (t_success && ctxt . EvalExprAsStringRef(m_output_file_expr, EE_OUTPUT_BADFILENAME, &t_output_filename))
			t_success = true;
		else
			t_success = false;

		if (s_payload_minizip != nil)
		{
			void *t_patch_data;
			uint32_t t_patch_data_size;
			t_patch_data = nil;
			if (t_success)
				t_success = MCMiniZipExtractItemToMemory(s_payload_minizip, *t_patch_item, t_patch_data, t_patch_data_size);

			void *t_base_data;
			uint32_t t_base_data_size;
			t_base_data = nil;
			if (t_success)
				t_success = MCMiniZipExtractItemToMemory(s_payload_minizip, *t_base_item, t_base_data, t_base_data_size);

			IO_handle t_output_handle;
			t_output_handle = nil;
			if (t_success)
			{
				t_output_handle = MCS_open(*t_output_filename, kMCOpenFileModeWrite, False, False, 0);
				if (t_output_handle == nil)
					t_success = false;
			}

			if (t_success)
			{
				InputStream t_patch_stream, t_base_stream;
				OutputStream t_output_stream;
				t_patch_stream . data = t_patch_data;
				t_patch_stream . data_size = t_patch_data_size;
				t_patch_stream . data_ptr = 0;
				t_base_stream . data = t_base_data;
				t_base_stream . data_size = t_base_data_size;
				t_base_stream . data_ptr = 0;
				t_output_stream . handle = t_output_handle;
				t_success = MCBsDiffApply(&t_patch_stream, &t_base_stream, &t_output_stream);
			}

			if (!t_success)
				ctxt . SetTheResultToCString("patch failed");

			if (t_output_handle != nil)
				MCS_close(t_output_handle);

			MCMemoryDeallocate(t_base_data);
			MCMemoryDeallocate(t_patch_data);
		}
		else
			ctxt . SetTheResultToCString("payload not open");

		return;
	}

private:
	struct InputStream: public MCBsDiffInputStream
	{
		void *data;
		uint32_t data_size;
		uint32_t data_ptr;

		bool Measure(uint32_t& r_size)
		{
			r_size = data_size;
			return true;
		}

		bool ReadInt32(int32_t& r_value)
		{
			if (data_size - data_ptr < 4)
				return false;

			MCMemoryCopy(&r_value, (char *)data + data_ptr, 4);
			r_value = (int32_t)MCSwapInt32NetworkToHost(r_value);

			data_ptr += 4;

			return true;
		}

		bool ReadBytes(void *p_buffer, uint32_t p_count)
		{
			if (data_size - data_ptr < p_count)
				return false;

			MCMemoryCopy(p_buffer, (char *)data + data_ptr, p_count);
			data_ptr += p_count;

			return true;
		}
	};

	struct OutputStream: public MCBsDiffOutputStream
	{
		IO_handle handle;

		bool Rewind(void)
		{
			return MCS_seek_set(handle, 0) == IO_NORMAL;
		}

		bool WriteBytes(const void *buffer, uint32_t count)
		{
			return IO_write(buffer, 1, count, handle) == IO_NORMAL;
		}

		bool WriteInt32(int32_t value)
		{
			return IO_write_int4(value, handle) == IO_NORMAL;
		}
	};

	MCExpression *m_patch_item_expr;
	MCExpression *m_base_item_expr;
	MCExpression *m_output_file_expr;
};

////////////////////////////////////////////////////////////////////////////////

typedef bool (*MCSystemListProcessesCallback)(void *context, uint32_t id, MCStringRef path, MCStringRef description);
typedef bool (*MCSystemListProcessModulesCallback)(void *context, MCStringRef path);

bool MCSystemListProcesses(MCSystemListProcessesCallback p_callback, void* p_context);
bool MCSystemListProcessModules(uint32_t p_process_id, MCSystemListProcessModulesCallback p_callback, void *p_context);

class MCInternalListTasksWithModule: public MCStatement
{
public:
	MCInternalListTasksWithModule(void)
	{
		m_module = nil;
	}

	~MCInternalListTasksWithModule(void)
	{
		delete m_module;
	}

	Parse_stat parse(MCScriptPoint& sp)
	{
		initpoint(sp);

		if (sp . parseexp(False, True, &m_module) != PS_NORMAL)
			return PS_ERROR;

		return PS_NORMAL;
	}
    
    
    void exec_ctxt(MCExecContext& ctxt)
	{
        MCAutoStringRef t_module_str;
        if (!ctxt . EvalExprAsStringRef(m_module, EE_INTERNAL_TASKS_BADMODULE, &t_module_str))
            return;
        
		State t_state;
		t_state . module = *t_module_str;
		/* UNCHECKED */ MCListCreateMutable('\n', t_state.list);
    
		MCSystemListProcesses(ListProcessCallback, &t_state);
        
		ctxt . SetTheResultToValue(t_state.list);
	}
    
private:
	MCExpression *m_module;

	struct State
	{
		MCStringRef module;
		MCListRef list;
		bool found;
	};

	static bool ListProcessModulesCallback(void *p_state, MCStringRef p_module)
	{
		State *state;
		state = (State *)p_state;
        state -> found = MCStringIsEqualTo(state -> module, p_module, kMCStringOptionCompareCaseless);
		return !state -> found;
	}

	static bool ListProcessCallback(void *p_state, uint32_t p_id, MCStringRef p_path, MCStringRef p_desc)
	{
		State *state;
		state = (State *)p_state;
		state -> found = false;
		MCSystemListProcessModules(p_id, ListProcessModulesCallback, state);
		if (state -> found)
		{
            MCListAppendFormat(state ->list, "%@,%@", p_path, p_desc);
        }

		return true;
	}
};

////////////////////////////////////////////////////////////////////////////////

bool MCSystemCanDeleteKey(MCStringRef key);
bool MCSystemCanDeleteFile(MCStringRef file);

class MCInternalCanDeleteKey: public MCStatement
{
public:
	MCInternalCanDeleteKey(void)
	{
		m_key = nil;
	}

	~MCInternalCanDeleteKey(void)
	{
		delete m_key;
	}

	Parse_stat parse(MCScriptPoint& sp)
	{
		initpoint(sp);

		if (sp . parseexp(False, True, &m_key) != PS_NORMAL)
			return PS_ERROR;

		return PS_NORMAL;
	}

	void exec_ctxt(MCExecContext& ctxt)
	{
        MCAutoStringRef t_string;
        if (!ctxt . EvalExprAsStringRef(m_key, EE_INTERNAL_DELETE_BADKEY, &t_string))
            return;
        
        MCresult -> setvalueref(MCSystemCanDeleteKey(*t_string) == true ? kMCTrue : kMCFalse);

		return;
	}

private:
	MCExpression *m_key;
};

class MCInternalCanDeleteFile: public MCStatement
{
public:
	MCInternalCanDeleteFile(void)
	{
		m_file = nil;
	}

	~MCInternalCanDeleteFile(void)
	{
		delete m_file;
	}

	Parse_stat parse(MCScriptPoint& sp)
	{
		initpoint(sp);

		if (sp . parseexp(False, True, &m_file) != PS_NORMAL)
			return PS_ERROR;

		return PS_NORMAL;
	}

    
    void exec_ctxt(MCExecContext& ctxt)
	{
        MCAutoStringRef t_string;
        if (!ctxt . EvalExprAsStringRef(m_file, EE_INTERNAL_DELETE_BADFILENAME, &t_string))
            return;

        MCresult -> setvalueref(MCSystemCanDeleteFile(*t_string) == true ? kMCTrue : kMCFalse);
        
		return;
	}

private:
	MCExpression *m_file;
};

////////////////////////////////////////////////////////////////////////////////

// MM-2011-03-16: Added internal verb bounce.  On OS X this makes the doc icon bounce.  Not implemented on Windows or Linux.
void MCSystemRequestUserAttention(void);
void MCSystemCancelRequestUserAttention(void);

class MCInternalRequestUserAttention: public MCStatement
{
public:
	Parse_stat parse(MCScriptPoint& sp)
	{
		return PS_NORMAL;
	}
	
	void exec_ctxt(MCExecContext& ctxt)
	{
		MCSystemRequestUserAttention();
		return;
	}	
};

class MCInternalCancelRequestUserAttention: public MCStatement
{
public:	
	Parse_stat parse(MCScriptPoint& sp)
	{
		return PS_NORMAL;
	}
	
	void exec_ctxt(MCExecContext& ctxt)
	{
		MCSystemCancelRequestUserAttention();
		return;
	}	
};

// MM-2011-03-24: Added internal verb showBalloon. Takes two params, a title and a message.
// On Windows, this displays a balloon above the taskbar icon (if there is one). Not yet implemented on OS X and Linux.
void MCSystemBalloonNotification(MCStringRef , MCStringRef);

class MCInternalBalloonNotification: public MCStatement
{
public:
	MCInternalBalloonNotification(void)
	{
		m_title = nil;
		m_message = nil;
	}

	~MCInternalBalloonNotification(void)
	{
		delete m_title;
		delete m_message;
	}

	Parse_stat parse(MCScriptPoint& sp)
	{
		initpoint(sp);
		if (sp . parseexp(False, True, &m_title) != PS_NORMAL)
			return PS_ERROR;
		if (sp . parseexp(False, True, &m_message) != PS_NORMAL)
			return PS_ERROR;
		return PS_NORMAL;
	}
	
	void exec_ctxt(MCExecContext& ctxt)
	{
        MCAutoStringRef t_title;
        if (!ctxt . EvalOptionalExprAsNullableStringRef(m_title, EE_UNDEFINED, &t_title))
            return;
    
        MCAutoStringRef t_message;
        if (!ctxt . EvalOptionalExprAsNullableStringRef(m_message, EE_UNDEFINED, &t_message))
            return;
		
		MCSystemBalloonNotification(*t_title, *t_message);
		return ;
	}

private:
	MCExpression *m_title;
	MCExpression *m_message;
};

////////////////////////////////////////////////////////////////////////////////

// Small helper template
template<class T> inline MCStatement *class_factory(void) { return new T; }

// The internal verb table used by the '_internal' command
MCInternalVerbInfo MCinternalverbs[] =
{
	{ "payload", "open", class_factory<MCInternalPayloadOpen> },
	{ "payload", "close", class_factory<MCInternalPayloadClose> },
	{ "payload", "list", class_factory<MCInternalPayloadList> },
	{ "payload", "describe", class_factory<MCInternalPayloadDescribe> },
	{ "payload", "extract", class_factory<MCInternalPayloadExtract> },
	{ "payload", "patch", class_factory<MCInternalPayloadPatch> },
	{ "listTasksWithModule", nil, class_factory<MCInternalListTasksWithModule> },
	{ "canDeleteKey", nil, class_factory<MCInternalCanDeleteKey> },
	{ "canDeleteFile", nil, class_factory<MCInternalCanDeleteFile> },
	{ "bounce", nil, class_factory<MCInternalRequestUserAttention> },
	{ "bounceCancel", nil, class_factory<MCInternalCancelRequestUserAttention> },
	{ "showBalloon", nil, class_factory<MCInternalBalloonNotification> },
	{ nil, nil, nil }
};

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of MCDispatch::startup method for INSTALLER mode.
//

extern IO_stat readheader(IO_handle& stream, char *version);
extern void send_startup_message(bool p_do_relaunch = true);

// This structure contains the information we collect from reading in the
// project.
struct MCStandaloneCapsuleInfo
{
	uint32_t origin;
	MCStack *stack;
	bool done;
};

bool MCStandaloneCapsuleCallback(void *p_self, const uint8_t *p_digest, MCCapsuleSectionType p_type, uint32_t p_length, IO_handle p_stream)
{
	MCStandaloneCapsuleInfo *self;
	self = static_cast<MCStandaloneCapsuleInfo *>(p_self);

	// If we've already seen the epilogue, we are done.
	if (self -> done)
	{
		MCresult -> sets("unexpected data encountered");
		return false;
	}

	switch(p_type)
	{
	case kMCCapsuleSectionTypeEpilogue:
		self -> done = true;
		break;

	case kMCCapsuleSectionTypePrologue:
	{
		MCCapsulePrologueSection t_prologue;
		if (IO_read(&t_prologue, sizeof(t_prologue), p_stream) != IO_NORMAL)
		{
			MCresult -> sets("failed to read project prologue");
			return false;
		}
	}
	break;

	case kMCCapsuleSectionTypeMainStack:
		if (MCdispatcher -> readstartupstack(p_stream, self -> stack) != IO_NORMAL)
		{
			MCresult -> sets("failed to read project stack");
			return false;
		}
            
        // MW-2012-10-25: [[ Bug ]] Make sure we set these to the main stack so that
        //   the startup script and such work.
        MCstaticdefaultstackptr = MCdefaultstackptr = self -> stack;
	break;
            
    case kMCCapsuleSectionTypeScriptOnlyMainStack:
        if (MCdispatcher -> readscriptonlystartupstack(p_stream, p_length, self -> stack) != IO_NORMAL)
        {
            MCresult -> sets("failed to read project stack");
            return false;
        }
        
        // MW-2012-10-25: [[ Bug ]] Make sure we set these to the main stack so that
        //   the startup script and such work.
        MCstaticdefaultstackptr = MCdefaultstackptr = self -> stack;
        break;

	case kMCCapsuleSectionTypeDigest:
		uint8_t t_read_digest[16];
		if (IO_read(t_read_digest, 16, p_stream) != IO_NORMAL)
		{
			MCresult -> sets("failed to read project checksum");
			return false;
		}
		if (memcmp(t_read_digest, p_digest, 16) != 0)
		{
			MCresult -> sets("project checksum mismatch");
			return false;
		}
		break;
            
    case kMCCapsuleSectionTypeStartupScript:
    {
        char *t_script;
        t_script = new (nothrow) char[p_length];
        if (IO_read(t_script, p_length, p_stream) != IO_NORMAL)
        {
            MCresult -> sets("failed to read startup script");
            return false;
        }
            
        // Execute the startup script at this point since we have loaded
        // all stacks.
        MCAutoStringRef t_script_str;
        /* UNCHECKED */ MCStringCreateWithCString(t_script, &t_script_str);
        self -> stack -> domess(*t_script_str);

        delete[] t_script;
        }
        break;
			
    case kMCCapsuleSectionTypeAuxiliaryStack:
    {
        MCStack *t_aux_stack;
        const char *t_result;
        if (MCdispatcher -> trytoreadbinarystack(kMCEmptyString,
                                                 kMCEmptyString,
                                                 p_stream, nullptr,
                                                 t_aux_stack, t_result) != IO_NORMAL)
        {
            MCresult -> sets("failed to read auxillary stack");
            return false;
        }
        MCdispatcher -> processstack(kMCEmptyString, t_aux_stack);
    }
        break;
            
    case kMCCapsuleSectionTypeScriptOnlyAuxiliaryStack:
    {
        MCStack *t_aux_stack;
        const char *t_result;
        if (MCdispatcher -> trytoreadscriptonlystackofsize(kMCEmptyString,
                                                           p_stream,
                                                           p_length,
                                                           nullptr,
                                                           t_aux_stack,
                                                           t_result)
            != IO_NORMAL)
        {
            MCresult -> sets("failed to read auxillary stack");
            return false;
        }
        MCdispatcher -> processstack(kMCEmptyString, t_aux_stack);
    }
        break;
			
	case kMCCapsuleSectionTypeLicense:
	{
		// Just read the license info and ignore it in installer mode.
		uint8_t t_class;
        MCAutoValueRef t_addons;
        if (IO_read(&t_class, 1, p_stream) != IO_NORMAL ||
            (p_length > 1 && IO_read_valueref_new(&t_addons, p_stream) != IO_NORMAL))
		{
			MCresult -> sets("failed to read license");
			return false;
		}
	}
		break;
			
	default:
		MCresult -> sets("unrecognized section encountered");
		return false;
	}
	
	return true;
}

IO_stat MCDispatch::startup(void)
{
    char *t_mccmd;
    /* UNCHECKED */ MCStringConvertToCString(MCcmd, t_mccmd);
	startdir = MCS_getcurdir();
	enginedir = t_mccmd;
	char *eptr = strrchr(enginedir, PATH_SEPARATOR);
	if (eptr != NULL)
		*eptr = '\0';
	else
		*enginedir = '\0';
	
	// set up image cache before the first stack is opened
	MCCachedImageRep::init();
	
	// This little snippet of code allows an easy way to attach VS to a standalone
	// instance to debug startup.
#ifdef _DEBUG
#ifdef _WINDOWS
	if (getenv("TEST_BREAK") != nil)
	{
		while(!IsDebuggerPresent())
			Sleep(50);
		Sleep(250);
		DebugBreak();
	}
#endif
#endif

	// Lookup the name of the project section - if this can't be found there's
	// something up with the exe.
	MCCapsuleInfo *t_project_info;
	t_project_info = (MCCapsuleInfo *)MCExecutableFindSection(PROJECT_SECTION_NAME);
	if (t_project_info == nil || t_project_info -> size <= sizeof(MCCapsuleInfo))
	{
#if DEBUG_INSTALLER_STARTUP
        char *openpath = t_mccmd; //point to MCcmd string
        MCStack *t_stack;
		IO_handle t_stream;
		t_stream = MCS_open(getenv("TEST_STACK"), IO_READ_MODE, False, False, 0);
		if (MCdispatcher -> readstartupstack(t_stream, t_stack) != IO_NORMAL)
		{
			MCresult -> sets("failed to read installer stack");
			return IO_ERROR;
		}
		MCS_close(t_stream);
		
		/* UNCHECKED */ MCStringCreateWithCString(openpath, MCcmd);
		MCdefaultstackptr = MCstaticdefaultstackptr = t_stack;
		
		t_stack -> extraopen(false);
		
        MCdispatcher->resolveparentscripts();
        
		MCscreen->resetcursors();
		MCImage::init();
		send_startup_message();
		if (!MCquit)
			t_stack -> open();

		return IO_NORMAL;
#else
		MCresult -> sets("installer corrupted");
		return IO_ERROR;
#endif
	}
	
	// The info structure that will be filled in while parsing the capsule.
	MCStandaloneCapsuleInfo t_info;
	memset(&t_info, 0, sizeof(MCStandaloneCapsuleInfo));

	// Create a capsule and fill with the standalone data
	MCCapsuleRef t_capsule;
	t_capsule = nil;
	if (!MCCapsuleOpen(MCStandaloneCapsuleCallback, &t_info, t_capsule))
		return IO_ERROR;

	// Decide where to fill from, depending on whether the project data was
	// spilled or not.
	if (((t_project_info -> size) & (1U << 31)) == 0)
	{
		// Capsule is not spilled - just use the project section.
		if (!MCCapsuleFillNoCopy(t_capsule, (const void *)&t_project_info -> data, t_project_info -> size - 4, true))
		{
			MCCapsuleClose(t_capsule);
			return IO_ERROR;
		}
	}
    else
    {
        // Capsule is spilled fill from:
        //   0..2044 from project section
        //   spill file
        //   rest from project section
        MCAutoStringRef t_spill;
        /* UNCHECKED */ MCStringFormat(&t_spill, "%@.dat", MCcmd);
        if (!MCCapsuleFillFromFile(t_capsule, *t_spill, 0, true))
        {
            MCCapsuleClose(t_capsule);
            return IO_ERROR;
        }
    }

	// Process the capsule
	if (!MCCapsuleProcess(t_capsule))
	{
		MCCapsuleClose(t_capsule);
		return IO_ERROR;
	}

	MCdefaultstackptr = MCstaticdefaultstackptr = t_info . stack;
	MCCapsuleClose(t_capsule);

	t_info . stack -> extraopen(false);
    
    MCdispatcher->resolveparentscripts();
    
	MCscreen->resetcursors();
	MCtemplateimage->init();
	send_startup_message();
	if (!MCquit)
		t_info . stack -> open();

	return IO_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of MCStack::mode* hooks for INSTALLER mode.
//

void MCStack::mode_load(void)
{
}

void MCStack::mode_getrealrect(MCRectangle& r_rect)
{
	MCscreen->getwindowgeometry(window, r_rect);
}

void MCStack::mode_takewindow(MCStack *other)
{
}

void MCStack::mode_takefocus(void)
{
	MCscreen->setinputfocus(window);
}

bool MCStack::mode_needstoopen(void)
{
	return true;
}

void MCStack::mode_setgeom(void)
{
}

void MCStack::mode_setcursor(void)
{
	MCscreen->setcursor(window, cursor);
}

bool MCStack::mode_openasdialog(void)
{
	return true;
}

void MCStack::mode_closeasdialog(void)
{
}

void MCStack::mode_openasmenu(MCStack *grab)
{
}

void MCStack::mode_closeasmenu(void)
{
}

void MCStack::mode_constrain(MCRectangle& rect)
{
}

#ifdef _WINDOWS
MCSysWindowHandle MCStack::getrealwindow(void)
{
	return window->handle.window;
}

MCSysWindowHandle MCStack::getqtwindow(void)
{
	return window->handle.window;
}

#endif


////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of mode hooks for INSTALLER mode.
//

// In standalone mode, the standalone stack built into the engine cannot
// be saved.
IO_stat MCModeCheckSaveStack(MCStack *sptr, const MCStringRef filename)
{
	if (sptr == MCdispatcher -> getstacks())
	{
		MCresult->sets("can't save into installer");
		return IO_ERROR;
	}

	return IO_NORMAL;
}

// In installer mode, the environment depends on the command line args
MCNameRef MCModeGetEnvironment(void)
{
    if (MCnoui)
        return MCN_installer_cmdline;
    
	return MCN_installer;
}

uint32_t MCModeGetEnvironmentType(void)
{
	return kMCModeEnvironmentTypeInstaller;
}

// SN-2015-01-16: [[ Bug 14295 ]] Installer-mode is standalone
void MCModeGetResourcesFolder(MCStringRef &r_resources_folder)
{
    MCS_getresourcesfolder(true, r_resources_folder);
}

// In standalone mode, we are never licensed.
bool MCModeGetLicensed(void)
{
	return false;
}

// In standalone mode, the executable is $0 if there is an embedded stack.
bool MCModeIsExecutableFirstArgument(void)
{
	return true;
}

// In installer mode, we have command line name / arguments
bool MCModeHasCommandLineArguments(void)
{
    return true;
}

// In installer mode, we have environment variables
bool
MCModeHasEnvironmentVariables()
{
	return true;
}

// In standalone mode, we only automatically open stacks if there isn't an
// embedded stack.
bool MCModeShouldLoadStacksOnStartup(void)
{
	return false;
}

// In standalone mode, we try to work out what went wrong...
void MCModeGetStartupErrorMessage(MCStringRef& r_caption, MCStringRef& r_text)
{
	r_caption = MCSTR("Initialization Error");
	if (MCValueGetTypeCode(MCresult -> getvalueref()) == kMCValueTypeCodeString)
		r_text = MCValueRetain((MCStringRef)MCresult -> getvalueref());
	else
		r_text = MCSTR("unknown reason");
}

// In standalone mode, we can only set an object's script if has non-zero id.
bool MCModeCanSetObjectScript(uint4 obj_id)
{
	return obj_id != 0;
}

// In standalone mode, we must check the old CANT_STANDALONE flag.
bool MCModeShouldCheckCantStandalone(void)
{
	return true;
}

// The standalone mode causes a relaunch message.
bool MCModeHandleRelaunch(MCStringRef &r_id)
{
#ifdef _WINDOWS
	bool t_do_relaunch;
    t_do_relaunch = MCdefaultstackptr -> handlesmessage(MCM_relaunch) == True;
    /* UNCHECKED */ MCStringCopy(MCNameGetString(MCdefaultstackptr -> getname()), r_id);
    return t_do_relaunch;
#else
	return false;
#endif
}

// The standalone mode's startup stack depends on whether it has a stack embedded.
const char *MCModeGetStartupStack(void)
{
	return NULL;
}

bool MCModeCanLoadHome(void)
{
	return false;
}

MCStatement *MCModeNewCommand(int2 which)
{
	return NULL;
}

MCExpression *MCModeNewFunction(int2 which)
{
	return NULL;
}

MCObject *MCModeGetU3MessageTarget(void)
{
	return MCdefaultstackptr -> getcard();
}

bool MCModeShouldQueueOpeningStacks(void)
{
	return MCscreen == NULL;
}

bool MCModeShouldPreprocessOpeningStacks(void)
{
	return false;
}

Window MCModeGetParentWindow(void)
{
	Window t_window;
	t_window = MCdefaultstackptr -> getwindow();
	if (t_window == NULL && MCtopstackptr)
		t_window = MCtopstackptr -> getwindow();
	return t_window;
}

bool MCModeCanAccessDomain(MCStringRef p_name)
{
	return false;
}

void MCModeQueueEvents(void)
{
}

Exec_stat MCModeExecuteScriptInBrowser(MCStringRef p_script)
{
	MCeerror -> add(EE_ENVDO_NOTSUPPORTED, 0, 0);
	return ES_ERROR;
}

bool MCModeMakeLocalWindows(void)
{
	return true;
}

void MCModeActivateIme(MCStack *p_stack, bool p_activate)
{
	MCscreen -> activateIME(p_activate);
}

void MCModeConfigureIme(MCStack *p_stack, bool p_enabled, int32_t x, int32_t y)
{
	if (!p_enabled)
		MCscreen -> clearIME(p_stack -> getwindow());
    else
        MCscreen -> configureIME(x, y);
}

void MCModeShowToolTip(int32_t x, int32_t y, uint32_t text_size, uint32_t bg_color, MCStringRef text_font, MCStringRef message)
{
}

void MCModeHideToolTip(void)
{
}

void MCModeResetCursors(void)
{
	MCscreen -> resetcursors();
}

bool MCModeCollectEntropy(void)
{
	return false;
}

// We return false here as at present, in installers, the first stack does not
// sit in the message path of all stacks.
bool MCModeHasHomeStack(void)
{
	return false;
}

// Pixel scaling can be enabled in the installer
bool MCModeCanEnablePixelScaling()
{
	return true;
}

// IM-2014-08-08: [[ Bug 12372 ]] Pixel scaling is enabled for the installer
bool MCModeGetPixelScalingEnabled(void)
{
	return true;
}

void MCModeFinalize(void)
{
    
}

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of remote dialog methods
//

void MCRemoteFileDialog(MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint32_t p_type_count, MCStringRef p_initial_folder, MCStringRef p_initial_file, bool p_save, bool p_files, MCStringRef &r_value)
{
}

void MCRemoteColorDialog(MCStringRef p_title, uint32_t p_red, uint32_t p_green, uint32_t p_blue, bool& r_chosen, MCColor& r_chosen_color)
{
}

void MCRemoteFolderDialog(MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial, MCStringRef &r_value)
{
}

void MCRemotePrintSetupDialog(MCDataRef p_config_data, MCDataRef &r_reply_data, uint32_t &r_result)
{
}

void MCRemotePageSetupDialog(MCDataRef p_config_data, MCDataRef &r_reply_data, uint32_t &r_result)
{
}

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of Windows-specific mode hooks for INSTALLER mode.
//

#ifdef TARGET_PLATFORM_WINDOWS

typedef BOOL (WINAPI *AttachConsolePtr)(DWORD id);
void MCModePreMain(void)
{
	HMODULE t_kernel;
	t_kernel = LoadLibraryA("kernel32.dll");
	if (t_kernel != nil)
	{
		void *t_attach_console;
		t_attach_console = GetProcAddress(t_kernel, "AttachConsole");
		if (t_attach_console != nil)
		{
			((AttachConsolePtr)t_attach_console)(-1);
			return;
		}
	}
}

void MCModeSetupCrashReporting(void)
{
}

bool MCModeHandleMessage(LPARAM lparam)
{
	return false;
}

#endif

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of Mac OS X-specific mode hooks for INSTALLER mode.
//

#ifdef _MACOSX

bool MCModePreWaitNextEvent(Boolean anyevent)
{
	return false;
}

#endif

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of Linux-specific mode hooks for INSTALLER mode.
//

#ifdef _LINUX

void MCModePreSelectHook(int& maxfd, fd_set& rfds, fd_set& wfds, fd_set& efds)
{
}

void MCModePostSelectHook(fd_set& rfds, fd_set& wfds, fd_set& efds)
{
}

#endif

////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of the per-platform section location functions
//

#if defined(_WINDOWS)
static void *MCExecutableFindSection(const char *p_name)
{
	// The 'module handle' is nothing more than the base address of the PE image
	void *t_base;
	t_base = GetModuleHandle(NULL);

	// PE images start with a DOS_HEADER
	IMAGE_DOS_HEADER *t_dos_header;
	t_dos_header = (IMAGE_DOS_HEADER *)t_base;

	// The main (NT) header is located based on an offset in the DOS header
	IMAGE_NT_HEADERS *t_nt_headers;
	t_nt_headers = (IMAGE_NT_HEADERS *)((char *)t_base + t_dos_header -> e_lfanew);

	// The list of sections is after the 'OPTIONAL_HEADER', this macro computes
	// the address for us.
	IMAGE_SECTION_HEADER *t_sections;
	t_sections = IMAGE_FIRST_SECTION(t_nt_headers);

	// Now just loop through until we find the section we are looking for. Note
	// that the 'VirtualAddress' field is relative to ImageBase - hence the
	// adjustment.
	for(uint32_t i = 0; i < t_nt_headers -> FileHeader . NumberOfSections; i++)
		if (memcmp(t_sections[i] . Name, p_name, MCU_min((unsigned)IMAGE_SIZEOF_SHORT_NAME, strlen(p_name))) == 0)
			return (void *)(t_sections[i] . VirtualAddress + t_nt_headers -> OptionalHeader . ImageBase);

	// We didn't find the section - oh dear.
	return NULL;
}
#elif defined(_MACOSX)
#include <mach-o/dyld.h>

/* We only use the 64 bit engine for the installer so this function will only work on that engine */
static void *MCExecutableFindSection(const char *p_name)
{
	const mach_header_64 *t_header;
	t_header = reinterpret_cast<const struct mach_header_64 *>(_dyld_get_image_header(0));
	
	const load_command *t_command;
	t_command = reinterpret_cast<const load_command *>(t_header + 1);
	
	for(uint32_t i = 0; i < t_header -> ncmds; i++)
	{
		if (t_command -> cmd == LC_SEGMENT_64)
		{
			const segment_command_64 *t_segment;
			t_segment = reinterpret_cast<const segment_command_64 *>(t_command);
			
			if (MCMemoryEqual(t_segment -> segname, p_name, MCMin(16, strlen(p_name) + 1)))
			{
				const section_64 *t_section;
                t_section =  reinterpret_cast<const section_64 *>(t_segment + 1);
				return reinterpret_cast<char *>(t_section -> addr) + _dyld_get_image_vmaddr_slide(0);
			}
		}
		
		t_command = (const load_command *)((uint8_t *)t_command + t_command -> cmdsize);
	}
	
	return NULL;
}
#elif defined(_LINUX)
#include <elf.h>

// MW-2013-05-03: [[ Linux64 ]] Make sure we use the correct structs for section
//   searching.

#ifdef __LP64__
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Shdr Elf_Shdr;
typedef Elf64_Phdr Elf_Phdr;
#else
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Shdr Elf_Shdr;
typedef Elf32_Phdr Elf_Phdr;
#endif

static void *MCExecutableFindSection(const char *p_name)
{
	bool t_success;
	t_success = true;

	// It is not clear whether the section table on Linux is mapped into memory,
	// however this isn't really a problem, we just load it direct from the file.
	FILE *t_exe;
	t_exe = NULL;
	if (t_success)
	{
        MCAutoStringRefAsUTF8String t_mccmd_utf8;
        t_mccmd_utf8 . Lock(MCcmd);
		t_exe = fopen(*t_mccmd_utf8, "rb");
		if (t_exe == NULL)
			t_success = false;
	}

	// Load the header
	Elf_Ehdr t_header;
	if (t_success)
		if (fread(&t_header, sizeof(Elf_Ehdr), 1, t_exe) != 1)
			t_success = false;

	// Allocate memory for the section table
	Elf_Shdr *t_sections;
	t_sections = nil;
	if (t_success)
		t_success = MCMemoryAllocate(sizeof(Elf_Shdr) * t_header . e_shnum, t_sections);

	// Now read in the sections
	for(uint32_t i = 0; t_success && i < t_header . e_shnum; i++)
	{
		if (fseek(t_exe, t_header . e_shoff + i * t_header . e_shentsize, SEEK_SET) != 0 ||
			fread(&t_sections[i], sizeof(Elf_Shdr), 1, t_exe) != 1)
			t_success = false;
	}

	// Next allocate memory for the string table, and read it in
	char *t_strings;
	t_strings = nil;
	if (t_success)
		t_success =
			MCMemoryAllocate(t_sections[t_header . e_shstrndx] . sh_size, t_strings) &&
			fseek(t_exe, t_sections[t_header . e_shstrndx] . sh_offset, SEEK_SET) == 0 &&
			fread(t_strings, t_sections[t_header . e_shstrndx] . sh_size, 1, t_exe) == 1;

	// Now we can search for our section
	void *t_address;
	t_address = NULL;
	for(uint32_t i = 0; t_success && i < t_header . e_shnum; i++)
		if (strcmp(p_name, t_strings + t_sections[i] . sh_name) == 0)
		{
			t_address = (void *)t_sections[i] . sh_addr;
			break;
		}

	// Free up all the resources we allocated
	MCMemoryDeallocate(t_strings);
	MCMemoryDeallocate(t_sections);
	if (t_exe != NULL)
		fclose(t_exe);

	return t_address;
}
#endif

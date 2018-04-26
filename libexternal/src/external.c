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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef _WINDOWS
#include <stdint.h>
#endif

#include <revolution/external.h>

#ifdef _WINDOWS
#define LIBRARY_EXPORT __declspec(dllexport)
#else
#define LIBRARY_EXPORT __attribute__((__visibility__("default")))
#endif

enum
{
	OPERATION_SET_IDLE_RATE__DEPRECATED,
	OPERATION_ABORT__DEPRECATED,
	OPERATION_SET_IDLE_FUNC__DEPRECATED,
	OPERATION_SEND_CARD_MESSAGE,
	OPERATION_SEND_MC_MESSAGE__DEPRECATED,
	OPERATION_EVAL_EXP,
	OPERATION_GET_GLOBAL,
	OPERATION_SET_GLOBAL,
	OPERATION_GET_FIELD_BY_NAME,
	OPERATION_GET_FIELD_BY_NUM,
	OPERATION_GET_FIELD_BY_ID,
	OPERATION_SET_FIELD_BY_NAME,
	OPERATION_SET_FIELD_BY_NUM,
	OPERATION_SET_FIELD_BY_ID,
	OPERATION_SHOW_IMAGE_BY_NAME,
	OPERATION_SHOW_IMAGE_BY_NUM,
	OPERATION_SHOW_IMAGE_BY_ID,
	OPERATION_GET_VARIABLE,
	OPERATION_SET_VARIABLE,
	OPERATION_GET_VARIABLE_EX,
	OPERATION_SET_VARIABLE_EX,
	OPERATION_GET_ARRAY,
	OPERATION_SET_ARRAY,

	// IM-2014-03-06: [[ revBrowserCEF ]] Add externals extensions for V1
	/* V1 */ OPERATION_ADD_RUNLOOP_ACTION,
	/* V1 */ OPERATION_REMOVE_RUNLOOP_ACTION,
	/* V1 */ OPERATION_RUNLOOP_WAIT,
    
    // IM-2014-07-09: [[ Bug 12225 ]] Add coordinate conversion functions
	/* V1 */ OPERATION_STACK_TO_WINDOW_RECT,
	/* V1 */ OPERATION_WINDOW_TO_STACK_RECT,
    
    // SN-2014-07-04: [[ UnicodeExternalsV0 ]] Add externals extensions to allow utf8-encoded arguments    
	/* V2 */ OPERATION_SEND_CARD_MESSAGE_UTF8,
	/* V2 */ OPERATION_EVAL_EXP_UTF8,
	/* V2 */ OPERATION_GET_GLOBAL_UTF8,
	/* V2 */ OPERATION_SET_GLOBAL_UTF8,
	/* V2 */ OPERATION_GET_FIELD_BY_NAME_UTF8,
	/* V2 */ OPERATION_GET_FIELD_BY_NUM_UTF8,
	/* V2 */ OPERATION_GET_FIELD_BY_ID_UTF8,
	/* V2 */ OPERATION_SET_FIELD_BY_NAME_UTF8,
	/* V2 */ OPERATION_SET_FIELD_BY_NUM_UTF8,
	/* V2 */ OPERATION_SET_FIELD_BY_ID_UTF8,
	/* V2 */ OPERATION_SHOW_IMAGE_BY_NAME_UTF8,
	/* V2 */ OPERATION_SHOW_IMAGE_BY_NUM_UTF8,
	/* V2 */ OPERATION_SHOW_IMAGE_BY_ID_UTF8,
	/* V2 */ OPERATION_GET_VARIABLE_UTF8,
	/* V2 */ OPERATION_SET_VARIABLE_UTF8,
	/* V2 */ OPERATION_GET_VARIABLE_EX_UTF8_TEXT,
	/* V2 */ OPERATION_GET_VARIABLE_EX_UTF8_BINARY,
	/* V2 */ OPERATION_SET_VARIABLE_EX_UTF8_TEXT,
	/* V2 */ OPERATION_SET_VARIABLE_EX_UTF8_BINARY,
	/* V2 */ OPERATION_GET_ARRAY_UTF8_TEXT,
	/* V2 */ OPERATION_GET_ARRAY_UTF8_BINARY,
    /* V2 */ OPERATION_SET_ARRAY_UTF8_TEXT,
    /* V2 */ OPERATION_SET_ARRAY_UTF8_BINARY,
    
    // AL-2015-02-06: [[ SB Inclusions ]] Add new callbacks for resource loading.
    /* V3 */ OPERATION_LOAD_MODULE,
    /* V3 */ OPERATION_UNLOAD_MODULE,
    /* V3 */ OPERATION_RESOLVE_SYMBOL_IN_MODULE,

    // SN-2015-02-25: [[ Merge 6.7.4-rc-1 ]] OPERATION_GET_XDISPLAY_HANDLE is
    //  actually from the version 4 (V3 is Ali's module functions).
    /* V3 */ OPERATION_GET_XDISPLAY_HANDLE,
    // SN-2015-03-12: [[ Bug 14413 ]] Add new UTF-8 <-> native conversion functions
    /* V4 */ OPERATION_CONVERT_FROM_NATIVE_TO_UTF8,
    /* V4 */ OPERATION_CONVERT_TO_NATIVE_FROM_UTF8,

    /* V5 */ OPERATION_COPY_NATIVE_PATH_OF_MODULE,
};

enum
{
	SECURITY_CHECK_FILE,
	SECURITY_CHECK_HOST,
	SECURITY_CHECK_LIBRARY,
    
    // SN-2014-07-04: [[ UnicodeExternalsV0 ]] Add security checks with unicode parameters
	/* V2 */ SECURITY_CHECK_FILE_UTF8,
	/* V2 */ SECURITY_CHECK_HOST_UTF8,
	/* V2 */ SECURITY_CHECK_LIBRARY_UTF8
};

typedef char *(*ExternalOperationCallback)(const void *p_arg_1, const void *p_arg_2, const void *p_arg_3, int *r_success);
typedef void (*ExternalDeleteCallback)(void *p_block);

typedef Bool (*ExternalSecurityHandler)(const char *p_op);

extern const char *g_external_name;
extern ExternalDeclaration g_external_table[];

// IM-2014-03-06: [[ revBrowserCEF ]] Initialise externals interface version to V0
static unsigned int s_external_interface_version = 0;

static ExternalOperationCallback *s_operations = NULL;
static ExternalDeleteCallback s_delete = NULL;

static ExternalSecurityHandler *s_security_handlers = NULL;

#if !defined(_WIN32)
void getXtable(ExternalOperationCallback p_operations[], ExternalDeleteCallback p_delete, const char **r_name, ExternalDeclaration **r_table, ExternalDeleteCallback *r_external_delete) __attribute__((visibility("default")));
void configureSecurity(ExternalSecurityHandler *p_handlers) __attribute__((visibility("default")));
void setExternalInterfaceVersion(unsigned int p_version) __attribute__((visibility("default")));
#endif

// IM-2014-03-06: [[ revBrowserCEF ]] The engine may call this to notify the external of the availability of extended engine callbacks
void LIBRARY_EXPORT setExternalInterfaceVersion(unsigned int p_version)
{
	s_external_interface_version = p_version;
}

void LIBRARY_EXPORT getXtable(ExternalOperationCallback p_operations[],
															ExternalDeleteCallback p_delete,
															const char **r_name,
															ExternalDeclaration **r_table,
															ExternalDeleteCallback *r_external_delete)
{
	s_operations = p_operations;
	s_delete = p_delete;
	*r_name = g_external_name;
	*r_table = g_external_table;
	*r_external_delete = free;
}

void LIBRARY_EXPORT configureSecurity(ExternalSecurityHandler *p_handlers)
{
	s_security_handlers = p_handlers;
}

static char *retstr(char *p_string)
{
	if (p_string != NULL)
	{
		char *t_result;
		t_result = strdup(p_string);
		s_delete(p_string);
		return t_result;
	}

	return NULL;
}

void SendCardMessage(const char *p_message, int *r_success)
{
	char *t_result;
	t_result = (s_operations[OPERATION_SEND_CARD_MESSAGE])(p_message, NULL, NULL, r_success);
	if (t_result != NULL)
		(s_delete)(t_result);
}


char *EvalExpr(const char *p_expression, int *r_success)
{
	char *t_result;
	t_result = (s_operations[OPERATION_EVAL_EXP])(p_expression, NULL, NULL, r_success);
	return retstr(t_result);
}

char *GetGlobal(const char *p_name, int *r_success)
{
	char *t_result;
	t_result = (s_operations[OPERATION_GET_GLOBAL])(p_name, NULL, NULL, r_success);
	return retstr(t_result);

}

void SetGlobal(const char *p_name, const char *p_value, int *r_success)
{
	char *t_result;
	t_result = (s_operations[OPERATION_SET_GLOBAL])(p_name, p_value, NULL, r_success);
	if (t_result != NULL)
		(s_delete)(t_result);
}

char *GetFieldByName(const char *p_group, const char *p_name, int *r_success)
{
	char *t_result;
	// MDW-2013-05-08 : fix for bug 7913
	t_result = (s_operations[OPERATION_GET_FIELD_BY_NAME])(p_name, p_group, NULL, r_success);
	return retstr(t_result);
}

char *GetFieldByNum(const char *p_group, int p_index, int *r_success)
{
	char t_index_str[16];
	char *t_result;
	
	sprintf(t_index_str, "%d", p_index);
	t_result = (s_operations[OPERATION_GET_FIELD_BY_NUM])(t_index_str, p_group, NULL, r_success);

	return retstr(t_result);
}

char *GetFieldById(const char *p_group, unsigned long p_id, int *r_success)
{
	char t_index_str[16];
	char *t_result;

	sprintf(t_index_str, "%ld", p_id);
	t_result = (s_operations[OPERATION_GET_FIELD_BY_ID])(t_index_str, p_group, NULL, r_success);

	return retstr(t_result);
}

void SetFieldByName(const char *p_group, const char *p_name, const char *p_value, int *r_success)
{
	char *t_result;
	// MDW-2013-05-08 : fix for bug 7913
	t_result = (s_operations[OPERATION_SET_FIELD_BY_NAME])(p_name, p_group, p_value, r_success);
	if (t_result != NULL)
		(s_delete)(t_result);
}

void SetFieldByNum(const char *p_group, int p_index, const char *p_value, int *r_success)
{
	char t_index_str[16];
	char *t_result;

	sprintf(t_index_str, "%d", p_index);
	t_result = (s_operations[OPERATION_SET_FIELD_BY_NUM])(t_index_str, p_group, p_value, r_success);
	if (t_result != NULL)
		s_delete(t_result);
}

void SetFieldById(const char *p_group, unsigned long p_id, const char *p_value, int *r_success)
{
	char t_index_str[16];
	char *t_result;
	sprintf(t_index_str, "%ld", p_id);

	t_result = (s_operations[OPERATION_SET_FIELD_BY_ID])(t_index_str, p_group, p_value, r_success);
	if (t_result != NULL)
		s_delete(t_result);
}

void ShowImageByName(const char *p_group, const char *p_name, int *r_success)
{
	char *t_result;
	// MDW-2013-05-08 : fix for bug 7913
	t_result = (s_operations[OPERATION_SHOW_IMAGE_BY_NAME])(p_name, p_group, NULL, r_success);
	if (t_result != NULL)
		(s_delete)(t_result);
}

void ShowImageByNum(const char *p_group, int p_index, int *r_success)
{
	char t_index_str[16];
	char *t_result;

	sprintf(t_index_str, "%d", p_index);
	t_result = (s_operations[OPERATION_SHOW_IMAGE_BY_NUM])(t_index_str, p_group, NULL, r_success);
	if (t_result != NULL)
		s_delete(t_result);
}

void ShowImageById(const char *p_group, unsigned long p_id, int *r_success)
{
	char t_index_str[16];
	char *t_result;

	sprintf(t_index_str, "%ld", p_id);
	t_result = (s_operations[OPERATION_SHOW_IMAGE_BY_ID])(t_index_str, p_group, NULL, r_success);
	if (t_result != NULL)
		s_delete(t_result);
}

void ShowImageByLongId(const char *p_long_id, int *r_success)
{
	char *t_result;
	t_result = (s_operations[OPERATION_SHOW_IMAGE_BY_ID])(p_long_id, "false", NULL, r_success);
	if (t_result != NULL)
		s_delete(t_result);
}

char *GetVariable(const char *p_name, int *r_success)
{
	char *t_result;
	t_result = (s_operations[OPERATION_GET_VARIABLE])(p_name, NULL, NULL, r_success);
  return retstr(t_result);
}

void SetVariable(const char *p_name, const char *p_value, int *r_success)
{
	char *t_result;
	t_result = (s_operations[OPERATION_SET_VARIABLE])(p_name, p_value, NULL, r_success);
  if (t_result != NULL)
		s_delete(t_result);
}

void GetVariableEx(const char *p_name, const char *p_key, ExternalString *r_value, int *r_success)
{
	char *t_result;
	t_result = (s_operations[OPERATION_GET_VARIABLE_EX])(p_name, p_key, (char *)r_value, r_success);
  if (t_result != NULL)
		s_delete(t_result);
}

void SetVariableEx(const char *p_name, const char *p_key, const ExternalString *p_value, int *r_success)
{
	char *t_result;
	t_result = (s_operations[OPERATION_SET_VARIABLE_EX])(p_name, p_key, (char *)p_value, r_success);
  if (t_result != NULL)
		s_delete(t_result);
}

void GetArray(const char *p_name, int *r_element_count, ExternalString *r_values, char **r_keys, int *r_success)
{
	ExternalArray t_array;
	char *t_result;

	t_array . nelements = *r_element_count;
	t_array . strings = r_values;
	t_array . keys = r_keys;
	t_result = (s_operations[OPERATION_GET_ARRAY])(p_name, NULL, (char *)&t_array, r_success);
	if (t_result != NULL)
		s_delete(t_result);

	if (*r_success == EXTERNAL_SUCCESS)
		*r_element_count = t_array . nelements;
}

void SetArray(const char *p_name, int p_element_count, ExternalString *p_values, char **p_keys, int *r_success)
{
	ExternalArray t_array;
	char *t_result;

	t_array . nelements = p_element_count;
	t_array . strings = p_values;
	t_array . keys = p_keys;
	t_result = (s_operations[OPERATION_SET_ARRAY])(p_name, NULL, (char *)&t_array, r_success);
	if (t_result != NULL)
		s_delete(t_result);
}

Bool SecurityCanAccessFile(const char *p_file)
{
	if (s_security_handlers != NULL)
		return s_security_handlers[SECURITY_CHECK_FILE](p_file);
	return True;
}

Bool SecurityCanAccessHost(const char *p_host)
{
	if (s_security_handlers != NULL)
		return s_security_handlers[SECURITY_CHECK_HOST](p_host);
	return True;
}

Bool SecurityCanAccessLibrary(const char *p_library)
{
	if (s_security_handlers != NULL)
		return s_security_handlers[SECURITY_CHECK_LIBRARY](p_library);
	return True;
}

////////////////////////////////////////////////////////////////////////////////
// IM-2014-03-06: [[ revBrowserCEF ]] Add external interface V1 functions
void AddRunloopAction(MCRunloopActionCallback p_callback, void *p_context, MCRunloopActionRef *r_action, int *r_success)
{
	char *t_result;

	if (s_external_interface_version < 1)
	{
		*r_success = EXTERNAL_FAILURE;
		return;
	}

	t_result = (s_operations[OPERATION_ADD_RUNLOOP_ACTION])(p_callback, p_context, r_action, r_success);
	if (t_result != NULL)
		s_delete(t_result);
}

void RemoveRunloopAction(MCRunloopActionRef p_action, int *r_success)
{
	char *t_result;

	if (s_external_interface_version < 1)
	{
		*r_success = EXTERNAL_FAILURE;
		return;
	}

	t_result = (s_operations[OPERATION_REMOVE_RUNLOOP_ACTION])(p_action, NULL, NULL, r_success);
	if (t_result != NULL)
		s_delete(t_result);
}

void RunloopWait(int *r_success)
{
	char *t_result;

	if (s_external_interface_version < 1)
	{
		*r_success = EXTERNAL_FAILURE;
		return;
	}

	t_result = (s_operations[OPERATION_RUNLOOP_WAIT])(NULL, NULL, NULL, r_success);
	if (t_result != NULL)
		s_delete(t_result);
}

// IM-2014-07-09: [[ Bug 12225 ]] Add coordinate conversion functions
void StackToWindowRect(uintptr_t p_win_id, MCRectangle32 *x_rect, int *r_success)
{
    char *t_result;
    
    if (s_external_interface_version < 1)
	{
		*r_success = EXTERNAL_FAILURE;
		return;
	}
    
    t_result = (s_operations[OPERATION_STACK_TO_WINDOW_RECT])((const void*)p_win_id, x_rect, NULL, r_success);
	if (t_result != NULL)
		s_delete(t_result);
}

void WindowToStackRect(uintptr_t p_win_id, MCRectangle32 *x_rect, int *r_success)
{
	char *t_result;
    
	if (s_external_interface_version < 1)
	{
		*r_success = EXTERNAL_FAILURE;
		return;
	}
    
	t_result = (s_operations[OPERATION_WINDOW_TO_STACK_RECT])((const void*)p_win_id, x_rect, NULL, r_success);
	if (t_result != NULL)
		s_delete(t_result);
}

////////////////////////////////////////////////////////////////////////////////
// 
// SN-2014-07-04: [[ UnicodeExternalsV0 ]] Add externals functions with utf8-encoded parameters
//

void SendCardMessageUTF8(const char *p_message, int *r_success)
{
	char *t_result;
    
    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[OPERATION_SEND_CARD_MESSAGE_UTF8] == NULL)
	{
		*r_success = EXTERNAL_FAILURE;
		return;
	}
    
	t_result = (s_operations[OPERATION_SEND_CARD_MESSAGE_UTF8])(p_message, NULL, NULL, r_success);
	if (t_result != NULL)
		(s_delete)(t_result);
}


char *EvalExprUTF8(const char *p_expression, int *r_success)
{
	char *t_result;

    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[OPERATION_EVAL_EXP_UTF8] == NULL)
	{
		*r_success = EXTERNAL_FAILURE;
		return NULL;
	}
    
	t_result = (s_operations[OPERATION_EVAL_EXP_UTF8])(p_expression, NULL, NULL, r_success);
	return retstr(t_result);
}

char *GetGlobalUTF8(const char *p_name, int *r_success)
{
	char *t_result;

    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[OPERATION_GET_GLOBAL_UTF8] == NULL)
	{
		*r_success = EXTERNAL_FAILURE;
		return NULL;
	}
    
	t_result = (s_operations[OPERATION_GET_GLOBAL_UTF8])(p_name, NULL, NULL, r_success);
	return retstr(t_result);
    
}

void SetGlobalUTF8(const char *p_name, const char *p_value, int *r_success)
{
	char *t_result;

    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[OPERATION_SET_GLOBAL_UTF8] == NULL)
	{
		*r_success = EXTERNAL_FAILURE;
		return;
	}
    
	t_result = (s_operations[OPERATION_SET_GLOBAL_UTF8])(p_name, p_value, NULL, r_success);
	if (t_result != NULL)
		(s_delete)(t_result);
}

char *GetFieldByNameUTF8(const char *p_group, const char *p_name, int *r_success)
{
	char *t_result;

    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[OPERATION_GET_FIELD_BY_NAME_UTF8] == NULL)
	{
		*r_success = EXTERNAL_FAILURE;
		return NULL;
	}
    
	// MDW-2013-05-08 : fix for bug 7913
	t_result = (s_operations[OPERATION_GET_FIELD_BY_NAME_UTF8])(p_name, p_group, NULL, r_success);
	return retstr(t_result);
}

char *GetFieldByNumUTF8(const char *p_group, int p_index, int *r_success)
{
	char t_index_str[16];
	char *t_result;

    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[OPERATION_GET_FIELD_BY_NUM_UTF8] == NULL)
	{
		*r_success = EXTERNAL_FAILURE;
		return NULL;
	}
	
	sprintf(t_index_str, "%d", p_index);
	t_result = (s_operations[OPERATION_GET_FIELD_BY_NUM_UTF8])(t_index_str, p_group, NULL, r_success);
    
	return retstr(t_result);
}

char *GetFieldByIdUTF8(const char *p_group, unsigned long p_id, int *r_success)
{
	char t_index_str[16];
	char *t_result;

    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[OPERATION_GET_FIELD_BY_ID_UTF8] == NULL)
	{
		*r_success = EXTERNAL_FAILURE;
		return NULL;
	}
    
	sprintf(t_index_str, "%ld", p_id);
    t_result = (s_operations[OPERATION_GET_FIELD_BY_ID_UTF8])(t_index_str, p_group, NULL, r_success);
    
	return retstr(t_result);
}

void SetFieldByNameUTF8(const char *p_group, const char *p_name, const char *p_value, int *r_success)
{
	char *t_result;

    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[OPERATION_SET_FIELD_BY_NAME_UTF8] == NULL)
	{
		*r_success = EXTERNAL_FAILURE;
		return;
	}
    
	// MDW-2013-05-08 : fix for bug 7913
	t_result = (s_operations[OPERATION_SET_FIELD_BY_NAME_UTF8])(p_name, p_group, p_value, r_success);
	if (t_result != NULL)
		(s_delete)(t_result);
}

void SetFieldByNumUTF8(const char *p_group, int p_index, const char *p_value, int *r_success)
{
	char t_index_str[16];
	char *t_result;

    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[OPERATION_SET_FIELD_BY_NUM_UTF8] == NULL)
	{
		*r_success = EXTERNAL_FAILURE;
		return;
	}
    
	sprintf(t_index_str, "%d", p_index);
	t_result = (s_operations[OPERATION_SET_FIELD_BY_NUM_UTF8])(t_index_str, p_group, p_value, r_success);
	if (t_result != NULL)
		s_delete(t_result);
}

void SetFieldByIdUTF8(const char *p_group, unsigned long p_id, const char *p_value, int *r_success)
{
	char t_index_str[16];
	char *t_result;

    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[OPERATION_SET_FIELD_BY_ID_UTF8] == NULL)
	{
		*r_success = EXTERNAL_FAILURE;
		return;
	}
    
	sprintf(t_index_str, "%ld", p_id);    
    t_result = (s_operations[OPERATION_SET_FIELD_BY_ID_UTF8])(t_index_str, p_group, p_value, r_success);
	if (t_result != NULL)
		s_delete(t_result);
}

void ShowImageByNameUTF8(const char *p_group, const char *p_name, int *r_success)
{
	char *t_result;

    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[OPERATION_SHOW_IMAGE_BY_NAME_UTF8] == NULL)
	{
		*r_success = EXTERNAL_FAILURE;
		return;
	}
    
	// MDW-2013-05-08 : fix for bug 7913
	t_result = (s_operations[OPERATION_SHOW_IMAGE_BY_NAME_UTF8])(p_name, p_group, NULL, r_success);
	if (t_result != NULL)
		(s_delete)(t_result);
}

void ShowImageByNumUTF8(const char *p_group, int p_index, int *r_success)
{
	char t_index_str[16];
	char *t_result;

    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[OPERATION_SHOW_IMAGE_BY_NUM_UTF8] == NULL)
	{
		*r_success = EXTERNAL_FAILURE;
		return;
	}
    
	sprintf(t_index_str, "%d", p_index);
	t_result = (s_operations[OPERATION_SHOW_IMAGE_BY_NUM_UTF8])(t_index_str, p_group, NULL, r_success);
	if (t_result != NULL)
		s_delete(t_result);
}

void ShowImageByIdUTF8(const char *p_group, unsigned long p_id, int *r_success)
{
	char t_index_str[16];
	char *t_result;

    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[OPERATION_SHOW_IMAGE_BY_ID_UTF8] == NULL)
	{
		*r_success = EXTERNAL_FAILURE;
		return;
	}
    
	sprintf(t_index_str, "%ld", p_id);
    t_result = (s_operations[OPERATION_SHOW_IMAGE_BY_ID_UTF8])(t_index_str, p_group, NULL, r_success);
	if (t_result != NULL)
		s_delete(t_result);
}

char *GetVariableUTF8(const char *p_name, int *r_success)
{
	char *t_result;

    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[OPERATION_GET_VARIABLE_UTF8] == NULL)
	{
		*r_success = EXTERNAL_FAILURE;
		return NULL;
	}
    
	t_result = (s_operations[OPERATION_GET_VARIABLE_UTF8])(p_name, NULL, NULL, r_success);
    return retstr(t_result);
}

void SetVariableUTF8(const char *p_name, const char *p_value, int *r_success)
{
	char *t_result;

    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[OPERATION_SET_VARIABLE_UTF8] == NULL)
	{
		*r_success = EXTERNAL_FAILURE;
		return;
	}
    
	t_result = (s_operations[OPERATION_SET_VARIABLE_UTF8])(p_name, p_value, NULL, r_success);
    if (t_result != NULL)
		s_delete(t_result);
}

void GetVariableExUTF8(const char *p_name, const char *p_key, const ExternalString *r_value, Bool p_is_text, int *r_success)
{
	char *t_result;
    int t_operation;
    
    if (p_is_text)
        t_operation = OPERATION_GET_VARIABLE_EX_UTF8_TEXT;
    else
        t_operation = OPERATION_GET_VARIABLE_EX_UTF8_BINARY;

    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[t_operation] == NULL)
    {
        *r_success = EXTERNAL_FAILURE;
        return;
    }
    
	t_result = (s_operations[t_operation])(p_name, p_key, (char *)r_value, r_success);
    if (t_result != NULL)
		s_delete(t_result);
}

void SetVariableExUTF8(const char *p_name, const char *p_key, const ExternalString *p_value, Bool p_is_text, int *r_success)
{
	char *t_result;
    int t_operation;
    
    if (p_is_text)
        t_operation = OPERATION_SET_VARIABLE_EX_UTF8_TEXT;
    else
        t_operation = OPERATION_SET_VARIABLE_EX_UTF8_BINARY;


    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[t_operation] == NULL)
    {
        *r_success = EXTERNAL_FAILURE;
        return;
    }
    
	t_result = (s_operations[t_operation])(p_name, p_key, (char *)p_value, r_success);
    if (t_result != NULL)
		s_delete(t_result);
}

void GetArrayUTF8(const char *p_name, int *r_element_count, ExternalString *r_values, char **r_keys, Bool p_is_text, int *r_success)
{
	ExternalArray t_array;
	int t_operation;
    char *t_result;

    if (p_is_text)
        t_operation = OPERATION_GET_ARRAY_UTF8_TEXT;
    else
        t_operation = OPERATION_GET_ARRAY_UTF8_BINARY;


    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[t_operation] == NULL)
    {
        *r_success = EXTERNAL_FAILURE;
        return;
    }

	t_array . nelements = *r_element_count;
	t_array . strings = r_values;
	t_array . keys = r_keys;
	t_result = (s_operations[t_operation])(p_name, NULL, (char *)&t_array, r_success);
	if (t_result != NULL)
		s_delete(t_result);
    
	if (*r_success == EXTERNAL_SUCCESS)
		*r_element_count = t_array . nelements;
}

void SetArrayUTF8(const char *p_name, int p_element_count, ExternalString *p_values, char **p_keys, Bool p_is_text, int *r_success)
{
	ExternalArray t_array;
	int t_operation;
    char *t_result;
    
    if (p_is_text)
        t_operation = OPERATION_SET_ARRAY_UTF8_TEXT;
    else
        t_operation = OPERATION_SET_ARRAY_UTF8_BINARY;

    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[t_operation] == NULL)
    {
        *r_success = EXTERNAL_FAILURE;
        return;
    }
    
	t_array . nelements = p_element_count;
	t_array . strings = p_values;
	t_array . keys = p_keys;
	t_result = (s_operations[t_operation])(p_name, NULL, (char *)&t_array, r_success);
	if (t_result != NULL)
		s_delete(t_result);
}

Bool SecurityCanAccessFileUTF8(const char *p_file)
{
    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[SECURITY_CHECK_FILE_UTF8] == NULL)
		return False;
    
	if (s_security_handlers != NULL)
		return s_security_handlers[SECURITY_CHECK_FILE_UTF8](p_file);
	return True;
}

Bool SecurityCanAccessHostUTF8(const char *p_host)
{
    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[SECURITY_CHECK_HOST_UTF8] == NULL)
		return False;
    
	if (s_security_handlers != NULL)
		return s_security_handlers[SECURITY_CHECK_HOST_UTF8](p_host);
	return True;
}

Bool SecurityCanAccessLibraryUTF8(const char *p_library)
{
    // ExternalV0, interface V3 from LiveCode 6.7.4 have no UTF-8 functions
    if (s_external_interface_version < 2 ||
            s_operations[SECURITY_CHECK_LIBRARY_UTF8] == NULL)
		return False;
    
	if (s_security_handlers != NULL)
		return s_security_handlers[SECURITY_CHECK_LIBRARY_UTF8](p_library);
	return True;
}

////////////////////////////////////////////////////////////////////////////////

// AL-2015-02-10: [[ SB Inclusions ]] Add wrappers for ExternalV0 module loading callbacks
// SN-2015-02-24: [[ Broken Win Compilation ]] LoadModule is a Win32 API function...
void LoadModuleByName(const char *p_module, void **r_handle, int *r_success)
{
    char *t_result;

    if (s_external_interface_version < 3)
    {
        *r_success = EXTERNAL_FAILURE;
        return;
    }

    t_result = (s_operations[OPERATION_LOAD_MODULE])(p_module, (const char *)r_handle, NULL, r_success);
    
    if (t_result != NULL)
        s_delete(t_result);
}

void UnloadModule(void *p_handle, int *r_success)
{
    char *t_result;

    if (s_external_interface_version < 3)
    {
        *r_success = EXTERNAL_FAILURE;
        return;
    }

    t_result = (s_operations[OPERATION_UNLOAD_MODULE])(p_handle, NULL, NULL, r_success);
    
    if (t_result != NULL)
        s_delete(t_result);
}

void ResolveSymbolInModule(void *p_handle, const char *p_symbol, void **r_resolved, int *r_success)
{
    char *t_result;

    if (s_external_interface_version < 3)
    {
        *r_success = EXTERNAL_FAILURE;
        return;
    }

    t_result = (s_operations[OPERATION_RESOLVE_SYMBOL_IN_MODULE])(p_handle, p_symbol, (const char *)r_resolved, r_success);
    
    if (t_result != NULL)
        s_delete(t_result);
}

char *CopyNativePathOfModule(void *p_handle, int *r_success)
{
    char *t_result;

    if (s_external_interface_version < 5)
    {
        *r_success = EXTERNAL_FAILURE;
        return NULL;
    }

    t_result = (s_operations[OPERATION_COPY_NATIVE_PATH_OF_MODULE])(p_handle, NULL, NULL, r_success);
    
    return retstr(t_result);
}

////////////////////////////////////////////////////////////////////////////////
//
// IM-2014-09-23: [[ RevBrowserCEF ]] External V4 functions

extern void GetXDisplayHandle(void **r_display, int *r_success)
{
    char *t_result;

    if (s_external_interface_version < 4 ||
            s_operations[OPERATION_GET_XDISPLAY_HANDLE] == NULL)
    {
        *r_success = EXTERNAL_FAILURE;
        return;
    }

    t_result = (s_operations[OPERATION_GET_XDISPLAY_HANDLE])(NULL, NULL, r_display, r_success);
    if (t_result != NULL)
        s_delete(t_result);
}

////////////////////////////////////////////////////////////////////////////////

// V4: UTF-8 <-> native string conversion

const char *ConvertCStringFromNativeToUTF8(const char *p_native, int *r_success)
{
    char *t_result;
    
    if (s_external_interface_version < 4)
    {
        *r_success = EXTERNAL_FAILURE;
        return NULL;
    }
    
    t_result = (s_operations[OPERATION_CONVERT_FROM_NATIVE_TO_UTF8])(p_native, NULL, NULL, r_success);
    
    return t_result;
}

const char *ConvertCStringToNativeFromUTF8(const char *p_utf8, int *r_success)
{
    char *t_result;
    
    if (s_external_interface_version < 4)
    {
        *r_success = EXTERNAL_FAILURE;
        return NULL;
    }
    
    t_result = (s_operations[OPERATION_CONVERT_TO_NATIVE_FROM_UTF8])(p_utf8, NULL, NULL, r_success);
    
    return t_result;
}

////////////////////////////////////////////////////////////////////////////////

// These stubs allow externals to use the weak linking script.

void *MCSupportLibraryLoad(const char *p_path)
{
    int t_retval = EXTERNAL_SUCCESS;
    void *t_result = NULL;
    LoadModuleByName(p_path, &t_result, &t_retval);
    return t_retval == EXTERNAL_SUCCESS ? t_result : NULL;
}

void MCSupportLibraryUnload(void *p_handle)
{
    int t_retval = EXTERNAL_SUCCESS;
    UnloadModule(p_handle, &t_retval);
}

void *MCSupportLibraryLookupSymbol(void *p_handle, const char *p_symbol)
{
    int t_retval = EXTERNAL_SUCCESS;
    void *t_pointer = NULL;
    ResolveSymbolInModule(p_handle, p_symbol, &t_pointer, &t_retval);
    return t_retval == EXTERNAL_SUCCESS ? t_pointer : NULL;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef TARGET_SUBPLATFORM_IPHONE
struct LibExport
{
	const char *name;
	void *address;
};

struct LibInfo
{
	const char **name;
	struct LibExport *exports;
};

static struct LibExport __libexports[] =
{
	{ "getXtable", getXtable },
	{ "configureSecurity", configureSecurity },
    { "setExternalInterfaceVersion", setExternalInterfaceVersion },
	{ 0, 0 }
};

struct LibInfo __libinfo =
{
	&g_external_name,
	__libexports
};
#endif

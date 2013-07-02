#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <revolution/external.h>

#ifdef _WINDOWS
#define LIBRARY_EXPORT __declspec(dllexport)
#else
#define LIBRARY_EXPORT
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
	OPERATION_SET_ARRAY
};

enum
{
	SECURITY_CHECK_FILE,
	SECURITY_CHECK_HOST,
	SECURITY_CHECK_LIBRARY
};

typedef char *(*ExternalOperationCallback)(const char *p_arg_1, const char *p_arg_2, const char *p_arg_3, int *r_success);
typedef void (*ExternalDeleteCallback)(void *p_block);

typedef Bool (*ExternalSecurityHandler)(const char *p_op);

extern const char *g_external_name;
extern ExternalDeclaration g_external_table[];

static ExternalOperationCallback *s_operations = NULL;
static ExternalDeleteCallback s_delete = NULL;

static ExternalSecurityHandler *s_security_handlers = NULL;

#if defined(_LINUX) || defined(__MACOSX) || defined(TARGET_SUBPLATFORM_ANDROID)
void getXtable(ExternalOperationCallback p_operations[], ExternalDeleteCallback p_delete, const char **r_name, ExternalDeclaration **r_table, ExternalDeleteCallback *r_external_delete) __attribute__((visibility("default")));
void configureSecurity(ExternalSecurityHandler *p_handlers) __attribute__((visibility("default")));
#endif

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
	{ 0, 0 }
};

struct LibInfo __libinfo =
{
	&g_external_name,
	__libexports
};
#endif

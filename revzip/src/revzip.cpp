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

#include <map>
#include <string>
#include <list>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>

#include <fcntl.h>
#include <errno.h>

#include <zip.h>

#include <revolution/external.h>
#include <revolution/support.h>

#if defined(_MACOSX)
#define stricmp strcasecmp
#include <CoreServices/CoreServices.h>
#endif

#ifdef _WINDOWS
#define stricmp _stricmp
#endif

#ifdef _LINUX
#include <unistd.h>
#define stricmp strcasecmp
#endif

#if defined(TARGET_SUBPLATFORM_IPHONE) || defined(TARGET_SUBPLATFORM_ANDROID)
#define stricmp strcasecmp
#include <unistd.h>
#endif

#define REVZIP_READ_BUFFER_SIZE 8192

typedef std::map<std::string, struct zip *> zipmap_t;
typedef zipmap_t::iterator zipmap_iterator_t;
typedef zipmap_t::const_iterator zipmap_const_iterator_t;

static zipmap_t s_zip_container;

static char *s_progress_callback = NULL;
static bool s_operation_in_progress = false;
static bool s_operation_cancelled = false;


// Utility function to abstract the process of converting a path to native format
// and resolving it, as this is done several times in this external. The returned
// buffer must be freed by the caller.
char *utilityProcessPath(const char *p_path)
{
    // SN-2014-11-17: [[ Bug 14032 ]] Now gets a UTF-8 encoded string as input
    char *t_resolved_path;
    t_resolved_path = os_path_resolve(p_path);
    
    return t_resolved_path;
}

/*
 Searches for zip* in the container.
 Returns the pointer if successfully, the NULL - if not has found.
*/
struct zip *find_zip_by_name(const char* p_name)
{
	struct zip *t_result;
	t_result = NULL;

	zipmap_const_iterator_t t_it = s_zip_container.find(p_name);
	if( t_it != s_zip_container.end() )
		t_result = t_it->second;

	return t_result;
}

void* imemdup(const void *p_sptr, size_t p_size)
{
  void *t_dptr;
  t_dptr = malloc(p_size);
  memcpy(t_dptr, p_sptr, p_size);
  return t_dptr;
}


int zip_progress_callback(void *p_context, struct zip *p_archive, const char *p_item, 
						   int p_type, unsigned long p_item_progress, unsigned long p_item_total, 
						   unsigned long p_global_progress, unsigned long p_global_total)
{
	if (s_progress_callback == NULL)
		return 0;

	if (s_operation_cancelled)
		return 1;

	char t_message[1024];
	int t_return_value;

    // SN-2014-11-17: [[ Bug 14032 ]] The path is kept in UTF-8
    char *t_path;
	t_path = strdup(zip_get_path(p_archive));
	
	sprintf(t_message, "%s \"%s\", \"%s\", \"%s\", %lu, %lu, %lu, %lu",
			  	s_progress_callback,
					t_path, p_item,
					p_type == 0 ? "unpacking" : "packing",
					p_item_progress, p_item_total, 
					p_global_progress, p_global_total);
	
	if (t_path != NULL)
		free(t_path);
	
    // SN-2014-11-17: [[ Bug 14032 ]] The name of the callback, and the path, are UTF-8 encoded
	SendCardMessageUTF8(t_message, &t_return_value);
	
	if (s_operation_cancelled)
		return 1;
	
	return 0;
}

void revZipOpenArchive(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	char *t_result = NULL;
	Bool t_error = False;

	if (p_argument_count != 2)
	{
		t_result = strdup("ziperr,illegal arguments");
		t_error = True;
	}

	if (!t_error)
	{
        // SN-2014-11-17: [[ Bug 14032 ]] Update the parameters to UTF-8
		if (!SecurityCanAccessFileUTF8(p_arguments[0]))
		{
			t_result = strdup("ziperr,file access not permitted");
			t_error = True;
		}
	}

	char *t_path = NULL;
	if (!t_error)
	{
		t_path = utilityProcessPath(p_arguments[0]);

		if (t_path == NULL)
		{
			t_result = strdup("ziperr,illegal path");
		}
	}

	struct zip *t_archive = NULL;
	int t_err;
	char t_errstr[1024]; 

	if (t_result == NULL)
	{
		int t_openflag = -1;
		if((stricmp(p_arguments[1], "write")) == 0)
			t_openflag = ZIP_CREATE | ZIP_EXCL;
		if((stricmp(p_arguments[1], "read")) == 0)
			t_openflag = 0;
		if((stricmp(p_arguments[1], "update")) == 0)
			t_openflag = ZIP_CREATE;
		if( t_openflag == -1 )
		{
			t_result = strdup("ziperr,unknown access mode");
			t_error = False;
		}
		else
		{
			if((t_archive = zip_open(t_path, t_openflag, &t_err)) == NULL) 
			{
				zip_error_to_str(t_errstr, sizeof(t_errstr), t_err, errno);
				std::string t_outerr = "ziperr," + std::string(t_errstr);
				t_result = strdup(t_outerr.c_str());
			}
			else
			{
				s_zip_container[t_path] = t_archive;
				zip_set_progress_callback(t_archive, zip_progress_callback, 0);
			}
		}
	}

	if( t_result == NULL )
		t_result = strdup("");

	if( t_path )
		free(t_path);

	*r_pass = False;
	*r_err = t_error;
	*r_result = t_result;
}


void revZipCloseArchive(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	char *t_result = NULL;
	Bool t_error = False;

	if (p_argument_count != 1)
	{
		t_result = strdup("ziperr,illegal arguments");
		t_error = True;
	}

	char *t_path = NULL;
	if( t_result == NULL )
	{
		t_path = utilityProcessPath(p_arguments[0]);
		if (t_path == NULL)
		{
			t_result = strdup("ziperr,illegal path");
			t_error = False;
		}
	}

	struct zip *t_archive;
	t_archive = NULL;
	if (t_result == NULL)
	{
		t_archive = find_zip_by_name( t_path );
		if( !t_archive )
		{
			t_result = strdup("ziperr,archive not open");
			t_error = False;
		}
	}
	
	if (t_result == NULL)
	{
		int t_err;
		char t_errstr[1024]; 

		s_operation_in_progress = true;
		s_operation_cancelled = false;
		t_err = zip_close(t_archive);
		s_operation_in_progress = false;
		
		if (s_operation_cancelled)
		{
			s_operation_cancelled = false;
			t_result = strdup("cancelled");
			t_error = False;
		}
		else if (t_err != 0)
		{
			zip_error_to_str(t_errstr, sizeof(t_errstr), t_err, errno);
			std::string t_outerr = "ziperr," + std::string(t_errstr);
			t_result = strdup(t_outerr.c_str());
			t_error = False;
		}
		s_zip_container.erase(t_path);
	}

	if (t_result == NULL)
		t_result = strdup("");

	if (t_path)
		free(t_path);

	*r_pass = False;
	*r_err = t_error;
	*r_result = t_result;
}


void revZipOpenArchives(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	char *t_result = NULL;
	Bool t_error = False;

	if (p_argument_count != 0)
	{
		t_result = strdup("ziperr,illegal arguments");
		t_error = True;
	}

	if (t_result == NULL)
	{
		std::string t_strlist;
		for(zipmap_const_iterator_t it = s_zip_container.begin(); it != s_zip_container.end(); ++it)
		{
			char* t_line;
            // SN-2014-11-17: [[ Bug 14032 ]] We now keep the UTF-8 encoded string
			t_line = strdup(it->first.c_str());
			if( t_line )
			{
				t_strlist += std::string(t_line);
				t_strlist += "\n";
				free(t_line);
			}
		}
		if( !t_strlist.empty() )
			t_result = strdup(t_strlist.c_str());
	}

	if (t_result == NULL)
		t_result = strdup("");

	*r_pass = False;
	*r_err = t_error;
	*r_result = t_result;
}

static void revZipAddItemWithDataAndCompression(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err, bool p_compressed)
{
		char *t_result = NULL;
	Bool t_error = False;

	if (p_argument_count != 3)
	{
		t_result = strdup("ziperr,illegal arguments");
		t_error = True;
	}

	char *t_path = NULL;
	if( t_result == NULL )
	{
		t_path = utilityProcessPath(p_arguments[0]);
		if( t_path == NULL )
		{
			t_result = strdup("ziperr,illegal path");
			t_error = False;
		}
	}

	struct zip *t_archive;
	t_archive = NULL;
	if (t_result == NULL)
	{
		t_archive = find_zip_by_name( t_path );
		if( !t_archive )
		{
			t_result = strdup("ziperr,archive not open");
			t_error = False;
		}
	}
	
	if (t_result == NULL)
	{
		struct zip_source *t_source = NULL;

		ExternalString mcData;
		int intRetValue;
        // SN-2014-11-17: [[ Bug 14032 ]] The variable name is UTF-8 encoded - not the data
		GetVariableExUTF8 (p_arguments[2], "", &mcData, false, &intRetValue);
		if( intRetValue != EXTERNAL_SUCCESS )
		{
			t_result = strdup("ziperr,illegal variable");
			t_error = False;
		}
		else
		{
			char* t_data = NULL;
			t_data = (char*) imemdup(mcData.buffer, mcData.length);
			if (((t_source = zip_source_buffer(t_archive, t_data, mcData.length, 1)) == NULL) ||
				 (zip_add(t_archive, p_arguments[1], t_source) < 0))
			{
				zip_source_free(t_source);
				std::string t_outerr = "ziperr," + std::string((zip_strerror(t_archive)));
				t_result = strdup(t_outerr.c_str());
				t_error = False;
			}
			else
			{
				if (!p_compressed)
					zip_recompress(t_archive, zip_name_locate(t_archive, p_arguments[1], 0), ZIP_CM_STORE);
			}
		}
	}

	if( t_result == NULL )
		t_result = strdup("");
	
	if( t_path )
		free(t_path);

	*r_pass = False;
	*r_err = t_error;
	*r_result = t_result;
}

void revZipAddItemWithData(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	revZipAddItemWithDataAndCompression(p_arguments, p_argument_count, r_result, r_pass, r_err, true);
}

void revZipAddUncompressedItemWithData(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	revZipAddItemWithDataAndCompression(p_arguments, p_argument_count, r_result, r_pass, r_err, false);
}

static void revZipAddItemWithFileAndCompression(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err, bool p_compressed)
{
	char *t_result = NULL;
	Bool t_error = False;

	if (p_argument_count != 3)
	{
		t_result = strdup("ziperr,illegal arguments");
		t_error = True;
	}

	char *t_path = NULL;
	char *t_filepath = NULL;
	if (t_result == NULL)
	{
		t_path = utilityProcessPath(p_arguments[0]);
		t_filepath = utilityProcessPath(p_arguments[2]);

		if (t_path == NULL || t_filepath == NULL)
		{
			t_result = strdup("ziperr,illegal path");
			t_error = False;
		}
	}

	struct zip *t_archive;
	t_archive = NULL;
	if (t_result == NULL)
	{
		t_archive = find_zip_by_name( t_path );
		if( !t_archive )
		{
			t_result =strdup("ziperr,archive not open");
			t_error = False;
		}
	}

	struct zip_source *t_source;
	t_source = NULL;
	if (t_result == NULL)
	{
		if (((t_source = zip_source_filename(t_archive, t_filepath, 0, 0)) == NULL) ||
			 (zip_add(t_archive, p_arguments[1], t_source) < 0))
		{
			zip_source_free(t_source);
			std::string t_outerr = "ziperr," + std::string((zip_strerror(t_archive)));
			t_result = strdup(t_outerr.c_str());
			t_error = False;
		}
		else
		{
			if (!p_compressed)
				zip_recompress(t_archive, zip_name_locate(t_archive, p_arguments[1], 0), ZIP_CM_STORE);
		}
	}

	if (t_result == NULL)
		t_result = strdup("");

	if( t_path ) 
		free( t_path );
		
	if( t_filepath ) 
		free( t_filepath );

	*r_pass = False;
	*r_err = t_error;
	*r_result = t_result;
}

void revZipAddItemWithFile(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	revZipAddItemWithFileAndCompression(p_arguments, p_argument_count, r_result, r_pass, r_err, true);
}

void revZipAddUncompressedItemWithFile(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	revZipAddItemWithFileAndCompression(p_arguments, p_argument_count, r_result, r_pass, r_err, false);
}

void revZipExtractItemToVariable(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	char *t_result = NULL;
	Bool t_error = False;

	if (p_argument_count != 3)
	{
		t_result = strdup("ziperr,illegal arguments");
		t_error = True;
	}

	char *t_path = NULL;
	if( t_result == NULL )
	{
		t_path = utilityProcessPath(p_arguments[0]);
		if( t_path == NULL )
		{
			t_result = strdup("ziperr,illegal path");
			t_error = False;
		}
	}

	struct zip *t_archive;
	t_archive = NULL;
	if (t_result == NULL)
	{
		t_archive = find_zip_by_name( t_path );
		if( !t_archive )
		{
			t_result = strdup("ziperr,archive not open");
			t_error = False;
		}
	}
	
	int t_index;
	if (t_result == NULL)
	{
		t_index = zip_name_locate(t_archive, p_arguments[1], ZIP_FL_NOCASE);
		if( t_index == -1 )
		{
			t_result = strdup("ziperr,file not found");
			t_error = False;
		}
	}

	struct zip_stat t_stat;
	if (t_result == NULL)
	{
		if (zip_stat_index(t_archive, t_index, 0, &t_stat) != 0)
		{
			std::string t_outerr = "ziperr," + std::string(zip_strerror(t_archive));
			t_result = strdup(t_outerr . c_str());
			t_error = False;
		}
	}

	char *t_data = NULL;
	if (t_result == NULL)
	{
		t_data = (char *)malloc(t_stat . size);
		if (t_data == NULL)
		{
			t_result = strdup("ziperr,out of memory");
			t_error = False;
		}
	}

	struct zip_file *t_file;
	t_file = NULL;
	if (t_result == NULL)
	{
		t_file = zip_fopen_index(t_archive, t_index, ZIP_FL_UNCHANGED);
		if (t_file == NULL)
		{
			std::string t_outerr = "ziperr," + std::string((zip_strerror(t_archive)));
			t_result = strdup(t_outerr.c_str());
			t_error = False;
		}
	}

	if (t_result == NULL)
	{
		ssize_t t_read;
		t_read = 0;
		
		s_operation_in_progress = true;
		s_operation_cancelled = false;
		do
		{
			ssize_t t_bytes_read;
			t_bytes_read = zip_fread(t_file, t_data + t_read, REVZIP_READ_BUFFER_SIZE);
			if (t_bytes_read <= 0)
				break;

			t_read += t_bytes_read;
		}
		while(t_read != t_stat . size && !s_operation_cancelled);
		s_operation_in_progress = false;

		if (s_operation_cancelled)
		{
			s_operation_cancelled = false;
			t_result = strdup("cancelled");
			t_error = False;
		}
		else if (t_read == t_stat . size)
		{
			ExternalString t_mcData;
			int t_retval;

			t_mcData.buffer = t_data;
			t_mcData.length = t_stat . size;
            // SN-2014-11-17: [[ Bug 14032 ]] The variable name is UTF-8 encoded - not the data
			SetVariableExUTF8 (p_arguments[2], "", &t_mcData, false, &t_retval);
			if(t_retval != EXTERNAL_SUCCESS)
			{
				t_result = strdup("ziperr,illegal variable");
				t_error = False;
			}
		}
		else
		{
			std::string t_outerr = "ziperr," + std::string((zip_strerror(t_archive)));
			t_result = strdup(t_outerr.c_str());
			t_error = False;
		}
	}

	if (t_file != NULL)
		zip_fclose(t_file);

	if (t_data != NULL)
		free(t_data);

	if (t_path != NULL)
		free(t_path);

	if (t_result == NULL)
	{
		t_result = strdup("");
		t_error = False;
	}

	*r_pass = False;
	*r_err = t_error;
	*r_result = t_result;
}


void revZipExtractItemToFile(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	char *t_result = NULL;
	Bool t_error = True;

	if (p_argument_count != 3)
	{
		t_result = strdup("ziperr,illegal arguments");
		t_error = True;
	}

	char *t_path = NULL;
	char *t_out_filename = NULL;
	if (t_result == NULL)
	{
		t_path = utilityProcessPath(p_arguments[0]);
		t_out_filename = utilityProcessPath(p_arguments[2]);
		if( t_path == NULL || t_out_filename == NULL )
		{
			t_result = strdup("ziperr,illegal path");
			t_error = False;
		}
	}

	struct zip *t_archive;
	t_archive = NULL;
	if (t_result == NULL)
	{
		t_archive = find_zip_by_name( t_path );
		if (!t_archive)
		{
			t_result = strdup("ziperr,archive not open");
			t_error = False;
		}
	}
	
	int t_index;
	t_index = -1;
	if (t_result == NULL)
	{
		t_index = zip_name_locate(t_archive, p_arguments[1], ZIP_FL_NOCASE);
		if (t_index == -1)
		{
			t_result = strdup("ziperr,file not found");
			t_error = False;
		}
	}
	
	struct zip_file *t_file;
	t_file = NULL;
	if (t_result == NULL)
	{
		t_file = zip_fopen_index(t_archive, t_index, ZIP_FL_UNCHANGED);
		if (t_file == NULL)
		{
			std::string t_outerr = "ziperr," + std::string((zip_strerror(t_archive)));
			t_result = strdup(t_outerr.c_str());
			t_error = False;
		}
	}
	
	FILE *t_out_stream;
	t_out_stream = NULL;
	if (t_result == NULL)
	{
		t_out_stream = fopen(t_out_filename, "wb");
		if (t_out_stream == NULL)
		{
			t_result = strdup("ziperr,unable to open output file");
			t_error = False;
		}
	}
	
	if (t_result == NULL)
	{
		char t_buffer[REVZIP_READ_BUFFER_SIZE];
		int t_read;
		t_read = 0;
		
		s_operation_in_progress = true;
		s_operation_cancelled = false;
		do
		{
			t_read = zip_fread(t_file, t_buffer, REVZIP_READ_BUFFER_SIZE);
			if (t_read > 0)
			{
				int t_written;
				t_written = fwrite(t_buffer, t_read, 1, t_out_stream);
				if (t_written != 1)
				{
					t_result = strdup("ziperr,error while writing file");
					t_error = False;
				}
			}
			else if (t_read == -1)
			{
				t_result = strdup("ziperr,error while reading zipped data");
				t_error = False;
			}
		}
		while(t_read != 0 && t_result == NULL && !s_operation_cancelled);
		s_operation_in_progress = false;

		if (s_operation_cancelled)
		{
			s_operation_cancelled = false;
			t_result = strdup("cancelled");
			t_error = False;
		}
	}
	
	if (t_out_stream != NULL)
	{
		fclose(t_out_stream);
		if (t_result != NULL)
			unlink(t_out_filename);
	}
	
	if (t_file != NULL)
		zip_fclose(t_file);

	if (t_path != NULL)
		free(t_path);
		
	if (t_out_filename != NULL)
		free(t_out_filename);
	
	if (t_result == NULL)
	{
		t_result = strdup("");
		t_error = False;
	}
	
	*r_pass = False;
	*r_err = t_error;
	*r_result = t_result;
}

void revZipReplaceItemWithFile(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	char *t_result = NULL;
	Bool t_error = False;

	if (p_argument_count != 3)
	{
		t_result = strdup("ziperr,illegal arguments");
		t_error = True;
	}

	char *t_path = NULL;
	char *t_filepath = NULL;
	if (t_result == NULL)
	{
		t_path = utilityProcessPath(p_arguments[0]);
		t_filepath = utilityProcessPath(p_arguments[2]);
		if (t_path == NULL || t_filepath == NULL)
		{
			t_result = strdup("ziperr,illegal path");
			t_error = False;
		}
	}

	struct zip *t_archive;
	t_archive = NULL;
	if(t_result == NULL)
	{
		t_archive = find_zip_by_name( t_path );
		if( !t_archive )
		{
			t_result = strdup("ziperr,archive not open");
			t_error = True;
		}
	}

	struct zip_source *t_source;
	t_source = NULL;
	int t_index;

	if(t_result == NULL)
	{
		t_index = zip_name_locate(t_archive, p_arguments[1], ZIP_FL_NOCASE);
		if( t_index == -1 )
		{
			t_result = strdup("ziperr,file not found");
			t_error = False;
		}
		else
		{
			if (((t_source = zip_source_filename(t_archive, t_filepath, 0, 0)) == NULL) ||
				 (zip_replace(t_archive, t_index, t_source) < 0))
			{
				zip_source_free(t_source);
				std::string t_outerr = "ziperr," + std::string((zip_strerror(t_archive)));
				t_result = strdup(t_outerr.c_str());
				t_error = False;
			}
		}
	}

	if (t_result == NULL)
		t_result = strdup("");

	if( t_path ) 
		free( t_path );
	if( t_filepath ) 
		free( t_filepath );

	*r_pass = False;
	*r_err = t_error;
	*r_result = t_result;
}


void revZipReplaceItemWithData(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	char *t_result = NULL;
	Bool t_error = False;

	if (p_argument_count != 3)
	{
		t_result = strdup("ziperr,illegal arguments");
		t_error = True;
	}

	char *t_path = NULL;
	if( t_result == NULL )
	{
		t_path = utilityProcessPath(p_arguments[0]);
		if( t_path == NULL )
		{
			t_result = strdup("ziperr,illegal path");
			t_error = False;
		}
	}

	struct zip *t_archive;
	int t_index;
	t_archive = NULL;
	if (t_result == NULL)
	{
		t_archive = find_zip_by_name( t_path );
		if( !t_archive )
		{
			t_result = strdup("ziperr,archive not open");
			t_error = False;
		}
	}
	
	if (t_result == NULL)
	{
		t_index = zip_name_locate(t_archive, p_arguments[1], ZIP_FL_NOCASE);
		if( t_index == -1 )
		{
			t_result = strdup("ziperr,file not found");
			t_error = False;
		}
		else
		{
			struct zip_source *t_source = NULL;

			ExternalString mcData;
			int intRetValue;
            // SN-2014-11-17: [[ Bug 14032 ]] The variable name is UTF-8 encoded - not the data
			GetVariableExUTF8 (p_arguments[2], "", &mcData, false, &intRetValue);
			if( intRetValue != EXTERNAL_SUCCESS )
			{
				t_result = strdup("ziperr,illegal variable");
				t_error = False;
			}
			else
			{
				char* t_data = NULL;
				t_data = (char*) imemdup(mcData.buffer, mcData.length);
				if (((t_source = zip_source_buffer(t_archive, t_data, mcData.length, 1)) == NULL) ||
					 (zip_replace(t_archive, t_index, t_source) < 0))
				{
					zip_source_free(t_source);
					std::string t_outerr = "ziperr," + std::string((zip_strerror(t_archive)));
					t_result = strdup(t_outerr.c_str());
					t_error = False;
				}
			}
		}
	}

	if( t_result == NULL )
		t_result = strdup("");
	
	if( t_path )
		free(t_path);

	*r_pass = False;
	*r_err = t_error;
	*r_result = t_result;
}


void revZipRenameItem(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	char *t_result = NULL;
	Bool t_error = False;

	if (p_argument_count != 3)
	{
		t_result = strdup("ziperr,illegal arguments");
		t_error = True;
	}

	char *t_path = NULL;
	if( t_result == NULL )
	{
		t_path = utilityProcessPath(p_arguments[0]);
		if( t_path == NULL )
		{
			t_result = strdup("ziperr,illegal path");
			t_error = False;
		}
	}

	struct zip *t_archive;
	int t_index;
	t_archive = NULL;
	if( t_result == NULL )
	{
		t_archive = find_zip_by_name( t_path );
		if( !t_archive )
		{
			t_result = strdup("ziperr,archive not open");
			t_error = False;
		}
	}
	
	if( t_result == NULL )
	{
		t_index = zip_name_locate(t_archive, p_arguments[1], ZIP_FL_NOCASE);
		if( t_index == -1 )
		{
			t_result = strdup("ziperr,file not found");
			t_error = False;
		}
		else
		{
			if (zip_rename(t_archive, t_index, p_arguments[2]) != 0)
			{
				std::string t_outerr = "ziperr," + std::string((zip_strerror(t_archive)));
				t_result = strdup(t_outerr.c_str());
				t_error = False;
			}
		}
	}

	if( t_result == NULL )
		t_result = strdup("");
	
	if( t_path )
		free(t_path);

	*r_pass = False;
	*r_err = t_error;
	*r_result = t_result;
}


void revZipGetItemAttributes(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	char *t_result = NULL;
	Bool t_error = False;

	if (p_argument_count != 2)
	{
		t_result = strdup("ziperr,illegal arguments");
		t_error = True;
	}

	char *t_path = NULL;
	if( t_result == NULL )
	{
		t_path = utilityProcessPath(p_arguments[0]);
		if( t_path == NULL )
		{
			t_result = strdup("ziperr,illegal path");
			t_error = False;
		}
	}

	struct zip *t_archive;
	int t_index;
	t_archive = NULL;
	if( t_result == NULL )
	{
		t_archive = find_zip_by_name(t_path);
		if( !t_archive )
		{
			t_result = strdup("ziperr,archive not open");
			t_error = False;
		}
	}
	
	if( t_result == NULL )
	{
		t_index = zip_name_locate(t_archive, p_arguments[1], ZIP_FL_NOCASE);
		if( t_index == -1 )
		{
			t_result = strdup("ziperr,file not found");
			t_error = False;
		}
		else
		{
			unsigned char t_madeby;
			unsigned int t_attributes;

			if (zip_get_attributes(t_archive, t_index, &t_madeby, &t_attributes) != 0)
			{
				std::string t_outerr = "ziperr," + std::string((zip_strerror(t_archive)));
				t_result = strdup(t_outerr.c_str());
				t_error = False;
			}
			else
			{
				// 1 for the madeby, 1 for the separating comma, 10 for the attributes and 1 for a null termination.
				t_result = (char *)malloc(1 + 1 + 10 + 1);
				sprintf(t_result, "%u,%u", t_madeby, t_attributes);
				t_error = False;
			}
		}
	}

	if( t_result == NULL )
		t_result = strdup("");
	
	if( t_path )
		free(t_path);

	*r_pass = False;
	*r_err = t_error;
	*r_result = t_result;

}


// arguments[0] : archive path
// arguments[1] : item name
// arguments[2] : made by (integer between 0 and 255)
// arguments[3] : external attributes (integer)
void revZipSetItemAttributes(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	char *t_result = NULL;
	Bool t_error = False;

	if (p_argument_count != 4)
	{
		t_result = strdup("ziperr,illegal arguments");
		t_error = True;
	}

	char *t_path = NULL;
	if( t_result == NULL )
	{
		t_path = utilityProcessPath(p_arguments[0]);
		if( t_path == NULL )
		{
			t_result = strdup("ziperr,illegal path");
			t_error = False;
		}
	}

	struct zip *t_archive;
	int t_index;
	t_archive = NULL;
	if( t_result == NULL )
	{
		t_archive = find_zip_by_name( t_path );
		if( !t_archive )
		{
			t_result = strdup("ziperr,archive not open");
			t_error = False;
		}
	}
	
	if( t_result == NULL )
	{
		t_index = zip_name_locate(t_archive, p_arguments[1], ZIP_FL_NOCASE);
		if( t_index == -1 )
		{
			t_result = strdup("ziperr,file not found");
			t_error = False;
		}
		else
		{
			unsigned char t_madeby;
			t_madeby = (unsigned char)atoi(p_arguments[2]);

			unsigned int t_attributes;
			t_attributes = (unsigned int)atoi(p_arguments[3]);

			if (zip_set_attributes(t_archive, t_index, t_madeby, t_attributes) != 0)
			{
				std::string t_outerr = "ziperr," + std::string((zip_strerror(t_archive)));
				t_result = strdup(t_outerr.c_str());
				t_error = False;
			}
		}
	}

	if( t_result == NULL )
		t_result = strdup("");
	
	if( t_path )
		free(t_path);

	*r_pass = False;
	*r_err = t_error;
	*r_result = t_result;
}


void revZipDeleteItem(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	char *t_result = NULL;
	Bool t_error = False;

	if (p_argument_count != 2)
	{
		t_result = strdup("ziperr,illegal arguments");
		t_error = True;
	}

	char *t_path = NULL;
	if( t_result == NULL )
	{
		t_path = utilityProcessPath(p_arguments[0]);
		if( t_path == NULL )
		{
			t_result = strdup("ziperr,illegal path");
			t_error = False;
		}
	}

	struct zip *t_archive;
	int t_index;
	t_archive = NULL;
	if (t_result == NULL)
	{
		t_archive = find_zip_by_name( t_path );
		if( !t_archive )
		{
			t_result = strdup("ziperr,archive not open");
			t_error = False;
		}
	}
	
	if (t_result == NULL)
	{
		t_index = zip_name_locate(t_archive, p_arguments[1], ZIP_FL_NOCASE);
		if( t_index == -1 )
		{
			t_result = strdup("ziperr,file not found");
			t_error = False;
		}
		else
		{
			if (zip_delete(t_archive, t_index) != 0)
			{
				std::string t_outerr = "ziperr," + std::string((zip_strerror(t_archive)));
				t_result = strdup(t_outerr.c_str());
				t_error = False;
			}
		}
	}

	if( t_result == NULL )
		t_result = strdup("");
	
	if( t_path )
		free(t_path);

	*r_pass = False;
	*r_err = t_error;
	*r_result = t_result;
}


void revZipEnumerateItems(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	char *t_result = NULL;
	Bool t_error = False;

	if (p_argument_count != 1)
	{
		t_result = strdup("ziperr,illegal arguments");
		t_error = True;
	}

	char *t_path = NULL;
	if( t_result == NULL )
	{
		t_path = utilityProcessPath(p_arguments[0]);
		if( t_path == NULL )
		{
			//t_result = strdup("ziperr,illegal path");
			t_result = strdup("");
			t_error = False;
		}
	}

	struct zip *t_archive;
	t_archive = NULL;
	if (t_result == NULL)
	{
		t_archive = find_zip_by_name( t_path );
		if( !t_archive )
		{
			//t_result = strdup("ziperr,archive not open");
			t_result = strdup("");
			t_error = False;
		}
	}
	
	if (t_result == NULL)
	{
		int t_num_files;
		t_num_files = zip_get_num_files(t_archive);
		std::string t_str_names;
	
		for( int i = 0; i < t_num_files; ++i )
		{
			struct zip_stat t_stat;

			// SN-2015-03-11: [[ Bug 14413 ]] We want to get the bitflags
			//  alongside the name: zip_stat_index provides this.
			if (zip_stat_index(t_archive, i, 0, &t_stat) != 0)
			{
				std::string t_outerr = "ziperr," + std::string((zip_strerror(t_archive)));
				t_result = strdup(t_outerr.c_str());
                // SN-2015-06-02: [[ CID 90610 ]] Quit the loop if an error is
                //  encountered - and set t_error to the right value.
				t_error = True;
                break;
			}
			else
			{
				// SN-2015-03-10: [[ Bug 14413 ]] We convert the string to UTF-8
				//  in case it was natively encoded, as revZipEnumerateItems is
				//  meant to return a UTF-8 encoded string.
				const char *t_converted_name;
                int t_success;

				if (t_stat.bitflags & ZIP_UTF8_FLAG)
				{
                    t_success = EXTERNAL_SUCCESS;
					t_converted_name = t_stat.name;
				}
				else
					t_converted_name = ConvertCStringFromNativeToUTF8(t_stat.name, &t_success);

                if (t_success == EXTERNAL_SUCCESS)
				{
					t_str_names += std::string(t_converted_name);
					t_str_names += "\n";
				}
				else
				{
					t_result = strdup("");
					t_error  = True;
					break;
				}
			}
		}
		
		if( !t_str_names.empty() && t_error == False)
		{
			t_result = strdup(t_str_names.c_str());
			t_error = False;
		}
	}

	if( t_result == NULL )
		t_result = strdup("");
	
	if( t_path )
		free(t_path);

	*r_pass = False;
	*r_err = t_error;
	*r_result = t_result;
}


void revZipDescribeItem(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	char *t_result = NULL;
	Bool t_error = False;

	if (p_argument_count != 2)
	{
		t_result = strdup("ziperr,illegal arguments");
		t_error = True;
	}

	char *t_path = NULL;
	if( t_result == NULL )
	{
		t_path = utilityProcessPath(p_arguments[0]);
		if( t_path == NULL )
		{
			//t_result = strdup("ziperr,illegal path");
			t_result = strdup("");
			t_error = False;
		}
	}

	struct zip *t_archive;
	int t_index;
	t_archive = NULL;
	if (t_result == NULL)
	{
		t_archive = find_zip_by_name( t_path );
		if( !t_archive )
		{
			//t_result = strdup("ziperr,archive not open");
			t_result = strdup("");
			t_error = False;
		}
	}

	if (t_result == NULL)
	{
		t_index = zip_name_locate(t_archive, p_arguments[1], ZIP_FL_NOCASE);
		if( t_index == -1 )
		{
			//t_result = strdup("ziperr,file not found");
			t_result = strdup("");
			t_error = False;
		}
		else
		{
			struct zip_stat* t_stat = (struct zip_stat*) malloc(sizeof(struct zip_stat));
			if((t_stat) && (zip_stat_index(t_archive, t_index, ZIP_FL_NOCASE, t_stat) == 0))
			{
				std::stringstream t_strstream;
				t_strstream << t_stat->index << "," << t_stat->crc << "," << t_stat->size << ",";
				t_strstream << t_stat->mtime << "," << t_stat->comp_size << ",";
				switch( t_stat->comp_method )
				{
				case 0:
					t_strstream << "none";
					break;
				case 1:
					t_strstream << "shrink";
					break;
				case 2:
					t_strstream << "reduce_1";
					break;
				case 3:
					t_strstream << "reduce_2";
					break;
				case 4:
					t_strstream << "reduce_3";
					break;
				case 5:
					t_strstream << "reduce_4";
					break;
				case 6:
					t_strstream << "implode";
					break;
				case 8:
					t_strstream << "deflate";
					break;
				case 9:
					t_strstream << "deflate64";
					break;
				case 10:
					t_strstream << "pkware_implode";
					break;
				default:
					t_strstream << "unknown";
				}
				t_strstream << std::ends;
				t_result = strdup(t_strstream.str() . c_str());
				free( t_stat );
			}
		}
	}

	if (t_result == NULL)
		t_result = strdup("");
	
	if( t_path )
		free(t_path);

	*r_pass = False;
	*r_err = t_error;
	*r_result = t_result;
}

void revZipSetProgressCallback(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	char *t_result = NULL;
	Bool t_error = False;

	if (p_argument_count > 1)
	{
		t_result = strdup("ziperr,illegal arguments");
		t_error = True;
	}

	if (t_result == NULL)
	{
		if (s_progress_callback != NULL)
		{
			free(s_progress_callback);
			s_progress_callback = NULL;
		}
		s_progress_callback = p_argument_count == 0 || *p_arguments[0] == '\0' ? NULL : strdup(p_arguments[0]);
	}

	if (t_result == NULL)
		t_result = strdup("");

	*r_pass = False;
	*r_err = t_error;
	*r_result = t_result;
}

void revZipCancel(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	char *t_result = NULL;
	Bool t_error = False;
	
	if (p_argument_count != 0)
	{
		t_result = strdup("ziperr,illegal arguments");
		t_error = True;
	}
	else if (!s_operation_in_progress)
	{
		t_result = strdup("ziperr,no current operation");
	}

	if (t_result == NULL)
		s_operation_cancelled = true;

	if (t_result == NULL)
		t_result = strdup("");
		
	*r_pass = False;
	*r_err = t_error;
	*r_result = t_result;
}

// SN-2014-11-17: [[ Bug 14032 ]] Update the appropriate functions to get UTF-8 parameters
EXTERNAL_BEGIN_DECLARATIONS("revZip")
	EXTERNAL_DECLARE_COMMAND_UTF8("revZipOpenArchive", revZipOpenArchive)
	EXTERNAL_DECLARE_COMMAND_UTF8("revZipCloseArchive", revZipCloseArchive)
	EXTERNAL_DECLARE_COMMAND_UTF8("revZipAddItemWithFile", revZipAddItemWithFile)
	EXTERNAL_DECLARE_COMMAND_UTF8("revZipAddUncompressedItemWithFile", revZipAddUncompressedItemWithFile)
	EXTERNAL_DECLARE_COMMAND_UTF8("revZipAddItemWithData", revZipAddItemWithData)
	EXTERNAL_DECLARE_COMMAND_UTF8("revZipAddUncompressedItemWithData", revZipAddUncompressedItemWithData)
	EXTERNAL_DECLARE_FUNCTION_UTF8("revZipOpenArchives", revZipOpenArchives)
	EXTERNAL_DECLARE_COMMAND_UTF8("revZipExtractItemToVariable", revZipExtractItemToVariable)
	EXTERNAL_DECLARE_COMMAND_UTF8("revZipExtractItemToFile", revZipExtractItemToFile)
	EXTERNAL_DECLARE_COMMAND_UTF8("revZipReplaceItemWithFile", revZipReplaceItemWithFile)
	EXTERNAL_DECLARE_COMMAND_UTF8("revZipReplaceItemWithData", revZipReplaceItemWithData)
	EXTERNAL_DECLARE_COMMAND_UTF8("revZipRenameItem", revZipRenameItem)
	EXTERNAL_DECLARE_COMMAND_UTF8("revZipDeleteItem", revZipDeleteItem)
	EXTERNAL_DECLARE_COMMAND_UTF8("revZipSetItemAttributes", revZipSetItemAttributes)
	EXTERNAL_DECLARE_FUNCTION_UTF8("revZipGetItemAttributes", revZipGetItemAttributes)
	EXTERNAL_DECLARE_FUNCTION_UTF8("revZipEnumerateItems", revZipEnumerateItems)
	EXTERNAL_DECLARE_FUNCTION_UTF8("revZipDescribeItem", revZipDescribeItem)
	EXTERNAL_DECLARE_COMMAND_UTF8("revZipSetProgressCallback", revZipSetProgressCallback)
	EXTERNAL_DECLARE_COMMAND("revZipCancel", revZipCancel)
EXTERNAL_END_DECLARATIONS

#ifdef _WINDOWS

#include <windows.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved)
{
#ifdef _DEBUG
	if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		s_zip_container . clear();
		_CrtDumpMemoryLeaks();
	}
#endif

    return TRUE;
}

#endif

#ifdef TARGET_SUBPLATFORM_IPHONE
extern "C"
{
extern struct LibInfo __libinfo;
__attribute((section("__DATA,__libs"))) volatile struct LibInfo *__libinfoptr_revzip __attribute__((__visibility__("default"))) = &__libinfo;
}
#endif

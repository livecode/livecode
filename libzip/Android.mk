LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := libzip
LOCAL_SRC_FILES := \
	$(addprefix src/,zip_unchange_data.c zip_open.c zip_stat.c zip_file_get_offset.c \
	zip_source_buffer.c zip_set_progress_callback.c zip_err_str.c zip_fread.c \
	zip_name_locate.c zip_new.c zip_get_num_files.c zip_source_free.c zip_error.c \
	mkstemp.c zip_add.c zip_fopen_index.c zip_free.c zip_entry_new.c \
	zip_close.c zip_source_zip.c zip_rename.c zip_unchange_all.c zip_file_error_get.c \
	zip_error_strerror.c zip_unchange.c zip_error_get_sys_type.c zip_source_file.c zip_source_filep.c \
	zip_get_name.c zip_set_name.c zip_file_strerror.c zip_fopen.c \
	zip_dirent.c zip_delete.c zip_error_to_str.c zip_strerror.c zip_error_get.c \
	zip_source_filename.c zip_source_function.c zip_entry_free.c  zip_stat_index.c \
	zip_recompress.c zip_get_path.c zip_replace.c zip_fclose.c \
	zip_set_attributes.c zip_get_attributes.c)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

include $(BUILD_STATIC_LIBRARY)

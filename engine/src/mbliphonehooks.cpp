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

#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>

// MM-2014-10-07: [[ Bug 13583 ]] Due to complications with getting the trampolines to work, we now use symlinks when using the iOS 8 sim.
#if defined(__i386__) && !defined (__IPHONE_8_0)

////////////////////////////////////////////////////////////////////////////////

extern MCStringRef MCcmd;

////////////////////////////////////////////////////////////////////////////////

struct redirect_t
{
	char *src;
	char *dst;
};

static uint32_t s_redirect_count = 0;
static redirect_t *s_redirects = nil;
static char *s_redirect_base = nil;
static char *s_redirect_cwd = nil;

void add_simulator_redirect(const char *p_redirect_def)
{
	const char *t_dst_offset;
	t_dst_offset = strstr(p_redirect_def, "//");
	if (t_dst_offset == nil)
		return;
	
	if (!MCMemoryResizeArray(s_redirect_count + 1, s_redirects, s_redirect_count))
		return;
	
	if (s_redirect_base == nil)
    {
        MCAutoStringRef t_substring;
        uindex_t t_index;
        t_index = MCStringGetLength(MCcmd);
        /* UNCHECKED */ MCStringLastIndexOfChar(MCcmd, '/', t_index, kMCCompareExact, t_index);
        /* UNCHECKED */ MCStringCopySubstring(MCcmd, MCRangeMake(0, t_index), &t_substring);
        /* UNCHECKED */ MCStringConvertToCString(*t_substring, s_redirect_base);
    }

	
	MCCStringCloneSubstring(p_redirect_def, t_dst_offset - p_redirect_def, s_redirects[s_redirect_count - 1] . src);
	
	// MW-2012-10-30: [[ Bug 10495 ]] Redirects are given to us in MacRoman, so convert to
	//   UTF-8 (which the FS expects).
	MCCStringFromNative(t_dst_offset + 2, s_redirects[s_redirect_count - 1] . dst);
}

static void compute_simulator_redirect(const char *p_input, char*& r_output)
{
	if (s_redirects == nil)
	{
		r_output = (char *)p_input;
		return;
	}

	char *t_resolved_input;
	t_resolved_input = nil;
	if (*p_input != '/')
	{
		char *t_cwd;
		t_cwd = getcwd(nil, 0);
		MCCStringFormat(t_resolved_input, "%s/%s", t_cwd, p_input);
		free(t_cwd);
	}
	else
		MCCStringClone(p_input, t_resolved_input);
	
	char **t_components;
	uint32_t t_component_count;
	MCCStringSplit(t_resolved_input, '/', t_components, t_component_count);
	MCCStringFree(t_resolved_input);
	
	uint32_t t_count;
	t_count = 1;
	for(uint32_t j = 1; j < t_component_count; j++)
	{
		if (MCCStringEqual(t_components[j], ".") ||
			MCCStringEqual(t_components[j], ""))
		{
			MCCStringFree(t_components[j]);
			continue;
		}
		
		if (MCCStringEqual(t_components[j], ".."))
		{
			MCCStringFree(t_components[j]);
			if (t_count > 1)
			{
				MCCStringFree(t_components[t_count - 1]);
				t_count -= 1;
			}
			continue;
		}
		
		t_components[t_count] = t_components[j];
		t_count += 1;
	}
	
	MCCStringCombine(t_components, t_count, '/', t_resolved_input);
	MCCStringArrayFree(t_components, t_count);
	
	r_output = t_resolved_input;
	
	if (MCCStringBeginsWith(t_resolved_input, s_redirect_base))
	{
		const char *t_input_leaf;
		t_input_leaf = t_resolved_input + strlen(s_redirect_base) + 1;
		for(uint32_t i = 0; i < s_redirect_count; i++)
		{
			if (MCCStringEqual(s_redirects[i] . src, t_input_leaf))
			{
				r_output = strdup(s_redirects[i] . dst);
				break;
			}
			
			if (MCCStringBeginsWith(t_input_leaf, s_redirects[i] . src) &&
				t_input_leaf[MCCStringLength(s_redirects[i] . src)] == '/')
			{
				MCCStringFormat(r_output, "%s%s", s_redirects[i] . dst, t_input_leaf + MCCStringLength(s_redirects[i] . src));
				break;
			}
		}
	
	}
	
	if (r_output != t_resolved_input)
		MCCStringFree(t_resolved_input);
}

////////////////////////////////////////////////////////////////////////////////

typedef FILE* (*fopen_ptr_t)(const char *, const char *);
static fopen_ptr_t fopen_trampoline = nil;
static FILE* fopen_wrapper(const char *path, const char *mode)
{
	char *t_resolved_path;
	compute_simulator_redirect(path, t_resolved_path);
	
	FILE *t_result;
	t_result = fopen_trampoline(t_resolved_path, mode);
	
	if (t_resolved_path != path)
		free(t_resolved_path);
	
	return t_result;
}

typedef int (*open_ptr_t)(const char *, int, ...);
static open_ptr_t open_trampoline = nil;
static int open_wrapper(const char *path, int oflag, mode_t mode)
{
	char *t_resolved_path;
	compute_simulator_redirect(path, t_resolved_path);
	
	int t_result;
	if ((oflag & O_CREAT) != 0)
		t_result = open_trampoline(t_resolved_path, oflag, mode);
	else
		t_result = open_trampoline(t_resolved_path, oflag);
	
	if (t_resolved_path != path)
		free(t_resolved_path);
	
	return t_result;
}

//////////

typedef int (*stat_ptr_t)(const char *, struct stat *);
static stat_ptr_t stat_trampoline = nil;
static int stat_wrapper(const char *path, struct stat *buf)
{
	char *t_resolved_path;
	compute_simulator_redirect(path, t_resolved_path);
	
	int t_result;
	t_result = stat_trampoline(t_resolved_path, buf);
	
	if (t_resolved_path != path)
		free(t_resolved_path);
	
	return t_result;
}

typedef int (*lstat_ptr_t)(const char *, struct stat *);
static stat_ptr_t lstat_trampoline = nil;
static int lstat_wrapper(const char *path, struct stat *buf)
{
	char *t_resolved_path;
	compute_simulator_redirect(path, t_resolved_path);
	
	int t_result;
	t_result = lstat_trampoline(t_resolved_path, buf);
	
	if (t_resolved_path != path)
		free(t_resolved_path);
	
	return t_result;
}

//////////

struct DIR_wrapper
{
	int fd;
	DIR *sys_dir;
	uint32_t index;
	uint32_t entry_count;
	char **entries;
	struct dirent entry;
};

typedef DIR* (*opendir_ptr_t)(const char *);
static opendir_ptr_t opendir_trampoline = nil;
static DIR* opendir_wrapper(const char *dirname)
{
	char *t_resolved_path;
	compute_simulator_redirect(dirname, t_resolved_path);
	
	if (s_redirect_base != nil && MCCStringEqual(t_resolved_path, s_redirect_base))
	{
		DIR *t_sys_dir;
		t_sys_dir = opendir_trampoline(t_resolved_path);
		if (t_sys_dir == nil)
			return nil;
		
		DIR_wrapper *t_wrapper;
		t_wrapper = (DIR_wrapper *)malloc(sizeof(DIR_wrapper));
		t_wrapper -> fd = -t_sys_dir -> __dd_fd;
		t_wrapper -> sys_dir = t_sys_dir;
		t_wrapper -> entry_count = 0;
		t_wrapper -> entries = nil;
		t_wrapper -> index = 0;
		
		for(;;)
		{
			struct dirent *t_dir_entry;
			t_dir_entry = readdir(t_sys_dir);
			if (t_dir_entry == nil)
				break;
			
			MCMemoryResizeArray(t_wrapper -> entry_count + 1, t_wrapper -> entries, t_wrapper -> entry_count);
			MCCStringClone(t_dir_entry -> d_name, t_wrapper -> entries[t_wrapper -> entry_count - 1]);
		}
		
		for(uint32_t i = 0; i < s_redirect_count; i++)
		{
			const char *t_src, *t_src_end;
			t_src = s_redirects[i] . src;
			t_src_end = strchr(t_src, '/');
			if (t_src_end == nil)
				t_src_end = t_src + strlen(t_src);
			
			bool t_found;
			t_found = false;
			for(uint32_t j = 0; j < t_wrapper -> entry_count; j++)
				if (strlen(t_wrapper -> entries[j]) == (t_src_end - t_src) &&
					strncmp(t_wrapper -> entries[j], t_src, t_src_end - t_src) == 0)
				{
					t_found = true;
					break;
				}
			
			if (t_found)
				continue;
			
			MCMemoryResizeArray(t_wrapper -> entry_count + 1, t_wrapper -> entries, t_wrapper -> entry_count);
			MCCStringCloneSubstring(t_src, t_src_end - t_src, t_wrapper -> entries[t_wrapper -> entry_count - 1]);
		}

		return (DIR *)t_wrapper;
	}
	
	DIR *t_result;
	t_result = opendir_trampoline(t_resolved_path);
	
	if (t_resolved_path != dirname)
		free(t_resolved_path);
	
	return t_result;
}

typedef struct dirent* (*readdir_ptr_t)(DIR *);
static readdir_ptr_t readdir_trampoline = nil;
static struct dirent *readdir_wrapper(DIR *dirp)
{
	if (dirp -> __dd_fd < 0)
	{
		DIR_wrapper *t_wrapper;
		t_wrapper = (DIR_wrapper *)dirp;
		
		struct dirent *t_result;
		if (readdir_r(dirp, &t_wrapper -> entry, &t_result) != 0)
			return nil;
		
		return t_result;
	}
	
	return readdir_trampoline(dirp);
}

typedef int (*readdir_r_ptr_t)(DIR *, struct dirent *, struct dirent **);
static readdir_r_ptr_t readdir_r_trampoline = nil;
static int readdir_r_wrapper(DIR *dirp, struct dirent *entry, struct dirent **result)
{
	if (dirp -> __dd_fd < 0)
	{
		DIR_wrapper *t_wrapper;
		t_wrapper = (DIR_wrapper *)dirp;
		
		if (t_wrapper -> index == t_wrapper -> entry_count)
		{
			*result = nil;
			return 0;
		}
		
		char *t_item_path;
		MCCStringFormat(t_item_path, "%s/%s", s_redirect_base, t_wrapper -> entries[t_wrapper -> index]);
		struct stat t_stat;
		lstat(t_item_path, &t_stat);
		MCCStringFree(t_item_path);
		
		entry -> d_fileno = t_stat . st_ino;
		if (S_ISBLK(t_stat . st_mode))
			entry -> d_type = DT_BLK;
		else if (S_ISCHR(t_stat . st_mode))
			entry -> d_type = DT_CHR;
		else if (S_ISDIR(t_stat . st_mode))
			entry -> d_type = DT_DIR;
		else if (S_ISFIFO(t_stat . st_mode))
			entry -> d_type = DT_FIFO;
		else if (S_ISREG(t_stat . st_mode))
			entry -> d_type = DT_REG;
		else if (S_ISLNK(t_stat . st_mode))
			entry -> d_type = DT_LNK;
		else if (S_ISSOCK(t_stat . st_mode))
			entry -> d_type = DT_SOCK;
		else if (S_ISWHT(t_stat . st_mode))
			entry -> d_type = DT_WHT;
		else
			entry -> d_type = DT_UNKNOWN;
		
		strcpy(entry -> d_name, t_wrapper -> entries[t_wrapper -> index]);
		
		entry -> d_reclen = sizeof(struct dirent) - sizeof(entry -> d_name) + ((strlen(entry -> d_name) + 4) & ~3);
		
		t_wrapper -> index += 1;
		
		*result = entry;
		
		return 0;
	}
	
	return readdir_r_trampoline(dirp, entry, result);
}

typedef int (*closedir_ptr_t)(DIR *);
static closedir_ptr_t closedir_trampoline = nil;
static int closedir_wrapper(DIR *dirp)
{
	if (dirp -> __dd_fd < 0)
	{
		DIR_wrapper *t_wrapper;
		t_wrapper = (DIR_wrapper *)dirp;
		
		int t_result;
		t_result = closedir(t_wrapper -> sys_dir);
		
		MCCStringArrayFree(t_wrapper -> entries, t_wrapper -> entry_count);
		free(t_wrapper);
		
		return t_result;
	}
	
	return closedir_trampoline(dirp);
}

typedef void (*rewinddir_ptr_t)(DIR *);
static rewinddir_ptr_t rewinddir_trampoline = nil;
static void rewinddir_wrapper(DIR *dirp)
{
	if (dirp -> __dd_fd < 0)
	{
		DIR_wrapper *t_wrapper;
		t_wrapper = (DIR_wrapper *)dirp;
		t_wrapper -> index = 0;
		return;
	}
	
	rewinddir_trampoline(dirp);
}

typedef void (*seekdir_ptr_t)(DIR *, long loc);
static seekdir_ptr_t seekdir_trampoline = nil;
static void seekdir_wrapper(DIR *dirp, long loc)
{
	if (dirp -> __dd_fd < 0)
	{
		DIR_wrapper *t_wrapper;
		t_wrapper = (DIR_wrapper *)dirp;
		t_wrapper -> index = MCU_max(MCU_min(loc, (signed)t_wrapper -> entry_count), 0);
		return;
	}
	seekdir_trampoline(dirp, loc);
}

typedef long (*telldir_ptr_t)(DIR *);
static telldir_ptr_t telldir_trampoline = nil;
static long telldir_wrapper(DIR *dirp)
{
	if (dirp -> __dd_fd < 0)
	{
		DIR_wrapper *t_wrapper;
		t_wrapper = (DIR_wrapper *)dirp;
		return t_wrapper -> index;
	}
	
	return telldir_trampoline(dirp);
}

//////////

typedef char *(*getcwd_ptr_t)(char *buf, size_t size);
static getcwd_ptr_t getcwd_trampoline = nil;
static char *getcwd_wrapper(char *buf, size_t size)
{
	if (s_redirect_cwd != nil)
	{
		if (buf == nil)
			return strdup(s_redirect_cwd);
		
		if (strlen(s_redirect_cwd) >= size)
		{
			*buf = 0;
			errno = ERANGE;
			return NULL;
		}
		
		strcpy(buf, s_redirect_cwd);
		return buf;
	}
	
	return getcwd_trampoline(buf, size);
}

typedef int (*chdir_ptr_t)(const char *path);
static chdir_ptr_t chdir_trampoline = nil;
static int chdir_wrapper(const char *path)
{
	char *t_resolved_path;
	compute_simulator_redirect(path, t_resolved_path);
	
	int t_result;
	t_result = chdir_trampoline(t_resolved_path);
	
	if (t_result == 0)
	{
		free(s_redirect_cwd);
		s_redirect_cwd = nil;
		
		if (t_resolved_path != path)
			s_redirect_cwd = strdup(t_resolved_path);
	}
	
	if (t_resolved_path != path)
		free(t_resolved_path);
	
	return t_result;
}

////////////////////////////////////////////////////////////////////////////////

// Our patching mechanism is very simple - we simply replace the first 5 bytes
// at the target function address with a jump to the wrapper. In order to allow
// us to call the original, though, we construct a trampoline that consists of
// the overwritten instructions, followed by a jump back to where they originally
// ended.

// Different versions of iOS have slightly different instructions at the start
// of each function we wrap. The best way to determine the instructions is to
// use the gdb immediate window and use 'x/16i <function>' to disassemble the
// instructions; and then 'x/16b <function>' to get the instruction's bytes.
// Note that we can't just use the first five bytes at each function address
// since we need to make sure we end the trampoline on an instruction boundary.

struct trampoline_info_t
{
    // The function that we are wrapping.
	void *target;
    // The function to call instead of the original.
	void *wrapper;
    // The 'trampoline' that the wrapper calls to defer to the original.
	void **trampoline;
    // The instructions our patch overwrites with a jump to wrapper.
	uint8_t *prefix;
};

// MM-2014-09-26: [[ iOS 8 Support ]] Added prefix for jmpl instruction, used by open, stat and lstat.
#ifdef __IPHONE_8_0
static uint8_t s_jmpl_prefix[] =
{
    6,
    0xff,
};
#endif

#ifdef __IPHONE_6_0

// 0x37b080b <opendir>:     0x55            push   %ebp
// 0x37b080c <opendir+1>:	0x89 0xe5       mov    %esp,%ebp
// 0x37b080e <opendir+3>:	0x83 0xec 0x08  sub    $0x8,%esp
static uint8_t s_push_mov_sub_8_prefix[] =
{
    6,
    0x55,
    0x89, 0xe5,
    0x83, 0xec, 0x08,
};

// readdir / readdir_r / closedir / seekdir
// 0x37b1334 <readdir>:     0x55        push   %ebp
// 0x37b1335 <readdir+1>:	0x89 0xe5   mov    %esp,%ebp
// 0x37b1337 <readdir+3>:	0x57        push   %edi
// 0x37b1338 <readdir+4>:	0x56        push   %esi
static uint8_t s_push_mov_push_push_prefix[] =
{
    5,
    0x55,
    0x89, 0xe5,
    0x57,
    0x56
};

// telldir
// 0x37b323a <telldir>:	push   %ebp
// 0x37b323b <telldir+1>:	mov    %esp,%ebp
// 0x37b323d <telldir+3>:	push   %ebx
// 0x37b323e <telldir+4>:	push   %edi
static uint8_t s_push_mov_push_ebx_push_edi_prefix[] =
{
    5,
    0x55,
    0x89, 0xe5,
    0x53,
    0x57
};



// rewinddir
// 0x37b1a4b <rewinddir>:	0x55            push   %ebp
// 0x37b1a4c <rewinddir+1>:	0x89 0xe5       mov    %esp,%ebp
// 0x37b1a4e <rewinddir+3>:	0x56            push   %esi
// 0x37b1a4f <rewinddir+4>:	0x83 0xec 0x14  sub    $0x14,%esp
static uint8_t s_push_mov_push_sub_14_prefix[] =
{
    7,
    0x55,
    0x89, 0xe5,
    0x56,
    0x83, 0xec, 0x14
};

// 

#endif

#ifdef __IPHONE_5_0

// open / stat / opendir / readdir / readdir_r / closedir / rewinddir / seekdir / telldir:
//   0x219b5fc 0x55                 <open>:	push   %ebp
//   0x219b5fd 0x89 0xe5            <open+1>:	mov    %esp,%ebp
//   0x219b5ff 0x83 0xe8 0x14       <open+3>:	sub    $0x14,%esp
static uint8_t s_push_move_sub_14_prefix[] =
{
	6,
	0x55,
	0x89, 0xe5,
	0x83, 0xec, 0x14,
};

#else

// open / stat / opendir / readdir / readdir_r / closedir / rewinddir / seekdir / telldir:
//   0x01b15130 0x55						<open+0>:	push   %ebp
//   0x01b15131 0x89 0xe5					<open+1>:	mov    %esp,%ebp
//   0x01b15133 0x57						<open+3>:	push   %edi
//   0x01b15134 0x83 0xec 0x14				<open+4>:	sub    $0x14,%esp
//   0x01b15137 0xe8 0x00 0x00 0x00 0x00	<open+7>:	call   0x1b1513c <open+12>
//   0x01b1513c 0x5a						<open+12>:	pop    %edx
static uint8_t s_push_mov_push_sub_14_prefix[] =
{
	7,
	0x55,
	0x89, 0xe5,
	0x57,
	0x83, 0xec, 0x14,
};

#endif

// getcwd:
//   0x967f69b7 <getcwd+0>:	push   %ebp
//   0x967f69b8 <getcwd+1>:	mov    %esp,%ebp
//   0x967f69ba <getcwd+3>:	sub    $0x18,%esp
static uint8_t s_push_mov_sub_18_prefix[] =
{
	6,
	0x55,
	0x89, 0xe5,
	0x83, 0xec, 0x18,
};

// chdir:
//   0x96836014 0xb8 0x0c 0x00 0x08 0x00	<chdir+0>: mov    $0x4000c,%eax
static uint8_t s_mov_4000c_prefix[] =
{
	5,
	0xb8, 0x0c, 0x00, 0x08, 0x00
};

static trampoline_info_t s_trampolines[] =
{
#if defined (__IPHONE_8_0)
    // MM-2014-09-26: [[ iOS 8 Support ]] Updated trampolines for iOS 8.
    { (void *)open, (void *)open_wrapper, (void **)&open_trampoline, s_jmpl_prefix },
    { (void *)fopen, (void *)fopen_wrapper, (void **)&fopen_trampoline, s_push_mov_push_ebx_push_edi_prefix },
    
    { (void *)stat, (void *)stat_wrapper, (void **)&stat_trampoline, s_jmpl_prefix },
    { (void *)lstat, (void *)lstat_wrapper, (void **)&lstat_trampoline, s_jmpl_prefix },
    
    { (void *)opendir, (void *)opendir_wrapper, (void **)&opendir_trampoline, s_push_mov_sub_8_prefix },
    { (void *)readdir, (void *)readdir_wrapper, (void **)&readdir_trampoline, s_push_mov_push_push_prefix },
    { (void *)readdir_r, (void *)readdir_r_wrapper, (void **)&readdir_r_trampoline, s_push_mov_push_ebx_push_edi_prefix },
    { (void *)closedir, (void *)closedir_wrapper, (void **)&closedir_trampoline, s_push_mov_push_ebx_push_edi_prefix },
    { (void *)rewinddir, (void *)rewinddir_wrapper, (void **)&rewinddir_trampoline, s_push_mov_push_sub_14_prefix },
    { (void *)seekdir, (void *)seekdir_wrapper, (void **)&seekdir_trampoline, s_push_mov_push_ebx_push_edi_prefix },
    { (void *)telldir, (void *)telldir_wrapper, (void **)&telldir_trampoline, s_push_mov_push_ebx_push_edi_prefix },
#elif defined(__IPHONE_7_0)
	{ (void *)open, (void *)open_wrapper, (void **)&open_trampoline, s_push_mov_sub_18_prefix },
    // MM-2013-09-23: [[ iOS7 Support ]] Tweaked fopen for iOS7.
	{ (void *)fopen, (void *)fopen_wrapper, (void **)&fopen_trampoline, s_push_mov_push_ebx_push_edi_prefix },
	
	{ (void *)stat, (void *)stat_wrapper, (void **)&stat_trampoline, s_push_mov_sub_18_prefix },
	{ (void *)lstat, (void *)lstat_wrapper, (void **)&lstat_trampoline, s_push_mov_sub_18_prefix },
	
	{ (void *)opendir, (void *)opendir_wrapper, (void **)&opendir_trampoline, s_push_mov_sub_8_prefix },
	{ (void *)readdir, (void *)readdir_wrapper, (void **)&readdir_trampoline, s_push_mov_push_push_prefix },
    
    // MM-2013-10-04: [[ Bug ]] Changed readdir_r from s_push_mov_push_push_prefix to s_push_mov_push_ebx_push_edi_prefix.
    //    This fix along with 11234 will hopefully fix bug 11252 (which looks to be simulator hook related).
	{ (void *)readdir_r, (void *)readdir_r_wrapper, (void **)&readdir_r_trampoline, s_push_mov_push_ebx_push_edi_prefix },
    
    // MM-2013-10-04: [[ Bug 11234 ]] Changed closedir from s_push_mov_push_push_prefix to s_push_mov_push_ebx_push_edi_prefix.
    //   This was causing ask/answer commands to hang.
	{ (void *)closedir, (void *)closedir_wrapper, (void **)&closedir_trampoline, s_push_mov_push_ebx_push_edi_prefix },
    
	{ (void *)rewinddir, (void *)rewinddir_wrapper, (void **)&rewinddir_trampoline, s_push_mov_push_sub_14_prefix },
	{ (void *)seekdir, (void *)seekdir_wrapper, (void **)&seekdir_trampoline, s_push_mov_push_push_prefix },
	{ (void *)telldir, (void *)telldir_wrapper, (void **)&telldir_trampoline, s_push_mov_push_ebx_push_edi_prefix },
#elif defined(__IPHONE_6_0)
	{ (void *)open, (void *)open_wrapper, (void **)&open_trampoline, s_push_mov_sub_18_prefix },
	{ (void *)fopen, (void *)fopen_wrapper, (void **)&fopen_trampoline, s_push_mov_sub_18_prefix },
	
	{ (void *)stat, (void *)stat_wrapper, (void **)&stat_trampoline, s_push_mov_sub_18_prefix },
	{ (void *)lstat, (void *)lstat_wrapper, (void **)&lstat_trampoline, s_push_mov_sub_18_prefix },
	
	{ (void *)opendir, (void *)opendir_wrapper, (void **)&opendir_trampoline, s_push_mov_sub_8_prefix },
	{ (void *)readdir, (void *)readdir_wrapper, (void **)&readdir_trampoline, s_push_mov_push_push_prefix },
    
    // MM-2013-10-04: [[ Bug ]] Changed readdir_r from s_push_mov_push_push_prefix to s_push_mov_push_ebx_push_edi_prefix.
    //   The issue, though,  doesn't appear to have manifested itself as a bug in LiveCode.
	{ (void *)readdir_r, (void *)readdir_r_wrapper, (void **)&readdir_r_trampoline, s_push_mov_push_ebx_push_edi_prefix },
    
    // MM-2013-10-04: [[ Bug 10888 ]] Changed closedir from s_push_mov_push_push_prefix to s_push_mov_push_ebx_push_edi_prefix.
    //   This was causing video streaming to crash.
	{ (void *)closedir, (void *)closedir_wrapper, (void **)&closedir_trampoline, s_push_mov_push_ebx_push_edi_prefix },
    
	{ (void *)rewinddir, (void *)rewinddir_wrapper, (void **)&rewinddir_trampoline, s_push_mov_push_sub_14_prefix },
    
    // MM-2013-10-04: [[ Bug ]] Changed seekdir from s_push_mov_push_push_prefix to s_push_mov_push_ebx_push_edi_prefix.
    //   The issue, though,  doesn't appear to have manifested itself as a bug in LiveCode.
	{ (void *)seekdir, (void *)seekdir_wrapper, (void **)&seekdir_trampoline, s_push_mov_push_ebx_push_edi_prefix },
    
	{ (void *)telldir, (void *)telldir_wrapper, (void **)&telldir_trampoline, s_push_mov_push_ebx_push_edi_prefix },
#elif defined(__IPHONE_5_0)
	{ (void *)open, (void *)open_wrapper, (void **)&open_trampoline, s_push_move_sub_14_prefix },
	{ (void *)fopen, (void *)fopen_wrapper, (void **)&fopen_trampoline, s_push_move_sub_14_prefix },
	
	{ (void *)stat, (void *)stat_wrapper, (void **)&stat_trampoline, s_push_move_sub_14_prefix },
	{ (void *)lstat, (void *)lstat_wrapper, (void **)&lstat_trampoline, s_push_move_sub_14_prefix },
	
	{ (void *)opendir, (void *)opendir_wrapper, (void **)&opendir_trampoline, s_push_move_sub_14_prefix },
	{ (void *)readdir, (void *)readdir_wrapper, (void **)&readdir_trampoline, s_push_move_sub_14_prefix },
	{ (void *)readdir_r, (void *)readdir_r_wrapper, (void **)&readdir_r_trampoline, s_push_move_sub_14_prefix },
	{ (void *)closedir, (void *)closedir_wrapper, (void **)&closedir_trampoline, s_push_move_sub_14_prefix },
	{ (void *)rewinddir, (void *)rewinddir_wrapper, (void **)&rewinddir_trampoline, s_push_move_sub_14_prefix },
	{ (void *)seekdir, (void *)seekdir_wrapper, (void **)&seekdir_trampoline, s_push_move_sub_14_prefix },
	{ (void *)telldir, (void *)telldir_wrapper, (void **)&telldir_trampoline, s_push_move_sub_14_prefix },
#else
	{ (void *)open, (void *)open_wrapper, (void **)&open_trampoline, s_push_mov_push_sub_14_prefix },
	{ (void *)fopen, (void *)fopen_wrapper, (void **)&fopen_trampoline, s_push_mov_push_sub_14_prefix },
	
	{ (void *)stat, (void *)stat_wrapper, (void **)&stat_trampoline, s_push_mov_push_sub_14_prefix },
	{ (void *)lstat, (void *)lstat_wrapper, (void **)&lstat_trampoline, s_push_mov_push_sub_14_prefix },
	
	{ (void *)opendir, (void *)opendir_wrapper, (void **)&opendir_trampoline, s_push_mov_push_sub_14_prefix },
	{ (void *)readdir, (void *)readdir_wrapper, (void **)&readdir_trampoline, s_push_mov_push_sub_14_prefix },
	{ (void *)readdir_r, (void *)readdir_r_wrapper, (void **)&readdir_r_trampoline, s_push_mov_push_sub_14_prefix },
	{ (void *)closedir, (void *)closedir_wrapper, (void **)&closedir_trampoline, s_push_mov_push_sub_14_prefix },
	{ (void *)rewinddir, (void *)rewinddir_wrapper, (void **)&rewinddir_trampoline, s_push_mov_push_sub_14_prefix },
	{ (void *)seekdir, (void *)seekdir_wrapper, (void **)&seekdir_trampoline, s_push_mov_push_sub_14_prefix },
	{ (void *)telldir, (void *)telldir_wrapper, (void **)&telldir_trampoline, s_push_mov_push_sub_14_prefix },
#endif
    
    // These are implemented as direct traps into syscalls so aren't found (as functions) in libSystem.
	{ (void *)getcwd, (void *)getcwd_wrapper, (void **)&getcwd_trampoline, s_push_mov_sub_18_prefix },
	{ (void *)chdir, (void *)chdir_wrapper, (void **)&chdir_trampoline, s_mov_4000c_prefix },
};

////////////////////////////////////////////////////////////////////////////////

static void push_bytes(uint8_t*& x_buffer, uint32_t p_count, ...)
{
	va_list t_args;
	va_start(t_args, p_count);
	for(uint32_t i = 0; i < p_count; i++)
		*x_buffer++ = (uint8_t)va_arg(t_args, int);
	va_end(t_args);
}

static void push_ints(uint8_t*& x_buffer, uint32_t p_count, ...)
{
	va_list t_args;
	va_start(t_args, p_count);
	for(uint32_t i = 0; i < p_count; i++)
	{
		*(int *)x_buffer = va_arg(t_args, int);
		x_buffer += 4;
	}
	va_end(t_args);
}

void setup_simulator_hooks(void)
{
    // MW-2011-10-07: On Lion, you cannot execute pages which have not been
    //   explicitly marked as EXEC, so allocate a 4K page, and then protect
    //   it appropriately.
    uint8_t *t_trampolines;
    posix_memalign((void **)&t_trampolines, 4096, 4096);
    mprotect(t_trampolines, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);

    for(uint32_t i = 0; i < sizeof(s_trampolines) / sizeof(trampoline_info_t); i++)
    {
        *(s_trampolines[i] . trampoline) = t_trampolines;
        
        uint8_t *t_target;
        t_target = (uint8_t *)s_trampolines[i] . target;
        
        // MM-2014-09-26: [[ iOS 8 Support ]] If the first byte of the prefix is 255 then we want to copy the given number of bytes from the target address.
        //  Allows us to support the jmpl instruction which has a dynamic jump address.
        if (s_trampolines[i] . prefix[1] == 0xff)
            memcpy(t_trampolines, t_target, s_trampolines[i] . prefix[0]);
        else
            memcpy(t_trampolines, s_trampolines[i] . prefix + 1, s_trampolines[i] . prefix[0]);
        t_trampolines += s_trampolines[i] . prefix[0];
        
        mprotect((void *)((uintptr_t)t_target & ~4095), 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
        push_bytes(t_target, 1, 0xE9);
        push_ints(t_target, 1, (uint8_t *)(s_trampolines[i] . wrapper) - (t_target + 4));
        mprotect((void *)((uintptr_t)t_target & ~4095), 4096, PROT_READ | PROT_EXEC);
        
        push_bytes(t_trampolines, 1, 0xe9);
        push_ints(t_trampolines, 1, ((uint8_t *)(s_trampolines[i] . target) + s_trampolines[i] . prefix[0]) - (t_trampolines + 4));
    }
}

////////////////////////////////////////////////////////////////////////////////

#else

void setup_simulator_hooks(void)
{
}

void add_simulator_redirect(const char *p_redirect_def)
{
}

#endif


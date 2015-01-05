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

#include <foundation.h>
#include <foundation-auto.h>

#include "foundation-private.h"
#include "foundation-file.h"

#ifdef _MACOSX
#include <pwd.h>
#include <sys/stat.h>
#endif

////////////////////////////////////////////////////////////////////////////////

#if defined(_WINDOWS)
#define IO_APPEND_MODE "ab"
#define IO_READ_MODE "rb"
#define IO_WRITE_MODE "wb"
#define IO_UPDATE_MODE "r+b"
#define ENV_SEPARATOR ';'
#elif defined(_MACOSX)
#define IO_APPEND_MODE 	"ab"
#define IO_READ_MODE	"rb"
#define IO_WRITE_MODE	"wb"
#define IO_UPDATE_MODE 	"r+b"
#define IO_CREATE_MODE "wb+"
#define ENV_SEPARATOR 	':'
#elif defined(_LINUX) || defined(_SERVER) || defined(_MOBILE)
#define IO_APPEND_MODE "a"
#define IO_READ_MODE "r"
#define IO_WRITE_MODE "w"
#define IO_UPDATE_MODE "r+"
#define IO_CREATE_MODE "w+"
#define ENV_SEPARATOR ':'
#endif

struct __MCFileStream
{
    MCOpenFileMode m_mode;
    FILE *m_stream;
    
    void GetAvailableForRead()
    {
    }
    
    void GetAvailableForWrite()
    {
    }
    
    void Destroy()
    {
        fclose(m_stream);
    }
    
    bool Read(void *p_buffer, size_t p_amount)
    {
        size_t nread, toread;
        toread = p_amount;
        
        byte_t t_buffer[p_amount];
        size_t t_total_read = 0;
        
        while ((nread = fread(&t_buffer[t_total_read], 1, toread, m_stream)) != toread)
        {
            t_total_read += nread;
            
            if (feof(m_stream) && t_total_read != p_amount)
                return false;
            
            if (ferror(m_stream))
            {
                clearerr(m_stream);
                
                if (errno == EAGAIN)
                    return false;
                
                if (errno == EINTR)
                {
                    toread -= nread;
                    continue;
                }
                return false;
            }
            return false;
        }
        
        MCMemoryCopy(p_buffer, t_buffer, p_amount);
        return true;
    }
    
    bool Write(const void *p_data, size_t p_length)
    {
        return fwrite(p_data, 1, p_length, m_stream) == p_length;
    }
    
    bool IsFinished(bool& r_finished)
    {
        r_finished = feof(m_stream);
        
        return true;
    }
    
    bool Seek(filepos_t p_position)
    {
        return fseeko(m_stream, p_position, SEEK_SET) == 0;
    }
    
    bool Tell(filepos_t& r_position)
    {
        r_position = ftello(m_stream);
        return true;
    }
};

static bool __MCFileStreamGetAvailableForRead(MCStreamRef p_stream, size_t& r_amount)
{
    return false;
}

static bool __MCFileStreamGetAvailableForWrite(MCStreamRef p_stream, size_t& r_amount)
{
    return false;
}

static void __MCFileStreamDestroy(MCStreamRef p_stream)
{
    __MCFileStream *self;
    self = (__MCFileStream *)MCStreamGetExtraBytesPtr(p_stream);
    
    return self -> Destroy();
}

static bool __MCFileStreamRead(MCStreamRef p_stream, void *p_buffer, size_t p_amount)
{
    __MCFileStream *self;
    self = (__MCFileStream *)MCStreamGetExtraBytesPtr(p_stream);
    
    return self -> Write(p_buffer, p_amount);
}

static bool __MCFileStreamWrite(MCStreamRef p_stream, const void *p_data, size_t p_length)
{
    __MCFileStream *self;
    self = (__MCFileStream *)MCStreamGetExtraBytesPtr(p_stream);
    
    return self -> Write(p_data, p_length);
}

static bool __MCFileStreamIsFinished(MCStreamRef p_stream, bool& r_finished)
{
    __MCFileStream *self;
    self = (__MCFileStream *)MCStreamGetExtraBytesPtr(p_stream);
    
    return self -> IsFinished(r_finished);
}

static bool __MCFileStreamSeek(MCStreamRef p_stream, filepos_t p_position)
{
    __MCFileStream *self;
    self = (__MCFileStream *)MCStreamGetExtraBytesPtr(p_stream);
    
    return self -> Seek(p_position);
}

static bool __MCFileStreamTell(MCStreamRef p_stream, filepos_t& r_position)
{
    __MCFileStream *self;
    self = (__MCFileStream *)MCStreamGetExtraBytesPtr(p_stream);
    
    return self -> Tell(r_position);
}

static MCStreamCallbacks kMCFileReadableStreamCallbacks =
{
    __MCFileStreamDestroy,
    __MCFileStreamIsFinished,
    __MCFileStreamGetAvailableForRead,
    __MCFileStreamRead,
    nil,
    nil,
    nil,
    nil,
    nil,
    __MCFileStreamTell,
    __MCFileStreamSeek,
};

static MCStreamCallbacks kMCFileWritableStreamCallbacks =
{
    __MCFileStreamDestroy,
    __MCFileStreamIsFinished,
    nil,
    nil,
    __MCFileStreamGetAvailableForWrite,
    __MCFileStreamWrite,
    nil,
    nil,
    nil,
    __MCFileStreamTell,
    __MCFileStreamSeek,
};

static MCStreamCallbacks kMCFileReadWritableStreamCallbacks =
{
    __MCFileStreamDestroy,
    __MCFileStreamIsFinished,
    __MCFileStreamGetAvailableForRead,
    __MCFileStreamRead,
    __MCFileStreamGetAvailableForWrite,
    __MCFileStreamWrite,
    nil,
    nil,
    nil,
    __MCFileStreamTell,
    __MCFileStreamSeek,
};

bool MCFileCreateStreamForFile(MCStringRef p_filename, MCOpenFileMode p_mode, MCStreamRef& r_stream)
{

#ifdef _LINUX
    MCAutoStringRefAsSysString t_path_sys;
#else
    MCAutoStringRefAsUTF8String t_path_sys;
#endif
    if (!t_path_sys.Lock(p_filename))
        return false;
    
    FILE *t_fptr;
    const char *t_mode;
    MCStreamRef t_stream;
    
    switch (p_mode)
    {
        case kMCOpenFileModeRead:
            t_mode = IO_READ_MODE;
            if (!MCStreamCreate(&kMCFileReadableStreamCallbacks, sizeof(__MCFileStream), t_stream))
                return false;
            break;
        case kMCOpenFileModeWrite:
            t_mode = IO_READ_MODE;
            if (!MCStreamCreate(&kMCFileWritableStreamCallbacks, sizeof(__MCFileStream), t_stream))
                return false;
            break;
        case kMCOpenFileModeUpdate:
        default:
            t_mode = IO_READ_MODE;
            if (!MCStreamCreate(&kMCFileReadWritableStreamCallbacks, sizeof(__MCFileStream), t_stream))
                return false;
            break;
    }
    
    t_fptr = fopen(*t_path_sys, t_mode);
    
    if (t_fptr == NULL && p_mode != kMCOpenFileModeRead)
        t_fptr = fopen(*t_path_sys, IO_CREATE_MODE);
    
    if (t_fptr == NULL)
        return false;
    
    __MCFileStream *self;
    self = (__MCFileStream *)MCStreamGetExtraBytesPtr(t_stream);
    
    self -> m_stream = t_fptr;
    self -> m_mode = p_mode;
    
    r_stream = t_stream;
    
    return true;
}


bool MCFilePathToNative(MCStringRef p_path, MCStringRef& r_native)
{
#ifdef _WIN32
    if (MCStringIsEmpty(p_path))
        return MCStringCopy(p_path, r_native);
    
    unichar_t *t_dst;
    uindex_t t_length;
    t_dst = new unichar_t[t_length + 1];
    t_length = MCStringGetChars(p_path, MCRangeMake(0, t_length), t_dst);
    
    for (uindex_t i = 0; i < t_length; i++)
    {
        if (t_dst[i] == '/')
            t_dst[i] = '\\';
        else if (t_dst[i] == '\\')
            t_dst[i] = '/';
    }
    
    return MCStringCreateWithCharsAndRelease(t_dst, t_length, r_native);
#else
    return MCStringCopy(p_path, r_native);
#endif
}

// This function strips the leading \\?\ or \\?\UNC\ from an NT-style path
static void nt_path_to_legacy_path(MCStringRef p_nt, MCStringRef &r_legacy)
{
    // Check for the leading NT path characters
    if (MCStringBeginsWithCString(p_nt, (const char_t*)"\\\\?\\UNC\\", kMCStringOptionCompareCaseless))
    {
        MCStringRef t_temp;
        /* UNCHECKED */ MCStringMutableCopySubstring(p_nt, MCRangeMake(8, MCStringGetLength(p_nt) - 8), t_temp);
        /* UNCHECKED */ MCStringPrepend(t_temp, MCSTR("\\\\"));
        /* UNCHECKED */ MCStringCopyAndRelease(t_temp, r_legacy);
    }
    else if (MCStringBeginsWithCString(p_nt, (const char_t*)"\\\\?\\", kMCStringOptionCompareCaseless))
    {
        /* UNCHECKED */ MCStringCopySubstring(p_nt, MCRangeMake(4, MCStringGetLength(p_nt) - 4), r_legacy);
    }
    else
    {
        // Not an NT-style path, no changes required.
        r_legacy = MCValueRetain(p_nt);
    }
}

bool MCFilePathFromNative(MCStringRef p_path, MCStringRef& r_livecode_path)
{
#ifdef _WIN32
    if (MCStringIsEmpty(p_native))
    {
        r_livecode_path = MCValueRetain(kMCEmptyString);
        return true;
    }
    
    // Remove any NT-style path prefix
    MCAutoStringRef t_legacy_path;
    nt_path_to_legacy_path(p_native, &t_legacy_path);
    
    // The / and \ characters in the path need to be swapped
    MCAutoArray<unichar_t> t_swapped;
    t_swapped.New(MCStringGetLength(*t_legacy_path));
    
    for (uindex_t i = 0; i < MCStringGetLength(*t_legacy_path); i++)
    {
        unichar_t t_char;
        t_char = MCStringGetCharAtIndex(*t_legacy_path, i);
        if (t_char == '/')
            t_swapped[i] = '\\';
        else if (t_char == '\\')
            t_swapped[i] = '/';
        else
            t_swapped[i] = t_char;
    }
    
    return MCStringCreateWithChars(t_swapped.Ptr(), t_swapped.Size(), r_livecode_path);
#else
    return MCStringCopy(p_path, r_livecode_path);
#endif
}

// MW-2004-11-26: Copy null-terminated string at p_src to p_dest, the strings
//   are allowed to overlap.
// SN-2014-01-09: Same as above, handling unicode chars
// Returns the characters suppressed in case the string is the same
inline index_t strmove(unichar_t *p_dest, const unichar_t *p_src, bool p_same_string)
{
    while(*p_src != 0)
        *p_dest++ = *p_src++;
    *p_dest = 0;
    
    if (p_same_string)
        return p_src - p_dest;
    else
        return 0;
}

static void fix_path(MCStringRef in, MCStringRef& r_out)
{
    unichar_t *t_unicode_str;
    uindex_t t_length;
    t_length = MCStringGetLength(in);
    
    t_unicode_str = new unichar_t[t_length + 1];
    t_length = MCStringGetChars(in, MCRangeMake(0, t_length), t_unicode_str);
    t_unicode_str[t_length] = 0;
    
    unichar_t *fptr = t_unicode_str; //pointer to search forward in curdir
    while (*fptr)
    {
        if (*fptr == '/' && *(fptr + 1) == '.'
            && *(fptr + 2) == '.' && *(fptr + 3) == '/')
        {//look for "/../" pattern
            if (fptr == t_unicode_str)
            /* Delete "/.." component */
                t_length -= strmove(fptr, fptr + 3, true);
            else
            {
                unichar_t *bptr = fptr - 1;
                while (true)
                { //search backword for '/'
                    if (*bptr == '/')
                    {
                        /* Delete "/xxx/.." component */
                        t_length -= strmove(bptr, fptr + 3, true);
                        fptr = bptr;
                        break;
                    }
                    else if (bptr == t_unicode_str)
                    {
                        /* Delete "xxx/../" component */
                        t_length -= strmove (bptr, fptr + 4, true);
                        fptr = bptr;
                        break;
                    }
                    else
                        bptr--;
                }
            }
        }
        else
            if (*fptr == '/' && *(fptr + 1) == '.' && *(fptr + 2) == '/')
                t_length -= strmove(fptr, fptr + 2, true); //erase the '/./'
            else
#ifdef _MACOSX
                if (*fptr == '/' && *(fptr + 1) == '/')
#else
                    if (fptr != t_unicode_str && *fptr == '/' && *(fptr + 1) == '/')
#endif
                        
                        t_length -= strmove(fptr, fptr + 1, true); //erase the extra '/'
                    else
                        fptr++;
    }
    
    /* UNCHECKED */ MCStringCreateWithChars(t_unicode_str, t_length, r_out);
    delete[] t_unicode_str;
}

bool MCFileGetCurrentFolder(MCStringRef& r_path)
{
#ifdef _WIN32
#elif defined (_MACOSX)
    char namebuf[PATH_MAX + 2];
    if (NULL == getcwd(namebuf, PATH_MAX))
        return false;
    
    if (!MCStringCreateWithBytes((byte_t*)namebuf, strlen(namebuf), kMCStringEncodingUTF8, false, r_path))
    {
        r_path = MCValueRetain(kMCEmptyString);
        return false;
    }
    return true;
#elif defined (_LINUX)
#else
#endif
}

bool MCFileResolveNativePath(MCStringRef p_path, MCStringRef& r_resolved_path)
{
#ifdef _WIN32
    if (MCStringGetLength(p_path) == 0)
    {
        // NOTE:
        // Get/SetCurrentDirectory are not supported by Windows in multithreaded environments
        MCAutoArray<unichar_t> t_buffer;
        
        // Retrieve the length of the current directory
        DWORD t_path_len = GetCurrentDirectoryW(0, NULL);
        /* UNCHECKED */ t_buffer.New(t_path_len);
        
        DWORD t_result = GetCurrentDirectoryW(t_path_len, t_buffer.Ptr());
        if (t_result == 0 || t_result >= t_path_len)
        {
            // Something went wrong
            return false;
        }
        
        return MCStringCreateWithChars(t_buffer.Ptr(), t_result, r_path);
    }
    
    fix_path(p_path, r_resolved_path);
    return true;
#else
    return MCStringCopy(p_path, r_resolved_path);
#endif
}

static bool mac_is_link(MCStringRef p_path)
{
    struct stat buf;
    MCAutoStringRefAsUTF8String t_utf8_path;
    /* UNCHECKED */ t_utf8_path.Lock(p_path);
    return (lstat(*t_utf8_path, &buf) == 0 && S_ISLNK(buf.st_mode));
}

static bool mac_readlink(MCStringRef p_path, MCStringRef& r_link)
{
    struct stat t_stat;
    ssize_t t_size;
    MCAutoNativeCharArray t_buffer;
    MCAutoStringRefAsUTF8String t_utf8_path;
    /* UNCHECKED */ t_utf8_path.Lock(p_path);
    if (lstat(*t_utf8_path, &t_stat) == -1 ||
        !t_buffer.New(t_stat.st_size))
        return false;
    
    t_size = readlink(*t_utf8_path, (char*)t_buffer.Chars(), t_stat.st_size);
    
    return (t_size == t_stat.st_size) && t_buffer.CreateStringAndRelease(r_link);
}

bool MCFileResolvePath(MCStringRef p_path, MCStringRef& r_resolved_path)
{
    if (MCStringGetLength(p_path) == 0)
    {
        MCFileGetCurrentFolder(r_resolved_path);
        return true;
    }

#ifdef _WIN32
    return MCFileResolveNativePath(p_path, r_resolved_path);
#elif defined (_MACOSX)
    MCAutoStringRef t_tilde_path;
    if (MCStringGetCharAtIndex(p_path, 0) == '~')
    {
        uindex_t t_user_end;
        if (!MCStringFirstIndexOfChar(p_path, '/', 0, kMCStringOptionCompareExact, t_user_end))
            t_user_end = MCStringGetLength(p_path);
        
        // Prepend user name
        struct passwd *t_password;
        if (t_user_end == 1)
            t_password = getpwuid(getuid());
        else
        {
            MCAutoStringRef t_username;
            if (!MCStringCopySubstring(p_path, MCRangeMake(1, t_user_end - 1), &t_username))
                return false;
            MCAutoStringRefAsUTF8String t_utf8_username;
            /* UNCHECKED */ t_utf8_username . Lock(*t_username);
            t_password = getpwnam(*t_utf8_username);
        }
        
        if (t_password != NULL)
        {
            if (!MCStringCreateMutable(0, &t_tilde_path) ||
                !MCStringAppendNativeChars(*t_tilde_path, (char_t*)t_password->pw_dir, strlen(t_password->pw_dir)) ||
                !MCStringAppendSubstring(*t_tilde_path, p_path, MCRangeMake(t_user_end, MCStringGetLength(p_path) - t_user_end)))
                return false;
        }
        else
            t_tilde_path = p_path;
    }
    else
        t_tilde_path = p_path;
    
    MCAutoStringRef t_fullpath;
    if (MCStringGetCharAtIndex(*t_tilde_path, 0) != '/')
    {
        MCAutoStringRef t_folder;
        /* UNCHECKED */ MCFileGetCurrentFolder(&t_folder);
        
        MCAutoStringRef t_resolved;
        if (!MCStringMutableCopy(*t_folder, &t_fullpath) ||
            !MCStringAppendChar(*t_fullpath, '/') ||
            !MCStringAppend(*t_fullpath, *t_tilde_path))
            return false;
    }
    else
        t_fullpath = *t_tilde_path;
    
    if (!mac_is_link(*t_fullpath))
        return MCStringCopy(*t_fullpath, r_resolved_path);
    
    MCAutoStringRef t_newname;
    if (!mac_readlink(*t_fullpath, &t_newname))
        return false;
    
    // IM - Should we really be using the original p_path parameter here?
    // seems like we should use the computed t_fullpath value.
    if (MCStringGetCharAtIndex(*t_newname, 0) != '/')
    {
        MCAutoStringRef t_resolved;
        
        uindex_t t_last_component;
        uindex_t t_path_length;
        
        if (MCStringLastIndexOfChar(p_path, '/', MCStringGetLength(p_path), kMCStringOptionCompareExact, t_last_component))
            t_last_component++;
        else
            t_last_component = 0;
        
        if (!MCStringMutableCopySubstring(p_path, MCRangeMake(0, t_last_component), &t_resolved) ||
            !MCStringAppend(*t_resolved, *t_newname))
            return false;
        
        return MCStringCopy(*t_resolved, r_resolved_path);
    }
    else
        return MCStringCopy(*t_newname, r_resolved_path);
#elif defined(_LINUX)
    MCAutoStringRef t_tilde_path;
    if (MCStringGetCharAtIndex(p_path, 0) == '~')
    {
        uindex_t t_user_end;
        if (!MCStringFirstIndexOfChar(p_path, '/', 0, kMCStringOptionCompareExact, t_user_end))
            t_user_end = MCStringGetLength(p_path);
        
        // Prepend user name
        struct passwd *t_password;
        if (t_user_end == 1)
            t_password = getpwuid(getuid());
        else
        {
            MCAutoStringRef t_username;
            if (!MCStringCopySubstring(p_path, MCRangeMake(1, t_user_end), &t_username))
                return false;
            
            MCAutoStringRefAsSysString t_username_sys;
            /* UNCHECKED */ t_username_sys.Lock(*t_username);
            
            t_password = getpwnam(*t_username_sys);
        }
        
        if (t_password != NULL)
        {
            MCAutoStringRef t_pw_dir;
            /* UNCHECKED */ MCStringCreateWithSysString(t_password->pw_dir, &t_pw_dir);
            
            if (!MCStringCreateMutable(0, &t_tilde_path) ||
                !MCStringAppend(*t_tilde_path, *t_pw_dir) ||
                !MCStringAppendSubstring(*t_tilde_path, p_path, MCRangeMake(t_user_end, MCStringGetLength(p_path) - t_user_end)))
                return false;
        }
        else
            t_tilde_path = p_path;
    }
    else
        t_tilde_path = p_path;
    
    // SN-2014-12-18: [[ Bug 14001 ]] Update the server file resolution to use realpath
    //  so that we get the absolute path (needed for MCcmd for instance).
#ifdef _SERVER
    MCAutoStringRefAsSysString t_tilde_path_sys;
    t_tilde_path_sys . Lock(*t_tilde_path);
    
    char *t_resolved_path;
    bool t_success;
    
    t_resolved_path = realpath(*t_tilde_path_sys, NULL);
    
    // If the does not exist, then realpath will fail: we want to
    // return something though, so we keep the input path (as it
    // is done for desktop).
    if (t_resolved_path != NULL)
        t_success = MCStringCreateWithSysString(t_resolved_path, r_resolved_path);
    else
        t_success = MCStringCopy(*t_tilde_path, r_resolved_path);
    
    MCMemoryDelete(t_resolved_path);
    
    return t_success;
#else
    
    // IM-2012-07-23
    // Keep (somewhat odd) semantics of the original function for now
    if (!MCS_lnx_is_link(*t_tilde_path))
        return MCStringCopy(*t_tilde_path, r_resolved_path);
    
    MCAutoStringRef t_newname;
    if (!MCS_lnx_readlink(*t_tilde_path, &t_newname))
        return false;
    
    if (MCStringGetCharAtIndex(*t_newname, 0) != '/')
    {
        MCAutoStringRef t_resolved;
        
        uindex_t t_last_component;
        uindex_t t_path_length;
        
        t_path_length = MCStringGetLength(p_path);
        
        if (MCStringLastIndexOfChar(p_path, '/', t_path_length, kMCStringOptionCompareExact, t_last_component))
            t_last_component++;
        else
            t_last_component = 0;
        
        if (!MCStringMutableCopySubstring(p_path, MCRangeMake(0, t_last_component), &t_resolved) ||
            !MCStringAppend(*t_resolved, *t_newname))
            return false;
        
        return MCStringCopy(*t_resolved, r_resolved_path);
    }
    else
        return MCStringCopy(*t_newname, r_resolved_path);
#endif
#else
    return MCStringCopy(p_path, r_resolved_path);
#endif
}




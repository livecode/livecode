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

#include "osxprefix.h"

#include "parsedef.h"
#include "filedefs.h"

#include "execpt.h"
#include "globals.h"
#include "system.h"
#include "osspec.h"
#include "util.h"
#include <sys/stat.h>

#include "foundation.h"

///////////////////////////////////////////////////////////////////////////////

// Special Folders

// MW-2012-10-10: [[ Bug 10453 ]] Added 'mactag' field which is the tag to use in FSFindFolder.
//   This allows macfolder to be 0, which means don't alias the tag to the specified disk.
typedef struct
{
	MCNameRef *token;
	unsigned long macfolder;
	OSType domain;
	unsigned long mactag;
}
sysfolders;

// MW-2008-01-18: [[ Bug 5799 ]] It seems that we are requesting things in the
//   wrong domain - particularly for 'temp'. See:
// http://lists.apple.com/archives/carbon-development/2003/Oct/msg00318.html

static sysfolders sysfolderlist[] = {
    {&MCN_apple, 'amnu', kOnAppropriateDisk, 'amnu'},
    {&MCN_desktop, 'desk', kOnAppropriateDisk, 'desk'},
    {&MCN_control, 'ctrl', kOnAppropriateDisk, 'ctrl'},
    {&MCN_extension,'extn', kOnAppropriateDisk, 'extn'},
    {&MCN_fonts,'font', kOnAppropriateDisk, 'font'},
    {&MCN_preferences,'pref', kUserDomain, 'pref'},
    {&MCN_temporary,'temp', kUserDomain, 'temp'},
    {&MCN_system, 'macs', kOnAppropriateDisk, 'macs'},
    // TS-2007-08-20: Added to allow a common notion of "home" between all platforms
    {&MCN_home, 'cusr', kUserDomain, 'cusr'},
    // MW-2007-09-11: Added for uniformity across platforms
    {&MCN_documents, 'docs', kUserDomain, 'docs'},
    // MW-2007-10-08: [[ Bug 10277 ] Add support for the 'application support' at user level.
    {&MCN_support, 0, kUserDomain, 'asup'},
};

bool MCS_specialfolder_to_mac_folder(MCStringRef p_type, uint32_t& r_folder, OSType& r_domain)
{
	for (uindex_t i = 0; i < ELEMENTS(sysfolderlist); i++)
	{
		if (MCStringIsEqualTo(p_type, MCNameGetString(*(sysfolderlist[i].token)), kMCStringOptionCompareCaseless))
		{
			r_folder = sysfolderlist[i].macfolder;
			r_domain = sysfolderlist[i].domain;
            return true;
		}
	}
    return false;
}

///////////////////////////////////////////////////////////////////////////////

class MCStdioFileHandle: public MCSystemFileHandle
{
public:
	static MCStdioFileHandle *Open(const char *p_path, const char *p_mode)
	{
		FILE *t_stream;
		t_stream = fopen(p_path, p_mode);
		if (t_stream == NULL)
			return NULL;
		
		MCStdioFileHandle *t_handle;
		t_handle = new MCStdioFileHandle;
		t_handle -> m_stream = t_stream;
		
		return t_handle;
	}
	
	static MCStdioFileHandle *OpenFd(int fd, const char *p_mode)
	{
		FILE *t_stream;
		t_stream = fdopen(fd, p_mode);
		if (t_stream == NULL)
			return NULL;
		
		// MW-2011-06-27: [[ SERVER ]] Turn off buffering for output stderr / stdout
		if (fd == 1 || fd == 2)
			setbuf(t_stream, NULL);
		
		MCStdioFileHandle *t_handle;
		t_handle = new MCStdioFileHandle;
		t_handle -> m_stream = t_stream;
		
		return t_handle;
		
	}
	
	virtual void Close(void)
	{
		fclose(m_stream);
		delete this;
	}
	
	virtual bool Read(void *p_buffer, uint32_t p_length, uint32_t& r_read)
	{
		size_t t_amount;
		t_amount = fread(p_buffer, 1, p_length, m_stream);
		r_read = t_amount;
		
		if (t_amount < p_length)
			return ferror(m_stream) == 0;
		
		return true;
	}
	
	virtual bool Write(const void *p_buffer, uint32_t p_length, uint32_t& r_written)
	{
		size_t t_amount;
		t_amount = fwrite(p_buffer, 1, p_length, m_stream);
		r_written = t_amount;
		
		if (t_amount < p_length)
			return false;
		
		return true;
	}
	
	virtual bool Seek(int64_t offset, int p_dir)
	{
		return fseeko(m_stream, offset, p_dir < 0 ? SEEK_END : (p_dir > 0 ? SEEK_SET : SEEK_CUR)) == 0;
	}
	
	virtual bool Truncate(void)
	{
		return ftruncate(fileno(m_stream), ftell(m_stream)) == 0;
	}
	
	virtual bool Sync(void)
	{
		int64_t t_pos;
		t_pos = ftello(m_stream);
		return fseeko(m_stream, t_pos, SEEK_SET) == 0;
	}
	
	virtual bool Flush(void)
	{
		return fflush(m_stream) == 0;
	}
	
	virtual bool PutBack(char p_char)
	{
		return ungetc(p_char, m_stream) != EOF;
	}
	
	virtual int64_t Tell(void)
	{
		return ftello(m_stream);
	}
	
	virtual int64_t GetFileSize(void)
	{
		struct stat t_info;
		if (fstat(fileno(m_stream), &t_info) != 0)
			return 0;
		return t_info . st_size;
	}
	
	virtual void *GetFilePointer(void)
	{
		return NULL;
	}
	
	FILE *GetStream(void)
	{
		return m_stream;
	}
	
private:
	FILE *m_stream;
};

struct MCMacDesktop: public MCSystemInterface
{
	virtual bool Initialize(void)
    {
        
    }
	virtual void Finalize(void)
    {
        
    }
	
	virtual void Debug(MCStringRef p_string)
    {
        
    }
    
	virtual real64_t GetCurrentTime(void)
    {
        
    }
    
	virtual bool GetVersion(MCStringRef& r_string)
    {
        
    }
	virtual bool GetMachine(MCStringRef& r_string)
    {
        
    }
	virtual MCNameRef GetProcessor(void)
    {
        
    }
	virtual bool GetAddress(MCStringRef& r_string)
    {
        
    }
    
	virtual uint32_t GetProcessId(void)
    {
        
    }
	
	virtual void Alarm(real64_t p_when)
    {
        
    }
	virtual void Sleep(real64_t p_when)
    {
        
    }
	
	virtual void SetEnv(MCStringRef name, MCStringRef value)
    {
        
    }
	virtual void GetEnv(MCStringRef name, MCStringRef& value)
    {
        
    }
	
	virtual Boolean CreateFolder(MCStringRef p_path)
    {
#ifdef /* MCS_mkdir_dsk_mac */ LEGACY_SYSTEM
    
	char *newpath = path2utf(MCS_resolvepath(path));
	Boolean done = mkdir(newpath, 0777) == 0;
	delete newpath;
	return done;
#endif /* MCS_mkdir_dsk_mac */
        MCAutoStringRefAsUTF8String t_path;
        if (!t_path.Lock(p_path))
            return False;
        
        if (mkdir(*t_path, 0777) != 0)
            return False;
            
        return True;
    }
    
	virtual Boolean DeleteFolder(MCStringRef p_path)
    {
#ifdef /* MCS_rmdir_dsk_mac */ LEGACY_SYSTEM
    
    char *newpath = path2utf(MCS_resolvepath(path));
    Boolean done = rmdir(newpath) == 0;
    delete newpath;
    return done;
#endif /* MCS_rmdir_dsk_mac */
        MCAutoStringRefAsUTF8String t_path;
        if (!t_path.Lock(p_path))
            return False;
        
        if (rmdir(*t_path) != 0)
            return False;
        
        return True;
    }
	
	virtual Boolean DeleteFile(MCStringRef p_path)
    {
#ifdef /* MCS_unlink_dsk_mac */ LEGACY_SYSTEM
    char *newpath = path2utf(MCS_resolvepath(path));
    Boolean done = remove(newpath) == 0;
    delete newpath;
    return done;
#endif /* MCS_unlink_dsk_mac */
        MCAutoStringRefAsUTF8String t_path;
        if (!t_path.Lock(p_path))
            return False;
        
        if (remove(*t_path) != 0)
            return False;
        
        return True;
    }
	
	virtual Boolean RenameFileOrFolder(MCStringRef p_old_name, MCStringRef p_new_name)
    {
#ifdef /* MCS_rename_dsk_mac */ LEGACY_SYSTEM
    //rename a file or directory
    
	char *oldpath = path2utf(MCS_resolvepath(oname));
	char *newpath = path2utf(MCS_resolvepath(nname));
	Boolean done = rename(oldpath, newpath) == 0;
    
	delete oldpath;
	delete newpath;
	return done;
#endif /* MCS_rename_dsk_mac */
        MCAutoStringRefAsUTF8String t_old_name, t_new_name;
        
        if (!t_old_name.Lock(p_old_name) || !t_new_name.Lock(p_new_name))
            return False;
        
        if (rename(*t_old_name, *t_new_name) != 0)
            return False;
        
        return True;
    }
	
    // MW-2007-07-16: [[ Bug 5214 ]] Use rename instead of FSExchangeObjects since
    //   the latter isn't supported on all FS's.
    // MW-2007-12-12: [[ Bug 5674 ]] Unfortunately, just renaming the current stack
    //   causes all Finder meta-data to be lost, so what we will do is first try
    //   to FSExchangeObjects and if that fails, do a rename.
	virtual Boolean BackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
    {
#ifdef /* MCS_backup_dsk_mac */ LEGACY_SYSTEM
	bool t_error;
	t_error = false;
	
	FSRef t_src_ref;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = MCS_pathtoref(p_src_path, &t_src_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
    
	FSRef t_dst_parent_ref;
	FSRef t_dst_ref;
	UniChar *t_dst_leaf;
	t_dst_leaf = NULL;
	UniCharCount t_dst_leaf_length;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = MCS_pathtoref(p_dst_path, &t_dst_ref);
		if (t_os_error == noErr)
			FSDeleteObject(&t_dst_ref);
        
		// Get the information to create the file
		t_os_error = MCS_pathtoref_and_leaf(p_dst_path, t_dst_parent_ref, t_dst_leaf, t_dst_leaf_length);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	FSCatalogInfo t_dst_catalog;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSGetCatalogInfo(&t_src_ref, kFSCatInfoFinderInfo, &t_dst_catalog, NULL, NULL, NULL);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	if (!t_error)
	{
		memcpy(&((FileInfo *) t_dst_catalog . finderInfo) -> fileType, &MCfiletype[4], 4);
		memcpy(&((FileInfo *) t_dst_catalog . finderInfo) -> fileCreator, MCfiletype, 4);
		((FileInfo *) t_dst_catalog . finderInfo) -> fileType = MCSwapInt32NetworkToHost(((FileInfo *) t_dst_catalog . finderInfo) -> fileType);
		((FileInfo *) t_dst_catalog . finderInfo) -> fileCreator = MCSwapInt32NetworkToHost(((FileInfo *) t_dst_catalog . finderInfo) -> fileCreator);
	}
	
	bool t_created_dst;
	t_created_dst = false;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSCreateFileUnicode(&t_dst_parent_ref, t_dst_leaf_length, t_dst_leaf, kFSCatInfoFinderInfo, &t_dst_catalog, &t_dst_ref, NULL);
		if (t_os_error == noErr)
			t_created_dst = true;
		else
			t_error = true;
	}
	
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSExchangeObjects(&t_src_ref, &t_dst_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	if (t_error && t_created_dst)
		FSDeleteObject(&t_dst_ref);
	
	if (t_dst_leaf != NULL)
		delete t_dst_leaf;
    
	if (t_error)
		t_error = !MCS_rename(p_src_path, p_dst_path);
    
	return !t_error;
#endif /* MCS_backup_dsk_mac */
        bool t_error;
        t_error = false;
        
        FSRef t_src_ref;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = MCS_pathtoref(p_old_name, t_src_ref);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        FSRef t_dst_parent_ref;
        FSRef t_dst_ref;
        UniChar *t_dst_leaf;
        t_dst_leaf = NULL;
        UniCharCount t_dst_leaf_length;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = MCS_pathtoref(p_new_name, t_dst_ref);
            if (t_os_error == noErr)
                FSDeleteObject(&t_dst_ref);
			
            // Get the information to create the file
            t_os_error = MCS_pathtoref_and_leaf(p_new_name, t_dst_parent_ref, t_dst_leaf, t_dst_leaf_length);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        FSCatalogInfo t_dst_catalog;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = FSGetCatalogInfo(&t_src_ref, kFSCatInfoFinderInfo, &t_dst_catalog, NULL, NULL, NULL);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        if (!t_error)
        {
            memcpy(&((FileInfo *) t_dst_catalog . finderInfo) -> fileType, &MCfiletype[4], 4);
            memcpy(&((FileInfo *) t_dst_catalog . finderInfo) -> fileCreator, MCfiletype, 4);
            ((FileInfo *) t_dst_catalog . finderInfo) -> fileType = MCSwapInt32NetworkToHost(((FileInfo *) t_dst_catalog . finderInfo) -> fileType);
            ((FileInfo *) t_dst_catalog . finderInfo) -> fileCreator = MCSwapInt32NetworkToHost(((FileInfo *) t_dst_catalog . finderInfo) -> fileCreator);
        }
        
        bool t_created_dst;
        t_created_dst = false;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = FSCreateFileUnicode(&t_dst_parent_ref, t_dst_leaf_length, t_dst_leaf, kFSCatInfoFinderInfo, &t_dst_catalog, &t_dst_ref, NULL);
            if (t_os_error == noErr)
                t_created_dst = true;
            else
                t_error = true;
        }
        
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = FSExchangeObjects(&t_src_ref, &t_dst_ref);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        if (t_error && t_created_dst)
            FSDeleteObject(&t_dst_ref);
        
        if (t_dst_leaf != NULL)
            delete t_dst_leaf;
		
        if (t_error)
            t_error = !RenameFileOrFolder(p_old_name, p_new_name);
		
        if (t_error)
            return False;
        
        return True;
    }
    
	virtual Boolean UnbackupFile(MCStringRef p_old_name, MCStringRef p_new_name)
    {
#ifdef /* MCS_unbackup_dsk_mac */ LEGACY_SYSTEM
	bool t_error;
	t_error = false;
	
	FSRef t_src_ref;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = MCS_pathtoref(p_src_path, &t_src_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	FSRef t_dst_ref;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = MCS_pathtoref(p_dst_path, &t_dst_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	// It appears that the source file here is the ~file, the backup file.
	// So copy it over to p_dst_path, and delete it.
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSExchangeObjects(&t_src_ref, &t_dst_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSDeleteObject(&t_src_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
    
	if (t_error)
		t_error = !MCS_rename(p_src_path, p_dst_path);
    
	return !t_error;
#endif /* MCS_unbackup_dsk_mac */
        bool t_error;
        t_error = false;
        
        FSRef t_src_ref;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = MCS_pathtoref(p_old_name, t_src_ref);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        FSRef t_dst_ref;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = MCS_pathtoref(p_new_name, t_dst_ref);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        // It appears that the source file here is the ~file, the backup file.
        // So copy it over to p_dst_path, and delete it.
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = FSExchangeObjects(&t_src_ref, &t_dst_ref);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = FSDeleteObject(&t_src_ref);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        if (t_error)
            t_error = !MCS_rename(p_old_name, p_new_name);
		
        if (t_error)
            return False;
        
        return True;
    }
	
	virtual Boolean CreateAlias(MCStringRef p_target, MCStringRef p_alias)
    {
#ifdef /* MCS_createalias_dsk_mac */ LEGACY_SYSTEM
	bool t_error;
	t_error = false;
	
	// Check if the destination exists already and return an error if it does
	if (!t_error)
	{
		FSRef t_dst_ref;
		OSErr t_os_error;
		t_os_error = MCS_pathtoref(p_dest_path, &t_dst_ref);
		if (t_os_error == noErr)
			return False; // we expect an error
	}
    
	FSRef t_src_ref;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = MCS_pathtoref(p_source_path, &t_src_ref);
		if (t_os_error != noErr)
			t_error = true;
	}
    
	FSRef t_dst_parent_ref;
	UniChar *t_dst_leaf_name;
	UniCharCount t_dst_leaf_name_length;
	t_dst_leaf_name = NULL;
	t_dst_leaf_name_length = 0;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = MCS_pathtoref_and_leaf(p_dest_path, t_dst_parent_ref, t_dst_leaf_name, t_dst_leaf_name_length);
		if (t_os_error != noErr)
			t_error = true;
	}
    
	AliasHandle t_alias;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSNewAlias(NULL, &t_src_ref, &t_alias);
		if (t_os_error != noErr)
			t_error = true;
	}
	
	IconRef t_src_icon;
	t_src_icon = NULL;
	if (!t_error)
	{
		OSErr t_os_error;
		SInt16 t_unused_label;
		t_os_error = GetIconRefFromFileInfo(&t_src_ref, 0, NULL, kFSCatInfoNone, NULL, kIconServicesNormalUsageFlag, &t_src_icon, &t_unused_label);
		if (t_os_error != noErr)
			t_src_icon = NULL;
	}
	
	IconFamilyHandle t_icon_family;
	t_icon_family = NULL;
	if (!t_error && t_src_icon != NULL)
	{
		OSErr t_os_error;
		IconRefToIconFamily(t_src_icon, kSelectorAllAvailableData, &t_icon_family);
	}
	
	HFSUniStr255 t_fork_name;
	if (!t_error)
		FSGetResourceForkName(&t_fork_name);
    
	FSRef t_dst_ref;
	FSSpec t_dst_spec;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSCreateResourceFile(&t_dst_parent_ref, t_dst_leaf_name_length, t_dst_leaf_name,
                                          kFSCatInfoNone, NULL, t_fork_name . length, t_fork_name . unicode, &t_dst_ref, &t_dst_spec);
		if (t_os_error != noErr)
			t_error = true;
	}
    
	ResFileRefNum t_res_file;
	bool t_res_file_opened;
	if (!t_error)
	{
		OSErr t_os_error;
		t_os_error = FSOpenResourceFile(&t_dst_ref, t_fork_name . length, t_fork_name . unicode, 3, &t_res_file);
		if (t_os_error != noErr)
			t_error = true;
		else
			t_res_file_opened = true;
	}
    
	if (!t_error)
	{
		AddResource((Handle)t_alias, rAliasType, 0, (ConstStr255Param)"");
		if (ResError() != noErr)
			t_error = true;
	}
	
	if (!t_error && t_icon_family != NULL)
		AddResource((Handle)t_icon_family, 'icns', -16496, NULL);
    
	if (t_res_file_opened)
		CloseResFile(t_res_file);
	
	if (!t_error)
	{
		FSCatalogInfo t_info;
		FSGetCatalogInfo(&t_dst_ref, kFSCatInfoFinderInfo, &t_info, NULL, NULL, NULL);
		((FileInfo *)&t_info . finderInfo) -> finderFlags |= kIsAlias;
		if (t_icon_family != NULL)
			((FileInfo *)&t_info . finderInfo) -> finderFlags |= kHasCustomIcon;
		FSSetCatalogInfo(&t_dst_ref, kFSCatInfoFinderInfo, &t_info);
	}
    
	if (t_src_icon != NULL)
		ReleaseIconRef(t_src_icon);
    
	if (t_dst_leaf_name != NULL)
		delete t_dst_leaf_name;
	
	if (t_error)
	{
		if (t_icon_family != NULL)
			DisposeHandle((Handle)t_icon_family);
		FSDeleteObject(&t_dst_ref);
	}
    
	return !t_error;       
#endif /* MCS_createalias_dsk_mac */
        bool t_error;
        t_error = false;
        
        // Check if the destination exists already and return an error if it does
        if (!t_error)
        {
            FSRef t_dst_ref;
            OSErr t_os_error;
            t_os_error = MCS_pathtoref(p_alias, t_dst_ref);
            if (t_os_error == noErr)
                return False; // we expect an error
        }
        
        FSRef t_src_ref;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = MCS_pathtoref(p_target, t_src_ref);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        FSRef t_dst_parent_ref;
        UniChar *t_dst_leaf_name;
        UniCharCount t_dst_leaf_name_length;
        t_dst_leaf_name = NULL;
        t_dst_leaf_name_length = 0;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = MCS_pathtoref_and_leaf(p_alias, t_dst_parent_ref, t_dst_leaf_name, t_dst_leaf_name_length);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        AliasHandle t_alias;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = FSNewAlias(NULL, &t_src_ref, &t_alias);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        IconRef t_src_icon;
        t_src_icon = NULL;
        if (!t_error)
        {
            OSErr t_os_error;
            SInt16 t_unused_label;
            t_os_error = GetIconRefFromFileInfo(&t_src_ref, 0, NULL, kFSCatInfoNone, NULL, kIconServicesNormalUsageFlag, &t_src_icon, &t_unused_label);
            if (t_os_error != noErr)
                t_src_icon = NULL;
        }
        
        IconFamilyHandle t_icon_family;
        t_icon_family = NULL;
        if (!t_error && t_src_icon != NULL)
        {
            OSErr t_os_error;
            IconRefToIconFamily(t_src_icon, kSelectorAllAvailableData, &t_icon_family);
        }
        
        HFSUniStr255 t_fork_name;
        if (!t_error)
            FSGetResourceForkName(&t_fork_name);
        
        FSRef t_dst_ref;
        FSSpec t_dst_spec;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = FSCreateResourceFile(&t_dst_parent_ref, t_dst_leaf_name_length, t_dst_leaf_name,
                                              kFSCatInfoNone, NULL, t_fork_name . length, t_fork_name . unicode, &t_dst_ref, &t_dst_spec);
            if (t_os_error != noErr)
                t_error = true;
        }
        
        ResFileRefNum t_res_file;
        bool t_res_file_opened;
        if (!t_error)
        {
            OSErr t_os_error;
            t_os_error = FSOpenResourceFile(&t_dst_ref, t_fork_name . length, t_fork_name . unicode, 3, &t_res_file);
            if (t_os_error != noErr)
                t_error = true;
            else
                t_res_file_opened = true;
        }
        
        if (!t_error)
        {
            AddResource((Handle)t_alias, rAliasType, 0, (ConstStr255Param)"");
            if (ResError() != noErr)
                t_error = true;
        }
        
        if (!t_error && t_icon_family != NULL)
            AddResource((Handle)t_icon_family, 'icns', -16496, NULL);
        
        if (t_res_file_opened)
            CloseResFile(t_res_file);
        
        if (!t_error)
        {
            FSCatalogInfo t_info;
            FSGetCatalogInfo(&t_dst_ref, kFSCatInfoFinderInfo, &t_info, NULL, NULL, NULL);
            ((FileInfo *)&t_info . finderInfo) -> finderFlags |= kIsAlias;
            if (t_icon_family != NULL)
                ((FileInfo *)&t_info . finderInfo) -> finderFlags |= kHasCustomIcon;
            FSSetCatalogInfo(&t_dst_ref, kFSCatInfoFinderInfo, &t_info);		
        }
        
        if (t_src_icon != NULL)
            ReleaseIconRef(t_src_icon);
        
        if (t_dst_leaf_name != NULL)
            delete t_dst_leaf_name;
        
        if (t_error)
        {
            if (t_icon_family != NULL)
                DisposeHandle((Handle)t_icon_family);
            FSDeleteObject(&t_dst_ref);
        }
		
        if (t_error)
            return False;
        
        return True;
    }
	// NOTE: 'ResolveAlias' returns a standard (not native) path.
	virtual Boolean ResolveAlias(MCStringRef p_target, MCStringRef& r_resolved_path)
    {
#ifdef /* MCS_resolvealias_dsk_mac */ LEGACY_SYSTEM
    FSRef t_fsref;
    
    OSErr t_os_error;
    t_os_error = MCS_pathtoref(p_path, t_fsref);
    if (t_os_error != noErr)
        return MCStringCreateWithCString("file not found", r_error);
    
    Boolean t_is_folder;
    Boolean t_is_alias;
    
    t_os_error = FSResolveAliasFile(&t_fsref, TRUE, &t_is_folder, &t_is_alias);
    if (t_os_error != noErr || !t_is_alias) // this always seems to be false
        return MCStringCreateWithCString("can't get alias", r_error);
    
    if (!MCS_fsref_to_path(t_fsref, r_resolved))
        return MCStringCreateWithCString("can't get alias path", r_error);
    
    return true;
#endif /* MCS_resolvealias_dsk_mac */
        FSRef t_fsref;
        
        OSErr t_os_error;
        t_os_error = MCS_pathtoref(p_target, t_fsref);
        if (t_os_error != noErr)
        {
            if (!MCStringCreateWithCString("file not found", r_resolved_path))
                return False;
            else
                return True;
        }
        
        Boolean t_is_folder;
        Boolean t_is_alias;
        
        t_os_error = FSResolveAliasFile(&t_fsref, TRUE, &t_is_folder, &t_is_alias);
        if (t_os_error != noErr || !t_is_alias) // this always seems to be false
        {
            if (!MCStringCreateWithCString("can't get alias", r_resolved_path))
                return False;
            else
                return True;
        }
        
        if (!MCS_fsref_to_path(t_fsref, r_resolved_path))
        {
            if (!MCStringCreateWithCString("can't get alias path", r_resolved_path))
                return False;
            
            return True;
        }
        
        return True;
    }
	
	virtual Boolean GetCurrentFolder(MCStringRef& r_path)
    {
#ifdef /* MCS_getcurdir_dsk_mac */ LEGACY_SYSTEM
    char namebuf[PATH_MAX + 2];
    if (NULL == getcwd(namebuf, PATH_MAX))
        return false;
        
    MCAutoNativeCharArray t_buffer;
    if (!t_buffer.New(PATH_MAX + 1))
        return false;
        
    uint4 outlen;
    outlen = PATH_MAX + 1;
    MCS_utf8tonative(namebuf, strlen(namebuf), (char*)t_buffer.Chars(), outlen);
    t_buffer.Shrink(outlen);
    return t_buffer.CreateStringAndRelease(r_path);
#endif /* MCS_getcurdir_dsk_mac */
        char namebuf[PATH_MAX + 2];
        if (NULL == getcwd(namebuf, PATH_MAX))
            return false;
        
        MCAutoNativeCharArray t_buffer;
        if (!t_buffer.New(PATH_MAX + 1))
            return false;
        
        uint4 outlen;
        outlen = PATH_MAX + 1;
        MCS_utf8tonative(namebuf, strlen(namebuf), (char*)t_buffer.Chars(), outlen);
        t_buffer.Shrink(outlen);
        if (!t_buffer.CreateStringAndRelease(r_path))
            return False;
        
        return True;
    }
    
    // MW-2006-04-07: Bug 3201 - MCS_resolvepath returns NULL if unable to find a ~<username> folder.
	virtual Boolean SetCurrentFolder(MCStringRef p_path)
    {
#ifdef /* MCS_setcurdir_dsk_mac */ LEGACY_SYSTEM
    char *t_resolved_path;
    t_resolved_path = MCS_resolvepath(path);
    if (t_resolved_path == NULL)
        return False;
        
    char *newpath = NULL;
    newpath = path2utf(t_resolved_path);
    
    Boolean done = chdir(newpath) == 0;
    delete newpath;
    if (!done)
        return False;
    
    return True;
#endif /* MCS_setcurdir_dsk_mac */
        MCAutoStringRefAsUTF8String t_utf8_string;
        if (!t_utf8_string.Lock(p_path))
            return false;
        
        if (chdir(*t_utf8_string) != 0)
            return False;
        
        return True;
    }
	
	// NOTE: 'GetStandardFolder' returns a standard (not native) path.
	virtual Boolean GetStandardFolder(MCNameRef p_type, MCStringRef& r_folder)
    {
#ifdef /* MCS_getspecialfolder_dsk_mac */ LEGACY_SYSTEM
    uint32_t t_mac_folder = 0;
    OSType t_domain = kOnAppropriateDisk;
    bool t_found_folder = false;
    
    if (MCS_specialfolder_to_mac_folder(p_type, t_mac_folder, t_domain))
        t_found_folder = true;
    else if (MCStringGetLength(p_type) == 4)
    {
        t_mac_folder = MCSwapInt32NetworkToHost(*((uint32_t*)MCStringGetBytePtr(p_type)));
        
        uindex_t t_i;
        for (t_i = 0 ; t_i < ELEMENTS(sysfolderlist); t_i++)
            if (t_mac_folder == sysfolderlist[t_i] . macfolder)
            {
                t_domain = sysfolderlist[t_i] . domain;
                t_mac_folder = sysfolderlist[t_i] . mactag;
                t_found_folder = true;
                break;
            }
    }
    
    FSRef t_folder_ref;
    if (t_found_folder)
    {
        OSErr t_os_error;
        Boolean t_create_folder;
        t_create_folder = t_domain == kUserDomain ? kCreateFolder : kDontCreateFolder;
        t_os_error = FSFindFolder(t_domain, t_mac_folder, t_create_folder, &t_folder_ref);
        t_found_folder = t_os_error == noErr;
    }
    
    if (!t_found_folder)
    {
        r_path = MCValueRetain(kMCEmptyString);
        return true;
    }
    
    return MCS_fsref_to_path(t_folder_ref, r_path);
#endif /* MCS_getspecialfolder_dsk_mac */
        uint32_t t_mac_folder = 0;
        OSType t_domain = kOnAppropriateDisk;
        bool t_found_folder = false;
        
        if (MCS_specialfolder_to_mac_folder(MCNameGetString(p_type), t_mac_folder, t_domain))
            t_found_folder = true;
        else if (MCStringGetLength(MCNameGetString(p_type)) == 4)
        {
            t_mac_folder = MCSwapInt32NetworkToHost(*((uint32_t*)MCStringGetBytePtr(MCNameGetString(p_type))));
			
            uindex_t t_i;
            for (t_i = 0 ; t_i < ELEMENTS(sysfolderlist); t_i++)
                if (t_mac_folder == sysfolderlist[t_i] . macfolder)
                {
                    t_domain = sysfolderlist[t_i] . domain;
                    t_mac_folder = sysfolderlist[t_i] . mactag;
                    t_found_folder = true;
                    break;
                }
        }
        
        FSRef t_folder_ref;
        if (t_found_folder)
        {
            OSErr t_os_error;
            Boolean t_create_folder;
            t_create_folder = t_domain == kUserDomain ? kCreateFolder : kDontCreateFolder;
            t_os_error = FSFindFolder(t_domain, t_mac_folder, t_create_folder, &t_folder_ref);
            t_found_folder = t_os_error == noErr;
        }
        
        if (!t_found_folder)
        {
            r_folder = MCValueRetain(kMCEmptyString);
            return True;
        }
		
        if (!MCS_fsref_to_path(t_folder_ref, r_folder))
            return False;
        
        return True;
    }
	
	virtual Boolean FileExists(MCStringRef p_path)
    {
#ifdef /* MCS_exists_dsk_mac */ LEGACY_SYSTEM
    if (MCStringGetLength(p_path) == 0)
        return false;
    
    MCAutoStringRef t_resolved, t_utf8_path;
    if (!MCS_resolvepath(p_path, &t_resolved) ||
        !MCU_nativetoutf8(*t_resolved, &t_utf8_path))
        return false;
        
    bool t_found;
    struct stat buf;
    t_found = stat(MCStringGetCString(*t_utf8_path), (struct stat *)&buf) == 0;
    if (t_found)
        t_found = (p_is_file == ((buf.st_mode & S_IFDIR) == 0));
    
    return t_found;
#endif /* MCS_exists_dsk_mac */
        if (MCStringGetLength(p_path) == 0)
            return False;
        
        MCAutoStringRefAsUTF8String t_utf8_path;
        if (!t_utf8_path.Lock(p_path))
            return False;
        
        bool t_found;
        struct stat buf;
        t_found = stat(*t_utf8_path, (struct stat *)&buf) == 0;
        if (t_found)
            t_found = (buf.st_mode & S_IFDIR);
        
        if (!t_found)
            return False;
        
        return True;
    }
    
	virtual Boolean FolderExists(MCStringRef p_path)
    {
        if (MCStringGetLength(p_path) == 0)
            return False;
        
        MCAutoStringRefAsUTF8String t_utf8_path;
        if (!t_utf8_path.Lock(p_path))
            return False;
        
        bool t_found;
        struct stat buf;
        t_found = stat(*t_utf8_path, (struct stat *)&buf) == 0;
        if (t_found)
            t_found = (buf.st_mode & S_IFDIR) == 0;
        
        if (!t_found)
            return False;
        
        return True;
    }
    
	virtual Boolean FileNotAccessible(MCStringRef p_path)
    {
#ifdef /* MCS_noperm_dsk_mac */ LEGACY_SYSTEM
    return False;
#endif /* MCS_noperm_dsk_mac */
        return False;
    }
	
	virtual Boolean ChangePermissions(MCStringRef p_path, uint2 p_mask)
    {
#ifdef /* MCS_chmodMacDsk_dsk_mac */ LEGACY_SYSTEM
    return IO_NORMAL;
#endif /* MCS_chmodMacDsk_dsk_mac */
        return True;
    }
	virtual uint2 UMask(uint2 p_mask)
    {
#ifdef /* MCS_umask_dsk_mac */ LEGACY_SYSTEM
	return 0;
#endif /* MCS_umask_dsk_mac */
        return 0;
    }
	
	// NOTE: 'GetTemporaryFileName' returns a standard (not native) path.
	virtual void GetTemporaryFileName(MCStringRef& r_tmp_name)
    {
#ifdef /* MCS_tmpnam_dsk_mac */ LEGACY_SYSTEM
	char *t_temp_file = nil;
	FSRef t_folder_ref;
	if (FSFindFolder(kOnSystemDisk, kTemporaryFolderType, TRUE, &t_folder_ref) == noErr)
	{
		t_temp_file = MCS_fsref_to_path(t_folder_ref);
		MCCStringAppendFormat(t_temp_file, "/tmp.%d.XXXXXXXX", getpid());
		
		int t_fd;
		t_fd = mkstemp(t_temp_file);
		if (t_fd == -1)
		{
			delete t_temp_file;
			return false;
		}

		close(t_fd);
		unlink(t_temp_file);
	}
	
	if (t_temp_file == nil)
	{
		r_path = MCValueRetain(kMCEmptyString);
		return true;
	}
	
	bool t_success = MCStringCreateWithCString(t_temp_file, r_path);
	delete t_temp_file;
	return t_success;
#endif /* MCS_tmpnam_dsk_mac */
        bool t_error = false;
        MCAutoStringRef t_temp_file_auto;
        FSRef t_folder_ref;
        char* t_temp_file_chars;
        
        t_temp_file_chars = nil;        
        t_error = !MCStringCreateMutable(0, &t_temp_file_auto);
        
        if (!t_error && FSFindFolder(kOnSystemDisk, kTemporaryFolderType, TRUE, &t_folder_ref) == noErr)
        {
            int t_fd;
            t_error = !MCS_fsref_to_path(t_folder_ref, &t_temp_file_auto);
            
            if (!t_error)
                t_error = MCStringAppendFormat(&t_temp_file_auto, "/tmp.%d.XXXXXXXX", getpid());
            
            t_error = MCMemoryAllocateCopy(MCStringGetCString(*t_temp_file_auto), MCStringGetLength(*t_temp_file_auto) + 1, t_temp_file_chars);
            
            if (!t_error)
            {
                t_fd = mkstemp(t_temp_file_chars);
                t_error = t_fd != -1;
            }
            
            if (!t_error)
            {
                close(t_fd);
                t_error = unlink(t_temp_file_chars) != 0;
            }
        }
        
        if (!t_error)
            t_error = !MCStringCreateWithCString(t_temp_file_chars, r_tmp_name);
        
        if (t_error)
            r_tmp_name = MCValueRetain(kMCEmptyString);
        
        MCMemoryDeallocate(t_temp_file_chars);
    }
    
#define CATALOG_MAX_ENTRIES 16
	virtual bool ListFolderEntries(bool p_files, bool p_detailed, MCListRef& r_list)
    {
#ifdef /* MCS_getentries_dsk_mac */ LEGACY_SYSTEM
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	OSStatus t_os_status;
	
	Boolean t_is_folder;
	FSRef t_current_fsref;
	
	t_os_status = FSPathMakeRef((const UInt8 *)".", &t_current_fsref, &t_is_folder);
	if (t_os_status != noErr || !t_is_folder)
		return false;

	// Create the iterator, pass kFSIterateFlat to iterate over the current subtree only
	FSIterator t_catalog_iterator;
	t_os_status = FSOpenIterator(&t_current_fsref, kFSIterateFlat, &t_catalog_iterator);
	if (t_os_status != noErr)
		return false;
	
	uint4 t_entry_count;
	t_entry_count = 0;
	
	if (!p_files)
	{
		t_entry_count++;
		/* UNCHECKED */ MCListAppendCString(*t_list, "..");
	}
	
	ItemCount t_max_objects, t_actual_objects;
	t_max_objects = CATALOG_MAX_ENTRIES;
	t_actual_objects = 0;
	FSCatalogInfo t_catalog_infos[CATALOG_MAX_ENTRIES];
	HFSUniStr255 t_names[CATALOG_MAX_ENTRIES];
	
	FSCatalogInfoBitmap t_info_bitmap;
	t_info_bitmap = kFSCatInfoAllDates |
					kFSCatInfoPermissions |
					kFSCatInfoUserAccess |
					kFSCatInfoFinderInfo | 
					kFSCatInfoDataSizes |
					kFSCatInfoRsrcSizes |
					kFSCatInfoNodeFlags;

	MCExecPoint t_tmp_context(NULL, NULL, NULL);	
	OSErr t_oserror;
	do
	{
		t_oserror = FSGetCatalogInfoBulk(t_catalog_iterator, t_max_objects, &t_actual_objects, NULL, t_info_bitmap, t_catalog_infos, NULL, NULL, t_names);
		if (t_oserror != noErr && t_oserror != errFSNoMoreItems)
		{	// clean up and exit
			FSCloseIterator(t_catalog_iterator);
			return false;
		}
		
		for(uint4 t_i = 0; t_i < (uint4)t_actual_objects; t_i++)
		{
			// folders
			UInt16 t_is_folder;
			t_is_folder = t_catalog_infos[t_i] . nodeFlags & kFSNodeIsDirectoryMask;
			if ( (!p_files && t_is_folder) || (p_files && !t_is_folder))
			{
				char t_native_name[256];
				uint4 t_native_length;
				t_native_length = 256;
				MCS_utf16tonative((const unsigned short *)t_names[t_i] . unicode, t_names[t_i] . length, t_native_name, t_native_length);
				
				// MW-2008-02-27: [[ Bug 5920 ]] Make sure we convert Finder to POSIX style paths
				for(uint4 i = 0; i < t_native_length; ++i)
					if (t_native_name[i] == '/')
						t_native_name[i] = ':';
				
				char t_buffer[512];
				if (p_detailed)
				{ // the detailed|long files
					FSPermissionInfo *t_permissions;
					t_permissions = (FSPermissionInfo *)&(t_catalog_infos[t_i] . permissions);
				
					t_tmp_context . copysvalue(t_native_name, t_native_length);
					MCU_urlencode(t_tmp_context);
				
					char t_filetype[9];
					if (!t_is_folder)
					{
						FileInfo *t_file_info;
						t_file_info = (FileInfo *) &t_catalog_infos[t_i] . finderInfo;
						uint4 t_creator;
						t_creator = MCSwapInt32NetworkToHost(t_file_info -> fileCreator);
						uint4 t_type;
						t_type = MCSwapInt32NetworkToHost(t_file_info -> fileType);
						
						if (t_file_info != NULL)
						{
							memcpy(t_filetype, (char*)&t_creator, 4);
							memcpy(&t_filetype[4], (char *)&t_type, 4);
							t_filetype[8] = '\0';
						}
						else
							t_filetype[0] = '\0';
					} else
						strcpy(t_filetype, "????????"); // this is what the "old" getentries did	

					CFAbsoluteTime t_creation_time;
					UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . createDate, &t_creation_time);
					t_creation_time += kCFAbsoluteTimeIntervalSince1970;

					CFAbsoluteTime t_modification_time;
					UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . contentModDate, &t_modification_time);
					t_modification_time += kCFAbsoluteTimeIntervalSince1970;

					CFAbsoluteTime t_access_time;
					UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . accessDate, &t_access_time);
					t_access_time += kCFAbsoluteTimeIntervalSince1970;

					CFAbsoluteTime t_backup_time;
					if (t_catalog_infos[t_i] . backupDate . highSeconds == 0 && t_catalog_infos[t_i] . backupDate . lowSeconds == 0 && t_catalog_infos[t_i] . backupDate . fraction == 0)
						t_backup_time = 0;
					else
					{
						UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . backupDate, &t_backup_time);
						t_backup_time += kCFAbsoluteTimeIntervalSince1970;
					}

					sprintf(t_buffer, "%*.*s,%llu,%llu,%.0lf,%.0lf,%.0lf,%.0lf,%d,%d,%03o,%.8s",
						t_tmp_context . getsvalue() . getlength(),  
					    t_tmp_context . getsvalue() . getlength(),  
					   	t_tmp_context . getsvalue() . getstring(),
						t_catalog_infos[t_i] . dataLogicalSize,
						t_catalog_infos[t_i] . rsrcLogicalSize,
						t_creation_time,
						t_modification_time,
						t_access_time,
						t_backup_time,
						t_permissions -> userID,
						t_permissions -> groupID,
						t_permissions -> mode & 0777,
						t_filetype);
						
					/* UNCHECKED */ MCListAppendCString(*t_list, t_buffer);
				}
				else
					/* UNCHECKED */ MCListAppendNativeChars(*t_list, (const char_t *)t_native_name, t_native_length);
					
				t_entry_count += 1;		
			}
		}	
	} while(t_oserror != errFSNoMoreItems);
	
	FSCloseIterator(t_catalog_iterator);
	return MCListCopy(*t_list, r_list);
#endif /* MCS_getentries_dsk_mac_dsk_mac */
        MCAutoListRef t_list;
        if (!MCListCreateMutable('\n', &t_list))
            return false;
        
        OSStatus t_os_status;
        
        Boolean t_is_folder;
        FSRef t_current_fsref;
        
        t_os_status = FSPathMakeRef((const UInt8 *)".", &t_current_fsref, &t_is_folder);
        if (t_os_status != noErr || !t_is_folder)
            return false;
        
        // Create the iterator, pass kFSIterateFlat to iterate over the current subtree only
        FSIterator t_catalog_iterator;
        t_os_status = FSOpenIterator(&t_current_fsref, kFSIterateFlat, &t_catalog_iterator);
        if (t_os_status != noErr)
            return false;
        
        uint4 t_entry_count;
        t_entry_count = 0;
        
        if (!p_files)
        {
            t_entry_count++;
            /* UNCHECKED */ MCListAppendCString(*t_list, "..");
        }
        
        ItemCount t_max_objects, t_actual_objects;
        t_max_objects = CATALOG_MAX_ENTRIES;
        t_actual_objects = 0;
        FSCatalogInfo t_catalog_infos[CATALOG_MAX_ENTRIES];
        HFSUniStr255 t_names[CATALOG_MAX_ENTRIES];
        
        FSCatalogInfoBitmap t_info_bitmap;
        t_info_bitmap = kFSCatInfoAllDates |
        kFSCatInfoPermissions |
        kFSCatInfoUserAccess |
        kFSCatInfoFinderInfo |
        kFSCatInfoDataSizes |
        kFSCatInfoRsrcSizes |
        kFSCatInfoNodeFlags;
        
        MCExecPoint t_tmp_context(NULL, NULL, NULL);
        OSErr t_oserror;
        do
        {
            t_oserror = FSGetCatalogInfoBulk(t_catalog_iterator, t_max_objects, &t_actual_objects, NULL, t_info_bitmap, t_catalog_infos, NULL, NULL, t_names);
            if (t_oserror != noErr && t_oserror != errFSNoMoreItems)
            {	// clean up and exit
                FSCloseIterator(t_catalog_iterator);
                return false;
            }
            
            for(uint4 t_i = 0; t_i < (uint4)t_actual_objects; t_i++)
            {
                // folders
                UInt16 t_is_folder;
                t_is_folder = t_catalog_infos[t_i] . nodeFlags & kFSNodeIsDirectoryMask;
                if ( (!p_files && t_is_folder) || (p_files && !t_is_folder))
                {
                    char t_native_name[256];
                    uint4 t_native_length;
                    t_native_length = 256;
                    MCS_utf16tonative((const unsigned short *)t_names[t_i] . unicode, t_names[t_i] . length, t_native_name, t_native_length);
                    
                    // MW-2008-02-27: [[ Bug 5920 ]] Make sure we convert Finder to POSIX style paths
                    for(uint4 i = 0; i < t_native_length; ++i)
                        if (t_native_name[i] == '/')
                            t_native_name[i] = ':';
                    
                    char t_buffer[512];
                    if (p_detailed)
                    { // the detailed|long files
                        FSPermissionInfo *t_permissions;
                        t_permissions = (FSPermissionInfo *)&(t_catalog_infos[t_i] . permissions);
                        
                        t_tmp_context . copysvalue(t_native_name, t_native_length);
                        MCU_urlencode(t_tmp_context);
                        
                        char t_filetype[9];
                        if (!t_is_folder)
                        {
                            FileInfo *t_file_info;
                            t_file_info = (FileInfo *) &t_catalog_infos[t_i] . finderInfo;
                            uint4 t_creator;
                            t_creator = MCSwapInt32NetworkToHost(t_file_info -> fileCreator);
                            uint4 t_type;
                            t_type = MCSwapInt32NetworkToHost(t_file_info -> fileType);
                            
                            if (t_file_info != NULL)
                            {
                                memcpy(t_filetype, (char*)&t_creator, 4);
                                memcpy(&t_filetype[4], (char *)&t_type, 4);
                                t_filetype[8] = '\0';
                            }
                            else
                                t_filetype[0] = '\0';
                        } else
                            strcpy(t_filetype, "????????"); // this is what the "old" getentries did
                        
                        CFAbsoluteTime t_creation_time;
                        UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . createDate, &t_creation_time);
                        t_creation_time += kCFAbsoluteTimeIntervalSince1970;
                        
                        CFAbsoluteTime t_modification_time;
                        UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . contentModDate, &t_modification_time);
                        t_modification_time += kCFAbsoluteTimeIntervalSince1970;
                        
                        CFAbsoluteTime t_access_time;
                        UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . accessDate, &t_access_time);
                        t_access_time += kCFAbsoluteTimeIntervalSince1970;
                        
                        CFAbsoluteTime t_backup_time;
                        if (t_catalog_infos[t_i] . backupDate . highSeconds == 0 && t_catalog_infos[t_i] . backupDate . lowSeconds == 0 && t_catalog_infos[t_i] . backupDate . fraction == 0)
                            t_backup_time = 0;
                        else
                        {
                            UCConvertUTCDateTimeToCFAbsoluteTime(&t_catalog_infos[t_i] . backupDate, &t_backup_time);
                            t_backup_time += kCFAbsoluteTimeIntervalSince1970;
                        }
                        
                        sprintf(t_buffer, "%*.*s,%llu,%llu,%.0lf,%.0lf,%.0lf,%.0lf,%d,%d,%03o,%.8s",
                                t_tmp_context . getsvalue() . getlength(),  
                                t_tmp_context . getsvalue() . getlength(),  
                                t_tmp_context . getsvalue() . getstring(),
                                t_catalog_infos[t_i] . dataLogicalSize,
                                t_catalog_infos[t_i] . rsrcLogicalSize,
                                t_creation_time,
                                t_modification_time,
                                t_access_time,
                                t_backup_time,
                                t_permissions -> userID,
                                t_permissions -> groupID,
                                t_permissions -> mode & 0777,
                                t_filetype);
						
                        /* UNCHECKED */ MCListAppendCString(*t_list, t_buffer);
                    }
                    else
					/* UNCHECKED */ MCListAppendNativeChars(*t_list, (const char_t *)t_native_name, t_native_length);
					
                    t_entry_count += 1;		
                }
            }	
        } while(t_oserror != errFSNoMoreItems);
        
        FSCloseIterator(t_catalog_iterator);
        return MCListCopy(*t_list, r_list);
        
    }
    
    virtual real8 GetFreeDiskSpace()
    {
        
    }
    
    virtual Boolean GetDevices(MCStringRef& r_devices)
    {
        
    }
    
    virtual Boolean GetDrives(MCStringRef& r_drives)
    {
    
    }
    
	virtual bool PathToNative(MCStringRef p_path, MCStringRef& r_native)
    {
        
    }
    
	virtual bool PathFromNative(MCStringRef p_native, MCStringRef& r_path)
    {
        
    }
    
	virtual bool ResolvePath(MCStringRef p_path, MCStringRef& r_resolved_path)
    {
        
    }
	virtual bool ResolveNativePath(MCStringRef p_path, MCStringRef& r_resolved_path)
    {
        
    }
	
	virtual MCSystemFileHandle *OpenFile(MCStringRef p_path, uint32_t p_mode, bool p_map)
    {
        
    }
	virtual MCSystemFileHandle *OpenStdFile(uint32_t i)
    {
        
    }
	virtual MCSystemFileHandle *OpenDevice(MCStringRef p_path, uint32_t p_mode, MCStringRef p_control_string)
    {
        
    }
	
	virtual void *LoadModule(MCStringRef p_path)
    {
        
    }
	virtual void *ResolveModuleSymbol(void *p_module, MCStringRef p_symbol)
    {
        
    }
	virtual void UnloadModule(void *p_module)
    {
        
    }
	
	virtual bool LongFilePath(MCStringRef p_path, MCStringRef& r_long_path)
    {
        
    }
	virtual bool ShortFilePath(MCStringRef p_path, MCStringRef& r_short_path)
    {
        
    }
	
	virtual IO_stat MCS_runcmd(MCStringRef command, MCStringRef& r_output)
    {
        
    }
    
	virtual char *GetHostName(void)
    {
        
    }
	virtual bool HostNameToAddress(MCStringRef p_hostname, MCSystemHostResolveCallback p_callback, void *p_context)
    {
        
    }
	virtual bool AddressToHostName(MCStringRef p_address, MCSystemHostResolveCallback p_callback, void *p_context)
    {
        
    }
    
	virtual uint32_t TextConvert(const void *string, uint32_t string_length, void *buffer, uint32_t buffer_length, uint32_t from_charset, uint32_t to_charset)
    {
        
    }
	virtual bool TextConvertToUnicode(uint32_t p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used)
    {
        
    }
};

////////////////////////////////////////////////////////////////////////////////

MCSystemInterface *MCDesktopCreateMacSystem(void)
{
	return new MCMacDesktop;
}

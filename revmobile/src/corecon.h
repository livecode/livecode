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

#ifndef __CORECON__
#define __CORECON__

class ICcPropertyContainer;

class __declspec(uuid("4710E236-63E2-4DE8-865C-840722C84A09")) ICcObject: public IDispatch
{
public:
    STDMETHOD(get_Name)(BSTR* out_pbstrName) = 0;
    STDMETHOD(put_Name)(BSTR in_bstrName) = 0;
    STDMETHOD(get_ID)(BSTR* out_pbstrID) = 0;
    STDMETHOD(get_IsProtected)(VARIANT_BOOL* out_pbIsProtected) = 0;
    STDMETHOD(get_PropertyContainer)(ICcPropertyContainer** out_ppiPC) = 0;
};

class __declspec(uuid("6F3923AE-6495-4A78-9CB1-494B7397C388")) ICcCollection: public IDispatch
{
public:
    STDMETHOD(get_Count)(long* pVal) = 0;
    STDMETHOD(get_Item)(long index, ICcObject** pVal) = 0;
    STDMETHOD(get_NewEnum)(IUnknown** pVal) = 0;
};

class __declspec(uuid("26D1CA67-824A-403B-A218-151F9B2271D1")) ICcObjectContainer: public IDispatch
{
public:
    STDMETHOD(FindObject)(BSTR in_bstrNameOrID, ICcObject** out_ppiObject) = 0;
    STDMETHOD(EnumerateObjects)(ICcCollection** out_ppiCollection) = 0;
    STDMETHOD(AddObject)(BSTR in_bstrName, BSTR in_bstrID, ICcObject** out_ppiObject) = 0;
    STDMETHOD(DeleteObject)(BSTR in_bstrNameOrID) = 0;
};

class __declspec(uuid("9A304B61-E054-47D8-82A5-3A2B960A9858")) ICcPropertyContainer : public IDispatch
{
}; 

class __declspec(uuid("19F4FCE7-CA26-48FA-A905-168E9FF32A0A")) ICcServiceInfo : public IDispatch
{
    //TODO
}; 

class __declspec(uuid("A9DD4ABA-C0DC-4FE2-8A01-D7493C7352DD")) ICcService : public IUnknown
{
public:
    STDMETHOD(Abort)(DWORD dwCookieId);
    STDMETHOD(CategoryId)(BSTR* pbstrServiceCategoryId);
    STDMETHOD(CLSID)(BSTR* pbstrServiceCLSID);
    STDMETHOD(Connect)(DWORD dwTimeout, VARIANT_BOOL bAsync, DWORD* pdwCookieId);
    STDMETHOD(Device)(BSTR* pbstrDeviceId);
    STDMETHOD(Disconnect)();
    STDMETHOD(GetCurrentState)(DWORD* pdwCurrentState);
    STDMETHOD(Initialize)(LPCOLESTR pwszDeviceId);
    STDMETHOD(LockService)();
    STDMETHOD(UnlockService)();
}; 


class __declspec(uuid("8F023C2C-3A42-4DE9-B655-0061455C4566")) ICcDevice: public IDispatch
{
    STDMETHOD(SetServiceMap)(BSTR in_bstrServiceCategoryID, BSTR in_bstrServiceInfoID) = 0;
    STDMETHOD(ClearServiceMap)(BSTR in_bstrServiceCategoryID) = 0;
    STDMETHOD(SetOSImage)(BSTR in_bstrOSImage) = 0;
    STDMETHOD(ClearOSImage)() = 0;
    STDMETHOD(GetOSImage)(HRESULT GetOSImage) = 0;
    STDMETHOD(GetServiceMap)(BSTR in_bstrServiceCategoryID, ICcServiceInfo** out_ppiServiceInfo) = 0;
};

class __declspec(uuid("80F8C92F-A3D6-4EDE-80AC-05845C77C598")) ICcServiceCB : public IDispatch
{
public:
    STDMETHOD(OnProgressNotify)(DWORD dwPreviousState, DWORD dwCurrentState, DWORD dwCurrentValue, DWORD dwMaxValue) = 0;
    STDMETHOD(OnStatusChangeNotify)(DWORD dwPreviousState, DWORD dwCurrentState, LPCOLESTR pwszStringData, IUnknown* pUnk) = 0;
};

class __declspec(uuid("720925ED-BF3D-4B1A-9404-5142C4DF799C")) ICcTransportStream: public IDispatch
{
    //TODO
};

typedef struct tagFileInfo{
  LONG m_FileAttribues;
  LONGLONG m_FileSize;
  FILETIME m_CreationTime;
  FILETIME m_LastAccessTime;
  FILETIME m_LastWriteTime;
} FileInfo;

typedef struct tagPlatformInfo{
  DWORD m_OSMajor;
  DWORD m_OSMinor;
  DWORD m_BuildNo;
  DWORD m_ProcessorArchitecture;
  DWORD m_InstructionSet;
} SystemInfo;

typedef enum tagFileCustomAction{
  FileCustomAction_None = 0x0000,
  FileCustomAction_COM = 0x0001,
} FileAction;

typedef struct tagFileVerifyVersion{
  DWORD m_Major;
  DWORD m_Minor;
  DWORD m_Build;
  DWORD m_Revision;
} FileVerifyVersion;

#define PUBLIC_KEY_TOKEN_LENGTH 8

typedef struct tagFileVerifyInfo{
  FileVerifyVersion m_AssemblyVersion;
  FileVerifyVersion m_Win32Version;
  BSTR m_Culture;
  BYTE m_PublicKeyToken[PUBLIC_KEY_TOKEN_LENGTH];
  DWORD m_Flags;
} FileVerifyInfo;

typedef struct tagFileVerifyReference{
  BSTR m_Name;
  BSTR m_SourcePath;
  FileVerifyInfo m_Info;
} FileVerifyReference;

typedef struct tagFileVerifyResult{
  DWORD m_Version;
  FileVerifyInfo _Info;
} FileVerifyResult;

class __declspec(uuid("1CDD94DC-394B-4A94-88C1-F5440A1FB55B")) ICcConnection: public IDispatch
{
public:
    STDMETHOD(DeviceId)/*1*/(BSTR* pbstrDeviceId ) = 0;
    STDMETHOD(GetSystemInfo)/*1*/(SystemInfo *out_pSystemInfo) = 0;
    STDMETHOD(SendFile)/*5*/(LPCOLESTR in_szDesktopFile, LPCOLESTR in_szDeviceFile, /*FileActionDWORD in_FileAction,*/ DWORD dwCreationDisposition, LPCOLESTR in_FileCustomAction) = 0;
    STDMETHOD(ReceiveFile)/*3*/(LPCOLESTR in_szDeviceFile, LPCOLESTR in_szDesktopFile, /*FileActionDWORD*/DWORD dwCreationDispositionin_FileAction) = 0;
    STDMETHOD(RemoveFile)/*1*/(LPCOLESTR in_szDeviceFile) = 0;
    STDMETHOD(GetFileInfo)/*2*/(LPCOLESTR in_szDeviceFile, FileInfo* out_pFileInfo) = 0;
    STDMETHOD(SetFileInfo)/*2*/(LPCOLESTR in_szDeviceFile, FileInfo* out_pFileInfo) = 0;    
    STDMETHOD(DownloadPackage)/*1*/(LPCOLESTR pwszPackageId) = 0;
    STDMETHOD(MakeDirectory)/*1*/(LPCOLESTR in_szDeviceDirectory) = 0;
    STDMETHOD(DeleteDirectory)/*2*/(LPCOLESTR in_szDeviceDirectory, VARIANT_BOOL bRemoveAll) = 0;
    STDMETHOD(LaunchProcess)/*5*/(LPCOLESTR in_szExecutable, LPCOLESTR in_szArgument, DWORD in_CreationFlags, DWORD* out_pProcessID, DWORD* out_pProcessHandle) = 0;
    STDMETHOD(TerminateProcess)/*1*/(DWORD in_ProcessID) = 0;
    STDMETHOD(GetProcessExitCode)/*3*/(DWORD in_ProcessID, VARIANT_BOOL* out_pbProcessExited,DWORD* out_pdwProcessExited) = 0;
    STDMETHOD(RegistryCreateKey)/*2*/(LONG hKey, LPCOLESTR lpSubKey) = 0;
    STDMETHOD(RegistryDeleteKey)/*2*/(LONG hKey, LPCOLESTR lpSubKey) = 0;
    STDMETHOD(RegistrySetValue)/*6*/(LONG hKey, LPCOLESTR lpSubKey, LPCOLESTR lpValueName, DWORD dwType, LPCOLESTR lpData, DWORD cbData) = 0;
    STDMETHOD(RegistryQueryValue)/*6*/(LONG hKey, LPCOLESTR lpSubKey, LPCOLESTR lpValueName, DWORD *dwType, WCHAR lpValue[], LONG* pcbValue) = 0;
    STDMETHOD(RegistryDeleteValue)/*3*/(LONG hKey, LPCOLESTR lpSubKey, LPCOLESTR lpValueName) = 0;
    STDMETHOD(IsConnected)/*1*/(VARIANT_BOOL* pbConnected) = 0;
    STDMETHOD(CreateStream)/*5*/(BSTR bstrStreamId, DWORD dwTimeout, ICcServiceCB* piCallback, DWORD* pdwCookieId, ICcTransportStream** piStream) = 0;
    STDMETHOD(ConnectDevice)() = 0;
    STDMETHOD(DisconnectDevice)() = 0;
    STDMETHOD(VerifyFilesInstalled)/*3*/(DWORD in_ArraySize, FileVerifyReference in_InfoArray[], FileVerifyResult out_ExistenceArray[]) = 0;
    STDMETHOD(SearchFileSystem)/*3*/(LPCOLESTR in_Criteria, LPCOLESTR in_StartingDirectory, SAFEARRAY/*( BSTR )*/* out_FileSystems) = 0;
    STDMETHOD(EnumerateProcesses)/*2*/(SAFEARRAY/*(BSTR)*/* out_Processes, SAFEARRAY/*(DWORD)*/* out_ProcessesId) = 0; 
    STDMETHOD(CloseProcessHandle)/*1*/(DWORD in_ProcessHandle) = 0;    
};

class __declspec(uuid("980E8361-8EDA-41BA-8D47-64BC57C9414A")) ICcDeviceContainer: public IDispatch
{
};

class __declspec(uuid("C069A16A-576C-4917-9452-1D244C925A20")) ICcOSImageContainer : public IDispatch
{
};


class __declspec(uuid("771F5AAE-7100-4764-9BE8-D3110E38EAAC")) ICcPlatformContainer : public IDispatch
{
}; 

class __declspec(uuid("4491E81C-5661-4079-A994-826B512F3AB2")) ICcPackageContainer : public IDispatch
{
}; 

class __declspec(uuid("E2C748E9-C19F-48C8-B708-77BA9E116488")) ICcFormFactorContainer : public IDispatch
{
}; 

class __declspec(uuid("5846D7CA-0B42-4CB9-94D3-6E5A04B24056")) ICcServiceCategoryContainer : public IDispatch
{
};

class __declspec(uuid("10534DCE-69BC-4FFB-A441-716D1AE73087")) ICcProjectContainer : public IDispatch
{
};

class __declspec(uuid("52353EA6-0E12-400C-9D75-DDD5DA72141F")) ICcDatastore: public IDispatch
{
public:
    STDMETHOD(Save)() = 0;
    STDMETHOD(RegisterRefreshEvent)(BSTR in_bstrEventName) = 0;
    STDMETHOD(UnregisterRefreshEvent)() = 0;
    STDMETHOD(get_DeviceContainer)(ICcDeviceContainer** out_ppiDC) = 0;
    STDMETHOD(get_OSImageContainer)(ICcOSImageContainer** out_ppiOC) = 0;
    STDMETHOD(get_PackageContainer)(ICcPackageContainer** out_ppiPC) = 0;
    STDMETHOD(get_PlatformContainer)(ICcPlatformContainer** out_ppiPC) = 0;
    STDMETHOD(get_PropertyContainer)(ICcPropertyContainer** out_ppiSCC) = 0;
    STDMETHOD(get_ServiceCategoryContainer)(ICcServiceCategoryContainer** out_ppiSCC) = 0;
    STDMETHOD(Version)(BSTR* out_pbstrVersion) = 0;
};

class __declspec(uuid("3183E9CF-37E5-41E8-9082-39780D626F13")) ICcServer: public IDispatch
{
public:
    STDMETHOD(get_Locale)(DWORD* pdwLocale) = 0;
    STDMETHOD(put_Locale)(DWORD dwLocale) = 0;
    STDMETHOD(GetDatastore)(DWORD dwLocale, ICcDatastore** ppiDatastore) = 0;
    STDMETHOD(GetConnection)(ICcDevice* piDevice, DWORD dwTimeout, ICcServiceCB* piCallback, BSTR* pbstrConnectionId, ICcConnection** piTargetDevice) = 0;
    STDMETHOD(EnumerateConnections)(DWORD dwSizeActual, DWORD* pdwSizeReturned, BSTR *Connections, VARIANT_BOOL* pfMoreEntries) = 0;
    STDMETHOD(GetConnectionFromId)(BSTR bstrConnectionId, ICcConnection** ppiConnection) = 0;
};

class __declspec(uuid("F248445A-D48E-416D-8089-6D78EF55ADE7")) IConManServer: public IDispatch
{
    //TODO
};

enum DeviceInfo {
     SYSTEM_INFO_PROCESSOR_ARCHITECTURE,
     SYSTME_INFO_PAGE_SIZE,
     SYSTEM_INFO_ACTIVE_PROCESSOR_MASK,
     SYSTEM_INFO_NUMBER_OF_PROCESSORS,
     SYSTEM_INFO_PROCESSOR_TYPE,
     SYSTEM_INFO_ALLOCATION_GRANULARITY,
     SYSTEM_INFO_PROCESSOR_LEVEL,
     SYSTEM_INFO_PROCESSOR_REVISION,
     SYSTEM_INFO_QUERY_INSTRUCTION_SET,
     OSVERSIONINFO_MAJOR_VERSION,
     OSVERSIONINFO_MINOR_VERSION,
     OSVERSIONINFO_BUILD_NUMBER,
     OSVERSIONINFO_PLATFORM_ID
};

#define KTS_DEFAULT 0
#define KTS_ETHER 1
#define KTS_SERIAL 2
#define KTS_USB 3
#define KTS_NONE 63
#define KTS_PASSIVE_MODE 0x40 


class __declspec(uuid("D714154B-29D6-4AC2-985C-24D8A1AE3964")) ICcBootstrap: public IDispatch
{
public:
    STDMETHOD(Download)(DWORD dwTimeout, VARIANT_BOOL bAsync, LPCOLESTR wszSrcFullPath, LPCOLESTR wszDestFullPath, VARIANT_BOOL bOverwrite, DWORD* pdwCookieId) = 0;
    STDMETHOD(GetDeviceInfo)(DeviceInfo diRequested, DWORD* pdwPropertyValue) = 0;
    STDMETHOD(GetFile)(DWORD dwTimeout, LPCOLESTR wszSrcFullPath, LPCOLESTR wszDestFullPath, VARIANT_BOOL bOverwrite) = 0;
    STDMETHOD(GetFileInfo)(LPCOLESTR wszDeviceFile, FileInfo* pFileInfo) = 0;
    STDMETHOD(Launch)(BSTR bstrCommand, LPCOLESTR bstrArguments, DWORD dwLaunchFlags) = 0;
};

class __declspec(uuid("12FCCB25-D251-4E65-A055-9472FBEB862C")) IConMan : public IUnknown
{
public:
    STDMETHOD(SetLocale)(DWORD in_dwLangID) = 0;
    STDMETHOD(SetRegistryRoot)(BSTR in_bstrRegistryRoot) = 0;
    STDMETHOD(CreateNewServerGuid)(ICcDevice* in_piDevice, BSTR in_bstrEndPoint, GUID* out_pguidServer) = 0;
    STDMETHOD(GetServerGuidInfo)(REFGUID in_guidServer, BSTR* out_pbstrEndPoint) = 0;
    STDMETHOD(GetServerFromServerGuid)(REFGUID in_guidServer, IConManServer** out_ppunkServer) = 0;
    STDMETHOD(GetDeviceFromServerGuid)(REFGUID in_guidServer, ICcDevice** out_ppiDevice) = 0;
    STDMETHOD(EnumerateServerGuids)(DWORD in_ArraySize, DWORD* out_pNoOfServerGuids, GUID out_ArrayOfServerGuids[ ], BOOL* out_pbMoreServerGuids) = 0;
    STDMETHOD(get_Datastore)(ICcDatastore** out_ppiDatastore) = 0;
};

class __declspec(uuid("1D7A8E8E-FAFC-4AA8-98A0-E7AF3F44199E")) ICcPlatform: public IDispatch
{
public:
    STDMETHOD(get_ProjectContainer)(ICcProjectContainer** out_ppiPC) = 0;
    STDMETHOD(get_DeviceContainer)(ICcDeviceContainer** out_ppiDC) = 0;
    STDMETHOD(get_FormFactorContainer)(ICcFormFactorContainer** out_ppiFC) = 0;
};

struct __declspec(uuid("74AD2302-A606-428E-B40F-F04B8964ADB6"))
VsdConnectionManager;
//DEFINE_GUID(CLSID_VsdConnectionManager, 0x74AD2302, 0xA606, 0x428e, 0xB4, 0x0F, 0xF0, 0x4B, 0x89, 0x64, 0xAD, 0xB6);

#endif

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

#include <winsock2.h>
#include <windows.h>
#include <rapi2.h>
#include <stdio.h>
#include <process.h>

#include "corecon.h"
#include "devemu.h"
#include "comutil.h"

#include "external.h"

////////////////////////////////////////////////////////////////////////////////

const char *g_error_message = nil;

bool Throw(const char *p_message)
{
	g_error_message = p_message;
	return false;
}

void Catch(MCVariableRef result)
{
	VariableFormat(result, "%s", g_error_message);
}

bool CheckError(MCError err)
{
	if (err == kMCErrorNone)
		return true;
	return Throw("error");
}

bool VariableFormat(MCVariableRef var, const char *p_format, ...)
{
	va_list t_args;
	int t_count;
	va_start(t_args, p_format);
	t_count = _vscprintf(p_format, t_args);
	va_end(t_args);

	char *t_new_string;
	t_new_string = new char[t_count + 1];
	if (t_new_string == nil)
		return false;

	va_start(t_args, p_format);
	vsprintf(t_new_string, p_format, t_args);
	va_end(t_args);

	MCError t_error;
	t_error = MCVariableStore(var, kMCOptionAsCString, &t_new_string);

	delete[] t_new_string;

	return t_error == kMCErrorNone;
}

bool VariableAppendFormat(MCVariableRef var, const char *p_format, ...)
{
	va_list t_args;
	int t_count;
	va_start(t_args, p_format);
	t_count = _vscprintf(p_format, t_args);
	va_end(t_args);

	char *t_new_string;
	t_new_string = new char[t_count + 1];
	if (t_new_string == nil)
		return false;

	va_start(t_args, p_format);
	vsprintf(t_new_string, p_format, t_args);
	va_end(t_args);

	MCError t_error;
	t_error = MCVariableAppend(var, kMCOptionAsCString, &t_new_string);

	delete[] t_new_string;

	return t_error == kMCErrorNone;
}

bool IsRefVariable(MCVariableRef var)
{
	bool t_is_ref;
	if (MCVariableIsTransient(var, &t_is_ref) != kMCErrorNone)
		return false;
	return !t_is_ref;
}

////////////////////////////////////////////////////////////////////////////////

bool AnsiToWide(const char *p_ansi, LPWSTR& r_wide)
{
	LPWSTR t_wide_buffer;
	t_wide_buffer = new WCHAR[strlen(p_ansi) + 1];
	MultiByteToWideChar(1252, 0, p_ansi, -1, t_wide_buffer, strlen(p_ansi) + 1);
	r_wide = t_wide_buffer;
	return true;
}

bool GetWideArg(MCVariableRef arg, LPWSTR& r_filename)
{
	bool t_success;
	t_success = true;

	const char *t_filename;
	if (t_success)
		t_success = CheckError(MCVariableFetch(arg, kMCOptionAsCString, &t_filename));

	if (t_success)
		t_success = AnsiToWide(t_filename, r_filename);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

IDeviceEmulatorManager *s_dem = nil;

bool revMobileInitializeEmulators(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	// Do nothing if already initialized
	if (s_dem != nil)
		return true;

	bool t_success;
	t_success = true;

	ComPointer<IDeviceEmulatorManager> t_dem;
	if (t_success)
		t_success = t_dem . CreateInstance<DeviceEmulatorManager>();

	if (t_success)
		s_dem = t_dem . Take();

	if (!t_success)
		Catch(result);

	return t_success;

}

bool revMobileFinalizeEmulators(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	if (s_dem != nil)
	{
		s_dem -> Release();
		s_dem = nil;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

typedef bool (*ForEachEmulatorCallback)(void *context, BSTR node, BSTR sdk, IDeviceEmulatorManagerVMID *device, bool& r_continue);

static bool ForEachEmulator(ForEachEmulatorCallback p_callback, void *p_context)
{
	bool t_success;
	t_success = true;

	if (s_dem == nil)
		t_success = Throw("emulator manager not initialized");

	if (t_success)
	{
		s_dem -> Refresh();
		s_dem -> Reset();
	}

	bool t_continue;
	t_continue = true;

	while(t_success && t_continue)
	{
		ComString t_node_name;
		if (t_success)
			t_success = ComCheck(s_dem -> get_Name(&t_node_name));

		ComPointer<IEnumManagerSDKs> t_sdk_enum;
		if (t_success)
			t_success = ComCheck(s_dem -> EnumerateSDKs(&t_sdk_enum));

		while(t_success && t_continue)
		{
			ComString t_sdk_name;
			bool t_sdk_loaded;
			t_sdk_loaded = true;
			if (t_success)
			{
				HRESULT hr;
				hr = t_sdk_enum -> get_Name(&t_sdk_name);
				if (hr == E_ENUMSDK_NOT_LOADED)
					t_sdk_loaded = false;
				else if (hr != S_OK)
					t_success = false;
			}

			if (t_sdk_loaded)
			{
				ComPointer<IEnumVMIDs> t_device_enum;
				if (t_success)
					t_success = ComCheck(t_sdk_enum -> EnumerateVMIDs(&t_device_enum));

				while(t_success && t_continue)
				{
					ComPointer<IDeviceEmulatorManagerVMID> t_device;
					bool t_vmid_loaded;
					t_vmid_loaded = true;
					if (t_success)
					{
						HRESULT hr;
						hr = t_device_enum -> GetVMID(&t_device);
						if (hr == E_ENUMVMID_NOT_LOADED || hr == E_ENUMVMID_INVALID_VMID)
							t_vmid_loaded = false;
						else if (hr != S_OK)
							t_success = false;
					}

					if (t_success && t_vmid_loaded)
						t_success = p_callback(p_context, t_node_name, t_sdk_name, t_device, t_continue);

					// When this call fails we've reached the end of the list
					if (t_device_enum -> MoveNext() != S_OK)
						break;
				}
			}

			// When this call fails we've reached the end of the list
			if (t_sdk_enum -> MoveNext() != S_OK)
				break;
		}

		// When this call fails we've reached the end of the list
		if (s_dem -> MoveNext() != S_OK)
			break;
	}

	return t_success;
}

/////////

static bool ListEmulatorsCallback(void *context, BSTR p_node, BSTR p_sdk, IDeviceEmulatorManagerVMID *p_device, bool& r_continue)
{
	bool t_success;
	t_success = true;

	ComString t_device_name, t_device_id;
	EMULATOR_STATE t_device_state;
	if (t_success)
		t_success =
			ComCheck(p_device -> get_Name(&t_device_name)) &&
			ComCheck(p_device -> get_VMID(&t_device_id)) &&
			ComCheck(p_device -> get_State(&t_device_state));

	// If we are successful then we have node, sdk and device names
	if (t_success)
	{
		static const char *s_device_states[] = { "not running", "running", "cradled" };
		t_success = VariableAppendFormat((MCVariableRef)context, "%S,%S,%S,%S,%s\n", p_node, p_sdk, t_device_name, t_device_id, s_device_states[t_device_state]);
	}

	r_continue = true;

	return t_success;
}

/////////

struct FindEmulatorContext
{
	LPWSTR id;
	IDeviceEmulatorManagerVMID *device;
};

static bool FindEmulatorCallback(void *p_context, BSTR p_node, BSTR p_sdk, IDeviceEmulatorManagerVMID *p_device, bool& r_continue)
{
	FindEmulatorContext *context;
	context = (FindEmulatorContext *)p_context;

	bool t_success;
	t_success = true;

	ComString t_device_id;
	if (t_success)
		t_success = ComCheck(p_device -> get_VMID(&t_device_id));

	if (t_success &&
		_wcsicmp(context -> id, t_device_id) == 0)
	{
		context -> device = p_device;
		p_device -> AddRef();
		r_continue = false;
	}

	return t_success;
}

static bool FindEmulator(LPWSTR p_id, IDeviceEmulatorManagerVMID*& r_device)
{
	bool t_success;
	t_success = true;

	FindEmulatorContext t_context;
	t_context . id = p_id;
	t_context . device = nil;
	t_success = ForEachEmulator(FindEmulatorCallback, &t_context);

	r_device = t_context . device;

	return t_success;	
}

/////////

bool revMobileListEmulators(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	if (t_success)
		t_success = ForEachEmulator(ListEmulatorsCallback, result);

	if (!t_success)
		Catch(result);

	return t_success;
}

bool revMobileStartEmulator(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	if (argc != 1)
		t_success = Throw("syntax error");

	const char *t_id_ansi;
	if (t_success)
		t_success = CheckError(MCVariableFetch(argv[0], kMCOptionAsCString, &t_id_ansi));

	LPWSTR t_id;
	t_id = nil;
	if (t_success)
		t_success = AnsiToWide(t_id_ansi, t_id);

	IDeviceEmulatorManagerVMID *t_device;
	if (t_success)
		t_success = FindEmulator(t_id, t_device);

	if (t_device != nil)
	{
		EMULATOR_STATE t_state;
		if (t_success)
			t_success = ComCheck(t_device -> get_State(&t_state));
		if (t_success && t_state == EMU_NOT_RUNNING)
		{
			t_success = ComCheck(t_device -> Connect());
			if (t_success)
				t_success = ComCheck(t_device -> get_State(&t_state));
		}
		if (t_success && t_state == EMU_RUNNING)
			t_success = ComCheck(t_device -> Cradle());
		if (t_success)
			t_success = ComCheck(t_device -> BringToFront());
	}

	if (t_device != nil)
		t_device -> Release();

	delete[] t_id;

	return true;
}

bool revMobileStopEmulator(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	if (argc != 1)
		t_success = Throw("syntax error");

	const char *t_id_ansi;
	if (t_success)
		t_success = CheckError(MCVariableFetch(argv[0], kMCOptionAsCString, &t_id_ansi));

	LPWSTR t_id;
	t_id = nil;
	if (t_success)
		t_success = AnsiToWide(t_id_ansi, t_id);

	IDeviceEmulatorManagerVMID *t_device;
	if (t_success)
		t_success = FindEmulator(t_id, t_device);

	if (t_device != nil)
	{
		EMULATOR_STATE t_state;
		if (t_success)
			t_success = ComCheck(t_device -> get_State(&t_state));
		if (t_success && t_state == EMU_CRADLED)
			t_success = ComCheck(t_device -> UnCradle());
	}

	if (t_device != nil)
		t_device -> Release();

	delete[] t_id;

	return true;
}

////////////////////////////////////////////////////////////////////////////////

static IRAPIDesktop *s_rapi_desktop = nil;
static IRAPISink *s_rapi_sink = nil;
static DWORD s_rapi_sink_context = 0;
static MCObjectRef s_rapi_target = nil;

class RevMobileRapiSink: public IRAPISink
{
public:
	RevMobileRapiSink(void);

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);

	HRESULT STDMETHODCALLTYPE OnDeviceConnected(IRAPIDevice *pIDevice);
    HRESULT STDMETHODCALLTYPE OnDeviceDisconnected(IRAPIDevice *pIDevice);

private:
	LONG m_references;
};

RevMobileRapiSink::RevMobileRapiSink(void)
{
	m_references = 1;
}

HRESULT STDMETHODCALLTYPE RevMobileRapiSink::QueryInterface(REFIID riid, void **ppvObject)
{
	if (riid == __uuidof(IUnknown) || riid == __uuidof(IRAPISink))
	{
		AddRef();
		*ppvObject = this;
		return S_OK;
	}
	
	*ppvObject = nil;
	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE RevMobileRapiSink::AddRef(void)
{
	return InterlockedIncrement(&m_references);
}

ULONG STDMETHODCALLTYPE RevMobileRapiSink::Release(void)
{
	if (InterlockedDecrement(&m_references) != 0)
		return m_references;

	delete this;
	return 0;
}

static void RevMobileRapiSinkCallback(void *p_state)
{
	MCObjectDispatch(s_rapi_target, kMCDispatchTypeCommand, (const char *)p_state, nil, 0, nil);
}

// There is a bug in 4.0.0-gm-1 MCRunOnMainThread that means 'thread later' breaks so we make
// a fixed version that spawns a separate thread and blocks.
//
static void romt_thread(void *context)
{
	MCRunOnMainThread(RevMobileRapiSinkCallback, context, kMCRunOnMainThreadNow);
}

static MCError MCRunOnMainThread_Fix(MCThreadCallback callback, void *state, MCRunOnMainThreadOptions options)
{
	_beginthread(romt_thread, 0, state);
	return kMCErrorNone;
}

HRESULT STDMETHODCALLTYPE RevMobileRapiSink::OnDeviceConnected(IRAPIDevice *p_device)
{
	MCRunOnMainThread_Fix(RevMobileRapiSinkCallback, "deviceConnected", kMCRunOnMainThreadLater);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE RevMobileRapiSink::OnDeviceDisconnected(IRAPIDevice *p_device)
{
	MCRunOnMainThread_Fix(RevMobileRapiSinkCallback, "deviceDisconnected", kMCRunOnMainThreadLater);
	return S_OK;
}

bool revMobileInitializeRapi(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	if (t_success && s_rapi_desktop != nil)
	{
		t_success = Throw("already initialized");
		Catch(result);
		return false;
	}

	ComPointer<IRAPIDesktop> t_desktop;
	if (t_success)
		t_success = t_desktop . CreateInstance<RAPI>();

	RevMobileRapiSink *t_sink;
	t_sink = nil;
	if (t_success)
	{
		t_sink = new RevMobileRapiSink;
		if (t_sink == nil)
			t_success = Throw("no memory");
	}

	MCObjectRef t_target;
	t_target = nil;
	if (t_success)
		t_success = CheckError(MCContextMe(&t_target));

	DWORD t_context;
	t_context = 0;
	if (t_success)
		t_success = ComCheck(t_desktop -> Advise(t_sink, &t_context));

	if (t_success)
	{
		s_rapi_desktop = t_desktop . Take();
		s_rapi_sink = t_sink;
		s_rapi_target = t_target;
		s_rapi_sink_context = t_context;
	}
	else
	{
		if (t_target != nil)
			MCObjectRelease(t_target);

		if (s_rapi_sink != nil)
			s_rapi_sink -> Release();

		Catch(result);
	}

	return t_success;
}

bool revMobileFinalizeRapi(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	if (s_rapi_sink_context != 0)
	{
		s_rapi_desktop -> UnAdvise(s_rapi_sink_context);
		s_rapi_sink_context = 0;
	}

	if (s_rapi_target != nil)
	{
		MCObjectRelease(s_rapi_target);
		s_rapi_target = nil;
	}

	if (s_rapi_sink != nil)
	{
		s_rapi_sink -> Release();
		s_rapi_sink = nil;
	}

	if (s_rapi_desktop != nil)
	{
		s_rapi_desktop -> Release();
		s_rapi_desktop = nil;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

static IRAPIDevice *s_rapi_device = nil;
static IRAPISession *s_rapi_session = nil;

typedef bool (*ListDevicesCallback)(void *context, IRAPIDevice *p_device, bool& r_continue);

static bool ForEachDeviceRapi(ListDevicesCallback p_callback, void *p_context)
{
	bool t_success;
	t_success = true;

	ComPointer<IRAPIDesktop> t_desktop;
	if (t_success)
		t_success = t_desktop . CreateInstance<RAPI>();

	ComPointer<IRAPIEnumDevices> t_devices;
	if (t_success)
		t_success = ComCheck(t_desktop -> EnumDevices(&t_devices));

	bool t_continue;
	t_continue = true;
	while(t_success && t_continue)
	{
		ComPointer<IRAPIDevice> t_device;
		if (t_devices -> Next(&t_device) != S_OK)
			break;

		t_success = p_callback(p_context, t_device, t_continue);
	}

	return t_success;
}

static bool revMobileListDevicesRapiCallback(void *p_context, IRAPIDevice *p_device, bool& r_continue)
{
	bool t_success;
	t_success = true;

	RAPI_DEVICEINFO t_info;
	if (t_success)
		t_success = ComCheck(p_device -> GetDeviceInfo(&t_info));

	if (t_success)
	{
		WCHAR t_id[64];
		StringFromGUID2(t_info . DeviceId, t_id, 64);
		t_success = VariableAppendFormat((MCVariableRef)p_context, "%S,%S,%S\n", t_info . bstrPlatform, t_info . bstrName, t_id);
	}

	r_continue = true;

	return t_success;
}

bool revMobileListDevicesRapi(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	if (t_success)
		t_success = ForEachDeviceRapi(revMobileListDevicesRapiCallback, result);

	if (!t_success)
		Catch(result);

	return true;
}

//////////

struct FindDeviceContext
{
	GUID id;
	IRAPIDevice *device;
};

static bool FindDeviceRapiCallback(void *p_context, IRAPIDevice *p_device, bool& r_continue)
{
	FindDeviceContext *context;
	context = (FindDeviceContext *)p_context;

	bool t_success;
	t_success = true;

	RAPI_DEVICEINFO t_info;
	if (t_success)
		t_success = ComCheck(p_device -> GetDeviceInfo(&t_info));

	if (t_success && t_info . DeviceId == context -> id)
	{
		context -> device = p_device;
		p_device -> AddRef();
		r_continue = false;
	}

	return t_success;
}

bool revMobileConnectRapi(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	if (t_success && argc != 1)
		t_success = false;

	if (t_success && s_rapi_session != nil)
		t_success = false;

	ComPointer<IRAPIDesktop> t_desktop;
	if (t_success)
		t_success = t_desktop . CreateInstance<RAPI>();

	IRAPIDevice *t_device;
	t_device = nil;
	if (t_success)
	{
		const char *t_id_string;
		MCVariableFetch(argv[0], kMCOptionAsCString, &t_id_string);

		LPWSTR t_id_string_wide;
		AnsiToWide(t_id_string, t_id_string_wide);

		FindDeviceContext t_context;
		CLSIDFromString(t_id_string_wide, &t_context . id);
		t_context . device = nil;

		delete[] t_id_string_wide;

		t_success = ForEachDeviceRapi(FindDeviceRapiCallback, &t_context);
		if (t_success)
			t_device = t_context . device;
		//(t_desktop -> FindDevice(&t_id, RAPI_GETDEVICE_NONBLOCKING, &t_device));
	}

	ComPointer<IRAPISession> t_session;
	if (t_success)
		t_success = ComCheck(t_device -> CreateSession(&t_session));

	if (t_success)
		t_success = ComCheck(t_session -> CeRapiInit());

	if (t_success)
	{
		s_rapi_device = t_device;
		s_rapi_session = t_session . Take();
	}
	else
	{
		if (t_device != nil)
			t_device -> Release();
		Catch(result);
	}
	
	return t_success;
}

bool revMobileDisconnectRapi(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	if (s_rapi_session != nil)
	{
		s_rapi_session -> CeRapiUninit();
		s_rapi_session -> Release();
		s_rapi_session = nil;
	}

	if (s_rapi_device != nil)
	{
		s_rapi_device -> Release();
		s_rapi_device = nil;
	}

	return true;
}

bool revMobileDescribeRapi(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	if (t_success && s_rapi_session == nil)
		t_success = Throw("not connected");

	CEOSVERSIONINFO t_info;
	if (t_success)
	{
		if (!s_rapi_session -> CeGetVersionEx(&t_info))
			t_success = Throw("get version ex failed");
	}

	if (t_success)
		t_success = VariableFormat(result, "%S", t_info . szCSDVersion);
	else
		Catch(result);

	return t_success;
}

//////////

bool revMobileInvokeRapi(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	if (argc != 4 || !IsRefVariable(argv[3]))
		t_success = Throw("syntax error");

	if (t_success && s_rapi_session == nil)
		t_success = Throw("not connected");

	LPWSTR t_dll_path;
	t_dll_path = nil;
	if (t_success)
		t_success = GetWideArg(argv[0], t_dll_path);

	LPWSTR t_function_name;
	t_function_name = nil;
	if (t_success)
		t_success = GetWideArg(argv[1], t_function_name);

	MCString t_input;
	if (t_success)
		t_success = CheckError(MCVariableFetch(argv[2], kMCOptionAsString, &t_input));

	MCString t_output;
	t_output . buffer= nil;
	if (t_success)
		t_success = ComCheck(s_rapi_session -> CeRapiInvoke(t_dll_path, t_function_name, t_input . length, (BYTE *)t_input . buffer, (DWORD *)&t_output . length, (BYTE **)&t_output . buffer, nil, 0));

	if (t_success)
		t_success = CheckError(MCVariableStore(argv[3], kMCOptionAsString, &t_output));

	if (t_output . buffer != nil)
		LocalFree(t_output . buffer);

	delete[] t_function_name;
	delete[] t_dll_path;

	return t_success;
}

//////////

bool revMobileLaunchRapi(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	if (argc < 1 || argc > 2)
		t_success = Throw("syntax error");

	if (t_success && s_rapi_session == nil)
		t_success = Throw("not connected");

	const char *t_command_ansi;
	if (t_success)
		t_success = CheckError(MCVariableFetch(argv[0], kMCOptionAsCString, &t_command_ansi));

	const char *t_arguments_ansi;
	t_arguments_ansi = "";
	if (t_success && argc == 2)
		t_success = CheckError(MCVariableFetch(argv[1], kMCOptionAsCString, &t_arguments_ansi));

	LPWSTR t_command_wide;
	t_command_wide = nil;
	if (t_success)
		t_success = AnsiToWide(t_command_ansi, t_command_wide);

	LPWSTR t_arguments_wide;
	t_arguments_wide = nil;
	if (t_success)
		t_success = AnsiToWide(t_arguments_ansi, t_arguments_wide);

	PROCESS_INFORMATION t_info;
	if (t_success)
	{
		if (s_rapi_session -> CeCreateProcess(t_command_wide, t_arguments_wide, nil, nil, FALSE, 0, nil, nil, nil, &t_info) != 0)
		{
			s_rapi_session -> CeCloseHandle(t_info . hProcess);
			s_rapi_session -> CeCloseHandle(t_info . hThread);
		}
		else
			t_success = false;
	}

	delete[] t_command_wide;

	if (!t_success)
		Catch(result);

	return t_success;
}

//////////

static void UpdateProgress(MCVariableRef p_message, uint32_t p_progress, uint32_t p_total)
{
	bool t_success;
	t_success = true;

	const char *t_message;
	if (t_success)
		t_success = CheckError(MCVariableFetch(p_message, kMCOptionAsCString, &t_message));

	MCObjectRef t_object;
	t_object = nil;
	if (t_success)
		t_success = CheckError(MCContextMe(&t_object));

	MCVariableRef t_args[2];
	t_args[0] = t_args[1] = nil;
	if (t_success)
		t_success =
			CheckError(MCVariableCreate(&t_args[0])) &&
			CheckError(MCVariableCreate(&t_args[1]));

	if (t_success)
		t_success =
			CheckError(MCVariableStore(t_args[0], kMCOptionAsInteger, &p_progress)) &&
			CheckError(MCVariableStore(t_args[1], kMCOptionAsInteger, &p_total));

	if (t_success)
		t_success = CheckError(MCObjectDispatch(t_object, kMCDispatchTypeCommand, t_message, t_args, 2, nil));

	if (t_args[0] != nil)
		MCVariableRelease(t_args[0]);

	if (t_args[1] != nil)
		MCVariableRelease(t_args[1]);

	if (t_object != nil)
		MCObjectRelease(t_object);
}

static bool DoOpenFile(MCVariableRef p_filename, DWORD p_access, DWORD p_creation, HANDLE& r_handle)
{
	bool t_success;
	t_success = true;

	LPWSTR t_filename_wide;
	t_filename_wide = nil;
	if (t_success)
		t_success = GetWideArg(p_filename, t_filename_wide);

	HANDLE t_handle;
	t_handle = INVALID_HANDLE_VALUE;
	if (t_success)
	{
		t_handle = s_rapi_session -> CeCreateFile(t_filename_wide, p_access, 0, nil, p_creation, FILE_ATTRIBUTE_NORMAL, nil);
		if (t_handle == INVALID_HANDLE_VALUE)
			t_success = Throw("could not open file");
	}

	if (t_success)
		r_handle = t_handle;

	delete[] t_filename_wide;

	return t_success;
}

static bool DoFindFile(MCVariableRef p_filename, bool& r_found, CE_FIND_DATA& r_info)
{
	bool t_success;
	t_success = true;

	LPWSTR t_filename;
	t_filename = nil;
	if (t_success)
		t_success = GetWideArg(p_filename, t_filename);

	if (t_success)
	{
		HANDLE t_find;
		t_find = s_rapi_session -> CeFindFirstFile(t_filename, &r_info);
		if (t_find != INVALID_HANDLE_VALUE)
		{
			s_rapi_session -> CeFindClose(t_find);
			r_found = true;
		}
		else
		{
			DWORD t_error;
			t_error = s_rapi_session -> CeGetLastError();
			if (t_error == ERROR_FILE_NOT_FOUND || t_error == ERROR_PATH_NOT_FOUND || t_error == ERROR_NO_MORE_FILES)
				r_found = false;
			else
				t_success = Throw("find file failed");
		}
	}

	delete[] t_filename;

	return t_success;
}

static uint32_t FileTimeToSeconds(const FILETIME& p_time)
{
	LARGE_INTEGER t_file_time_64;
	memcpy(&t_file_time_64, &p_time, sizeof(FILETIME));

	/* Subtract the value for 1970-01-01 00:00 (UTC) */
	t_file_time_64.QuadPart -= 0x19db1ded53e8000;

	/* Convert to seconds. */
	return (uint32_t)(t_file_time_64.QuadPart / 10000000);
}

bool revMobileFileExistsRapi(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	if (t_success && argc != 1)
		t_success = Throw("syntax error");

	if (t_success && s_rapi_session == nil)
		t_success = Throw("not connected");

	CE_FIND_DATA t_info;
	bool t_found;
	if (t_success)
		t_success = DoFindFile(argv[0], t_found, t_info);

	if (t_success && t_found)
		t_found = (t_info . dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;

	if (t_success)
		VariableFormat(result, "%s", t_found ? "true" : "false");
	else
		Catch(result);

	return t_success;
}

bool revMobileDescribeFileRapi(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	if (t_success && argc != 1)
		t_success = Throw("syntax error");

	if (t_success && s_rapi_session == nil)
		t_success = Throw("not connected");

	CE_FIND_DATA t_info;
	bool t_found;
	if (t_success)
		t_success = DoFindFile(argv[0], t_found, t_info);

	if (t_success && !t_found)
		t_success = Throw("file not found");

	if (t_success)
		VariableFormat(result, "%u,%u,%u,%u",
			FileTimeToSeconds(t_info . ftCreationTime),
			FileTimeToSeconds(t_info . ftLastAccessTime),
			FileTimeToSeconds(t_info . ftLastWriteTime),
			t_info . nFileSizeLow);
	else
		Catch(result);

	return t_success;
}

bool revMobileWriteFileRapi(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	if (t_success && (argc < 2 || argc > 3))
		t_success = Throw("syntax error");

	if (t_success && s_rapi_session == nil)
		t_success = Throw("not connected");

	MCString t_data;
	if (t_success)
		t_success = CheckError(MCVariableFetch(argv[1], kMCOptionAsString, &t_data));

	HANDLE t_file;
	t_file = INVALID_HANDLE_VALUE;
	if (t_success)
		t_success = DoOpenFile(argv[0], GENERIC_WRITE, CREATE_ALWAYS, t_file);

	uint32_t t_progress_total, t_progress_current;
	t_progress_total = t_data . length;
	t_progress_current = 0;
	while(t_success && t_data . length > 0)
	{
		DWORD t_in_amount, t_out_amount;
		t_in_amount = t_data . length;
		if (t_in_amount > 32768)
			t_in_amount = 32768;
		if (s_rapi_session -> CeWriteFile(t_file, t_data . buffer, t_in_amount, &t_out_amount, nil) &&
			t_out_amount == t_in_amount)
		{
			t_data . length -= t_in_amount;
			t_data . buffer += t_in_amount;
			t_progress_current += t_in_amount;

			if (argc == 3)
				UpdateProgress(argv[2], t_progress_current, t_progress_total);
		}
		else
			t_success = Throw("i/o error");
	}

	if (t_file != INVALID_HANDLE_VALUE)
		s_rapi_session -> CeCloseHandle(t_file);

	if (!t_success)
		Catch(result);

	return t_success;
}

bool revMobileReadFileRapi(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	if (t_success && (argc < 2 || argc > 3 || !IsRefVariable(argv[1])))
		t_success = Throw("syntax error");

	if (t_success && s_rapi_session == nil)
		t_success = Throw("not connected");

	HANDLE t_file;
	t_file = INVALID_HANDLE_VALUE;
	if (t_success)
		t_success = DoOpenFile(argv[0], GENERIC_READ, OPEN_EXISTING, t_file);

	DWORD t_file_size;
	if (t_success)
	{
		t_file_size = s_rapi_session -> CeGetFileSize(t_file, nil);
		if (t_file_size == 0xffffffff)
			t_success = Throw("i/o error");
	}

	MCString t_data;
	t_data . buffer = nil;
	t_data . length = 0;
	if (t_success)
	{
		t_data . buffer = new char[t_file_size];
		t_data . length = t_file_size;
		if (t_data . buffer == nil)
			t_success = Throw("no memory");
	}

	uint32_t t_position;
	t_position = 0;
	while(t_success && t_position < t_data . length)
	{
		DWORD t_in_amount, t_out_amount;
		t_in_amount = t_data . length - t_position;
		if (t_in_amount > 32768)
			t_in_amount = 32768;
		if (s_rapi_session -> CeReadFile(t_file, t_data . buffer + t_position, t_in_amount, &t_out_amount, nil) &&
			t_in_amount == t_out_amount)
		{
			t_position += t_in_amount;
			if (argc == 3)
				UpdateProgress(argv[2], t_position, t_data . length);
		}
		else
		{
			DWORD t_error;
			t_error = s_rapi_session -> CeGetLastError();
			t_success = Throw("i/o error");
		}
	}

	if (t_file != INVALID_HANDLE_VALUE)
		s_rapi_session -> CeCloseHandle(t_file);

	if (t_success)
		t_success = CheckError(MCVariableStore(argv[1], kMCOptionAsString, &t_data));

	if (!t_success)
		Catch(result);

	delete[] t_data . buffer;

	return t_success;
}

bool revMobileDeleteFileRapi(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	if (t_success && argc != 1)
		t_success = Throw("syntax error");

	if (t_success && s_rapi_session == nil)
		t_success = Throw("not connected");

	LPWSTR t_filename;
	t_filename = nil;
	if (t_success)
		t_success = GetWideArg(argv[0], t_filename);

	if (t_success)
	{
		if (!s_rapi_session -> CeDeleteFile(t_filename))
		{
			DWORD t_last_error;
			t_last_error = s_rapi_session -> CeGetLastError();
			if (t_last_error == ERROR_FILE_NOT_FOUND || t_last_error == ERROR_PATH_NOT_FOUND)
				t_success = Throw("file not found");
			else
				t_success = Throw("delete file failed");
		}
	}

	if (!t_success)
		Catch(result);

	delete[] t_filename;

	return t_success;
}

//////////

bool revMobileFolderExistsRapi(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	if (t_success && argc != 1)
		t_success = Throw("syntax error");

	if (t_success && s_rapi_session == nil)
		t_success = Throw("not connected");

	CE_FIND_DATA t_info;
	bool t_found;
	if (t_success)
		t_success = DoFindFile(argv[0], t_found, t_info);

	if (t_success && t_found)
		t_found = (t_info . dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

	if (t_success)
		VariableFormat(result, "%s", t_found ? "true" : "false");
	else
		Catch(result);

	return t_success;
}

bool revMobileMakeFolderRapi(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	if (t_success && argc != 1)
		t_success = Throw("syntax error");

	if (t_success && s_rapi_session == nil)
		t_success = Throw("not connected");

	LPWSTR t_filename;
	t_filename = nil;
	if (t_success)
		t_success = GetWideArg(argv[0], t_filename);

	if (t_success)
	{
		if (!s_rapi_session -> CeCreateDirectory(t_filename, nil))
			t_success = Throw("create directory failed");
	}

	if (!t_success)
		Catch(result);

	delete[] t_filename;

	return t_success;
}

bool revMobileDeleteFolderRapi(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	LPWSTR t_filename;
	t_filename = nil;
	if (t_success)
		t_success = GetWideArg(argv[0], t_filename);

	if (t_success)
	{
		if (!s_rapi_session -> CeRemoveDirectory(t_filename))
		{
			DWORD t_last_error;
			t_last_error = s_rapi_session -> CeGetLastError();
			if (t_last_error == ERROR_FILE_NOT_FOUND || t_last_error == ERROR_PATH_NOT_FOUND)
				t_success = Throw("folder not found");
			else
				t_success = Throw("remove directory failed");
		}
	}

	if (!t_success)
		Catch(result);

	delete[] t_filename;

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

// This would be the preferred method of accessing devices... Unfortunately you
// need to install Gigs of stuff to get the CoreCon interfaces installed :o(
bool revMobileListDevices(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	ComPointer<ICcServer> t_server;
	if (t_success)
		t_success = t_server . CreateInstance<VsdConnectionManager>();

	ComPointer<ICcDatastore> t_datastore;
	if (t_success)
		t_success = ComCheck(t_server -> GetDatastore(1033, &t_datastore));

	ComPointer<ICcPlatformContainer> t_platform_con;
	if (t_success)
		t_success = ComCheck(t_datastore -> get_PlatformContainer(&t_platform_con));

	ComPointer<ICcObjectContainer> t_platform_con_objs;
	if (t_success)
		t_success = t_platform_con_objs . QueryInterface(t_platform_con);

	ComPointer<ICcCollection> t_platform_coll;
	if (t_success)
		t_success = ComCheck(t_platform_con_objs -> EnumerateObjects(&t_platform_coll));

	long t_platform_coll_count;
	if (t_success)
		t_success = ComCheck(t_platform_coll -> get_Count(&t_platform_coll_count));

	for(long i = 0; i < t_platform_coll_count; i++)
	{
		ComPointer<ICcObject> t_platform_obj;
		if (t_success)
			t_success = ComCheck(t_platform_coll -> get_Item(i, &t_platform_obj));

		ComPointer<ICcPlatform> t_platform;
		if (t_success)
			t_success = t_platform . QueryInterface(t_platform_obj);

		ComPointer<ICcDeviceContainer> t_device_con;
		if (t_success)
			t_success = ComCheck(t_platform -> get_DeviceContainer(&t_device_con));

		ComPointer<ICcObjectContainer> t_device_con_objs;
		if (t_success)
			t_success = t_device_con_objs . QueryInterface(t_device_con);

		ComPointer<ICcCollection> t_device_coll;
		if (t_success)
			t_success = ComCheck(t_device_con_objs -> EnumerateObjects(&t_device_coll));

		long t_device_coll_count;
		if (t_success)
			t_success = ComCheck(t_device_coll -> get_Count(&t_device_coll_count));

		for(long i = 0; i < t_device_coll_count; i++)
		{
			ComPointer<ICcObject> t_device_obj;
			if (t_success)
				t_success = ComCheck(t_device_coll -> get_Item(i, &t_device_obj));

			ComString t_device_name, t_device_id;
			if (t_success)
				t_success =
					ComCheck(t_device_obj -> get_Name(&t_device_name)) &&
					ComCheck(t_device_obj -> get_ID(&t_device_id));

			ComPointer<ICcDevice> t_device;
			if (t_success)
				t_success = t_device . QueryInterface(t_device_obj);

			if (t_success)
				VariableAppendFormat(result, "%S,%S\n", t_device_name, t_device_id);
		}
	}

	if (!t_success)
		VariableFormat(result, "error");

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool revMobileStartup(void)
{
	return true;
}

void revMobileShutdown(void)
{
}

////////////////////////////////////////////////////////////////////////////////

MC_EXTERNAL_NAME("revmobile")
MC_EXTERNAL_STARTUP(revMobileStartup)
MC_EXTERNAL_SHUTDOWN(revMobileShutdown)
MC_EXTERNAL_HANDLERS_BEGIN
	MC_EXTERNAL_COMMAND("revMobileInitializeEmulators", revMobileInitializeEmulators)
	MC_EXTERNAL_COMMAND("revMobileFinalizeEmulators", revMobileFinalizeEmulators)
	MC_EXTERNAL_FUNCTION("revMobileListEmulators", revMobileListEmulators)
	MC_EXTERNAL_COMMAND("revMobileCradleEmulator", revMobileStartEmulator)
	MC_EXTERNAL_COMMAND("revMobileUncradleEmulator", revMobileStopEmulator)

	MC_EXTERNAL_FUNCTION("revMobileListDevices", revMobileListDevicesRapi)
	
	MC_EXTERNAL_COMMAND("revMobileInitialize", revMobileInitializeRapi)
	MC_EXTERNAL_COMMAND("revMobileFinalize", revMobileFinalizeRapi)

	MC_EXTERNAL_COMMAND("revMobileConnect", revMobileConnectRapi)
	MC_EXTERNAL_COMMAND("revMobileDisconnect", revMobileDisconnectRapi)
	MC_EXTERNAL_FUNCTION("revMobileDescribe", revMobileDescribeRapi)

	MC_EXTERNAL_COMMAND("revMobileInvoke", revMobileInvokeRapi)
	MC_EXTERNAL_COMMAND("revMobileLaunch", revMobileLaunchRapi)

	MC_EXTERNAL_FUNCTION("revMobileFileExists", revMobileFileExistsRapi)
	MC_EXTERNAL_FUNCTION("revMobileDescribeFile", revMobileDescribeFileRapi)
	MC_EXTERNAL_COMMAND("revMobileWriteFile", revMobileWriteFileRapi)
	MC_EXTERNAL_COMMAND("revMobileReadFile", revMobileReadFileRapi)
	MC_EXTERNAL_COMMAND("revMobileDeleteFile", revMobileDeleteFileRapi)
	MC_EXTERNAL_FUNCTION("revMobileFolderExists", revMobileFolderExistsRapi)
	MC_EXTERNAL_COMMAND("revMobileMakeFolder", revMobileMakeFolderRapi)
	MC_EXTERNAL_COMMAND("revMobileDeleteFolder", revMobileDeleteFolderRapi)
MC_EXTERNAL_HANDLERS_END

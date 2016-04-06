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

#ifndef __DEVEMU__

struct __declspec(uuid("fa19ef6d-8ff0-4ea0-a45c-d95ece8399e3"))
DeviceEmulatorManager;

enum EMULATOR_STATE
{
    EMU_NOT_RUNNING = 0,
    EMU_RUNNING = 1,
    EMU_CRADLED = 2
};

struct __declspec(uuid("9ea66969-b785-408f-bf2f-f80a2f055b73"))
IDeviceEmulatorManagerVMID : IDispatch
{
	virtual HRESULT __stdcall get_VMID ( /*[out,retval]*/ BSTR * pVMID ) = 0;
	virtual HRESULT __stdcall get_State ( /*[out,retval]*/ enum EMULATOR_STATE * pState ) = 0;
	virtual HRESULT __stdcall get_Name ( /*[out,retval]*/ BSTR * pbstrName ) = 0;
	virtual HRESULT __stdcall Connect ( ) = 0;
	virtual HRESULT __stdcall Cradle ( ) = 0;
	virtual HRESULT __stdcall UnCradle ( ) = 0;
	virtual HRESULT __stdcall Shutdown ( /*[in]*/ long bSaveState ) = 0;
	virtual HRESULT __stdcall Reset ( /*[in]*/ long bSoft ) = 0;
	virtual HRESULT __stdcall ClearSaveState ( ) = 0;
	virtual HRESULT __stdcall BringToFront ( ) = 0;
	virtual HRESULT __stdcall GetConfiguration ( /*[out,retval]*/ BSTR * lpbstrConfig ) = 0;
	virtual HRESULT __stdcall SetConfiguration ( /*[in]*/ BSTR bstrConfig ) = 0;
};

struct __declspec(uuid("8e20a119-c257-43bf-b365-3d4c800fd5a7"))
IEnumVMIDs : IDispatch
{
	virtual HRESULT __stdcall GetVMID ( /*[out,retval]*/ struct IDeviceEmulatorManagerVMID * * ppVMID ) = 0;
	virtual HRESULT __stdcall MoveNext ( ) = 0;
	virtual HRESULT __stdcall Reset ( ) = 0;
};

struct __declspec(uuid("cc52d848-1b6e-43b9-8dd6-90ca07db5efc"))
IEnumManagerSDKs : IDispatch
{
	virtual HRESULT __stdcall EnumerateVMIDs ( /*[out,retval]*/ struct IEnumVMIDs * * ppEnumVMIDs ) = 0;
	virtual HRESULT __stdcall MoveNext ( ) = 0;
	virtual HRESULT __stdcall Reset ( ) = 0;
	virtual HRESULT __stdcall get_Name ( /*[out,retval]*/ BSTR * pbstrName ) = 0;
};

struct __declspec(uuid("be72da87-85cc-4c35-b336-8921d82aa41e"))
IDeviceEmulatorManager : IDispatch
{
	virtual HRESULT __stdcall EnumerateSDKs ( struct IEnumManagerSDKs * * ppEnumManagerSDKs ) = 0;
	virtual HRESULT __stdcall ShowManagerUI ( /*[in]*/ long fShow ) = 0;
	virtual HRESULT __stdcall MoveNext ( ) = 0;
	virtual HRESULT __stdcall Reset ( ) = 0;
	virtual HRESULT __stdcall Refresh ( ) = 0;
	virtual HRESULT __stdcall get_Name ( /*[out,retval]*/ BSTR * pbstrName ) = 0;
	virtual HRESULT __stdcall RegisterRefreshEvent ( BSTR bstrEventName ) = 0;
	virtual HRESULT __stdcall UnRegisterRefreshEvent ( ) = 0;
};

const int E_ENUMSDK_NOT_LOADED = -2147209216;
const int E_ENUMSDK_INVALID_ENUMVMID = -2147209215;
const int E_ENUMVMID_NOT_LOADED = -2147209214;
const int E_ENUMVMID_INVALID_VMID = -2147209213;

#endif

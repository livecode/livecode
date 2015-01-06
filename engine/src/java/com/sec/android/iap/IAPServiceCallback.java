/*
 * This file is auto-generated.  DO NOT MODIFY.
 * Original file: /Users/alilloyd/Documents/GitHub/livecode-ali/refactor-syntax/engine/src/java/com/sec/android/iap/IAPServiceCallback.aidl
 */
package com.sec.android.iap;
public interface IAPServiceCallback extends android.os.IInterface
{
/** Local-side IPC implementation stub class. */
public static abstract class Stub extends android.os.Binder implements com.sec.android.iap.IAPServiceCallback
{
private static final java.lang.String DESCRIPTOR = "com.sec.android.iap.IAPServiceCallback";
/** Construct the stub at attach it to the interface. */
public Stub()
{
this.attachInterface(this, DESCRIPTOR);
}
/**
 * Cast an IBinder object into an com.sec.android.iap.IAPServiceCallback interface,
 * generating a proxy if needed.
 */
public static com.sec.android.iap.IAPServiceCallback asInterface(android.os.IBinder obj)
{
if ((obj==null)) {
return null;
}
android.os.IInterface iin = (android.os.IInterface)obj.queryLocalInterface(DESCRIPTOR);
if (((iin!=null)&&(iin instanceof com.sec.android.iap.IAPServiceCallback))) {
return ((com.sec.android.iap.IAPServiceCallback)iin);
}
return new com.sec.android.iap.IAPServiceCallback.Stub.Proxy(obj);
}
public android.os.IBinder asBinder()
{
return this;
}
@Override public boolean onTransact(int code, android.os.Parcel data, android.os.Parcel reply, int flags) throws android.os.RemoteException
{
switch (code)
{
case INTERFACE_TRANSACTION:
{
reply.writeString(DESCRIPTOR);
return true;
}
case TRANSACTION_responseCallback:
{
data.enforceInterface(DESCRIPTOR);
android.os.Bundle _arg0;
if ((0!=data.readInt())) {
_arg0 = android.os.Bundle.CREATOR.createFromParcel(data);
}
else {
_arg0 = null;
}
this.responseCallback(_arg0);
return true;
}
}
return super.onTransact(code, data, reply, flags);
}
private static class Proxy implements com.sec.android.iap.IAPServiceCallback
{
private android.os.IBinder mRemote;
Proxy(android.os.IBinder remote)
{
mRemote = remote;
}
public android.os.IBinder asBinder()
{
return mRemote;
}
public java.lang.String getInterfaceDescriptor()
{
return DESCRIPTOR;
}
public void responseCallback(android.os.Bundle bundle) throws android.os.RemoteException
{
android.os.Parcel _data = android.os.Parcel.obtain();
try {
_data.writeInterfaceToken(DESCRIPTOR);
if ((bundle!=null)) {
_data.writeInt(1);
bundle.writeToParcel(_data, 0);
}
else {
_data.writeInt(0);
}
mRemote.transact(Stub.TRANSACTION_responseCallback, _data, null, android.os.IBinder.FLAG_ONEWAY);
}
finally {
_data.recycle();
}
}
}
static final int TRANSACTION_responseCallback = (android.os.IBinder.FIRST_CALL_TRANSACTION + 0);
}
public void responseCallback(android.os.Bundle bundle) throws android.os.RemoteException;
}

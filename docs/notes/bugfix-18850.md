# LCB modules can declare Android app permissions and features

LCB module metadata is now checked for Android permissions which will 
be added to the manifest when building for Android. For example, a 
module containing

	metadata android.features is "hardware.bluetooth,hardware.camera"
	metadata android.hardware.camera.required is "false"
	metadata android.hardware.bluetooth.required is "true"
	metadata android.permissions is "BLUETOOTH_ADMIN"

will result in the following lines being added to the Android manifest:

	<uses-feature android:name="android.hardware.camera" android:required="false"/>
	<uses-feature android:name="android.hardware.bluetooth" android:required="true"/>
	<uses-permission android:name="android.permission.BLUETOOTH_ADMIN" />
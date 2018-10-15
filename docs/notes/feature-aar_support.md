# Android AAR support
Extensions can now include android AARs on which they depend.
The aars must be included in the extension's code/jvm-android 
folder. Currently, the supported AAR contents are:
	- Jar files
	- Resources
	- Manifests
In particular, native code contained in AARs is not yet supported.

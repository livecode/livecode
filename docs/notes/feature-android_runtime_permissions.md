# Android 6.0 runtime permissions

Android 6.0 (API 23) Marshmallow introduced a new permissions model
that lets apps request permissions from the user at runtime, rather
than prior to installation. Apps built with LC 9.0.1 do support this
new permissions model, and request permissions automatically when the
app actually requires the services or data protected by the services. 

For example, if the app calls `mobilePickPhoto "camera"`, a dialog will
be shown to the user asking for permission to access the device camera.

If the user does not grant permission, the call will fail. Moreover, the
app can use the function `androidRequestPermission(permissionName)` to
check if the permission for `permissionName` has been granted. 

Notes:

- You have to make sure that you check the required permissions for your
app in the standalone settings.
- Apps that run on devices running Android 6+ will work with the new
permissions model.
- Apps that run on older devices (less than Android 6) will continue to
work with the old permissions model.
- If the user does not grant a permission when the dialog appears for the
first time, they can change this preference from the Settings app.


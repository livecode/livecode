# Android Permission Checking

The ability to check Android permissions in LCB has been added to the android
utility module.

Use these handlers to check and request permissions before accessing resources
(e.g. camera access).

The following handler have been added:

* `AndroidRequestPermission` - Display a dialog requesting a given permission.
* `AndroidPermissionExists` - Check to see if a given permission name is valid.
* `AndroidHasPermission` - Check to see if a given permission has been granted.

See the dictionary for full details.

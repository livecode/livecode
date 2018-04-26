# New NFC tag feature on Android

It is now possible to access data directly from NFC tags read by Android devices.

The functions **mobileIsNFCAvailable** and **mobileIsNFCEnabled** can be used to test if
the device is able to read NFC tags.

The commmands **mobileEnableNFCDispatch** and **mobileDisableNFCDispatch** can be used to
control whether or not your app intercepts all NFC tags while in the foreground.

The handler **nfcTagReceived** will be sent with the tag data to your app whenever an NFC
tag is read by the device.

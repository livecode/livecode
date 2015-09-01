#  mobileLaunchData function

This new function is available on Android and returns an array containing information from the Intent object used to launch the currently running app.

## Available information
* `action` - The general action the app was launched to perform.
* `data` - The data to operate on.
* `type` - The MIME type of the data provided.
* `categories` - Additional information about the action to perform.
* `extras` - An array of action-specific data set by the calling activity.

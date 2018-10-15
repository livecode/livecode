# Toast Library
A toast library has been added. This provides the ability to pop up a
transient, non-modal notification to the user.
Currently, the library is only supported on android.
The library places the following handlers in the message path:
- `mobileToast pMessage, pDuration`: Display a toast for the specified duration
- `mobileToastCancel`: Cancel the current toast

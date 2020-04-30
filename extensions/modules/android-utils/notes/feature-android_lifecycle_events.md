# Registering for lifecycle SensorEventListener

A new handler `AndroidRegisterLifecycleListener` has been added that allows for
widgets to track application lifecycle events, specifying handlers to be called
when the application is paused or resumed. The first parameter is the handler to
be called when the application is paused. The second parameter is the handler to
be called when the application is resumed.

The listener object returned by `AndroidRegisterLifecycleListener` can be
unregistered by calling `AndroidUnregisterLifecycleListener`, meaning the pause
and resume handlers will no longer be called.
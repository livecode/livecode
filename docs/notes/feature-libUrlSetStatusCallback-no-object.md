# libURLSetStatusCallback no longer requires a target object for the message

Passing an object reference as a second parameter to libURLSetStatusCallback 
is no longer required. If no object is passed in then the message will be sent
to revLibURL itself and you can handle the message anywhere in the message path.

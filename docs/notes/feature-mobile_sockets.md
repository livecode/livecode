# Mobile Sockets

Socket support has been added to the mobile platforms. The following syntax has been added to the iOS and Android engines.

Commands:
* accept
* open socket
* close socket
* read from socket
* write to socket
* secure socket

Functions:
* openSockets

Messages:
* socketClosed
* socketError
* socketTimeout

Properties:
* socketTimeoutInterval

If you are secure sockets, the SSL library must be included in your standalone. To do this for iOS, make sure the "Encryption" checkbox of "Basic Application Settings" section on the iOS screen of the Standalone Application Settings window is selected. To do this for Android, make sure the "SSL &amp; Encryption" checkbox of "Basic Application Settings" section on the Android screen of the Standalone Application Settings window is selected.

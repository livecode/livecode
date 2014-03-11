# OpenSSL & Encryption Updates

Open SSL & encryption support has been added to the iOS and Android engines (using LibOpenSSL version 1.0.1e). This allows developers to use the **encrypt** and **decrypt** commands on the mobile platforms in exactly the same way they would on desktop. In order to include the SSL and encryption libraries, developers must tick the "SSL & Encryption" checkbox in the iOS/Android pane of the standalone builder.

In addition to mobile support, LiveCode now includes its own version of the encryption and SSL libraries on OS X (LibOpenSSL version 1.0.1e). This means developers are now no longer relying on the system installed security libraries on OS X.

Windows and Linux remain unchanged.
# 'secure socket' command
A new command, **secure socket**, has been added. Use **secure socket** to convert an unsecured socket created using *open socket* into a secured socket. This way, all future communications over the socket will be encrypted using SSL.

The secure socket command has 3 variants:

**secure socket** *socket*
**secure socket** *socket* **with verification**
**secure socket** *socket* **without verification**

If 'with verification' is specified, when connecting to a remote peer, the client verifies the peers certificate during the handshake process. The **sslCertificates** can be used to specify a list of certificates to verify against. In addition you can place system wide certificates in System/Library/OpenSSL/certs.

If 'without verification' is specified then peers credentials are not authenticated, and any connection is accepted.

Once secured:

* All pending and future reads from the socket will be assumed to be encrypted.
* All pending writes will complete unencrypted. All future writes will be encrypted.

If the socket fails secure, a **socketError** message is sent to the object that opened the socket (not the object that attempted to secure it).

open socket to "127.0.0.0:8080"
write "unencrypted message" to socket "127.0.0.0:8080"
secure socket "127.0.0.0:8080"
write "encrypted message" to socket "127.0.0.0:8080"

# New variant of open and secure socket

New variants of open and secure socket have been added:

**open secure socket** *socket* **with verification for host** *host*
**secure socket** *socket* **with verification for host** *host*

The new host parameter allows the user to specify the host name the connection should be verified against. This is particularly useful if server your socket is directly connected to is not the end host you are talking to. For example when tunnelling through a proxy to connect to a HTTPS URL.

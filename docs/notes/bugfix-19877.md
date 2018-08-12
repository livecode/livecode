# Add support for accepting socket connections on a port in the ephemeral port range

When accepting connections on port 0 the OS will assign an available
port within it's ephemeral port range. The accept command now names the
socket with the bound port number rather than 0 so that the bound port
will appear in the value of the openSockets function and sets the it
variable to the port number.

# SQLite binary entries are non-standard.
The revdb SQLite driver has always encoded any binary data that has been inserted into a column in a non-standard way (the reason for this dates back to V2 of sqlite which couldn't cope with NUL's in data). The problem with this is that it means binary columns in sqlite databases produced and accessed via LiveCode are incompatible with third-party tools and applications that use SQLite databases.

To resolve this issue, SQLite databases can now be opened in a new mode which turns off the binary encoding and decoding that the driver previously performed.

To open an SQLite database with compatibility treatment of binary data, nothing has changed.

To open an SQLite database with the new binary behavior, pass "binary" as the third parameter of the revOpenDatabase call.

For example:

get revOpenDatabase("sqlite", "mydb.sqlite") -- opens in compatibility binary mode (LiveCode compatible only)
get revOpenDatabase("sqlite", "mydb.sqlite", "binary") -- opens in new binary mode (third-party compatible)

How the driver treats binary data is a per-connection property so different connections to the same database can use different modes if needed.
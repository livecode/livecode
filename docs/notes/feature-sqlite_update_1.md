# SQLite support updated and improved
The version of SQLite has been updated to 3.8.2.

The SQLite RTREE module is now available.

SQLite loadable extensions are now supported. To utilize loadable extensions, the 'extensions' option must be passed to the revOpenDatabase() call when creating the database connection (see below).

Binary data can now be placed into SQLite databases verbatim (without the encoding that used to occur) - this means databases can be made to contain binary data which is compatible with other applications. To utilize this functionality, the 'binary' option must be passed to the revOpenDatabase() call when creating the database connection (see below).

The SQLite revOpenDatabase() call no longer requires 5 arguments and only requires a minimum of 2. It has been updated as follows:

  **revOpenDatabase("sqlite", *database-file*, [ *options* ])**

The *database-file* parameter is the filename of the SQLite database to connect to.

The *options* parameter is optional and if present should be a comma-separated list of option keywords. The *binary* keyword means place binary data into the database verbatim (without LiveCode encoding). The *extensions* keyword means enable loadable extensions for the connection. Note that the order of the items in the options parameter is not important.

For example:

   put revOpenDatabase("sqlite", "mydb.sqlite") -- open with legacy binary mode and loadable extensions disabled
   put revOpenDatabase("sqlite", "mydb.sqlite", "binary") -- open the connection in the 'new' binary mode
   put revOpenDatabase("sqlite", "mydb.sqlite", "extensions") -- enable loadable extensions for this connection
   put revOpenDatabase("sqlite", "mydb.sqlite", "binary,extensions") -- enable both 'new' binary mode and loadable extensions

Note: We are hoping to update SQLite to 3.8.3 before release - assuming that SQLite has declared that version stable before then.

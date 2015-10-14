# SSL Support for PostgreSQL Connections

The PostgreSQL database driver has been updated to support secure connections. The desired SSL connection mode can be specified in the sixth parameter of revOpenDatabase.

The syntax for connecting to PostgreSQL databases is now as follows:

***revOpenDatabase(*** "postgresql", *<host>[<:port>]*, *<databasename>*, *[<username>]*, *[<password>]*, *[<sslmode>]*, *[<sslcompression>]*, *[<sslcert>]*, *[<sslkey>]*, *[<sslrootcert>]*, *[<sslcrl>]* ***)***

For full information on the new parameters see the dictionary entry for ***revOpenDatabase***.

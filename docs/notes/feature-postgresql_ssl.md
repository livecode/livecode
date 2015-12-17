---
version: 8.0.0-dp-8
---
# SSL Support for PostgreSQL Connections

The PostgreSQL database driver has been updated to support secure
connections. The desired SSL connection options can be specified as
key value pairs in the additional parameters of **revOpenDatabase**.

The syntax for connecting to PostgreSQL databases is now as follows:

    revOpenDatabase("postgresql", host[:port], databasename,
                    [username], [password], [ssloption=*ssloptionvalue],
                    ...)

For full information on the new parameters see the dictionary entry
for **revOpenDatabase**.

**This feature was sponsored by the community Feature Exchange.**

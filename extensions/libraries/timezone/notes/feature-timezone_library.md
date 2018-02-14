# Time zone library
A time zone library has been added for converting to and from
universal time in various time zones. It uses the IANA timezone
database to do the conversion.

The library contains three handlers:
* `ToUniversalTime(pSeconds, pTimeZone)` - convert from local time in the specified time zone to universal time
* `FromUniversalTime(pSeconds, pTimeZone)` - convert from universal time to local time in the specified time zone 
* `TimeZones()` - list of valid time zones, one per line

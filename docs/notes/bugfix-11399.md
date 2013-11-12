# iOS doesn't load any data unless HTTP status code is 200
The iOS engine will now continue and load any data returned from any status code in the 200's - even if empty. In particular, it will load data for status code 206 - which means a partial response due to range headers.

# LiveCode Builder Standard Library

## Date and time

* `the local date` now returns a list with an additional 7th element.  This is the local time zone's offset from UTC, in seconds.

* The new `the universal date` expression provides the date in the UTC+00:00 timezone.  It returns a list in the same format as `the local date`.

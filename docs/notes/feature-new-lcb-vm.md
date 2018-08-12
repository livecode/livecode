# Re-written LCB VM

The "virtual machine" used to run LiveCode Builder code has been re-written
from scratch. This new VM provides a framework enabling better extensibility,
better error reporting and, in future, more comprehensive optimizations.

Most existing LCB code should run without any changes. There may be some code
that worked on the previous VM but doesn't in the new VM due to more
comprehensive run-time checking; this is usually fixable with only very minor
changes to the source code.

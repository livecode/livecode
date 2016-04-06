# Objects are only deleted on idle

The engine will now flush any recently deleted objects after each command as long as they were
created during the current event handling loop.

If an object is created during one event handling loop, and then deleted during another nested
event handling loop it won't be flushed until control returns to the original event handling
loop.

The upshot is that in tight loops, creating and deleting objects will result in objects being
flushed immediately, reducing memory usage and making it easier to write object processing
code which creates and deleted many objects.


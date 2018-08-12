# Fix deletion of the target in safe cases

You can now safely 'delete the target' as long as there are
no handlers on the stack owned by the target.

After deleting 'the target', 'the target' will become
empty which will result in an execution error when an attempt
is made to dereference it.

# Improvements for saving in old stack file formats

It is now possible to specify a version of the stack file format to be
used when saving directly with the `save` command, by using the `save
STACK with format VERSION` form of the command.

You can request the latest supported file format using the `save STACK
with newest format` form.

The `stackFileVersion` global property is now deprecated, and should not be
used in new stacks.

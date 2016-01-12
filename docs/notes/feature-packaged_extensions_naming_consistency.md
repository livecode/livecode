# Packaged extension name changes

Earlier versions of the widgets and libraries which are bundled with
the IDE were named inconsistently.

Now all LiveCode extensions are named either
**com.livecode.widget.<widget name>** or
**com.livecode.library.<library name>**.

Their directories in the installed LiveCode bundle have also been updated to
match.

**Note:** This change will break some stacks that have widgets saved
on them, or scripts which refer to a widget by its kind.

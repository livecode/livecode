# Group command does not return new group id in it

The group command has been changed so that it returns the long id
of the newly created group in the 'it' variable.

If no group is created (as a result of using 'group' with an empty
selection) the 'it' variable will be set to empty.


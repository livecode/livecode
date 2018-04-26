# Script only stacks with behavior
Script only stacks can now store a stack behavior as part of the file
format. A 'with behavior' clause is added to the header of a script
only stack, if it has a behavior property which references a stack.

When a script-only-stack with such a clause is loaded, the behavior
is set as part of the loading.

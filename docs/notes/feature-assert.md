# 'assert' command (experimental)
An experimental command has been added to support writing tests. The assert command has two forms.

The first form evaluates an expression and checks to see if the it evaluates to true or false:

    **assert** *expr*
    **assert** **true** *expr*
    **assert** **false** *expr*

The second form evaluates an expression and checks to see if it throws an error or not:

    **assert** **success** *expr*
    **assert** **failure** *expr*

In either case if the condition fails, then an **assertError** message is sent to the object containing the command:

    **assertError** *handlerName*, *line*, *column*, *objectLongId*

Examples:

   assert 1 is 1 -- succeeds
   assert true "black" is "white" -- fails
   assert false "hello" is a number -- succeeds
   assert false 1 + 1 = 2 -- fails

   assert success 1 + 1 -- succeeds
   assert failure 1 + "z" -- fails


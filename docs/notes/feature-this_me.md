# Access to the current behavior object.
It is now possible to use 'this me' to refer to the object to which the currently executing script is attached.
This is particularly useful in the context of chained behaviors, in which context 'this me' will be the object of the current behavior. This differs from 'me' in this context, which will always be the object the behavior is acting upon.
For example, if Button A's behavior property is set to Button B, and Button B has script:
    on mouseUp
      put the short name of this me
    end mouseUp
Then clicking on Button A will result in 'B' being output (as the mouseUp handler is in Button B's script).


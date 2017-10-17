# Send script form of send command
The syntax 

	send script <script> to <obj>

has been added to allow a chunk of script to be executed in the context 
of an object without any attempted evaluation of parameters that occurs 
with the original form of the send command.

For example, suppose there is a stack named "Stack" with script
    
    on doAnswer pParam
       answer pParam
    end doAnswer

    function myName
       return the short name of me
    end myName
    
and a button on the stack named "Button" with script

    on mouseUp
       send "doAnswer myName()" to this stack
       send script "doAnswer myName()" to this stack
    end mouseUp

    function myName
       return the short name of me
    end myName

clicking the button would result in an answer dialog first saying "Button" as the
`myName` function would be evaluated in the button context, then "Stack" as 
using the `script` form would result in the `myName` function being evaluated in the 
stack context.

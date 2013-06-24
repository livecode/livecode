# Chained Behaviors
The behavior property of a control currently being used as a behavior will now be taken into account and result in the child behaviour deferring to the parent behavior in the same way a control defers to its behavior. For example, let's say you have the following setup:
`field "Action" - behaviour set to button "Derived"
button "Derived" - behaviour set to button "Root"
button "Root"`
Then the message path will be:
`field "Action"
   button "Derived"
   button "Root"
 <parent of field>`

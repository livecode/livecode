# Inspect Icon

When in read only mode, the tree widget will now display an 'open in new window' icon if 
the value of the array at the specified path contains a newline character, or is too large
to display in the widget. 

If this icon is clicked, an `actionInspect` message is sent to the widget's script object.
It has one parameter: the path associated with the clicked row.
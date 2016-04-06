---
version: 8.0.0-dp-5
---
# Signals

When in read only mode, the tree widget will now display an 
'open in new window' icon if the value of the array at the specified 
path contains a newline character, or is too large to display in the 
widget. 

If this icon is clicked, an `actionInspect` signal is emitted. It has 
one parameter: the path associated with the clicked row.
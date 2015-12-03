# Ability to access individual node data

The following handlers have been added to the tree view widget:
- `SetArrayDataOfElement(<path>, <value>)`
- `GetArrayDataOfElement(<path>)`

This allows the use of syntax such as 
`the arrayData of element "a" of element "b" of widget "Tree View"`
in LiveCode Script, in order to access the data `tArray["b"]["a"]`, where tArray is the 
Tree View's underlying array. This allows modification of a particular node of the Tree 
View without causing a costly complete recalculation.
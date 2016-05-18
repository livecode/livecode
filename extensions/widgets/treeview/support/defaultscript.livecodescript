script "com.livecode.widget.treeView.__DefaultScript"
-- Sent when a leaf node of the tree view is double-clicked
-- pPathToNode is a comma-delimited list of keys in the underlying array
on actionDoubleClick pPathToNode
   
end actionDoubleClick

-- Sent when the inspect icon is clicked in read only mode
-- pPathToNode is a comma-delimited list of keys in the underlying array
on actionInspect pPathToNode
   
end actionInspect

-- Sent when an element is hilited
-- Use the hilitedElement property to get a comma-delimited list of
-- keys in the underlying array
on hiliteChanged
   local tElement
   put the hilitedElement of the target into tElement
end hiliteChanged

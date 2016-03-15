script "com.livecode.widget.navbar.__DefaultScript"
-- Sent when a navigation item is clicked
--
-- Use the "hilitedItem" property to get the current item number and the
-- "hilitedItemName" property to get its item name
on hiliteChanged
   put the hilitedItem of me && the hilitedItemName of me
end hiliteChanged

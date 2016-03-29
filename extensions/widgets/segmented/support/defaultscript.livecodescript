script "com.livecode.widget.segmented.__DefaultScript"
-- Sent when a navigation item is clicked
--
-- Use the "hilitedItems" property to get the current item number and the
-- "hilitedItemNames" property to get its item name
on hiliteChanged
   put the hilitedItems of me && the hilitedItemNames of me
end hiliteChanged

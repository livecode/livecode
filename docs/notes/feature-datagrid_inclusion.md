# DataGrid added to the Standalone Settings script libraries list

Formerly, the DataGrid library was only included in a standalone application if the stack saved as a standalone was using a DataGrid.

This have been causing issues, in case the stack saved does not use DataGrid, but loads a stack which uses it: the DataGrid library was not saved with the standalone, and the DataGrid would not work in the loaded stack.

To tackle this issue, we have added 'DataGrid' in the list of Script Libraries in the Standalone Settings. You can now force the inclusion of the DataGrid library, to ensure that any stack loaded by the standalone can use DataGrids.
 
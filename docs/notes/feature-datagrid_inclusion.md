---
version: 8.0.0-dp-8
---
# DataGrid added to the Standalone Settings script libraries list

Formerly, the DataGrid library was only included in a standalone
application if the stack saved as a standalone was using a DataGrid.
When the stack saved did not use DataGrid, but loaded a stack which
used it, the DataGrid library was not saved with the standalone, and
the DataGrid would not work in the loaded stack.

To correct this, "DataGrid" has been added to the list of "Script
Libraries" in the "Standalone Settings" dialog. You can now force the
inclusion of the DataGrid library, to ensure that any stack loaded by
the standalone can use DataGrids.

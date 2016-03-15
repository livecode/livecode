---
version: 8.0.0-dp-5
---
# LiveCode Builder Standard Library
## Sorting

* Lists can now be sorted using an arbitrary comparison handler.

  * A new `SortCompare` handler type has been added to the sort
    module.  A handler that conforms with `SortCompare` might be
    declared like:

    ```
    MyComparisonHandler(in pLeft as any, in pRight as any) returns Integer
    ```

  * To sort using a `SortCompare` handler, use the new `sort <List>
    using handler <SortCompare>` statement.

# LiveCode Builder Language

## Sort using arbitrary comparison handler

The ability to sort a list using an arbitrary comparison handler has been added. The syntax is 

``` sort <List> using handler <Handler> ```

A public handler type SortCompare has been added to the sort module. 
The handler used for sort comparison must be of type SortCompare, i.e. be of the form

```MyComparisonHandler(in pLeft as any, in pRight as any) returns Integer```
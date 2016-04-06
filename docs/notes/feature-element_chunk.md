# Widget element chunk

In order to facilitate more efficient manipulation of a widget's underlying data, the following LiveCode Script syntax has been added:

    the <property> of { element <index> , of } <widget>

This syntax maps to the following LiveCode Builder handlers:
- `Set<Property>OfElement(<path>, <value>)`
- `Get<Property>OfElement(<path>)`

For example, once the above handlers have been added to the Tree View widget, we could use `the arrayData of element "a" of element "b" of widget "Tree View"` in order to access the data `tArray["b"]["a"]`, where tArray is the Tree View's underlying array.

This allows modification of a particular node of the Tree View without causing a costly complete recalculation.

The `<index>` in the element chunk expression may also be an indexed array which keeps the same sense as indexing, i.e. if `tArray[1]` is `"x"` and `tArray[2]` is `"y"`, then

`element y of element x of widget 1 === element tArray of widget 1`

Currently this syntax is limited to property-based expressions only - use of the element chunk in any other context will result in an error.

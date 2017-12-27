# Add icon

* Icons can be added individually.  The default family is "custom".  Added icons
  can specify the family in the name using the form "family/name".

* Multiple icons can be added using an array (same format that `iconData()` returns).
  The family must be specified.

New handlers:
* `iconFamilies()` - returns a list of current icon families in the library
* `iconNamesForFamily()` - returns a string list of icon names for a specified family
* `iconListForFamily()` - returns a LCB list of icon names for a specified family
* `iconDataForFamily()` - returns LCB array of icon data for a specified family
* `addIcon()` - add an icon to the iconsvg library
* `addIconsForFamily()` - add a family of icons to the iconsvg library

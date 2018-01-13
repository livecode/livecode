# Add icon / Set current family

* Icons can be added individually.  The default family is "custom".  Added icons
  can specify the family in the name using the form "family/name".

* Multiple icons can be added using an array (same format that `iconData()` returns).
  The family must be specified.

* The current icon family can now be set.  This allows existing tools in the IDE to
  show icons from any loaded family.

New handlers:
* `iconFamilies()` - returns a list of current icon families in the library
* `iconNamesForFamily()` - returns a string list of icon names for a specified family
* `iconListForFamily()` - returns a LCB list of icon names for a specified family
* `iconDataForFamily()` - returns LCB array of icon data for a specified family
* `addIcon()` - add an icon to the iconsvg library
* `addIconsForFamily()` - add a family of icons to the iconsvg library
* `getCurrentIconFamily()` - get the family used as default (i.e. for iconPicker)
* `setCurrentIconFamily()` - set the family to be used as default (i.e. for iconPicker)
* `deleteIconFamily()` - remove a family from the library
* `iconArrayMatchingInAllFamilies()` - search for icon matches among all families

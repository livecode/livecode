# LiveCode Builder Host Library
## Engine library

A new syntax variant `property <propertyName> of set <setName> of <object>` has
been added to allow access to values from custom property sets.

For example to save and restore the rect of a widget to a custom property set:
	// save
	set property "savedRect" of set "cGeometry" of my script object to my rect
	
	// restore
	set property "rect" of my script object to property "savedRect" of set "cGeometry" of my script object
	
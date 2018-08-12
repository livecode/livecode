---
version: 8.1.0-dp-1
---
# Automatic LCB extension inclusion in standalones

When a standalone is built, the modules required for the widgets that
are on the stack (or any of its substacks) are now included in the
application automatically, regardless of whether the 'Search for
required inclusions...' option is selected in the standalone settings.

If 'Search for required inclusions' is enabled, the scripts of the
application will be searched for uses of the public handlers of any
available LCB libraries, and any uses of the 'kind' of available
widgets to determine whether the relevant modules are included. For
example, if the script contains:

	create widget as "com.livecode.widget.svgpath"
	
then the 'SVG Path' widget and all its dependencies will be included.

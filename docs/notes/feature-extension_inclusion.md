# Automatic LCB extension inclusion in standalones

When a standalone is built, the modules required for the widgets that 
are on the stack (or any of its substacks) are now included in the 
application automatically, regardless of whether search for inclusions
is selected in the standalone settings.

If search for inclusions is selected, the scripts of the application
will be searched for uses of the public handlers of any available LCB
libraries, and any uses of the 'kind' of available widgets to determine
whether the relevant modules are included. For example, if the script
contains

	create widget as "com.livecode.widget.svgpath"
	
then the SVG Path widget module and all its dependencies will be 
included.
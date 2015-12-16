---
version: 8.0.0-dp-5
---
# New "popup widget" command

A new **popup widget** command has been added which opens a widget
within a popup window.  The syntax is:

    popup widget <kind> [ at <location> ] [ with properties <propertyArray> ]

For example, this command can be used to show a color picker widget as
a popup:

	local tProps
	-- Set the size of the popup
	put "0,0,120,50" into tProps["rect"]
	-- Set the initial color value
	put "1,1,0.5" into tProps["initialColor"]
	
	-- Show the widget in a popup window
	popup widget "com.example.mycolorpicker" at the mouseloc with properties tProps

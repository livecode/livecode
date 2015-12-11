---
version: 8.0.0-dp-5
---
# LCS-Widget: Add 'popup widget' command

Syntax: `popup widget <kind> [ at <location> ] [ with properties <propertyArray> ]`

Summary: Opens a widget within a popup window.

Example:

	local tProps
	// Set the size of the popup
	put "0,0,120,50" into tProps["rect"]
	// Set the initial color value
	put "1,1,0.5" into tProps["initialColor"]
	
	// Show the widget in a popup window
	popup widget "com.example.mycolorpicker" at the mouseloc with properties tProps

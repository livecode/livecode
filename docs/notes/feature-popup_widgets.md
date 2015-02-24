# Feature: Popup Widgets

Added the ability to use widgets within popup dialog windows.

## New Syntax:
* popup widget <Kind> at <Position> [ with properties <Properties> ]
*	Launch the named widget as a popup. The popup can return a value in the result.
* currently popped up
*	test if this widget is part of a popup
* close popup [ returning <Result>
*	set the result of the calling popup statement to <Result>


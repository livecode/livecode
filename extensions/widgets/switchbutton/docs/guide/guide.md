# Switch Button
The switch button widget is an on/off switch, consisting of two mutually exclusive choices or states.

Windows / Linux / Android

![Switch Button widget](images/switchbutton.png)

Mac / iOS

![Switch Button widget](images/switchbutton-ios.png)

## Creating a Switch Button
A switch button widget can be created by dragging it out from the Tools
Palette, where it appears with the following icon:

<svg viewBox="0 0 120 38" style="display:block;margin:auto" width="auto" height="50">
  <path d="M47.3,0H18.5C8.3,0,0,8.3,0,18.5v0C0,28.7,8.3,37,18.5,37h28.8c10.2,0,18.5-8.3,18.5-18.5v0C65.8,8.3,57.5,0,47.3,0zM19.8,33.5c-8.3,0-15-6.7-15-15c0-8.3,6.7-15,15-15s15,6.7,15,15C34.8,26.8,28,33.5,19.8,33.5z" />
</svg>


Alternatively it can be created in script using:

	create widget as "com.livecode.widget.switchbutton"

## Using the Switch Button
The switch button state is reflected in its `hilite` property. When
the switch is clicked, or the property is set through script, the value 
of the `hilite` changes accordingly and the widget's appearance changes
to reflect the value.

## Switching Theme
Setting the `theme` of the header bar to "Android" or "iOS" will 
temporarily display it using the metrics of the chosen platform. This
property is transient - it is not saved with the stack as it uses the 
appropriate mobile theme for the platform is is running on.
# Theming and appearance

* The switch button is now drawn the same size for both its "iOS" and
  "android" themes.  It is no longer possible to resize the bounding box of
  the switch button so that some of it is "cut off" at the edges.

* The switch button now uses the **theme** property to control its appearance.
  Currently, the **theme** property is _not_ saved, and is reset to "native"
  whenever the widget is loaded.

* The switch button colors are now controlled by the standard LiveCode
  **hiliteColor**, **borderColor** and **backgroundColor** properties.  They
  can be edited in the property inspector.

* The switch button now obeys the standard LiveCode **showBorder** property,
  which can be edited in the property inspector.

# Properties

* The **widgetTheme** property has been removed.  Use the **theme** property
  instead.

* The **colorScheme** property has been removed.  Set the **hiliteColor**,
  **borderColor** and **backgroundColor** properties instead.

* The **switchIsOn** property has been removed.  Use the **hilited** property
  instead.

# Signals

* The **switchChanged** signal has been renamed to **hiliteChanged**.

# Default script

* The switch button now has a default script.

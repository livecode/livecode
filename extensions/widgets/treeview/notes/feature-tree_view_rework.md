# Theming and Appearance

* The tree view widget has been reworked to make it more fully
  integrated into the IDE and behave more like a classic control.

  * The tree view now uses the built-in color properties
    **backgroundColor**, **foregroundColor**, **hiliteColor** and
    **borderColor** for its colors.  When no colors are set, it will use
    a set of generic native theme colors.

  * The **borderColor** is now used as the color for the separator, if
    present. The separator is now drawn with a stroke width of 1, but
    its hit rect extends 2 pixels either side of the line.

# Properties

* The tree view now uses standard property names (where they exist) for 
  its properties.

  * **showFrameBorder** is now called **showBorder**
  * **selectedRowColor** is now called **hiliteColor**

# Signals

* The **selectedElementChanged** signal has been renamed to
  **hiliteChanged**.

# Theming and Appearance
The tree view widget has been reworked to make it more fully
integrated into the IDE and behave more like a classic control.

The tree view now uses the built-in color properties `backgroundColor`, 
`foregroundColor`, `hiliteColor` and `borderColor` for its colors.
When no colors are set, it will use a set of generic native theme 
colors.

# Properties
The tree view now uses standard property names (where they exist) for 
its properties. 

- `showFrameBorder` is now called `showBorder`
- `selectedRowColor` is now called `hiliteColor`
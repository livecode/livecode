---
version: 8.0.0-dp-6
---
# Widget Theme

The theme of the header bar widget can now be set to iOS or Android.

The following property has been added:

* **widgetTheme**: either "iOS" or "Android"

The default theme is "iOS".

The following properties have also been added, which can only be set
if the theme of the widget is "Android":

* **colorScheme**: this can be selected from the list of Android color schemes
* **leftAction**: the icon displayed on the left of the header
* **titleVisibility**: whether the title in the header is displayed or not
* **distinctTitle**: whether the title in the header is distinct or not

The following properties of the widget can only be set if the theme of the widget is "iOS":
* **headerSubtitle**
* **leftLabel**
* **actionStyle**
* **showBackIcon**
* **showSearchIcon**
* **actionColor**
* **showDivide**

The following properties of the widget can be set regardless of the theme:
* **widgetTheme**
* **headerTitle**
* **headerActions**
* **backgroundOpacity**

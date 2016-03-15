# Properties

* The segmented control widget has been reworked to make it more fully
  integrated into the IDE and behave more like a classic control.

  The segmented control now uses standard property names for its 
  properties (where they exist). 

  * **multiSelect** is now called **multipleHilites**
  * **showFrameBorder** is now called **showBorder**
  * **segmentColor** is now called **backColor**
  * **segmentLabelColor** is now called **foreColor**
  * **selectedSegmentColor** is now called **hiliteColor**
  * **segmentSelectedLabelColor** is now called **hilitedTextColor**

* All of the existing properties previously named `segment<property>`
  have been renamed to use the word item. This is in order to unify
  related property names of several different widgets.

  * **segmentCount** is now **itemCount**
  * **segmentDisplay** is now **itemStyle**
  * **segmentNames** is now **itemNames**
  * **segmentLabels** is now **itemLabels**
  * **segmentIcons** is now **itemIcons**
  * **segmentSelectedIcons** is now **hilitedItemIcons**
  * **segmentMinWidth** is now **itemMinWidths**
  * **selectedSegments** is now **hilitedItems**

* A new property **hilitedItemNames** has been added, which contains.
  the names of the currently highlighted segments, as opposed to
  **hililtedItems** which returns their indices.

* The **style** property has been removed.

# Appearance and Theming

* When no colors are set on the segmented control, it will use a set
  of native theme colors associated with a list field.

# Signals

* The **segmentSelected** and **segmentUnselected** signals have been
  removed. Use the **hiliteChanged** message instead.

* The **hiliteChanged** message no longer has a parameter. Use the
  **hilitedItems** or **hilitedItemNames** properties as required to
  obtain the new highlight state.

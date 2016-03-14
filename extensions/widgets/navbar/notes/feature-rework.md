# Theming and appearance

* The navbar colors are now controlled by the standard LiveCode
  **foregroundColor**, **backgroundColor**, **hiliteColor** and **borderColor**
  properties.  They can be edited in the property inspector.

* The visibility of the border at the top edge of the navbar is now
  controlled by the standard LiveCode **showBorder** property, which can be
  edited in the property inspector.

* There are no longer separate Android and iOS themes for the navbar.

# Properties

* The **widgetTheme** property has been removed.

* The **backgroundOpacity** property has been removed, and has been replaced by
  the **opaque** property which can be set to `true` or `false`.

* The **showDivide** property has been renamed to **showBorder**.

* The **navData** property has been renamed to **itemArray**.

* The **navNames** property has been renamed to **itemNames**.

* The **navIcons** property has been renamed to **itemIcons**.

* The **navSelectedIcons** property has been renamed to **hilitedItemIcons**.

* The **navLabels** property has been renamed to **itemLabels**.

* The **selectedItem** property has been renamed to **hilitedItem**.

* A new **hilitedItemName** property has been added.  It contains the name of
  the currently-highlighted item.  It can be set to change which item is
  highlighted.

# Signals

* The **navigate** signal has been renamed to **hiliteChanged**, and no longer
  passes any arguments.  The currently-highlighted item can be accessed in a
  **hiliteChanged** handler by using the **hilitedItem** and/or
  **hilitedItemName** properties.

# [15142] Navbar "navigate" message should not send label
# [16981] Navbar docs have incorrect entry for OnSave()

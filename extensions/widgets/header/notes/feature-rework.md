# Theming and appearance

* The header bar colors are now controlled by the standard LiveCode
  **foregroundColor**, **backgroundColor**, **hiliteColor** and **borderColor**
  properties.  They can be edited in the property inspector.

* The visibility of the border at the bottom edge of the header bar is now
  controlled by the standard LiveCode **showBorder** property, which can be
  edited in the property inspector.

* The header bar now uses the **theme** property to control its appearance.
  Currently, the **theme** property is _not_ saved, and is reset to "native"
  whenever the widget is loaded.

# Properties

* The **widgetTheme** property has been removed.  Use the **theme** property
  instead.

* The **colorScheme** property has been removed.  Set the **hiliteColor**,
  **borderColor** and **backgroundColor** properties instead.

* The **actionColor** property has been removed. Use the **hiliteColor**
  property instead.

* The **leftLabel** and **showBackIcon** properties have been removed.
  Use the **firstItemLeft** property instead.

* The **headerSubtitle** property has been removed.

* The **distinctTitle** property has been removed.

* The **headerTitle** property has been renamed to **label**.

* The **titleVisibility** property has been renamed to **showLabel**.

* The **backgroundOpacity** property has been renamed to **opaque**.

* The **showDivide** property has been renamed to **showBorder**.

* The **actionData** property has been renamed to **itemArray**.

* The **actionNames** property has been renamed to **itemNames**.

* The **actionIcons** property has been renamed to **itemIcons**.

* The **actionSelectedIcons** property has been renamed to **hilitedItemIcons**.

* The **actionLabels** property has been renamed to **itemLabels**.

* The **actionStyle** property has been renamed to **itemStyle**.

* A new property **firstItemLeft** has been added. If true, the first 
  action in the **itemArray** is displayed on the left hand side of the
  header bar. On iOS, the icon and label of a left action are both drawn.
  On Android, only the left action icon is drawn.

* A new read-only **mouseAction** property has been added.  It contains 
  the name of the action item that the mouse is currently over, or empty
  if there is no such action.

# Signals

* The **headerAction**, **leftAction** and **backAction** signals have 
  been removed. To execute code in response to an action being clicked,
  use a combination of the **mouseUp** signal and the **mouseAction** 
  property.

# [17006] Header widget throws error if leftLabel is empty
# [16979] Header Widget loses header actions if show back icon unchecked


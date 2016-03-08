# Theming and appearance

* The SVG path widget's paints are now controlled by the standard LiveCode
  **foregroundColor** and **hiliteColor** properties, and can use patterns.
  They can be edited in the Property Inspector.

# Properties

* All properties are now fully integrated into the IDE Property Inspector.

* The **iconColor** and **iconHiliteColor** properties have been removed.  Set
  the standard LiveCode **foregroundColor** and **hiliteColor** properties
  instead.

* The **isHilited** property has been removed.  Use the standard LiveCode
  **hilited** property instead.

* The **iconPathPreset** property has been renamed to **iconPresetName** to
  more accurately reflect the information it contains.

* The **flipVertically** property has been renamed to **flipped**.

* The **iconAngle** property has been renamed to **angle**.

* The **toggleHilite** property has been removed, since the SVG path widget no
  longer handles any mouse events.

# Signals

* The SVG path widget no longer handles any mouse events.  It passes the the
  **mouseDown**, **mouseUp**, **mouseEnter** and **mouseLeave** messages on to
  its widget script.

  * The previous non-toggling behaviour can be reproduced using a script similar
    to:

    ```
    on mouseDown
       set the hilited of me to true
    end mouseDown

    on mouseUp
       set the hilited of me to false
    end mouseUp
    ```

  * The previous toggling behaviour can be reproduced using a script similar to:

    ```
    on mouseUp
       set the hilited of me to (not the hilited of me)
    end if
    ```

# Default script

* The SVG path widget now has a default script.

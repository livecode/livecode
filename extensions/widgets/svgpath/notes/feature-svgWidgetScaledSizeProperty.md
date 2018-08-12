---
version: 8.1.0-dp-1
---
# Properties

* New **scaledWidth** and **scaledHeight** properties have been added.
  These are read-only properties that expose the effective size of the
  rendered SVG path, independent of the widget's size.

  When **maintainAspectRatio** is false, these values are equal
  to the width and height of the widget.

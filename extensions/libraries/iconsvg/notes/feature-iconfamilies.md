# Icon families

* Icons are now organized by "family", and divided into two icon families by
  default: a "fontawesome" family and an "ide" family.  The "fontawesome"
  family is the default family.  This is to enable future extensibility.

* The `iconNames()` handler now lists only the icons in the default family.

* The `iconSVGPathFromName()` and `iconCodepointFromName()` handlers now
  accept icon names in the form "name" or "family/name".  If no family part is
  present, they will search the default family, followed by any other available
  families.

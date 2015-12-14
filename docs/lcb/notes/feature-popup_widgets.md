---
version: 8.0.0-dp-3
---
# LiveCode Builder Host Library
## Popup widgets

* Added the ability to use widgets within popup dialog windows.  New syntax added:

  * `popup widget <Kind> at <Position> [ with properties <Properties> ]`

    Launch the named widget as a popup. The popup can return a value in the result.

  * `currently popped up`

    Test if this widget is part of a popup.

  * `close popup [ returning <Result> ]`

    Set the result of the calling popup statement to `<Result>`.

---
version: 8.0.0-dp-3
---
# LiveCode Builder Host Library
## Widget library

* Added the ability to use widgets within popup dialog windows.

  * `popup widget <Kind> at <Position> [ with properties <Properties> ]`: Launches the named widget as a popup. The popup can return a value in the result.
  * `currently popped up`: Evaluates to true if this widget is part of a popup.
  * `close popup [ returning <Result> ]`: Closes this widget's popup, setting the result of the calling popup statement to `<Result>`.

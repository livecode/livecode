# Known issues

* The installer will currently fail if you run it from a network share on Windows. Please copy the installer to a local disk before launching on this platform.
* The new property inspector lacks some properties present in the old property inspector
* The supplied widgets are examples and lack features and general robustness
* The extension builder plugin “Test” feature fails if the widget being tested is already installed - uninstalling the widget and restarting the IDE should help
* All installed widgets are built into any standalones produced
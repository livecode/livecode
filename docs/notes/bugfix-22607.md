# Dark mode detection

A new `systemAppearance` property has been added which returns `dark` if
the application is running in dark mode and `light` otherwise.

A new `systemAppearanceChanged` message is now sent to the current card of the
defaultStack when the system appearance changes.
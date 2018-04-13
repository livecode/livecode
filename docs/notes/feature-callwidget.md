# Call Widget

The call command can now be used to call a public widget handler directly.
This is useful if a widget should perform an action rather than get or set
a property value. To call a widget handler use the syntax:

    call widget <handername> of <widget reference> with <argumentList>

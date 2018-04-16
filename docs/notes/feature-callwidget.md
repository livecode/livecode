# Call & Send Widget

The call and send commands can now be used to execute a public widget handler
directly. This is useful if a widget should perform an action rather than
get or set a property value. 

To call a widget handler use the syntax:

    call widget <handername> of <widget reference> with <argumentList>

To send a widget handler use the syntax:

    send widget <handername> to <widget reference> [in <time> [{seconds | ticks | milliseconds}] with <argumentList>

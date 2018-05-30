# LiveCode Builder Host Library
## Engine library

* You can now use `the <modifierkey> is [currently] down` expression to
determine if the shift, command, control, alt/option or caps lock keys
are down. Use the optional `currently` adverb to differentiate between
the shift key being down at the present moment and it being down when the
current event occurred.

    if the shift key is down then
       DoShiftKeyThing()
    end if

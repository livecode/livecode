# LiveCode Builder Host Library
## Widget library
New syntax has been added to enable property triggers to be 
fired directly from within a widget: trigger all.

At the moment the only property trigger that exists is the 
‘property listening’ mechanism available in the IDE. You should 
use ‘trigger all’ after a widget property has changed internally 
(for example because something in a native layer has changed) so 
that the IDE can be notified that the property has changed.

In the future single property variants and other types of trigger 
may be introduced.

# minStackFileVersion property

A new stack property has been added to determine the minimum stack file
version that can be safely used to save a stack without data loss. For
example, if a widget is on the stack the `minStackFileVersion` will be
8.0 and saving with a lower stack file version will result in the loss
of the widget from the stack. The minimum `minStackFileVersion` reported
by the property is 7.0.

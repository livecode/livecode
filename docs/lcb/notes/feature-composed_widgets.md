# LiveCode Builder Host Library

## Composed widgets

The ability to compose widget objects has been added. Widgets can either be 'host' widgets, 
created when a widget is directly embedded in a stack, or 'child' widgets which are created 
when a widget is used as a child widget within another widget.

### Syntax

A `Widget` type has been added, so that variables can contain references to child widget objects.
A variable to hold a widget reference can be defined in the usual way, e.g.
```variable tWidget as Widget```

New widget syntax has been added to create, place, unplace and manipulate child widgets.	

* ```a new widget <kind>``` - Creates a widget object of the specified kind.
* ```place <widget> [at (bottom|top) | (below|above) <other widget>]``` - Adds a child widget to the parent on the specified layer.
* ```unplace <widget>``` - Removes a child widget from the parent.
* ```the target``` - Returns the child widget that started the current execution.
* ```my children``` - Returns a list of the currently placed child widgets of this widget.	
* ```property <property> of <widget>``` - Enables manipulation of a property implemented by a child widget.
* ```the rectangle of <widget>``` - Enables manipulation of the rectangle property of a child widget.
* ```the width of <widget>``` - Enables manipulation of the width property of a child widget.
* ```the height of <widget>``` - Enables manipulation of the height property of a child widget.
* ```the location of <widget>``` - Enables manipulation of the location property of a child widget.
* ```the enabled of <widget>``` - Enables manipulation of the enabled property of a child widget.
* ```the disabled of <widget>``` - Enables manipulation of the disabled property of a child widget.
* ```annotation <name> of <widget>``` - Enables tagging of child widgets with named values.

### Events

Events triggered on child widgets (such as OnMouseUp) are automatically passed up to the 
parent, as long as the child's event handler returns nothing. If any event handler returns 
something, the event is considered handled and is not passed to the parent.

### Messages

Messages posted by the child widget can be handled by the parent in an `On<message name>` handler. 
For example, if the child has the code
```post "dataChanged" with [mDataArray]```, 
this can be handled in the parent by adding 
```public handler OnDataChanged(in pArray as Array)```.
Posted messages can only be handled by a direct parent, and a widget's script object will 
only receive messages posted by host widget, i.e. the topmost parent. 

### Example
See https://github.com/livecode/livecode/blob/develop/extensions/widgets/simplecomposed/simplecomposed.lcb 
for an example of how the host/child relationship can be used.


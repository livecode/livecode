# Selected stop API

## selectedStop property
The gradient ramp widget now has a **selectedStop** property whose value is
the index of the widget's selected gradient stop, or 0 if there is no
selected stop.

## selectedStopChanged message
The gradient ramp widget posts a **selectedStopChanged** message when the 
selected stop of the widget is changed either by script (via the 
**selectedStop** property) or by user interaction, i.e. clicking on one 
of the widget's gradient stops.

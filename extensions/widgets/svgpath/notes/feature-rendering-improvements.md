# Implementation

Improved redrawing performance by recording frequently reused values and by
combining flipping, rotating, scaling, and translation of the path into a single
transform operation applied to the canvas. 

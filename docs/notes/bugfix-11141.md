#Gradient ramp rounding error
When the ramp offset value of a grandient carries 5 digits, the value parsed from the string keeps its value.
Previously, it was rounded 0.00001 under its initial value, due to the parsing and rounding steps.
Adding 0.000005 to the value before storing and displaying it fixes the rounding error.
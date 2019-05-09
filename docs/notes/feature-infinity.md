# Infinity
The constant `infinity` has been added to the language.

This constant resolves to the floating point value
representing positive infinity. When `infinity` is used in
math operations, they never throw errors. Usually they will
return `infinity` or `NaN` (not a number)

    (infinity / 0)^2 - 5 = infinity

    infinity / infinity = NaN

# Ensure 'the effective rect of stack' is more accurate on Linux

With the move to GDK since 7.0 there is a better method for computing the effective rect of a window. The engine has been updated to use this method, rather than the heuristic which was there before.

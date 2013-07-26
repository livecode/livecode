# Scrollbar properties not returned in correct format.

Previously these properties returned different values when obtained as numbers and string - as strings they returned the value rounded. Now they return the same value - rounded to a precision specified by the numberFormat of the scrollbar.

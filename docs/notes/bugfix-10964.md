# Inconsistent rounding of floating point values.
Previously when setting properties expecting integers, some properties would truncate real values, others would round-to-nearest. The rounding mode for all conversions is now consistent - round-to-nearest.

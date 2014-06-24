# Spaces required between numeric and non-numeric components of a date to parse correctly

Date format string parsing has been made less strict in its processing, now collapsing one or more spaces together to allow a ' ' in a format string to match one or more input spaces. Additionally, spaces in the input after a formatting specifier was successfully matched are ignored. With these changes, both "10:41 PM" and "10:41PM" are accepted as valid times; previously, only the former was acceptable.

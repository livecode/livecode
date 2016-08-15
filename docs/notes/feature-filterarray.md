# Filtering array keys and elements has been added to the filter command

The filter command now supports the filtering of arrays by matching keys
or elements.

Example:

    local tArray
    put true into tArray["foo"]
    put false into tArray["bar"]
    filter keys of tArray with "f*"
    put the keys of tArray is "foo"

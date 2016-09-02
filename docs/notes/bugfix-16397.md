# Change in empty is among the items of `1,2,3,` behavior

In previous versions of LiveCode `is among the items of` behaved
incorrectly when a trailing delimiter was present. Trailing delimiters
are generally ignored in the LiveCode engine and legacy behaviors where
they are not are documented anomalies that are maintained for backwards
compatibility reasons. This change resolves the anomaly detailed in
[bug report 16297](http://quality.livecode.com/show_bug.cgi?id=16397)

For example:

    put the number of items of "1,2,3," is 3 --> puts true in all engines
    put empty is among the items of "1,2,3," --> puts true in LiveCode 6
    put empty is among the items of "1,2,3," --> puts false in LiveCode 8

> **Warning** If your stacks depend on the legacy anomalous behavior
> they should be checked to ensure all trailing empty items are wrapped
> by an item delimiter 

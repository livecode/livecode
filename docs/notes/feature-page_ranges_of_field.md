# Getting the Page Ranges of a Field
There is a new property **the pageRanges of <field>**. This property (notionally) splits up the field content into pages based on the height of the field, and then returns a return-delimited list of char ranges. Each char range corresponds to an individual page.

For example, for a field capable of displaying two lines of text and containing these lines:
`Line 1
Line 2
Line 3
Line 4
Line 5
Line 6`
The **pageRanges** would return:
`1,1415,2829,42`

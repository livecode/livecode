# JSON parser improvements

* `JsonImport()` no longer incorrectly accepts garbage at the end of a
  JSON file.

* `JsonImport()` no longer incorrectly accepts unescaped control
  characters in strings

* "null" is a valid JSON file, and `JsonImport("null")` no longer
  throws an error. It returns `nothing` in LCB and the empty string in
  LiveCode Script.

* A number by itself is a valid JSON file, and `JSONImport("25")` now
  returns 25, rather than throwing a syntax error.

# [18697] Fix parsing of "lonely number" JSON files

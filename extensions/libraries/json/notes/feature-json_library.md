# JSON Library Added

An LCB library, **com.livecode.library.json**, has been written to provide support for generating 
and parsing JavaScript Object Notation (JSON) data.  See also <http://json.org>. 

## Functions
The library has two public handlers, `JsonImport` and `JsonExport`. `JsonImport` takes a string 
containing JSON-formatted text and parses it into a LiveCode value. `JsonExport` takes a LiveCode 
value and returns the equivalent value as a string in JSON format.

## Using the library
The library is automatically loaded into the IDE, and the `JsonImport` and `JsonExport` handlers 
placed in the message path where they are available to call from any object.

In LiveCode Script, `JsonExport` takes any value and converts it to a string representing a JSON 
encoded value.

To use the library from a LiveCode Builder widget or library, simply add it to the list of use 
clauses:

    use com.livecode.library.json

When using the `JsonExport` handler in LCB, an error is thrown if the value passed 
is not of one of the following types:

- String
- Number
- List
- Array
- Boolean
- nothing

## Examples

From LiveCode Script:

    local tData, tJSON
    put "a,b,c,d" into tData
    split tData by comma
    put JsonExport(tData) into tJSON -- contains {"1": "a","2": "b","3": "c","4": "d"}

From LiveCode Builder:

	variable tJSON as String
	put "[1,1,2,3,5,8]" into tJSON
	
	variable tData as List
	put JsonImport(tJSON) into tData -- contains [1,1,2,3,5,8]

# Improved properties Property
The 'properties' object property has been significantly revised. In particular on fetching:
* It returns the minimal set of properties of an object to allow it to be recreated exactly.
* It returns unicode variants of properties if, and only if, they are needed.
And on storing it will ensure that properties are set in the correct order to ensure correct recreation of the object.
*Note:* When setting properties which contain both non-empty color and pattern properties of the same type, the color property will take precedence.

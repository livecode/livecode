# Consistent value union and intersect semantics

The array union and intersect and their recursive counterparts now follow a consistent
recipe to produce their result.

`union <left> with <right>` now has the semantics of the following LiveCode function:

```
function ArrayUnion(pLeft, pRight, pRecursive)
	repeat for each key tKey in pRight
		if tKey is not among the keys of pLeft then
			put pRight[tKey] into pLeft[tKey]
		else if pRecursive then
			put ArrayUnion(pLeft[tKey], pRight[tKey], true) into pLeft[tKey]
		end if
	end repeat
	
	return pLeft
end ArrayUnion
```

and `intersect <left> with <right>` the following:

```
function ArrayIntersect(pLeft, pRight, pRecursive)
	repeat for each key tKey in pLeft
		if tKey is not among the keys of pRight then
			delete variable pLeft[tKey]
		else if pRecursive then
			put ArrayIntersect(pLeft[tKey], pRight[tKey], true) into pLeft[tKey]
		end if
	end repeat
	
	return pLeft
end ArrayIntersect
```

Previously the semantics for intersect were different depending on whether the intersect
was in a recursive step or not. The two affected forms are:

`intersect <string> with <value>`
- in 6.7, `<string>` becomes `empty`.
- in 8.0 DP 10, `<string>` is preserved.

and 

```
put "a" into tLeftArray[1][1]
put "b" into tRightArray[1]
intersect tLeftArray with tRightArray recursively
```
- in 6.7 `tLeftArray` is unchanged.
- from 8.0 DP 10, `tLeftArray[1]` becomes `empty`
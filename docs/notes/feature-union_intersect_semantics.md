---
version: 8.0.0-dp-10
---
# Consistent value union and intersect semantics

The array **union** and **intersect** commands and their recursive
counterparts now follow a consistent recipe to produce their result.

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

Previously the semantics for **intersect** were different depending on
whether the intersect was in a recursive step or not. The two affected
forms are:

```
intersect <string> with <value>
```

- in LiveCode 6.7, `<string>` becomes `empty`.
- in LiveCode 8.0, `<string>` is unchanged.

```
put "a" into tLeftArray[1][1]
put "b" into tRightArray[1]
intersect tLeftArray with tRightArray recursively
```

- in LiveCode 6.7 `tLeftArray` is unchanged.
- in LiveCode 8.0, `tLeftArray[1]` becomes `empty`

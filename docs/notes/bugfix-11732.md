# <> operator is different from 'is not' operator for arrays
The <> operator will now work identically to 'is not' when comparing with an array. Previously, the <> operator would convert arrays to the empty string before comparing.

More specifically:
   put 100 into tLeft[1]
   put tLeft is empty -- false
   put tLeft = empty -- false (same as is)
   put tLeft is not empty -- true
   put tLeft <> empty -- true (previously this was false)
   
Note: This is a subtle change which could impact existing scripts. However, since '<>' and 'is not' should be synonyms it is considered that it is more likely that it will fix unnoticed bugs in existing scripts rather than cause them.

# New vectorDotProduct function

A new **vectorDotProduct** function has been added.  It computes the
vector dot product of two single-dimensional arrays with identical
keys.

More specifically:

    vectorDotProduct(tArray1, tArray2)

Will compute:

    put 0.0 into tSum
    repeat for each key tKey in tArray1
        add tArray1[tKey] * tArray2[tKey] to tSum
    end repeat
    return tSum

If the two arrays do not have the same set of keys, then an error is
thrown.

# Improve efficiency of compiled regex cache

The efficiency of lookups of previously compiled regexs has been
improved. To take full advantage of the regex cache make sure
that you either use a string constant for the regex pattern, or
a variable which is not mutated between uses. e.g.

    get matchText(tTarget, "someregexpattern") -- efficient
    get matchText(tTarget, tUnchangedPatternVar) -- efficient
    get matchText(tTarget, tPatternPrefix & tPatternSuffix) -- inefficient

In general you will only gain advantage from the regex cache
if you repeated use the same regex pattern in the way described
above repeatedly in a tight loop.

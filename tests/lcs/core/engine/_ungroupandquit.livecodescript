script "_ungroupandquit"
on startup
    create button "one"
    create button "two"
    group button "one" and button "two"
    ungroup the last group
    quit 0
end startup
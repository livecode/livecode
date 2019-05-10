script "CoreConstants"
/*
Copyright (C) 2019 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

on TestConstantNameEqualToConstantValue
    local tInfinity = infinity
    TestAssert "infinity as uql is infinity with no further context", \
            tInfinity is infinity
    TestAssert "infinity as uql is 'infinity' in string context", \
            tInfinity & " " is "infinity "
    TestAssert "infinity as uql is inf in numeric context", \
            tInfinity + 0 is infinity


    local tTrue = true
    TestAssert "true as uql is true with no further context", \
            tTrue is true
    TestAssert "true as uql is 'true' in string context", \
            true & " " is "true "
    TestAssert "true as uql is true in boolean context", \
            tTrue

    local tFalse = false
    TestAssert "false as uql is true with no further context", \
            tFalse is false
    TestAssert "false as uql is 'true' in string context", \
            tFalse & " " is "false "
    TestAssert "false as uql is true in boolean context", \
            not tFalse

    local tDown = down
    TestAssert "down as uql is 'down'", tDown is "down"

    local tUp = up
    TestAssert "up as uql is 'up'", tUp is "up"

    local tLeft = left
    TestAssert "left as uql is 'left'", tLeft is "left"

    local tRight = right
    TestAssert "right as uql is 'done'", tRight is "right"

    local tDone = done
    TestAssert "done as uql is 'done'", tDone is "done"
end TestConstantNameEqualToConstantValue

on TestConstantUqlNoExplicitVars
    set the explicitVariables to false

    local tEmpty = empty
    TestAssert "empty as uql is 'empty'", tEmpty is "empty"
end TestConstantUqlNoExplicitVars

on TestNumericConstants
    TestAssert "constant one is 1", one is 1
    TestAssert "constant two is 2", two is 2
    TestAssert "constant three is 3", three is 3
    TestAssert "constant four is 4", four is 4
    TestAssert "constant five is 5", five is 5
    TestAssert "constant six is 6", six is 6
    TestAssert "constant seven is 7", seven is 7
    TestAssert "constant eight is 8", eight is 8
    TestAssert "constant nine is 9", nine is 9
    TestAssert "constant ten is 10", ten is 10
end TestNumericConstants

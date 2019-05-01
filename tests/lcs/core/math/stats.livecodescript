script "CoreMathStats"
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

private function ComputeAverage pMeanKind
    local tTotal, tCount, tParams
    put char 1 to -2 of item 2 to -1 of the params into tParams
    put 0 into tTotal
    put the number of items in tParams into tCount
    if tCount is 0 then
        return tTotal
    end if
    repeat for each item tItem in tParams
        local tValue
        // Strip quotes
        put char 2 to -2 of tItem into tValue
        switch pMeanKind
        case "arithmetic"
            add tValue to tTotal
            break
        case "harmonic"
            add (1 / tValue) to tTotal
            break
        case "geometric"
            if tTotal is 0 then
                put 1 into tTotal
            end if
            multiply tTotal by (tValue ^ (1 / tCount))
            break
        end switch
    end repeat

    switch pMeanKind
    case "arithmetic"
        return tTotal / tCount
    case "harmonic"
        return tCount / tTotal
    case "geometric"
        return tTotal
        break
    end switch
end ComputeAverage

on TestAverage
    TestAssert "test arithmetic mean no params", average() is ComputeAverage("arithmetic")
    TestAssert "test arithmetic mean one param", average(1.23) is ComputeAverage("arithmetic", 1.23)
    TestAssert "test arithmetic mean string param", average("1, 2, 3, 4, 5") is ComputeAverage("arithmetic", 1, 2, 3, 4, 5)
    TestAssert "test arithmetic mean numeric params", average(1, 2, 3, 4, 5) is ComputeAverage("arithmetic", 1, 2, 3, 4, 5)

    local tArray
    repeat with i = 1 to 5
        put i into tArray[i]
    end repeat
    TestAssert "test arithmetic mean array param", average(tArray) is ComputeAverage("arithmetic", 1, 2, 3, 4, 5)
    TestDiagnostic average(tArray)
end TestAverage

on TestHarmonicMean
    TestAssert "test harmonic mean no params", harmonicMean() is ComputeAverage("harmonic")
    TestAssert "test harmonic mean one param", harmonicMean(1.23) is ComputeAverage("harmonic", 1.23)
    TestAssert "test harmonic mean string param", harmonicMean("1, 2, 3, 4, 5") is ComputeAverage("harmonic", 1, 2, 3, 4, 5)
    TestAssert "test harmonic mean numeric params", harmonicMean(1, 2, 3, 4, 5) is ComputeAverage("harmonic", 1, 2, 3, 4, 5)

    local tArray
    repeat with i = 1 to 5
        put i into tArray[i]
    end repeat
    TestAssert "test arithmetic mean array param", harmonicMean(tArray) is ComputeAverage("harmonic", 1, 2, 3, 4, 5)
end TestHarmonicMean

on TestGeometricMean
    TestAssert "test geometric mean no params", geometricMean() is ComputeAverage("geometric")
    TestAssert "test geometric mean one param", geometricMean(1.23) is ComputeAverage("geometric", 1.23)
    TestAssert "test geometric mean string param", geometricMean("1, 2, 3, 4, 5") is ComputeAverage("geometric", 1, 2, 3, 4, 5)
    TestAssert "test geometric mean numeric params", geometricMean(1, 2, 3, 4, 5) is ComputeAverage("geometric", 1, 2, 3, 4, 5)

    local tArray
    repeat with i = 1 to 5
        put i into tArray[i]
    end repeat
    TestAssert "test geometric mean array param", geometricMean(tArray) is ComputeAverage("geometric", 1, 2, 3, 4, 5)
end TestGeometricMean

private function ComputeStats pFunction
    local tTotal, tCount, tParams, tMean
    put char 1 to -2 of item 2 to -1 of the params into tParams
    put 0 into tTotal
    put the number of items in tParams into tCount
    if tCount is 0 then
        return tTotal
    end if

    if (pFunction is "sampleStandardDeviation" or pFunction is "sampleVariance") \
            and tCount is 1 then
        return 0
    end if

    local tParamArray
    repeat with x = 1 to the number of items in tParams
        // Strip quotes
        put char 2 to -2 of item x of tParams into tParamArray[x]
    end repeat
    put average(tParamArray) into tMean

    repeat for each element tValue in tParamArray
        subtract tMean from tValue
        switch pFunction
        case "averageDeviation"
            add abs(tValue) to tTotal
            break
        case "populationStandardDeviation"
        case "populationVariance"
        case "sampleStandardDeviation"
        case "sampleVariance"
            add tValue^2 to tTotal
            break
        end switch
    end repeat

    switch pFunction
    case "averageDeviation"
        return tTotal / tCount
    case "populationStandardDeviation"
        return sqrt(tTotal / tCount)
    case "populationVariance"
        return tTotal / tCount
    case "sampleStandardDeviation"
        return sqrt(tTotal / (tCount - 1))
    case "sampleVariance"
        return tTotal / (tCount - 1)
    end switch
end ComputeStats

on TestAverageDeviation
    TestAssert "test average deviation no params", averageDeviation() is ComputeStats("averageDeviation")
    TestAssert "test average deviation one param", averageDeviation(1.23) is ComputeStats("averageDeviation", 1.23)
    TestAssert "test average deviation string param", averageDeviation("1, 2, 3, 4, 5") is ComputeStats("averageDeviation", 1, 2, 3, 4, 5)
    TestAssert "test average deviation numeric params", averageDeviation(1, 2, 3, 4, 5) is ComputeStats("averageDeviation", 1, 2, 3, 4, 5)

    local tArray
    repeat with i = 1 to 5
        put i into tArray[i]
    end repeat
    TestAssert "test average deviation array param", averageDeviation(tArray) is ComputeStats("averageDeviation", 1, 2, 3, 4, 5)
end TestAverageDeviation

on TestPopulationStandardDeviation
    TestAssert "test population standard deviation no params", populationStandardDeviation() is ComputeStats("populationStandardDeviation")
    TestAssert "test population standard deviation one param", populationStandardDeviation(1.23) is ComputeStats("populationStandardDeviation", 1.23)
    TestAssert "test population standard deviation string param", populationStandardDeviation("1, 2, 3, 4, 5") is ComputeStats("populationStandardDeviation", 1, 2, 3, 4, 5)
    TestAssert "test population standard deviation numeric params", populationStandardDeviation(1, 2, 3, 4, 5) is ComputeStats("populationStandardDeviation", 1, 2, 3, 4, 5)

    local tArray
    repeat with i = 1 to 5
        put i into tArray[i]
    end repeat
    TestAssert "test population standard deviation array param", populationStandardDeviation(tArray) is ComputeStats("populationStandardDeviation", 1, 2, 3, 4, 5)
end TestPopulationStandardDeviation

on TestPopulationVariance
    TestAssert "test population variance no params", populationVariance() is ComputeStats("populationVariance")
    TestAssert "test population variance one param", populationVariance(1.23) is ComputeStats("populationVariance", 1.23)
    TestAssert "test population variance string param", populationVariance("1, 2, 3, 4, 5") is ComputeStats("populationVariance", 1, 2, 3, 4, 5)
    TestAssert "test population variance numeric params", populationVariance(1, 2, 3, 4, 5) is ComputeStats("populationVariance", 1, 2, 3, 4, 5)

    local tArray
    repeat with i = 1 to 5
        put i into tArray[i]
    end repeat
    TestAssert "test population variance array param", populationVariance(tArray) is ComputeStats("populationVariance", 1, 2, 3, 4, 5)
end TestPopulationVariance

on TestSampleStandardDeviation
    TestAssert "test sample standard deviation no params", sampleStandardDeviation() is ComputeStats("sampleStandardDeviation")
    TestAssert "test sample standard deviation one param", sampleStandardDeviation(1.23) is ComputeStats("sampleStandardDeviation", 1.23)
    TestAssert "test sample standard deviation string param", sampleStandardDeviation("1, 2, 3, 4, 5") is ComputeStats("sampleStandardDeviation", 1, 2, 3, 4, 5)
    TestAssert "test sample standard deviation numeric params", sampleStandardDeviation(1, 2, 3, 4, 5) is ComputeStats("sampleStandardDeviation", 1, 2, 3, 4, 5)

    local tArray
    repeat with i = 1 to 5
        put i into tArray[i]
    end repeat
    TestAssert "test sample standard deviation array param", sampleStandardDeviation(tArray) is ComputeStats("sampleStandardDeviation", 1, 2, 3, 4, 5)
end TestSampleStandardDeviation

on TestSampleVariance
    TestAssert "test sample variance no params", sampleVariance() is ComputeStats("sampleVariance")
    TestAssert "test sample variance one param", sampleVariance(1.23) is ComputeStats("sampleVariance", 1.23)
    TestAssert "test sample variance string param", sampleVariance("1, 2, 3, 4, 5") is ComputeStats("sampleVariance", 1, 2, 3, 4, 5)
    TestAssert "test sample variance numeric params", sampleVariance(1, 2, 3, 4, 5) is ComputeStats("sampleVariance", 1, 2, 3, 4, 5)

    local tArray
    repeat with i = 1 to 5
        put i into tArray[i]
    end repeat
    TestAssert "test sample variance array param", sampleVariance(tArray) is ComputeStats("sampleVariance", 1, 2, 3, 4, 5)
end TestSampleVariance

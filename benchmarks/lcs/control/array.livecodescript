script "ControlArray"
/*
Copyright (C) 2017 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of  the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

on BenchmarkArrayBug17434
	local tMax
	put 100 into tMax

	local sA
	BenchmarkStartTiming "Bug 17434 - 3-dim matrix"
	repeat with i = 1 to tMax
		repeat with j = 1 to tMax
			repeat with k = 1 to tMax
				put any item of "bob,carol,ted,alice" into sA[i][j][k]
			end repeat
		end repeat
	end repeat
	BenchmarkStopTiming
end BenchmarkArrayBug17434

on BenchmarkArrayWordCount
	/* Generate 10Mb of text from a public domain book */
	local tContent
	BenchmarkLoadNativeTextFile "the_adventures_of_sherlock_holmes.txt"
	repeat while (the number of chars in tContent) < 10 * 1024 * 1024
		put the result after tContent
	end repeat

	local tWordCount, tKeyWords, tResult
	BenchmarkStartTiming "WordCount - iterate"
	repeat for each word tWord in tContent
	end repeat
	BenchmarkStopTiming

	put empty into tWordCount
	BenchmarkStartTiming "WordCount - count"
	repeat for each word tWord in tContent
		add 1 to tWordCount[tWord]
	end repeat
	BenchmarkStopTiming

	BenchmarkStartTiming "WordCount - keys then sort then combine"
	put the keys of tWordCount into tKeyWords
	sort tKeyWords
	repeat for each line tWord in tKeyWords
		put tWord & tab & tWordCount[tWord] & return after tResult
	end repeat
	BenchmarkStopTiming

	put empty into tResult
	BenchmarkStartTiming "WordCount - combine then sort"
	repeat for each line tWord in tKeyWords
		put tWord & tab & tWordCount[tWord] & return after tResult
	end repeat
	set the itemDelimiter to tab
	sort tResult by item 1 of each
	BenchmarkStopTiming
end BenchmarkArrayWordCount

on BenchmarkArrayFilterRecords
	local tRecords, tFilteredRecords
	put _BenchmarkArrayFilterGetRecords() into tRecords

	local tWantedColumns
	put "First name,Test1,Grade" into tWantedColumns

	local tListTime
	BenchmarkStartTiming "FilterRecords - stringlist"
	repeat 100000 times
		get _BenchmarkArrayFilterRecords(tRecords, tWantedColumns)
	end repeat
	BenchmarkStopTiming

	split tWantedColumns by comma as set
	BenchmarkStartTiming "FilterRecords - set"
	repeat 100000 times
		get _BenchmarkArrayFilterRecordsSet(tRecords, tWantedColumns)
	end repeat
	BenchmarkStopTiming
end BenchmarkArrayFilterRecords

private function _BenchmarkArrayFilterRecords pRecords, pColumnNames
   local tFilteredRecords, tFilteredRecord
   repeat for each element tRecord in pRecords
      repeat for each item tColumnName in pColumnNames
         put tRecord[tColumnName] into tFilteredRecord[tColumnName]
      end repeat
      put tFilteredRecord into \
            tFilteredRecords[the number of elements in tFilteredRecords + 1]
   end repeat
   return tFilteredRecords
end _BenchmarkArrayFilterRecords

private function _BenchmarkArrayFilterRecordsSet pRecords, pColumnNameSet
   local tFilteredRecords, tFilteredRecord
   repeat for each element tRecord in pRecords
      repeat for each key tColumnName in pColumnNameSet
         put tRecord[tColumnName] into tFilteredRecord[tColumnName]
      end repeat
      put tFilteredRecord into \
            tFilteredRecords[the number of elements in tFilteredRecords + 1]
   end repeat
   return tFilteredRecords
end _BenchmarkArrayFilterRecordsSet

private function _BenchmarkArrayFilterGetRecords
	local tData
	BenchmarkLoadNativeTextFile "example_tsv_records.txt"
	put the result into tData

	local tColumnNames
	put line 1 of tData into tColumnNames
	split tColumnNames by tab

	local tRecords, tRecordIndex
	put 1 into tRecordIndex
	repeat for each line tRow in line 2 to -1 of tData
		split tRow by tab
		repeat with tColumnIndex = 1 to the number of elements in tRow
			put tRow[tColumnIndex] into \
				tRecords[tRecordIndex][tColumnNames[tColumnIndex]]
		end repeat
		add 1 to tRecordIndex
	end repeat
	return tRecords
end _BenchmarkArrayFilterGetRecords

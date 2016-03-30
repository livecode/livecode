script "JSONLibraryNumbers"

on TestSetup
	TestLoadExtension "com.livecode.library.json"
end TestSetup

-- Bug 17239
on TestExportInteger
	local tArray, tExpected
	put (14592610+0) into tArray[1]
	put "{" & quote & "1" & quote & ": 14592610}" into tExpected

	local tJson
	put JsonExport(tArray) into tJson
	TestAssertBroken "export large integer losslessly", tExpected is tJson, \
			"bug 17239"
end TestExportInteger

-- Bug 17239
on TestImportScientific
	local tJson, tExpected
	put "[ 1.459260e+09 ]" into tJson
	put 1.459260e+09 into tExpected[1]

	local tArray
	put JsonImport(tJson) into tArray
	TestAssert "import number in scientific notation", tExpected is tArray
end TestImportScientific

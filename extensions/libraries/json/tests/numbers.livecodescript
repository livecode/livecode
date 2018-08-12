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

-- Bug 18697
on __TestImportLonelyInt_Minus pParam
   get JSONImport("-")
end __TestImportLonelyInt_Minus
on TestImportLonelyInt
	local tJson, tExpected, tValue

	TestPlan 2

	put "42" into tJson
	put 42 into tExpected
	put JsonImport(tJson) into tValue
	TestAssert "import lonely multi-digit integer", tExpected is tValue

	put "2" into tJson
	put 2 into tExpected
	put JsonImport(tJson) into tValue
	TestAssert "import lonely single-digit integer", tExpected is tValue

	TestAssertThrow "import lonely minus sign", "__TestImportLonelyInt_Minus", \
		the long id of me, "EE_EXTENSION_ERROR_DOMAIN"
end TestImportLonelyInt

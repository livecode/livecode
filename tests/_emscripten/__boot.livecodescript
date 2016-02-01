script "EmscriptenTestRunner"

private command TestLoadLibrary pBasename
	local tStackname
	put the name of stack ("tests/" & pBasename & ".livecodescript") into tStackname

	dispatch revLoadLibrary to tStackname
end TestLoadLibrary

on startup

	-- Load test libraries
	write "# Loading test libraries" & return to stdout
	TestLoadLibrary "_testlib"
	TestLoadLibrary "_testerlib"
	TestLoadLibrary "_inputlib"

	-- Run tests
	local tTestFiles, tTestFile, tStackName, tCommand, tSetupResult, tError

	put TesterGetTestFileNames("/boot/standalone") into tTestFiles

	repeat for each element tTestFile in tTestFiles
		-- Ensure that the test stack filename is absolute
		if the first char of tTestFile is not slash then
			put "/boot/standalone/" before tTestFile
		end if

		put the name of stack tTestFile into tStackName

		TestDiagnostic "framework: Running tests from" && tTestFile
		write return to stdout

		repeat for each element tCommand in TesterParseTestCommandNames(tTestFile)

			TestDiagnostic "framework: Running" && tCommand

			-- Send setup command and check if skip is requested
			dispatch "TestSetup" to tStackName
			put the result into tSetupResult
			if word 1 of tSetupResult is "skip" then
				TestSkip tCommand, word 2 to -1 of tSetupResult
				write return to stdout
				next repeat
			end if

			-- Run tests
			try
				dispatch tCommand to tStackName
			catch tError
				TestDiagnostic tError
				TestAssert empty, false
			end try

			-- Send teardown command
			dispatch "TestTearDown" to tStackName

			write return to stdout
		end repeat
	end repeat

end startup

on ErrorDialog executionError, parseError
   write executionError & return to stderr
   quit 1
end ErrorDialog

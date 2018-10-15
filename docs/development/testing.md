# How to test LiveCode

Tests are small programs that check that a particular, specific function works correctly.  They are run automatically to check whether LiveCode works properly.  They're really useful for ensuring that changes to one part of LiveCode don't break other things!

The main LiveCode engine repository contains the following sets of tests ("test suites"):

* **LiveCode Script tests:** script-only stacks that are run using the LiveCode standalone engine.  They test features of the LiveCode Script language.
* **LiveCode Builder tests:** LCB modules that are run using the **lc-run** tool.   They test features of the LCB core language and standard library.
* **LiveCode Builder Compiler Frontend tests:** Fragments of LCB code which are run through the compiler and check that the compile succeeds, or emits the correct warnings or errors.
* **LiveCode Script parser tests:** Fragments of LCS code which are run through the parser and check that the compile succeeds, or emits the correct warnings or errors.
* **C++ tests:** low-level tests written in C++ using [Google Test](https://github.com/google/googletest).  These perform low-level checks for things that can't be tested any other way.

## Running the Tests

This assumes that you've already got the LiveCode source code and that you've successfully compiled it.  See the [installation instructions](../README.md) for more details.

### Running tests on Mac OS X and Linux

From the top directory of the livecode git repository working tree, run `make check`.  This will run all the test suites.

### Running tests on Windows

Open the `livecode.sln` solution file in Visual Studio, and build the "check" project.  This will run the C++-based tests.

There's not currently a convenient way to run the LiveCode Script and LiveCode Builder tests on Windows.

### Running tests on Emscripten

To run the C++ tests, run `make check-emscripten` from the top of the livecode git repository working tree.

To run the LiveCode Script tests:

1) Run `tools/emscripten_testgen.sh`.  This generates an HTML5 standalone in the `_tests/emscripten` directory.

2) Open `_tests/emscripten/tests.html` in a web browser.

The tests are run automatically as the web page loads, and the TAP log
output is shown in the browser.

## Writing Tests

If at all possible, please add tests whenever make a change to LiveCode -- whether it's a feature added, a bug fixed, or a behaviour tweaked.

### LiveCode Script

Script-only stack-based tests live in the `tests/lcs` directory and its subdirectories.

Each group of related tests lives in a suitably-named `.livecodescript` file.  For example, tests related to desktop clipboard integration are located in `tests/lcs/core/engine/clipboard.livecodescript`.  When you add a new script-only stack file, it'll get picked up by the test suite automatically; there's no need to add it to a list anywhere.

Each script-only stack contains a set of test commands, with names beginning with `Test`.  Each test command gets run in a fresh copy of LiveCode.  A test command might look like:

````
on TestMyFeature
   -- Test actions and assertions go here
end TestMyFeature
````

Before running each test command, the test framework inserts a test library stack, called `TestLibrary`, into the backscripts.  This provides a set of useful utility handlers that can be used when writing test commands.  Currently, the following are available:

* `TestDiagnostic pMessage`: Write *pMessage* to the test log as a message.
* `TestAssert pDescription, pExpectTrue`: Make a test assertion.  The test is recorded as a failure if *pExpectTrue* is false.  *pDescription* should be a short string that describes the test (e.g. "clipboard is clear").
* `TestSkip pDescription, pReasonSkipped`: Record a test as having been skipped.  *pReasonSkipped* should be a short explanation of why the test was skipped (e.g. "not supported on Windows").
* `TestSkipIf pRequirement, pOptions`: Skip a test if the requirements 
are met. `pOptions` varies depending on the `pRequirement` enum (if no 
options are explicitly specified then no options are available for that
particular `pRequirement`. The following requirements are implemented:
   - `ide` - the IDE repo is available
   - `lcb` - LCB compilation supported
   - `docs` - the docs are available
   - `standalone` - the test is running in the standalone test runner
   - `securityPermissions` - Option `set` to skip if a test should not
   set the `securityPermissions`
   - `platform` - options are comma delimited platform strings
   - `processor` - options are comma delimited processor strings
   - `stack` - options are comma delimited stack names to test if they
   are available
   - `environment` - options are comma delimited environment strings
   - `clipboard` - access to the clipboard is available
   - `wait` - the `wait` command is available and works as expected
   - `security` - the security module is available
   - `write` - write access to the filesystem is available
   - `ui` - the test is running in a graphical environment (as opposed
   to the command line)
   - `desktop` - the test is running on a desktop computer
   - `mobile` - the test is running on a mobile device
   - `external` - an external module can be loaded/used. Options are a
   comma delimited list of external module names
   - `database` - an database module can be loaded/used. Options are a
   comma delimited list of external module names
   - `jvm` - the Java Virtual Machine is available
* `TestSkipIfNot pRequirement, pOptions`: Skip a test if the 
requirements are not met. Requirements and options are the same as for 
`TestSkipIf`.
* `TestAssertBroken pDescription, pExpectTrue, pReasonBroken`: The same as `TestAssert`, but marking the test as "expected to fail".  *pReasonBroken* should be a short explanation of why the test is currently expected to fail; it should almost always be a reference to a bug report, e.g. "bug 54321".
* `TestAssertThrow pDescription, pHandlerName, pTarget, pExpectedError, pParam`: Assert that a given handler triggers the expected error message. *pHandlerName* is the name of the handler containing the script expected to cause an error; it is dispatched to *pTarget* with *pParam* as a parameter within a try/catch structure. *pExpectedError* is the expected script execution error name in the enumeration in engine/src/executionerrors.h - e.g. `"EE_PROPERTY_CANTSET"`.
* `TestAssertDoesNotThrow pDescription, pHandlerName, pTarget, pParam`: Assert that a given handler does not trigger any exceptions. *pHandlerName* is the name of the handler containing the script expected to cause an error; it is dispatched to *pTarget* with *pParam* as a parameter within a try/catch structure.
* `TestGetEngineRepositoryPath`: A function that returns the path to the main LiveCode engine repository.
* `TestGetIDERepositoryPath`: A function that returns the path to the LiveCode IDE repository.
* `TestLoadExtension pName`: Attempt to load the extension with name `pName`, eg `TestLoadExtension "json"` will load the JSON library extension.
* `TestLoadAllExtensions`: Attempt to load all available extensions.
* `TestRepeat pDesc, pHandler, pTarget, pTimeOut, pParamsArray`: Repeatedly check the result of a handler for a test. The test is recorded as a success if the result is ever true before the given time runs out, or a failure otherwise.
	- `pHandlerName` is the name of the handler which returns a result.
	- `pTarget` is the object to which `pHandlerName` should be dispatched.
	- `pTimeOut` is the amount of milliseconds to continue testing the result of the handler.
	- `pParamsArray` is an array of parameters, keyed by the 1-based index of the required parameter to be passed to the handler.
* `TestAssertErrorDialog pDescription, pErrorCode`: Assert that this test triggers an errorDialog message with the given error.
* `TestIsInStandalone()`: Checks if the test is being run by the
standalone test runner.

Tests can have additional setup requirements before running, for example loading custom libraries. If the script test contains a handler called `TestSetup`, this will be run prior to running each test command. For example:
````
on TestSetup
   TestSkipIfNot "docs"
   -- All the tests in this script require access to the docs parser
   start using stack (TestGetEngineRepositoryPath() & slash & "ide-support" & slash & "revdocsparser.livecodescript")
end TestSetup
````

The `TestSetup` handler can indicate that a test should be skipped
*entirely* by returning a value that begins with the word "skip" or by
using the `TestSkipIf` or `TestSkipIfNot` commands. For example:

````
on TestSetup
   TestSkipIfNot "platform", "Win32"
   -- is the same as
   if the platform is not "Win32" then
      return "SKIP Feature is only supported on Windows"
   end if
end TestSetup
````


Tests may need to clean up temporary files or other resources after running.  If a script test contains a handler called `TestTeardown`, this will be run after running each test command -- even if the test failed.  N.b. `TestTeardown` won't be run if running the test command causes an engine crash.

Any new objects created on the test stack, `mainstacks`, `sockets`,
`open processes` and `open files` are automatically cleared after each
test and do not need to be included in the `TestTeardown` handler or
teardown included in the test. Other global properties and variables
should be reset in the test teardown. The test stack is deleted from
memory and reloaded for the next test if multiple tests are being run
within the same process as in the standalone test runner.

Crashes or uncaught errors from a test command cause the test to immediately fail.

### LiveCode Builder

LCB tests live in the `tests/lcb` directory and its subdirectories.  There are currently two groups of tests:

* `tests/lcb/stdlib` contains tests that check that syntax and handlers in the LCB standard library work correctly.  Each of the `.lcb` files in that directory is named the same as the standard library that it tests.  For example, the `com.livecode.list` library is tested by `list.lcb`.
* `tests/lcb/vm` contains tests for the LCB bytecode interpreter and virtual machine works correctly.  Each of the `.lcb` files is named according to the VM feature that it tests.  For example, `dynamic-call.lcb` tests passing LCB handlers as callable handler objects.

Just like for the LCS tests described above, new `.lcb` files added to the test suite get detected, compiled and run automatically.

Each test module contains a set of `public handler` definitions, with names beginning with `Test`.  Each test command gets run in a fresh LiveCode Builder environment.

The LCB standard library has built-in syntax for writing unit tests, provided by the `com.livecode.unittest` module.  For more information and example code, look up `com.livecode.unittest` in the LiveCode Builder dictionary.

### LiveCode Builder Compiler Frontend Tests

LCB compiler frontend tests live in the 'tests/lcb/compiler/frontend' directory and its subdirectories. Compiler test files all have the extension '.compilertest'.

Unlike LCB and LCS tests, the frontend compiler tests consist of fragments of LCB code which are fed to the compiler to check that it either succeeds, or emits the correct warnings or errors.

The compilertest files use directives beginning with '%' to describe how the different LCB code fragments should behave when passed through the compiler. A single compilertest file can contain as many code fragments to check as necessary. The syntax is as follows:

    CompilerTest
       : { Line, NEWLINE }

    Line
       : WHITESPACE+
       | '%%' ANYTHING_BUT_NEWLINE
       | Test

    Test
       : '%TEST' <Name: Identifier> NEWLINE
             { Code, NEWLINE }
         '%EXPECT' ('PASS' | 'FAIL' | 'SKIP') [ <Reason: String> ] NEWLINE
             { Assertion, NEWLINE }
         '%ENDTEST'

    Code
       : { ANYTHING_BUT_NEWLINE | '%{' <Position: Identifier> '}' }

    Assertion
       : '%ERROR' <Message: String> 'AT' <Position: Identifier>
       | '%WARNING' <Message: String> 'AT' <Position: Identifier>
       | '%SUCCESS'

Each %TEST clause indicates a separate test. Within the code fragments the '%{...}' clauses indicate named positions within the code which are used to check the position information provided for an error or a warning.

At least one assertion must be present, and you cannot have an %ERROR and %SUCCESS assertion present in the same test.

For each test present in the compilertest file, the code fragment is extracted, the positions of the '%{...}' references are noted and then references are removed. The resulting code is passed to lc-compile and its stderr output evaluated. The output is checked to ensure that each claimed assertion exists, and is in the correct position. When checking for assertion matches, the type (error or warning) must match, the position must match and the asserted string must be within the output message the compiler generates.

For example, to check whether the scope of variables within 'repeat forever' statements blocks is correct, you might use:

    %TEST RepeatForeverScope
    module compiler_test
    handler TestHandler()
      variable tOuterVariable
      repeat forever
        variable tInnerVariable
      end repeat
      put %{BEFORE_BADINNERVARIABLE}tInnerVariable into tOuterVariable
    end handler
    end module
    %EXPECT PASS
    %ERROR "Identifier 'tInnerVariable' not declared" AT BEFORE_BADINNERVARIABLE
    %ENDTEST

When compiled, lc-compile will emit an error on the 'put' line because tInnerVariable is not declared at that point. This matches the specified '%ERROR' assertion and so the test will pass (i.e. the compiler is correctly identifying the fact that tInnerVariable is not declared outside of the scope of the 'repeat forever' construct).

To help debug compiler tests, set the LCC_VERBOSE environment variable to 1 before running the compiler test. This will cause the compiler testrunner to emit diagnostic information, including the full output of the compile command which is being run.

### LiveCode Script Parser Tests

The syntax for LiveCode Script parser tests is the same as that of the 
LCB compiler frontent tests above. LCS parser test files all have the 
extension '.parsertest'.

Expected errors are referred to by their name in the parse errors 
enumeration. For example the following tests the variable shadowing 
parse error "local: name shadows another variable or constant":

	%TEST ShadowVariable
	local sVar
	local %{BEFORE_SHADOW}sVar
	%EXPECT PASS
	%ERROR PE_LOCAL_SHADOW AT BEFORE_SHADOW
	%ENDTEST

The directive `%SET` can be used to specify the value of global properties
used when running the test. In particular, it can be used to set the 
value of the `explicitvariables` property. If the `explicitvariables` 
property is not set then the test will be run with it set to `true` and 
to `false`, and the test will fail if the result differs. For example:

	%TEST CommentedContinuation
	on compiler_test
	-- comment \
 	with%{SYNTAX} continuation character
	end compiler_test
	%EXPECT PASS
	%ERROR PE_EXPRESSION_NOTLITERAL AT SYNTAX
	%ENDTEST

will fail with "error: test ambiguity with explicit vars".

	%TEST CommentedContinuationExplicit
	%SET explicitvariables true
	on compiler_test
	-- comment \
	with %{SYNTAX}continuation character
	end compiler_test
	%EXPECT PASS
	%ERROR PE_EXPRESSION_NOTLITERAL AT SYNTAX
	%ENDTEST

will succeed.

### C++ tests with Google Test

In general, C++ tests should only be used for things that cannot be tested any other way.

Each test is a `.cpp` file added to the `test` directory for the program or library to be tested.  At the moment, the C++ test sets are available for **libcpptest**, **libfoundation** and **engine**.

When you add a new C++ test source file, you need to add it to the target's corresponding `module_test_sources` gyp variable.  These are currently set in the top-level `.gyp` file for each project, except for the engine, for which you should edit the `engine_test_source_files` variable in `engine/engine-sources.gypi`.

For more information on writing C++ tests with Google Test, please consult the [Google Test documentation](https://github.com/google/googletest/blob/master/googletest/docs/Documentation.md).

## Other tests

The lc-run program has some basic smoke tests for its command-line
interface and start-up process.  You can find them in
`tests/_lcruntests.livecodescript`, with support files in
`tests/lc-run/`.  lc-run is used to run the LiveCode Builder tests
mentioned above; only add to the smoke tests for things that *can't*
be tested by writing a "normal" LiveCode Builder test.

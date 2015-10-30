# How to test LiveCode

Tests are small programs that check that a particular, specific function works correctly.  They are run automatically to check whether LiveCode works properly.  They're really useful for ensuring that changes to one part of LiveCode don't break other things!

The main LiveCode engine repository contains three sets of tests ("test suites"):

* **LiveCode Script tests:**  script-only stacks that are run using the LiveCode standalone engine.  They test features of the LiveCode Script language.
* **LiveCode Builder tests:** LCB modules that are run using the **lc-run** tool.   They test features of the LCB core language and standard library.
* **C++ tests:** low-level tests written in C++ using [Google Test](https://github.com/google/googletest).  These perform low-level checks for things that can't be tested any other way.

## Running the Tests

This assumes that you've already got the LiveCode source code and that you've successfully compiled it.  See the [installation instructions](../README.md) for more details.

### Running tests on Mac OS X and Linux

From the top directory of the livecode git repository working tree, run `make check`.  This will run all the test suites.

### Running tests on Windows

Open the `livecode.sln` solution file in Visual Studio, and build the "check" project.  This will run the C++-based tests.

There's not currently a convenient way to run the LiveCode Script and LiveCode Builder tests on Windows.

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
* `TestAssertBroken pDescription, pExpectTrue, pReasonBroken`: The same as `TestAssert`, but marking the test as "expected to fail".  *pReasonBroken* should be a short explanation of why the test is currently expected to fail; it should almost always be a reference to a bug report, e.g. "bug 54321".
* `TestGetEngineRepositoryPath`: A function that returns the path to the main LiveCode engine repository.
* `TestGetIDERepositoryPath`: A function that returns the path to the LiveCode IDE repository.

Tests can have additional setup requirements before running, for example loading custom libraries. If the script test contains a handler called `TestSetup`, this will be run prior to running each test command. For example:
````
on TestSetup
   -- All the tests in this script require access to the docs parser
   start using stack (TestGetEngineRepositoryPath() & slash & "ide-support" & slash & "revdocsparser.livecodescript")
end TestSetup
````

Tests may need to clean up temporary files or other resources after running.  If a script test contains a handler called `TestTeardown`, this will be run after running each test command -- even if the test failed.  N.b. `TestTeardown` won't be run if running the test command causes an engine crash.

Crashes or uncaught errors from a test command cause the test to immediately fail.

### LiveCode Builder

LCB tests live in the `tests/lcb` directory and its subdirectories.  There are currently two groups of tests:

* `tests/lcb/stdlib` contains tests that check that syntax and handlers in the LCB standard library work correctly.  Each of the `.lcb` files in that directory is named the same as the standard library that it tests.  For example, the `com.livecode.list` library is tested by `list.lcb`.
* `tests/lcb/vm` contains tests for the LCB bytecode interpreter and virtual machine works correctly.  Each of the `.lcb` files is named according to the VM feature that it tests.  For example, `dynamic-call.lcb` tests passing LCB handlers as callable handler objects.

Just like for the LCS tests described above, new `.lcb` files added to the test suite get detected, compiled and run automatically.

Each test module contains a set of `public handler` definitions, with names beginning with `Test`.  Each test command gets run in a fresh LiveCode Builder environment.

The LCB standard library has built-in syntax for writing unit tests, provided by the `com.livecode.unittest` module.  For more information and example code, look up `com.livecode.unittest` in the LiveCode Builder dictionary.

### C++ tests with Google Test

In general, C++ tests should only be used for things that cannot be tested any other way.

Each test is a `.cpp` file added to the `test` directory for the program or library to be tested.  At the moment, the C++ test sets are available for **libcpptest**, **libfoundation** and **engine**.

When you add a new C++ test source file, you need to add it to the target's corresponding `module_test_sources` gyp variable.  These are currently set in the top-level `.gyp` file for each project, except for the engine, for which you should edit the `engine_test_source_files` variable in `engine/engine-sources.gypi`.

For more information on writing C++ tests with Google Test, please consult the [Google Test documentation](https://github.com/google/googletest/blob/master/googletest/docs/Documentation.md).

# Contributing to LiveCode

![LiveCode Community Logo](http://livecode.com/wp-content/uploads/2015/02/livecode-logo.png)

Copyright © 2003-2016 LiveCode Ltd., Edinburgh, UK

See also the [documentation contributions guide](docs/contributing_to_docs.md).

## Contributors' forums

General discussion about contributing to the LiveCode Community open-source projects takes place on the [LiveCode Open Source forums](http://forums.livecode.com/viewforum.php?f=65), and in particular the [Engine Contributors forum](http://forums.livecode.com/viewforum.php?f=66).

## Contributor's License Agreement (CLA)

If you wish to contribute to development of LiveCode, you must sign the [Contributor's Agreement](http://livecode.com/account/developer/contribute).  This agreement is required because the LiveCode project is dual-licensed both under the GPLv3 and a commercial (closed-source) license; you need to give LiveCode Ltd. permission to use your submissions in this way.

**Note:** LiveCode cannot accept any pull-requests from individuals who have not signed this agreement.

## Using GitHub

The LiveCode workflow is a typical git workflow, where contributors fork the [livecode/livecode](https://github.com/livecode/livecode) repository, make their changes on a branch, and then submit a pull request.

### Setting up git with your user information

Please ensure that your full name and e-mail address are in the git configuration.  Ideally, the e-mail address you use with git should be the same as the e-mail address you used to sign the CLA (i.e. the one used for your LiveCode customer account).

You can set up the name and e-mail address from the command line:

    git config --global user.name "<your name>"
    git config --global user.email "<your email address>"

**Note**: Pull requests which add commits containing incorrect author name and e-mail addresses will be rejected.

### Branches in GitHub

You should base your changes on an appropriate branch:

* The `develop` branch is where work on the next major, experimental release of LiveCode takes place.  It usually has all of the most exciting new features, but also a lot of bugs to go with them.  If you are adding a new feature to LiveCode, submit your changes to this branch.

* The `develop-X.Y` branches work towards the next X.Y.Z release.  They are usually much more stable than the `develop` branch.  If you are fixing a bug in LiveCode, submit your changes to the "oldest" `develop-X.Y` branch that exhibits the bug.

* The `release-X.Y` branches are used as part of the release process.  Unless you are helping to make a release, you should not normally work with these branches.

### Creating a pull request

When you submit a pull request, please make sure to follow the following steps:

1. Ensure that all the commits have good log messages, in the following format:

  ```
  Summary line of less than 80 characters

  Explanation of what the commit fixes and why it's the right
  fix, possibly using multiple paragraphs.  For example, you
  might want to describe other options and why the one you
  chose is better.
  ```

  It's very important that readers can get a good idea of what the commit is about just by reading the summary line.  To help with this, we use some special "tags" at the start of a commit message summary line:

  * If the commit fixes a bug, please add `[[Bug <bug number>]]` at the start.

  * If the commit relates to a particular new feature — and there are several commits and pull requests involved in the feature — please add `[<feature-name>]` at the start.

  * If the commit fixes a [Coverity Scan defect](https://scan.coverity.com/projects/4036), please add `[CID <defect number>]` at the start.

2. Make sure that the pull request only relates to *one* change (one bug fix, one new feature, etc.) or to a group of very closely-related fixes.  Please make sure that the pull request has a good description too (often you can base the title and body of the pull request on the commit messages).

  Please highlight any areas of your changes that you thought were particularly difficult to figure out.  This will help make sure that your code gets thoroughly reviewed.

### Pull request process

After you submit a pull request, a member of the LiveCode team will review your changes.  They will probably find some improvements that need to be made.  Please note that if a reviewer asks you to change your code, it doesn't necessarily mean that there was anything wrong with the changes you've made.  It often means that they have spotted a way to fix other things at the same time, or to make your change fit in better with other things that are being worked on elsewhere.

Once a reviewer is happy with the changes, they will mark the pull request as reviewed.  The LiveCode continuous integration system will then take your code, and automatically build & test it on all of the platforms supported by LiveCode.

If the tests don't pass, then you will need to make some more changes to fix the problems that were found.  These will then be reviewed, etc.

Once your changes have been reviewed and tested, they will be merged in time for the next release.

## Bugs

Finding and fixing bugs in LiveCode is a particularly valuable contribution.

If you've found a bug, please add a ticket to the [LiveCode issue tracking system](http://quality.livecode.com/).  This will give you a bug number which can be used whenever discussing the issue, and included in git commit log messages and in GitHub pull request descriptions.  This will help other contributors keep track of who's working on what.

When you submit a pull request that fixes a bug, the status of the bug should be set to "AWAITING_MERGE" -- please also add a comment to the bug with a link to the pull request's page.

When the pull request is merged, the status should be set to "AWAITING_BUILD".

## Coding style

### C++ coding style

The majority of the engine is written in C++, but the style treats C++ as an improved version of C, and doesn't use the whole of the language's feature set.  In particular:

* The engine doesn't use exceptions or RTTI (run time type information).
* Templates are used sparingly, and typically as "sanitized macros" for efficiency purposes or for resource management.
* No standard C++ library is used (i.e. you cannot use anything in the `std` namespace)

#### Naming convention

The engine source code follows a standard naming convention.

Variable and function names should be descriptive.  Don't be scared of verbosity (but don't go too overboard on symbol lengths)


Variable names:
* should be lowercase, with words separated by underscores ("\_")
* should be prefixed to indicate scope:
  * "t\_": local variables
  * "p\_": in parameters
  * "r\_": out parameters
  * "x\_": in-out parameters
  * "m\_": class or struct instance member variables
  * "s\_": class, function, or file-local static variables
  * "g\_": global variables
  * example: `t_foo_bar`

Constants' names (including enum members):
* should be camel-case
* should prefixed with `kMC` and the module name
* example: `kMCModuleFooBar`

Function names:
* should generally be camel-case
* public / exported functions should have names prefixed with "MC" followed by the module name.
* file-local static functions may also be in lower-case and underscore-separated
* example: `MCFooBar()`

#### Coding practices

* Declare and initialize local variables on separate lines
* Initialize all variables to a base value, e.g.
  * pointers to `nil`
  * bools to `true` or `false`
  * numbers to 0
* Only pass `bool` values to conditions (e.g. `if`, `while`, etc.). Don't assume that `nil`/`null`/`0` are false.
* Always check the success of memory allocations; if the calling code can't handle memory failure then prefix the line that allocates with `/* UNCHECKED */`
* Avoid using preprocessor macros to abbreviate code; use inline and/or template functions instead
* Use implicit resource management (e.g. `MCAutoStringRef`) wherever reasonably possible
* Functions should not modify "out" function parameters until immediately before returning, and only on success
* `goto` is only usually acceptable for implementing cleanup-on-error
* Avoid using the ternary operator (`<condition> ? <expr-if-true> : <expr-if-false>`)
* Use a bit-field when declaring boolean values in a struct or class (e.g. `bool m_bool_member : 1`)
* Whenever adding a function, add a comment that explains precisely:
  * What the function does, under what conditions it succeeds, and how it behaves when it fails
  * What the function expects as inputs, and what outputs it generates

#### Layout and style

* Indent with tabs, using a 4-space tab width.
* All curly braces should be on a line on their own.  They should be indented to match the level of the construct they relate to.  For example:

  ````
  if (/* <condition> */)
  {
    /* <body> */
  }
  ````
* Use a single space after the `for`, `while`, `if`, and `switch` keywords, as in the example above.
* Don't insert a space between a function or macro name and its parameter list
* Insert a single space after each comma (",")
* Insert a single space before and after binary operators (e.g. `x == y`)
* Put a single statement on each line
* Split overly-long lines (> 80 characters) appropriately.  Try to place a line-break after any binary operators or commas.
* Use a single blank line to separate different areas of code with in a function.
* Use a single blank line between function and type definitions
* Separate significant areas of code with comment bars, e.g. a line containing only 80 slash characters ("/")

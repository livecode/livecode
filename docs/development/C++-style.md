# C++ Coding Guidelines for LiveCode

This file describes some key C++ coding style guidelines for
contribution to the LiveCode source code.  In the past, C++ was used
as an improved version of C, and didn't use the whole of the
language's feature set.  Since the introduction of C++11 support to
the LiveCode build toolchain, some more advanced features of C++ are
recommended to be used when developing LiveCode.

See also [C++ Feature Usage Guidelines for LiveCode](C++-features.md).

## Naming things

* Variable and function names should be descriptive.  Don't be scared
  of verbosity (but don't go too overboard on symbol lengths).

* Variable names must be lowercase, with words separated by
  underscores (`_`).  They should be prefixed to indicate scope:

  * `t_`: local variables
  * `p_`: in parameters
  * `r_`: out parameters
  * `x_`: in-out parameters
  * `m_`: `class` or `struct` instance member variables
  * `s_`: `class`, function, or file-local static variables
  * `g_`: global variables

  Good: `static RegExp s_regexp_cache[10];`

  Bad: `int x, y, w, h;`

* Constant names, including `enum` members, should be title case and
  prefixed with `kMC` and the module name.

  Good:

        const MCByteOrder kMCByteOrderHost = kMCByteOrderBigEndian;`

  Bad:

        const int MAX_LENGTH = 20;

* Function names should generally be title case.  Public (exported)
  functions should have names prefixed with `MC` and the module name.

  Good:

        hash_t MCHashPointer(void *p_pointer);

## Comments

* Whenever adding a function, add a comment that explains precisely:

  * what the function does, under what conditions it succeeds, and
  how it behaves when it fails
  * what the function expects as inputs, and what outputs it generates

## Coding practice

### Type definitions

* Types declared with `struct` must be Plain Old Data (POD).

* All composite types should have a default constructor.

* All constructors should initialise all fields, preferably in an
  initialiser list rather than in the constructor body.

* Use a bit field when declaring boolean values in a `struct` or
  `class`:

        bool m_bool_member : 1

* Avoid including minimum, maximum or default members in an `enum`.
  Consider:

  * declaring typed constants alongside the enumeration
  * overloading `std::begin()` and `std::end()` for the enumeration

### Lifetime handling

* Use "managed lifetime" types wherever possible.  If
  `std::unique_ptr` or `std::shared_ptr` are not suitable, use the
  `MCAuto` types from `foundation-auto.h`.

  Good (automatic cleanup):

        MCAutoStringRef t_unicode;
        if (!MCStringUnicodeCopy(p_input, &t_unicode));
            return false;
        return MCDoSomethingWithUnicode(*t_unicode);

  Bad (explicit cleanup):

        MCStringRef t_unicode;
        if (!MCStringUnicodeCopy(p_input, t_unicode))
            return false;
        auto t_result = MCDoSomethingWithUnicode(*t_unicode);
        MCValueRelease(t_unicode);
        return t_result;

* When class instances "own" instances of other types, use
  "managed lifetime" types as the class members.

* As a rule, avoid writing code that requires explicit `delete`,
  `delete[]`, `free()`, `MCValueRelease()`, `MCMemoryDeallocate()`,
  etc.  If possible, also avoid explicit allocation

* Always check the success of memory allocations.  If the calling code
  can't handle memory allocation failure, prefix the allocating
  statement with `/*UNCHECKED*/`.

* When using the [`new` operator](http://en.cppreference.com/w/cpp/memory/new/operator_new),
  always either use non-allocating placement allocation ("placement new"), or
  specify [`std::nothrow`](http://en.cppreference.com/w/cpp/memory/new/nothrow):

        int *t_array = new (nothrow) int[10];

### Local variables

* Always initialise local variables at the point of declaration.

  * Initialise POD types using `=`:

        bool t_success = true;

  * Initialise non-POD types using construction syntax:

        MCSpan<byte_t>(p_pointer, p_length);

  * In the absence of another suitable value, initialise:
    * pointers to `nullptr`
    * `bool` values to `true` or `false`
    * numbers to `0`

* Use the `auto` type specifier when:

  1) the full type is irrelevant
  2) the full type is explicitly stated on the line
  3) writing templated code (e.g. implementing `min()`/`max()`)

### Control flow

* Functions must not modify "out" parameters until immediately before
  returning, and only on success.

* `goto` is usually only acceptable for cleaning up on errors; try to
  use managed lifetime types instead.

* When disabling code with an always-false condition, prefix the
  condition with `/* DISABLES CODE */`:

        if (/* DISABLES CODE */ (false))
        {
            /* statements */
        }

### Miscellaneous

* Declare all compile time constants as fully `const`.  This is
  particularly important for data or lookup tables.

* Only pass `bool` values to conditions (e.g. `if`, `while`, etc.)  Do
  not assume that `nullptr` or `0` are false.

* Never use C-style casts. Use:

  1) Construction, if possible
  2) `reinterpret_cast` for bit-casts (e.g. converting a pointer to an
     integer or _vice versa_).
  3) `dynamic_cast` to cast a `class` to or from a derived `class`.
  4) `static_cast` for other types of type cast

  Avoid using `const_cast`.

* Never pass a pointer and length as separate arguments to a
  function.  Use an `MCSpan` instead.

* Avoid using preprocessor macros to abbreviate code; use `inline`
  functions or templates instead.

* Avoid using the ternary `/*condition*/ ? /*if_true*/ : /*if_false*/`
  operator.  You may need to use one when initialising a `const`
  value.  However, when the condition or the true/false values require
  large expressions, or when there are more than two possible values,
  an immediately-invoked function expression can do the job more
  clearly:

      const auto&& t_value = [&, this]() {
          if (/*condition*/)
          {
              return /* compute value if true */;
          }
          else
          {
              return /* compute value if false */;
          })();

* When writing lambda expressions, try to avoid return type
  specification; use inference or, if necessary, cast or construct a
  value in the `return` statement.

      auto t_valid_handle =
          [](const ObjectHandle& p_obj) { return bool{p_obj}; }

## Code layout, indentation and whitespace

* Use spaces for indentation and alignment, with 4 spaces per level of
  indentation.

* Avoid lines over 80 characters in length.  When wrapping
  expressions, across multiple lines, try to place a line break after
  a binary logical operator (`&&`, `||`) or after a comma.

* Use only one statement per line.

* Use single blank lines to separate areas of code within a function,
  and to separate function and type definitions.

* All curly braces should be on a line on their own, indented to match
  the level of the construct they relate to.

  Good:

        if (/* condition */)
        {
            /* statements */
        }

  Bad:

        if (/* condition */) {
            /* statements */
        }

* Use a single space after the `for`, `while`, `if` and `switch`
  keywords, as in the example above.

* Do not insert a space between a function or macro name and its
  parameter list.

  Good:

        void main(int argc, char**argv);

  Bad:

        void main (int argc, char **argv);

* Place a space or newline after each comma, as in the example above.

* Place a space before and after binary operator.

  Good:

        bool t_okay = (t_left == t_right);

  Bad:

        double t_area = 2*acos(0.0)*t_radius*t_radius;

* When using preprocessor macros, ensure that the `#define`d identifier 
  has a value.

  Good:

        #define FEATURE_PLATFORM_PLAYER (1)

  Bad:

        #define FEATURE_PLATFORM_PLAYER

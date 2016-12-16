# C++ Feature Guidelines for LiveCode

This file documents the usage of C++ language and library features within the
LiveCode engine and ancillary projects (externals, etc). It covers technical
aspects rather than matters that are purely of coding style (for that, see
CONTRIBUTING.md instead).

The LiveCode codebase dates back to the early 1990s as MetaCard and was
originally written in C. Over its lifetime, it supported a large number of Unix 
and Unix-like operating systems (e.g. SunOS, IRIX, AIX) as well as Windows (both
16-bit Windows 3.1 as well as 32-bit Windows 95+) and "classic" (pre-OSX) MacOS
in addition to the currently supported platforms (modern versions of Windows,
OSX, Linux, Android and iOS as well as browser-based HTML5+JavaScript). Because
of this wide variety of operating environments, both past and present, the
adoption of C++ language features (as well as library facilities) has been very
conservative in order to compile and run on the numerous supported platforms.

The following is a list of major C++ language features (by C++ standard
revision) and whether they are permitted in the LiveCode codebase or not. If a
feature is prohibited, a rationale is provided in order that the reason for
avoiding the feature is understood. The subsequent section details which
compilers LiveCode is built with and which C++ language features those
compilers provide.

## Core language features

### C++98 and C++03

C++98 is the first standard governing the C++ language and C++03 is a set of 
minor revisions to correct defects in the original standard. All of the
compilers used in the LiveCode build process support these standards. Despite
this, some features from these revisions are prohibited but for reasons other
than compiler support.

Generally speaking, any features of the language or library that require runtime
support should not be used for reasons of compatibility. More details are given
in the descriptions of the individual features. (Because most features are
permitted, only prohibitions or features that are permitted with caveats are
listed here). Anything that also requires a C++ ABI with system libraries should
also be avoided.

#### Exceptions: PARTIALLY PROHIBITED

The only exception that should normally be thrown or caught within
LiveCode source code is `std::bad_alloc` for memory allocation
failures.

Catch exceptions thrown by third party C++ libraries used by LiveCode
and translate them to LiveCode's internal error handling mechanisms.

**Rationale:**

> Correctness.
>
> The age of the codebase (as well as its C origins) mean that the vast majority
> of the code in the engine is not exception-safe (i.e it will leak resources
> and/or fail to clean up properly as the exception propagates). Because writing
> good exception-safe code is hard enough already, use the engine's existing
> error handling mechanisms wherever possible.

**Timescale:**
> Indefinite. Making the engine exception-safe is a huge undertaking and is not
> currently being contemplated. Even if it were done, exceptions can be tricky
> to use correctly so any use would need to be carefully discussed and
> considered before being accepted.

#### `dynamic_cast` and RTTI (`typeid`, `typeinfo`): PARTIALLY PROHIBITED

Use `dynamic_cast` when casting within a type hierarchy.  This ensures
early error detection when an instance is cast to an incompatible
type.

Other uses of RTTI are currently prohibited.

**Rationale:**
> Correctness, style and ABI dependencies.
>
> There are few good uses of RTTI features that wouldn't be better done using
> virtual methods or other mechanisms. The performance consequences of using
> `dynamic_cast` vary between platforms though the penalties are not as large as
> they have been on some historical systems.
>
> In addition to the "correctness" of using these features, RTTI (including
> `dynamic_cast`) introduces C++ ABI dependencies on the layout of class vtables
> and typeinfo structures.

**Timescale:**
> Indefinite. Improved C++ implementations are gradually reducing the
> cost of using RTTI features, and newer C++ standards are introducing
> library features that make use of RTTI. It may make sense to use
> RTTI features beyond `dynamic_cast` in the future.

#### C++ standard template library: PROHIBITED

**Note:**
> This is the entirety of the C++ standard library in C++98/03 (i.e everything
> in the `std` namespace. This explicitly excludes global `operator new` and
> cousins (`new[]`, `delete`, `delete[]`) as they are in the global namespace,
> not `std`.
>
> There are some features in the C++ standard library in more modern language
> revisions that *are* permitted, but these are documented separately in a
> section on the C++ standard library.

**Rationale:**
> Primarily C++ ABI issues but also performance characteristics.
>
> Many parts of the STL pull in symbols that are provided in a system library
> rather than being inlined in templates. This introduces a large, complex C++
> ABI dependency which would vastly complicate the LiveCode build and testing
> process.
>
> Additionally, the performance characteristics of the various STL containers
> were not specified until C++11 or newer so cannot be relied upon.

### C++11

C++11 (previously named C++0x then C++1x) was the first major revision to the
C++ standard. It introduced a large number of new features, both as part of the
core language and in the standard library.

The limitations in this section generally stem from compiler support issues:
only those features implemented by all compilers used in the LiveCode build
process may be used outside platform-specific code. (Use within platform-
specific code of a feature supported by all compilers for that platform is
permissible but requires careful consideration before use).

If a feature in this list is not supported but will be permitted once supported,
no special annotation is used. If it would be prohibited even if it were
supported, this is specifically called out and a rationale for the prohibition
is supplied.

#### RValue references: PERMITTED

References with move semantics (rather than copy semantics). Named for their
position on the right-hand side of an assignment expression, e.g.
`FooClass foo = getFoo();` calls `foo.FooClass::operator=(foo&&)` with the
return value from `Foo getFoo()`.

#### Reference qualifiers on member functions: NOT SUPPORTED

Overloading member functions based on reference type (lvalue vs rvalue):
`Foo getFoo() const &; Foo getFoo() &&;` will call the former to copy the Foo in
most cases but the latter to move the Foo in rvalue context. This is used
alongside rvalue references to permit copy-vs-move optimisations of certain
common patterns.

**Requires:**
> MSVC: 2015
> GCC: 4.8.1
> Clang: 2.9

#### Non-static data initialisers: NOT SUPPORTED

Avoids repetition of information when defining classes with multiple
constructors. For example:
````
class Foo
{
    int m_Bar = 42;
    ...
};
````
will initialise `m_Bar` to 42 in any constructor that doesn't explicitly
initialise that member to something else.

**Requires:**
> MSVC: 2013
> GCC: 4.7
> Clang: 3.0

#### Variadic templates: NOT SUPPORTED

Allows the definition of templates with a variable number of template
parameters. For example:
````
template <class Functor, class... Args>
auto CallFunctionWithArgs(Functor f, Args... args)
{
    return f(args...);
}
````

**Requires:**
> MSVC: 2013
> GCC: 4.3
> Clang: 2.9

#### Initialiser lists: NOT SUPPORTED

Used when initialising an aggregate (struct, class) with multiple elements of
same type. Usually used for initialising arrays with multiple elements:
````
std::vector<int> t_array = { 1, 1, 2, 3, 5, 8, 13, 21, 34, 55 };
````

**Requires:**
> MSVC: 2013
> GCC: 4.4
> Clang: 3.1

#### Static assertions: PERMITTED

Used to test compile-time conditions, e.g.:
````
static_assert(sizeof(int) == 4, "Expected int to be 4 bytes");
````

#### `auto` type specifier: PERMITTED

Allows inference of types for variables, leading to shorter/less redundant code.
Especially useful when writing templates where the compiler can deduce a type
but the author cannot, e.g.:
````
template <class T>
void callBarThenQuux(T& t)
{
    auto t_bar = t.Bar();
    t.quux(t_bar);
}
````

This can also be used when writing functions:
````
template <class T>
auto callFooWith42(T& t)
{
	return t.Foo(42);
}
````

#### Trailing return types: PERMITTED

This is an extension of the `auto` type specifier that allows an explicit (but
deduced) type to be given. Similarly, most useful when writing templates:
````
template <class T, class U>
auto addThings(T t, U u) -> decltype(t + u)
{
    return t + u;
}
````
This notation says that the return type is the type that is the return type of
the expression `(t + u)`.

#### Lambda expressions: PERMITTED

Lambda expressions are a convenient way to define functions inline and come in
two forms: capturing and non-capturing. As an example, consider a method `apply`
that applies a given function to each element of an array. One use of a non-
capturing lambda would be:
````
Array t_array = ...;
auto increment = [](array_element_t& element) { element++; };
t_array.apply(increment);
````
while a capturing lambda might be used to do:
````
Array t_array = ...;
int sum = 0;
auto sum = [&sum](array_element_t& element) { sum += element; };
t_array.apply(sum);
````

#### `decltype` expression: PERMITTED

The `decltype` keyword returns the type of an expression. This could be the name
of a variable (e.g. `decltype(t_foo)`) or a more complex expression (e.g.
`decltype(t_foo.Bar(t_baz) + t_quux)`). It can be used anywhere a type can be
used:
````
int t_foo = 42;
long t_bar = 0;
decltype(t_foo + t_bar) t_baz = t_foo + t_bar;
````

#### `>>` inside template parameter lists: PERMITTED

In previous versions of C++, instantiating a template with a type that is also
a template (e.g. `Foo<Bar<int>>`) would cause the final `>>` to be interpreted
as a right-shift operator rather than terminating the template parameters. As of
C++11, preference is now given to the template meaning. Consequently, all uses
of the right-shift operator in a template parameter list should be parenthesised
in order to get the operator meaning (`Quux<(42 >> 3)>`).

#### Default template arguments for template functions: NOT SUPPORTED

Previously, template functions could not have default values for the template's
arguments; this restriction has been lifted in C++11:
````
template <class T, class U = T>
U addOne(T t)
{
    return U(t) + 1;
}
````

**Requires:**
> MSVC: 2013
> GCC: 4.3
> Clang: 2.9

#### SFINAE in expressions: NOT SUPPORTED

C++ makes available a technique called "SFINAE" (Substitution Failure Is Not An
Error) whereby errors encountered when substituting template parameters are not
considered errors. This has been extended to certain expressions, too (in
particular, `sizeof`, `decltype` and `noexcept` expressions). By way of example:
````
template <class T>
auto addIfPossible(T t1, T t2) -> decltype(t1 + t2)
{
    return t1 + t2;
}

template <class T>
T addIfPossible(T t1, ...)
{
    return t1;
}
````
will, when used as `addIfPossible(Foo(), Foo())`, call the first form when `Foo`
has an appropriate `operator+` defined and the second form otherwise (as it is
the only overload that is valid).

**Requires:**
> MSVC: (Not yet implemented)
> GCC: 4.4
> Clang: 2.9

#### Alias templates: NOT SUPPORTED

There is no way in C++ to make a template typedef so C++11 introduces a template
using expression, which has the same effect:
````
template <class T>
using Foo = Bar<T, Quux, 42>;
````
introduces the template `Foo<x>` as an alias for `Bar<x, Quux, 42>`. This can
also be used in place of a normal typedef (i.e without the template): 
`using uint = unsigned int;`

**Requires:**
> MSVC: 2013
> GCC: 4.7
> Clang: 3.0

#### Extern templates: PERMITTED

C++ has always provided a syntax to explicitly instantiate a template for a
given set of template parameters (`template Foo<X, Y, Z>;`) but didn't provide
the complement: a way to suppress a given instantiation in the current
compilation unit (usually because it is explicitly instantiated elsewhere).
Extern template declarations are the new mechanism to do this:
````
extern template Foo<X, Y, Z>;
````

#### `nullptr` constant: PERMITTED

The `NULL` macro in C++ poses a number of problems. Because of the stricter type
checking, it cannot be defined as `((void*)0)` as is recommended in C so is
instead usually defined as `0`. Unfortunately, `0` is an integer which can cause
unexpected behaviour. The resolution to this is to introduce the `nullptr`
constant, which has pointer type:
````
void foo(int);
void foo(void*);

void foo(NULL);    // Calls the int overload
void foo(nullptr); // Calls the void* overload
````

There is also a corresponding type made available in the standard library
(`nullptr_t`) which is the type of this constant (it is a different type from
any other pointer type). This type can also be accessed as `decltype(nullptr)`.

#### Strongly-typed enums: PERMITTED

In C++98, the underlying type of an `enum` is determined by the members of the
enum and is the smallest type that can represent all member values. This means
that it is not possible to forward-declare enums as the underlying type cannot
be known until the enum is fully defined. C++11 adds a way of declaring the
underlying type of an enum, both for explicitness and to allow forward
declarations:
````
enum Foo : uint32_t;

enum Bar : int8_t { ... };
````

Another pitfall of enums is that the names of the members are injected into the
outer scope and can conflict with other symbols in that scope. C++11 adds a
mechanism to prevent this (as well as various other generally undesirable
features of enums like the existence of arithmetic operators on the enum):
````
enum class Quux { Wibble = 42 };

Quux q = Quux::Wibble;
````
(if the arithmetic operators are desired, they can be manually defined as, e.g.
`operator+(Quux, Quux)` in the scope enclosing the enum).

Both features can be combined: `enum class Quux : uint16_t {...};`.

**Note:**
> MSVC versions before 2012 don't support the `enum class` syntax nor the
> forward declaration of enums but do support the other enum-related
> enhancements.

#### Attributes: NOT SUPPORTED

C++ compilers have long supported various syntaxes for applying attributes to
declarations but the mechanisms have been non-standard. MSVC uses `declspec(x)`
while GCC and Clang use `__attribute__((x))`. This has now been standardised as
`[[x]]` (e.g. `[[warn_unused_result]] bool operationThatMightFail(Foo&);`).

Particular attributes are discussed individually. Except for those defined by a
C++ standard, attributes should be protected by an appropriate check:
````
#ifdef __GNUC__
#  define WARN_UNUSED_RESULT [[gnu::warn_unused_result]]
#else
#  define WARN_UNUSED RESULT
#endif
````

**Requires:**
> MSVC: 2015
> GCC: 4.8
> Clang: 3.3

#### `constexpr`: NOT SUPPORTED

Certain contexts in C++ require constant expressions (e.g. the size of an array)
but there is no way to use function calls in those expressions in C++98. C++11
adds a keyword that marks a function as being evaluatable at compile time:
````
constexpr size_t doubleSize(size_t x)
{
    return 2*x;
}

int someArray[doubleSize(42)];
````

In C++11, `constexpr` functions must consist of a single return statement
(though the expression within that statement may be of arbitrary complexity and
may even be recursive). C++14 relaxes this restriction (see the C++14 section
for support for this feature).

Generally speaking, `constexpr` should be added to any function that might be
evaluatable at compile time; this would be any function that is a pure function
of its arguments (i.e does not depend on statics, globals, members, etc). It can
also be used for constructors that simply perform simple initialisations of an
object's members using constants or (`constexpr` functions of) the constructor's
parameters.

**Requires:**
> MSVC: 2015
> GCC: 4.8
> Clang: 3.1

#### Explicit alignment: NOT SUPPORTED

C++11 adds the ability to query and set the alignment of types and values using
the `alignof(...)` and `alignas(...)` storage specifiers. For example:
````
using double_bytes = char[sizeof(double)] alignas(alignof(double));
````
(though `alignas(alignof(type))` can be shortened to `alignas(type)`).

**Requires:**
> MSVC: 2015
> GCC: 4.8
> Clang: 3.3

#### Delegating constructors: NOT SUPPORTED

C++11 allows constructors of a class to call other constructors of that class
as part of its initialisation:
````
class Foo
{
    Bar* m_barptr;
    
    Foo() : Foo(nullptr) {}
    Foo(Bar* ptr) : m_barptr(ptr) {}
    Foo(const Foo& foo) : Foo(foo.m_barptr) {}
};
````

**Requires:**
> MSVC: 2013
> GCC: 4.7
> Clang: 3.0

#### Inheriting constructors: NOT SUPPORTED

Allows the import of (all of) a base class' constructors into the scope of a
derived class. For example:
````
template <class T>
class Augmenter : public T
{
	void someAugmentationMethod();
	using T::T;
};
````

**Requires:**
> MSVC: 2015
> GCC: 4.8
> Clang: 3.3

#### `explicit` conversion operators: NOT SUPPORTED

Constructors of classes in C++ have the `explicit` keyword added to prevent them
from being used in implicit conversions. For example, the `std::string` class
has a `string(size_t)` constructor which, if it didn't have an `explicit`
attached, would allow the following:
````
void foo(const std::string&);
foo(42);
````

However, type conversion operators have many of the same problems as conversion
constructors but did not support `explicit` before C++11. This is particularly
evident in classes with an `operator bool`: without `explicit`, objects of the
class can be used in integer expressions as `bool` is an integer type!

As with conversion constructors, conversion operators should always be
`explicit` unless there is a particular reason to leave it off.

**Requires:**
> MSVC: 2013
> GCC: 4.5
> Clang: 3.0

#### `char16_t` and `char32_t` types: NOT SUPPORTED

These types are explicitly defined to be UTF-16 and UTF-32 codeunits,
respectively, but are otherwise similar to `char`.

**Requires:**
> MSVC: 2015
> GCC: 4.4
> Clang: 2.9

#### Unicode string literals: NOT SUPPORTED

These are string literals with types `const char[]`, `const char16_t[]` and
`const char32_t[]` where all elements are the appropriate UTF-8, UTF-16 and
UTF-32 codeunits, respectively:
````
const char* utf8_string = u8"Hello!";
const char16_t* utf16_string = u"Hello!";
const char32_t* utf32_string = U"Hello!";
````

**Requires:**
> MSVC: 2015
> GCC: 4.5
> Clang: 3.0

#### Raw string literals: NOT SUPPORTED

These are string literals with user-defined delimiters in which escape
characters are not interpreted:
````7
// Content of regex is:
//      s/"/'/g
const char* regex = R"some_delim(s/"/'/g)some_delim";
````

**Requires:**
> MSVC: 2015
> GCC: 4.5
> Clang: 3.0

#### Unicode escapes in literals: NOT SUPPORTED

These are escape sequences allowing the specification of Unicode codepoints
within UTF-8, UTF-16 and UTF-32 string literals. There are two forms of escape:
`\uXXXX` for specifying codepoints within the Basic Multilingual Plane (BMP) and
`\UXXXXXXXX` for specifying codepoints outside the BMP.

**Requires:**
> MSVC: 2015
> GCC: 4.5
> Clang: 3.1

#### User-defined literals: NOT SUPPORTED

C++ supports a number of suffixes for literals (for example `3.14f` has type
`float` and `42ULL` has type `unsigned long long`) but these have previously
always been implemented by the compiler. C++11 adds a mechanism for adding
new suffixes:
````
unsigned int operator ""_mysuffix(const char*);
unsigned int foo = "wibble"_mysuffix;
````

**Requires:**
> MSVC: 2015
> GCC: 4.7
> Clang: 3.1

#### Standard-layout types: NOT SUPPORTED

"Plain Old Data" (POD) is a term used in C++ to specify that a type has the
same layout and meaning as an equivalent type in C. This means that it cannot
have virtual methods, data members with different access types (`public`,
`private`, `protected`) or other features that may affect class layout.

In C++98, the definition of POD was very strict and prohibited the type from
having constructors, destructors, assignment operators, base classes, etc. In
C++11, this requirement has been relaxed (as long as the base classes are
themselves POD and the constructors (etc) are "trivial" (i.e do nothing other
than directly assign data members)):
````
// Plain old data in C++11, non-POD in C++98
struct Pod11
{
	int 	m_foo;
	void* 	m_bar;
	
	Pod11() :
	  m_foo(0), m_bar(nullptr) {}
};
````

**Requires:**:
> MSVC: 2012
> GCC: 4.5
> Clang: 3.0

#### `default`ed/`delete`d functions: NOT SUPPORTED

It is often suggested that one should follow the "rule of six" when designing
classes in C++: if any of the default constructor, copy constructor, move
constructor, copy assignment operator, move assignment operator or destructor
are customised for a class, the rest should be too (as failing to do so suggests
an invariant of the class is not being enforced).

Sometimes, however, the default that would have been generated is correct. In
order to prevent repetition, there is a syntax for explicitly requesting that
the default be used:
````
class Foo
{
    Foo() = default;
};
````

On the other hand, sometimes the opposite is desired: a method (or overload)
should explicitly not be provided:
````
class Bar
{
	// No default constructor!
	Bar(const Foo&);
	Bar() = delete;
};
````

**Requires:**:
> MSVC: 2013
> GCC: 4.4
> Clang: 3.0

#### Extended friend declarations: PERMITTED

This change subtly alters the name lookup rules for `friend` declarations,
turning the following invalid C++98 code into valid code in C++11:
````
class C;
namespace Foo
{
    class Bar
    {
        // Only checks the parent (not grandparent or further ancestor) scopes
        // in C++98; checks all ancestor scopes in C++11.
        friend C;
    };
}
````

#### Extended `sizeof`: NOT SUPPORTED

In C++98, `sizeof` cannot be used on non-static data members of a class without
an object. C++11 removes this restriction:
````
class Foo
{
    int m_Bar;
};

Foo foo;
int foobar1 = sizeof(foo.m_Bar);    // Valid in both C++98 and C++11
int foobar2 = sizeof(Foo::m_Bar);   // Only valid in C++11
````

**Requires:**:
> MSVC: 2015
> GCC: 4.4
> Clang: 3.1

#### Inline namespaces: NOT SUPPORTED

This feature allows the entirety of a given namespace to be made visible in
the parent namespace as if they were defined in that namespace. This is subtly
different to using `using namespace Foo` directive: templates, for example, can
only be specialised in the namespace they were defined in; the `using` directive
only makes the symbols visible while the `inline namespace` acts as if it were
just another member of the parent namespace:
````
namespace Foo
{
    namespace Bar
    {
        template <class> class Quux;
    }
    
    // Error: cannot specialise template outside its original namespace
    using namespace Bar1;
    template <> class Quux<int>;
    
    inline namespace Baz
    {
        template <class> class Wibble;
    }
    
    // Okay: Wibble can be specialised inside Foo as namespace Baz is inline
    template <> class Wibble<int>;
}
````

A secondary use is that compilers mangle the names in the inline namespace as
being within that namespace rather than the parent, allowing inline namespaces
to be used as part of ABI versioning:
````
namespace Foo
{
    namespace Bar_v1
    {
        // Visible as and mangled as Foo::Bar_v1::Quux
        class Quux;
    }
    
    inline namespace Bar_v2
    {
        // Visible as Foo::Quux but mangled as Foo::Bar_v2::Quux
        class Quux;
    }
}
````

**Requires:**:
> MSVC: 2015
> GCC: 4.4
> Clang: 2.9

#### Unrestricted unions: NOT SUPPORTED

In C++98, unions cannot contain any types that have non-trivial constructors or
destructors as the compiler cannot automatically generate an appropriate
constructor or destructor for the union. C++11 removes this restriction with the
caveat that the constructor and destructor for the union need to be defined. It
should be noted that it is still tricky to use these types of objects in a union
as there is no way to determine which type the union currently holds:
````
class Foo
{
    ~Foo();
};

class Bar
{
    ~Bar();
};

union Quux
{
    Foo foo;
    Bar bar;
    int baz;
    
    ~Quux()
    {
        // Should this call foo.~Foo(), bar.~Bar() or do nothing?
        // ??????
    }
};
````

**Requires:**:
> MSVC: 2015
> GCC: 4.6
> Clang: 3.1

#### Local types as template arguments: PERMITTED

In C++98, any types used as template arguments must have class or namespace
scope; they couldn't use types defined at function scope. C++11 relaxes this
requirement:
````
template <class T> void frob(T&);

void Foo()
{
    class Wibble {};
    Wibble wobble;
    
    // Invalid in C++98, fine in C++11
    frob<Wibble>(wobble);
}
````

#### Range-based for loop: NOT SUPPORTED

C++11 adds a new way of iterating over the contents of arrays and objects that
define iterators:
````
template <class T>
T sumVector(const std::vector<T>& p_vec)
{
    T t_sum = 0;
    for (const T& t_element : p_vec)
      t_sum += t_element;
    return t_sum;
}
````

This can be used on any objects that either have `begin()` and `end()` member
functions or have `begin(X)` and `end(X)` non-member functions defined. It can
also be combined with automatic type deduction, which makes writing mutators for
generic containers much easier:
````
template <class T>
void incrementContainer(T& p_container)
{
    for (auto& t_element : p_container)
      t_element++;
}
````

**Requires:**:
> MSVC: 2012
> GCC: 4.6
> Clang: 3.0

#### `override` and `final`: PARTIALLY SUPPORTED

When defining virtual methods, it is really easy to accidentally create a new
overload rather than overriding the original method. Using the `override`
keyword indicates that a method declaration is supposed to be an override and
an error should be emitted if it isn't. The `final` keyword is similar, but
prohibits any further overrides in derived classes. `final` may be used either
on individual methods or for whole classes:
````
class Base
{
    virtual void Foo(int);
};

class Derived1 :
  public Base
{
    // Is this supposed to be an override or is it a deliberate overload?
    virtual void Foo(float);
};

class Derived2 :
  public Base
{
    // Error: this declaration doesn't represent an override
    void Foo(float) override;
};

class NoDerivedClasses final :
  public Base
{
    void Foo(int) final;
};
````

**Note:**
> MSVC 2010 supports the `override` keyword but not `final`.

**Requires:**:
> MSVC: 2012
> GCC: 4.7
> Clang: 3.0

#### `noexcept`: NOT SUPPORTED

C++ has the (little-used) ability to add exception specifications to functions:
````
// Can throw std::bad_alloc exceptions
void Foo() throw (std::bad_alloc);

// Can't throw anything
void Bar() throw ();
````

In addition to being tedious to write, they impose a run-time overhead as any
exceptions propagating out of a function with an exception specification need
to be checked against that specification and another exception thrown if an
unexpected exception type was encountered.

C++11 adds a different approach: the `noexcept` keyword. Functions declared as
`noexcept` can be assumed to not throw and optimisations can be made on that
assumption:
````
void Quux() noexcept;
````

The `noexcept` keyword can optionally take a boolean expression that can be
used to disable the keyword if false. It can also be used on an expression to
return true if all function calls in that expression are `noexcept` and false
otherwise. Combined, this is useful for conditionally enabling `noexcept` on
templates:
````
template <class T>
T add(T t1, T t2) noexcept(noexcept(t1 + t2));
````

**Requires:**:
> MSVC: 2015
> GCC: 4.6
> Clang: 3.0

#### Thread-local storage: NOT SUPPORTED, PROHIBITED

Instead of using platform-specific solutions (like the pthreads API for thread-
local storage), C++11 adds a new storage specifier for variables:
`thread_local`. Any variable declared with this value is implemented per-thread
and can be used like any other variable. Note that the specifier only makes
sense on global and static variables as stack-allocated variables are not
visible to other threads anyway.
````
class Foo
{
    // Like static s_bar, but per-thread instead of shared
    thread_local tls_bar;
};
````

**Note:**
> There is not currently a prefix defined for `thread_local` variables in the
> LiveCode engine coding style. Neither `s_` nor `g_` should be used as the
> prefix as it obscures the semantics of the variable. When compiler support is
> available (and a decision to permit their use is made), a prefix will be 
> defined.

**Rationale:**
> Uncertain run-time support.
>
> Although support is available in the appropriate compilers, not all platforms
> are known to provide the requisite run-time support for thread-local storage
> via this mechanism (some iOS versions apparently raise run-time errors when it
> is used but this has not yet been verified). Until support can be confirmed on
> all platforms, the use of `thread_local` as a storage specifier is prohibited,
> even when supported by all compilers.

**Requires:**:
> MSVC: 2015
> GCC: 4.8
> Clang: 3.3

### C++14

This section is under construction.

### C++1z (C++17)

This section is under construction.

## C++ Standard Library

Note that some PROHIBITED headers may be included as an implementation detail of
other PERMITTED headers; use of any symbols thus included remain PROHIBITED.

**TODO:** determine which parts of the library are supplied by which compiler
versions.

### Language support library

#### Types (`<cstddef>`): PERMITTED

#### Implementation properties (`<limits>`, `<climits>`, `<cfloat>`): PERMITTED

#### Integer types (`<cstdint>`): PERMITTED

#### Start and termination (`<cstdlib>`): PERMITTED

#### Dynamic memory management (`<new>`): PERMITTED

#### Type identification (`<typeinfo>`): PROHIBITED

**Rationale:**
> Use of RTTI language features other than `dynamic_cast` is prohibited.
>
> Without the RTTI language features, use of the `<typeinfo>` header
> (particularly for the `std::typeinfo` type) is not useful.

#### Exception handling (`<exception>`): PERMITTED

#### Initialiser lists (`<initializer_list>`): PERMITTED

#### Other runtime support: PARTIALLY PROHIBITED

**Permitted:**
> `<csignal>`, `<cstdalign>`, `<cstdarg>`, `<cstdbool>`, `<cstdlib>`

**Prohibited:**
> `<csetjmp>`, `<ctime>`

**Rationale:**
> Run-time requirements (with exceptions for compatibility).
>
> The `<csetjmp>`, `<ctime>`, `<csignal>` and `<cstdlib>` headers require run-
> time support. Because signals and use of some features in `<cstdlib>` are
> necessary for correct behaviour of the LiveCode engine and/or interfacing with
> external code.

### Diagnostics library

#### Exception classes (`<cstdexcept>`): PERMITTED

#### Assertions (`<cassert>`): PERMITTED

#### Error numbers (`<cerrno>`): PERMITTED

#### System error support (`<system_error>`): PERMITTED

### General utilities library

#### Utility components (`<utility>`): PERMITTED

#### Tuples (`<tuple>`): PERMITTED

#### Fixed-size sequences of bits (`<bitset>`): PERMITTED

#### Memory (`<memory>`, `<cstdlib>`, `<cstring>`): PERMITTED

#### Function objects (`<functional>`): PERMITTED

#### Type traits (`<type_traits>`): PERMITTED

#### Compile-time rational arithmetic (`<ratio>`): PERMITTED

#### Time utilities (`<chrono>`, `<ctime>`): PROHIBITED

**Rationale:**
> Requires run-time support.

#### Scoped allocators (`<scoped_allocator>`): PROHIBITED

**Rationale:**
> Only (currently) used in the STL for container classes.
>
> There's no reason that scoped allocator types couldn't be used in the LiveCode
> engine except that they use the allocator-as-a-type model, which does not
> match the memory allocation patterns used within the LiveCode engine.

#### Type indexes (`<typeindex>`): PROHIBITED

**Rationale:**
> Only useful alongside prohibited RTTI language features.

### Strings library

#### Character traits and string classes (`<string>`): PROHIBITED

**Rationale:**
> Runtime support requirements.
>
> The string classes provided by the STL (including `std::string`) are complex
> and often use large run-time support libraries.

#### Null-terminated sequence utilities (`<cctype>`, `<cwctype>`, `<cstring>`, `<cwchar>`, `<cstdlib>`, `<cuchar>`): PARTIALLY PROHIBITED

**Permitted:**
> `<cstring>`, `<cstdlib>`

**Prohibited:**
> `<cctype>`, `<cwctype>`, `<cwchar>`, `<cuchar>`

**Rationale:**
> Run-time support and an alternative being available.
>
> For character classification, the support functions provided by libICU (via
> libFoundation) should be used instead of through these headers.

### Localisation library

#### Locales (`<locale>`): PROHIBITED

**Rationale:**
> Run-time support requirements.

#### Standard code conversion facets (`<codecvt>`): PROHIBITED

**Rationale:**
> Run-time support requirements.

#### C library locales (`<clocale>`): PROHIBITED

**Rationale:**
> Run-time support requirements.

### Containers library

#### Sequence containers (`<array>`, `<deque>`, `<forward_list>`, `<list>`, `<vector>`): PROHIBITED

#### Associative containers (`<map>`, `<set>`): PROHIBITED

#### Unordered associative containers (`<unordered_map>`, `<unordered_set>`): PROHIBITED

#### Container adaptors (`<queue>`, `<stack>`): PROHIBITED

### Iterators library

#### Iterator traits and adaptors (`<iterator>`): PERMITTED

### Algorithms library

#### Sequence, sorting and related operations (`<algorithm>`): PERMITTED

#### C library algorithms (`<cstdlib>`): PERMITTED

### Numerics library

#### Floating-point environment (`<cfenv>`): PERMITTED

#### Complex numbers (`<complex>`): PERMITTED

#### Random number generation (`<random>`): PROHIBITED

**Rationale:**
> Run-time support requirements.

#### Numeric arrays (`<valarray>`): PROHIBITED

**Rationale:**
> TODO

#### Generalised numeric operations (`<numeric>`): PROHIBITED

**Rationale:**
> TODO

### Input/output library

All prohibited headers in this section are due to run-time support requirements.

#### Forward declarations (`<iosfwd>`): PROHIBITED

#### Standard iostream objects (`<iostream>`): PROHIBITED

#### Iostreams base classes (`<ios>`): PROHIBITED

#### Stream buffers (`<streambuf>`): PROHIBITED

#### Formatting and manipulators (`<istream>`, `<ostream>`, `<iomanip>`): PROHIBITED

#### String streams (`<sstream>`): PROHIBITED

#### File streams (`<fstream>`, `<cstdio>`, `<cinttypes>`): PROHIBITED

### Regular expressions library

#### Regular expressions (`<regex>`): PROHIBITED

**Rationale:**
> Run-time support requirements.

### Atomic operations library

#### Atomic types and operations (`<atomic>`): PERMITTED

### Thread support library

All prohibited headers in this section are due to run-time support requirements.

#### Threads (`<thread>`): PROHIBITED

#### Mutual exclusion (`<mutex>`): PROHIBITED

#### Condition variables (`<condition_variable>`): PROHIBITED

#### Futures (`<future>`): PROHIBITED

## Appendix: supported compiler versions

The LiveCode project supports a number of different operating operating systems
and therefore a wide range of compilers. The set of compilers used by the
LiveCode build servers are currently:

- Windows: MSVC 2010
- Linux: GCC 4.7
- iOS: Clang (version is kept up-to-date with latest non-Beta Xcode release)
- OSX: Clang (version is kept up-to-date with latest non-Beta Xcode release)
- Android: Clang 3.5

Generally speaking, newer versions of compilers can be used to build LiveCode
but older versions cannot (as they do not implement the set of required C++
features used by the engine).

## Appendix: C vs C++

Wherever possible, C++ should be used in preference to C. If use of C is
necessary, the code has to be written in C89 style as not all platforms have
a native C compiler that is compatible with C99 (in particular, the C compiler
supplied as part of MSVC doesn't understand most C99 extensions). Often, it is
possible to compile C99 code in C++ mode, which is supported by all compilers.

There are some features of C that are not present in C++, but many of these can
be worked around with careful use of headers in the C standard library; for
example, include `stdbool.h` and use the `bool` typedef instead of the raw
`_Bool` built-in type.

**Note:**
> One particular "gotcha" is that the MSVC C compiler does not support C++-style
> variable declarations: all variables must be declared at the beginning of a 
> block before any other statements. It also does not support declaration of
> variables inside the condition of a `for` loop.

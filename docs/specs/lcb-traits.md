# Traits in LiveCode Builder
Copyright 2015 LiveCode Ltd.

## Introduction

There's a need in Builder to be able to define operations that are specialized
by type.

Traits allow methods to be "added on" to other types and allow in turn allow
those other types to be used anywhere where the trait may be used.

## Background: the string formatting problem

An example of this requirement is the `formatted as string` syntax.  Ideally,
a value of any type `T` should be able to be used with the `formatted as
string` syntax, e.g.:

    variable tVar as T
    variable tString as string
    put tVar formatted as string into tString

### Polymorphic syntax is insufficient

Since Builder has polymorphic syntax, it would be legitimate to write a syntax
definition and associated handler of the form:

    syntax TFormattedAsString is postfix operator with precedence 1
        <Operand: Expression> "formatted" "as" "string"
    begin
        TFormatAsString(Operand, output)
    end syntax

    public handler TFormatAsString(in pValue as T, out rFormat as string)
        -- Perform formatting on T
    end handler

There are significant drawbacks to this approach.

1. Syntax definition has a low level of accessibility.  It's often not clear
   what precedence a given syntax definition should be given, and what the
   implications of the various syntax classes are.

2. In the current Builder implementation, syntax must be built into the
   compiler.  Hooking in additional syntax requires a large number of
   modifications around the LiveCode build system, and a full recompilation of
   the whole of LiveCode.  This provides an extremely high barrier to entry if
   all that's wanted is to add a type specialization to a syntax that is
   already standard.

3. The only "interesting" part of the specialization is the handler which
   performs the formatting.  The rest of the definition is redundant.

### Generic handlers are insufficient

It has previously been suggested that Builder should have generic handlers.
For example, the standard library could contain a generic handler declaration:

    module S

    public generic handler FormatAsString(in pOperand as generic, \
                                          out rFormat as string)

Then, another module could define a formattable type:

    module M
    use S
    public type T -- Some full declaration

    public handler FormatAsString(in pOperand as T, out rFormat as string)
        -- Perform formatting on T
    end handler

In this case, the compiler would identify that `M.FormatAsString<T>()` has a
name that matches a generic handler declaration, and that it conforms (all
generic parameters are specialised and all non-generic parameters are of the
correct types).  Subsequent calls to `FormatAsString()` from *anywhere in the
program* would then be able to access `M.FormatAsString<T>()` if the first
argument of `FormatAsString()` conforms to `T`.

The main drawback here is that Builder is optionally-typed, but it would be
good to ensure that it's possible to write code where all types are specified
*and can be verified* at compile time.  This doesn't satisfy the requirement.

For example, it would be convenient to write a `Log()` handler that can log
any type that can also be formatted.  However, with the generic handlers scheme
described above, `Log()` would have to take arguments as `any` and defer
type-checking to runtime.

## Traits

Traits are collections of methods that can be used to extend the functionality
of a type.

Traits have no storage associated with them.  Storage is provided by
implementing the trait for a specific storage type.

### Defining traits

    public trait type CanFormatAsString
        method type FormatAsString(in me) as string
    end type

A trait definition is a block containing a list of zero or more `method type`
declarations.  `method type` declarations take the same form as existing
`handler type` declarations.

Two additional symbols are defined within a trait definition:

1. `MyType` is always equivalent to the type of the value that the method is
   being called for.
2. `me` must always be the first parameter a `method` definition or
   `method type` declaration; it is equivalent to writing `me as MyType`.  The
   `me` parameter must always be `in` or `inout`.

### Implementing traits

    public implementation of CanFormatAsString for T
        method FormatAsString(in me) as string
            variable tFormatted as string
            -- Format me parameter
            return tFormatted
        end method
    end implementation

`method` definitions take the same form as existing `handler` definitions.

In order for an `implementation of U for T` to be valid, the following criteria
must be fulfilled:

1. **Completeness Rule**.  All methods of `U` must be implemented (either in
   the `implementation` block or via a default implementation in the trait
   definition).
2. **Locality Rule**.  Either `U` or `T` must be local, i.e. defined in the
   current module.
3. **Visibility Rule**.  The visibility of the implementation must be at least
   the accessibility of `U` (i.e. if `U` is `public`, then the implementation
   must also be `public`).

#### Completeness rule

The **Completeness Rule** is required to ensure that it is possible to call any
method of a trait `U` for any value of type `T` for which `U` has been
implemented.

#### Locality rule

The **Locality Rule** prevents two implementations of `T`.  Without this rule,
the following scenario would be possible:

* Module `A` defines and type `T`
* Module `B` implements `CanFormatAsString` for `T`
* Module `C` implements `CanFormatAsString` for `T` (differently)
* Module `D` uses modules `B` and `C` and attempts to format an instance of `T`
  as a string.  Which implementation should be used?

By requiring either the trait type or the concrete type to be defined locally
to the implementation, this diamond problem can never occur.

#### Visibility rule

The **Visibility Rule** is required for partially-typed and untyped code that
calls trait methods.  Consider the following partially-typed module:

    type T

    public implementation of CanFormatAsString for T
        -- Implementation here
    end implementation

    public handler LeakInstanceOfT() as list
        variable tVar as T
        -- Construct tVar
        return [tVar]
    end handler

Coupled with another module that contains a call:
`LeakInstanceOfT().FormatAsString()`.  Requiring any implementation of a
`public` trait to also be `public` allows this behaviour to be intuitively
correct.

It may be possible to relax this rule if each module has a different list of
trait implementations attached to the `T` type.  However, the rule simplifies
things to start off with!

### Method calls

    public handler MCStringEvalFormatAsString(in pValue as CanFormatAsString,
                                              out rFormatted as string)
        pValue.FormatAsString() into rFormatted
    end handler

Selecting an appropriate method implementation for a value is a type of
namespace operation.  Currently, `.` is used for module-based namespace
disambiguation.  It is therefore logical to reuse it for method calls.

    syntax FormattedAsString is postfix operator with precedence 1
        <Operand: Expression> "formatted" "as" "string"
    begin
        MCStringEvalFormatAsString(Operand, output)
    end syntax

Handler-based `syntax` definitions can be used to provide type-generic syntax
by using trait types.  Methods do not need to directly used in `syntax` blocks.

### Derived traits

    public trait type Circle derived from Shape
        method type Radius(in me) as number
    end type

One trait can be derived from other traits.  A derived trait inherits all of
the `method type` declarations.

    public trait type Circle derived from (Shape, CanBeEqual)
        method type Radius(in me) as number
        method type Centre(in me) as Point
    end type

When declaring a derived trait, the **Uniqueness Rule** must be observed.

#### Uniqueness rule

For a trait `A` and its supertraits `B, ..., k`, no pair of methods declared in
`A, B, ..., k` may have the same name but a different prototype.

This may be relaxed if support for polymorphic dispatch is added to Builder.

## Further work

### Parameterized types

Traits do not fully solve the problem of implementing `formatted as string`
syntax with full generality and type safety.  There remains the problem of
formatting polymorphic compound types such as `list`.  Consider the following
naïve approach:

    public implementation of CanFormatAsString for list
        method FormatAsString(in me) as string
            variable tString as string
            put "(" into tString

            variable tIter as any
            repeat for each element tElement of me
                variable tElement as CanFormatAsString
                put tIter into tElement -- TYPE UNSAFE
                put tElement formatted as string after tString
            end repeat

            put ")" after tString
            return tString
        end method
    end implementation

If the above method is compiled with strict type checking, then the marked line
should fail to compile because `tIter` is of unknown type `any`, and therefore
does not necessarily implement the `CanFormatAsString` trait.

It would be better if the `CanFormatAsString` trait was only defined for lists
where every element also conforms to `CanFormatAsString`.

Similar arguments apply for the `array` type.

To get full generality and type safety of the `formatted as string` trait
Builder therefore also needs type specialization (e.g. `list of
CanFormatAsString`).

## Conclusions

Traits provide a reasonably straightforward and clean way to provide extensible
standard library syntax without the need for generic handlers and (mostly)
preserving the ability to perform compile-time type checking.

## Appendices

### Trait implementations vs. versioning

Suppose Alice creates a library `org.example.alice@1` declaring type `A@1`.

Subsequently, Bob creates a library `org.example.bob@1`, which defines a
super useful trait `B@1` and implements it for a bunch of types, including
`A@1`.

Types are opaque outside the defining module.  Alice therefore realises that
she can provide a much more efficient implementation of `B@1` inside the
`org.example.alice@2` module.  However:

* Alice cannot add such an implementation in the `org.example.alice` module,
  because it would introduce a cyclic dependency with `org.example.bob`.
* Alice cannot add an implementation in a new `org.example.alice.bob-support`
  module, both because of the Locality Rule and because `A@1` would be opaque
  (outside the defining module).

However, Alice *can* define a new `Bprime@2` trait in `org.example.alice` that
has the same methods as `B@1`, implement `Bprime@2` for `A@1`, and encourage
Bob to release a new `org.example.bob@2` version where the implementation of
`B@1` for `A@1` tightly wraps `Bprime@2`.

### Interaction with optional typing

Builder is an optionally-typed language.  In order to facilitate the use of
traits in untyped Builder code, there are some implementation requirements.

#### Storage typeinfos must collect all implemented traits

Consider the `implementation of CanFormatAsString for list` in the
"Parameterized Types" section above.  This is valid partially-typed Builder
code and should work.

In order for it to work, it must be possible to decide whether each value in
the `list` has an implementation of`CanFormatAsString` by direct examination of
the value.

Each concrete typeinfo (and thus each value) will need to have an attached list
of all trait implementations for that type.

#### Invoked traits must be in lexical scope

As described above, all trait implementations for a concrete type are attached
to its typeinfo.  However, this causes a potential problem when working with
partially-typed code.

    public trait type U
        method f(in me)
    end type

    public handler CastsToAny(in pIn as U)
        variable tVar as any
        put pIn into tVar

        tVar.f()
    end handler

If `U` is the only trait in this module and its dependency modules that defines
a `f()` method, this should be unambiguous.  It shouldn't be possible for a
remote implementation of some other trait `V` to break `CastsToAny()`.

For method dispatch, therefore, it should only be possible to call methods of
traits that are in scope.

#### Optional typing can reveal implementation details

Consider the following module:

    public trait type U
    end type

    public handler ReturnsArgument(in pIn as U) as U
        return pIn
    end handler

In the initial implementation, an untyped user of the module could implement
`U` for a new type `T`, filter it through `ReturnsArgument()`, and continue to
call `T`-specific methods on the return value.  If in a subsequent version of
the module, `ReturnsArgument()` was modified to return some other type that
conforms to `U`, then the untyped user code would break even though the API
may be identical.

This concern could be resolved by providing type-parameterized generic
functions where type parameters could be bounded by trait.  For example
(notional syntax):

    public generic (T as U) handler ReturnsArgument(in pIn as T) as T
        return pIn
    end handler

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
        in method type FormatAsString() as string
    end type

    public trait type CanParseFromString
        handler type ParseFromString(in pString as String) as MyType
    end handler

A trait definition is a block containing a list of one or more
`method type` or `handler type` declarations.

An additional symbol, `MyType`, is defined within a trait definition.
It is always equivalent to the concrete type for which the trait is
implemented.

#### Trait methods

    public trait type AtomicArithmeticOps
        inout method type AtomicIncrement() as MyType
        inout method type AtomicDecrement() as MyType
    end type

A trait body may contain `method type` declarations.  `method type`
declarations are similar to `handler type` declarations.

Methods take an implicit argument named `me` of type `MyType`.  This
is the value for which the method is being called.

The `method type` declaration may have an `in` or `inout` qualifier
that controls the mutability of the `me` special argument.  If neither
`in` nor `inout` are specified, `me` is immutable.

Methods may only be invoked for values of concrete types for which the
trait is implemented.

#### Trait handlers

A trait body may contain `handler type` declarations.  These are
procedures that are never invoked for a value.  For example, they may
be used for construction.

Trait handlers must always fulfil the **Relevance Rule**.

#### Relevance rule

The **Relevance Rule** is required to ensure that implementations of a
trait handler are always distinct.

Trait handler prototypes must therefore mention `MyType`.  This means
that the prototype must either return `MyType`, or accept a `MyType`
parameter.

### Implementing traits

    public implementation of CanFormatAsString for T
        method FormatAsString() as string
            variable tFormatted as string
            -- Format implicit "me" parameter
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
`LeakInstanceOfT():FormatAsString()`.  Requiring any implementation of a
`public` trait to also be `public` allows this behaviour to be intuitively
correct.

It may be possible to relax this rule if each module has a different list of
trait implementations attached to the `T` type.  However, the rule simplifies
things to start off with!

### Trait method calls

    public handler MCStringEvalFormatAsString(in pValue as CanFormatAsString,
                                              out rFormatted as String)
        pValue:FormatAsString() into rFormatted
    end handler

The `.` character is reserved for namespace qualification in Builder.
Method invocation is subtly different because unlike a namespace
qualification the result cannot be determined entirely statically.
It's also important to be able to distinguish between calling a method
on a type object and calling a handler of a type object. The `:`
character is therefore used for method invocation syntax.

    syntax FormattedAsString is postfix operator with precedence 1
        <Operand: Expression> "formatted" "as" "string"
    begin
        MCStringEvalFormatAsString(Operand, output)
    end syntax

Handler-based `syntax` definitions can be used to provide type-generic syntax
by using trait types.  Methods do not need to directly used in `syntax` blocks.

### Trait handler calls

    public handler MCLogicEvalStringParsedAsBool(in pValue as String,
                                                 out rParsed as Boolean)
        put Boolean.ParseFromString(pValue) into rParsed
    end handler

Trait handler calls are pure namespace operations, and use the `.`
character.

In the absence of generic functions, trait handlers are most useful as
a convenience mnemonic for frequently-used constructors.

    syntax StringParsedAsBoolean is postfix operator with precedence 1
        <Operand: Expression> "parsed" "as" "boolean"
    begin
        MCLogicEvalStringParsedAsBool(Operand, output)
    end syntax

Trait handlers do not need to be directly used in `syntax` blocks.

## Trait type conversion

In general, explicit casting from one type to another is performed
using the same syntax as for type, parameter and variable
declarations: `as U`, where `U` is the target type.

### Concrete to trait cast in expression context

    trait type U
        method type Func()
    end type
    implementation of U for Number
        method Func()
        end method
    end implementation

    handler DoFunc(in pParam as U)
        U:Func()
    end handler

    public handler main()
        DoFunc(5)
    end handler

Implementation-based implicit type conversion occurs wherever
necessary and unambiguous.

In this case, `DoFunc` requires an parameter of type `U`.  The
parameter supplied is of type `Number`.  Since `U` is implemented for
`Number`, the `Number` can be passed to `DoFunc` without an explicit
type cast.

    trait type U
        method type Func() as Number
    end type
    implementation of U for Number
        method Func()
            return me + 1
        end method
    end implementation

    trait type V
        method type Func() as Number
    end type
    implementation of V for Number
        method Func()
            return me - 1
        end method
    end implementation

    public handler main() as Number
        return (5 as U):Func() -- returns 6
    end handler

It is possible for a concrete type to have multiple implementations in
scope with conflicting method names, leading to ambiguity.  If so, an
explicit instance cast is required to remove the ambiguity.

In this case, both the `U` and `V` traits declare a `Func()` method,
and both are implemented for `Number`.  It is therefore an error to
invoke `5:Func()` directly; an explicit instance cast to either `U` or
`V` is required.

### Concrete to trait cast in type expression context

    trait type U
        handler type Func() as MyType
    end type
    implementation of U for Number
        handler Func()
            return 1
        end handler
    end implementation

    trait type V
        handler type Func() as MyType
    end type
    implementation of V for Number
        handler Func()
            return -1
        end handler
    end implementation

    public handler main() as Number
        return (Number as U).Func() -- returns 1
    end handler

It is possible for a concrete type to have multiple implementations in
scope with conflicting handler names, leading to ambiguity.  If so, an
explicit type cast is required to remove the ambiguity.

In this case, both the `U` and `V` traits declare a `Func()` handler,
and both are implemented for `Number`.  It's therefore an error to
invoke `Number.Func()` directly; an explicit type cast to either `U`
or `V` is required.

## Conclusions

Traits provide a reasonably straightforward and clean way to provide extensible
standard library syntax without the need for generic handlers and (mostly)
preserving the ability to perform compile-time type checking.

## Appendices

### Parameterized types

Traits do not fully solve the problem of implementing `formatted as string`
syntax with full generality and type safety.  There remains the problem of
formatting polymorphic compound types such as `list`.  Consider the following
naïve approach:

    public implementation of CanFormatAsString for list
        method FormatAsString() as string
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

        tVar:f()
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

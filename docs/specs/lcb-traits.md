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
    end trait

A trait definition is a block containing a list zero or more `method`
definitions or `method type` declarations.  `method type` declarations take the
same form as existing `handler type` declarations.  `method` definitions take
the same form as existing `handler` declarations.

    public trait type CanFormatAsString
        method FormatAsString(in me) as string
            return "<unknown>"
        end method
    end trait

`method` definitions allow a trait's definition to include default
implementations of the trait's methods.  Default implementations can call other
methods of the trait or perform any operations valid for the generic `Self`
type.

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

The `me` and `MyType` symbols are also defined inside implementation blocks.

In order for an `implementation of U for T` to be valid, the following criteria
must be fulfilled:

1. Either `U` or `T` must be local, i.e. defined in the current module.  This
   makes it impossible for two identical implementations to exist for the same
   trait.
2. All methods of `U` must be implemented (either in the `implementation`
   block or via a default implementation in the trait definition.

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
    end

Handler-based `syntax` definitions can be used to provide type-generic syntax
by using trait types.  Methods do not need to directly used in `syntax` blocks.

### Derived traits

    public trait type AsString derived from FormatAsString
        method type AsString(in me) as string
        method FormatAsString(in me) as string
            return me.AsString()
        end method
    end trait

One trait can be derived from other traits.  A derived trait inherits all of
the `method type` declarations and default `method` implementations from its
supertraits.  A derived trait definition can provide new default `method`
implementations for supertrait methods.

    public trait type Point derived from (CanFormatAsString, CanBeEqual)
        method type X(in me) as number
        method type Y(in me) as number

        method IsEqual(in me, in pOther as Point) as boolean
            if me.X() = pOther.X() and me.Y() = pOther.Y() then
                return true
            else
                return false
            end if
        end method

        method FormatAsString(in me) as string
            return "(" & me.X() formatted as string & ", " & \
                   me.Y() formatted as string & ")"
        end method
    end trait

Traits can be derived from multiple supertraits.

## Trait objects

    public trait U
        method type F(in me)
    end trait

    type T

    implementation of U for T
        method F(in me)
        end method
    end implementation

    public handler G(in pParam as U)
        pParam.F()
    end handler

A trait object is created whenever a concrete type `T` needs to be passed by
trait `U` rather than directly as an `T`.  It packages the `T` value with
information that identifies the methods of the implementation of `U` that was
used.

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

### Interaction with runtime typing

    trait type T
    end trait

    implementation of T for integer
    end implementation

    handler Confuse(in pIn as T) as T
        return pIn
    end handler

    public handler main()
        variable tVar as any
        put 15 into tVar        -- contents of tVar is an integer
        Confuse(tVar) into tVar -- contents of tVar is a T trait object
        add 1 to tVar           -- error?
    end handler

It is not clear what the correct behaviour is here.

Instead of an error, one option is to "unpack" any trait object when assigned
to an `any` slot.  However, consider:

    public trait type T
        method F(in me)
    end trait

    public handler F(in pIn as T) -- pIn is a T trait object
        variable tVar as any
        put pIn into tVar         -- tVar is a ?? (out of lexical scope)

        tVar.F()                  -- error?
    end module

## Conclusions

Traits provide a reasonably straightforward and clean way to provide extensible
standard library syntax without the need for generic handlers and (mostly)
preserving the ability to perform compile-time type checking.

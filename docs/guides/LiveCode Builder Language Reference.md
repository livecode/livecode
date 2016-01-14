# Introduction

LiveCode Builder is a variant of the current LiveCode scripting language (LiveCode Script) which has been designed for 'systems' building. It is statically compiled with optional static typing and direct foreign code interconnect (allowing easy access to APIs written in other languages).

Unlike most languages, LiveCode Builder has been designed around the idea of extensible syntax. Indeed, the core language is very small - comprising declarations and control structures - with the majority of the language syntax and functionality being defined in modules.

> **Note:** It is an eventual aim that control structures will also be extensible, however this is not the case in the current incarnation).

The syntax will be familiar to anyone familiar with LiveCode Script, however LiveCode Builder is a great deal more strict - the reason being it is intended that it will eventually be compilable to machine code with the performance and efficiency you'd expect from any 'traditional' programming language. Indeed, over time we hope to move the majority of implementation of the whole LiveCode system over to being written in LiveCode Builder.

> **Note:** One of the principal differences is that type conversion is strict - there is no automatic conversion between different types such as between number and string. Such conversion must be explicitly specified using syntax (currently this is using things like *... parsed as number* and *... formatted as string*.

# Tokens

The structure of tokens is similar to LiveCode Script, but again a little stricter. The regular expressions describing the tokens are as follows:

 - **Identifier**: [A-Za-z_][A-Za-z0-9_.]*
 - **Integer**: [0-9]+
 - **Real**: [0-9]+"."[0-9]+([eE][-+]?[0-9]+)?
 - **String**: "[^\n\r"]*"
 - **Separator**: Any whitespace containing at least one newline

Strings use backslash ('\') as an escape - the following are understood:

 - **\n**: LF (ASCII 10)
 - **\r**: CR (ASCII 13)
 - **\t**: TAB (ASCII 9)
 - **\q**: quote '"'
 - **\u{X...X}: character with unicode codepoint U+X...X - any number of nibbles may be specified, but any values greater than 0x10FFFF will be replaced by U+FFFD.
 - **\\**: backslash '\'

> **Note:** The presence of '.' in identifiers are used as a namespace scope delimiter.

> **Note:** Source files are presumed to be in UTF-8 encoding.

## Case-Sensitivity

At the moment, due to the nature of the parser being used, keywords are all case-sensitive and reserved. The result of this is that, using all lower-case identifiers for names of definitions should be avoided. However, identifiers *are* case-insensitive - so a variable with name pFoo can also be referenced as PFOO, PfOO, pfoO etc.

> **Aside:** The current parser and syntax rules for LiveCode Builder are constructed at build-time of the LiveCode Builder compiler and uses *bison* (a standard parser generator tool) to build the parser. Unfortunately, this means that any keywords have to be reserved as the parser cannot distinguish the use of an identifier in context (whether it is a keyword at a particular point, or a name of a definition).

It is highly recommended that the following naming conventions be used for identifiers:

 - **tVar** - for local variables
 - **pVar** - for in parameterns
 - **rVar** - for out parameters
 - **xVar** - for inout parameters
 - **mVar** - for global variables in widgets
 - **sVar** - for global variables in libraries
 - **kConstant** - for constants
 - Use identifiers starting with an uppercase letter for handler and type names.

By following this convention, there will not be any ambiguity between identifiers and keywords. (All keywords are all lower-case).

> **Note:** When we have a better parsing technology we will be evaluating whether to make keywords case-insensitive as well. At the very least, at that point, we expect to be able to make all keywords unreserved.

# Typing

Modular LiveCode is a typed language, although typing is completely optional in most places (the only exception being in foreign handler declarations). If a type annotation is not specified it is simply taken to be the most general type *optional any* (meaning any value, including nothing).

The range of core types is relatively small, comprising the following:

 - **nothing**: the single value *nothing*
 - **Boolean**: one of *true* or *false*
 - **Integer**: any integral numeric value (size limitations apply)
 - **Real**: any numeric value (size and accuracy limitations apply)
 - **Number**: any integer or real value
 - **String**: a sequence of UTF-16 code units
 - **Data**: a sequence of bytes
 - **List**: a sequence of any values
 - **Array**: a mapping from strings to values
 - **any**: a value of any type

Additionally, all types can be annotated with **optional**. An optional annotation means the value may be the original type or nothing.

> **Note:** The current compiler does not do type-checking; all type-checking happens at runtime. However, this is being worked on so there will soon be a compiler which will give you type errors at compile-time.

# Modules

    Module
        : 'module' <Name: Identifier> SEPARATOR
            { ( Definition | Metadata | Import ) SEPARATOR }
          'end' 'module'

The smallest compilable unit of Modular LiveCode is the module. Each module is uniquely named using reverse DNS notation, and the names of modules are considered to live in a global namespace.

A module is a collection of public and private definitions, including constants, variables, types and handlers.

A module may depend on another module through import. An imported modules public definitions become accessible to the importing module.

> **Note:** For integration with the existing LiveCode system, there are two module variants which may be used. Widgets (use 'widget' instead of 'module') and Libraries (use 'library' instead of 'module'). A widget appears in LiveCode as a control, whilst a library adds all its public handlers to the bottom of the message path.

# Metadata

    Metadata
        : 'metadata' <Name: Identifier> 'is' <Value: String>

The metadata clauses allow a set of key-values to be encoded in the compiled module. These are not used in compilation or execution, but may be used by the system loading and using the module.

At the moment, the following keys are understood:

 - title: a human-readable name for the module
 - description: a simple description of the module's purpose
 - version: a string in the form X.Y.Z (with X, Y and Z integers) describing the modules version
 - author: the name of the author of the module

> **Note:** The current metadata mechanism is unlikely to remain part of the language. It is intended that it will be replaced by a package description file, which will allow modules to be grouped together with other resources.

# Imports

    Import
        : 'use' <Name: Identifier>

The use clauses allow a module to refer to another module by importing all the target module's public definitions into its namespace.

The name of the module specified must be its full name, e.g. com.livecode.canvas.

A module may use any other module, as long as doing so does not cause a cycle in the dependency graph.

> **Note:** The current IDE and extension installation system does not yet implement arbitrary dependencies - the only dependencies it understands are those which are builtin to the system (e.g. com.livecode.canvas). However, you can still write and test out modules with dependencies locally - they just cannot be uploaded to the extensions portal.

# Definitions

    Definition
      : ( 'public' | 'private' ) ConstantDefinition
      | ( 'public' | 'private' ) TypeDefinition
      | ( 'public' | 'private' ) HandlerTypeDefinition
      | ( 'public' | 'private' ) VariableDefinition
      | ( 'public' | 'private' ) HandlerDefinition
      | ( 'public' | 'private' ) ForeignHandlerDefinition
      | PropertyDefinition
      | EventDefinition

Definitions are what are used to define usable entities in the language. All definitions are named using a unique identifier (so you cannot have two definitions with the same name).

Definitions can be either *public* or *private* (the default is private - so there is no need to explicitly specify that). Public definitions are available when the module is used by another module whereas private definitions can only be used within the module.

> **Note**: Properties and events are, by their nature, always public as they define things which only make sense to access from outside.

> **Note**: When writing a library module, all public handlers are added to bottom of the message path in LiveCode Script.

## Constants

    ConstantDefinition
      : 'constant' <Name: Identifier> is <Value: Expression>

A constant definition defines a named constant. The value can be any expression which depends on only on constant values to evaluate.

> **Note:** Constants are not currently implemented, although the syntax is recognised.

## Types

    TypeDefinition
      : 'type' <Name: Identifier> 'is' <TypeOf: Type>

A type definition defines an alias, it names the given type with the given Name, allowing the name to be used instead of the type.

    Type
      : <Name: Identifier>
      | 'optional' <Target: Type>
      | 'any'
      | 'nothing'
      | 'Boolean'
      | 'Integer'
      | 'Real'
      | 'Number'
      | 'String'
      | 'Data'
      | 'Array'
      | 'List'
      | 'Pointer'

A type clause describes the kind of value which can be used in a variable or parameter.

If a type is an identifier, then this is taken to be a named type defined in a type definition clause.

An optional type means the value can be either the specified type or nothing. Variables which are of optional type are automatically initial zed to nothing.

The remaining types are as follows:

 - **any**: any value
 - **Boolean**: a boolean value, either the value *true* or *false*.
 - **Integer**: any integer number value
 - **Real**: any real number value
 - **Number**: any number value
 - **String**: a sequence of UTF-16 code units
 - **Data**: a sequence of bytes
 - **Array**: a map from string to any value (i.e. an associative array, just like in LiveCode Script)
 - **List**: a sequence of any value
 - **nothing**: a single value *nothing* (this is used to describe handlers with no return value - i.e. void)
 - **Pointer**: a low-level pointer (this is used with foreign code interconnect and shouldn't be generally used).

> **Note:** *Integer* and *Real* are currently the same as *Number*.

> **Note:** In a subsequent update you will be able to specify lists and arrays of fixed types. For example, *List of String*.

> **Note:** In a subsequent update you will be able to define record types (named collections of values - like structs in C) and handler types (allowing dynamic handler calls through a variable - like function pointers in C).

## Handler Types

    HandlerTypeDefinition
      : [ 'foreign' ] 'handler' 'type' <Name: Identifier> '(' [ ParameterList ] ')' [ 'returns' <ReturnType: Type> ]

A handler type definition defines a type which can hold a handler. Variables of
such types can hold handlers (just like function pointers in C) which allows them
to be called dynamically.

If the handler type is defined as foreign then automatic bridging to a C function
pointer will occur when the type appears as the type of a parameter in a foreign
handler definition.

> **Note:** Passing an LCB handler to a foreign function requires creation of
a function pointer. The lifetime of the function pointer is the same as the widget
or module which created it.

## Variables

    VariableDefinition
      : 'variable' <Name: Identifier> [ 'as' <TypeOf: Type> ]

A variable definition defines a module-scope variable. In a widget module, such variables are per-widget (i.e. instance variables). In a library module, there is only a single instance (i.e. a private global variable).

The type specification for the variable is optional, if it is not specified the type of the variable is *optional any* meaning that it can hold any value, including being nothing.

## Handlers

    HandlerDefinition
      : 'handler' <Name: Identifier> '(' [ ParameterList ] ')' [ 'returns' <ReturnType: Type> ] SEPARATOR
          { Statement }
        'end' 'handler'

Handler definitions are used to define functions which can be called from LiveCode Builder code, invoked as a result of events triggering in a widget module, or called from LiveCode Script if public and inside a library module.

There is no distinction between handlers which return a value and ones which do not, apart from the return type. Handlers can be called either in expression context, or in statement context. If a handler which returns no value (it is specified as *returns nothing*) is called in expression context then its value is *nothing*.

	ParameterList
	  : { Parameter , ',' }

    Parameter
      : ( 'in' | 'out' | 'inout' ) <Name: Identifier> [ 'as' <ParamType: Type>

The parameter list describes the parameters which can be passed to the handler. Handlers must be called with the correct number of parameters, using expressions which are appropriate to the mode.

An in parameter means that the value from the caller is copied to the parameter variable in the callee handler.

An out parameter means that no value is copied from the caller, and the value on exit of the callee handler is copied back to the caller on return.

> **Note:** It is a checked runtime error to return from a handler without ensuring all non-optional 'out' parameters have been assigned a value.

An inout parameter means that the value from the caller is copied to the parameter variable in the callee handler on entry, and copied back out again on exit.

The type of parameter is optional, if no type is specified it is taken to be *optional any* meaning it can be of any type.

> **Note:** Only assignable expressions can be passed as arguments to inout or out parameters. It is a checked compile-time error to pass a non-assignable expression to such a parameter.

## Foreign Handlers

    ForeignHandlerDefinition
      : 'foreign' 'handler' <Name: Identifier> '(' [ ParameterList ] ')' [ 'returns' <ReturnType: Type> ) ] 'binds' 'to' <Binding: String>

    ForeignType
      : Type
      | 'CBool'
      | 'CInt'
      | 'CUInt'
      | 'CFloat'
      | 'CDouble'

A foreign handler definition binds an identifier to a handler defined in foreign code.

Foreign handler definitions can contain more types in their parameters than elsewhere - those specified in the ForeignType clause. These allow low-level types to be specified making it easier to interoperate.

Foreign types map to high-level types as follows:

 - bool maps to boolean
 - int and uint map to integer (number)
 - float and double map to real (number)

This mapping means that a foreign handler with a bool parameter say, will accept a boolean from LiveCode Builder code when called.

At present, only C binding is allowed and follow these rules:

 - any type passes an MCValueRef
 - nothing type passes as the null pointer
 - Boolean type passes an MCBooleanRef
 - Integer type passes an MCNumberRef
 - Real type passes an MCNumberRef
 - Number type passes an MCNumberRef
 - String type passes an MCStringRef
 - Data type passes an MCDataRef
 - Array type passes an MCArrayRef
 - List type passes an MCProperListRef
 - Pointer type passes a void *
 - CBool type passes a bool (i.e. an int - pre-C99).
 - CInt type passes an int
 - CUInt type passes an unsigned int
 - CFloat type passes a float
 - CDouble type passes a double

Modes map as follows:

 - in mode is just pass by value
 - out mode passes a pointer to a variable of one of the above types, the variable is uninitialized on entry
 - inout mode passes a pointer to a variable of one of the above types, the variable is initialized to a value on entry

If the return type is of a Ref type, then it must be a copy.

If an out parameter is of a Ref type, then it must be a copy (on exit)

If an inout parameter is of a Ref type, then its existing value must be released, and replaced by a copy (on exit).

The binding string for foreign handlers has the following form:

    [lang:][library>][class.]function[!calling]

Here *lang* specifies the language (must be 'c' at the moment)

Here *library* specifies the name of the library to bind to (if no library is specified a symbol from the engine executable is assumed).

Here *class* is currently unused.

Here *function* specifies the name of the function symbol to bind to (on Windows, the symbol must be unadorned, and so exported from the library by listing it in a DEF module).

Here *calling* specifies the calling convention which can be one of:

 - default
 - stdcall
 - thiscall
 - fastcall
 - cdecl
 - pascal
 - register

All but 'default' are Win32-only, and on Win32 'default' maps to 'cdecl'. If a Win32-only calling convention is specified on a non-Windows platform then it is taken to be 'default'.

Foreign handler's bound symbols are resolved on first use and an error is thrown if the symbol cannot be found.

> **Note:** The current foreign handler definition is an initial version, mainly existing to allow binding to implementation of the syntax present in the standard language modules. It will be expanded and improved in a subsequent version to make it very easy to import and use functions (and types) from other languages including Objective-C (on Mac and iOS) and Java (on Android).

## Properties

    PropertyDefinition
      : 'property' <Name: Identifier> 'get' <Getter: Identifier> [ 'set' <Setter: Identifier> ]

Property definitions can only appear in widget modules. They define a property which can be accessed from LiveCode Script in the usual way (e.g. *the myProperty of widget 1*).

Both getter and setter clauses can use either a variable or handler identifier. If a variable identifier is used, then the property value is fetched (and stored) from that variable. If a handler identifier is used then a handler is called instead.

A getter handler must take no arguments and return a value. A setter handler must take a single argument and return no value.

The set clause is optional. If it is not present then the property is read-only.

## Events

    EventDefinition
      : 'event' <Name: Identifier> '(' [ ParameterList ] ')' [ 'as' <ReturnType: Type> ]

Event definitions define a callable handler which calls back to the environment.

> **Note**: Whilst events can be defined they currently cannot be used. To send a message to the LiveCode Script environment use the *dispatch* command which allows sending messages to arbitrary LiveCode Script objects.

# Statements

    Statement
      : VariableStatement
      | IfStatement
      | RepeatStatement
      | ThrowStatement
      | ReturnStatement
      | PutStatement
      | SetStatement
      | GetStatement
      | CallStatement

There are a number of built-in statements which define control flow, variables, and basic variable transfer. The remaining syntax for statement is defined in auxiliary modules.

## Variable Statements

    VariableStatement
      : 'variable' <Name: Identifier> [ 'as' <TypeOf: Type> ]

A variable statement defines a handler-scope variable. Such variables can be used after the variable statement, but not before.

> **Note:** Variables are currently not block-scoped, they are defined from the point of declaration to the end of the handler - this might change in a subsequent revision.

Variables are initially undefined and thus cannot be fetched without a runtime error occurring until a value is placed into them. If a variable has been annotated with an optional type, its initial value will be nothing.

> **Note:** It is a checked runtime error to attempt to use a non-optionally typed variable before it has a value.

The type specification for the variable is optional, if it is not specified the type of the variable is *optional any* meaning that it can hold any value, including being nothing.

## If Statements

    IfStatement
      : 'if' <Condition: Expression> 'then' SEPARATOR
          { Statement }
        [ { 'else' 'if' <Condition: Expression> 'then' SEPARATOR
            { Statement } ]
        [ 'else' SEPARATOR
            { Statement } ]
        'end' 'if'

The if statement enables conditional execution based on the result of an expression which evaluates to a boolean.

> **Note:** It is a checked runtime error to use an expression which does not evaluate to a boolean in any condition expression.

## Repeat Statements

    RepeatStatement
      : RepeatHeader SEPARATOR
          { Statement }
        'end' 'repeat'
      | 'next' 'repeat'
      | 'exit' 'repeat'

    RepeatHeader
      : 'repeat' 'forever'
      | 'repeat' <Count: Expression> 'times'
      | 'repeat' 'while' <Condition: Expression>
      | 'repeat' 'until' <Condition: Expression>
      | 'repeat' 'with' <Counter: Identifier> 'from' <Start: Expression> ( 'up' | 'down' ) 'to' <Finish: Expression> [ 'by' <Step: Expression> ]
      | 'repeat' 'for' 'each' <Iterator> 'in' <Container: Expression>

The repeat statements allow iterative execute of a sequence of statements.

The **repeat forever** form repeats the body continually. To exit the loop, either an error must be thrown, or **exit repeat** must be executed.

The **repeat times** form repeats the body Count number times. If Count evaluates to a negative integer, it is taken to be zero.

> **Note:** It is a checked runtime error to use an expression not evaluating to a number as the Count.

The **repeat while** form repeats the body until the Condition expression evaluates to false.

> **Note:** It is a checked runtime error to use an expression not evaluating to a boolean as the Condition.

The **repeat until** form repeats the body until the Condition expression evaluates to true.

> **Note:** It is a checked runtime error to use an expression not evaluating to a boolean as the Condition.

The **repeat with** form repeats the body until the Counter variable reaches or crosses (depending on iteration direction) the value of the Finish expression. The counter variable is adjusted by the value of the Step expression on each iteration. The start, finish and step expressions are evaluated before the loop starts and are not re-evaluated. The Counter variable must be declared before the repeat statement.

> **Note:** It is a checked runtime error to use expressions not evaluating to a number as Start, Finish or Step.

The **repeat for each** form evaluates the Container expression, and then iterates through it in a custom manner depending on the Iterator syntax. For example:

    repeat for each char tChar in "abcdef"
      -- do something with tChar
    end repeat

The **next repeat** statement terminates the current iteration of the enclosing loop and starts the next iteration of the loop, or exits if currently on the last iteration.

The **exit repeat** statement terminates the current iteration of the enclosing loop, continuing execution at the statement after the enclosing loop's **end repeat**.

## Throw Statements

    ThrowStatement
      : 'throw' <Error: Expression>

The **throw** statement causes an error to be raised. This causes execution to terminate, and the error is passed back to environment.

The Error expression must be an expression that evaluates to a string.

> **Note:** There is currently no try / catch mechanism in LiveCode Builder, so throwing an error will cause the error to be raised in LiveCode Script in the appropriate context.

## Return Statements

    ReturnStatement
      : 'return' [ <Value: Expression> ]

The **return** statement causes execution of the current handler to end, and control return to the caller.

If a Value expression is specified, it is evaluated and returned as the result of the handler call.

> **Note:** It is a checked runtime error for a value returned from a handler to not match the return type of the handler it is in.

## Transfer Statements

    PutStatement
      : 'put' <Value: Expression> into <Target: Expression>

    SetStatement
      : 'set' <Target: Expression> 'to' <Value: Expression>

The **put** and **set** statements evaluate the Value expression and assign the resulting value to the Target expression.

The target expression must be assignable.

> **Note:** It is a checked runtime error for the source value's type to not be compatible with the target expression's type.

    GetStatement
      : 'get' <Value: Expression>

The **get** statement evaluates the Value expression and returns it as the result of the statement. The value is subsequently available by using **the result** expression.

## Call Statements

    CallStatement
      : <Handler: Identifier> '(' [ <Arguments: ExpressionList> ] ')'

The call statement executes a handler with the given arguments.

The Handler identifier must be bound to either a handler or foreign handler definition.

The Arguments are evaluated from left to right and passed as parameters to the variable.

> **Note:** It is a checked runtime error for the types of 'in' and 'inout' arguments to not match the declared types of the handler's parameters.

Any parameters of 'out' type are not evaluated on entry, but assigned to on exit.

Any parameters of 'inout' type are evaluated on entry, and assigned on exit.

> **Note:** It is a checked compile-time error to pass non-assignable expressions to parameters which are of either 'out' or 'inout' type.

The return value of a handler is subsequently available by using **the result** expression.

> **Note:** All handlers return a value, even if it is nothing. This means that calling a handler will always change **the result**.

# Expressions

    Expression
      : ConstantValueExpression
      | VariableExpression
      | ResultExpression
      | ListExpression
      | ArrayExpression
      | CallExpression

There are a number of expressions which are built-in and allow constant values, access to call results, list construction and calls. The remaining syntax for expressions is defined in auxiliary modules.

## Constant Value Expressions

      ConstantValueExpression
        : 'nothing'
        | 'true'
        | 'false'
        | INTEGER
        | REAL
        | STRING

Constant value expressions evaluate to the specified constant value.

The **nothing** expression evaluates to the nothing value and can be assigned to any optional typed variable.

The **true** and **false** expressions evaluate to boolean values.

The INTEGER and REAL expressions evaluate to numeric values.

The STRING expression evaluates to a string value.

Constant value expressions are not assignable.

## Variable Expressions

    VariableExpression
      : <Name: Identifier>

Variable expressions evaluate to the value of the specified variable.

Variable expressions are assignable.

## Result Expressions

    ResultExpression
      : 'the' 'result'

The result expression evaluates to the return value of the previous (executed) non-control structure statement.

Result expressions are not assignable.

## List Expressions

    ListExpression
      : '[' [ <Elements: ExpressionList> ] ']'

A list expression evaluates all the elements in the expression list from left to right and constructs a list value with them as elements.

The elements list is optional, so the empty list can be specified as *[]*.

List expressions are not assignable.

## Array Expressions

    ArrayExpression
      : '{' [ <Contents: ArrayDatumList> ] '}'
    ArrayDatumList
      : <Head: ArrayDatum> [ ',' <Tail: ArrayDatumList> ]
    ArrayDatum
      : <Key: Expression> ':' <Value: Expression>

An array expression evaluates all of the key and value expressions
from left to right, and constructs an **Array** value as appropriate.
Each key expression must evaluate to a **String**.

The contents are optional, so the empty array can be written as `{}`.

Array expressions are not assignable.

## Call Expressions

    CallExpression
      : <Handler: Identifier> '(' [ <Arguments: ExpressionList> ] ')'

A call expression executes a handler.

Its use is identical to a call statement, except that the return value of the handler is the value of the expression, rather than being available as **the result**.

> **Note:** Handlers which return no value (i.e. have nothing as their result type) can still be used in call expressions. In this case the value of the call is **nothing**.

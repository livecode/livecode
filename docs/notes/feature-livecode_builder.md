---
version: 8.0.0-dp-1
---
# LiveCode Builder

### LiveCode Builder Language

LiveCode Builder (LCB) is a variant of the current LiveCode scripting
language (LiveCode Script) which has been designed for "systems"
building. It is statically compiled with optional static typing and
direct foreign code interconnect (allowing easy access to APIs written
in other languages such as C). The compiled bytecode can then be
packaged together with any required resources (icons, documentation,
images, etc) into a `.lce` extension package.

Unlike most languages, LiveCode Builder (LCB) has been designed around
the idea of extensible syntax. The core language is very small —
comprising declarations and control structures — with the majority of
the language syntax and functionality being defined in modules written
in LCB itself.

**Note:** It is an eventual aim that control structures will also be
extensible.  However, this has not yet been implemented.

The syntax will be familiar to anyone who has coded with LiveCode
Script.  However, LCB is a great deal more strict.  The increased
strictness is because LCB is intended to eventually be compilable to
machine code, with the performance and efficiency you'd expect from
any 'traditional' programming language like C or C++.  Over time we
hope to move the majority of implementation of the whole LiveCode
system over to being written in LCB.

**Note:** One of the principal differences is that type conversion is
strict.  There is no automatic conversion between different types,
such as between numbers and strings. conversion must be explicitly
specified using syntax (currently this is done using syntax like
`<expression> parsed as number` and `<expression> formatted as
string`.

### Extensions

There are two types of extensions which can be written in LiveCode
Builder: widgets and libraries. All installed extensions appear in the
new "Extension Manager" stack, which can be opened from the "Tools"
menu.

An LCB **library** is a new way of adding functions to the LiveCode
message path. Public handlers in loaded LCB libraries are available to
call from LiveCode Script.

A **widget** is a new type of custom control which, once compiled and
packaged, can be loaded into the IDE. Using the widget is no different
from any of the classic LiveCode controls you've been used to. Simply
drag it onto a stack and start interacting with it as you would any
another control.

You can reference the widget in script both as a control:

```
set the name of the last control to "clock"
```

and more specifically as a widget:

```
set the tooltip of widget 1 to "This is my nice new clock widget"
```

### Getting Started

To get started with LiveCode Builder, click on the "Dictionary" icon in the IDE
toolbar, select the "Guide" tab and then "Extending LiveCode" from the
drop-down menu. This will show you the user-guide on getting started
with writing widgets and libraries in LCB.

Alternatively, you can start by looking at some of the extensions
shipped with LiveCode 8.0. The source and other resources for these are
located in the `extensions` sub-folder of your LiveCode installation
directory.

LCB source files are named `*.lcb` and can be edited with a text
editor of your choice.  For example, there is a
[language-livecode ](https://atom.io/packages/language-livecode)
package available for the [Atom editor](https://atom.io/) which
provides syntax highlighting and autocompletion when editing LCB
source code.

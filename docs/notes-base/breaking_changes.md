# Breaking changes

## Boolean constants

In this release, boolean constants `true` and `false` have been changed so that
they resolve to values of boolean type (rather than string). This will affect
any uses of the `is strictly` operator on such values, i.e. previously the following
were true:

  true is strictly a string
  false is strictly a string

Now, they are both false, and the following are true:

  true is strictly a boolean
  false is strictly a boolean

Boolean constants passed as elements of arrays to LCB handlers will not require
conversion to boolean values in LCB - in fact any attempt to do so assuming they
are strings will cause an error. Any array elements which are intended to be
booleans in LCB should be checked for their type before conversion. For example,
any of the following could be done by an LCB library user:

    put true into tArray["enabled"]
    put "true" into tArray["enabled"]
    put (tVar is not "enabled") into tArray["enabled"]

An LCB handler to which `tArray` is passed should do the following:

    variable tEnabled as Boolean
    if tArray["enabled"] is a boolean then
        put tAction["enabled"] into tEnabled
    else
        put tAction["enabled"] parsed as boolean into tEnabled
    end if

## Infinity constant

The constant `infinity` has been added to the language in this release. As a
result, the unquoted literal `infinity` is now reserved. Any existing uses
of it should be quoted, as otherwise it will resolve to the floating point
value representing infinity, rather than the string "infinity".

## Implicit object

A number of LCB commands use an implicit object to provide context for their
execution. Some of these commands also allow specifying an explicit object.
These commands are:

- `execute script`
- `send`
- `post`
- `image from file`
- `resolve file` - new in this version

In previous releases `execute script` and `image from file` would use
`this card of the defaultStack` as the implicit object even if called from
a widget. The `send` and `post` commands, however, used
`this card of the defaultStack` when in a library module handler and the host
widget when in a widget module handler. This release changes `execute script`
and `image from file` to also use the host widget as the implicit object. This
means, for example, that `image from file` will resolve a relative file path
relative to the `stackFile` the host widget is on rather than the `stackFile` of
the `defaultStack`.

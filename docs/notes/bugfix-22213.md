# A new output kind `detailed-utf8` has been added to the `files` and `folders` functions

The `{ long | detailed } { files | folders } of <folder>`,
`files(<folder>, "detailed")`, and `folders(<folder>, "detailed")` suffer from
an anomaly bug where file and folder names are native encoded before being
URL encoded to add to the list. The native encoding is can not represent
many unicode codepoints and is therefore lossy.

The new `detailed-utf8` output kind encodes file and folder names as utf8 before
URL encoding them. This allows the names to be decoded via
`textDecode(URLDecode(<name>), "utf8")`.
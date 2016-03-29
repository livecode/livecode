# Replacing text in fields

There is a new form of the **replace** command which works directly with fields:

    replace <pattern> with <text> in <field chunk> (replacing | preserving) styles

The `replacing` form of the command replaces each occurrence of 'pattern' in the specified field chunk with 'text', removing all styling from the found range.

The `preserving` form of the command replaces each occurrence of 'pattern' in the specified field chunk with 'text', retaining the style which was present on the first char of the occurrence of the pattern for the whole of the replacement.

**This feature was sponsored by the community Feature Exchange.**

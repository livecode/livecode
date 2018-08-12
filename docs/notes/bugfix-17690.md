# Fix truncation when saving field paragraphs where length exceeds 32767 characters.

Note: This fix introduces a new stack file format version (8.1) which is required to preserve the paragraph text. Saving with a legacy stack file version will result in loss of data for field text affected by this bug.

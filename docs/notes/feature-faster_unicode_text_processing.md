# Faster Unicode text processing

By using its own implementation of the Unicode grapheme cluster breaking
algorithm, the engine can now handle text processing with non-native 
strings about 25% faster, as it no longer needs to incur the penalty of
initialising an ICU grapheme break iterator which involved a complete
copy of the target string.
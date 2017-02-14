# Ensure matchText and replaceText don't affect target string

Previously calling matchText or replaceText on a string would
cause subsequent uses of that string to use slower codepaths
causing unexpected performance degredation.

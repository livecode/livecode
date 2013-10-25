# Flip does not work on referenced images.
The flip command will now work on referenced images.
Note that at the moment this behavior is the same as pre-6.0 where doing 'flip' on a referenced image would work, but would not be persistent and not interact well with other operations. This behavior will be improved in a subsequent release when image transformation abilities are improved.

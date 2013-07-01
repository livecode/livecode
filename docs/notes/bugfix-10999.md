# Crash when using 'flip' on a referenced image.
Previously attempting to use the 'flip' command on a referenced image (image with filename) would cause a crash. Attempting to use the flip command in this way now will cause an error 'object is not an editable image'.

Note: Prior to 6.0, the flip command would work on a referenced image but only temporarily. The new behavior is more consistent with the other image editing operations (such as rotate) and the ability to transform referenced images will be introduced in a subsequent version.

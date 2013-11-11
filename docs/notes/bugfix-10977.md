# Setting the image filename to empty unsets the image text and vice-versa

Previously, setting the filename of an image where it's filename was already empty would cause the text to be unset and vice-versa. This goes against the idea that setting a property to its existing value should have no effect, and is inconsistent with similar property pairs such as foreColor/forePattern.

Now, if you attempt to set the filename to empty when it is already empty, the action will have no effect. Similarly if you attempt to set the text to empty when it is already empty, it will have no effect.

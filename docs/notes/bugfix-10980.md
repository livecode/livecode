# Setting the filename of an image which already has a filename causes the property to be unset and 'could not load image' in the result.
Previously setting the filename of image which had a non-empty filename property would cause the property to be unset and an error in the result. Since the purpose of setting the filename to empty is to clear the reference, this behavior has been changed so no error is generated (in the result) in this case.


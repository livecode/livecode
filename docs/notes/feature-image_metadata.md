# Image metadata
A new read only image property has been added to access the metadata in the image file. The returned array is in the same format as that used for the export command. If no metadata is found then the property returns empty rather than an array with empty elements. Currently the only metadata key that is implemented is `density` which can be used to determine pixel density in pixels per inch. Metadata is currently only parsed from JPEG and PNG file formats.

For example:

    put the metadata of image 1 into metadataArray
	set the width of image 1 to the width of image 1 div (medatadaArray["density"] / 72)
	set the height of image 1 to the height of image 1 div (medatadaArray["density"] / 72)




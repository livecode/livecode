---
version: 8.0.0-dp-5
---
# LiveCode Builder Host Library
## Canvas library

* You can now create a new canvas `Effect` object without setting up
  an array of properties.  For example:

      variable tEffect as Effect
      put outer shadow effect into tEffect

  Default values will be assumed for unspecified properties:

  * **size**: 5
  * **spread**: 0
  * **distance**: 5
  * **angle**: 60

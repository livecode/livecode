# LCB: graphic effect properties not supported by Canvas library

The Canvas library now supports the *knockout* property of outer shadow effects, and the *source* property of inner glow effects.

* *knockout* (boolean) : Controls whether or not the alpha channel of the source image is applied to the blurred shadow created by the effect. Defaults to *true*.
* *source* (string) : One of "center" or "edge". Determines the location from which the glow originates. Defaults to "edge".

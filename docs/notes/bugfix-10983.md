# Images with filenames that look like URLs can cause slowdown.
Previously, if an image was set with a filename that roughly looked like a URL, it would cause an attempt to load the URL repeatedly. This could cause slowdown or, in the worse case, an infinite recursive loop.
Now, the engine checks filenames more closely to see if they really could be URLs before attempting a fetch, and if a fetch is attempted it will only occur once and not retried if it fails.


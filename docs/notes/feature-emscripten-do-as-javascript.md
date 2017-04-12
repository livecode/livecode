# Calling JavaScript from HTML5

JavaScript has been added to the **alternateLanguages** on the HTML5
platform.

It is now possible to call JavaScript code from HTML5 standalones by
using the `do <script> as <alternateLanguage>` form of the **do** command.

This allows HTML5 standalones to interact with the browser within which
they are running. The value of the JavaScript expression will be placed
in the **result** variable:

```
local tDocTitle
do "document.title" as "JavaScript"
put the result into tDocTitle
```

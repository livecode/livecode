# '#!' now recognised by server
When passing a file to the server engine on the command line, if the first line starts with '#!' it will be treated as a plain livecode script file. This means that '<?' type tags are not treated as blocks of code.

For example the following two are equivalent:

  #! /usr/bin/livecode-server
  put "Hello World!"

and

  <?lc
  put "Hello World!"
  ?>

In the former case, the whole file is treated as script. In the latter case, only text within the '<?'/'?>' sections are.


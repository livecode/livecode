# Community standalone engine stack processing
The community standalone engine has been updated to allow it to run with a stack passed on the command-line. If the standalone engine is invoked without having been built into a standalone, it will take the first non-option parameter (i.e. a parameter beginning with '-') to be a stackfile and will load and open that.

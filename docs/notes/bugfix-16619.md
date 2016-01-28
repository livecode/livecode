# No blocking syntax in HTML5 standalones

In LiveCode 8.0.0-dp-11, changes were made to the HTML5 standalone
engine to enable the use of the **wait** command (and related syntax;
see [bug 16076](http://quality.livecode.com/show_bug.cgi?id/16076).
These changes made the HTML5 engine run unacceptably slowly, and have
now been removed.

Instead of using **wait** in stacks designed for HTML5 deployment, you
can still use `send <message> in <time>`.

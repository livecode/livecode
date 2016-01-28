# No "wait" syntax in HTML5 standalones

In LiveCode 8.0.0-dp-11, changes were made to the HTML5 standalone
engine to enable the use of `wait` (and related syntax; see
[bug 16076](http://quality.livecode.com/show_bug.cgi?id/16076).  These
changes made the HTML5 engine run unacceptably slowly, and have now
been removed.

Instead of using `wait` in stacks for HTML5 deployment, the use of the
`send <message> in <time>` syntax is recommended.

# Creating multiple 'windows' in HTML5

Basic support for multiple windows has been added to the HTML5 engine. The windows are implemented as canvas elements within the HTML5 page.
This allows tooltips, dialogs, and pop-up menus to work within the HTML5 engine.

This also allows multiple stacks to be opened on the HTML5 page. Stacks other than the main stack will display in windows layered above the rest of the page.
Note that these windows do not have a titlebar or standard window controls, so the means to move, resize, or close stacks will need to be provided within the stack UI.

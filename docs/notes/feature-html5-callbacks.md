---
version: 9.0.0-dp-8
---
# HTML5 Callbacks - enable calling handlers in LiveCode emscripten standalones from JavaScript

**Note** This is still an experimental feature - details may change as development continues.
 
## Stack setup
 
Each stack can be configured to expose its handlers & functions to JavaScript. This is done through a custom property of the stack - `cJavascriptHandlers`. (This will be replaced in the future by a `javascriptHandlers` property which will then be a reserved keyword). The `cJavascriptHandlers` property is a return-delimited list of handler names. The named handlers do not have to be defined at the time the stack is loaded, however calling an undefined handler from JavaScript will result in an error.
 
 
## Calling from JavaScript
 
The standalone engine will create a `liveCode` object on the DOM `document` object. To this object will be attached the `findStackWithName` method that can be called to return a JavaScript stack object. Each stack object will have methods corresponding to the exposed handlers of that stack. For instance, a stack with the `cJavascriptHandlers` property set to :
 
    performAction
    setProperty
    getProperty

will have methods named accordingly, which when executed will call the corresponding handler with the provided arguments.
 
## JavaScript Example:
 
    var myStack = document.liveCode.findStackWithName(“HTMLTest”);
    var oldDocTitle = myStack.getProperty(‘documentTitle’);
    myStack.setProperty(‘documentTitle’, ‘Important Document’);
    myStack.performAction(‘sendEmail’);

# Android Native Field Widget

A new widget has been added which is a native field view on Android. On
all other platforms it presents a placeholder image with the Android 
logo. The widget is a wrapper around the Android EditText class. The 
implementation is based on the 'input' and 'multiline input' native 
controls on Android, but removes the need for mobileControlCreate / 
mobileControlSet syntax. The widget can be dragged from the tools 
palette and have properties set on it in the usual way. Moreover the
widget's object is the target of its messages, rather than the object
whose script created the control.

## Properties

* **enabled** - The enabled state of the field
* **editable** - Whether the field contents can be edited
* **fieldTextColor** - The color of the field text
* **textSize** - The size of the field text
* **multiline** - Whether the field can contain multiple lines of text
* **scrollingEnabled** - Whether the field contents can be scrolled
* **text** - The text contents of the field
* **textAlign** - The (horizontal) text alignment of the field
* **verticalTextAlign** - The (horizontal) text alignment of the field
* **autoCapitalizationType** - The auto-capitalization behavior of the field
* **autoCorrectionType** - The auto-correct behavior of the field
* **contentType** - Whether the field content is plain text or password
* **dataDetectorTypes** - The types of field data that are displayed as URLs
* **keyboardType** - The type of keyboard to display when the field has focus
* **returnKeyType** - The action associated with the return key

## Non-persistent properties

* **selectedRange** - The start and length of the field text selection
* **focused** - Whether the field is focused or not

## Messages

* **openField** - sent when the field gains focus
* **closeField** - sent when the field loses focus with its content having changed
* **exitField** - sent when the field loses focus with no change in content
* **returnKey** - sent when the return key is pressed

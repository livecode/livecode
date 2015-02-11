# File format change

In order to accommodate the saving and loading of unicode text throughout LiveCode, the file format of stacks has been changed. This means that stacks saved in 7.0 format cannot be opened in previous versions of LiveCode.

Legacy file formats are available to select when using the Save As... dialog. Saving in a legacy format will result in the loss of some information related to LiveCode 7.0, namely Unicode text in some areas (for example in object scripts), right-to-left formatting and tab alignment.
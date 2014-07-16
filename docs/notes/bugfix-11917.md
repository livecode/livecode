# Setting the label of an option or combo-box does not update the menuHistory.
Previously, setting the label of an option or combo-box control would not update the menuHistory property.
Now, setting the label of such a control will search through the list of items in the control and set the menuHistory to the first item that matches (taking into account the setting of the caseSensitive local property).
Note: Unlike setting the menuHistory property direct, this does not cause a menuPick message to be sent.

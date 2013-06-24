# Locking Group Updates
When a control within a group is resized or moved it causes all parents to recalculate their bounds appropriately. This action can now be temporarily suspended by using 'the lockUpdates' property of the group.

Typically, you'll want to set the lockUpdates of a group to true before performing moving or resizing child controls, and then turn it back to false afterwards. e.g.
`on updateGroupedControls
  set the lockUpdates of me to true
  set the left of button 1 of me to the left of me + 10
  set the bottom of button 2 of me to the bottom of me - 10
  set the lockUpdates of me to false
end updateGroupedControls`
**Note**: The lockUpdates property does not nest, and the group will not resize itself appropriately based on the child controls until it is set back to false.
# Queuing too many pending messages causes slowdown and random crashes.
A limit on the number of user-defined pending messages (those created with 'send in time') has been imposed. If there are more than 64k messages in the pending message queue, 'send in time' will now throw an error when attempting to queue another one.
This limit has been imposed to prevent engine lock up and eventual instability due to memory exhaustion in the case that pending message loops cause rapid increases in the number of pending messages.

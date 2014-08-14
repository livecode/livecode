# Focus gets confused if focus changes in response to a suspendStack message on Mac
Attempts to change window focus inside a suspendStack or resumeStack handler will not be honored on Mac - this is due to an apparant issue with Cocoa when attempting to change window focus in these circumstances.

# Offline activation fails in Ubuntu 10.04, 10.10, 12.04

The installer now does not offer to launch LiveCode after installation as root (e.g. via su or sudo) on Linux in order to prevent the product from creating its activation files with the wrong permissions. Instead, LiveCode should be launched in the normal way after installation and activation occurs then.


# Installer no longer required on OSX

The installation process on OSX has been updated to no longer require an installer. Instead, LiveCode now installs like most other OSX applications: drag the app from the DMG to your Applications folder.

Similarly, there is no longer an uninstaller; drag the app to the Trash to remove.

The IDE app bundle is now signed, which means it can now be used in certain situations that require verification using Apple's GateKeeper technology. As such, you should not modify the contents of the app bundle (for example, editing the IDE stacks) if you use the IDE for purposes that require verification as the signature will no longer be valid.

# Application Transport Security (ATS)

Apple introduced in iOS SDK 9.0 the Application Transport Security which 'enforces best practices in the secure connections between an app and its back end' (see [the technical notice](https://developer.apple.com/library/prerelease/ios/releasenotes/General/WhatsNewIniOS/Articles/iOS9.html#//apple_ref/doc/uid/TP40016198-SW14)).

The most noticeable effect on applications created using Xcode 7.0 (and following versions) is that URLs using *HTTP* protocol are no longer considered valid, and the iOS engine will not process them; only URLs using *HTTPS* are deemed secure enough, and will be loaded.

This means that `nativeBrowser` cannot load a webpage such as [http LiveCode](http://www.livecode.com), but will happily load [https LiveCode](https://www.livecode.com). The same applies to the LiveCode function *url*.

To allow our users to create apps letting Web navigation accept unsecure webpages, we added a checkbox **Disable ATS** in the Standalone Settings for iOS, in the Requirements and Restrictions section. If you check this box, then *ATS* will be disabled, and the application can load Webpages using *HTTP* (as it used to do).

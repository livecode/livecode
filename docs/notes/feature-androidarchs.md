# New Android Architectures

Android builds now support four architectures. Previously android was built
for `armv6` only. Android is now built for `armv7`, `arm64`, `x86` and `x86_64`.

Checkboxes are included on in the Android standalone settings to choose which
architectures to include in the build. When deploying your application via the
Test button to a device the device architecture will be detected and used even
if not chosen in standalone settings.
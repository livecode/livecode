# URL operations sometimes fail on Android
There is a bug in the Android Java HTTP layer which can cause URL operations to fail (https://code.google.com/p/google-http-java-client/issues/detail?id=116). This problem seems to be related to keep-alive connections, thus until this bug is addressed in Android the engine will now turn off keep-alive on startup.

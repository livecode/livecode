# Add param that suppresses success message when building standalone

By default `revSaveAsStandalone` displays the message `answer information "Standalone application saved successfully."` when it is done. You can turn off this message by setting the test environment to true but doing so suppresses all error messages and other feedback as well.

I am calling `revSaveAsStandalone` from my own scripts multiple times and want feedback and error reporting but not the success message. Adding an additional parameter to `revSaveAsStandalone` that suppress the success message would allow this.
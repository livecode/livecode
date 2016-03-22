# Ensure unload extension reports appropriate status in 'the result'

If 'unload extension' is called on a module which has not been loaded,
the result will be set to "module not loaded".

If 'unload extension' is called on a module which is currently in use,
the result will be set to "module in use".

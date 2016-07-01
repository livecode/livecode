# LiveCode Builder Language
## Unsafe Attributes

* The compiler now understands the idea of 'safety' of handlers and blocks of
  code.

* Handlers can be marked as being 'unsafe', e.g.

      unsafe handler Foo()
          ... do unsafe things ...
      end handler

* Blocks of statements can be marked as being 'unsafe', e.g.

      unsafe
          ... do unsafe things ...
      end unsafe

* All foreign handlers are considered to be 'unsafe'.

* All bytecode blocks are considered to be 'unsafe'.

* Calls to foreign handlers and unsafe handlers can only be made within unsafe
  handlers or unsafe statement blocks.

* Usage of bytecode blocks can only be made within unsafe handlers or unsafe
  statement blocks.

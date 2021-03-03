## Userspace testing of the ASPEED HACE engine

This runs the HACE u-boot code from the ASPEED SDK in userspace. You must be
running a kernel with `CONFIG_STRICT_DEVMEM` turned off.

It contains many bad ideas that you should not copy.

### Testing

The expected output is

```
2338705c3d1dd43a6e06c7dacace1a929a7ceec44473af1c7b3700df891cdd803c8cd85f9ebbb88c1b55d52a89b6a7feeb18d6a1dca05d2d39ace7d90928a5fb
```

You can verify this on your machine:

```
$ echo -n -e '\xde\xad\xbe\xef\xde\xad\xbe\xef' | dd of=/tmp/test
$ sha512sum /tmp/test
2338705c3d1dd43a6e06c7dacace1a929a7ceec44473af1c7b3700df891cdd803c8cd85f9ebbb88c1b55d52a89b6a7feeb18d6a1dca05d2d39ace7d90928a5fb  /tmp/test
```

### Licence

The `aspeed_hace.c` and `aspeed_hace.h` files come from ASPEED's SDK, and are
licenced under the GPL v2.0 or later.

All other code is licenced under the Apache Licence, version 2.0.

Copyright 2021 IBM Corp.

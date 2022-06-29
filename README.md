# pass-cache

A simple daemon that caches the response from `pass my_entry`.
For use with software that makes frequent calls to `pass`.

## build

```
gcc passcached.c -o passcached
gcc pass-cache.c -o pass-cache
```

## usage

Start `passcached` in the background.
Use `pass-cache` instead of `pass` to retrieve a secret.
You will only need to unlock your keyring for the first invocation of `pass-cache` for a given entry; subsequent requests are served from the cache.
The socket used for communication defaults to `$XDG_RUNTIME_DIR/pass-cache.sock`; this can be manually set with `PASS_CACHE_SOCK`.

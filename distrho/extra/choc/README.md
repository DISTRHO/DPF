Taken from https://github.com/Tracktion/choc

```
commit 2512542b2d65f3e92df7f2f1f7eeb712fa41a0de (HEAD -> main, origin/main, origin/HEAD)
Author: Cesare Ferrari <cesare.ferrari@gmail.com>
Date:   Sun Apr 28 12:53:17 2024 +0100

    Disable additional gcc warnin
```

With the big [choc.patch](./choc.patch) patch applied to top for:

- remove everything not related to windows
- remove everything unused by DPF
- convert webview JS callbacks to pass raw strings instead of json
- remove even more stuff (json no longer needed)
- convert choc asserts into distrho ones
- put everything inside distrho namespace

And then backported:

- https://github.com/Tracktion/choc/commit/792e4bd9bedf38b9a28f12be0535c90649d5616b

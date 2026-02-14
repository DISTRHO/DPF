Taken from https://github.com/Tracktion/choc

```
commit ae4c54d22b53a599222c1dfaa4b4007d0ec310c7 (HEAD -> main, origin/main, origin/HEAD)
Author: Julian Storer <julianstorer@gmail.com>
Date:   Thu Jan 15 08:56:50 2026 +0000

    Added a zip file creation class, ZipWriter
```

With the big [choc.patch](./choc.patch) patch applied to top for:

- remove everything not related to windows
- remove everything unused by DPF
- convert webview JS callbacks to pass raw strings instead of json
- remove even more stuff (json no longer needed)
- convert choc asserts into distrho ones
- put everything inside distrho namespace

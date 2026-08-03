These 2 files were downloaded from:
https://www.khronos.org/registry/OpenGL/api/GL/glext.h
https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h

They used to be downloaded on demand through cmake if compiler is MSVC.
Now they are stored directly in DPF because download through cmake no longer works.

As a side effect this allows to build against MSVC while offline.

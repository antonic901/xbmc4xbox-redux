# Xbox libcurl

Xbox port of libcurl. This version of libcurl is built as a
DLL file, and linked against MSVCRT from VC7.

## Limitations

- Anything higher than version `7.76.1` will require substantial changes to XBMC's DLL loader.
- TLS v1.3 doesn't work.

## Building

There are two ways to build the project, both are equally supported. Nevertheless, the `Clang` version is what's shipped with XBMC by default, when available.

### Building with .NET 2003

For this option, you will need:

- .NET 2003
- A Windows machine. Oldest version supported is Windows XP, newest tested is Windows 11.
- A i386 based processor (x64 works).

To build libcurl, wolfssl must be built first. Navigate to `../../../wolfssl/xbox`, open `wolfssl.sln` and build the library with the desired profile.

Finally, navigate back to libcurl build folder, open `libcurl.sln` and build the solution with desired profile.

### Building with Clang

To build with Clang, the following build tools are required:

- Full LLVM suite. (`clang`, `clang++`, `lld-link`, `llvm-lib` are used).
- Python >=3.9 and [SCons](https://pypi.org/project/SCons/)

These can be acquired in both Windows and Linux. In Windows, use MSYS2 MINGW64, and acquire them through `pacman`. On Linux, use your distro package manager.

The following build dependencies are needed:

- VC7.1 (or VC7) development libraries (Can be obtained from .NET 2003).
- Windows XP era Platform SDK. Can be had through `VC7 Platform SDK` (included in .NET 2003) or `Windows 2003 Platform SDK`, the latter is free and can be obtained directly from Microsoft, or from Archive.org.

Both of these dependencies must be placed in the same folder. We will call this folder `LSDK`. If W2003 Platform SDK is used, rename to `VC7_PSDK`. The resulting folder should look like this:

```
.
└── LSDK/
    ├── VC7/
    │   ├── include
    │   └── lib
    └── VC7_PSDK/
        ├── include
        └── lib
```

**If building on Linux, the `LSDK` folder must be mounted in a NTFS drive, or through a loop image: `lowntfs-3g -o loop,windows_names,ignore_case lsdk.img ./lsdk`**

Then, set environment variable `LSDK` to the created `LSDK` folder. Example: `export LSDK=/home/root/LSDK`

Finally, to build the project, run:

```
scons
```

Most projects can be compiled with multiple jobs, significantly reducing compilation times:

```
scons -j 4
```

This will build both wolfssl and libcurl.
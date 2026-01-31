# llvm-factory

In the past I've encountered situations where I needed various LLVM versions for multiple projects, with varying configurations and build settings.

Often, the official releases aren't enough for my purposes (for example, on Windows you don't get `llvm-config`), so over time I created my own script for building and packaging LLVM.

The original Makefile I used is added to this repo, however I have since then ported the script to C. This allows for more flexibility than Makefiles.

## Requirements
- `git` to clone the LLVM repository
- `cmake` to build LLVM
- `7z` to archive LLVM
- `ninja` to build LLVM
    - if you don't have this installed, change `"-G", "Ninja"` in [nob.c](./nob.c) to anything you want.
- `lld` to build LLVM
    - if you don't have this installed, remove `"-DLLVM_USE_LINKER=lld"` in [nob.c](./nob.c).
- `ccache`
    - if you don't have this installed, change `"-DLLVM_CCACHE_BUILD=ON"` in [nob.c](./nob.c) to `"-DLLVM_CCACHE_BUILD=OFF"`.

## Build Step
Simply compile [nob.c](./nob.c) with your favorite C compiler. It will then do the rest for you. After you change your configuration, it will automatically rebuild itself too.

## Configuration
By default, the script doesn't do anything. Building LLVM takes a long time, and you may want to first select the correct LLVM version, and make sure you're building a Release version etc.

For that purpose, the script by default does a dry run, only printing the commands it *would* execute, without actually executing them.

This lets you modify the configuration, which is also specified in [nob.c](./nob.c). The lines to look out for are:
```c
const char *LLVM[] = { ... };
const char *CONFIGS[] = { ... };
Link_Mode MODES[] = { ... };
```

Once you're done setting the configuration, comment out `#define DRY_RUN` and go to bed or hang out with some friends. I'd not recommend using the machine for anything else for the next few hours (or days, depending on your setup).

Do note that the current script will lead to a combinatorical explosion: If you specify 3 LLVM versions to build, with 2 configurations (for example, Release and Debug), and build both static and dynamic versions, you will compile LLVM 3x2x2=12 times.

This configuration step may be subject to change.

## Output
This script will build LLVM, MLIR, lldb, clang and lld. After that, it will create an archive using 7zip.

The archive will be named in the format `llvm-VERSION-CONFIG-LINK_MODE.7z`, for example `llvm-21.1.8-Release-Dynamic.7z`.

The installed build can be found in a directory with the same name, for example `./llvm-21.1.8-Release-Dynamic/`.

## Cache
Intermediate steps of the pipeline will be cached. Those steps are: Cloning the repository, configuring CMake, building LLVM, and running 7z.

This means that you don't have to run the script until completion, and may cancel it after those points.

Check files will be stored in directories starting with `chk-`, for example `./chk-llvm-21.1.8-Release-Dynamic/`.

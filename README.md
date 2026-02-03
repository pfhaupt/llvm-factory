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
Simply compile [nob.c](./nob.c) with your favorite C compiler, for example `clang nob.c -o nob`. It will then do the rest for you. After you change your configuration, it will automatically rebuild itself too.

## Configuration
By default, the script doesn't do anything. Building LLVM takes a long time, and you may want to first select the correct LLVM version, and make sure you're building a Release version etc.

For that purpose, the script by default does a dry run, only printing the commands it *would* execute, without actually executing them.

This lets you modify the configuration, which is also specified in [nob.c](./nob.c). The line to look out for is:
```c
const Config CONFIGS[] = {...};
```

Once you're done setting the configuration, comment out `#define DRY_RUN` and go to bed or hang out with some friends. I'd not recommend using the machine for anything else for the next few hours (or days, depending on your setup).

## Output
The script will build each specified configuration, and create an archive of each using 7zip.

The archive will be named in the format `PROJECTS-VERSION-CONFIG-LINK_MODE.7z`, for example `llvm-mlir-21.1.8-Release-Dynamic.7z`.

The installed build can be found in a directory with the same name, for example `./llvm-mlir-21.1.8-Release-Dynamic/`.

## Cache
Intermediate steps of the pipeline will be cached. Those steps are: Cloning the repository, configuring CMake, building LLVM, and running 7z.

This means that you don't have to run the script until completion, and may cancel it after those points.

The script creates check files in directories starting with `chk-`, for example `./chk-llvm-mlir-21.1.8-Release-Dynamic/`, delete them if you want a full rebuild.

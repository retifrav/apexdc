# ApexDC++

This is a modified mirror/fork of [ApexDC++](https://apexdc.net) project sources. It isn't exactly a mirror, because it's not a 1-to-1 copy of the original source, but it isn't exactly a fork either, because core source code is the same.

<!-- MarkdownTOC -->

- [Why mirror/fork](#why-mirrorfork)
- [Syncing with original sources](#syncing-with-original-sources)
- [How to build](#how-to-build)
    - [With CMake](#with-cmake)
        - [ARM is not supported](#arm-is-not-supported)
    - [With Visual Studio / MSBuild](#with-visual-studio--msbuild)
        - [Spectre mitigations](#spectre-mitigations)
        - [Version control definitions](#version-control-definitions)
- [Things to do](#things-to-do)
- [License](#license)

<!-- /MarkdownTOC -->

## Why mirror/fork

The point of creating this repository was to:

- get ApexDC++ sources under Git version control, as I failed to find a publicly available repository;
- make it build with CMake (*and probably drop Visual Studio / MSBuild support*);
- stop vendoring dependencies and resolve them with vcpkg instead;
- try to improve/implement High DPI support, so UI wouldn't be so blurry;
- customize some default settings such as font size, colors, etc.

The original sources are obtained from [release archives](https://sourceforge.net/projects/apexdc/files/ApexDC%2B%2B/).

## Syncing with original sources

Before syncing with a newer version from upstream/original sources, first remove executable attribute from the new files:

``` sh
$ find . -type f -print0 | xargs -0 chmod 0664
```

and then convert line endings to Unix style:

``` sh
$ find . -type f -print0 | xargs -0 dos2unix
```

## How to build

### With CMake

Required tools:

- CMake
- vcpkg
- MSVC

``` sh
$ cd /path/to/apexdc
$ cmake --preset vcpkg-windows
$ cmake --build --preset vcpkg-windows
```

#### ARM is not supported

Can't built for ARM:

``` bat
[1/217] Building CXX object CMakeFiles\apexdc.dir\client\BufferedSocket.cpp.obj
FAILED: CMakeFiles/apexdc.dir/client/BufferedSocket.cpp.obj
C:\programs\vs\vs2022\VC\Tools\MSVC\14.37.32822\bin\Hostx64\arm64\cl.exe  /nologo /TP -DUNICODE -D_UNICODE -IC:\programs\_src\apexdc\boost -external:IC:\programs\_src\apexdc\build\vcpkg-default-triplet\vcpkg_installed\arm64-windows-static-md\include -external:W0 /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -MD /showIncludes /FoCMakeFiles\apexdc.dir\client\BufferedSocket.cpp.obj /FdCMakeFiles\apexdc.dir\ /FS -c C:\programs\_src\apexdc\client\BufferedSocket.cpp
C:\programs\_src\apexdc\boost\boost/thread/win32/interlocked_read.hpp(90): error C2440: 'initializing': cannot convert from '__int64' to 'void *'
C:\programs\_src\apexdc\boost\boost/thread/win32/interlocked_read.hpp(90): note: Conversion from integral type to pointer type requires reinterpret_cast, C-style cast or parenthesized function-style cast
```

Choose x64 target/configuration instead:

- with Ninja generator it is enough to launch x64 Native Tools Command Prompt;
- with Visual Studio generator you'll also need to pass `-A x64`.

### With Visual Studio / MSBuild

If you'd like to build the project with Visual Studio / MSBuild, you'd need to checkout the `fee6571e462df94a72157d2b2a04aab8a1073ba5` revision with original sources. The build almost just works out of the box, only need to fix a [couple of things](#version-control-definitions) in MakeDefs subproject.

Required tools:

- Visual Studio
    + ATL
    + MSVC

#### Spectre mitigations

Both ATL and MSVC components should be with Spectre mitigations, otherwise you'll get these errors:

``` bat
4>C:\programs\vs\vs2022\MSBuild\Microsoft\VC\v170\Microsoft.CppBuild.targets(504,5): error MSB8040: Spectre-mitigated libraries are required for this project. Install them from the Visual Studio installer (Individual components tab) for any toolsets and architectures being used. Learn more: https://aka.ms/Ofhn4c
4>Done building project "StrongDC.vcxproj" -- FAILED.
```

``` bat
3>Could Not Find C:\programs\_src\apexdc\compiled\ApexDC-x64.pdb
3>LINK : fatal error LNK1104: cannot open file 'atls.lib'

install ATL with Spectre mitigations
```

So install them via Visual Studio Installer:

![Visual Studio installer MSVC with Spectre mitigations](https://raw.githubusercontent.com/retifrav/apexdc/master/misc/visual-studio-installer-msvc-spectre.png "Visual Studio installer MSVC with Spectre mitigations")

![Visual Studio installer ATL with Spectre mitigations](https://raw.githubusercontent.com/retifrav/apexdc/master/misc/visual-studio-installer-atl-spectre.png "Visual Studio installer ATL with Spectre mitigations")

#### Version control definitions

Since original Git tagging system isn't figured out yet, create a dummy `client/gitdefs.inc`:

``` cpp
#define NO_VERSION_CONTROL 1
```

and disable Pre-Build Event in `MakeDefs` project properties:

![Visual Studio, MakeDefs properties, removed Pre-Build Event](https://raw.githubusercontent.com/retifrav/apexdc/master/misc/visual-studio-makedefs-properties-pre-build-event.png "Visual Studio, MakeDefs properties, removed Pre-Build Event")

## Things to do

1. [ ] implement building with CMake
2. [ ] stop vendoring dependencies, use vcpkg instead
    - [ ] [boost](https://boost.org)
    - [x] [bzip2](https://sourceware.org/bzip2/)
    - [ ] [MiniUPnP](http://miniupnp.free.fr)
    - [ ] [natpmp](http://miniupnp.free.fr/libnatpmp.html)
    - [x] [OpenSSL](https://openssl.org)
    - [x] [WTL](https://sourceforge.net/projects/wtl/)
    - [x] [zlib](http://zlib.net)

## License

Original sources are licensed under GPLv2, and if it is possible, I'd like to license this repository under GPLv3. If not, then I'll revert to GPLv2.

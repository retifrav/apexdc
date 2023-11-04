# ApexDC++

This is a modified mirror/fork of [ApexDC++](https://apexdc.net) project sources. It isn't exactly a mirror, because it's not a 1-to-1 copy of the original source, but it isn't exactly a fork either, because core source code is the same.

<!-- MarkdownTOC -->

- [Why mirror/fork](#why-mirrorfork)
- [Syncing with original sources](#syncing-with-original-sources)
- [How to build](#how-to-build)
    - [With CMake](#with-cmake)
    - [With Visual Studio / MSBuild](#with-visual-studio--msbuild)
- [Roadmap / ToDo](#roadmap--todo)
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

No instructions yet.

### With Visual Studio / MSBuild

If you'd like to build the project with Visual Studio / MSBuild, you'd need to checkout the `542bbc6a9761ca9261198ec82d3105141e8ac21d` revision with original sources.

## Roadmap / ToDo

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

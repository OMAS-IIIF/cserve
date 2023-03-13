# cserve
A C++ based, extendable multithreaded  web server with embeded LUA scripting.

## Building

There are two builing options:

- local building. Supported are currently
  - OS X Big Sur, Monterey
  - Ubuntu 20.04 LTE
- building in docker

The sipi image is built as statically linked executable. All necessary libraries
are being downloaded and built during the build-process of sipi. Therefore an
internetconnection is necessary and a full build will take it's time.
Cserve needs the following external libraries:

- bz2 (https://sourceware.org/git/bzip2.git)
- Catch2 (https://github.com/catchorg/Catch2/archive/refs/heads/devel.zip)
- crypto (https://www.openssl.org/source/openssl-3.0.0.tar.gz)
- curl (https://curl.haxx.se/download/curl-7.80.0.tar.gz)
- fmt (https://github.com/fmtlib/fmt.git)
- lua (http://www.lua.org/ftp/lua-5.4.3.tar.gz)
- lzma (https://tukaani.org/xz/xz-5.2.5.tar.gz)
- magic (http://ftp.astron.com/pub/file/file-5.41.tar.gz)
- spdlog (https://github.com/gabime/spdlog/archive/refs/heads/v2.x.zip)
- sqlite3 (https://www.sqlite.org/2021/sqlite-autoconf-3360000.tar.gz)
- ssl (https://www.openssl.org/source/openssl-3.0.0.tar.gz)
- z (http://zlib.net/zlib-1.2.11.tar.gz)

### Local building

#### Prerequisites

The following packages have to be install locally:

- On OS X: `brew install inih` (required by exiv2)

- python > 3.9
- for testing (with pip3):
  - pytest
  - psutil
  - requests
  - pyJWT
  - pillow

The following steps have to be made:

- ```mkdir build```
- ```cd build```
- ```cmake ..```
- ```make```
- ```make install```

For testing, the command (in build directory) performs all the tests:

- ``` make test```





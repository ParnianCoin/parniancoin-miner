# parnianminer - Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com>
# Windows 32 bit cross-compile toolchain (mingw-w64, posix threads)
# usage: cmake .. -DCMAKE_TOOLCHAIN_FILE=../CMake/toolchain-mingw-w32.cmake -DRH_CPU_ONLY=ON -DBOOST_ROOT=/opt/boost-w32
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)
set(CMAKE_C_COMPILER   i686-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++-posix)
set(CMAKE_RC_COMPILER  i686-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

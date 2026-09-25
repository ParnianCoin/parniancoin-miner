# parnianminer - Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com>
# Windows 64 bit cross-compile toolchain (mingw-w64, posix threads)
# usage: cmake .. -DCMAKE_TOOLCHAIN_FILE=../CMake/toolchain-mingw-w64.cmake -DRH_CPU_ONLY=ON -DBOOST_ROOT=/opt/boost-w64
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

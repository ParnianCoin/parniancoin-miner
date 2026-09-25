# parnianminer - Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com>
# Portable Linux x64 build with zig (pip install ziglang).
# Targets glibc 2.17, so the binary runs on almost every Linux distribution
# (Ubuntu 16.04+, Debian 9+, CentOS 7+). The C++ runtime is linked statically.
# ZIG_WRAPPERS must contain zcc / zcxx / zar / zranlib scripts that call
#   python3 -m ziglang cc|c++ -target x86_64-linux-gnu.2.17 "$@"
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
if(NOT ZIG_WRAPPERS)
    set(ZIG_WRAPPERS $ENV{ZIG_WRAPPERS})
endif()
set(CMAKE_C_COMPILER   ${ZIG_WRAPPERS}/zcc)
set(CMAKE_CXX_COMPILER ${ZIG_WRAPPERS}/zcxx)
set(CMAKE_AR           ${ZIG_WRAPPERS}/zar CACHE FILEPATH "")
set(CMAKE_RANLIB       ${ZIG_WRAPPERS}/zranlib CACHE FILEPATH "")
set(CMAKE_CXX_FLAGS_INIT "-D_GNU_SOURCE -w")
set(CMAKE_C_FLAGS_INIT "-D_GNU_SOURCE -w")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-s")

# parnianminer
# Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com>
# https://parniancoin.com

cmake_minimum_required(VERSION 2.8)

option(RH_DEBUG_TARGET "Compile in Debug" OFF)
option(RH_CPU_ONLY "Compile only cpu code" OFF)
option(RH_ARCH "Architecture name" Maxwell)
option(RH_NO_SSE4 "Disable sse4 optimizations" OFF)

project(parnianminer)
set(RH_ARCH CPU)

set(CMAKE_CXX_STANDARD 11)

if(RH_NO_SSE4)
	message(STATUS "Disabling SSe4 intrinsics")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -mssse3 ")
	set(RH_ARCH CPU_OLDGEN)
	add_definitions(-DPARNIANMINER_NO_SSE4)
else()
	message(STATUS "ENABLING SSe4 intrinsics")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -msse4.1")
endif(RH_NO_SSE4)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake")

# Compatibility with modern Boost versions
add_definitions(-DBOOST_TIMER_ENABLE_DEPRECATED)
add_definitions(-DBOOST_BIND_GLOBAL_PLACEHOLDERS)
add_definitions(-DBOOST_ALLOW_DEPRECATED_HEADERS)

# Prevent boost::asio from embedding build-machine source file paths
# (file/line/function) into exception messages shown to end users.
add_definitions(-DBOOST_ASIO_DISABLE_SOURCE_LOCATION)
add_definitions(-DBOOST_ASIO_DISABLE_STD_SOURCE_LOCATION)
add_definitions(-DBOOST_ASIO_DISABLE_STD_EXPERIMENTAL_SOURCE_LOCATION)
add_definitions(-DBOOST_ASIO_DISABLE_BOOST_SOURCE_LOCATION)

# Also strip any remaining build-machine absolute paths from __FILE__/debug info
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    string(APPEND CMAKE_CXX_FLAGS " -ffile-prefix-map=${CMAKE_SOURCE_DIR}/=")
    if(DEFINED BOOST_ROOT)
        string(APPEND CMAKE_CXX_FLAGS " -ffile-prefix-map=${BOOST_ROOT}/=boost/")
    endif()
    string(APPEND CMAKE_CXX_FLAGS " -ffile-prefix-map=/usr/include/=")
endif()

if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    set(EXTRA_LIBS ${EXTRA_LIBS} ws2_32 mswsock iphlpapi bcrypt)
endif(WIN32)

include_directories(./)
include_directories(..)
include_directories(parnianminer)

include (CheckCCompilerFlag)
include (CheckCSourceCompiles)
include (CheckIncludeFile)
include (CheckIncludeFiles)
include (CheckFunctionExists)
include (CheckSymbolExists)
include (CMake/flags.cmake)


if(RH_CPU_ONLY)
	add_definitions(-DRH_COMPILE_CPU_ONLY)
else(RH_CPU_ONLY)
    message(STATUS "Conditional SSe4")
    add_definitions(-DPARNIANMINER_COND_SSE4)
endif(RH_CPU_ONLY)

add_definitions(-DRH_ARCH=${RH_ARCH})


if(RH_DEBUG_TARGET)
	message(STATUS "Building ${PROJECT_NAME} in DEBUG")
	add_definitions( -Wall -fexceptions -Wfatal-errors -Wno-sign-compare -Wno-deprecated-declarations -Wno-unused-variable)
	add_definitions(-D_DEBUG)
	set(CMAKE_BUILD_TYPE Debug)
else(RH_DEBUG_TARGET)
	message(STATUS "Building ${PROJECT_NAME} for ${RH_ARCH} architecture in Release")
    add_definitions( -Wall -Ofast -fomit-frame-pointer -fexceptions -Wfatal-errors -Wno-sign-compare -Wno-deprecated-declarations -Wno-unused-variable)
	set(CMAKE_BUILD_TYPE Release)
	add_definitions(-DNDEBUG)
endif(RH_DEBUG_TARGET)


set(HEADERS
    parnianminer/precomp.h    
    corelib/basetypes.h
    corelib/Common.h
    corelib/CommonData.h
    corelib/rh_compiler.h
    corelib/rh_endian.h
    corelib/FixedHash.h
    corelib/Guards.h
    corelib/Log.h
    corelib/ParnianWork.h
    corelib/utils.h
    corelib/vector_ref.h
    corelib/Worker.h
    corelib/miniweb.h
    MinersLib/CLMinerBase.h
    MinersLib/CPUMiner.h
    MinersLib/CudaHostMinerProxy.h
    MinersLib/Farm.h
    MinersLib/GenericCLMiner.h
    MinersLib/GenericFarmClient.h
    MinersLib/GenericMinerClient.h
    MinersLib/Global.h
    MinersLib/GpuManager.h
    MinersLib/KernelOffsetManager.h
    MinersLib/Miner.h
    MinersLib/StratumClient.h
    MinersLib/AutoPort.h
    MinersLib/wrapadl.h
    MinersLib/wrapamdsysfs.h
    MinersLib/wraphelper.h
    MinersLib/wrapnvml.h
    MinersLib/Algo/blake2b.h
    MinersLib/Algo/blake2s.h
    MinersLib/Algo/sph-haval.h
    MinersLib/Algo/sph_blake.h
    MinersLib/Algo/sph_panama.h
    MinersLib/Algo/sph_sha0.h
    MinersLib/Algo/sph_sha1.h
    MinersLib/Algo/sph_sha2.h  
    MinersLib/Algo/sph_types.h
    MinersLib/Parnian/ParnianCommon.h
    MinersLib/Parnian/RandomHash.h
    MinersLib/Parnian/RandomHashCLMiner.h
    MinersLib/Parnian/RandomHashCPUMiner.h
    MinersLib/Parnian/RandomHashHostCudaMiner.h
    MinersLib/Parnian/RandomHash_core.h
    MinersLib/Parnian/RandomHash_def.h
    MinersLib/Parnian/RandomHash_inl.h
    MinersLib/Parnian/RandomHash_mersenne_twister.h
    MinersLib/Parnian/RandomHash_MurMur3_32.h
    parnianminer/ClientManager.h
    parnianminer/CommandLineManager.h
   )

set(SOURCES
    MinersLib/Parnian/RandomHash_Cpu.cpp
    MinersLib/Parnian/RandomHashCLMiner.cpp
    MinersLib/Parnian/RandomHashCPUMiner.cpp
    MinersLib/Parnian/RandomHashHostCudaMiner.cpp
    MinersLib/CLMinerBase.cpp
    MinersLib/Farm.cpp
    MinersLib/GenericCLMiner.cpp
    MinersLib/GenericMinerClient.cpp
    MinersLib/Global.cpp
    MinersLib/GpuManager.cpp
    MinersLib/KernelOffsetManager.cpp
    MinersLib/Miner.cpp
    MinersLib/StratumClient.cpp
    MinersLib/wrapadl.cpp
    MinersLib/wrapamdsysfs.cpp
    MinersLib/wrapnvml.cpp
    MinersLib/Algo/blake.cpp
    MinersLib/Algo/blake2b.cpp
    MinersLib/Algo/blake2s.cpp
    MinersLib/Algo/Parnian.cpp
    MinersLib/Algo/haval.cpp
    MinersLib/Algo/sha0.cpp
    MinersLib/Algo/sha2.cpp    
    MinersLib/Algo/sha1.cpp
    MinersLib/Algo/sph_panama.cpp
    corelib/CommonData.cpp
    corelib/FixedHash.cpp
    corelib/Log.cpp
    corelib/ParnianWork.cpp
    corelib/utils.cpp
    corelib/Worker.cpp
    corelib/miniweb.cpp
    parnianminer/ClientManager.cpp
    parnianminer/CommandLineManager.cpp
    parnianminer/main.cpp
   )
   
# jsoncpp: by default the bundled copy (jsoncpp-1.8.0) is compiled and linked
# statically, so the final binary does not need libjsoncpp installed.
option(RH_BUNDLED_JSONCPP "Use the bundled jsoncpp, linked statically" ON)
if(RH_BUNDLED_JSONCPP)
    set(JSONCPP_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/jsoncpp-1.8.0/include)
    include_directories(BEFORE ${JSONCPP_INCLUDE_DIRS})
    add_library(jsoncpp_bundled STATIC
        jsoncpp-1.8.0/src/lib_json/json_reader.cpp
        jsoncpp-1.8.0/src/lib_json/json_value.cpp
        jsoncpp-1.8.0/src/lib_json/json_writer.cpp)
    target_include_directories(jsoncpp_bundled PRIVATE jsoncpp-1.8.0/src/lib_json)
    set(JSONCPP_LIBRARIES jsoncpp_bundled)
else()
    find_path(JSONCPP_INCLUDE_DIRS "json/json.h" PATH_SUFFIXES "jsoncpp" DOC "jsoncpp include directory")
    find_library(JSONCPP_LIBRARIES NAMES jsoncpp DOC "jsoncpp library")
    if(NOT JSONCPP_INCLUDE_DIRS OR NOT JSONCPP_LIBRARIES)
        message(FATAL_ERROR "jsoncpp not found")
    endif()
    include_directories(${JSONCPP_INCLUDE_DIRS})
endif()

set(Boost_USE_STATIC_LIBS   ON)
if(WIN32)
    set(Boost_USE_STATIC_RUNTIME ON)
endif()
set(JSONCPP_LIB_BUILD_STATIC ON)

find_package( Boost REQUIRED COMPONENTS atomic chrono system thread )
include_directories(${Boost_INCLUDE_DIR})
link_libraries(${BOOST_LIBRARIES})


if(RH_CPU_ONLY)
    set(PU_SPEC "cpu")
else(RH_CPU_ONLY)
    set(RH_CUDA_LIB cudalib)
    set(RH_OCLL OpenCL)
    add_subdirectory(cudalib)
endif(RH_CPU_ONLY)

#CHECK_INCLUDE_FILE (syslog.h HAVE_SYSLOG_H)
#if (HAVE_SYSLOG_H)
#    add_definitions(/DHAVE_SYSLOG_H)
#endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/parnianminer_${RH_ARCH})
message(STATUS "Output directory is ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

if(WIN32)
    # Version information + icon in the .exe (antivirus reputation).
    # Compiled with a custom command so the C++ compile flags are not passed to windres.
    set(RH_VERSION_RES ${CMAKE_BINARY_DIR}/version_res.o)
    add_custom_command(OUTPUT ${RH_VERSION_RES}
        COMMAND ${CMAKE_RC_COMPILER} -O coff -I ${CMAKE_SOURCE_DIR}/parnianminer
                -i ${CMAKE_SOURCE_DIR}/parnianminer/version.rc -o ${RH_VERSION_RES}
        DEPENDS ${CMAKE_SOURCE_DIR}/parnianminer/version.rc
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/parnianminer)
    set_source_files_properties(${RH_VERSION_RES} PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
    set(SOURCES ${SOURCES} ${RH_VERSION_RES})
endif(WIN32)

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})
if(WIN32)
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++ -static)
elseif(NOT APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # Linux: libstdc++, libgcc, boost and jsoncpp are linked statically so the
    # binary runs without installing any package (only the system libc is used)
    # (with the zig toolchain libc++ is always static, see CMake/toolchain-zig-linux.cmake)
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
endif()
target_link_libraries(${PROJECT_NAME} ${RH_CUDA_LIB} ${LIBS} ${LIBRARIES} ${EXTRA_LIBS} ${JSONCPP_LIBRARIES} ${CMAKE_DL_LIBS} ${Boost_ATOMIC_LIBRARY} ${Boost_CHRONO_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${RH_OCLL})



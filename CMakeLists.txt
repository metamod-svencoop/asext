cmake_minimum_required(VERSION 3.21)
option(FORMAT_VALIDATION_ONLY "Configure formatting without compilers or native dependencies" OFF)
if(FORMAT_VALIDATION_ONLY)
    project(FormatValidation LANGUAGES NONE)
    include(cmake/FormatValidation.cmake)
    return()
endif()
include(cmake/Metamod.cmake)
project(asext LANGUAGES C CXX)
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build configuration" FORCE)
endif()
include("${METAMOD_SOURCE_PATH}/cmake/SDK.cmake")
include(cmake/GoldSrcExt.cmake)
mmfg_configure_install_layout()
include(cmake/SDK.cmake)

set(ASEXT_BRIDGE_SOURCES
    src/bridge/asext_bridge.cpp
)
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    list(APPEND ASEXT_BRIDGE_SOURCES src/bridge/asext_bridge_legacy.cpp)
endif()

add_library(asext_hook_bridge STATIC ${ASEXT_BRIDGE_SOURCES})
target_compile_features(asext_hook_bridge PUBLIC cxx_std_17)
set_target_properties(asext_hook_bridge PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(asext_hook_bridge PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(asext_hook_bridge PUBLIC GoldSrcExt::Headers PRIVATE ASExt::SDK)
if(MSVC)
    target_compile_definitions(asext_hook_bridge PRIVATE WIN32 _CRT_SECURE_NO_WARNINGS
        $<$<CONFIG:Debug>:_DEBUG> $<$<NOT:$<CONFIG:Debug>>:NDEBUG>)
    target_compile_options(asext_hook_bridge PRIVATE /Zi)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(asext_hook_bridge PRIVATE PLATFORM_POSIX LINUX _LINUX
        $<$<CONFIG:Debug>:_DEBUG> $<$<NOT:$<CONFIG:Debug>>:NDEBUG>)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_compile_options(asext_hook_bridge PRIVATE -m64 -fno-exceptions -fno-rtti -fvisibility=hidden
            -g $<$<NOT:$<CONFIG:Debug>>:-O2>)
    else()
        target_compile_options(asext_hook_bridge PRIVATE -m32 -fno-exceptions -fno-rtti -fvisibility=hidden
            -march=i686 -mtune=generic -msse -msse2 -g $<$<NOT:$<CONFIG:Debug>>:-O2>)
    endif()
endif()

add_library(asext SHARED
    src/dllapi.cpp
    src/engine_api.cpp
    src/h_export.cpp
    src/meta_api.cpp
    src/server_hook.cpp
)
mmfg_configure_target(asext)
target_link_libraries(asext PRIVATE asext_hook_bridge ASExt::SDK)
if(WIN32)
    target_sources(asext PRIVATE src/dllmain.cpp)
    target_compile_definitions(asext PRIVATE ASEXT_EXPORTS)
endif()
mmfg_add_gamedata(asext scripts/manifests/asext.json)
mmfg_assign_source_groups(asext_hook_bridge)
mmfg_assign_source_groups(asext)

option(MMFG_ASEXT_BUILD_TESTS "Build ASExt ABI and host dispatch tests" ON)
if(MMFG_ASEXT_BUILD_TESTS)
    enable_testing()
    add_executable(mmfg_test_asext_abi tests/test_asext_abi.cpp)
    target_include_directories(mmfg_test_asext_abi PRIVATE src)
    target_link_libraries(mmfg_test_asext_abi PRIVATE ASExt::SDK GoldSrcExt::Headers)
    target_compile_features(mmfg_test_asext_abi PRIVATE cxx_std_17)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_compile_options(mmfg_test_asext_abi PRIVATE -fno-exceptions -fno-rtti)
    endif()
    add_test(NAME mmfg_test_asext_abi COMMAND mmfg_test_asext_abi)
endif()

include(cmake/FormatValidation.cmake)

#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(${use_installed_dependencies})
    #These need to be set for the functions included by c-utility 
    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/c-utility/src")
        message(FATAL_ERROR "c-utility directory is empty. Please pull in all submodules if building unit tests.")
    endif()
    set(SHARED_UTIL_SRC_FOLDER "${CMAKE_CURRENT_LIST_DIR}/c-utility/src")
    set(SHARED_UTIL_FOLDER "${CMAKE_CURRENT_LIST_DIR}/c-utility")
    set(SHARED_UTIL_ADAPTER_FOLDER "${CMAKE_CURRENT_LIST_DIR}/c-utility/adapters")
    set_platform_files("${CMAKE_CURRENT_LIST_DIR}/c-utility")
    find_package(umock_c REQUIRED CONFIG)
endif()
#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ctest_FOUND)
    find_package(ctest REQUIRED CONFIG)
endif()
if(NOT testrunnerswitcher_FOUND)
    find_package(testrunnerswitcher REQUIRED CONFIG)
endif()

include("${CMAKE_CURRENT_LIST_DIR}/umock_cTargets.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/umock_cFunctions.cmake")

get_target_property(UMOCK_C_INCLUDES umock_c INTERFACE_INCLUDE_DIRECTORIES)

set(UMOCK_C_INCLUDES ${UMOCK_C_INCLUDES} CACHE INTERNAL "")
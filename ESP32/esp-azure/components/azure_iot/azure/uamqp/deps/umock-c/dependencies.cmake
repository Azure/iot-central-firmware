#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(${use_installed_dependencies})
    find_package(ctest REQUIRED CONFIG)
    find_package(testrunnerswitcher REQUIRED CONFIG)
else()
    add_subdirectory(deps)
endif()
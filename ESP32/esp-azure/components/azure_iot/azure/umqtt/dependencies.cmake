#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if (${use_installed_dependencies})
    find_package(azure_c_shared_utility REQUIRED CONFIG)
else ()
    add_subdirectory(deps/c-utility EXCLUDE_FROM_ALL)
endif ()
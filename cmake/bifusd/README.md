# Welcome to Bifusd - A generic cmake build system

Bifusd is a cmake build system heavily inspired from Amino's build system. It
contains utilities to easily build your projects with cmake, optimized for the
Ninja generator. It is multi-platform and targets x64 architectures. It allows
your project to fully take advantage of artifactory access, doc generation,
binary version (including Windows binary version stamping), a testing framework
based on googletest.


## Subdirectories

- config -- configuration macros and defines related to the OS and compiler used
- versioning -- contains a library dedicated to handle versioning

## Getting Started

Add the Bifusd git project as a sub module of your project. You'll have access
to all of Bifusd's build, testing and analysis tools.

### Using Bifusd

Your main CMakeLists.txt file must set a few BIFUSD_ variables and call Bifusd
setup functions. All of Bifusd's functions are documented, including parameters
and modes; you are greatly encouraged to read them! See the utils.cmake file.

### A typical main CMakeLists.txt file would look like this:

#### Set the minimum version of cmake allowed
```cmake
cmake_minimum_required(VERSION 3.20.3)
message(STATUS "CMake version: ${CMAKE_VERSION}")
```

#### Give a name to your project
```cmake
project(bifrost-usd-pack NONE) # Enable no language yet.
```

```cmake
# Generates a 'compile_commands.json' file containing the exact compilation
# command for all translation units of the project in a machine-readable form.
#
# Useful for many offline tools and very useful for debugging the build.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# General utility functions
```

#### Set the Bifusd directory
```cmake
set(BIFUSD_TOOLS_DIR "${PROJECT_SOURCE_DIR}/gitmodules/bifusd")
```

#### Include Bifusd's build functions
```cmake
# This defines functions and macros that used later on; in particular
# in setup.cmake and the compiler setup.
include(${BIFUSD_TOOLS_DIR}/utils.cmake)

#==============================================================================
# Define common build variables
#==============================================================================
```

#### Set the version of your project
```cmake
set(BIFUSD_MAJOR_VERSION 0)
set(BIFUSD_MINOR_VERSION 1)
set(BIFUSD_PATCH_LEVEL   0)
set(BIFUSD_PRODUCT_NAME  "\"Usd_pack\"")
```

#### Include Bifusd's setup and version cmake files
```cmake
# This defines variables that used later on; in particular in the compiler setup.
include(${BIFUSD_TOOLS_DIR}/setup.cmake)
include(${BIFUSD_TOOLS_DIR}/version.cmake)

#==============================================================================
# COMPILER
#==============================================================================

# Include the file configuring the compiler used on the given platform.
# Enabling the languages and set the compiler flags after the project
# has been configured follows the suggestion in:
#
# http://www.cmake.org/Bug/print_bug_page.php?bug_id=11942
```

#### Tell Bifusd where are defined your exposed symbols for linux and mac

This is an optional step. The BIFUSD_EXPORTS_FILE and BIFUSD_SYMBOLS_FILE
variables can be left unset if exporting all symbols.  Currently, bifus
uses no export files.

```cmake
set(BIFUSD_EXPORTS_FILE ${PROJECT_SOURCE_DIR}/cmake/bifusd.exports)
set(BIFUSD_SYMBOLS_FILE ${PROJECT_SOURCE_DIR}/cmake/bifusd.symbols)
```

#### Set the right compiler
```cmake
set(CMAKE_USER_MAKE_RULES_OVERRIDE ${BIFUSD_TOOLS_DIR}/compiler_${CMAKE_SYSTEM_NAME}.cmake)

#==============================================================================
# LANGUAGES
#==============================================================================
# The platform setup needs to be called before enable_language
# It will setup the platform's SDK location, when necessary.
#
# On OSX BIFUSD_USE_ACTIVE_OSX_SDK can be set to ON to choose the
# active SDK on your OSX system. The default is to pick the oldest
# found/supported - currently 10.14
include(${BIFUSD_TOOLS_DIR}/platform_sdk_${CMAKE_SYSTEM_NAME}.cmake)

# Enabling the languages and set the compiler flags after the project
# has been configured follows the suggestion in:
#
# http://www.cmake.org/Bug/print_bug_page.php?bug_id=11942
enable_language(CXX)

# Finalize the various build variants now that the C++ language has been
# enabled.
include(${BIFUSD_TOOLS_DIR}/final_compiler_setup.cmake)

#==============================================================================
# PACKAGES USED WHILE BUILING
#==============================================================================
```

#### Include your own packages (such as java or python)

#### Include your own third parties
```cmake
#==============================================================================
# 3rd-parties
#==============================================================================

include(cmake/amino.cmake) # example
include(cmake/vnn.cmake)   # example
```

#### Include Bifusd's test framework
```cmake
#==============================================================================
# TESTING
#==============================================================================

include(CTest)
include(${BIFUSD_TOOLS_DIR}/tests.cmake)
```
#### Tell Bifusd what is the name of your package
```cmake
#==============================================================================
# PACKAGING
#==============================================================================

set(BIFUSD_PACKAGE_NAME usd_pack)
```

#### Include Bifusd's packaging mechanism
```cmake
include(${BIFUSD_TOOLS_DIR}/package.cmake)
```

#### Print your third party libs locations
```cmake
message(STATUS "USD       location:           ${USD_LOCATION}")
message(STATUS "MAYA      location:           ${MAYA_RUNTIME_LOCATION}")

#==============================================================================
# SUBDIRECTORIES
#==============================================================================
```

#### Include Bifusd's config and versioning directories
```cmake
add_subdirectory(${BIFUSD_TOOLS_DIR}/src/config)
add_subdirectory(${BIFUSD_TOOLS_DIR}/src/versioning)
```

#### Include your own directories
```cmake
#add_subdirectory(utils)
#add_subdirectory(src)
#add_subdirectory(test)
#add_subdirectory(doc)
```

#### Allow bifusd to finalize the build process
```cmake
#==============================================================================
# FINALIZATION
#==============================================================================

include(${BIFUSD_TOOLS_DIR}/finalize.cmake)
```

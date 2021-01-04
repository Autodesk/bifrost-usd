#-
#*****************************************************************************
# Copyright 2022 Autodesk, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#*****************************************************************************
#+

if(bifusd_version_included)
    return()
endif()
set(bifusd_version_included true)

#==============================================================================
# BIFUSD VERSION INFORMATION
#==============================================================================

# Git commit hash corresponding to this build
#
# This is used to report version information of the Git commit hash that
# corresponds to a build. It is set by Jenkins through our build.xml
# script. When unset, the default value DEVBLD is used instead as this usually
# corresponds to developers local builds (which might contain locally modified
# files).
set(BIFUSD_GIT_COMMIT "DEVBLD"
    CACHE STRING "Git commit hash corresponding to this build.")


# Git branch corresponding to this build
#
# This is used to report version information about the Git branch of a
# build. It is set by Jenkins through our build.xml script. When unset, the
# default value DEVBR is used instead as this usually corresponds to developers
# local builds (which might contain locally modified files).
set(BIFUSD_GIT_BRANCH "DEVBR"
    CACHE STRING "Change set identifier corresponding to this build.")


# Jenkins build number corresponding to this build
#
# This is used to report version information about the Jenkins build number
# that has produced this binary. It is set by Jenkins through our build.xml
# script. When unset, the default value "0" is used instead as this
# usually corresponds to developers local builds.
set(BIFUSD_JENKINS_BUILD "0"
    CACHE STRING "Jenkins build number corresponding to this build.")

#
# IMPORTANT IMPORTANT IMPORTANT:
#
# DON'T FORGET TO INCREMENT THE VERSION (MAJOR, MINOR OR PATCHLEVEL) BEFORE
# ANY RELEASE!
#
#
# The following information can help understand what follows:
#
# http://www.robertdickau.com/msi_tips.html#versions
# https://msdn.microsoft.com/en-us/library/windows/desktop/aa369786(v=vs.85).aspx
#
# It is very important to increase at least the patch level for any release so
# that an incremental MSI install properly updates the product DLLs.
#
#    Major upgrades in Windows Installer use only the first three fields of the
#    ProductVersion property: the FindRelatedProducts action won’t distinguish
#    version 1.2.3.4 of a product from version 1.2.3.5.
#
#    Some take this to mean that file versions in the File table are limited to
#    three fields, which isn’t the case. The Version field of the File table
#    uses the Version data type, described as xxx.xxx.xxx.xxx, where each field
#    is an integer from 0 through 65536.
#

set(BIFUSD_COMPANY_NAME      "\"Autodesk\"")
set(BIFUSD_LEGAL_COPYRIGHT   "\"Copyright (c) 2022 Autodesk, Inc. All rights reserved.\"")
set(BIFUSD_LEGAL_TRADEMARKS1 "\"Copyright (c) 2022 Autodesk, Inc. All rights reserved.\"")
set(BIFUSD_LEGAL_TRADEMARKS2 "\"Copyright (c) 2022 Autodesk, Inc. All rights reserved.\"")

function(bifusd_compute_timestamps)
    # BIFUSD_BUILD_ID_0 and BIFUSD_BUILD_ID_1 are based on Maya's
    string(TIMESTAMP BUILD_ID_0 "\"%Y%m%d%H%M-${BIFUSD_JENKINS_BUILD}-${BIFUSD_GIT_COMMIT}\"")
    string(TIMESTAMP BUILD_ID_1 "\"%Y%m%d%H%M-${BIFUSD_JENKINS_BUILD}-${BIFUSD_GIT_COMMIT}-1\"")

    # The date is formated the same way Maya formats its date.
    # weekdday month/day/fullyear, CONCAT(fullyear + month + day + fullhour + minute)
    string(TIMESTAMP WEEKDAY "%w")
    set(BIFUSD_WEEK_DAYS "Sun" "Mon" "Tue" "Wed" "Thu" "Fri" "Sat")
    list(GET BIFUSD_WEEK_DAYS ${WEEKDAY} WEEKDAY)
    string(TIMESTAMP PRODUCT_DATE "\"${WEEKDAY} %m/%d/%Y, %Y%m%d%H%M\"")

    set(BIFUSD_BUILD_ID_0   "${BUILD_ID_0}"   CACHE STRING "Build ID 0")
    set(BIFUSD_BUILD_ID_1   "${BUILD_ID_1}"   CACHE STRING "Build ID 1")
    set(BIFUSD_PRODUCT_DATE "${PRODUCT_DATE}" CACHE STRING "Product Date")

    # For non-Jenkins builds (i.e. developers), don't reset the timestamps
    # unless a clean build is made. This is annoying as it causes all the
    # version file to be regenerated and thus all libraries and executable to
    # be relinked. Else, do force a new timestamp!!!
    if (NOT BIFUSD_JENKINS_BUILD EQUAL 0)
        bifusd_reset_cache_entry(BIFUSD_BUILD_ID_0   "${BUILD_ID_0}"  )
        bifusd_reset_cache_entry(BIFUSD_BUILD_ID_1   "${BUILD_ID_1}"  )
        bifusd_reset_cache_entry(BIFUSD_PRODUCT_DATE "${PRODUCT_DATE}")
    endif()
endfunction(bifusd_compute_timestamps)
bifusd_compute_timestamps()

set(BIFUSD_BUILD_VERSION   "${BIFUSD_JENKINS_BUILD}")


# Variables used to initialize the library and executable target properties.
# See:
#   - http://www.cmake.org/cmake/help/v3.0/prop_tgt/VERSION.html
#   - http://www.cmake.org/cmake/help/v3.0/prop_tgt/SOVERSION.html
set(BIFUSD_VERSION "${BIFUSD_MAJOR_VERSION}.${BIFUSD_MINOR_VERSION}.${BIFUSD_PATCH_LEVEL}")

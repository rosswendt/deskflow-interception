# SPDX-FileCopyrightText: 2024-2025 Deskflow Developers
# SPDX-License-Identifier: MIT

# Capture this file's dir now (scope-safe)
set(MY_DIR ${CMAKE_CURRENT_LIST_DIR})

# Bundle MSVC runtime into the package (no external redist needed)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
include(InstallRequiredSystemLibraries)

# Pre-CPack script (used for portable artifacts)
configure_file(${MY_DIR}/pre-cpack.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/pre-cpack.cmake @ONLY)
set(CPACK_PRE_BUILD_SCRIPTS ${CMAKE_CURRENT_BINARY_DIR}/pre-cpack.cmake)

# Project-level CPack options injected at package time
configure_file(${MY_DIR}/cpack-options.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/cpack-options.cmake @ONLY)
set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/cpack-options.cmake)

# ------------------------------
# Robust architecture detection
# ------------------------------
# 1) Respect a user-provided CPACK_WIX_ARCHITECTURE (e.g., -DCPACK_WIX_ARCHITECTURE=x64)
# 2) Else, derive from generator platform or pointer size
if(NOT DEFINED CPACK_WIX_ARCHITECTURE OR CPACK_WIX_ARCHITECTURE STREQUAL "")
  if(DEFINED CMAKE_GENERATOR_PLATFORM AND NOT CMAKE_GENERATOR_PLATFORM STREQUAL "")
    string(TOLOWER "${CMAKE_GENERATOR_PLATFORM}" _cgp)
    if(_cgp MATCHES "^(x64|amd64)$")
      set(CPACK_WIX_ARCHITECTURE x64)
    elseif(_cgp MATCHES "^arm64$")
      set(CPACK_WIX_ARCHITECTURE arm64)
    else()
      set(CPACK_WIX_ARCHITECTURE x86)
    endif()
  else()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(CPACK_WIX_ARCHITECTURE x64)
    else()
      set(CPACK_WIX_ARCHITECTURE x86)
    endif()
  endif()
endif()

# Derive BUILD_ARCHITECTURE for any logic that still looks at it
if(NOT DEFINED BUILD_ARCHITECTURE OR BUILD_ARCHITECTURE STREQUAL "")
  set(BUILD_ARCHITECTURE ${CPACK_WIX_ARCHITECTURE})
endif()

# OS string used by deploy/CMakeLists.txt for package file name
if(NOT DEFINED OS_STRING OR OS_STRING STREQUAL "")
  set(OS_STRING "win-${CPACK_WIX_ARCHITECTURE}")
endif()

# ------------------------------
# Generators (portable + MSI)
# ------------------------------
# Always provide a portable archive
list(APPEND CPACK_GENERATOR "7Z")

# Add WiX MSI if wix.exe is available
find_program(WIX_APP wix)
if(NOT "${WIX_APP}" STREQUAL "")
  set(CPACK_WIX_VERSION 4)
  # Do not override CPACK_WIX_ARCHITECTURE here; it has been finalized above
  list(APPEND CPACK_GENERATOR "WIX")
endif()

# ------------------------------
# CPack/WiX metadata
# ------------------------------
set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_PROPER_NAME}")
set(CPACK_WIX_PROGRAM_MENU_FOLDER "${CMAKE_PROJECT_PROPER_NAME}")
set(CPACK_PACKAGE_EXECUTABLES "deskflow" "${CMAKE_PROJECT_PROPER_NAME}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CMAKE_PROJECT_PROPER_NAME}")

# Stable upgrade GUID (do not change across releases)
set(CPACK_WIX_UPGRADE_GUID "027D1C8A-E7A5-4754-BB93-B2D45BFDBDC8")

# WiX UI images
set(CPACK_WIX_UI_BANNER "${MY_DIR}/wix-banner.png")
set(CPACK_WIX_UI_DIALOG "${MY_DIR}/wix-dialog.png")

# WiX extensions and XML namespaces
list(APPEND CPACK_WIX_EXTENSIONS
     "WixToolset.Util.wixext"
     "WixToolset.Firewall.wixext")

list(APPEND CPACK_WIX_CUSTOM_XMLNS
     "util=http://wixtoolset.org/schemas/v4/wxs/util"
     "firewall=http://wixtoolset.org/schemas/v4/wxs/firewall")

# ------------------------------
# WiX patch (service, firewall, etc.)
# ------------------------------
configure_file(${MY_DIR}/wix-patch.xml.in
               ${CMAKE_CURRENT_BINARY_DIR}/wix-patch.xml @ONLY)
set(CPACK_WIX_PATCH_FILE "${CMAKE_CURRENT_BINARY_DIR}/wix-patch.xml")

# ------------------------------
# WiX custom action DLL
# ------------------------------
configure_file(${MY_DIR}/wix-custom.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/wix-custom.h @ONLY)

add_library(wix-custom SHARED
  ${MY_DIR}/wix-custom.cpp
)
target_include_directories(wix-custom PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(wix-custom PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_CURRENT_BINARY_DIR}"
)
target_link_libraries(wix-custom PRIVATE Msi)

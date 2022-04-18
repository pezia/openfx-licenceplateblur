# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("OpenFxPlugins")

# Include sub-projects.
add_subdirectory ("LicenceplateBlur")

MESSAGE (STATUS "  Platform:")
MESSAGE (STATUS "    Host:" ${CMAKE_HOST_SYSTEM_NAME} ${CMAKE_HOST_SYSTEM_VERSION} ${CMAKE_HOST_SYSTEM_PROCESSOR})
if(CMAKE_CROSSCOMPILING)
  MESSAGE (STATUS "    Target:"         ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION} ${CMAKE_SYSTEM_PROCESSOR})
endif()
MESSAGE (STATUS "    CMake:"            ${CMAKE_VERSION})
MESSAGE (STATUS "    CMake generator:"  ${CMAKE_GENERATOR})
MESSAGE (STATUS "    CMake build tool:" ${CMAKE_BUILD_TOOL})
if(MSVC)
  MESSAGE (STATUS "    MSVC:"           ${MSVC_VERSION})
endif()

include(FindOpenGL REQUIRED)

set (CMAKE_CXX_STANDARD 14)

# Use -Ofast when applicable (implies -ffast-math)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Wextra -Ofast -DNDEBUG")
    set (CMAKE_C_FLAGS_RELEASE   "-Wall -Wextra -Ofast -DNDEBUG")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Ofast -DNDEBUG")
    set (CMAKE_C_FLAGS_RELEASE   "-Wall -Ofast -DNDEBUG")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Wextra -Ofast -DNDEBUG")
    set (CMAKE_C_FLAGS_RELEASE   "-Wall -Wextra -Ofast -DNDEBUG")
endif()

SET(OPENFX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/openfx)

# Check that submodule have been initialized and updated
if(NOT EXISTS ${OPENFX_PATH}/include)
  message(FATAL_ERROR
    "\n submodule(s) are missing, please update your repository:\n"
    "  > git submodule update -i\n")
endif()

# Check that support extenstions submodule have been initialized and updated
if(NOT EXISTS ${PROJECT_SOURCE_DIR}/openfx-supportext)
  message(FATAL_ERROR
    "\n submodule(s) are missing, please update your repository:\n"
    "  > git submodule update -i\n")
endif()

# C++ Include directories
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/openfx-supportext)
INCLUDE_DIRECTORIES(${OPENFX_PATH}/include)
INCLUDE_DIRECTORIES(${OPENFX_PATH}/Support/include)
INCLUDE_DIRECTORIES(${OPENFX_PATH}/Support/Plugins/include)
# INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/CImg)


# Define "DEBUG" on debug builds
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

# /MP on MSVC
ADD_COMPILE_OPTIONS($<$<CXX_COMPILER_ID:MSVC>:/MP>)

SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

set(OFX_SUPPORT_HEADER_DIR "${OPENFX_PATH}/Support/include")
set(OFX_SUPPORT_LIBRARY_DIR "${OPENFX_PATH}/Support/Library")

FILE(GLOB SUPPORT_SOURCES
    "${OFX_SUPPORT_HEADER_DIR}/*.h"

    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsCore.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsImageEffect.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsInteract.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsLog.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsMultiThread.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsParams.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsProperty.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsPropertyValidation.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/*.h"
)


FILE(GLOB MISC_SOURCES
  "LicenceplateBLur/*.cpp"
  "openfx-supportext/tinythread.cpp"
  "openfx-supportext/ofxsThreadSuite.cpp"
  "openfx-supportext/ofxsFileOpen.cpp"
  "openfx-supportext/ofxsGenerator.cpp"
  "openfx-supportext/ofxsLut.cpp"
  "openfx-supportext/ofxsMultiPlane.cpp"
  "openfx-supportext/ofxsOGLTextRenderer.cpp"
  "openfx-supportext/ofxsOGLFontData.cpp"
  "openfx-supportext/ofxsRamp.cpp"
  "openfx-supportext/ofxsShutter.cpp"
  "openfx-supportext/ofxsTracking.cpp"
  "openfx-supportext/ofxsTransform3x3.cpp"
  "openfx-supportext/ofxsTransformInteract.cpp"
  "openfx-supportext/ofxsRectangleInteract.cpp"
  "openfx-supportext/*.h"
)

FILE(GLOB MISC_RESOURCES
  "LicenceplateBlur/hu.pezia.openfx.LicenceplateBlurPlugin.png"
  "LicenceplateBlur/hu.pezia.openfx.LicenceplateBlurPlugin.svg"
)

ADD_LIBRARY(Misc SHARED ${MISC_SOURCES} ${SUPPORT_SOURCES})
SET_TARGET_PROPERTIES(Misc PROPERTIES PREFIX "")
SET_TARGET_PROPERTIES(Misc PROPERTIES SUFFIX ".ofx")


# Add extra flags to the link step of the plugin
if(APPLE)
    set_target_properties(Misc PROPERTIES LINK_FLAGS "-fvisibility=hidden -exported_symbols_list,${OFX_SUPPORT_HEADER_DIR}/osxSymbols")
    set_target_properties(Misc PROPERTIES INSTALL_RPATH "@loader_path/../Frameworks;@loader_path/../Libraries")
elseif(MINGW)
    set_target_properties(Misc PROPERTIES LINK_FLAGS "-shared -fvisibility=hidden -Xlinker --version-script=${OFX_SUPPORT_HEADER_DIR}/linuxSymbols")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
# Linux & FreeBSD
    set_target_properties(Misc PROPERTIES LINK_FLAGS "-Wl,-fvisibility=hidden,--version-script=${OFX_SUPPORT_HEADER_DIR}/linuxSymbols")
    set_target_properties(Misc PROPERTIES INSTALL_RPATH "$ORIGIN/../../Libraries")
endif()

TARGET_COMPILE_DEFINITIONS(Misc PRIVATE OFX_EXTENSIONS_VEGAS OFX_EXTENSIONS_NUKE OFX_EXTENSIONS_NATRON OFX_EXTENSIONS_TUTTLE OFX_SUPPORTS_OPENGLRENDER NOMINMAX)
TARGET_LINK_LIBRARIES(Misc ${OPENGL_gl_LIBRARY})

IF (MSVC)
  # Some files require this option. This breaks compatibility with older linkers.
  SET_TARGET_PROPERTIES(Misc PROPERTIES COMPILE_FLAGS "/bigobj")
  # If the build system is at least Windows Vista (6.0),
  # compile for Windows Vista, to get interruptible CImg plugins (see CImgFilter.h)
  IF (NOT ${CMAKE_SYSTEM_VERSION} VERSION_LESS 6.0) # Windows Vista
    ADD_DEFINITIONS(-D_WIN32_WINNT=0x0600 -DWINVER=0x0600)
    MESSAGE(STATUS "CImg plugins should be interruptible")
  ELSE ()
    MESSAGE(STATUS "CImg plugins may not be interruptible")
  ENDIF ()
ENDIF ()

# Find and set the arch name.
# http://openeffects.org/documentation/reference/ch02s02.html
SET(OFX_ARCH UNKNOWN)

# Win
if(MINGW)
    set(OFX_ARCH "Win32")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        set(OFX_ARCH "Win64")
    endif()
# Visual Studio    
elseif(MSVC)
    set(OFX_ARCH "Win32")
    if((${CMAKE_SYSTEM_PROCESSOR} STREQUAL "IA64") OR (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64"))
        set(OFX_ARCH "Win64")
    endif()
# FreeBSD
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    set(OFX_ARCH "FreeBSD-x86")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        set(OFX_ARCH "FreeBSD-x86-64")
    endif()
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
        if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
            set(OFX_ARCH FreeBSD-arm-64)
        else()
            set(OFX_ARCH FreeBSD-arm)
        endif()
    endif()
# MacOS
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(OFX_ARCH "MacOS")
# Linux
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(OFX_ARCH "Linux-x86")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        set(OFX_ARCH "Linux-x86-64")
    endif()
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
        if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
            set(OFX_ARCH Linux-arm-64)
        else()
            set(OFX_ARCH Linux-arm)
        endif()
    endif()
endif()

SET(OFX_ARCH_NAME ${OFX_ARCH} CACHE STRING "OpenFX target OS and architecture")
IF("${OFX_ARCH_NAME}" EQUAL "UNKNOWN")
  MESSAGE(SEND_ERROR "OFX_ARCH_NAME was not detected. Install targets will be unavailable. See http://openeffects.org/documentation/reference/ch02s02.html to pick an arch name.")
ELSE()
  INSTALL(TARGETS Misc
    RUNTIME DESTINATION Misc.ofx.bundle/Contents/${OFX_ARCH_NAME}
    LIBRARY DESTINATION Misc.ofx.bundle/Contents/${OFX_ARCH_NAME}
  )

  INSTALL(FILES ${MISC_RESOURCES}
    DESTINATION Misc.ofx.bundle/Contents/Resources
  )

ENDIF()

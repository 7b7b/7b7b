cmake_minimum_required(VERSION 3.0)
set(PROJECT 7b7b)
project(${PROJECT})

# CMake Settings (policies to do AUTOMOC as much as possible)
cmake_policy(SET CMP0057 NEW)
cmake_policy(SET CMP0071 NEW)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# C++ Settings (pretend everything is modern)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(
    Qt6
    CONFIG
    REQUIRED
    Concurrent
    Core
    Gui
    Network
    Widgets	
)
find_package(X11 REQUIRED)
find_package(
    XCB
    COMPONENTS
    REQUIRED
    XCB
    XFIXES
    AUX
    COMPOSITE
    DAMAGE
    DPMS
    EWMH
    ICCCM
    IMAGE
)

# OS-detection
# This ignores the actual OS and just sets FreeBSD-useful values
add_definitions(
    -DL_ETCDIR=QStringLiteral\("/etc"\)
    -DL_SHAREDIR=QStringLiteral\("${CMAKE_INSTALL_PREFIX}/share"\)
)

# Build options
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

add_subdirectory(src)

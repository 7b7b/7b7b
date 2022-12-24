cmake_minimum_required (VERSION 3.8.2)
set(PROJECT 7b7b-open)
project(${PROJECT})

# Find includes in the build directories
include(GNUInstallDirs)

# Turn on automatic invocation of the MOC, UIC & RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Add a compiler flag
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_MODULE_PATH "${CMAKE_INSTALL_FULL_LIBDIR}/cmake/7b7b;${CMAKE_MODULE_PATH}")

# Dependencies
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Concurrent)
find_package(7b7b REQUIRED)
find_package(X11 REQUIRED)
find_package(
	XCB
	COMPONENTS
    XCB
    XFIXES
    AUX
    COMPOSITE
    DAMAGE
    DPMS
    EWMH
    ICCCM
    IMAGE
    RANDR
)

add_subdirectory(src)

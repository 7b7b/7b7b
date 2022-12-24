cmake_minimum_required (VERSION 3.8.2)
set(PROJECT 7b7b-desktop)
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
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Network Concurrent Svg)
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

install(FILES "data/start-7b7b-desktop" DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES "data/7b7b-DE.desktop" DESTINATION "${CMAKE_INSTALL_DATADIR}/xsessions")
install(FILES "data/defaults/7b7bDesktop.conf" DESTINATION "${CMAKE_INSTALL_DATADIR}/7b7b-desktop")
install(FILES "data/extrafiles/globs2" DESTINATION "${CMAKE_INSTALL_DATADIR}/7b7b-desktop")

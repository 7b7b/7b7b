cmake_minimum_required (VERSION 3.8.2)
set(PROJECT lumina-desktop)
project(${PROJECT})

# Find includes in the build directories
include(GNUInstallDirs)

# Turn on automatic invocation of the MOC, UIC & RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Add a compiler flag
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_MODULE_PATH "/usr/lib/cmake/Lumina;${CMAKE_MODULE_PATH}")

# Dependencies
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Network Concurrent Svg)
find_package(Lumina REQUIRED)
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

install(FILES "data/start-lumina-desktop" DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES "data/Lumina-DE.desktop" DESTINATION "${CMAKE_INSTALL_DATADIR}/xsessions")
install(FILES "data/defaults/luminaDesktop.conf" DESTINATION "${CMAKE_INSTALL_DATADIR}/lumina-desktop")
install(FILES "data/extrafiles/globs2" DESTINATION "${CMAKE_INSTALL_DATADIR}/lumina-desktop")

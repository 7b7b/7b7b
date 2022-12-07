cmake_minimum_required(VERSION 3.0)
set(PROJECT Lumina)
project(Lumina)

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

set(LUMINA_INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}/${PROJECT}")
set(SRCS
    src/DesktopSettings.cpp
    src/LDesktopUtils.cpp
    src/LIconCache.cpp
    src/LUtils.cpp
    src/LuminaSingleApplication.cpp
    src/LuminaX11.cpp
    src/LuminaXDG.cpp
    src/ResizeMenu.cpp
    src/XDGMime.cpp
	src/LuminaOS.cpp
)

set(PUB_HDRS
    src/DesktopSettings.h
    src/ExternalProcess.h
	src/LDesktopUtils.h
    src/LIconCache.h
    src/LuminaSingleApplication.h
    src/LuminaX11.h
    src/LuminaXDG.h
    src/LuminaOS.h
    src/LUtils.h
    src/ResizeMenu.h
    src/XDGMime.h
)

add_library(
    ${PROJECT}
    SHARED
    ${PUB_HDRS}
    ${SRCS}
)

target_include_directories(${PROJECT}
    PUBLIC
    ${XCB_INCLUDE_DIR}
)

target_link_libraries(
	${PROJECT}
	Qt6::Concurrent
	Qt6::Core
	Qt6::GuiPrivate
	Qt6::Widgets
	Qt6::Network
	XCB::XCB
	XCB::AUX
	XCB::IMAGE
	XCB::COMPOSITE
	XCB::DAMAGE
	XCB::ICCCM
	XCB::EWMH
	XCB::DPMS
	X11::Xdamage
)

install(TARGETS
	${PROJECT}
	DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	EXPORT FindLumina
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	COMPONENT Runtime
)

install(FILES
    ${PUB_HDRS}
    DESTINATION "${LUMINA_INSTALL_INCLUDE_DIR}"
    COMPONENT Devel
)

install(EXPORT FindLumina DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT}")

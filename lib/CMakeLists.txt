cmake_minimum_required(VERSION 3.0)
project(libLumina
LANGUAGES C CXX
HOMEPAGE_URL https://lumina-project.org
DESCRIPTION "Libraries for lumina"
VERSION 1.6.2)

set(LIB_NAME "lumina" CACHE STRING "lumina")

set(LIBRARY_NAME lumina)

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

find_package(X11)
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

if(NOT X11_FOUND OR NOT XCB_XCB_FOUND)
    message(FATAL_ERROR "Required XCB component for X11 not found.")
endif()

# OS-detection
# This ignores the actual OS and just sets FreeBSD-useful values
add_definitions(
    -DL_ETCDIR=QStringLiteral\("/etc"\)
    -DL_SHAREDIR=QStringLiteral\("${CMAKE_INSTALL_PREFIX}/share"\)
)

# Build options
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(LUMINA_INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}/${LIBRARY_NAME}")
set(SRCS
    src/DesktopSettings.cpp
    src/LDesktopUtils.cpp
    src/LIconCache.cpp
    src/LUtils.cpp
    src/LuminaSingleApplication.cpp
    src/LuminaRandR-X11.cpp
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
    src/LuminaRandR.h
    src/LuminaSingleApplication.h
    src/LuminaX11.h
    src/LuminaXDG.h
    src/LuminaOS.h
    src/LUtils.h
    src/ResizeMenu.h
    src/XDGMime.h
)

add_library(
    ${LIBRARY_NAME}
    SHARED
    ${PUB_HDRS}
    ${SRCS}
)

target_include_directories(${LIBRARY_NAME}
    PUBLIC
    ${XCB_INCLUDE_DIR}
)

target_link_libraries(
    ${LIBRARY_NAME}
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
        XCB::RANDR
        XCB::EWMH
        XCB::DPMS
        X11::Xdamage
)

set(target ${LIBRARY_NAME})
set(pc_req_public "Qt6Core Qt6Network xcb xcb-aux xcb-damage x11")

configure_file(lumina.pc.in lumina.pc @ONLY)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/${LIB_NAME}-config.cmake.in"
    "${CMAKE_BINARY_DIR}/${LIB_NAME}-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${LIB_NAME}"
)

write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/${LIB_NAME}-config-version.cmake"
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_BINARY_DIR}/${LIB_NAME}-config-version.cmake"
    "${CMAKE_BINARY_DIR}/${LIB_NAME}-config.cmake"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${LIB_NAME}"
    COMPONENT Devel
)

install(EXPORT
    "${LIB_NAME}-targets"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${LIB_NAME}"
    COMPONENT Devel
)

install(TARGETS
	${LIBRARY_NAME}
	DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	EXPORT lumina-targets
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	COMPONENT Runtime
)

export(TARGETS ${LIB_NAME}
    FILE "${CMAKE_BINARY_DIR}/${LIB_NAME}-targets.cmake"
    EXPORT_LINK_INTERFACE_LIBRARIES
)

install(FILES
    ${CMAKE_BINARY_DIR}/lumina.pc
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
    COMPONENT Devel
)

install(FILES
    ${PUB_HDRS}
    DESTINATION "${LUMINA_INSTALL_INCLUDE_DIR}"
    COMPONENT Devel
)


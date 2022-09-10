# libLumina build, CMake edition. EXPERIMENTAL.
#
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: Copyright 2022 Adriaan de Groot <adridg@FreeBSD.org>

cmake_minimum_required(VERSION 3.0)
project(libLumina
LANGUAGES C CXX
HOMEPAGE_URL https://lumina-project.org
DESCRIPTION "Libraries for lumina"
VERSION 1.6.2)

set(LIBRARY_NAME lumina)

# CMake Settings (policies to do AUTOMOC as much as possible)
cmake_policy(SET CMP0057 NEW)
cmake_policy(SET CMP0071 NEW)
include(GNUInstallDirs)

# C++ Settings (pretend everything is modern)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ECM setup
include(FeatureSummary)
find_package(ECM 5.88.0 NO_MODULE)
set_package_properties(
    ECM
    PROPERTIES
    TYPE REQUIRED
    DESCRIPTION "Extra CMake Modules."
    URL "https://commits.kde.org/extra-cmake-modules"
)
feature_summary(
    WHAT REQUIRED_PACKAGES_NOT_FOUND
    FATAL_ON_MISSING_REQUIRED_PACKAGES
)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

# Dependencies
set(QT_VERSION 5.15.0)
find_package(
    Qt6
    CONFIG
    REQUIRED
    Concurrent
    Core
    Gui
    Network
    Svg
    Widgets
    #X11Extras
)

find_package(X11)
find_package(
    XCB
    COMPONENTS
    XCB
    KEYSYMS
    XKB
    XINPUT
    XFIXES
    AUX
    COMPOSITE
    DAMAGE
    DPMS
    EWMH
    ICCCM
    IMAGE
    SCREENSAVER
    RANDR
    OPTIONAL_COMPONENTS XTEST
)

if(NOT X11_FOUND OR NOT XCB_XCB_FOUND)
    message(FATAL_ERROR "Required XCB component for X11 not found.")
endif()

# OS-detection
#
# This ignores the actual OS and just sets FreeBSD-useful values
add_definitions(
    -DL_ETCDIR=QStringLiteral\("/etc"\)
    -DL_SHAREDIR=QStringLiteral\("${CMAKE_INSTALL_PREFIX}/share"\)
)

# Build options
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

#set(LUMINA_INTREE_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/include")
set(LUMINA_INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}/${LIBRARY_NAME}")

set(SRCS
    DesktopSettings.cpp
    LDesktopUtils.cpp
    LFileInfo.cpp
    LIconCache.cpp
    LInputDevice.cpp
    LUtils.cpp
    LuminaSingleApplication.cpp
    LuminaRandR-X11.cpp
    LuminaX11.cpp
    LuminaXDG.cpp
    ResizeMenu.cpp
    XDGMime.cpp

	# Template, you should change it to your own OS
	LuminaOS-Linux.cpp
)

set(PUB_HDRS
    DesktopSettings.h
    ExternalProcess.h
    LDesktopUtils.h
    LFileInfo.h
    LIconCache.h
    LInputDevice.h
    LuminaRandR.h
    LuminaSingleApplication.h
    LuminaX11.h
    LuminaXDG.h
    LuminaOS.h
    LUtils.h
    ResizeMenu.h
    XDGMime.h
)

add_library(
    ${LIBRARY_NAME}
    SHARED
    ${PUB_HDRS}
    ${SRCS}
)

target_include_directories(${LIBRARY_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${XCB_INCLUDE_DIR}
)

target_link_libraries(
    ${LIBRARY_NAME}
        Qt6::Concurrent
        Qt6::Core
        Qt6::GuiPrivate
        Qt6::Svg
        Qt6::Widgets
        Qt6::Network
        #Qt6::X11Extras
        XCB::XCB
        XCB::AUX
        XCB::IMAGE
        XCB::COMPOSITE
        XCB::DAMAGE
        XCB::XINPUT
        XCB::ICCCM
        XCB::RANDR
        XCB::EWMH
        XCB::SCREENSAVER
        XCB::DPMS
        X11::Xdamage
)

set(target ${LIBRARY_NAME})
set(pc_req_public "Qt6Core Qt6Network xcb xcb-aux xcb-damage xcb-screensaver x11")

configure_file(lumina.pc.in lumina.pc @ONLY)

install(TARGETS
	${LIBRARY_NAME}
	DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	EXPORT libLumina-targets
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	COMPONENT Runtime
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

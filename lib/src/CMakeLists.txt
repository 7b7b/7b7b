set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SRCS
    DesktopSettings.cpp
    LDesktopUtils.cpp
    LIconCache.cpp
    LUtils.cpp
    LuminaSingleApplication.cpp
    LuminaX11.cpp
    LuminaXDG.cpp
    LuminaOS.cpp
)

set(PUB_HDRS
    DesktopSettings.h
    ExternalProcess.h
	LDesktopUtils.h
    LIconCache.h
    LuminaSingleApplication.h
    LuminaX11.h
    LuminaXDG.h
	LuminaOS.h
    LUtils.h
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
    DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/${PROJECT}"
    COMPONENT Devel
)

install(EXPORT FindLumina DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT}")

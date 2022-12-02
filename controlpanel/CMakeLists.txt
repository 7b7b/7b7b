cmake_minimum_required (VERSION 3.8.2)
set(PROJECT lumina-config)
project(${PROJECT})

# Find includes in the build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include(GNUInstallDirs)

# Turn on automatic invocation of the MOC, UIC & RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Add a compiler flag
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_MODULE_PATH "/usr/lib/cmake/Lumina;${CMAKE_MODULE_PATH}")

# Dependencies
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Concurrent)
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
set(LUMINA_LIBS
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

# Tell CMake to create the executable
add_executable(${PROJECT}
	main.cpp
	mainWindow.cpp
	plugins/LPlugins.cpp
	plugins/GetPluginDialog.cpp
	pages/PageWidget.h
	globals.h
	plugins/AppDialog.h
	plugins/PanelWidget.cpp
	pages/getPage.cpp
	pages/page_main.cpp
	pages/wallpaper/page_wallpaper.cpp
	pages/autostart/page_autostart.cpp
	pages/defaultapps/page_defaultapps.cpp
	pages/desktop/page_interface_desktop.cpp
	pages/menu/page_interface_menu.cpp
	pages/panels/page_interface_panels.cpp
	pages/locale/page_session_locale.cpp
	pages/options/page_session_options.cpp
)

# Add the dependencies for linking
target_link_libraries(${PROJECT}
	Qt6::Widgets
	Qt6::GuiPrivate
	Qt6::Network
	Lumina
	${LUMINA_LIBS}
)

# Include header files
include_directories(
	"${CMAKE_INSTALL_FULL_INCLUDEDIR}/Lumina"
)

install(TARGETS ${PROJECT} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES "resources/lumina-config.desktop" DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")

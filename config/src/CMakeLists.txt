set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(7B7B_LIBS
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
	widgets/AppDialog.h
	widgets/PanelWidget.cpp
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
	Qt6::Network
	7b7b
	${7B7B_LIBS}
)

# Include header files
include_directories(
	"${CMAKE_INSTALL_FULL_INCLUDEDIR}/7b7b"
)

install(TARGETS ${PROJECT} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

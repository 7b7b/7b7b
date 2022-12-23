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

add_executable(${PROJECT}
	main.cpp
	Globals.h
	LXcbEventFilter.cpp
	LSession.cpp
	desktop/LDesktop.cpp
	desktop/LDesktopBackground.cpp
	desktop/LDesktopPluginSpace.cpp
	panel/LPanel.cpp
	LWinInfo.cpp

	widgets/AppMenu.cpp
	widgets/SystemWindow.cpp

	desktop/desktop-plugins/applauncher/AppLauncherPlugin.cpp
	desktop/desktop-plugins/applauncher/OutlineToolButton.h
	desktop/desktop-plugins/desktopview/DesktopViewPlugin.cpp

	panel/panel-plugins/appmenu/LAppMenuPlugin.cpp
	panel/panel-plugins/taskmanager/LTBWidget.h
	panel/panel-plugins/taskmanager/LTaskManagerPlugin.cpp
	panel/panel-plugins/taskmanager/LTaskButton.cpp
	panel/panel-plugins/spacer/LSpacer.h
	panel/panel-plugins/systemtray/LSysTray.cpp
	panel/panel-plugins/systemtray/TrayIcon.cpp
	panel/panel-plugins/clock/LClock.cpp
	panel/panel-plugins/showdesktop/LHomeButton.cpp

	desktop/desktop-plugins/LDPlugin.cpp
	panel/panel-plugins/LPPlugin.h
	panel/panel-plugins/NewPP.h
	desktop/desktop-plugins/LDPlugin.h
	desktop/desktop-plugins/NewDP.h
)

target_link_libraries(${PROJECT}
	Qt6::Widgets
	Qt6::GuiPrivate
	Qt6::Network
	Qt6::Concurrent
	Qt6::Svg
	7b7b
	${7B7B_LIBS}
)

include_directories(
	"${CMAKE_INSTALL_FULL_INCLUDEDIR}/7b7b"
	${XCB_INCLUDE_DIR}
)

install(TARGETS ${PROJECT} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

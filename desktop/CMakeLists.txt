cmake_minimum_required (VERSION 3.8.2)
set(PROJECT lumina-desktop)
project(${PROJECT})

# Find includes in the build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Turn on automatic invocation of the MOC, UIC & RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Add a compiler flag
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

#Find pkgconfig
find_package(PkgConfig REQUIRED)

# OS-detection
# This ignores the actual OS and just sets FreeBSD-useful values
add_definitions(
    -DL_ETCDIR=QStringLiteral\("/etc"\)
    -DL_SHAREDIR=QStringLiteral\("${CMAKE_INSTALL_PREFIX}/share"\)
)

# Dependencies
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Network Concurrent Svg)
pkg_check_modules(LUMINA REQUIRED lumina)

# Tell CMake to create the executable
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

 # Widgets
 widgets/AppMenu.cpp
 widgets/SystemWindow.cpp
 
 # Desktop plugins
 desktop/desktop-plugins/applauncher/AppLauncherPlugin.cpp
 desktop/desktop-plugins/applauncher/OutlineToolButton.h
 desktop/desktop-plugins/desktopview/DesktopViewPlugin.cpp

 # Panel plugins
 panel/panel-plugins/appmenu/LAppMenuPlugin.cpp
 panel/panel-plugins/taskmanager/LTBWidget.h
 panel/panel-plugins/taskmanager/LTaskManagerPlugin.cpp
 panel/panel-plugins/taskmanager/LTaskButton.cpp
 panel/panel-plugins/spacer/LSpacer.h
 panel/panel-plugins/systemtray/LSysTray.cpp
 panel/panel-plugins/systemtray/TrayIcon.cpp
 panel/panel-plugins/clock/LClock.cpp
 panel/panel-plugins/showdesktop/LHomeButton.cpp
	
 #Test
 desktop/desktop-plugins/LDPlugin.cpp
 panel/panel-plugins/LPPlugin.h
 panel/panel-plugins/NewPP.h
 desktop/desktop-plugins/LDPlugin.h
 desktop/desktop-plugins/NewDP.h
)

# Add the dependencies for linking
target_link_libraries(${PROJECT}
 Qt6::Widgets
 Qt6::GuiPrivate
 Qt6::Network
 Qt6::Concurrent
 Qt6::Svg
 ${LUMINA_LIBRARIES}
)

# Include header files
include_directories(
    ${LUMINA_INCLUDE_DIRS}
)

install(TARGETS ${PROJECT} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(FILES "data/start-lumina-desktop" DESTINATION "${CMAKE_INSTALL_BINDIR}")

# XSession
install(FILES "data/Lumina-DE.desktop" DESTINATION "${CMAKE_INSTALL_DATADIR}/xsessions")
# PNG
install(FILES "data/Lumina-DE.png" DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps")
install(FILES "data/Lumina-DE.png" DESTINATION "${CMAKE_INSTALL_DATADIR}/lumina-desktop")
## Defaults
install(FILES "data/defaults/luminaDesktop.conf" DESTINATION "${CMAKE_INSTALL_DATADIR}/lumina-desktop")
## Extrafiles
install(FILES "data/extrafiles/globs2" DESTINATION "${CMAKE_INSTALL_DATADIR}/lumina-desktop")

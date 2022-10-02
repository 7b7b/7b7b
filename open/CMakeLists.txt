cmake_minimum_required (VERSION 3.8.2)
set(PROJECT lumina-open)
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

# Dependencies
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui)
pkg_check_modules(LUMINA REQUIRED lumina)

# Tell CMake to create the executable
add_executable(${PROJECT}
 main.cpp
 LFileDialog.cpp
)

# Add the dependencies for linking
target_link_libraries(${PROJECT}
 Qt6::Widgets
 Qt6::GuiPrivate
 ${LUMINA_LIBRARIES}
)

# Include header files
include_directories(
    ${LUMINA_INCLUDE_DIRS}
)

install(TARGETS ${PROJECT} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

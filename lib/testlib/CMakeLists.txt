cmake_minimum_required(VERSION 3.1.0)

project(test)

find_package(PkgConfig REQUIRED)

pkg_check_modules(QT REQUIRED Qt5Core Qt5Widgets Qt5X11Extras Qt5Network)
pkg_check_modules(LUMINA REQUIRED lumina)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

add_executable(test main.cpp)

target_link_libraries(
    test
    ${QT_LIBRARIES}
    ${LUMINA_LIBRARIES}
)

include_directories(
    ${QT_INCLUDE_DIRS}
    ${LUMINA_INCLUDE_DIRS}
)

install(TARGETS test)

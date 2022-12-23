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
	LDialog.cpp
)

# Add the dependencies for linking
target_link_libraries(${PROJECT}
	Qt6::Widgets
	7b7b
	${7B7B_LIBS}
)

# Include header files
include_directories(
    "${CMAKE_INSTALL_FULL_INCLUDEDIR}/7b7b"
)

install(TARGETS ${PROJECT} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

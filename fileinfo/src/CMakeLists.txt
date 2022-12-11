set(CMAKE_INCLUDE_CURRENT_DIR ON)

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
	MainUI.cpp
)

# Add the dependencies for linking
target_link_libraries(${PROJECT}
	Qt6::Widgets
	Qt6::Concurrent
	Lumina
	${LUMINA_LIBS}

)

# Include header files
include_directories(
	"${CMAKE_INSTALL_FULL_INCLUDEDIR}/Lumina"
)

install(TARGETS ${PROJECT} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

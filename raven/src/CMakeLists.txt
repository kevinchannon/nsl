
cmake_minimum_required(VERSION 3.15)

add_library(libRavenClient
  "udp_output_socket.cpp"
	"udp_input_socket.cpp"
 "targetver.h")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Find Boost libraries
find_package(Boost 1.81.0)

set_property(TARGET libRavenClient PROPERTY CXX_STANDARD 20)

target_compile_options(libRavenClient PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

target_include_directories(libRavenClient PUBLIC
	${CMAKE_SOURCE_DIR}
)

target_link_libraries(libRavenClient
	PRIVATE
		ws2_32
)

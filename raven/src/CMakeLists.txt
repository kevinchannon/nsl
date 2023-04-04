
cmake_minimum_required(VERSION 3.15)


Include(FetchContent)

FetchContent_Declare(
  wite
	GIT_REPOSITORY https://github.com/kevinchannon/wite.git
  GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(wite)

add_library(libRavenClient
  "udp_emitter.cpp"
	"udp_receiver.cpp")

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
  ${wite_SOURCE_DIR}
)

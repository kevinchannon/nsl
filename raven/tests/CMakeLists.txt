
cmake_minimum_required(VERSION 3.15)

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.3.2
)

FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
  wite
	GIT_REPOSITORY https://github.com/kevinchannon/wite.git
  GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(wite)

enable_testing()

add_executable(ravenTest
  "udp_comms.tests.cpp"
  "tftp_client.tests.cpp"
)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Find Boost libraries
find_package(Boost 1.81.0)

set_property(TARGET ravenTest PROPERTY CXX_STANDARD 20)

target_compile_options(ravenTest PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

target_include_directories(ravenTest PRIVATE
  "../src"
	${CMAKE_SOURCE_DIR}
  ${wite_SOURCE_DIR}
)

target_link_libraries(ravenTest
  PRIVATE
    libRavenClient
    Catch2::Catch2WithMain
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)

catch_discover_tests(ravenTest)

add_test(NAME ravenTest COMMAND ravenTest)


cmake_minimum_required(VERSION 3.15)

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.3.2
)

FetchContent_MakeAvailable(Catch2)

enable_testing()

add_executable(ravenTest
  "main.cpp"
  "udp_socket.tests.cpp"
)

set_property(TARGET ravenTest PROPERTY CXX_STANDARD 20)

target_compile_options(ravenTest PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

target_include_directories(ravenTest PRIVATE
  "../src"
	${CMAKE_SOURCE_DIR}
)

target_link_libraries(ravenTest
  PRIVATE
    libRavenClient
    Catch2::Catch2
    ws2_32
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)

catch_discover_tests(ravenTest)

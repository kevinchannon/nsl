
cmake_minimum_required(VERSION 3.15)

Include(FetchContent)

FetchContent_Declare(
  wite
	GIT_REPOSITORY https://github.com/kevinchannon/wite.git
  GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(wite)

enable_testing()

add_executable(nslTest
  "ostream.tests.cpp"
  "istream.tests.cpp"
  "stream.tests.cpp"
)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

# Find Boost libraries
find_package(Boost 1.81.0) 

set_property(TARGET nslTest PROPERTY CXX_STANDARD 20)

target_compile_options(nslTest PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

target_include_directories(nslTest PRIVATE
	${CMAKE_SOURCE_DIR}
  ${wite_SOURCE_DIR}
)

target_link_libraries(nslTest
  PRIVATE
    CONAN_PKG::catch2
    CONAN_PKG::boost
    CONAN_PKG::nlohmann_json
    nsl::nsl
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)

catch_discover_tests(nslTest)

add_test(NAME nslTest COMMAND nslTest)

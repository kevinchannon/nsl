#include "tftp_types.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace raven;

TEST_CASE("Requests tests") {
  SECTION("write_request") {
    SECTION("has op code of 2") {
      REQUIRE(2 == tftp::write_request{""}.op_code);
    }

    SECTION("has the expected filename") {
      REQUIRE("the_filename" == tftp::write_request{"the_filename"}.filename);
    }

    SECTION("throws a std::invalid_argument exception if the filename is too long") {
      const auto long_filename = std::string(504, 'X');
      REQUIRE_THROWS_AS(tftp::write_request{long_filename}, std::invalid_argument);
    }

    SECTION("has a mode of 'octet'") {
      REQUIRE(tftp::write_request::write_mode::octet == tftp::write_request{"foo"}.mode);
    }
  }
}

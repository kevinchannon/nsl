#include "tftp_client.hpp"
#include "tftp_types.hpp"

#include <wite/io.hpp>

#include <string_view>
#include <tuple>

using namespace std::string_view_literals;

namespace raven::tftp {

void client::connect(std::ostream& target, std::string_view filename) {
  target << write_request{filename};
}

}  // namespace raven::tftp

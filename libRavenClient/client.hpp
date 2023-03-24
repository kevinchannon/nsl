#pragma once

#include <string>

namespace raven {

class client {
 public:
  explicit client(std::string ip) : _ip{std::move(ip)} {}

  const std::string& ip() const { return _ip; }
 private:
  std::string _ip;
};

}  // namespace raven

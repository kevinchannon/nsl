#pragma once

#include "framework.h"
#include <boost/asio.hpp>

#include <future>

namespace raven::test {

class io_runner {
 public:
  explicit io_runner(boost::asio::io_context& io) : _io{io}, _result{std::async([this]() { _io.run(); })} {}
  ~io_runner() { _io.stop(); }

 private:
  boost::asio::io_context& _io;
  std::future<void> _result;
};

}  // namespace test

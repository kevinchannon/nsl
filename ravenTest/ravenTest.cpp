#include <client.hpp>

#include "CppUnitTest.h"

#include <string>

using namespace std::string_literals;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace raven_test {

TEST_CLASS(ClientTests){
public : 
  TEST_METHOD(ClientCanBeInstantiated){
    const auto client = raven::client{"0.0.0.0"};
    Assert::AreEqual("0.0.0.0"s, client.ip());
  }  
};

} // namespace ravenTest

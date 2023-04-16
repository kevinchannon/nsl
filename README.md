![Windows | MSVC++ | x64 | Debug](https://github.com/kevinchannon/nsl/actions/workflows/build_and_test_windows_msvc_x64_debug.yml/badge.svg)<br>
![Windows | MSVC++ | x64 | Release](https://github.com/kevinchannon/nsl/actions/workflows/build_and_test_windows_msvc_x64_release.yml/badge.svg)

# Network Streams Library

The Network Streams Library (NSL) is a C++ library that combines boost ASIO sockets with Boost IOStreams to give a natural experience for the user trying to send and receive network data in a C++ application.

NOTE: This work is not complete.
* The implementation of asynchronous sending needs to be finished (the tests fail).
* The tests on Ubuntu|GCC|Debug hang.
* The tests on Ubuntu|GCC|Release don't run because the exe isn't found, for some reason
* There's no way to receive incomming UDP without specifying the port that it's coming in on.

If you're reading this an want to fix these things, then be my guest! Alternatively, if you're reading this and you want to use this library for something but can't because of these issues, then let me know and I'll see what I can do.

## Installation
NSL is header only at the moment, so just copy the files into your repo and off you go!  In time I will add some kind of packaging so that it works with CMake FetchContent, Nuget, Conan, VCPKG, etc. but for now just copy and paste :)

## Prerequisites
This thing requires Boost ASIO and Boost IOStreams (and their dependencies). It also uses C++20 features, so you'll need a compiler that supports that version of the standard.

If you want to build and run the tests, then you will also need the following:
* Catch2
* Nlohmann JSON
* Fmt

## Getting and Building

To get the code, install all the dependencies, build and run the tests, then do:
```bash
# Get the code
git clone git@github.com:kevinchannon/nsl.git
cd nsl

# Install dependencies
conan install . --build=missing -s build_type=Debug --install-folder=out/build/x64

# Build the tests
cmake -B .\out\build\x64
cmake --build .\out\build\x64

# Run the tests
.\out\build\x64\Debug\nslTest.exe
```

If you don't have Conan, then you'll need to do the following before the conan install commands above:
```bash
pip install "conan=1.59.0"
cd <repo root.>
conan profile new default --detect
```

This uses Pip, so if you don't have that, then you'll need to install a Python3 distribution to get Pip.

## Examples
Here are some simple things you can do:

### Sent a string to a remote endpoint
To send things, you'll need the `udp/ostream.hpp` include:
```c++
#include <nsl/udp/ostream.hpp>
```
Say you want to send a string via UDP to some remote host with IP 192.168.2.13 on port 45001:
```c++
auto udp_out = nsl::udp::ostream{"192.168.2.13", 45001};

udp_out << "Hello, easy network comms!" << std::endl;
```
Done! That's it.

### Reveive a string from a remote endpoint
To receive things, you'll need the `udp/istream.hpp` include:
```c++
#include <nsl/udp/istream.hpp>
```
Say you want to receive a string via UDP on port 45001:
```c++

auto io = boost::asio::io_context{};

auto udp_in = nsl::udp::istream{io, 45001};
auto recv_msg = std::string{};
udp_in >> recv_msg;
```
### Send some bytes via UDP
It's important to appreciate that the mechanism shown above is sending *formatted* messages via UDP. So, think about what it would look like if we swapped the UDP streams in the examples for `std::cout`, or something.  This means that, if you want to send a numerical value, like this:
```c++
auto my_int = std::uint32_t{123456};
udp_out << my_int << std::endl;
```
This is actually going to send the values as the *string* `"123456"`. Often, this will be fine (if you're sending JSON messages, for example) but sometimes you just want to send some bytes without formatting them into text. To facilitate this, `operator<<` and `operator>>` are overloaded in NSL to work on range-like things containing byte-like things. So, if I have a `std::vector` of `std::bytes` that I want to send, then I can do that like this:
```c++
auto my_bytes = std::vector<std::byte>{ /* Imagine my bytes are here */ };
udp_out << my_bytes << nsl::udp::flush;
```

So, that's basically how you do that. Notice that the stream is flushed to make sure that the whole thing is send at the point that I expect using `nsl::udp::flush`. This is necessary because IO streams do some buffering into chunks of some size. This is fine, but it means that you should flush the buffer if you 100% want that data to be sent at that exact point in the code.

### Receive some bytes via UDP
Similarly to sending bytes, we can use `operator>>` to receive some bytes:
```c++
auto my_bytes = std::vector<char>(512, '\0');
udp_in >> my_bytes;
```
Note that the range that you're putting the bytes into needs to be resized to the size that you expect to fill. This call will block until it receives that nummber of bytes. This is kind of necessary, because otherwise we don't know how many bytes to wait for.

### Receive data asynchronously
Most commonly, we don't want to block execution on the receipt of some data from some remote endpoint that doesn't know, or care, about the smooth operation of our application. To prevent this, we can receive the data from the endpoint asynchronously. In NSL, we indicate that we want to do this be streaming the incomming data into a function object:
```c++
auto io = boost::asio::io_context{};

// Here's our function that's going to handle the asynchronous arrival of our data.
// In reality, you probably want some kind of synchronisation in here, but for this example
// we're just keeping it simple.
auto recv_data       = std::string{};
auto receive_a_value = [&](auto&& is, size_t n) {
	recv_data.resize(n);
	is.read(recv_data.data(), n);
};

// Make our istream and start receiving the data...
auto udp_in = nsl::udp::istream{io, 45001};
udp_in >> receive_a_value;

// The above operation just put the receive job on the io_context's queue. now we need to start
// running the io_context so that it processes work.
std::async([&io](){ io.run(); });

// Here you'd have your code for doing things with all the lovely data that was coming from your
// remote endpoint...
```
This example is a little more complex than the ones above because you need to make sure that the `boost::asio::io_context` is running and all that. There isn't really a way around that at the moment.

### Send data asynchronously
TODO: Implement this...

### Bidirectional communication
```c++
#include <nsl/udp/stream.hpp>
```
If you want to have some kind of two-way communication via UDP, then you can use `nsl::udp::stream` for that. Here's an example thats communicating with some UDP server using this method:
```c++
auto data_ready      = std::condition_variable{};
auto handle_response = [&](auto&& is, size_t n) {
	auto str = std::string(n, '\0');
	is.read(str.data(), n);
	response = json::parse(str);
	{ auto _ = std::unique_lock{mtx}; }
	data_ready.notify_all();
};

auto remote = udp::stream{io, test_port, "localhost", test_port + 1};
remote >> handle_response;

auto _ = test::io_runner{io};

auto request  = json::parse(R"({"int_field": 12345, "string_field": "ahoy there!"})");
remote << request << std::endl;
data_ready.wait(lock);
```

## TODO list
* Async UDP send
* Work out how to recieve from any remove port, without having to name it in istream constructor
* TCP streams
* Packaging
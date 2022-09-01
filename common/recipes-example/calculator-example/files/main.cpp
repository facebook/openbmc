// SPDX-License-Identifier: Apache-2.0

#include <net/poettering/Calculator/client.hpp>
#include <net/poettering/Calculator/error.hpp>
#include <net/poettering/Calculator/server.hpp>
#include <sdbusplus/server.hpp>

#include <iostream>
#include <string_view>

using Calculator_inherit =
    sdbusplus::server::object_t<sdbusplus::net::poettering::server::Calculator>;

/** Example implementation of net.poettering.Calculator */
struct Calculator : Calculator_inherit
{
    /** Constructor */
    Calculator(sdbusplus::bus_t& bus, const char* path) :
        Calculator_inherit(bus, path)
    {}

    /** Multiply (x*y), update lastResult */
    int64_t multiply(int64_t x, int64_t y) override
    {
        return lastResult(x * y);
    }

    /** Divide (x/y), update lastResult
     *
     *  Throws DivisionByZero on error.
     */
    int64_t divide(int64_t x, int64_t y) override
    {
        using sdbusplus::net::poettering::Calculator::Error::DivisionByZero;
        if (y == 0)
        {
            status(State::Error);
            throw DivisionByZero();
        }

        return lastResult(x / y);
    }

    /** Clear lastResult, broadcast 'Cleared' signal */
    void clear() override
    {
        auto v = lastResult();
        lastResult(0);
        cleared(v);
        return;
    }
};

int main()
{
    // Define a dbus path location to place the object.
    constexpr auto path = "/net/poettering/calculator";

    static_assert(
        std::string_view(
            sdbusplus::net::poettering::client::Calculator::interface) ==
        std::string_view(Calculator::interface));

    // Create a new bus and affix an object manager for the subtree path we
    // intend to place objects at..
    auto b = sdbusplus::bus::new_default();
    sdbusplus::server::manager_t m{b, path};

    // Reserve the dbus service name : net.poettering.Calculator
    b.request_name("net.poettering.Calculator");

    // Create a calculator object at /net/poettering/calculator
    Calculator c1{b, path};

    // Handle dbus processing forever.
    while (1)
    {
        b.process_discard(); // discard any unhandled messages
        b.wait();
    }

    return 0;
}

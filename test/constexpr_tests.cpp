/*  
 *  C++ VCD Tracer Library Tests
 *
 *  For more information see https://github.com/nakane1chome/cpp-vcd-tracer
 *
 * Copyright (c) 2022, Philip Mulholland
 * All rights reserved.
 * 
 * Using the  BSD 3-Clause License
 *
 * See LICENSE for license details.
 */

#include <string_view>

#include <catch2/catch.hpp>
#include "../src/vcd_tracer.hpp"

constexpr bool strings_equal(char const *a, char const *b) {
    return std::string_view(a) == b;
}

constexpr std::array<char, 4> GenerateVcdKey(unsigned int number)// NOLINT(misc-no-recursion)
{
    vcd_tracer::identifier_generator k;
    for (unsigned int i = 0; i < number; i++) {
        k.next();
    }
    // Must return by VALUE
    const char *r = k.next();
    return { r[0], r[1], r[2], r[3] };
}


TEST_CASE("VCD identifiers are created-0 char", "[GenerateVcdKey-0]") {
    STATIC_REQUIRE(std::string_view(GenerateVcdKey(0).data()) == "!");
}

TEST_CASE("VCD identifiers are created-89 char", "[GenerateVcdKey-89]") {
    STATIC_REQUIRE(std::string_view(GenerateVcdKey(89).data()) == "z");
}

TEST_CASE("VCD identifiers are created-90 char", "[GenerateVcdKey-90]") {
    STATIC_REQUIRE(std::string_view(GenerateVcdKey(90).data()) == "!!");
}

TEST_CASE("VCD identifiers are created-180 char", "[GenerateVcdKey-180]") {
    STATIC_REQUIRE(std::string_view(GenerateVcdKey(180).data()) == "\"!");
}

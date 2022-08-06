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

#include <iostream>
#include <sstream>

#include <catch2/catch.hpp>
#include "../src/vcd_tracer.hpp"

// See https://en.wikipedia.org/wiki/Value_change_dump

std::string GenerateVcdKey(unsigned int number)// NOLINT(misc-no-recursion)
{
    vcd_tracer::identifier_generator k;
    std::string key;
    for (unsigned int i = 0; i < number; i++) {
        key = k.next();
    }
    key = k.next();
    return key;
}


TEST_CASE("VCD identifiers are created", "[GenerateVcdKey]") {
    REQUIRE(GenerateVcdKey(0) == "!");
    REQUIRE(GenerateVcdKey(8) == ")");
    REQUIRE(GenerateVcdKey(89) == "z");
    REQUIRE(GenerateVcdKey(90) == "!!");
    REQUIRE(GenerateVcdKey(91) == "!\"");
    REQUIRE(GenerateVcdKey(179) == "!z");
    REQUIRE(GenerateVcdKey(180) == "\"!");
}

TEST_CASE("VCD Integer Value", "VcdValue") {

    vcd_tracer::scope_fn::dumper_fn my_dumper = vcd_tracer::scope_fn::nop_dump;
    unsigned int my_bit_size = 88888;
    std::string my_full_path;

    auto update_fn = [&](vcd_tracer::scope_fn::dumper_fn fn) {
        my_dumper = fn;
    };

    auto add_fn = [&](const std::string_view full_path,
                      const std::string_view var_type,
                      unsigned int bit_size,
                      vcd_tracer::scope_fn::dumper_fn fn) {
        (void)var_type;
        my_full_path = full_path;
        my_dumper = fn;
        my_bit_size = bit_size;
        return vcd_tracer::value_context{ "vv", update_fn };
    };

    {
        vcd_tracer::value<int, 9> test_var(add_fn, "Path.To.Var");

        REQUIRE(my_bit_size == 9);
        REQUIRE(my_full_path == "Path.To.Var");

        {
            std::ostringstream dump_out;
            (void)my_dumper(dump_out, true);
            REQUIRE(dump_out.str() == "bx vv\n");
        }

        test_var.set(0x155);

        {
            std::ostringstream dump_out;
            (void)my_dumper(dump_out, true);
            REQUIRE(dump_out.str() == "b101010101 vv\n");
        }

        test_var.set(0x0AA);

        {
            std::ostringstream dump_out;
            (void)my_dumper(dump_out, true);
            REQUIRE(dump_out.str() == "b010101010 vv\n");
        }

        test_var.undriven();

        {
            std::ostringstream dump_out;
            (void)my_dumper(dump_out, true);
            REQUIRE(dump_out.str() == "bz vv\n");
        }
    }

    // Destructor will set dumper to nop function
    {
        std::ostringstream dump_out;
        (void)my_dumper(dump_out, true);
        REQUIRE(dump_out.str() == "");
    }

    {
        vcd_tracer::value<unsigned int, 15> test_var(add_fn, "Path.To.Var2", 0x4242);

        REQUIRE(my_bit_size == 15);
        REQUIRE(my_full_path == "Path.To.Var2");

        {
            std::ostringstream dump_out;
            (void)my_dumper(dump_out, true);
            REQUIRE(dump_out.str() == "b100001001000010 vv\n");
        }
    }


    {
        vcd_tracer::value<bool> test_var(add_fn, "Path.To.Var3");

        REQUIRE(my_bit_size == 1);
        REQUIRE(my_full_path == "Path.To.Var3");

        {
            std::ostringstream dump_out;
            (void)my_dumper(dump_out, true);
            REQUIRE(dump_out.str() == "xvv\n");
        }

        test_var.set(true);

        {
            std::ostringstream dump_out;
            (void)my_dumper(dump_out, true);
            REQUIRE(dump_out.str() == "1vv\n");
        }

        test_var.set(false);

        {
            std::ostringstream dump_out;
            (void)my_dumper(dump_out, true);
            REQUIRE(dump_out.str() == "0vv\n");
        }

        test_var.undriven();

        {
            std::ostringstream dump_out;
            (void)my_dumper(dump_out, true);
            REQUIRE(dump_out.str() == "zvv\n");
        }
    }

    {
        vcd_tracer::value<float> test_var(add_fn, "Path.To.Var4", 0.001f);

        REQUIRE(my_bit_size == 32);
        REQUIRE(my_full_path == "Path.To.Var4");


        {
            char tmp[64];
            std::ostringstream dump_out;
            (void)my_dumper(dump_out, true);
            snprintf(tmp, sizeof(tmp), "r%.16g vv\n", static_cast<double>(0.001f));
            REQUIRE(dump_out.str() == tmp);
        }

        test_var.set(10000000000000000.0f);

        {
            char tmp[64];
            std::ostringstream dump_out;
            (void)my_dumper(dump_out, true);
            snprintf(tmp, sizeof(tmp), "r%.16g vv\n", static_cast<double>(10000000000000000.0f));
            REQUIRE(dump_out.str() == tmp);
        }
    }

    {
        vcd_tracer::value<int, 11> test_var;

        {
            std::ostringstream dump_out;
            (void)my_dumper(dump_out, true);
            REQUIRE(dump_out.str() == "");
        }

        test_var.elaborate(add_fn, "Path.To.Var5");

        REQUIRE(my_bit_size == 11);
        REQUIRE(my_full_path == "Path.To.Var5");

        {
            std::ostringstream dump_out;
            (void)my_dumper(dump_out, true);
            REQUIRE(dump_out.str() == "bx vv\n");
        }

        test_var.set(0x355);

        {
            std::ostringstream dump_out;
            (void)my_dumper(dump_out, true);
            REQUIRE(dump_out.str() == "b01101010101 vv\n");
        }
    }

    {
        vcd_tracer::value<unsigned int, 17> test_var(0x1DEAD);

        {
            std::ostringstream dump_out;
            (void)my_dumper(dump_out, true);
            REQUIRE(dump_out.str() == "");
        }

        test_var.elaborate(add_fn, "Path.To.Var6");

        REQUIRE(my_bit_size == 17);
        REQUIRE(my_full_path == "Path.To.Var6");

        {
            std::ostringstream dump_out;
            (void)my_dumper(dump_out, true);
            // 2 LSB are compressed
            REQUIRE(dump_out.str() == "b101111010101101 vv\n");
        }

        test_var.set(0x0);

        {
            std::ostringstream dump_out;
            (void)my_dumper(dump_out, true);
            REQUIRE(dump_out.str() == "b0 vv\n");
        }
    }
}


TEST_CASE("VCD Trace Buffer", "VcdTraceBuffer") {


    static vcd_tracer::scope_fn::sequence_t seq = 42;


    vcd_tracer::scope_fn::dumper_fn my_dumper = vcd_tracer::scope_fn::nop_dump;// Default value - overwritten

    unsigned int my_bit_size = 88888;
    std::string my_full_path;

    auto update_fn = [&](vcd_tracer::scope_fn::dumper_fn fn) {
        my_dumper = fn;
    };

    auto add_fn = [&](const std::string_view full_path,
                      const std::string_view var_type,
                      unsigned int bit_size,
                      vcd_tracer::scope_fn::dumper_fn fn) {
        (void)var_type;
        my_full_path = full_path;
        my_dumper = fn;
        my_bit_size = bit_size;
        return vcd_tracer::value_context{ "vv", update_fn };
    };

    vcd_tracer::value<int, 9, 10, &seq> test_var(add_fn, "Path.To.Var");

    REQUIRE(my_bit_size == 9);
    REQUIRE(my_full_path == "Path.To.Var");

    test_var.set(1);
    seq++;
    test_var.set(2);
    seq++;
    test_var.set(3);
    seq++;
    test_var.set(4);
    seq++;
    test_var.set(5);
    seq++;
    test_var.set(0);
    seq++;

    {
        std::ostringstream dump_out;
        auto first_status = my_dumper(dump_out, true);
        REQUIRE(first_status.next.value() == 42);
        for (unsigned int i = 0; i < 6; i++) {
            auto status = my_dumper(dump_out, false);
            if (i == 5) {
                // No more values
                REQUIRE(status.next.has_value() == false);
            }
            else {
                REQUIRE(status.next.has_value() == true);
                if (status.next.has_value()) {
                    // The next sequence - first there is no dump - just the sequence
                    REQUIRE(status.next.value() == 42 + i + 1);
                }
            }
        }
        REQUIRE(dump_out.str() == "b01 vv\nb010 vv\nb011 vv\nb0100 vv\nb0101 vv\nb0 vv\n");
    }
}


TEST_CASE("VCD Module", "VcdModule") {

    vcd_tracer::scope_fn::dumper_fn my_dumper = vcd_tracer::scope_fn::nop_dump;
    std::string my_full_path;

    auto update_fn = [&](vcd_tracer::scope_fn::dumper_fn fn) {
        my_dumper = fn;
    };

    auto register_fn = [&](const std::string_view full_path,
                           vcd_tracer::scope_fn::dumper_fn fn) {
        my_full_path = full_path;
        my_dumper = fn;
        return vcd_tracer::value_context{ std::string(full_path), update_fn };
    };


    vcd_tracer::module root(register_fn, "root");
    vcd_tracer::module mod1(root, "mod1");
    vcd_tracer::module mod2(root, "mod2");
    vcd_tracer::module submod_a(mod1, "submod_a");
    vcd_tracer::module submod_b(mod1, "submod_b");
    vcd_tracer::module submod_c(mod2, "submod_c");

    vcd_tracer::value<float> root_var_ka(root.get_add_fn(), "ka");
    vcd_tracer::value<double> mod1_var_ki(mod1.get_add_fn(), "ki");
    vcd_tracer::value<bool> mod2_var_ku(mod2.get_add_fn(), "ku");
    vcd_tracer::value<std::uint32_t> submod_a_var_ke(submod_a.get_add_fn(), "ke");
    vcd_tracer::value<std::int16_t> submod_c_var_ko(submod_c.get_add_fn(), "ko");

    std::ostringstream header;
    root.finalize_header(header);
    static constexpr const char *EXPECTED_HEADER =
        "$scope module root $end\n"
        "$var real 32 root.ka ka $end\n"
        "$scope module mod1 $end\n"
        "$var real 64 root.mod1.ki ki $end\n"
        "$scope module submod_a $end\n"
        "$var wire 32 root.mod1.submod_a.ke ke $end\n"
        "$upscope $end\n"
        "$scope module submod_b $end\n"
        "$upscope $end\n"
        "$upscope $end\n"
        "$scope module mod2 $end\n"
        "$var wire 1 root.mod2.ku ku $end\n"
        "$scope module submod_c $end\n"
        "$var wire 16 root.mod2.submod_c.ko ko $end\n"
        "$upscope $end\n"
        "$upscope $end\n"
        "$upscope $end\n";

    REQUIRE(header.str() == EXPECTED_HEADER);
}


TEST_CASE("VCD Module Elaborate", "VcdModuleElaborate") {

    vcd_tracer::scope_fn::dumper_fn my_dumper = vcd_tracer::scope_fn::nop_dump;
    std::string my_full_path;

    auto update_fn = [&](vcd_tracer::scope_fn::dumper_fn fn) {
        my_dumper = fn;
    };

    auto register_fn = [&](const std::string_view full_path,
                           vcd_tracer::scope_fn::dumper_fn fn) {
        my_full_path = full_path;
        my_dumper = fn;
        return vcd_tracer::value_context{ std::string(full_path), update_fn };
    };


    vcd_tracer::module root(register_fn, "root");
    vcd_tracer::module mod1(root, "mod1");
    vcd_tracer::module mod2(root, "mod2");
    vcd_tracer::module submod_a(mod1, "submod_a");
    vcd_tracer::module submod_b(mod1, "submod_b");
    vcd_tracer::module submod_c(mod2, "submod_c");

    vcd_tracer::value<float> root_var_ka;
    vcd_tracer::value<double> mod1_var_ki;
    vcd_tracer::value<bool> mod2_var_ku;
    vcd_tracer::value<std::uint32_t> submod_a_var_ke;
    vcd_tracer::value<std::int16_t> submod_c_var_ko;

    root.elaborate(root_var_ka, "ka");
    mod1.elaborate(mod1_var_ki, "ki");
    mod2.elaborate(mod2_var_ku, "ku");
    submod_a.elaborate(submod_a_var_ke, "ke");
    submod_c.elaborate(submod_c_var_ko, "ko");

    std::ostringstream header;
    root.finalize_header(header);
    static constexpr const char *EXPECTED_HEADER =
        "$scope module root $end\n"
        "$var real 32 root.ka ka $end\n"
        "$scope module mod1 $end\n"
        "$var real 64 root.mod1.ki ki $end\n"
        "$scope module submod_a $end\n"
        "$var wire 32 root.mod1.submod_a.ke ke $end\n"
        "$upscope $end\n"
        "$scope module submod_b $end\n"
        "$upscope $end\n"
        "$upscope $end\n"
        "$scope module mod2 $end\n"
        "$var wire 1 root.mod2.ku ku $end\n"
        "$scope module submod_c $end\n"
        "$var wire 16 root.mod2.submod_c.ko ko $end\n"
        "$upscope $end\n"
        "$upscope $end\n"
        "$upscope $end\n";

    REQUIRE(header.str() == EXPECTED_HEADER);
}

TEST_CASE("VCD Top", "VcdTop") {

    vcd_tracer::top dumper("root");

    vcd_tracer::module mod1(dumper.root, "mod1");
    vcd_tracer::value<bool> mod1_var(mod1.get_add_fn(), "flag");

    {

        static constexpr const char *EXPECTED_HEADER =
            "$date\n"
            "   Thu Jan  1 00:00:00 1970\n"
            "$end\n"
            "$timescale\n"
            "   1ns\n"
            "$end\n"
            "$version\n"
            "   C++ Simple VCD Logger\n"
            "$end\n"
            "$scope module root $end\n"
            "$scope module mod1 $end\n"
            "$var wire 1 ! flag $end\n"
            "$upscope $end\n"
            "$upscope $end\n"
            "$enddefinitions $end\n"
            "#0\n"
            "x!\n";

        std::ostringstream header;
        dumper.finalize_header(header, std::chrono::system_clock::from_time_t(0));
        REQUIRE(header.str() == EXPECTED_HEADER);
    }
}


TEST_CASE("VCD Top Trace Buf", "VcdTopTraceBuf") {

    static vcd_tracer::scope_fn::sequence_t seq = 42;

    vcd_tracer::top dumper("root");

    vcd_tracer::module mod1(dumper.root, "mod1");

    vcd_tracer::value<int, 9, 10, &seq> var_1;
    vcd_tracer::value<int, 11, 12, &seq> var_2;

    mod1.elaborate(var_1, "ka");
    mod1.elaborate(var_2, "ki");


    {

        static constexpr const char *EXPECTED_HEADER =
            "$date\n"
            "   Thu Jan  1 00:00:00 1970\n"
            "$end\n"
            "$timescale\n"
            "   1ns\n"
            "$end\n"
            "$version\n"
            "   C++ Simple VCD Logger\n"
            "$end\n"
            "$scope module root $end\n"
            "$scope module mod1 $end\n"
            "$var wire 9 ! ka $end\n"
            "$var wire 11 \" ki $end\n"
            "$upscope $end\n"
            "$upscope $end\n"
            "$enddefinitions $end\n"
            "#0\n";

        std::ostringstream header;
        dumper.finalize_header(header, std::chrono::system_clock::from_time_t(0));
        REQUIRE(header.str() == EXPECTED_HEADER);

        std::ostringstream data;
        std::ostringstream edata;

        // Write 2 values to var1, 1 value to var2
        var_1.set(0x11);
        edata << "b010001 !\n";
        seq++;

        edata << "#1\n";
        var_1.set(0x12);
        edata << "b010010 !\n";
        seq++;

        edata << "#2\n";
        var_2.set(0x21);
        edata << "b0100001 \"\n";
        seq++;

        edata << "#3\n";
        var_2.set(0x22);
        edata << "b0100010 \"\n";
        seq++;

        edata << "#4\n";
        var_1.set(0x13);
        edata << "b010011 !\n";
        seq++;

        edata << "#5\n";
        var_1.set(0x14);
        edata << "b010100 !\n";
        seq++;

        edata << "#6\n";
        var_2.set(0x23);
        edata << "b0100011 \"\n";

        edata << "#10\n";


        // Expect the trace to be sorted correctly
        dumper.time_update_abs(data, std::chrono::nanoseconds{ 10 });

        REQUIRE(data.str() == edata.str());
    }
}

/*  
 *  C++ VCD Tracer Library Fuzz tests
 *
 *  For more information see https://github.com/nakane1chome/simple-vcd
 *
 * Copyright (c) 2022, Philip Mulholland
 * All rights reserved.
 * 
 * Using the  BSD 3-Clause License
 *
 * See LICENSE for license details.
 *
 * NOTE: THESE ARE NOT PASSING
 *
 */


#include <algorithm>
#include <cstring>
#include <sstream>

#include "../src/vcd_tracer.hpp"

static vcd_tracer::scope_fn::sequence_t seq = 42;

struct FuzzData {
    
    vcd_tracer::top dumper{"root"};
    
    vcd_tracer::value<uint8_t, 5, 10, &seq> var_1;
    vcd_tracer::value<uint16_t, 14, 620, &seq> var_2;
    vcd_tracer::value<uint32_t, 28, 5, &seq> var_3;
    vcd_tracer::value<uint64_t, 57, 77, &seq> var_4;

    std::ostringstream trace_data;

    FuzzData(void) {

        dumper.root.elaborate(var_1, "a");
        dumper.root.elaborate(var_2, "sixteen_bits_trace_var");
        dumper.root.elaborate(var_3, "word");
        dumper.root.elaborate(var_4, "big_trace_var");

        dumper.finalize_header(trace_data, std::chrono::system_clock::from_time_t(0));

        
    }

};


// Fuzzer that attempts to invoke undefined behavior for signed integer overflow
// cppcheck-suppress unusedFunction symbolName=LLVMFuzzerTestOneInput
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {

    static FuzzData fuzz_trace;
    size_t i = 0;
    while (i < size) {
        const uint8_t cmd = data[i] % 9;
        i++;
        switch (cmd) {
        case 0:
            fuzz_trace.var_1.unknown();
            break;
        case 1:
            if (i + 1 > size) { return 0; }
            fuzz_trace.var_1.set(data[i++]);
            break;
        case 2: {
            if (i + 2 > size) { return 0; }
            uint16_t v = 0;
            std::memcpy(&v, &data[i], sizeof(v));
            i += 2;
            fuzz_trace.var_2.set(v);
            break;
        }
        case 3:
            fuzz_trace.var_2.unknown();
            break;
        case 4: {
            if (i + 4 > size) { return 0; }
            uint32_t v = 0;
            std::memcpy(&v, &data[i], sizeof(v));
            i += 4;
            fuzz_trace.var_3.set(v);
            break;
        }
        case 5:
            fuzz_trace.var_3.unknown();
            break;
        case 6:
            fuzz_trace.var_4.unknown();
            break;
        case 7:
            fuzz_trace.var_2.undriven();
            break;
        case 8: {
            if (i + 8 > size) { return 0; }
            uint64_t v = 0;
            std::memcpy(&v, &data[i], sizeof(v));
            i += 8;
            fuzz_trace.var_4.set(v);
            break;
        }
        }
        seq++;
    }
    return 0;
}

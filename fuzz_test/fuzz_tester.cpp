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


#include <sstream>
#include <random>
#include <algorithm>

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
    while (size > i) {
        size_t take_bytes = std::min(static_cast<size_t>(data[i] % 9), size-i);
        i++;
        switch (take_bytes) {
        case 0:
            fuzz_trace.var_1.unknown();
            break;
        case 1:
            fuzz_trace.var_1.set(data[i++]);
            break;
        case 2: {
            uint16_t v = reinterpret_cast<const uint16_t*>(&data[i])[0];
            i+=2;
            fuzz_trace.var_2.set(v);
            break;
        }
        case 3: 
            fuzz_trace.var_2.unknown();      
            break;
        case 4: {
            uint32_t v = reinterpret_cast<const uint32_t*>(&data[i])[0];
            i+=4;
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
            uint64_t v = reinterpret_cast<const uint64_t*>(&data[i])[0];
            i+=8;
            fuzz_trace.var_4.set(v);
            break;
        }
        }
        seq++;
    }
    //fuzz_trace;
    return 0;
}
